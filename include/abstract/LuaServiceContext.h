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
 *  $Id: LuaServiceContext.h September 18, 2018 11:13 AM $
 * 
 **/


#ifndef __LUASERVICECONTEXT_H__
#  define __LUASERVICECONTEXT_H__

#include <string>
#include <abstract/ServiceContext.h>

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

static void init_nljson_module(lua_State* L)
{
  luaopen_nljson(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"nljson");
  lua_pop(L,lua_gettop(L));
}

static void init_nap_module(lua_State* L)
{
  luaopen_nap(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"nap");
  lua_pop(L,lua_gettop(L));
}

static void init_pam_auth_module(lua_State* L)
{
  luaopen_pam_auth(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"pam_auth");
  lua_pop(L,lua_gettop(L));
}

static void init_time_module(lua_State* L)
{
  luaopen_time(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"time");
  lua_pop(L,lua_gettop(L));
}

static void init_murmur_module(lua_State* L)
{
  luaopen_murmur(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"murmur");
  lua_pop(L,lua_gettop(L));
}

static void init_mqr_module(lua_State* L)
{
  luaopen_mqr(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"mqr");
  lua_pop(L,lua_gettop(L));
}

static void init_cws_module(lua_State* L)
{
  luaopen_cws(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"cws");
  lua_pop(L,lua_gettop(L));
}

static void init_ws_module(lua_State* L)
{
  luaopen_wssend(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"ws");
  lua_pop(L,lua_gettop(L));
}

static void init_bcast_module(lua_State* L)
{
  luaopen_bcast(L);
  lua_setfield(L,LUA_GLOBALSINDEX,"bcast");
  lua_pop(L,lua_gettop(L));
}

static thread_local const std::map<std::string,void(*)(lua_State*)> modules_map={
  {"nap",init_nap_module},
  {"time",init_time_module},
  {"pam_auth",init_pam_auth_module},
  {"murmur",init_murmur_module},
  {"mqr",init_mqr_module},
  {"cws",init_cws_module}
};

namespace LAppS
{
  namespace abstract
  {
    class LuaServiceContext : public ServiceContext
    {
     protected:
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
        try{
          checkForLuaErrorsOnRequire(ret,module_name.c_str());
          lua_setfield(mLState, LUA_GLOBALSINDEX, module_name.c_str());
          ITC_INFO(__FILE__,__LINE__,"Service-module [{}] is initialized",this->getName().c_str());
        }catch(const std::exception& e)
        {
          ITC_ERROR(__FILE__,__LINE__,"Can't load Service-module [{}], exception: {}",this->getName().c_str(), e.what());
          cleanLuaStack();
          return false;
        }
        cleanLuaStack();
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
              throw std::runtime_error(std::string("Lua runtime error on "+on+" "+module_or_method+": ")+std::string(lua_tostring(mLState,lua_gettop(mLState))));
              break;
            case LUA_ERRMEM:
              throw std::system_error(ENOMEM,std::system_category(),"Not enough memory to "+on+" "+module_or_method+": "+std::string(lua_tostring(mLState,lua_gettop(mLState))));
            case LUA_ERRERR:
              throw std::logic_error("Error in lua error handler on "+on+" "+module_or_method+": "+std::string(lua_tostring(mLState,lua_gettop(mLState))));
              break;
            default:
              throw std::system_error(EINVAL,std::system_category(),"Unknown error on lua_pcall()");
          }
        }
      }

     public:
      explicit LuaServiceContext(const std::string& name)
      : ServiceContext(name), mLState{luaL_newstate()}
      {
        luaL_openlibs(mLState);
        init_nljson_module(mLState);
        
        try{
          json& modules=LAppSConfig::getInstance()->getLAppSConfig()["services"][this->getName()]["preload"];
          
          for(const std::string& module: modules)
          {
            auto it=modules_map.find(module);
            if(it!=modules_map.end())
            {
              it->second(mLState);
            }
          }
        }catch(const std::exception& e)
        {
          ITC_ERROR(__FILE__,__LINE__,"An exception {} is risen on modules preload for service {}", e.what(), this->getName().c_str());
        }
      }
      
      LuaServiceContext()=delete;
      LuaServiceContext(LuaServiceContext&)=delete;
      LuaServiceContext(const LuaServiceContext&)=delete;
      
      const ServiceLanguage getLanguage() const
      {
        return ServiceLanguage::LUA;
      }
      
      virtual const bool isServiceModuleValid()  = 0;
      
      ~LuaServiceContext()
      {
        if(mLState)
        {
          lua_close(mLState);
          mLState=nullptr;
        }
      }
    };
  }
}

#endif /* __LUASERVICECONTEXT_H__ */

