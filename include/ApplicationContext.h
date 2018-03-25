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
 *  $Id: ApplicationContext.h March 21, 2018 3:36 PM $
 * 
 **/


#ifndef __APPLICATIONCONTEXT_H__
#  define __APPLICATIONCONTEXT_H__
#include <ext/json.hpp>
#include <abstract/Application.h>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdexcept>

}

#include <modules/nljson.h>
#include <WebSocket.h>

using json = nlohmann::json;


namespace LAppS
{
  template <bool TLSEnable, bool StatsEnable, ApplicationProtocol Tproto>
  class ApplicationContext
  {
  public:
    
  private:
    itc::utils::Int2Type<Tproto> mProtocol;
    std::mutex mMutex;
    std::string mName;
    lua_State* mLState;
    abstract::Application* mParent;

    void callAppOnMessage()
    {
      lua_getfield (mLState, LUA_GLOBALSINDEX, mName.c_str());
      lua_pushstring (mLState, "onMessage");
      int ret = lua_pcall (mLState, 1, 1, 0);
      if(ret != 0)
      {
        switch(ret)
        {
          case LUA_ERRRUN:
            throw std::runtime_error("Lua runtime error on calling method "+mName+"::onMessage()");
            break;
          case LUA_ERRMEM:
            throw std::system_error(ENOMEM,std::system_category(),"Not enough memory to call method "+mName+"::onMessage()");
          case LUA_ERRERR:
            throw std::logic_error("Error in lua error handler on calling method "+mName+"::onMessage()");
            break;
        }
      }
    }
    const bool require(const std::string& module_name)
    {
      lua_getfield (mLState, LUA_GLOBALSINDEX, "require");
      lua_pushstring (mLState, module_name.c_str());
      int ret = lua_pcall (mLState, 1, 1, 0);
      if(ret != 0)
      {
        switch(ret)
        {
          case LUA_ERRRUN:
            throw std::runtime_error("Lua runtime error on loading module "+module_name);
            break;
          case LUA_ERRMEM:
            throw std::system_error(ENOMEM,std::system_category(),"Not enough memory to load module "+module_name);
          case LUA_ERRERR:
            throw std::logic_error("Error in lua error handler on loading module "+module_name);
            break;
        }
      }
      lua_setfield(mLState, LUA_GLOBALSINDEX, module_name.c_str());
      return true;
    }
    
