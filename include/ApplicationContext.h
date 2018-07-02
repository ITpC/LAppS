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


#include <WebSocket.h>
#include <abstract/Worker.h>
#include <abstract/ApplicationContext.h>

#include <modules/nljson.h>
#include <modules/wsSend.h>
#include <modules/bcast.h>

using json = nlohmann::json;


namespace LAppS
{
  template <bool TLSEnable, bool StatsEnable, ApplicationProtocol Tproto>
  class ApplicationContext : public ::abstract::ApplicationContext
  {
  private:
    typedef std::shared_ptr<::abstract::Worker> WorkerSPtrType;
    typedef WebSocket<TLSEnable,StatsEnable> WSType;
    typedef std::shared_ptr<WSType> WSSPtrType;
    
    enum LAppSInMessageType { INVALID, CN, CN_WITH_PARAMS, REQUEST, REQUEST_WITH_PARAMS };
    itc::utils::Int2Type<Tproto> mProtocol;
    abstract::Application* mParent;
    std::atomic<bool>      mustStop;

    
    void callAppOnMessage()
    {
      int ret = lua_pcall (mLState, 3, 1, 0);
      checkForLuaErrorsOnPcall(ret,"onMessage");
    }
    
    const bool isAppModuleValid()
    {
      bool ret=false;
      // validate presence of required methods: onStart,onMessage,onShutdown
      lua_getglobal(mLState, mName.c_str());
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
      
      lua_getfield(mLState, -1, "onMessage");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      
      lua_getfield(mLState, -1, "onDisconnect");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      cleanLuaStack();
      return true;
    }
    
