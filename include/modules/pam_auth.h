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
#include <functional>

#include <security/pam_appl.h>

using json = nlohmann::json;

static thread_local std::vector<pam_response> __resp(1);

int lapps_pam_conversation(int n, const struct pam_message **msg, struct pam_response **resp, void *data)
{
  if (n!=1)
    return PAM_CONV_ERR;

  __resp[0].resp_retcode = 0;
  __resp[0].resp=strdup(static_cast<const char*>(data));
  *resp = __resp.data();

  return PAM_SUCCESS;
}

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
    static const char* usage="Usage: boolean[,string] pam_auth:login(string service,string username,string password) returns true on successful authentication, otherwise false. In case of inappropriate usage returns this error message as well.";
    
    pam_handle_t *pamh=NULL;
    int ret;
    
    auto argc=lua_gettop(L);
    if(argc!=4)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    
    bool args_proper=((!lua_isnil(L,argc))&&(!lua_isnil(L,argc-1))&&(!lua_isnil(L,argc-2)));
    args_proper=args_proper&&lua_isstring(L,argc)&&lua_isstring(L,argc-1)&&lua_isstring(L,argc-2);
    
    if(!args_proper)
    {
      lua_pushboolean(L,false);
      lua_pushstring(L,usage);
      return 2;
    }
    
    const char *service=lua_tostring(L,argc-2);
    const char *username=lua_tostring(L,argc-1);
    
    
    struct pam_conv conv = {
      lapps_pam_conversation,
      (char*)(lua_tostring(L,argc))
    };

    
    ret = pam_start(service, username, &conv, &pamh);
    
    if(ret == PAM_SUCCESS)
    {
      ret=pam_authenticate(pamh, 0);
      if(ret == PAM_SUCCESS)
      {
        ret=pam_acct_mgmt(pamh, 0);
      }
      
      if(ret == PAM_SUCCESS)
      {
        if (pam_end(pamh,ret) == PAM_SUCCESS)
        {
          pamh=NULL;
          __resp[0].resp=NULL;
          lua_pushboolean(L,true);
          return 1;
        }
      }
    }
    
    lua_pushboolean(L,false);
    return 1;
  }
  
  LUA_API int luaopen_pam_auth(lua_State *L)
  {
    static const struct luaL_reg members[] = {
      {"login", pam_auth},
      {nullptr,nullptr}
    };
    luaL_newmetatable(L,"pam_auth");
    luaL_openlib(L, NULL, members,0);


    return 1;
  }
}

#endif /* __PAM_AUTH_H__ */

