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

//#include <WSWorkersPool.h>
#include <Singleton.h>
#include <abstract/Worker.h>

#include <ext/json.hpp>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdexcept>

#include "NetworkACL.h"
}


using json = nlohmann::json;

namespace LAppS
{
  template <bool TLSEnable, bool StatsEnable, ApplicationProtocol Tproto> 
  class Application : public ::abstract::Application
  {
  private:
    typedef std::shared_ptr<::abstract::Worker> WorkerSPtrType;
    
    std::atomic<bool>                                 mMayRun;
    std::atomic<bool>                                 mCanStop;
    size_t                                            mMaxInMsgSize;
    
    LAppS::NetworkACL                                 mACL;
    
    std::string                                       mName;
    std::string                                       mTarget;
    
    ApplicationContext<TLSEnable,StatsEnable,Tproto>  mAppContext;
    itc::tsbqueue<abstract::AppInEvent>               mEvents;
    
 public:
    const bool isUp() const
    {
      return mMayRun;
    }
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
       mCanStop.store(true);
    }
    void shutdown()
    {
       mMayRun.store(false);
    }
    
    explicit Application(
      const std::string& appName,
      const std::string& target,
      const size_t mims,const LAppS::Network_ACL_Policy& _policy, 
      const json& policy_exclude
    ): abstract::Application(), mMayRun{true},  mCanStop{false},  mMaxInMsgSize(mims),        mACL(_policy),
        mName(appName), mTarget(target),  mAppContext(appName,this),  mEvents()
    {
      for(auto it=policy_exclude.begin();it!=policy_exclude.end();++it)
      {
        std::string exclude=it.value();
        mACL.add(std::move(LAppS::addrinfo(std::move(exclude))));
      }
    }
    
    const bool filter(const uint32_t address)
    {
      switch(mACL.mPolicy)
      {
        case LAppS::Network_ACL_Policy::ALLOW:
        {
          if(mACL.match(address))
            return true;
          return false;
        } break;
        case LAppS::Network_ACL_Policy::DENY:
        {
          if(mACL.match(address))
            return false;
          return true;
        } break;
      }
      return false;
    }
    
    
    const size_t getMaxMSGSize() const
    {
      return mMaxInMsgSize;
    }
    
    const bool try_enqueue(const std::vector<abstract::AppInEvent>& events)
    {
      try
      {
        if(mEvents.try_send(events))
          return true;
        else return false;
      }
      catch(const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Can't enqueue request to application %s, exception: %s",mName.c_str(),e.what());
        return false;
      }
    }
    void enqueue(const abstract::AppInEvent& event)
    {
      try {
        mEvents.send(event);
      }
      catch (const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Can't enqueue request to application %s, exception: %s",mName.c_str(),e.what());
      }
    }
    
    void enqueue(const std::vector<abstract::AppInEvent>& event)
    {
      try {
        mEvents.send(event);
      }
      catch (const std::exception& e)
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
          std::queue<abstract::AppInEvent> events;
          mEvents.recv(events);
          while(!events.empty())
          {
            auto event=std::move(events.front());
            events.pop();
            switch(event.opcode)
            {
              case WebSocketProtocol::OpCode::CLOSE:
                mAppContext.onDisconnect(event.websocket);
                if(event.websocket->getState() == abstract::WebSocket::State::MESSAGING)
                  try{
                    event.websocket->send(*event.message);
                    event.websocket->close();
                  }catch(const std::exception& e)
                  {// ignore errors (bad_weak_ptr and so one)
                  }
                break;
              case WebSocketProtocol::OpCode::PONG:
                if(event.websocket->getState() == abstract::WebSocket::State::MESSAGING)
                  try{
                    event.websocket->send(*event.message);
                  }catch(const std::exception& e)
                  {// ignore errors (bad_weak_ptr and so one)
                  }
                break;
              default:
              {
                const bool exec_result=mAppContext.onMessage(event);
                if(!exec_result)
                {
                  mMayRun.store(false);
                }
              }
            }
          }
        }catch(std::exception& e)
        {
          mMayRun.store(false);
          itc::getLog()->error(__FILE__,__LINE__,"Exception in Application[%s]::execute(): %s",mName.c_str(),e.what());
          itc::getLog()->flush();
        }
      }
      mCanStop.store(true);
      itc::getLog()->info(__FILE__,__LINE__,"Application instance [%s] main loop is finished.",mName.c_str());
    }
    
    Application()=delete;
    Application(const Application&)=delete;
    Application(Application&)=delete;
    ~Application() noexcept
    {
      mMayRun.store(false);
    }
  };
}

#endif /* __APPLICATION_H__ */

