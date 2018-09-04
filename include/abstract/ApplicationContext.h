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
 *  $Id: ApplicationContext.h May 7, 2018 8:22 AM $
 * 
 **/


#ifndef __IAPPLICATIONCONTEXT_H__
#  define __IAPPLICATIONCONTEXT_H__

#include <string>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdexcept>

}

#include <modules/nljson.h>
#include <modules/wsSend.h>
#include <modules/bcast.h>
#include <modules/pam_auth.h>
#include <modules/murmur.h>
#include <modules/time_now.h>
#include <modules/mqr.h>
#include <modules/cws.h>
#include <modules/nap.h>

#include <Config.h>

namespace abstract
{
  class ApplicationContext
  {
   protected: 
    std::string mName;
    lua_State*  mLState;
    
    void cleanLuaStack()
    {
      lua_pop(mLState,lua_gettop(mLState));
    }
    
    const bool require(const std::string& module_name)
    {
      lua_getfield (mLState, LUA_GLOBALSINDEX, "require");
      lua_pushstring (mLState, module_name.c_str());
      int ret = lua_pcall (mLState, 1, 1, 0);
      checkForLuaErrorsOnRequire(ret,module_name.c_str());
      lua_setfield(mLState, LUA_GLOBALSINDEX, module_name.c_str());
      cleanLuaStack();
      itc::getLog()->info(__FILE__,__LINE__,"Application instance [%s] is initialized",mName.c_str());
      return true;
    }
    
    void checkForLuaErrorsOnPcall(const int ret, const char* method)
    {
      checkForLuaErrorsOn(ret,"calling method",method);
    }
    
    void checkForLuaErrorsOnRequire(const int ret, const char* module)
    {
      checkForLuaErrorsOn(ret,"loading module",module);
    }
    
    void checkForLuaErrorsOn(const int ret,const std::string& on, const char* module_or_method)
    {
      if(ret != 0)
      {
        switch(ret)
        {
          case LUA_ERRRUN:
            itc::getLog()->flush();
            throw std::runtime_error(std::string("Lua runtime error on "+on+" "+module_or_method+": ")+std::string(lua_tostring(mLState,lua_gettop(mLState))));
            break;
          case LUA_ERRMEM:
            itc::getLog()->flush();
            throw std::system_error(ENOMEM,std::system_category(),"Not enough memory to "+on+" "+module_or_method+": "+std::string(lua_tostring(mLState,lua_gettop(mLState))));
          case LUA_ERRERR:
            itc::getLog()->flush();
            throw std::logic_error("Error in lua error handler on "+on+" "+module_or_method+": "+std::string(lua_tostring(mLState,lua_gettop(mLState))));
            break;
          default:
            itc::getLog()->flush();
            throw std::system_error(EINVAL,std::system_category(),"Unknown error on lua_pcall()");
        }
      }
    }
   public:
    ApplicationContext()=delete;
    ApplicationContext(const ApplicationContext&)=delete;
    ApplicationContext(ApplicationContext&)=delete;
    explicit ApplicationContext(const std::string& name)
    : mName(name),mLState(luaL_newstate())
    {
      luaL_openlibs(mLState);

      luaopen_nljson(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"nljson");
      cleanLuaStack();

      luaopen_nap(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"nap");
      cleanLuaStack();
      
      try{
        json& modules=LAppSConfig::getInstance()->getLAppSConfig()["services"][name]["preload"];
        
        for(const std::string& module: modules)
        {
          if(module == "pam_auth")
          {
            luaopen_pam_auth(mLState);
            lua_setfield(mLState,LUA_GLOBALSINDEX,"pam_auth");
            cleanLuaStack();
          }else if(module == "time")
          {
            luaopen_time(mLState);
            lua_setfield(mLState,LUA_GLOBALSINDEX,"time");
            cleanLuaStack();
          }else if(module == "murmur")
          {
            luaopen_murmur(mLState);
            lua_setfield(mLState,LUA_GLOBALSINDEX,"murmur");
            cleanLuaStack();
          }else if(module == "mqr")
          {
            luaopen_mqr(mLState);
            lua_setfield(mLState,LUA_GLOBALSINDEX,"mqr");
            cleanLuaStack();
          }else if(module == "cws")
          {
            luaopen_cws(mLState);
            lua_setfield(mLState,LUA_GLOBALSINDEX,"cws");
            cleanLuaStack();
          }else{
            itc::getLog()->error(__FILE__,__LINE__,"No such module %s", module.c_str());
          }
        }
      }catch(const std::exception& e)
      {
        itc::getLog()->error(__FILE__,__LINE__,"An exception %s is risen on modules preload for service %s", e.what(), name.c_str());
      }
    }
    const std::string& getName() const {
      return mName;
    }
    virtual ~ApplicationContext() noexcept = default;
  };
}

#endif /* __IAPPLICATIONCONTEXT_H__ */

