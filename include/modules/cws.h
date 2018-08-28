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
 *  $Id: cws.h August 13, 2018 3:37 PM $
 * 
 **/


#ifndef __CWS_H__
#  define __CWS_H__

#include <ClientWebSocket.h>
#include <WSClientsPool.h>


extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
  
  
  static thread_local LAppS::WSClientPool WSCPool;
  static thread_local std::vector<epoll_event> events(1000);
  
  static const bool is_error_event(const uint32_t event)
  {
    return (event & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
  }
  
  static const bool is_in_event(const uint32_t events)
  {
    return (events & EPOLLIN);
  }
  
  static const bool is_out_event(const uint32_t events)
  {
    return (events & EPOLLOUT);
  }
  
  static const bool is_inout_event(const uint32_t events)
  {
    return (events & (EPOLLIN|EPOLLOUT));
  }
  
  static void clearStack(lua_State *L)
  {
    lua_pop(L,lua_gettop(L));
  }
  
  static void onPCallErrorCheck(lua_State *L,const int retcode, const std::string& func)
  {
    if(retcode)
    {
      std::string errmsg{"Error on calling function "+func+(lua_tostring(L,lua_gettop(L)))};
      std::cerr << errmsg << std::endl;
    }
  }
  
  static void callOnError(lua_State *L, const int32_t fd, const std::string& message)
  {
    clearStack(L);
    luaL_getmetatable(L,"cws");
    lua_getfield(L,1,std::to_string(fd).c_str());
    lua_getfield(L,2,"onerror");
    lua_pushinteger(L,fd);
    if(!lua_isnil(L,3))
    {
      if(message.empty())
        onPCallErrorCheck(L,lua_pcall(L,1,0,0),"onerror");
      else
      {
        lua_pushlstring(L,message.data(),message.length());
        onPCallErrorCheck(L,lua_pcall(L,2,0,0),"onerror");
      }
    }
    else
    {
      std::cout << "onerror is nil" << std::endl;
    }
    clearStack(L);
  }
  
  static void cws_remove_handler(lua_State *L, const int32_t fd)
  {
    clearStack(L);
    luaL_getmetatable(L,"cws");
    lua_getfield(L,1,std::to_string(fd).c_str());
    lua_pushnil(L);
    lua_pushnil(L);
    lua_setfield(L,2,std::to_string(fd).c_str());
    lua_setmetatable(L,1);
    clearStack(L);
  }
  
  bool onEvent(lua_State *L,const LAppS::ClientWebSocketSPtr& ws, const WSEvent& ref)
  {
    switch(ref.type)
    {
      case WebSocketProtocol::TEXT:
        if(WSStreamProcessing::WSStreamParser::isValidUtf8(ref.message->data(),ref.message->size()))
        {
          // onmessage
          clearStack(L);
          luaL_getmetatable(L,"cws");
          lua_getfield(L,1,std::to_string(ws->getfd()).c_str());
          lua_getfield(L,2,"onmessage");
          lua_pushinteger(L,ws->getfd());
          lua_pushlstring(L,(const char*)(ref.message->data()),ref.message->size());
          lua_pushinteger(L,ref.type);
          if(!lua_isnil(L,3))
          {
            onPCallErrorCheck(L,lua_pcall(L,3,0,0),"onmessage");
          }
          else{
            std::cout << "onmessage is nil" << std::endl;
          }
          clearStack(L);
          return true;        
        }
        else 
        {
          ws->closeSocket(WebSocketProtocol::BAD_DATA);
          cws_remove_handler(L,ws->getfd());
          WSCPool.remove(ws->getfd());
          return false;
        }
      case WebSocketProtocol::BINARY:
      {
        //onmessage
        clearStack(L);
        luaL_getmetatable(L,"cws");
        lua_getfield(L,1,std::to_string(ws->getfd()).c_str());
        lua_getfield(L,2,"onmessage");
        lua_pushinteger(L,ws->getfd());
        lua_pushlstring(L,(const char*)(ref.message->data()),ref.message->size());
        lua_pushinteger(L,ref.type);
        if(!lua_isnil(L,3))
        {
          onPCallErrorCheck(L,lua_pcall(L,3,0,0),"onmessage");
        }
        else{
          std::cout << "onmessage is nil" << std::endl;
        }
        clearStack(L);
        return true;
      }
      break;
      case WebSocketProtocol::CLOSE:
      {
        ws->onCloseMessage(ref);
        // onclose
        clearStack(L);
        luaL_getmetatable(L,"cws");
        lua_getfield(L,1,std::to_string(ws->getfd()).c_str());
        lua_getfield(L,2,"onclose");
        lua_pushinteger(L,ws->getfd());
        
        if(ref.message->size()>0)
          lua_pushlstring(L,(const char*)(ref.message->data()),ref.message->size());
        
        
        if(!lua_isnil(L,3))
        {
          if(ref.message->size()>0)
          {
            onPCallErrorCheck(L,lua_pcall(L,2,0,0),"onclose");
          }
          else
          {
            onPCallErrorCheck(L,lua_pcall(L,1,0,0),"onclose");
          }
        }
        else
        {
          std::cerr << "onclode is nil" << std::endl;
        }
        cws_remove_handler(L,ws->getfd());
        WSCPool.remove(ws->getfd());
      }
      return false;
      case WebSocketProtocol::PING:
        ws->sendPong(ref);
        return true;
      case WebSocketProtocol::PONG: 
        // RFC 6455: A response to an unsolicited Pong frame is not expected.
        return true;
      default:
      {
        ws->closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
        callOnError(L,ws->getfd(),"Protocol violation, - unknown frame type");
      }
      return false;
    }
    return true;
  }
  
  
  LUA_API int cws_methods(lua_State *L)
  {
    luaL_getmetatable(L, "cws");
    lua_getfield(L,-1,lua_tostring(L,2));
    
    return 1;
  }

  LUA_API int cws_send(lua_State *L)
  {
    const int argc=lua_gettop(L);
    if((argc!=4)||(!lua_isnumber(L,argc-2))||(!lua_isstring(L,argc-1))||(!lua_isnumber(L,argc)))
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,"Usage: cws:send(handler,data,opcode), where handler is the handler provided by onmessage(), data is a string, opcode is an integer value equal to 1 or 2");
      return 2;
    }
    
    size_t datalength=0;
    int32_t fd=lua_tointeger(L,argc-2);
    uint8_t opcode=lua_tointeger(L,argc);
    if((opcode != 1) && (opcode !=2))
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,"Usage: cws:send(handler,data,opcode), where handler is the handler provided by onmessage(), data is a string, opcode is an integer value equal to 1 or 2");
      return 2;
    }
    const uint8_t* data=(const uint8_t*)(lua_tolstring(L,argc-1,&datalength));
    
    int32_t ret=WSCPool[fd]->send(data,datalength,static_cast<WebSocketProtocol::OpCode>(opcode));
    if(ret>0)
    {
      lua_pushboolean(L,true);
      return 1;
    }else{
      WSCPool[fd]->setCommError();
      lua_pushboolean(L,false);
      lua_pushstring(L,"Communication error");
      return 2;
    }
  }
  
  LUA_API int cws_create(lua_State *L)
  {
    const int argc=lua_gettop(L);
    
    if((argc!=3)||(!lua_istable(L,argc))||(!lua_isstring(L,argc-1)))
    {
      lua_pushnil(L);
      lua_pushstring(L,"Usage: cws:create(URI,callbacks) - where URI is a string, and the `callbacks' is the table consisting of user functions implementing WebSocket's Web API: onopen, onclose, onmessage, onerror");
      return 2;
    }
    
    auto udptr=static_cast<int32_t*>(lua_newuserdata(L,sizeof(int32_t)));
    std::string uri{lua_tostring(L,argc-1)};
    try
    {
      (*udptr)=WSCPool.create(std::move(uri));
      
      luaL_getmetatable(L,"cws");
      lua_pushvalue(L,3);
      lua_setfield(L,-2,std::to_string(*udptr).c_str());
      lua_setmetatable(L, -2);
      return 1;
    }
    catch(const std::exception& e)
    {
      lua_pushnil(L);
      std::string message("Can't create socket: ");
      message.append(e.what());
      lua_pushstring(L,message.c_str());
      return 2;
    }
    return 0;
  }

  
  
  static void callOnClose(lua_State *L, const int32_t fd)
  {
    clearStack(L);
    luaL_getmetatable(L,"cws");
    lua_getfield(L,1,std::to_string(fd).c_str());
    lua_getfield(L,2,"onclose");
    lua_pushinteger(L,fd);
    
    if(!lua_isnil(L,3))
    {
      onPCallErrorCheck(L,lua_pcall(L,1,0,0),"onclose");
    }
    else
    {
      std::cout << "onclose is nil" << std::endl;
    }
    clearStack(L);
  }
  
  static void callOnOpen(lua_State *L, const int32_t fd)
  {
    std::string strfd{std::to_string(fd)};
    
    clearStack(L);
    luaL_getmetatable(L,"cws");
    lua_getfield(L,1,strfd.c_str());
    lua_getfield(L,2,"onopen");
    if(!lua_isnil(L,3))
    {
      lua_pushinteger(L,fd);
      onPCallErrorCheck(L,lua_pcall(L,1,0,0),"onopen");
    }else
    {
      std::cout << "onopen is nil" << std::endl;
    }
    clearStack(L);
  }
  LUA_API int cws_eventloop(lua_State *L)
  {
    size_t ret=WSCPool.poll(events);
    for(size_t i=0;i<ret;++i)
    {
      const epoll_event& event=events[i];
      
      if(is_error_event(event.events))
      {
        callOnError(L,event.data.fd, "Communication error, socket closed by peer");
        cws_remove_handler(L,event.data.fd);
        WSCPool.remove(event.data.fd);
      }
      else if(is_in_event(event.events))
      {
        auto it=WSCPool.find(event.data.fd);
        
        if(it!=WSCPool.end())
        {
          auto client_ws=it->second;
          again:
          switch(client_ws->handleInput())
          {
            case LAppS::WSClient::InputStatus::FORCE_CLOSE:
              callOnClose(L,event.data.fd);
              cws_remove_handler(L,event.data.fd);
              WSCPool.remove(event.data.fd);
            break;
            case LAppS::WSClient::InputStatus::ERROR:
              callOnError(L,event.data.fd,"Communication error on read from socket, - it is already closed");
              cws_remove_handler(L,event.data.fd);
              WSCPool.remove(event.data.fd);
            break;
            case LAppS::WSClient::InputStatus::NEED_MORE_DATA:
              WSCPool.mod_in(event.data.fd);
            break;
            case LAppS::WSClient::InputStatus::MESSAGE_READY_BUFFER_IS_EMPTY:
            {
              auto wsevent{client_ws->getMessage()};
              onEvent(L,client_ws,wsevent);
              WSCPool.mod_in(event.data.fd);
            }
            break;
            case LAppS::WSClient::InputStatus::MESSAGE_READY_BUFFER_IS_NOT_EMPTY:
            {
              auto wsevent{client_ws->getMessage()};
              onEvent(L,client_ws,wsevent);
              WSCPool.mod_in(event.data.fd);
            }
            case LAppS::WSClient::InputStatus::CALL_ONCE_MORE:
              goto again;
            break;
            case LAppS::WSClient::InputStatus::UPGRADED:
            {
              callOnOpen(L,event.data.fd);
              WSCPool.mod_in(event.data.fd);
            }
            break;
          }
        }
      }
    }
    return 0;
  }
  
  
  LUA_API int luaopen_cws(lua_State *L)
  {
    static const struct luaL_reg members[] = {
      {"new", cws_create},
      {"send", cws_send},
      {"eventLoop",cws_eventloop},
      {"__index",cws_methods},
      {nullptr,nullptr}
    };
    
    luaL_newmetatable(L,"cws");
    luaL_openlib(L, NULL, members,0);
    return 1;
  }
}

#endif /* __CWS_H__ */

