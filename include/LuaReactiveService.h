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
 *  $Id: LuaReactiveService.h September 22, 2018 4:00 PM $
 * 
 **/


#ifndef __LUAREACTIVESERVICE_H__
#  define __LUAREACTIVESERVICE_H__

#include <string>

#include <LuaReactiveServiceContext.h>
#include <abstract/ReactiveService.h>
#include <ContextTypes.h>
#include <AppInEvent.h>
#include <NetworkACL.h>

#include <ext/json.hpp>

using json = nlohmann::json;

namespace LAppS
{
  template <ServiceProtocol TProto> class LuaReactiveService : public abstract::ReactiveService
  {
   private:
    using qtype=itc::tsbqueue<AppInEvent,itc::sys::mutex>;
    
    size_t                              mMaxInMsgSize;
    std::atomic<bool>                   mMayRun;
    std::atomic<bool>                   mCanStop;
    LuaReactiveServiceContext<TProto>   mContext;
    qtype                               mEvents;
    NetworkACL                          mACL;
    
    
   public:
    LuaReactiveService(
      const std::string& name,
      const std::string& target,
      const size_t mims,
      const Network_ACL_Policy& _policy, 
      const json& policy_exclude
    )
    : abstract::ReactiveService(target), mMaxInMsgSize{mims}, 
      mMayRun{true}, mCanStop{false}, mContext{name},
      mEvents(), mACL{_policy}
    {
      for(auto it=policy_exclude.begin();it!=policy_exclude.end();++it)
      {
        mACL.add(std::move(LAppS::addrinfo(it.value().get<std::string>())));
      }
    }
   
    const bool filterIP(const uint32_t address) const
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

    ~LuaReactiveService()
    {
      if(mMayRun.load())
      {
        this->shutdown();
      }
    }
      
    const std::string& getName() const
    {
      return mContext.getName();
    }
    const ServiceLanguage getLanguage() const
    {
      return mContext.getLanguage();
    }
    const ServiceProtocol getProtocol() const
    {
      return mContext.getProtocol();
    }
    
    const bool isUp() const
    {
      return mMayRun.load()&&(!mCanStop.load());
    }
    
    const bool isDown() const
    {
      return mCanStop.load();
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
    
    void onCancel()
    {
      this->shutdown();
    }
    
    void shutdown()
    {
       mMayRun.store(false);
    }
    
    void enqueue(const AppInEvent&& event)
    {
      try {
        mEvents.send(std::move(event));
      }
      catch (const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Can't enqueue request to application %s, exception: %s",this->getName().c_str(),e.what());
      }
    }
    
    void execute()
    {
      sigset_t sigpipe_mask;
      sigemptyset(&sigpipe_mask);
      sigaddset(&sigpipe_mask, SIGPIPE);
      sigset_t saved_mask;
      pthread_sigmask(SIG_BLOCK, &sigpipe_mask, &saved_mask);
      
      while(mMayRun.load())
      { 
        
        std::queue<AppInEvent> events;
        try{
          mEvents.recv<qtype::SWAP>(events);
        }catch(const std::exception& e)
        {
          mMayRun.store(false);
          itc::getLog()->info(__FILE__,__LINE__,"Instance [%u] of the service [%s] is going down",this->getInstanceId(),this->getName().c_str());
          break;
        }
        try
        {
          while(!events.empty())
          {
            auto event=std::move(events.front());
            events.pop();
            switch(event.opcode)
            {
              case WebSocketProtocol::OpCode::CLOSE:
                mContext.onDisconnect(event.websocket);
                if(event.websocket->getState() == ::abstract::WebSocket::State::MESSAGING)
                  try{
                    event.websocket->send(*event.message);
                    event.websocket->close();
                  }catch(const std::exception& e)
                  {// ignore errors (bad_weak_ptr and so one)
                  }
                break;
              case WebSocketProtocol::OpCode::PONG:
                if(event.websocket->getState() == ::abstract::WebSocket::State::MESSAGING)
                  try{
                    event.websocket->send(*event.message);
                  }catch(const std::exception& e)
                  {// ignore errors (bad_weak_ptr and so one)
                  }
                break;
              default:
              {
                const bool exec_result=mContext.onMessage(event);
                if(!exec_result)
                {
                  mMayRun.store(false);
                }
              }
            }
          }
        }catch(const std::exception& e)
        {
          mMayRun.store(false);
          itc::getLog()->error(__FILE__,__LINE__,"Exception in the instance [%u] of service [%s]::execute(): %s",getInstanceId(), this->getName().c_str(),e.what());
          itc::getLog()->flush();
        }
      }
      itc::getLog()->info(__FILE__,__LINE__,"Instance [%u] of the service [%s]: execution finished",getInstanceId(), this->getName().c_str());
      mCanStop.store(true);
    }
  };
}

#endif /* __LUAREACTIVESERVICE_H__ */