    void shutdownApp()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "onShutdown");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"onShutdown");
    }
    
    void startApp()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "onStart");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"onStart");
      cleanLuaStack();
    }
    static const LAppSInMessageType getLAppSInMessageType(const json& msg)
    {
      auto lapps=msg.find("lapps");
      // is it a proper protocol version?
      if((lapps!=msg.end()) && (!lapps.value().is_null()) && (lapps.value().is_number()) && lapps.value() == 1 )
      {
        auto method=msg.find("method");
        if(method!=msg.end())
        {
          if((!method.value().is_null())&&method.value().is_string())
          {
            const std::string& method_name=method.value();
            if(method_name.size()>0)
            {
              if(method_name[0] != '_')
              {
                if(method_name.find('.') != std::string::npos)
                  return INVALID;


                bool hasParams=false;

                auto params=msg.find("params");
                if(params != msg.end())
                {
                  if((!params.value().is_null()) && params.value().is_array() && (params.value().size() > 0))
                    hasParams=true;
                }

                auto cid=msg.find("cid");
                if(cid!=msg.end()) // Client's Notification
                {
                  if(cid.value().is_number())
                    if(hasParams)
                      return CN_WITH_PARAMS;
                    else return CN;
                  else return INVALID;
                }
                else // REQUEST
                {
                  if(hasParams)
                    return REQUEST_WITH_PARAMS;
                  else return REQUEST;
                }
              }
              else return INVALID;
            }
            else return INVALID;
          }
          else return INVALID;
        }
        else return INVALID;
      }
      else return INVALID;
    }
  
    const bool onMessage(const abstract::AppInEvent& event, const itc::utils::Int2Type<ApplicationProtocol::RAW>& protocol_is_raw)
    {
      cleanLuaStack();
      
      lua_getfield(mLState, LUA_GLOBALSINDEX, mName.c_str());
      lua_getfield(mLState,-1,"onMessage");
      
      lua_pushinteger(mLState, (lua_Integer)(event.websocket.get()));
      lua_pushinteger(mLState, event.opcode);
      lua_pushlstring(mLState,(const char*)(event.message->data()),event.message->size());
      
      callAppOnMessage(); 
      
      uint32_t argc=lua_gettop(mLState);
      
      if(argc != 2)
      {
        throw std::system_error(
          EINVAL,std::system_category(),
          mName+"::onMessage() is returned "+std::to_string(argc-1)+
          " values, but only one boolean value is expected"
        );
      }
      
      event.websocket->returnBuffer(event.message);
      
      if(lua_isboolean(mLState,argc))
      {
        return lua_toboolean(mLState,argc);
      }
      return false;
    }
    
    void pushRequest(const std::shared_ptr<json>& request)
    {
      auto udjsptr=static_cast<UDJSPTR**>(lua_newuserdata(mLState,sizeof(UDJSPTR*)));
      // allocate UDJSPTR and store its address to allocated block
      (*udjsptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
      // make JSPTR by parsing the string and store it to *(Userdata->ptr);
      *((*udjsptr)->ptr)=request;
      luaL_getmetatable(mLState, "nljson");
      lua_pushcfunction(mLState, destroy);
      lua_setfield(mLState, -2, "__gc");
      lua_setmetatable(mLState, -2);
    }
    
    const bool onMessage(const abstract::AppInEvent& event, const itc::utils::Int2Type<ApplicationProtocol::LAPPS>& protocol_is_lapps)
    {
      // exceptions from json and from lua stack MUST be handled in the Application class
      // We must prevent possibility to kill app with inappropriate message, therefore 
      // content errors may not throw the exceptions.
      
      // Protocol violation, LAppS protocol accepts only binary messages (CBOR encoded)
      if(event.opcode != WebSocketProtocol::OpCode::BINARY)
        return false;
      
      cleanLuaStack();
      
      auto msg=std::make_shared<json>(json::from_cbor(*event.message));

      auto msg_type=getLAppSInMessageType(*msg);

      // "Protocol violation, LAppS protocol accepts only binary messages (CBOR encoded)"
      if(msg_type == INVALID)
        return false;

      lua_getfield(mLState, LUA_GLOBALSINDEX, mName.c_str());
      lua_getfield(mLState,-1,"onMessage");

      lua_pushinteger(mLState,(lua_Integer)(event.websocket.get())); // socket handler for ws::send
      lua_pushinteger(mLState,msg_type);
      pushRequest(msg);
      
      

      callAppOnMessage(); // handler, type, userdata

      uint32_t argc=lua_gettop(mLState);

      if(argc != 2)
      {
        throw std::system_error(
          EINVAL,std::system_category(),
          mName+"::onMessage() is returned "+std::to_string(argc)+
          " values, but only one boolean value is expected"
        );
      }
      
      event.websocket->returnBuffer(std::move(event.message));
      
      if(lua_isboolean(mLState,argc))
      {
        return lua_toboolean(mLState,argc);
      }
      return false;
    }
    void add_bcast(const itc::utils::Int2Type<ApplicationProtocol::LAPPS>& protocol_is_lapps)
    {
      luaopen_bcast(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"bcast");
      cleanLuaStack();
    }
    void add_bcast(const itc::utils::Int2Type<ApplicationProtocol::RAW>& protocol_is_raw)
    {
    }
    
public:
    ApplicationContext(const ApplicationContext&)=delete;
    ApplicationContext(ApplicationContext&)=delete;
    
    explicit ApplicationContext(const std::string& appname, abstract::Application* parent)
    : ::abstract::ApplicationContext(appname), mParent(parent),mustStop{false}
    {
      luaL_openlibs(mLState);
      
      luaopen_nljson(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"nljson");
      
      luaopen_wssend(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"ws");
      
      add_bcast(mProtocol);
            
      if(require(mName))
      {
        itc::getLog()->info(__FILE__,__LINE__,"Lua module %s is registered",mName.c_str());
        if(isAppModuleValid())
        {
          itc::getLog()->info(
            __FILE__,__LINE__,
            "Lua module %s is a valid application (e.a. provides required methods, though there is no warranty it works properly)",
            mName.c_str()
          );
          try {
            this->startApp();
          }catch(const std::exception& e)
          {
            itc::getLog()->error(
              __FILE__,__LINE__,
              "Unable to start application %s, because of an exception: %s",
              mName.c_str(),e.what()
            );
            throw;
          }
        }
        else
        {
          itc::getLog()->error(__FILE__,__LINE__,"Lua module %s is not a valid LAppS application",mName.c_str());
          throw std::logic_error("Invalid application "+mName);
        }
      }
      cleanLuaStack();
    }
    void shutdown()
    {
      if(mLState)
      {
        try{
          shutdownApp();
        }catch(const std::exception& e){
          itc::getLog()->error(__FILE__,__LINE__,"Exception: %s",e.what());
        }
        lua_close(mLState);
        mLState=nullptr;
      }
    }
    
    virtual ~ApplicationContext() noexcept
    {
      if(mLState)
      {
        this->shutdown();
      }
    }
    const bool onMessage(const abstract::AppInEvent& event)
    {
      if(mustStop) return false;
      return onMessage(std::move(event),mProtocol);
    }
    
    void onDisconnect(const std::shared_ptr<::abstract::WebSocket>& ws)
    {
      this->cleanLuaStack();
      
      lua_getfield(mLState, LUA_GLOBALSINDEX, mName.c_str());
      lua_getfield(mLState,-1,"onDisconnect");
      lua_pushinteger(mLState,(lua_Integer)(ws.get()));
      
      int ret = lua_pcall (mLState, 1, 0, 0);
      checkForLuaErrorsOnPcall(ret,"onDisconnect");
    }
  };
}

#endif /* __APPLICATIONCONTEXT_H__ */

