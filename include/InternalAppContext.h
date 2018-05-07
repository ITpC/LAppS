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
 *  $Id: include/InternalAppContext.h May 5, 2018 6:42 AM $
 * 
 **/


#ifndef __INTERNALAPPCONTEXT_H__
#  define __INTERNALAPPCONTEXT_H__

#include <abstract/ApplicationContext.h>

#include <ext/json.hpp>

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdexcept>
}


#include <modules/nljson.h>
#include <modules/bcast.h>

namespace LAppS
{
  class InternalAppContext : public ::abstract::ApplicationContext
  {
  private:
   const bool isInternalAppValid() const
    {
      bool ret=false;
      
      lua_getglobal(mLState, mName.c_str());
      
      lua_getfield(mLState, -1, "init");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      
      lua_getfield(mLState, -1, "run");
      ret=lua_isfunction(mLState,-1);
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      
      lua_getfield(mLState, -1, "stop");
      ret=lua_isfunction(mLState,-1);
      
      if(ret){
        lua_pop(mLState,1);
      }
      else{
        lua_pop(mLState,1);
        return false;
      }
      
      return ret;
    }
  public:  
    InternalAppContext(const std::string& name):
    ::abstract::ApplicationContext(name)
    {
      require(name);
      if(!isInternalAppValid())
        throw std::system_error(EINVAL,std::system_category(),mName+" is an invalid LAppS application, - required method(s) are not provided: init(),run(),stop()");
      
      luaL_openlibs(mLState);
      
      luaopen_nljson(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"nljson");
      cleanLuaStack();
      
      
      luaopen_bcast(mLState);
      lua_setfield(mLState,LUA_GLOBALSINDEX,"bcast");
      cleanLuaStack();
      
    }
    void init()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "init");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"init");
      cleanLuaStack();
    }
    
    void stop()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "stop");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"stops");
      cleanLuaStack();
    }
    
    void run()
    {
      lua_getglobal(mLState, mName.c_str());
      lua_getfield(mLState, -1, "run");
      int ret = lua_pcall (mLState, 0, 0, 0);
      checkForLuaErrorsOnPcall(ret,"run");
      cleanLuaStack();
    }
    
    InternalAppContext(const InternalAppContext&)=delete;
    InternalAppContext(InternalAppContext&)=delete;
    InternalAppContext()=delete;    
  };
}

#endif /* __INTERNALAPPCONTEXT_H__ */

