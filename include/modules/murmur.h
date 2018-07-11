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
 *  $Id: murmur.h July 9, 2018 1:38 AM $
 * 
 **/


#ifndef __MURMUR_H__
#  define __MURMUR_H__

#include <functional>
#include <string>

extern "C" {
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
  #include <stdlib.h>
  #include <stdio.h>

  LUA_API int hashstr(lua_State *L)
  {
    int argc=lua_gettop(L);
    if(argc != 1)
    {
      lua_pushfstring(L,"Usage: murmur.hashstr(number|string); returns string. args now: ",argc);
      lua_error(L);
    }
    int type=lua_type(L,argc);
    switch(type)
    {
      case LUA_TSTRING:
      {
        std::string tmp(lua_tostring(L,argc));

        long long unsigned int digest=std::hash<std::string>{}(std::move(tmp));
        
        std::string result(16,'\0');
        
        snprintf((char*)(result.c_str()),16,"%016llx",digest);
        lua_pushlstring(L,result.c_str(),result.length());
      }
      break;
      case LUA_TNUMBER:
      {
        size_t tmp=static_cast<size_t>(lua_tonumber(L,argc));
        
        long long unsigned int digest=std::hash<std::size_t>{}(tmp);
        
        std::string result(16,'\0');
        
        snprintf((char*)(result.c_str()),16,"%016llx",digest);
        lua_pushlstring(L,result.c_str(),result.length());
      }
      break;
      default:
        lua_pushstring(L,"Usage: murmur.hashstr(number|string); returns string");
        lua_error(L);
    }
    return 1;
  }
  
  LUA_API int hashnum(lua_State *L)
  {
    int argc=lua_gettop(L);
    if(argc != 1)
    {
      lua_pushfstring(L,"Usage: murmur.hashnum(number|string); returns size_t. args now: ",argc);
      lua_error(L);
    }
    int type=lua_type(L,argc);
    switch(type)
    {
      case LUA_TSTRING:
      {
        std::string tmp(lua_tostring(L,argc));

        long long unsigned int digest=std::hash<std::string>{}(std::move(tmp));
        lua_pushinteger(L,digest);
      }
      break;
      case LUA_TNUMBER:
      {
        size_t tmp=static_cast<size_t>(lua_tonumber(L,argc));
        long long unsigned int digest=std::hash<size_t>{}(tmp);
        lua_pushinteger(L,digest);
      }
      break;
      default:
        lua_pushstring(L,"Usage: murmur.hashnum(number|string); returns size_t");
        lua_error(L);
    }
    return 1;
  }

  LUA_API int luaopen_murmur(lua_State *L)
  {
    luaL_Reg functions[]= {
      {"hashstr", hashstr },
      {"hashnum", hashnum },
      {nullptr,nullptr}
    };
    luaL_register(L,"murmur",functions);
    return 1;
  }
}


#endif /* __MURMUR_H__ */

