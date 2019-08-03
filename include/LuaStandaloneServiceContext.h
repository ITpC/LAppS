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
 *  $Id: LuaStandaloneServiceContext.h September 24, 2018 10:26 AM $
 * 
 **/


#ifndef __LUASTANDALONESERVICECONTEXT_H__
#  define __LUASTANDALONESERVICECONTEXT_H__

#include <abstract/LuaServiceContext.h>
#include <ext/json.hpp>
#include <ServiceRegistry.h>


extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdexcept>
  
  LUA_API int must_stop(lua_State* L)
  {
    static thread_local std::atomic<bool>* mustStopAddr{nullptr};
    
    if(mustStopAddr == nullptr)
    {
      lua_getglobal(L,"instance_id");
      const char *instance_id_str=lua_tostring(L,-1);
      size_t instance_id=0;
      sscanf(instance_id_str, "%zu", &instance_id);
      try
      {
        mustStopAddr=LAppS::SServiceRegistry::getInstance()->findById(instance_id)->get_stop_flag_address();
      }catch(const std::exception& e)
      {
      }
    }
    else if(mustStopAddr->load())
    {
      lua_pushboolean(L,true);
      return 1;
    }
    lua_pushboolean(L,false);
    return 1;
  }
}

namespace LAppS
{
  class LuaStandaloneServiceContext : public abstract::LuaServiceContext
  {
   private:
    std::atomic<bool> mCanStop;
    std::atomic<bool> mMustStop;
    
    
    const bool isServiceModuleValid()
    {
      bool ret=false;

      lua_getglobal(mLState, this->getName().c_str());

      lua_getfield(mLState, -1, "init");
      ret=lua_isfunction(mLState,-1);
      if(ret){
       lua_pop(mLState,1);
      }
      else{
       lua_pop(mLState,1);
       return false;
      }

      lua_getfield(mLState, -1, "run");
      ret=lua_isfunction(mLState,-1);
      if(ret){
       lua_pop(mLState,1);
      }
      else{
       lua_pop(mLState,1);
       return false;
      }
      return ret;
   }
    
   public:
    
    LuaStandaloneServiceContext(const std::string& name, const size_t instance_id)
    : abstract::LuaServiceContext(name),mCanStop{false},mMustStop{false}
    {
      init_bcast_module(mLState);
      
      lua_pushstring(mLState,std::to_string(instance_id).c_str());
      lua_setglobal(mLState,"instance_id");
      
      lua_pushcfunction(mLState,must_stop);
      lua_setglobal(mLState,"must_stop");
      
      if(require(this->getName()))
      {
        itc::getLog()->info(
          __FILE__,__LINE__,
          "Lua standalone Service-Module %s is registered",
          this->getName().c_str()
        );
        
        if(!isServiceModuleValid())
        {
          throw std::system_error(
            EINVAL,
            std::system_category(),
            this->getName()+" is not a valid LAppS standalone Service-Module, one or more required methods are not provided [init(), run()]"
          );
        }
        else
        {
          itc::getLog()->info(
            __FILE__,__LINE__,
            "%s is a valid standalone Service (e.a. provides required methods, though there is no warranty it works properly).",
            this->getName().c_str()
          );
        }
      }
      else
      {
        itc::getLog()->error(__FILE__,__LINE__,"Can't load Service %s, it is either does not exist or invalid",this->getName().c_str());
        throw std::logic_error("Invalid Service "+this->getName());
      }
      
    }
    
    std::atomic<bool>* get_stop_flag_address()
    {
      return &mMustStop;
    }
    
    void init()
    {
      mMustStop.store(false);
      mCanStop.store(false);
      lua_getglobal(mLState, this->getName().c_str());
      lua_getfield(mLState, -1, "init");
      int ret = lua_pcall(mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"init");
      cleanLuaStack();
      itc::getLog()->info(__FILE__,__LINE__,"Application instance [%s] is initialized",this->getName().c_str());
    }
    
    void stop()
    {
      mMustStop.store(true);
    }
    
    void run()
    {
      lua_getglobal(mLState, this->getName().c_str());
      lua_getfield(mLState, -1, "run");
      int ret = lua_pcall(mLState, 0, 0, 0);
      try {
        checkForLuaErrorsOnPcall(ret,"run");
      } catch (const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Runtime exception in service [%s] in context::run(): %s",this->getName().c_str(),e.what());
      }
      cleanLuaStack();
      itc::getLog()->info(__FILE__,__LINE__,"Service context [%s] is down",this->getName().c_str());
      itc::getLog()->flush();
      mCanStop.store(true);
    }
    
    const ::LAppS::ServiceProtocol getProtocol() const
    {
      return ::LAppS::ServiceProtocol::INTERNAL;
    }
    
    const bool isRunning() const
    {
      return !mCanStop.load();
    }
    
    void shutdown()
    {
      if(isRunning())
      {
        this->stop();
      }
    }
    ~LuaStandaloneServiceContext() noexcept
    {
      this->shutdown();
      while(isRunning())
      {
        itc::sys::sched_yield(5000);
      }
    }
  };
}

#endif /* __LUASTANDALONESERVICECONTEXT_H__ */

