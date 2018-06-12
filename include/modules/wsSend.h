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
 *  $Id: wsSend.h April 17, 2018 1:59 PM $
 * 
 **/


#ifndef __WSSEND_H__
#  define __WSSEND_H__

#include <exception>
#include <errno.h>
#include <memory>
#include <vector>

#include <abstract/Worker.h>
#include <modules/UserDataAdapter.h>
#include <WSServerMessage.h>

#include <ext/json.hpp>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
}

using json = nlohmann::json;

const bool isLAppSOutMessageValid(const json& msg)
{
  
  auto cid=msg.find("cid");
  
  if(cid == msg.end()) return false;
  
  if(cid.value().is_null()) return false;
  
  if(cid.value().is_number())
  {
  
    auto status=msg.find("status");
    if(status!=msg.end())
    {
      bool valid=status.value().is_number();
      if(valid && ((status.value() == 1) || (status.value() == 0)))
      {
        if(status.value() == 0)
        {
          if(cid.value() != 0) return false;
          
          auto error=msg.find("error");
          if(error!=msg.end())
          {
            auto error_code=error.value().find("code");
            auto error_message=error.value().find("message");
            valid =  (error_code != error.value().end())&&(!error_code.value().is_null()&&(error_code.value().is_number()));
            valid = valid && (error_message != error.value().end()) && (!error_message.value().is_null()) && error_message.value().is_string();
            return valid; // No checks for additional members. No idea if I need to invalidate messages with additional members
          }
          else return false;
        }
        else // not an error message
        {
          if(cid.value() == 0) // response object
          {
            auto result=msg.find("result");
            if(result != msg.end())
            {
              if(result.value().is_null()) return false;
              if(result.value().is_array()) return true;
              return false;
            }
            else
            {
              return false;
            }
          }
          else // OON object
          {
            auto message=msg.find("message");
            if(message!=msg.end())
            {
              if(message.value().is_null()) return false;
              if(message.value().is_array()) return true;
              return false;
            }
            return false;
          }
        }
      } else return false;
    } else return false;
  } else return false;
}

