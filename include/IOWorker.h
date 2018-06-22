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

#include <Application.h>
#include <ePoll.h>
#include <abstract/IView.h>
#include <WebSocket.h>
#include <Shakespeer.h>
#include <abstract/Worker.h>
#include <sys/atomic_mutex.h>
#include <ServiceProperties.h>
#include <Reader.h>


namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false> 
    class IOWorker :public ::abstract::Worker
  {
    public:
      typedef WebSocket<TLSEnable,StatsEnable>        WSType;
      typedef std::shared_ptr<WSType>                 WSSPtr;
      typedef ::LAppS::Reader<TLSEnable,StatsEnable>  ReaderType;
      typedef itc::sys::CancelableThread<ReaderType>  ReaderThreadType;
      typedef std::shared_ptr<ReaderThreadType>       ReaderSharedThreadType;
      typedef std::vector<ReaderSharedThreadType>     ReadersVector;
      
    private:
      itc::utils::Bool2Type<TLSEnable>          enableTLS;
      itc::utils::Bool2Type<StatsEnable>        enableStatsUpdate;
      
      std::atomic<bool>                         mMayRun;
      std::atomic<bool>                         mCanStop;
      
      itc::sys::AtomicMutex                     mConnectionsMutex;
      itc::sys::AtomicMutex                     mInboundMutex;
      itc::sys::AtomicMutex                     mSSQMutex;
      
      LAppS::Shakespeer<TLSEnable,StatsEnable>  mShakespeer;
      SharedEPollType                           mEPoll;
      
      std::queue<itc::CSocketSPtr>              mInboundConnections;
      std::map<int,WSSPtr>                      mConnections;
      
      std::vector<epoll_event>                  mEvents;
      
      itc::sys::Nap                             mNap;
      
      std::atomic<bool>                         haveConnections;
      std::atomic<bool>                         haveNewConnections;
      
      ReadersVector                             mReaders;
      size_t                                    readersIndex;
      size_t                                    mMaxReaders;
      
      bool error_bit(const uint32_t event) const
      {
        return (event & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
      }
      bool in_out_bits(const uint32_t events) const
      {
        return (events & (EPOLLIN|EPOLLOUT));
      }
      
    public:
     
    explicit IOWorker(const size_t id, const size_t maxConnections,const bool auto_fragment, const size_t preads)
    : Worker(id,maxConnections,auto_fragment), enableTLS(),        enableStatsUpdate(), 
      mMayRun{true},                mCanStop{false},    mConnectionsMutex(), 
      mInboundMutex(),              mShakespeer(),      mEPoll(std::make_shared<ePoll>()),
      mInboundConnections(),        mConnections(),     mEvents(1000),
      mNap(), haveConnections{false},haveNewConnections{false},readersIndex(0),mMaxReaders(preads)
    {
      mConnections.clear();
      for(size_t i=0;i<mMaxReaders;++i)
      {
        mReaders.push_back(
          std::move(
            std::make_shared<ReaderThreadType>(
              std::move(
                std::make_shared<ReaderType>()
              )
            )
          )
        );  
      }
    }
    
    IOWorker()=delete;
    IOWorker(const IOWorker&)=delete;
    IOWorker(IOWorker&)=delete;
    
    void onUpdate(const ::itc::TCPListener::value_type& socketsptr)
    {
      AtomicLock sync(mInboundMutex);
      mInboundConnections.push(std::move(socketsptr));
      mStats.mConnections=mConnections.size()+mInboundConnections.size();
      haveNewConnections.store(true);
    }
    
    void onUpdate(const std::vector<::itc::TCPListener::value_type>& socketsptr)
    {
      AtomicLock sync(mInboundMutex);
      for(size_t i=0;i<socketsptr.size();++i)
      {
        mInboundConnections.push(std::move(socketsptr[i]));
      }
      mStats.mConnections=mConnections.size()+mInboundConnections.size();
      haveNewConnections.store(true);
    }
        
    const size_t getConnectionsCount() const
    {
      AtomicLock sync(mConnectionsMutex);
      return mConnections.size();
    }
    const bool  isTLSEnabled() const
    {
      return TLSEnable;
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
        processInbound();
        if(haveConnections)
        {
          try{
            int ret=mEPoll->poll(mEvents,1);
            if(ret > 0)
            {
              mStats.mEventQSize+=ret;
              for(auto i=0;i<ret;++i)
              {
                if(error_bit(mEvents[i].events))
                {
                  AtomicLock sync(mConnectionsMutex);
                  deleteConnection(mEvents[i].data.fd);
                }
                else 
                {
                  processIO(mEvents[i].data.fd);
                }
              }
            }
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__,__LINE__,"Exception on ePoll::poll(): ", e.what());
            mMayRun.store(false);
          }
        }
        else
        {
          mNap.usleep(10000);
        }
      }
      mCanStop.store(true);
    }

    void shutdown()
    {
      mMayRun.store(false);
      AtomicLock sync(mConnectionsMutex);
      mConnections.clear();
      mCanStop.store(true);
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
      mStats.mConnections=mConnections.size()+mInboundConnections.size();
    }

    private:
      /**
       * @brief processing all inbound connections in bulk.
       **/
      void processInbound()
      {
        if(haveNewConnections&&mConnectionsMutex.try_lock())
        {
          AtomicLock sync(mInboundMutex);

          while(!mInboundConnections.empty())
          {
            try{
              auto current=mkWebSocket(mInboundConnections.front());
              int fd=current->getfd();

              if(mConnections.size()<mMaxConnections)
              {
                  itc::getLog()->info(
                  __FILE__,__LINE__,
                  "New inbound connection from %s with fd %d will be added to connection pool of worker %u ",
                  current->getPeerAddress().c_str(),fd,ID
                );
                mConnections.emplace(fd,std::move(current));
                mStats.mConnections=mConnections.size();
                mEPoll->mod_in(fd);
                haveConnections.store(true);
              }
              else
              {
                mShakespeer.sendForbidden(current);
                itc::getLog()->error(
                  __FILE__,__LINE__,
                  "Too many connections, new connection from %s on fd %d will is rejected",
                  current->getPeerAddress().c_str(),fd
                );
              }
            }catch(const std::exception& e)
            {
              itc::getLog()->error(__FILE__,__LINE__,"Connection on went invalid before handshake. Exception: %s",e.what());
            }

            mInboundConnections.pop();
          }
          mConnectionsMutex.unlock();
          haveNewConnections.store(false);
        }
      }
      
      void processIO(const int fd)
      {
        AtomicLock sync(mConnectionsMutex);
        auto it=mConnections.find(fd);
        if(it!=mConnections.end())
        {
          auto current=it->second;
          switch(current->getState())
          {
            case WSType::HANDSHAKE:
                mShakespeer.handshake(current,*::ApplicationRegistry::getInstance());
                if(current->getState() !=WSType::MESSAGING)
                {
                  deleteConnection(fd);
                }
            break;
            case WSType::MESSAGING:
                mReaders[readersIndex]->getRunnable()->enqueue(current);
                if((++readersIndex) == mMaxReaders)
                  readersIndex=0;
                mStats.mEventQSize=0;
                for(size_t i=0;i<mMaxReaders;++i)
                {
                  mStats.mEventQSize+=mReaders[readersIndex]->getRunnable()->getQSize();
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
        mStats.mConnections=mConnections.size();
        if(mConnections.empty())
        {
          haveConnections.store(false);
        }
      }
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound)
      {
        return mkWebSocket(inbound, enableTLS);
      }
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<false> tls_is_disabled)
      {
        return std::make_shared<WSType>(std::move(inbound),this->getID(),mEPoll,mustAutoFragment());
      }
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<true> tls_is_enabled)
      {
        auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
        if(tls_server_context)
          return std::make_shared<WSType>(std::move(inbound),this->getID(),mEPoll,mustAutoFragment(),tls_server_context);
        else throw std::system_error(EINVAL,std::system_category(),"TLS ServerContext is NULL");
      }
  };
}

typedef itc::sys::CancelableThread<abstract::Worker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;

#endif /* __IOWORKER_H__ */

