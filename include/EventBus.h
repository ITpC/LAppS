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
 *  $Id: EventBus.h April 11, 2018 4:18 PM $
 * 
 **/


#ifndef __EVENTBUS_H__
#  define __EVENTBUS_H__

#include <queue>
#include <list>
#include <mutex>
#include <functional>

#include <sys/synclock.h>
#include <sys/PosixSemaphore.h>

namespace LAppS
{
  namespace EBUS
  {
    enum EventName { NEW, OUT, IN, ERROR };
    struct Event
    {
      EventName event;
      int       value;
      const bool operator > (const Event& ref) const
      {
        return event > ref.event;
      }
      const bool operator < (const Event& ref) const
      {
        return event < ref.event;
      }
    };
    class EventBus
    {
     private:
      std::mutex  mMutex;
      itc::sys::Semaphore   mSem;
      std::queue<Event>     mEvents;
     public:
      
      explicit EventBus():mMutex(),mSem(0),mEvents(){}
      EventBus(const EventBus&)=delete;
      EventBus(EventBus&)=delete;
      ~EventBus()=default;
      
      const size_t size() const
      {
        return mEvents.size();
      }
      
      void push(const std::vector<Event>& e)
      {
        SyncLock sync(mMutex);
        for(size_t i=0;i<e.size();++i)
        {
          mEvents.push(e[i]);
          if(!mSem.post())
            throw std::system_error(errno,std::system_category(),"Can't increment semaphore, system is going down or semaphore error");
        }
      }
      
      void push(const Event& e)
      {
        SyncLock sync(mMutex);
        mEvents.push(e);
        if(!mSem.post())
          throw std::system_error(errno,std::system_category(),"Can't increment semaphore, system is going down or semaphore error");
      }
      
      void onEvent(std::function<void(const Event&)> processor)
      {
        if(!mSem.wait())
          throw std::system_error(errno,std::system_category(),"Can't wait on semaphore");
        
        Event event;
        
        try{
          SyncLock sync(mMutex);
          event=std::move(mEvents.front());
          mEvents.pop();
        }catch(const std::exception& e)
        {
          throw e;
        }
        processor(event);
      }
    };
  }
}

#endif /* __EVENTBUS_H__ */

