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
 *  $Id: Application.h February 4, 2018 5:58 PM $
 * 
 **/


#ifndef __APPLICATION_H__
#  define __APPLICATION_H__

#include <string>
#include <fstream>
#include <memory>
#include <exception>

#include <abstract/Runnable.h>
#include <WSStreamProcessor.h>
#include <sys/synclock.h>
#include <WebSocket.h>
#include <abstract/Application.h>
#include <ApplicationContext.h>
#include <tsbqueue.h>

#include <WSWorkersPool.h>
#include <Singleton.h>
#include <abstract/Worker.h>

#include <ext/json.hpp>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdexcept>
}


using json = nlohmann::json;

namespace LAppS
{
  
  template <bool TLSEnable, bool StatsEnable, ApplicationProtocol Tproto> 
  class Application : public ::abstract::Application
  {
  private:
    typedef std::shared_ptr<::abstract::Worker> WorkerSPtrType;
    std::atomic<bool> mMayRun;
    std::atomic<bool> mCanStop;
    std::mutex mMutex;
    std::string mName;
    std::string mTarget;
    ApplicationContext<TLSEnable,StatsEnable,Tproto> mAppContext;
    itc::tsbqueue<TaggedEvent> mEvents;
    std::vector<WorkerSPtrType> mWorkers;
    
 public:
    
    const std::string& getName() const
    {
      return mName;
    }
    const std::string& getTarget() const
    {
      return mTarget;
    }
    const ApplicationProtocol getProtocol() const
    {
      return Tproto;
    }
    
    void onCancel()
    {
       mMayRun.store(false);
    }
    void shutdown()
    {
       mMayRun.store(false);
    }
    
    explicit Application(const std::string& appName,const std::string& target)
    : mMayRun(true),mMutex(),mName(appName), mTarget(target),mAppContext(appName,this)
    {
      itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->getWorkers(mWorkers);
    }
    
    auto getWorker(const size_t wid)
    {
      if( wid<mWorkers.size() ) 
      {
        return mWorkers[wid];
      }
      else
      { // Attempt to refresh local workers;
        mWorkers.clear();
        itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->getWorkers(mWorkers);
        if( wid < mWorkers.size())
        {
          return mWorkers[wid];
        }
        else
        {
          itc::getLog()->error(__FILE__,__LINE__,"No worker with ID %d is found",wid);
          throw std::system_error(EINVAL,std::system_category(),"No requested worker is found. The system is probably going down");
        }
      }
    }
    
    void enqueue(const TaggedEvent& event)
    {
      try {
        mEvents.send(event);
      } catch (const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Can't enqueue request to application %s, exception: %s",mName.c_str(),e.what());
      }
    }
    void enqueueDisconnect(const size_t wid, const int32_t sockfd)
    {
      try {
        mEvents.send({wid,sockfd,{WebSocketProtocol::CLOSE,nullptr}});
        //mAppContext.onDisconnect(wid,sockfd);
      } catch (const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Can't enqueue request to application %s, exception: %s",mName.c_str(),e.what());
      }
    }
    void execute()
    {
      while(mMayRun)
      { 
        try
        {
          auto te=mEvents.recv();
          if(te.event.type == WebSocketProtocol::CLOSE)
            mAppContext.onDisconnect(te.wid,te.sockfd);
          else
          {
            const bool exec_result=mAppContext.onMessage(te.wid,te.sockfd,te.event);
            if(!exec_result)
            {
              getWorker(te.wid)->submitError(te.sockfd);
            }
          }
        }catch(std::exception& e)
        {
          mMayRun.store(false);
          itc::getLog()->error(__FILE__,__LINE__,"Exception in Application[%s]::execute(): %s",mName.c_str(),e.what());
          itc::getLog()->flush();
        }
      }
      mAppContext.clearCache();
      mCanStop.store(true);
      itc::getLog()->info(__FILE__,__LINE__,"Application instance [%s] main loop is finished.",mName.c_str());
    }
    
    Application()=delete;
    Application(const Application&)=delete;
    Application(Application&)=delete;
    ~Application() noexcept
    {
      mMayRun.store(false);
      itc::getLog()->info(__FILE__,__LINE__,"Application instance [%s] is destroyed.",mName.c_str());
      itc::getLog()->flush();
    }
  };
}

#endif /* __APPLICATION_H__ */