int wssend_raw(lua_State* L, abstract::WebSocket* handler)
{
  const int tpidx=3;
  const int udidx=4;
  
  if(lua_isstring(L,udidx)) // protocol::RAW 
  {
    try {
      WebSocketProtocol::OpCode opcode=WebSocketProtocol::OpCode::CLOSE;
      if(lua_isnumber(L,tpidx))
      {
        opcode=static_cast<WebSocketProtocol::OpCode>(lua_tointeger(L,tpidx));
      }
      else
      {
        lua_pushboolean(L,false);
        lua_pushstring(L,"Usage: ws::send(handler, opcode, string), opcode is not integer");
        return 2;
      }
      const bool opcode_valid=(opcode == 1)||(opcode == 2);
      
      if(!opcode_valid)
      {
        lua_pushboolean(L,false);
        lua_pushstring(L,"Usage: ws::send(handler, opcode, string), opcode is not TEXT or BINARY");
        return 2;
      }

      size_t len;
      const char* msg=lua_tolstring(L,udidx,&len);
      
      if(handler->mustAutoFragment())
      {
        WebSocketProtocol::FragmentedServerMessage::msgQType msgqueue;
        
        WebSocketProtocol::FragmentedServerMessage(msgqueue,opcode,msg,len);
        while(!msgqueue.empty())
        {
          handler->send(std::move(*msgqueue.front()));
          msgqueue.pop();
        }
      }
      else
      {
        MSGBufferType message;
        WebSocketProtocol::ServerMessage(message,opcode,msg,len);
        handler->send(std::move(message));
      }
  
      lua_pushboolean(L,true);
      return 1;
    }catch(const std::exception& e)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,e.what());
      return 2;
    }
  }else{
    lua_pushboolean(L,false);
    lua_pushstring(L,"Usage: ws::send(handler, opcode, string), string is not provided");
    return 2;
  }
}
int wssend_lapps(lua_State* L, abstract::WebSocket* handler)
{
  const int udidx=3;
  if(lua_isuserdata(L,udidx)) // protocol::LAPPS
  {
    try {
      auto opcode=WebSocketProtocol::BINARY;
      
      
      const json& msg=get_userdata_value(L,udidx);
      if(isLAppSOutMessageValid(msg))
      {
        if(handler->mustAutoFragment())
        {
          WebSocketProtocol::FragmentedServerMessage::msgQType msgqueue;

          WebSocketProtocol::FragmentedServerMessage(msgqueue,opcode,json::to_cbor(msg));
          
          while(!msgqueue.empty())
          {
            handler->send(std::move(*msgqueue.front()));
            msgqueue.pop();
          }
        }
        else
        {
          MSGBufferType message;
          WebSocketProtocol::ServerMessage(message,opcode,json::to_cbor(msg));
          handler->send(std::move(message));
        }
        lua_pushboolean(L,true);
        return 1;
      }else{
        lua_pushboolean(L,false);
        lua_pushstring(L,"An attempt to send an invalid LAppS-protocol message");
        return 2;
      }
    }catch(const std::exception& e)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,e.what());
      return 2;
    }
  }else
  {
    lua_pushboolean(L,false);
    lua_pushstring(L,"Usage: ws::send(handler, nljson), - userdata object of nljson type is not provided");
    return 2;
  }
}
// Lua interface: ws::close(handler,error_code [, err_string])
// 
int wsclose(lua_State*L,abstract::WebSocket* handler, const size_t argc)
{
  const int udidx=3;
  if(lua_isnumber(L,udidx)) // protocol::LAPPS
  {
    uint16_t close_code=lua_tointeger(L,udidx);
    
    try {
      MSGBufferType message;
      
      if(close_code>999&&((close_code < 1012)||((close_code>2999)&&(close_code<5000))))
      {
        if(argc == 3)
        {
          WebSocketProtocol::ServerCloseMessage(message,close_code);
        }else if(argc == 4)
        {
          if(lua_isstring(L,4))
          {
            size_t len;
            const char *errmsg=lua_tolstring(L,argc,&len);
            WebSocketProtocol::ServerCloseMessage(message,close_code,errmsg,len);
          }
          else
          {
            lua_pushboolean(L,false);
            lua_pushstring(L,"Usage: ws:close(handler, error_code [, error_string]) - error_string argument must be of a type string");
            return 2;
          }
        }else{
          // wrong call with number arguments not equal 3 or 4
          lua_pushboolean(L,false);
          lua_pushstring(L,"Usage: ws:close(handler, error_code [, error_string]) - wrong number of arguments is provided");
          return 2;
        }
        handler->send(std::move(message));
        lua_pushboolean(L,true);
        return 1;
      }
      else
      {
        // unsupported error code
        lua_pushboolean(L,false);
        lua_pushstring(L,"Unsupported error code is provided for ws:close()");
        return 2;
      }      
    }catch(const std::exception& e)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,e.what());
      return 2;
    }
  }else
  {
    lua_pushboolean(L,false);
    lua_pushstring(L,"Usage: ws::close(handler, error_code [, error_string]), - handler must be an integer");
    return 2;
  }
}
extern "C" {
    LUA_API int wsclose(lua_State* L)
    {
      size_t argc=lua_gettop(L);
      
      switch(argc)
      {
        case 3:
        case 4:
        {
          int hdidx=2;
          if(lua_isnumber(L, hdidx))
          {
            auto handler=(abstract::WebSocket*)(lua_tointeger(L,hdidx));
            return wsclose(L,handler,argc);
          }
          else{
            lua_pushboolean(L,false);
            lua_pushstring(L,"Usage: ws::close(handler, error_code [, error_string]), - handler is not integer");
            return 2;
          }
        }
        break;
        default:
          lua_pushboolean(L,false);
          lua_pushstring(L,"Usage: ws::close(handler, error_code [, error_string]), - inappropriate number of arguments used");
          return 2;
      }
    }
    
    LUA_API int wssend(lua_State* L)
    {
      size_t argc=lua_gettop(L);

      switch(argc)
      {
        case 3:
        {
          int hdidx=2;
          if(lua_isnumber(L, hdidx))
          {
            abstract::WebSocket* handler=(abstract::WebSocket*)(lua_tointeger(L,hdidx));
            return wssend_lapps(L,handler);
          }
          else{
            lua_pushboolean(L,false);
            lua_pushstring(L,"Usage: ws::send(handler, nljson), handler is not integer");
            return 2;
          }
        }
        break;
        case 4:
        {
          int hdidx=2;
          if(lua_isnumber(L, hdidx))
          {
            abstract::WebSocket* handler=(abstract::WebSocket*)lua_tointeger(L,hdidx);
            return wssend_raw(L,handler);
          }
          else{
            lua_pushboolean(L,false);
            lua_pushstring(L,"Usage: ws::send(handler, opcode, string), handler is not integer");
            return 2;
          }
        }
        break;
        default:
          lua_pushboolean(L,false);
          lua_pushstring(L,"Usage: ws::send(handler, (opcode, string) || nljson), - inappropriate number of arguments used");
          return 2;
      }
    }
    
    LUA_API int luaopen_wssend(lua_State *L)
    {
      static const struct luaL_reg members[] = {
        {"send",wssend},
        {"close", wsclose},
        {nullptr,nullptr}
      };
      
      luaL_newmetatable(L,"ws");
      luaL_openlib(L, NULL, members,0);

      return 1;
    }
}

#endif /* __WSSEND_H__ */

