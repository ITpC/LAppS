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
    constexpr ApplicationProtocol getProtocol() const
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
    : mName(appName), mTarget(target),mAppContext(appName)
    {
    }
    
    void enqueue(const TaggedEvent& event)
    {
      mEvents.send(event);
    }
    
    void execute()
    {
      while(mMayRun)
      {
        SyncLock sync(mMutex);
        TaggedEvent te;
        try
        {
          mEvents.recv(te);
          mAppContext.onMessage(te.wid,te.sockfd,te.event);
        }
        catch(std::exception& e)
        {
          itc::getLog()->info(__FILE__,__LINE__,"Queue goes down, stopping the application");
          mAppContext.shutdown();
          mMayRun=false;
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

