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

namespace abstract
{
  class ApplicationContext
  {
   protected: 
    std::string mName;
    lua_State*  mLState;
    
    void cleanLuaStack()
    {
      while(lua_gettop(mLState))
      {
        lua_pop(mLState,1);
      }
    }
    
    const bool require(const std::string& module_name)
    {
      lua_getfield (mLState, LUA_GLOBALSINDEX, "require");
      lua_pushstring (mLState, module_name.c_str());
      int ret = lua_pcall (mLState, 1, 1, 0);
      checkForLuaErrorsOnRequire(ret,module_name.c_str());
      lua_setfield(mLState, LUA_GLOBALSINDEX, module_name.c_str());
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
    
    void checkForLuaErrorsOn(const int ret,const std::string on, const char* module_or_method)
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
        }
      }
    }
   public:
    ApplicationContext()=delete;
    ApplicationContext(const ApplicationContext&)=delete;
    ApplicationContext(ApplicationContext&)=delete;
    explicit ApplicationContext(const std::string& name)
    : mName(name),mLState(luaL_newstate())
    {}
    virtual ~ApplicationContext() noexcept = default;
  };
}

#endif /* __IAPPLICATIONCONTEXT_H__ */

