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


namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false> 
    class IOWorker :public ::abstract::Worker
  {
    public:
      typedef WebSocket<TLSEnable,StatsEnable>        WSType;
      typedef std::shared_ptr<WSType>                 WSSPtr;
      
    private:
      itc::utils::Bool2Type<TLSEnable>          enableTLS;
      itc::utils::Bool2Type<StatsEnable>        enableStatsUpdate;
      
      std::atomic<bool>                         mMayRun;
      std::atomic<bool>                         mCanStop;
      
      mutable itc::sys::mutex                   mConnectionsMutex;
      itc::sys::mutex                           mInboundMutex;
      itc::sys::mutex                           mDCQMutex;
      
      LAppS::Shakespeer<TLSEnable,StatsEnable>  mShakespeer;
      SharedEPollType                           mEPoll;
      
      std::queue<itc::CSocketSPtr>              mInboundConnections;
      std::map<int,WSSPtr>                      mConnections;
      std::queue<int32_t>                       mDCQueue;
      
      std::vector<epoll_event>                  mEvents;
      
      std::atomic<bool>                         haveConnections;
      std::atomic<bool>                         haveNewConnections;
      std::atomic<bool>                         haveDisconnects;
      
      
      bool error_bit(const uint32_t event) const
      {
        return (event & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
      }
      bool in_out_bits(const uint32_t events) const
      {
        return (events & (EPOLLIN|EPOLLOUT));
      }

      const auto gatherStats(const itc::utils::Bool2Type<false> stats_disabled)
      {
        return std::move(std::make_shared<json>(json::object()));
      }

      const auto gatherStats(const itc::utils::Bool2Type<true> stats_enabled)
      {
        std::shared_ptr<json> nstats=std::make_shared<json>(json::object());
        
        for(const auto& connection : mConnections)
        {
          const auto sock_stats=connection.second->getStats();
          const std::string instanceid=std::to_string(connection.second->getApplication()->getInstanceId());
          const std::string service_name=connection.second->getApplication()->getName();
          
          if(nstats->find(service_name)==nstats->end())
          {
            (*nstats)[service_name]=json::object();
          }
          
          if((*nstats)[service_name].find(instanceid)==(*nstats)[service_name].end())
          {
            (*nstats)[service_name][instanceid]={
              {{"InMessageCount",0}},
              {{"OutMessageCount",0}},
              {{"Connections",0}},
              {{"InMessageMaxSize",0}},
              {{"OutMessageMaxSize",0}},
              {{"OutCMASize",0}},
              {{"InCMASize",0}}
            };
          }
          
          (*nstats)[service_name][instanceid]={
            {{"InMessageCount", (*nstats)[service_name][instanceid]["InMessageCount"].get<size_t>()+sock_stats.mInMessageCount}},
            {{"OutMessageCount", (*nstats)[service_name][instanceid]["OutMessageCount"].get<size_t>()+sock_stats.mOutMessageCount}},
            {{"Connections", (*nstats)[service_name][instanceid]["Connections"].get<size_t>()+1 }}
          };
          
          if((*nstats)[service_name][instanceid]["InMessageMaxSize"].get<size_t>()<sock_stats.mInMessageMaxSize)
            (*nstats)[service_name][instanceid]["InMessageMaxSize"]=sock_stats.mInMessageMaxSize;
          
          if((*nstats)[service_name][instanceid]["OutMessageMaxSize"].get<size_t>()<sock_stats.mOutMessageMaxSize)
            (*nstats)[service_name][instanceid]["OutMessageMaxSize"]=sock_stats.mOutMessageMaxSize;
          
          if((*nstats)[service_name][instanceid]["OutMessageCount"].get<size_t>() > 0)
          {
            (*nstats)[service_name][instanceid]["OutCMASize"]=
              (*nstats)[service_name][instanceid]["OutCMASize"].get<size_t>()+(
                sock_stats.mOutCMASize-(*nstats)[service_name][instanceid]["OutCMASize"].get<size_t>()
              )/(*nstats)[service_name][instanceid]["OutMessageCount"].get<size_t>();
          }
            
          if((*nstats)[service_name][instanceid]["InMessageCount"].get<size_t>() > 0)
          {
            (*nstats)[service_name][instanceid]["InCMASize"]=
              (*nstats)[service_name][instanceid]["InCMASize"].get<size_t>()+(
                sock_stats.mInCMASize-(*nstats)[service_name][instanceid]["InCMASize"].get<size_t>()
              )/(*nstats)[service_name][instanceid]["InMessageCount"].get<size_t>();
          }
          
        }
        return std::move(nstats);
      }      
    public:
     
    explicit IOWorker(const size_t id, const size_t maxConnections,const bool auto_fragment)
    : Worker(id,maxConnections,auto_fragment), enableTLS(),  enableStatsUpdate(), 
      mMayRun{true}, mCanStop{false}, mConnectionsMutex(),  mInboundMutex(), 
      mDCQMutex(), mShakespeer(), mEPoll(std::make_shared<ePoll>()),
      mInboundConnections(), mConnections(), mDCQueue(), mEvents(1000),
      haveConnections{false},haveNewConnections{false},haveDisconnects{false}
    {
      mConnections.clear();
    }
    
    IOWorker()=delete;
    IOWorker(const IOWorker&)=delete;
    IOWorker(IOWorker&)=delete;
    
    void onUpdate(const ::itc::TCPListener::value_type& socketsptr)
    {
      ITCSyncLock sync(mInboundMutex);
      mInboundConnections.push(std::move(socketsptr));
      updateStats();
      haveNewConnections.store(true);
    }
    
    void onUpdate(const std::vector<::itc::TCPListener::value_type>& socketsptr)
    {
      ITCSyncLock sync(mInboundMutex);
      for(size_t i=0;i<socketsptr.size();++i)
      {
        mInboundConnections.push(std::move(socketsptr[i]));
      }
      updateStats();
      haveNewConnections.store(true);
    }
    
    void disconnect(const int32_t fd)
    {
      ITCSyncLock sync(mDCQMutex);
      mDCQueue.push(fd);
      haveDisconnects.store(true);
    }
    
    const size_t getConnectionsCount() const
    {
      ITCSyncLock sync(mConnectionsMutex);
      return mConnections.size();
    }
    
    const size_t getInMessagesCount() const
    {
      return mStats.mInMessageCount;
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
        if(haveDisconnects)
        {
          ITCSyncLock syncdcq(mDCQMutex);
          ITCSyncLock sync(mConnectionsMutex);
          while(!mDCQueue.empty())
          {
            const int32_t fd=std::move(mDCQueue.front());
            mDCQueue.pop();
            deleteConnection(fd);
          }
          haveDisconnects.store(false);
        }
        if(haveConnections)
        {
          try{
            int ret=mEPoll->poll(mEvents,1);
            if(ret > 0)
            {
              mStats.mEventQSize=ret;
              for(auto i=0;i<ret;++i)
              {
                if(error_bit(mEvents[i].events))
                {
                  ITCSyncLock sync(mConnectionsMutex);
                  deleteConnection(mEvents[i].data.fd);
                }
                else 
                {
                  processIO(mEvents[i].data.fd);
                  mStats.mInMessageCount++;
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
          itc::sys::Nap nap;
          nap.usleep(1000);
        }
      }
      mCanStop.store(true);
    }

    void shutdown()
    {
      mMayRun.store(false);
      ITCSyncLock sync(mConnectionsMutex);
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

    const WorkerMinStats getMinStats() const
    {
      return { mStats.mConnections,mStats.mEventQSize };
    }
    
    const std::shared_ptr<json> getStats()
    {
      ITCSyncLock sync(mConnectionsMutex);
      return std::move(gatherStats(enableStatsUpdate));
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
          ITCSyncLock sync(mInboundMutex);

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
              itc::getLog()->error(__FILE__,__LINE__,"Connection became invalid before handshake. Exception: %s",e.what());
            }

            mInboundConnections.pop();
          }
          mConnectionsMutex.unlock();
          haveNewConnections.store(false);
        }
      }
      
      void processIO(const int fd)
      {
        ITCSyncLock sync(mConnectionsMutex);
        auto it=mConnections.find(fd);
        if(it!=mConnections.end())
        {
          auto current=it->second;
          switch(current->getState())
          {
            case WSType::HANDSHAKE:
                mShakespeer.handshake(current,*LAppS::SServiceRegistry::getInstance());
                if(current->getState() !=WSType::MESSAGING)
                {
                  itc::getLog()->error(__FILE__,__LINE__,"Handshake with the peer %s has been failed. Disconnecting.", current->getPeerAddress().c_str());
                  deleteConnection(fd);
                }
            break;
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
        auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
        if(tls_server_context)
          return std::make_shared<WSType>(std::move(inbound),mEPoll,this,mustAutoFragment(),tls_server_context);
        else throw std::system_error(EINVAL,std::system_category(),"TLS ServerContext is NULL");
      }
  };
}

typedef itc::sys::CancelableThread<abstract::Worker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;

#endif /* __IOWORKER_H__ */

