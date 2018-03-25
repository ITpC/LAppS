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
 *  $Id: Worker.h January 7, 2018 5:27 AM $
 * 
 **/


#ifndef __WSWORKER_H__
#  define __WSWORKER_H__

// c headers
#include <stdio.h>

// remove when done with debugging
#include <iostream>


// c++ headers
#include <map>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <set>

// ITC headers
#include <sys/synclock.h>
#include <sys/Nanosleep.h>
#include <TCPListener.h>





#include <tls.h>

// LAppS headers
#include <TLSServerContext.h>
#include <ePoll.h>
#include <WebSocket.h>
#include <InboundConnectionsAdapter.h>
#include <abstract/Worker.h>
#include <abstract/Runnable.h>
#include <abstract/Application.h>
#include <WorkerStats.h>
#include <Val2Type.h>
#include <tsbqueue.h>


/**
 * \@brief Connections worker.
 * To be used as a CancelableThread only
 **/
template <bool TLSEnable=false, bool StatsEnable=false> class WSWorker 
: public ::abstract::Worker
{
public:
 typedef WebSocket<TLSEnable,StatsEnable> WSType;
  typedef std::shared_ptr<WSType>          WSSPtr;
 typedef std::shared_ptr<itc::tsbqueue<WSSPtr>> InboundConnectionsQueue;
private:
 itc::utils::Bool2Type<TLSEnable> enableTLS;
 itc::utils::Bool2Type<StatsEnable> enableStatsUpdate;
 mutable std::mutex mStatsMutex;
 mutable std::mutex mOutQMutex;
 
 std::atomic<bool> doRun;
 std::atomic<bool> canStop;
 
 std::vector<epoll_event> events;
 ePoll<> mEPoll;
 
 std::map<int,WSSPtr> mConnections;

 InboundConnectionsQueue mInbound;
 
 std::vector<uint8_t> inbuffer;
 std::queue<TaggedEvent> mOutEvents;
 
 
public:
  WSWorker(
    const uint8_t id, const size_t maxConnections, 
    const InboundConnectionsQueue& ic, const std::string& path
  ) : abstract::Worker(id,maxConnections,path), enableTLS(), enableStatsUpdate(),
    mStatsMutex(),  doRun(true), canStop(false), events(maxConnections),
    mEPoll(), mInbound(ic), inbuffer(8192)
  {
  }
  void submitResponse(const TaggedEvent& event)
  {
    SyncLock sync(mOutQMutex);
    mOutEvents.push(event);
  }
  const std::string& getRequestTarget() const
  {
    return mRequestTarget;
  }
  
  bool error_bits(const uint32_t events)
  {
   return (events & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
  }

  bool in_bit(const uint32_t events)
  {
   return (events & EPOLLIN);
  }

  bool out_bit(const uint32_t events)
  {
   return (events & EPOLLOUT);
  }
  
  void waitForInbound()
  {
    if(!doRun.load()) return;

    WSSPtr tmp;
    try{
      mInbound->recv(tmp);

      if(tmp->isValid())
      {
        int sockfd=tmp->getFileDescriptor();
        tmp->setWorkerId(this->getID());
        mConnections.emplace(sockfd,tmp);
        mEPoll.add(sockfd);
      }
    }catch(const std::exception& e)
    {
      itc::getLog()->error(
        __FILE__,__LINE__,
        "Worker %p, in %s() an inbound connections queue is down [%s], exiting ...",
        this,__FUNCTION__,e.what()
      );
      doRun.store(false);
    }
  }
 
  void collectInbound()
  {
    try
    {
      int qSize;
      mInbound->size(qSize);
      if( qSize > 0)
      {
       ::timespec ts{0,100000};
       WSSPtr tmp;
       if(mInbound->tryRecv(tmp,ts))
       {
         int sockfd=tmp->getFileDescriptor();
         tmp->setWorkerId(this->getID());
         mConnections.emplace(sockfd,tmp);
         mEPoll.add(sockfd);
       }
      }
    }catch(const std::exception& e){
      itc::getLog()->error(
        __FILE__,__LINE__,
        "Worker %p, in %s() an inbound connections queue is down [%s], exiting ...",
        this,__FUNCTION__,e.what()
      );
      doRun.store(false);
    }
  }
 
 int pollForEvents()
 {
    events.clear();
    // poll the connections for events
    int ret=0;
    try {     
      events.resize(mConnections.size());
      ret=mEPoll.poll(events);
      return ret;
    }catch(const std::exception& e)
    {
      itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an exception %s, in %s()",this, e.what(),__FUNCTION__);
      doRun.store(false);
      return 0;
    }   
 }
  
  void deleteConnection(const int32_t fd)
  {
    auto it=mConnections.find(fd);
    if(it!=mConnections.end())
    {
      mConnections.erase(it);
    }
  }

  void handleIO(WSSPtr& current)
  {
    int received=current->recv(inbuffer,MSG_NOSIGNAL);
    switch(received)
    {
      case -1:
      {
        switch(errno)
        {
          case EAGAIN:

            break;
          default:
            itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an error %s, in %s()",this, strerror(errno),__FUNCTION__);
            break;
        }
      }
      break;
      case 0:
        break;
      default:
        current->processInput(inbuffer,received);
      break;
    }
  } 
  
  void cleanClosed()
  {
    if(!mConnections.empty())
    {
      std::queue<int> toDelete;
      for(auto it=mConnections.begin();it!=mConnections.end();++it)
      {
        if(it->second->getState()==WSType::CLOSED)
        {
          toDelete.push(it->first);
        }
      }
      while(!toDelete.empty())
      {
        deleteConnection(toDelete.front());
        toDelete.pop();
      }
    }
  }
  
public:
  constexpr const bool isTLSEnabled()
  {
    return TLSEnable;
  }
  
  const WorkerStats& getStats(const itc::utils::Bool2Type<false> stats_are_disabled) const
  {
    return mStats;
  }
  const WorkerStats& getStats(const itc::utils::Bool2Type<true> stats_are_enabled) const
  {
    SyncLock sync(mStatsMutex);
    return mStats;
  }
  
  const WorkerStats& getStats() const
  {
    return this->getStats(enableStatsUpdate);
  }
  
  void updateStats(const itc::utils::Bool2Type<false> fictive)
  {
    mStats.mConnections=mConnections.size();
  }
  
  void updateStats(const itc::utils::Bool2Type<true> fictive)
  {
    SyncLock sync(mStatsMutex);
    mStats.mConnections=mConnections.size();
    
    mStats.mInCMASize=0;
    mStats.mInMessageMaxSize=0;
    mStats.mInMessageCount=0;
    mStats.mOutCMASize=0;
    mStats.mOutMessageMaxSize=0;
    mStats.mOutMessageCount=0;    

    for(auto it=mConnections.begin();it!=mConnections.end();++it)
    {
      auto conn_stats=it->second->getStats();

      mStats.mInCMASize+=conn_stats.mInCMASize;
      mStats.mOutCMASize+=conn_stats.mOutCMASize;

      if(mStats.mInMessageMaxSize<conn_stats.mInMessageMaxSize)
        mStats.mInMessageMaxSize=conn_stats.mInMessageMaxSize;

      if(mStats.mOutMessageMaxSize<conn_stats.mOutMessageMaxSize)
        mStats.mOutMessageMaxSize=conn_stats.mOutMessageMaxSize;

      mStats.mInMessageCount+=conn_stats.mInMessageCount;
      mStats.mOutMessageCount+=conn_stats.mOutMessageCount;

    }
    mStats.mOutCMASize=mStats.mOutCMASize/mStats.mConnections;
    mStats.mInCMASize=mStats.mInCMASize/mStats.mConnections;
    
  }
  void updateStats()
  {
    updateStats(enableStatsUpdate);
  }
  
  void execute()
  {
    while(doRun)
    {
      cleanClosed();
      
      updateStats();
      
      if(mStats.mConnections < mMaxConnections)
      {
        collectInbound(); 
      }
      else
      {
        itc::getLog()->info(__FILE__,__LINE__,"Worker :%p[%d], maxConnections limit is reached.",this,ID);
      }
      
      if(mStats.mConnections > 0 )
      {
        int ret=pollForEvents();

        if(ret>0)
        {
          for(size_t i=0;(i<events.size())&&(i<static_cast<unsigned int>(ret));++i)
          {
            if(error_bits(events[i].events))
            {
              deleteConnection(events[i].data.fd);
            }
            else if(in_bit(events[i].events)) 
            {
              WSSPtr& current(mConnections[events[i].data.fd]);

              switch(current->getState())
              {
                case WSType::MESSAGING: 
                  handleIO(current);
                  if(current->getState()!=WSType::CLOSED)
                  {
                    mEPoll.mod(events[i].data.fd);
                  }
                break;
                case WSType::CLOSED:
                  deleteConnection(events[i].data.fd);
                break;
                case WSType::HANDSHAKE:
                  itc::getLog()->fatal(__FILE__,__LINE__,"Logic error, the HANDSHAKE state may never happen in WSWorker");
                  itc::getLog()->flush();
                  exit(1);
              }
            }
          }
        }
      } 
      else {
        waitForInbound();
        if(doRun.load() == false)
          break;
      }
    }
    canStop.store(true);
    itc::getLog()->info("Worker %p is stopped.",this);
  }
  void shutdown()
  {
    doRun.store(false);
    while(!canStop.load())
      sched_yield();
    
    mConnections.clear();
  }
  void onCancel()
  {
    this->shutdown();
  }
  ~WSWorker()
  {
    this->shutdown();
  }
};

typedef itc::sys::CancelableThread<abstract::Worker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;


#endif /* __WSWORKER_H__ */

