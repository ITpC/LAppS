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
    private:
      itc::utils::Bool2Type<TLSEnable>          enableTLS;
      itc::utils::Bool2Type<StatsEnable>        enableStatsUpdate;
      
      std::atomic<bool>                         mMayRun;
      std::atomic<bool>                         mCanStop;
      std::mutex                                mConnectionsMutex;
      std::mutex                                mInboundMutex;
      
      LAppS::Shakespeer<TLSEnable,StatsEnable>  mShakespeer;
      LAppS::ePollControllerThread              mEPollThr;
      LAppS::ePollControllerSPtrType            mEPoll;
      
      
      std::queue<itc::CSocketSPtr>              mInboundConnections;
      std::map<int,WSSPtr>                      mConnections;
      typename WSType::SharedEventBus           mEvents;
      
    public:
     
    explicit IOWorker(const size_t id, const size_t maxConnections,const bool af)
    : Worker(id,maxConnections,af), ::itc::abstract::IView<LAppS::EBUS::Event>(),
      enableTLS(), enableStatsUpdate(), mMayRun(true), mCanStop(false),
      mConnectionsMutex(), mInboundMutex(), mShakespeer(),
      mEPollThr(std::make_shared<LAppS::ePollController>(1000)),
      mEPoll(mEPollThr.getRunnable()), mInboundConnections(),
      mConnections(), mEvents(std::make_shared<LAppS::EBUS::EventBus>())
    {
      mConnections.clear();
    }
    
    IOWorker()=delete;
    IOWorker(const IOWorker&)=delete;
    IOWorker(IOWorker&)=delete;
    
    void onUpdate(const ::itc::TCPListener::value_type& socketsptr)
    {
      SyncLock sync(mInboundMutex);
      mInboundConnections.push(socketsptr);
      mEvents->push({LAppS::EBUS::NEW,socketsptr->getfd()});
    }
    
    void onUpdate(const std::vector<::itc::TCPListener::value_type>& socketsptr)
    {
      SyncLock sync(mInboundMutex);
      std::vector<LAppS::EBUS::Event> batch(socketsptr.size(),{LAppS::EBUS::NEW,0});
      for(size_t i=0;i<socketsptr.size();++i)
      {
        mInboundConnections.push(std::move(socketsptr[i]));
      }
      mEvents->push(batch);
    }
    
    void onUpdate(const LAppS::EBUS::Event& event)
    {
      mEvents->push(event);
    }
    
    void onUpdate(const std::vector<LAppS::EBUS::Event>& event)
    {
      mEvents->push(event);
    }
    
    auto getEPollController() const
    {
      return mEPoll;
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
        mEvents->onEvent(
          [this](const LAppS::EBUS::Event& event)
          {
            this->updateStats();
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
      SyncLock sync(mConnectionsMutex);
      mConnections.clear();
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
      mStats.mConnections=mConnections.size();
      mStats.mEventQSize=mEvents->size();
    }
    
    void setEPollController(const LAppS::ePollControllerSPtrType& ref)
    {
      mEPoll=ref;
    }
    
    void submitResponse(const int fd, std::queue<MSGBufferTypeSPtr>& messages)
    {
      SyncLock sync(mConnectionsMutex);
      auto it=mConnections.find(fd);
      if(it!=mConnections.end())
      {
        auto current=it->second;
        if(current)
        {
          if(current->getState() == WSType::MESSAGING)
          {
            current->pushOutMessage(messages);
          }
          else
          {
            mConnections.erase(it);
          }
        }else
        {
         deleteConnection(fd);
        }
      }else{
        try{
          mEPoll->del(fd);
        }catch(const std::exception& e)
        {
          // ignore
        }
      }
    }
    
    void submitResponse(const int fd, const MSGBufferTypeSPtr& msg)
    {
      SyncLock sync(mConnectionsMutex);
      auto it=mConnections.find(fd);
      if(it!=mConnections.end())
      {
        auto current=it->second;
        if(current)
        {
          if(current->getState() == WSType::MESSAGING)
          {
            current->pushOutMessage(msg);
          }
          else
          {
            mConnections.erase(it);
          }
        }
      }
    }
    
    private:
      /**
       * @brief processing all inbound connections in bulk.
       **/
      void processNewConnections()
      {
        SyncLock connSync(mConnectionsMutex);
        SyncLock sync(mInboundMutex);
        
        std::string peer;
        
        while(mEPoll&&(!mInboundConnections.empty()))
        {
          auto current=mkWebSocket(std::move(mInboundConnections.front()));
          int fd=current->getfd();
          mInboundConnections.pop();
          
          try{    
            current->getPeerIP(peer);
            itc::getLog()->info(__FILE__,__LINE__,"New inbound connection from %s with fd %d",peer.c_str(),fd);
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__,__LINE__,"Can't process new connection because of network error %s", e.what());
            return;
          }
          
          // check if it is already in mConnections:
          auto it=mConnections.find(fd);
          if(it==mConnections.end())
          {
            mConnections.emplace(fd,current);
            current->setWorkerId(this->getID());
            mEPoll->add(fd);
          }
          else
          {
            // drop connection otherwise.
            itc::getLog()->error(__FILE__,__LINE__,"Dropping connection %s because of non-unique fd",peer.c_str());
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
              std::string peer_ip;
              try{
                current->getPeerIP(peer_ip);
                itc::getLog()->error(
                  __FILE__,__LINE__,
                  "Can't send next outstanding message to peer %s with fd %d. Communication error. Removing this connection.",
                  peer_ip.c_str(),fd
                );
              }catch(const std::exception& e)
              {
                itc::getLog()->error(
                  __FILE__,__LINE__,
                  "Exception %s, on attempt to acquire peer's ip address. Root cause: peer closed connection.",
                  e.what()
                );
              }
              mConnections.erase(it);
            }
          }
          else
          {
            itc::getLog()->error(
              __FILE__,__LINE__,
              "processOutEvents(%d) - socket is in a wrong state, removing connection.",
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
              if(mConnections.size()<mMaxConnections)
              {
                mShakespeer.handshake(current);
                if(current->getState()==WSType::MESSAGING)
                {
                  mEPoll->mod(fd);
                }
              }
              else
              {
                mShakespeer.sendForbidden(current);
              }
              
              if(current->getState()==WSType::CLOSED)
              {
                deleteConnection(fd);
              }
              
            break;
            case WSType::MESSAGING:
              if(!current->handleInput())
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
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound)
      {
        return mkWebSocket(inbound, enableTLS);
      }
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<false> tls_is_disabled)
      {
        return std::make_shared<WSType>(inbound,mEvents);
      }
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<true> tls_is_enabled)
      {
        auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
        if(tls_server_context)
          return std::make_shared<WSType>(inbound,mEvents,tls_server_context);
        else throw std::system_error(EINVAL,std::system_category(),"TLS ServerContext is NULL");
      }
  };
}

typedef itc::sys::CancelableThread<abstract::Worker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;

#endif /* __IOWORKER_H__ */

