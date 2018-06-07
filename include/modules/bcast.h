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
 *  $Id: bcast.h May 3, 2018 10:08 AM $
 * 
 **/


#ifndef __BCAST_H__
#  define __BCAST_H__

#include <memory>

#include <Broadcasts.h>
#include <WSServerMessage.h>

#include <ext/json.hpp>
#include <modules/UserDataAdapter.h>

using json = nlohmann::json;


extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
#include <map>
}

static thread_local std::map<size_t,std::shared_ptr<LAppS::BCastType::BCastValueType>> broadcasts;


const bool isLAppSBcastMessageValid(const json& msg)
{
  
  auto cid=msg.find("cid");
  
  if(cid == msg.end()) return false;
  
  if(cid.value().is_null()) return false;
  
  if(cid.value().is_number())
  {
    auto message=msg.find("message");
    if(message!=msg.end())
    {
      bool valid=message.value().is_array();
      return valid;
    }
    return false;
  }
  return false;
}

auto find_bcast(const size_t bcastid)
{
  auto it=broadcasts.find(bcastid);
  
  if(it!=broadcasts.end())
  {
    return it->second;
  }
  
  auto bcast_addr=LAppS::BroadcastRegistry::getInstance()->find(bcastid);
  broadcasts.emplace(bcastid,bcast_addr);
  return bcast_addr;
}

extern "C"
{
  LUA_API int bcast_send(lua_State *L)
  {
    static const char* usage="Usage: boolean[,string] bcast:send(id,message), where `id' is a unique identifier of the broadcast channel. The `id' must be an unsigned integer. message - is a userdata of nljson type. Returns: true on success, false on error. Optionally an error message is returned.";
    
    auto argc=lua_gettop(L);
    
    if(argc!=3)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    
    if(lua_isnumber(L,argc-1))
    {
      try {
        const json& message=get_userdata_value(L,argc);
        
        if(isLAppSBcastMessageValid(message))
        {
          size_t bcastid=static_cast<size_t>(lua_tointeger(L,argc-1));
          auto bcast_addr=find_bcast(bcastid);
          auto msg=std::make_shared<MSGBufferType>();
          WebSocketProtocol::ServerMessage(msg,WebSocketProtocol::BINARY,json::to_cbor(message));
          bcast_addr->bcast(msg);
          lua_pushboolean(L,true);
          return 1;
        }else{
          lua_pushboolean(L,false);
          return 1;
        }
      }catch(const std::exception& e)
      {
        lua_pushboolean(L,false);
        lua_pushstring(L,e.what());
        return 2;
      }
    }
    lua_pushboolean(L,false);
    lua_pushstring(L,usage);
    return 2;
  }
  
  LUA_API int bcast_subscribe(lua_State *L)
  {
    static const char* usage="Usage: boolean[,string] bcast:subscribe(id,handler), where `id' is a unique identifier of the broadcast channel. `handler' is a IO handler provided for onMessage() method of your application. The `id' and the `handler' must be an unsigned integers. Returns: true on success, false on error. Optionally an error message is returned";
    
    auto argc=lua_gettop(L);
    if(argc!=3)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    if(lua_isnumber(L,argc)&&lua_isnumber(L,argc-1))
    {
      auto handler=(abstract::WebSocket*)(lua_tointeger(L,argc));
      size_t bcastid=static_cast<size_t>(lua_tointeger(L,argc-1));
      try{
        auto bcast_addr=find_bcast(bcastid);
        bcast_addr->subscribe(handler);
        lua_pushboolean(L,true);
        return 1;
        
      }catch(const std::exception& e){
        lua_pushboolean(L,false);
        lua_pushstring(L,e.what());
        return 2;
      }
    }
    lua_pushboolean(L,false);
    lua_pushstring(L,usage);
    return 2;
  }
  
  LUA_API int bcast_unsubscribe(lua_State *L)
  {
    static const char* usage="Usage: boolean[,string] bcast:unsubscribe(id,handler), where `id' is a unique identifier of the broadcast channel. `handler' is a IO handler provided for onMessage() method of your application. The `id' and the `handler' must be an unsigned integers. Returns: true on success, false on error. Optionally this message if the usage is wrong.";
    
    auto argc=lua_gettop(L);
    if(argc!=3)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    if(lua_isnumber(L,argc)&&lua_isnumber(L,argc-1))
    {
      auto handler=(abstract::WebSocket*)(lua_tointeger(L,argc));
      size_t bcastid=static_cast<size_t>(lua_tointeger(L,argc-1));
      try{
        auto bcast_addr=find_bcast(bcastid);
        bcast_addr->unsubscribe(handler);
        lua_pushboolean(L,true);
        return 1;
        
      }catch(const std::exception& e){
        lua_pushboolean(L,false);
        lua_pushstring(L,e.what());
        return 2;
      }
    }
    lua_pushboolean(L,false);
    lua_pushstring(L,usage);
    return 2;
  }
  LUA_API int bcast_create(lua_State *L)
  {
    static const char* usage="Usage: bcast:create(id), where `id' is a unique identifier of the broadcast channel. The `id' must be an unsigned integer.";
    auto argc=lua_gettop(L);
    if(argc!=2)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    
    if(lua_isnumber(L,argc))
    {
      size_t bcastid=lua_tointeger(L,argc);
      LAppS::BroadcastRegistry::getInstance()->create(bcastid);
      
      auto bcast_addr=find_bcast(bcastid);
      if(!bcast_addr)
      {
        lua_pushboolean(L,false);
        return 1;
      }
      lua_pushboolean(L,true);
      return 1;
    }
    
    lua_pushboolean(L,false);
    lua_pushstring(L,usage);
    return 2;
  }
  
  LUA_API int luaopen_bcast(lua_State *L)
    {
      static const struct luaL_reg members[] = {
        {"create", bcast_create},
        {"subscribe", bcast_subscribe},
        {"unsubscribe", bcast_unsubscribe},
        {"send", bcast_send},
        {nullptr,nullptr}
      };
      luaL_newmetatable(L,"bcast");
      luaL_openlib(L, NULL, members,0);


      return 1;
    }
}

#endif /* __BCAST_H__ */

