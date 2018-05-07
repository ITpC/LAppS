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
 *  $Id: nap.h May 7, 2018 12:27 PM $
 * 
 **/


#ifndef __NAP_H__
#  define __NAP_H__


extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
  
#include <sys/Nanosleep.h>
  
}

extern "C"
{
  LUA_API int nap_destroy(lua_State *L)
  {
    auto nap_ptr=static_cast<itc::sys::Nap**>(luaL_checkudata(L, lua_gettop(L), "nap"));
    if(nap_ptr)
    {
      delete *nap_ptr;
    }
    return 0;
  }
  
  
  LUA_API int nap_usleep(lua_State *L)
  {
    size_t argc=lua_gettop(L);
    
    if(argc!=2)
    {
      lua_pushstring(L,"Usage: nap:usleep(microseconds) - amount of microseconds to sleep, microseconds must be an unsigned integer.");
      return 1;
    }
    
    auto nap_ptr=static_cast<itc::sys::Nap**>(luaL_checkudata(L, argc-1, "nap"));
    if(nap_ptr)
    {
      if(lua_isnumber(L,argc))
      {
        size_t usecs=lua_tointeger(L,argc);
        static_cast<itc::sys::Nap*>(*nap_ptr)->usleep(usecs);
      }
    }
    
    return 0;
  }
  
  LUA_API int nap_nsleep(lua_State *L)
  {
    size_t argc=lua_gettop(L);
    
    if(argc!=2)
    {
      lua_pushstring(L,"Usage: nap:nsleep(nanoseconds) - amount of nanoseconds to sleep, nanoseconds must be an unsigned integer");
      return 1;
    }
    
    auto nap_ptr=static_cast<itc::sys::Nap**>(luaL_checkudata(L, argc-1, "nap"));
    if(nap_ptr)
    {
      if(lua_isnumber(L,argc))
      {
        size_t nsecs=lua_tointeger(L,argc);
        static_cast<itc::sys::Nap*>(*nap_ptr)->nanosleep(nsecs);
      }
    }
    
    return 0;
  }
  
  LUA_API int nap_sleep(lua_State *L)
  {
    size_t argc=lua_gettop(L);
    
    if(argc!=2)
    {
      lua_pushstring(L,"Usage: nap:sleep(seconds) - amount of seconds to sleep, seconds must be an unsigned integer");
      return 1;
    }
    
    auto nap_ptr=static_cast<itc::sys::Nap**>(luaL_checkudata(L, argc-1, "nap"));
    if(nap_ptr)
    {
      if(lua_isnumber(L,argc))
      {
        size_t secs=lua_tointeger(L,argc);
        static_cast<itc::sys::Nap*>(*nap_ptr)->sleep(secs);
      }
    }
    
    return 0;
  }
  
  LUA_API int nap_create(lua_State *L)
  {
    auto udptr=static_cast<itc::sys::Nap**>(lua_newuserdata(L,sizeof(itc::sys::Nap*)));
    (*udptr)=new itc::sys::Nap();

    luaL_getmetatable(L, "nap");
    lua_pushcfunction(L, nap_destroy);
    lua_setfield(L,-2, "__gc");
    lua_setmetatable(L, -2);
    
    return 1;
  }  
  
  LUA_API int nap_methods(lua_State *L)
  {
    luaL_getmetatable(L, "nap");
    lua_getfield(L,-1,lua_tostring(L,2));
    
    return 1;
  }
  
  LUA_API int luaopen_nap(lua_State *L)
  {
    static const struct luaL_reg members[] = {
      {"new", nap_create},
      {"sleep",nap_sleep},
      {"usleep",nap_usleep},
      {"nsleep",nap_nsleep},
      {"__index",nap_methods},
      {nullptr,nullptr}
    };
    luaL_newmetatable(L,"nap");
    luaL_openlib(L, NULL, members,0);
    return 1;
  }
}

#endif /* __NAP_H__ */

