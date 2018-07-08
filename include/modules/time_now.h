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
 *  $Id: time_now.h July 9, 2018 1:51 AM $
 * 
 **/


#ifndef __TIME_NOW_H__
#  define __TIME_NOW_H__


#include <chrono>

extern "C" {
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>

  LUA_API int now(lua_State *L)
  {

    uint64_t now=std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch()
    ).count();

    lua_pushinteger(L,now);
    return 1;
  }

  LUA_API int luaopen_time(lua_State *L)
  {
    luaL_Reg functions[]= {
      {"now", now },
      {nullptr,nullptr}
    };
    luaL_register(L,"time",functions);
    return 1;
  }
}


#endif /* __TIME_NOW_H__ */

