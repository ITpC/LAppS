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
 *  $Id: ePollController.h April 11, 2018 4:31 PM $
 * 
 **/

/***
 * DEPRECATED       DEPRECATED   DEPRECATED     DEPRECATED
 * DE     CATED     DE           DE    CATED    DE    CATED
 * DE     CATED     DE           DE    CATED    DE    CATED
 * DE     CATED     DEPRECATED   DEPRECATED     DEPRECATED
 * DE     CATED     DE           DE             DE REC
 * DE     CATED     DE           DE             DE  ECA
 * DEPRECATED       DEPRECATED   DE             DE   CATED  
 * 
 * ... you've got the idea. this sht is deprecated.
 ***/

#ifndef __EPOLLCONTROLLER_H__
#  define __EPOLLCONTROLLER_H__

#include <mutex>
#include <memory>
#include <atomic>

#include <abstract/Runnable.h>
#include <sys/CancelableThread.h>
#include <sys/PosixSemaphore.h>
#include <abstract/IController.h>
#include <sys/synclock.h>
#include <EventBus.h>
#include <ePoll.h>

namespace LAppS
{
  namespace EPoll
  {
    typedef ::itc::abstract::IController<LAppS::EBUS::Event>::ViewType     ViewType;
    typedef ::itc::abstract::IController<LAppS::EBUS::Event>::ViewTypeSPtr ViewSPtr;
  }
  
  class ePollController
  : public  ::itc::abstract::IRunnable,
            ::itc::abstract::IController<LAppS::EBUS::Event>
  {
   private:
    itc::sys::Semaphore               mIteration;
    std::atomic<bool>                 mMayRun;
    std::atomic<bool>                 mCanStop;
    std::vector<epoll_event>          mEvents;
    ePoll<ePollINDefaultOps>          mEPoll;
    EPoll::ViewSPtr                   mView;
    
    bool error_bit(const uint32_t events) const
    {
     return (events & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
    }

    bool in_bit(const uint32_t events) const
    {
     return (events & EPOLLIN);
    }
    
    void pushEvent(const LAppS::EBUS::EventName en, const int fd)
    {
      if(!this->notify({en,fd},mView))
      {
        itc::getLog()->info(__FILE__,__LINE__,"ePollController::execute(), - a recipient view is not available anymore, going down too.");
        mMayRun.store(false);
        mCanStop.store(true);
      }
    }
    
    void pushEvent(const std::vector<LAppS::EBUS::Event>& events)
    {
      if(!this->notify(events,mView))
      {
        itc::getLog()->info(__FILE__,__LINE__,"ePollController::execute(), - a recipient view is not available anymore, going down too.");
        mMayRun.store(false);
        mCanStop.store(true);
      }
    }
    
   public:
    
    
    
    explicit ePollController(const size_t maxFDs=1000) 
    : mMayRun(true),mCanStop(false), mEvents(maxFDs)
    {
      itc::getLog()->info(__FILE__,__LINE__,"ePollController %p is started",this);
    }
    ePollController(const ePollController&)=delete;
    ePollController(ePollController&)=delete;
    
    void setView(const EPoll::ViewSPtr& view)
    {
      mView=view;
      mIteration.post();
    }
    
    
    void add(const int fd)
    {
      mEPoll.add(fd);
    }
    
    void mod(const int fd)
    {
      mEPoll.mod(fd);
    }
    
    void del(const int fd)
    {
      mEPoll.del(fd);
    }
    
    void execute()
    {
      std::vector<LAppS::EBUS::Event> outEvents;
      while(mMayRun)
      {
        //mIteration.wait();
        try{
          auto events=mEPoll.poll(mEvents,10);
          if(events > 0)
          {
            outEvents.clear();
            outEvents.reserve(static_cast<size_t>(events));
            
            for(size_t i=0;i<static_cast<size_t>(events);++i)
            {
              if(error_bit(mEvents[i].events))
              {
                outEvents.push_back({LAppS::EBUS::ERROR,mEvents[i].data.fd});
              }
              else if(in_bit(mEvents[i].events))
              {
                outEvents.push_back({LAppS::EBUS::IN,mEvents[i].data.fd});
              }
            }
            pushEvent(outEvents);
          } // ignoring -1 as it is only appears on EINTR
        }catch(const std::exception& e)
        {
          itc::getLog()->error(
            __FILE__,__LINE__,
            "Exception on IN-Events polling: %s. The server probably goes down. Going down myself.",
            e.what()
          );
          mMayRun.store(false);
          mCanStop.store(true);
        }
        //mIteration.post();
      }
      mCanStop.store(true);
    }
    const std::vector<epoll_event>& getEvents() const
    {
      return mEvents;
    }
    void onCancel()
    {
      this->shutdown();
    }
    void shutdown()
    {
      mMayRun.store(false);
      while(!mCanStop)
      {
        sched_yield();
      }
    }
    ~ePollController()
    {
      if(!mCanStop) this->shutdown();
      itc::getLog()->info(__FILE__,__LINE__,"ePollController %p is down",this);
    }
  };
  typedef std::shared_ptr<ePollController> ePollControllerSPtrType;
  typedef std::weak_ptr<ePollController> ePollControllerWSPtrType;
  typedef itc::sys::CancelableThread<LAppS::ePollController> ePollControllerThread;
}

#endif /* __EPOLLCONTROLLER_H__ */

