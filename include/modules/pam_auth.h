/*
 * Copyright (C) 2018 Pavel Kraynyukhov <pavel.kraynyukhov@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * $Id: LAppS:pam_auth.h Jun 25, 2018 12:27:11 PM $
 * 
 */


#ifndef __PAM_AUTH_H__
#define __PAM_AUTH_H__

#include <memory>
#include <vector>
#include <security/pam_appl.h>

using json = nlohmann::json;


extern "C" 
{
  #include <lua.h>
  #include <lualib.h>
  #include <lauxlib.h>
  #include <stdio.h>
  #include <map>
  
  /**
   * @param service - mandatory
   * @param username - mandatory
   * @param password - mandatory
   * @returns true on successful authentication, otherwise false.
   * @returns false,usage message in case of inappropriate use.
   */
  LUA_API int pam_auth(lua_State *L)
  {
    static const char* usage="Usage: boolean[,string] pam_auth:check(string service,string username,string password) returns true on successful authentication, otherwise false.";
    pam_handle_t *pamh=NULL;
    int ret;
    
    auto argc=lua_gettop(L);
    if(argc!=4)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    
    bool args_proper=lua_isnumber(L,argc)&&lua_isnumber(L,argc-1)&&lua_isnumber(L,argc-2);
    
    if(!args_proper)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    
    const char *service=lua_tostring(L,argc-2);
    const char *username=lua_tostring(L,argc-1);
    const char *password=lua_tostring(L,argc);
    
    
    
    return 1;
  }
  
  LUA_API int luaopen_pam_auth(lua_State *L)
  {
    static const struct luaL_reg members[] = {
      {"check", pam_auth},
      {nullptr,nullptr}
    };
    luaL_newmetatable(L,"pam_auth");
    luaL_openlib(L, NULL, members,0);


    return 1;
  }
}

#endif /* __PAM_AUTH_H__ */

