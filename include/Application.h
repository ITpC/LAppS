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
    std::atomic<bool> mMayRun;
    std::mutex mMutex;
    std::string mName;
    std::string mTarget;
    ApplicationContext<TLSEnable,StatsEnable,Tproto> mAppContext;
    itc::tsbqueue<TaggedEvent> mEvents;
    
    
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

    }
    void shutdown()
    {

    }
    
    explicit Application(const std::string& appName,const std::string& target)
    : mMayRun(true),mMutex(),mName(appName), mTarget(target),mAppContext(appName)
    {
     
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
    
    void execute()
    {
      while(mMayRun)
      { 
        try
        {
          auto te=mEvents.recv();
          auto tmsg=mAppContext.onMessage(te.wid,te.sockfd,te.event);
          itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->get(tmsg.wid)->getRunnable()->submitResponse(tmsg);
        }catch(std::exception& e)
        {
          mMayRun=false;
          itc::getLog()->error(__FILE__,__LINE__,"exception in Application::execute(): %s",e.what());
          itc::getLog()->info(__FILE__,__LINE__,"Application going down (it seems that server shutting down too)");
          itc::getLog()->flush();
        }
      }
    }
    
    Application()=delete;
    Application(const Application&)=delete;
    Application(Application&)=delete;
    ~Application()
    {
    }
    
  };
}

#endif /* __APPLICATION_H__ */

