/**
 *  Copyright 2017-2018, Pavel Kraynyukhov <pavel.kraynyukhov@gmail.com>
 * 
 *  This file is a part of LAppS (Lua Application Server).
 *  
 *  LAppS is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  LAppS is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with LAppS.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  $Id: IOWorker.h April 11, 2018 4:23 PM $
 * 
 **/


#ifndef __IOWORKER_H__
#  define __IOWORKER_H__

#include <abstract/IView.h>
#include <EventBus.h>
#include <WebSocket.h>
#include <ePollController.h>
#include <Shakespeer.h>
#include <abstract/Worker.h>

namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false> 
    class IOWorker :public ::abstract::Worker,
                    public ::itc::abstract::IView<LAppS::EBUS::Event>
  {
    public:
      typedef WebSocket<TLSEnable,StatsEnable> WSType;
      typedef std::shared_ptr<WSType>          WSSPtr;
     
    explicit IOWorker(const size_t id, const size_t maxConnections)
    : Worker(id,maxConnections), mMayRun(true),mCanStop(false),
      mConnectionsMutex(), mInboundMutex(), mOutMutex(),
      mShakespeer(),
      mEPollThr(std::make_shared<LAppS::ePollController>(1000)),
      mEPoll(mEPollThr.getRunnable()), mInboundConnections(),
      mConnections(),mOutEvents(),mEvents(),mInBuffer(8192)
    {
    }
    
    void onUpdate(const ::itc::TCPListener::value_type& sockt)
    {
      newConnection(sockt);
    }
    
    auto getEPollController() const
    {
      return mEPoll;
    }
    
    IOWorker()=delete;
    IOWorker(const IOWorker&)=delete;
    IOWorker(IOWorker&)=delete;
    
    void onBatchUpdate(const std::vector<LAppS::EBUS::Event>& events)
    {
      mEvents.bachLock();
      for(auto i : events)
      {
        mEvents.unsecureBatchPush(i);
      }
      mEvents.batchUnLock();
    }
    
    void onUpdate(const LAppS::EBUS::Event& event)
    {
      mEvents.push(event);
    }
    
    void newConnection(const itc::CSocketSPtr& socketsptr)
    {
      SyncLock sync(mInboundMutex);
      mInboundConnections.push(socketsptr);
      mEvents.push({LAppS::EBUS::NEW,0});
    }
    
    const size_t getConnectionsCount() const
    {
      SyncLock sync(mConnectionsMutex);
      return mConnections.size();
    }
    
    void execute()
    {
      sigset_t sigpipe_mask;
      sigemptyset(&sigpipe_mask);
      sigaddset(&sigpipe_mask, SIGPIPE);
      sigset_t saved_mask;
      pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask);
      while(mMayRun)
      {
        mEvents.onEvent(
          [this](const LAppS::EBUS::Event& event)
          {
            switch(event.event)
            {
              case LAppS::EBUS::ERROR: // highest prio
                deleteConnection(event.value);
                break;
              case LAppS::EBUS::IN: // High prio
                processINEvents(event.value);
                break;
              case LAppS::EBUS::OUT: // Mid prio
                processOutEvents(event.value);
                break;
              case LAppS::EBUS::NEW: // Low prio
                processNewConnections();
                break;
            }
          }
        );
        // enablePollForConnections();
      }
      mCanStop.store(true);
    }
    
    void shutdown()
    {
      mMayRun.store(false);
      while(!mCanStop)
      {
        sched_yield();
      }
    }
    void onCancel()
    {
      this->shutdown();
    }
    ~IOWorker()
    {
      if(!mCanStop) this->shutdown();
    }
    
    const WorkerStats& getStats() const
    {
      return mStats;
    }
    
    void updateStats()
    {
      
    }
    
    void setEPollController(const LAppS::ePollControllerSPtrType& ref)
    {
      mEPoll=ref;
    }
    void submitResponse(const TaggedEvent& event)
    {
      SyncLock sync(mConnectionsMutex);
      auto it=mConnections.find(event.sockfd);
      if(it!=mConnections.end())
      {
        auto current=it->second;
        if(current)
        {
          if((current->getState() == WSType::MESSAGING)&&(current->isValid()))
          {
            current->enqueueOutMessage(event);
            mEvents.push({LAppS::EBUS::OUT,current->getFileDescriptor()});
          }
          else
          {
            mConnections.erase(it);
          }
        }
      }
    }
    
    private:
      itc::utils::Bool2Type<TLSEnable>          enableTLS;
      itc::utils::Bool2Type<StatsEnable>        enableStatsUpdate;
      std::atomic<bool>                         mMayRun;
      std::atomic<bool>                         mCanStop;
      std::mutex                                mConnectionsMutex;
      std::mutex                                mInboundMutex;
      std::mutex                                mOutMutex;
      
      LAppS::Shakespeer<TLSEnable,StatsEnable>  mShakespeer;
      LAppS::ePollControllerThread              mEPollThr;
      LAppS::ePollControllerSPtrType            mEPoll;
      
      
      std::queue<itc::CSocketSPtr>              mInboundConnections;
      std::map<int,WSSPtr>                      mConnections;
      std::queue<TaggedEvent>                   mOutEvents;
      LAppS::EBUS::EventBus                     mEvents;
      std::vector<uint8_t>                      mInBuffer;
      
      
      /**
       * @brief processing all inbound connections in bulk.
       **/
      void processNewConnections()
      {
        SyncLock sync(mInboundMutex);
        if(!mInboundConnections.empty())
        {
          if(mEPoll)
          {
            SyncLock connSync(mConnectionsMutex);
            while(!mInboundConnections.empty())
            {
              auto mSocketSPtr=mInboundConnections.front();
              mInboundConnections.pop();
              try{
                std::string peer_addr;
                mSocketSPtr->getpeeraddr(peer_addr);
              }catch(const std::exception& e)
              {
                itc::getLog()->error(
                  __FILE__,__LINE__,
                  "New inbound connection is invalid, can't get peer IP"
                );
                continue;
              }
              auto current=mkWebSocket(mSocketSPtr,enableTLS);
              current->setWorkerId(this->getID());
              mConnections.emplace(current->getFileDescriptor(),current);
              mEPoll->add(current->getFileDescriptor());
            }
          }else{
            mMayRun.store(false);
            itc::getLog()->info(__FILE__,__LINE__,"On IOWorker::processNewConnections(), - the ePollController is down. Going down as well");
          }
        }
      }
      
      void processOutEvents(const int fd)
      {
        SyncLock sync(mConnectionsMutex); 
        auto it=mConnections.find(fd);
        if(it!=mConnections.end())
        {
          auto current=it->second;
          if(current->getState()==WSType::MESSAGING)
          {
            if(!current->sendNext())
            {
              itc::getLog()->error(
                __FILE__,__LINE__,
                "Can't send next outstanding message to peer with fd %d. Communication error. Removing this connection.",fd
              );
              mConnections.erase(it);
            }
          }
          else
          {
            itc::getLog()->error(
              __FILE__,__LINE__,
              "processOutEvents(%d) - socket is in a wrong state, closing connection.",
              fd
            );
            mConnections.erase(it);
          }
        }else{
          itc::getLog()->error(
            __FILE__,__LINE__,
            "processOutEvents(%d) - connection is already closed.",
            fd
          );
        }
      }
      
      void processINEvents(const int fd)
      {
        SyncLock sync(mConnectionsMutex);
        auto it=mConnections.find(fd);
        if(it!=mConnections.end())
        {
          auto current=it->second;
          switch(current->getState())
          {
            case WSType::HANDSHAKE:
              mShakespeer.handshake(current);
              if(current->getState()==WSType::CLOSED)
              {
                deleteConnection(fd);
              }else{
                mEPoll->mod(fd);
              }
            break;
            case WSType::MESSAGING:
              handleInput(current);
              if(current->getState()==WSType::CLOSED)
              {
                deleteConnection(fd);
              }
              else{
                mEPoll->mod(fd);
              }
            break;
            case WSType::CLOSED:
                deleteConnection(fd);
            break;
          }
        }
      }

      void handleInput(const WSSPtr& current)
      {
        int received=current->recv(mInBuffer,MSG_NOSIGNAL);
        if(received == -1)
        {
          if(errno != EAGAIN) // in case we would use non blocking io
          {
            current->setState(WSType::CLOSED);
          }
        }else if(received > 0)
        {
          current->processInput(mInBuffer,received);
        }
      }
      void deleteConnection(const int32_t fd)
      {
        try
        {
          mEPoll->del(fd);
        }catch(const std::exception& e)
        {
          // do nothing here; It is not a problem that epoll has
          // already invalidated this fd.
        }
        auto it=mConnections.find(fd);
        if(it!=mConnections.end())
        {
          mConnections.erase(it);
        }
      }
      
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<false> tls_is_disabled)
      {
        return std::make_shared<WSType>(inbound);
      }
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<true> tls_is_enabled)
      {
        auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
        if(tls_server_context)
          return std::make_shared<WSType>(inbound,tls_server_context);
        else throw std::system_error(EINVAL,std::system_category(),"TLS ServerContext is NULL");
      }
  };
}

typedef itc::sys::CancelableThread<abstract::Worker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;

#endif /* __IOWORKER_H__ */

