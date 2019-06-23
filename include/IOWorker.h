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

#include <ePoll.h>
#include <abstract/IView.h>
#include <WebSocket.h>
#include <Shakespeer.h>
#include <abstract/Worker.h>
#include <sys/mutex.h>
#include <time.h>
#include <ext/tsl/robin_map.h>
#include <cfifo.h>
#include <InQueue.h>

#include <wolfSSLLib.h>

namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false> class IOWorker
  : public ::abstract::Worker
  {
    public:
      typedef WebSocket<TLSEnable,StatsEnable>        WSType;
      typedef std::shared_ptr<WSType>                 WSSPtr;
      
    private:
      using qtype=itc::tsbqueue<::itc::TCPListener::value_type,itc::sys::mutex>;
      using TLSContextType=std::shared_ptr<wolfSSLLib<TLS_SERVER>::wolfSSLContext>;
      
      itc::utils::Bool2Type<TLSEnable>          enableTLS;
      itc::utils::Bool2Type<StatsEnable>        enableStatsUpdate;
      
      std::atomic<bool>                         mMayRun;
      std::atomic<bool>                         mCanStop;
      size_t                                    mMaxEPollWait;
      LAppS::IOStats                            mStats;
      
      LAppS::Shakespeer<TLSEnable,StatsEnable>  mShakespeer;
      SharedEPollType                           mEPoll;
      
      qtype                                     mInQueue;
      tsl::robin_map<int,WSSPtr>                mConnections;
      itc::cfifo<int32_t>                       mDCQueue;
      
      std::vector<epoll_event>                  mEvents;
      
      std::atomic<bool>                         haveConnections;
      std::atomic<bool>                         haveDisconnects;
      
      itc::sys::Nap                             nap;
      
      TLSContextType                            mTLSContext;
      
      
      bool error_bit(const uint32_t event) const
      {
        return (event & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
      }
      bool in_out_bits(const uint32_t events) const
      {
        return (events & (EPOLLIN|EPOLLOUT));
      }
    
    public:
     
    explicit IOWorker(const size_t id, const size_t maxConnections,const bool auto_fragment)
    : Worker(id,maxConnections,auto_fragment), enableTLS(),  enableStatsUpdate(), 
      mMayRun{true}, mCanStop{false}, mMaxEPollWait{LAppSConfig::getInstance()->getWSConfig()["workers"]["max_poll_wait_ms"]},
      mShakespeer(), mEPoll(std::make_shared<ePoll>()),
      mInQueue(),mConnections(), mDCQueue(20), 
      mEvents{LAppSConfig::getInstance()->getWSConfig()["workers"]["max_poll_events"]},
      haveConnections{false},haveDisconnects{false},mTLSContext(wolfSSLServer::getInstance()->getContext())
    {
      mConnections.clear();
      LAppS::WStats::getInstance()->add_slot(getID());
    }
    
    IOWorker()=delete;
    IOWorker(const IOWorker&)=delete;
    IOWorker(IOWorker&)=delete;
    
    void enqueue(const ::itc::TCPListener::value_type& socket)
    {
      mInQueue.send(std::move(socket));
    }
    
    void disconnect(const int32_t fd)
    {
      mDCQueue.send(fd);
      haveDisconnects.store(true);
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
        if(haveDisconnects)
        {
          while(!mDCQueue.empty())
          {
            const int32_t fd=std::move(mDCQueue.recv());
            deleteConnection(fd);
          }
          haveDisconnects.store(false);
        }
        
        processInbound();
        
        if(haveConnections.load())
        {
          try{
            int ret=mEPoll->poll(mEvents,mMaxEPollWait);
            if(ret > 0)
            {  
              for(auto i=0;i<ret;++i)
              {
                if(error_bit(mEvents[i].events))
                {
                  deleteConnection(mEvents[i].data.fd);
                }
                else 
                {
                  processIO(mEvents[i].data.fd);
                  mStats.mInMessageCount++;
                }
              }
              //LAppS::WStats::getInstance()->try_update(getID(),mStats);
            }
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__,__LINE__,"Exception: %s", e.what());
            mMayRun.store(false);
          }
        }
        else
        {
          processInbound();
          nap.usleep(1000);
        }
      }
      mCanStop.store(true);
    }

    void shutdown()
    {
      mMayRun.store(false);
      bool expected=true;
      while(!mCanStop.compare_exchange_strong(expected,true))
      {
        expected=true;
      }
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

    
    private:
      /**
       * @brief processing all inbound connections in bulk.
       **/
      void processInbound()
      {
        try{
          /*
          tsl::robin_map<size_t,IOStats> stats;
          LAppS::WStats::getInstance()->getStats(stats);
          bool try_recv_new_socket=false;
          for(const auto& kv: stats)
          { 
            if(kv.first != this->getID())
            {
              if(kv.second.mConnections > mConnections.size())
              {
                try_recv_new_socket=true;
                break;
              }
            }
          }
          
          if(try_recv_new_socket)
          {
            
  */
          itc::TCPListener::value_type sockt;
          if(mInQueue.try_recv(sockt))
          {
            auto current=mkWebSocket(sockt);
            int fd=current->getfd();

            if(mConnections.size()<mMaxConnections)
            {
                itc::getLog()->info(
                __FILE__,__LINE__,
                "New inbound connection from %s with fd %d will be added to connection pool of worker %u ",
                current->getPeerAddress().c_str(),fd,ID
              );
              mConnections.emplace(fd,std::move(current));
              mEPoll->mod_in(fd);    
            }
            else
            {
              mShakespeer.sendForbidden(current);
              itc::getLog()->error(
                __FILE__,__LINE__,
                "Too many connections, new connection from %s on fd %d is rejected",
                current->getPeerAddress().c_str(),fd
              );
            }
            mStats.mConnections=mConnections.size();
            haveConnections.store(mStats.mConnections>0);
//            while(!LAppS::WStats::getInstance()->try_update(getID(),mStats));
          } 
        }catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__,__LINE__,"Connection became invalid before handshake. Exception: %s",e.what());
        }
      }
      
      void processIO(const int fd)
      {
        auto it=mConnections.find(fd);
        if(it!=mConnections.end())
        {
          auto current=it->second;
          switch(current->getState())
          {
            case WSType::MESSAGING:
              try {
                int ret=current->handleInput();
                if(ret == -1)
                {
                  itc::getLog()->info(__FILE__,__LINE__,"Disconnected: %s",current->getPeerAddress().c_str());
                  deleteConnection(fd);
                }
              }
              catch(const std::exception& e)
              {
                itc::getLog()->info(__FILE__,__LINE__,"Application is going down [Exception: %s], socket with fd %d must be disconnected ", e.what(),current->getfd());
                deleteConnection(fd);
              }
            break;
            case WSType::HANDSHAKE:
                mShakespeer.handshake(current,*LAppS::SServiceRegistry::getInstance());
                if(current->getState() !=WSType::MESSAGING)
                {
                  itc::getLog()->error(__FILE__,__LINE__,"Handshake with the peer %s has been failed. Disconnecting.", current->getPeerAddress().c_str());
                  deleteConnection(fd);
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
        return std::make_shared<WSType>(std::move(inbound),mEPoll,this,mustAutoFragment());
      }
      
      const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<true> tls_is_enabled)
      {
        //auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
        
        auto tls_server_context=mTLSContext->raw_context();
        
        if(tls_server_context)
          return std::make_shared<WSType>(std::move(inbound),mEPoll,this,mustAutoFragment(),tls_server_context);
        else throw std::system_error(EINVAL,std::system_category(),"TLS ServerContext is NULL");
      }
  };
}

typedef itc::sys::CancelableThread<::abstract::Worker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;

#endif /* __IOWORKER_H__ */

