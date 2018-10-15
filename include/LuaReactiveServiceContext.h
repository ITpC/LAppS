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
 *  $Id: LuaReactiveServiceContext.h September 20, 2018 10:10 AM $
 * 
 **/


#ifndef __LUAREACTIVESERVICECONTEXT_H__
#  define __LUAREACTIVESERVICECONTEXT_H__

#include <atomic>
#include <memory>

#include <Val2Type.h>
#include <abstract/LuaServiceContext.h>
#include <ContextTypes.h>
#include <AppInEvent.h>

#include <ext/json.hpp>
using json = nlohmann::json;



namespace LAppS
{
  template <ServiceProtocol Tproto> 
  class LuaReactiveServiceContext : public abstract::LuaServiceContext
  {
   private:
    enum      LAppSInMessageType { INVALID, CN, CN_WITH_PARAMS, REQUEST, REQUEST_WITH_PARAMS };
    
    itc::utils::Int2Type<Tproto> mProtocol;
    std::atomic<bool>            mustStop;
    
    
    void callAppOnMessage()
    {
      int ret = lua_pcall (mLState, 3, 1, 0);
      checkForLuaErrorsOnPcall(ret,"onMessage");
    }

    const bool isServiceModuleValid()
    {
      bool ret=false;
      // validate presence of required methods: onStart,onMessage,onShutdown
      lua_getglobal(mLState, this->getName().c_str());
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
    
    void startService()
    {
      lua_getglobal(mLState, this->getName().c_str());
      lua_getfield(mLState, -1, "onStart");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"onStart");
      cleanLuaStack();
    }
    
    void shutdownService()
    {
      lua_getglobal(mLState, this->getName().c_str());
      lua_getfield(mLState, -1, "onShutdown");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"onShutdown");
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
    
    
    void init_bcast_module(const itc::utils::Int2Type<ServiceProtocol::LAPPS>& protocol_is_lapps)
    {
      ::init_bcast_module(mLState);
    }
    
    void init_bcast_module(const itc::utils::Int2Type<ServiceProtocol::RAW>& protocol_is_raw)
    {
    }
    
    void init_bcast_module(const itc::utils::Int2Type<ServiceProtocol::INTERNAL>& protocol_is_internal)
    {
    }
    
    
    
    const bool onMessage(const AppInEvent& event, const itc::utils::Int2Type<ServiceProtocol::RAW>& protocol_is_raw)
    {
      cleanLuaStack();
      
      lua_getfield(mLState, LUA_GLOBALSINDEX, this->getName().c_str());
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
          this->getName()+"::onMessage() is returned "+std::to_string(argc-1)+
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
    
    void pushRequest(const std::shared_ptr<json>& request)
    {
      auto udjsptr=static_cast<UDJSPTR**>(lua_newuserdata(mLState,sizeof(UDJSPTR*)));
      // allocate UDJSPTR and store its address to allocated block
      (*udjsptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
      // make JSPTR by parsing the string and store it to *(Userdata->ptr);
      *((*udjsptr)->ptr)=std::move(request);
      luaL_getmetatable(mLState, "nljson");
      lua_pushcfunction(mLState, destroy);
      lua_setfield(mLState, -2, "__gc");
      lua_setmetatable(mLState, -2);
    }
    
    const bool onMessage(const AppInEvent& event, const itc::utils::Int2Type<ServiceProtocol::LAPPS>& protocol_is_lapps)
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

      lua_getfield(mLState, LUA_GLOBALSINDEX, this->getName().c_str());
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
          this->getName()+"::onMessage() is returned "+std::to_string(argc)+
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
    
    const bool onMessage(const AppInEvent& event, const itc::utils::Int2Type<ServiceProtocol::INTERNAL>& protocol_is_internal)
    {
      itc::getLog()->error(__FILE__,__LINE__,"Internal protocol is not allowed for Reactive Services");
      return false;
    }
    void init()
    {
       init_ws_module(mLState);
      
      init_bcast_module(mProtocol);
      
      
      if(require(this->getName()))
      {
        itc::getLog()->info(__FILE__,__LINE__,"Lua module %s is registered",this->getName().c_str());
        if(isServiceModuleValid())
        {
          itc::getLog()->info(
            __FILE__,__LINE__,
            "Lua module %s is a valid application (e.a. provides required methods, though there is no warranty it works properly)",
            this->getName().c_str()
          );
          try {
            this->startService();
          }catch(const std::exception& e)
          {
            itc::getLog()->error(
              __FILE__,__LINE__,
              "Unable to start application %s, because of an exception: %s",
              this->getName().c_str(),e.what()
            );
            throw;
          }
        }
        else
        {
          itc::getLog()->error(__FILE__,__LINE__,"Lua module %s is not a valid LAppS application",this->getName().c_str());
          throw std::logic_error("Invalid application "+this->getName());
        }
      }
      cleanLuaStack();
    }
    
    void shutdown()
    {
      if(mLState)
      {
        try{
          shutdownService();
          itc::getLog()->info(__FILE__,__LINE__,"Service-module %s is down",this->getName().c_str());
        }catch(const std::exception& e){
          itc::getLog()->error(__FILE__,__LINE__,"Can't stop Service-module [%s], exception: %s",this->getName().c_str(),e.what());
        }
      }
    }
    
   public:
    LuaReactiveServiceContext()=delete;
    LuaReactiveServiceContext(LuaReactiveServiceContext&)=delete;
    LuaReactiveServiceContext(const LuaReactiveServiceContext&)=delete;
    
    explicit LuaReactiveServiceContext(
      const std::string& name
    ) : abstract::LuaServiceContext(name), mustStop{false}
    {
      static_assert(Tproto != ServiceProtocol::INTERNAL, "LuaReactiveServiceContext does not supports INTERNAL protocol");
      this->init();
    }
    
    const bool onMessage(const AppInEvent& event)
    {
      if(mustStop) return false;
      return onMessage(std::move(event),mProtocol);
    }
    
    void onDisconnect(const std::shared_ptr<::abstract::WebSocket>& ws)
    {
      this->cleanLuaStack();
      
      lua_getfield(mLState, LUA_GLOBALSINDEX, this->getName().c_str());
      lua_getfield(mLState,-1,"onDisconnect");
      lua_pushinteger(mLState,(lua_Integer)(ws.get()));
      
      int ret = lua_pcall (mLState, 1, 0, 0);
      checkForLuaErrorsOnPcall(ret,"onDisconnect");
    }
    
    ~LuaReactiveServiceContext() noexcept
    {
      this->shutdown();
    }
    
    const ServiceProtocol getProtocol() const
    {
      return Tproto;
    }
  };
}

#endif /* __LUAREACTIVESERVICECONTEXT_H__ */