    const bool isAppModuleValid(const std::string& module_name)
    {
      bool ret=false;
      // validate presence of required methods: onStart,onMessage,onShutdown
      lua_getglobal(mLState, module_name.c_str());
      lua_getfield(mLState, -1, "onStart");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      
      lua_getfield(mLState, -1, "onMessage");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      
      lua_getfield(mLState, -1, "onShutdown");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      return true;
    }
    void shutdownApp()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "onShutdown");
      int ret = lua_pcall (mLState, 0, 0, 0);
      if(ret != 0)
      {
        switch(ret)
        {
          case LUA_ERRRUN:
            throw std::runtime_error("Lua runtime error on calling "+mName+".onShutdown()");
            break;
          case LUA_ERRMEM:
            throw std::system_error(ENOMEM,std::system_category(),"Not enough memory to call "+mName+".onShutdown()");
          case LUA_ERRERR:
            throw std::logic_error("Error in lua error handler on calling "+mName+".onShutdown()");
            break;
        }
      }
    }
    
    void startApp()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "onStart");
      int ret = lua_pcall (mLState, 0, 0, 0);
      if(ret != 0)
      {
        switch(ret)
        {
          case LUA_ERRRUN:
            throw std::runtime_error("Lua runtime error on calling "+mName+".onStart()");
            break;
          case LUA_ERRMEM:
            throw std::system_error(ENOMEM,std::system_category(),"Not enough memory to call "+mName+".onStart()");
          case LUA_ERRERR:
            throw std::logic_error("Error in lua error handler on calling "+mName+".onStart()");
            break;
        }
      }
    }
    
  
    const TaggedEvent onMessage(const uint8_t workerID, const int32_t socketfd, const WSEvent& event, const itc::utils::Int2Type<ApplicationProtocol::RAW>& protocol_is_raw)
    {
      lua_pushlstring(mLState,(const char*)(event.message->data()),event.message->size());
      callAppOnMessage(); // expecting only one result
      
      uint32_t argc=lua_gettop(mLState);
      if(argc != 1)
      {
        throw std::system_error(EINVAL,std::system_category(),mName+"::onMessage() returned "+std::to_string(argc)+",but only one result was expected");
      }
      size_t len;
      const char *str=lua_tolstring(mLState,-1,&len);
      auto ret=std::make_shared<MSGBufferType>(len);
      memcpy(ret->data(),str,len);
      return {workerID,socketfd,{event.type,ret}};
    }
    const TaggedEvent onMessage(const uint8_t workerID, const int32_t socketfd, const WSEvent& event, const itc::utils::Int2Type<ApplicationProtocol::LAPPS>& protocol_is_lapps)
    {
      auto retval=std::make_shared<MSGBufferType>(0);
      
      if(event.type != WebSocketProtocol::OpCode::BINARY)
        throw std::system_error(EINVAL,std::system_category(),"Protocol violation, LAppS protocol accepts only binary messages");
            
      try
      {
        auto udjsptr=static_cast<UDJSPTR**>(lua_newuserdata(mLState,sizeof(UDJSPTR*)));
        // allocate UDJSPTR and store its address to allocated block
        (*udjsptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
        // make JSPTR by parsing the string and store it to *(Userdata->ptr);
        auto toValidate=std::make_shared<json>(json::from_cbor(event.message.get()->data()));
        // TODO: Validate LAppS protocol Request or Notification
        *((*udjsptr)->ptr)=toValidate;
        luaL_getmetatable(mLState, "nljson");
        lua_pushcfunction(mLState, destroy);
        lua_setfield(mLState, -2, "__gc");
        lua_setmetatable(mLState, -2);
        
        callAppOnMessage(); // expecting userdata object (1 value only);
        uint32_t argc=lua_gettop(mLState);
        if(argc > 1)
        {
          throw std::system_error(EINVAL,std::system_category(),mName+"::onMessage() returned "+std::to_string(argc)+",but zero or one result were expected");
        }
        switch(argc)
        {
          case 0:
          {
            (*retval)=json::to_cbor(json("{}"));
          }
          break;
          case 1:
          {
            void *ptr=luaL_checkudata(mLState,-1,"nljson");
          
            if((*static_cast<UDJSON**>(ptr))->type == SHARED_PTR)
            {
              auto udptr=static_cast<UDJSPTR**>(ptr);
              auto jsptr=(*udptr)->ptr; // std::shared_ptr<json>*
              (*retval)=json::to_cbor(**jsptr);
            }
            else
            {
              auto udptr=static_cast<UDJSON**>(ptr);
              auto jsptr=(*udptr)->ptr; // json*
              (*retval)=json::to_cbor(*jsptr);
            }
          }
          break;
        }
      }
      catch( const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Exception on client's message CBOR parsing, socket will be closed for the protocol violation");
        throw std::system_error(EINVAL,std::system_category(),"Protocol violation, LAppS accepts only valid CBOR encoded messages");
      }
      return {workerID,socketfd,{event.type,retval}};
    }
public:
    ApplicationContext(const ApplicationContext&)=delete;
    ApplicationContext(ApplicationContext&)=delete;
    
    explicit ApplicationContext(const std::string& appname)
    : mName(appname), mLState(luaL_newstate()) /**,mParent(parent)**/
    {
      /**
      if(mParent == nullptr)
        throw std::logic_error("Can not instantiate ApplicationContext without an instance of Application class");
      **/
      
      luaL_openlibs(mLState);
      
      luaopen_nljson(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"nljson");
      
      if(require(mName))
      {
        itc::getLog()->info("Lua module %s is registered",mName.c_str());
        if(isAppModuleValid(mName))
        {
          itc::getLog()->info("Lua module %s is a valid application (e.a. provides required methods, though there is no warranty it works properly)",mName.c_str());
          try {
            startApp();
          }catch(const std::exception& e)
          {
            itc::getLog()->error(__FILE__,__LINE__,"Unable to start application %s, because of an exception: ",mName.c_str(),e.what());
            throw;
          }
        }
      }
    }
    void shutdown()
    {
      try{
        shutdownApp();
      }catch(const std::exception& e){
        itc::getLog()->error(__FILE__,__LINE__,"Exception: %s",e.what());
      }
      lua_close(mLState);
      mLState=nullptr;
    }
    ~ApplicationContext()
    {
      if(mLState)
      {
        this->shutdown();
      }
    }
    const TaggedEvent onMessage(const uint8_t workerID, const int32_t socketfd, const WSEvent& event)
    {
      return onMessage(workerID,socketfd,event,mProtocol);
    }
  };
}

#endif /* __APPLICATIONCONTEXT_H__ */

