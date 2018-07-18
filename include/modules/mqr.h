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
 *  $Id: mqr.h July 18, 2018 4:36 PM $
 * 
 **/


#ifndef __MQR_H__
#  define __MQR_H__
#include <exception>
#include <errno.h>
#include <memory>
#include <vector>


#include <MQRegistry.h>

#include <modules/UserDataAdapter.h>
#include <ext/json.hpp>

auto udptr2jsptr(void* ptr)
{
  if((*static_cast<UDJSON**>(ptr))->type == SHARED_PTR)
  {
    auto udptr=static_cast<UDJSPTR**>(ptr);
    return *((*udptr)->ptr); // std::shared_ptr<json>*
  }
  else
  {
    auto udptr=static_cast<UDJSON**>(ptr);
    auto jsptr=(*udptr)->ptr; // json*
    return std::make_shared<json>(*jsptr);
  }
}

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>

  static auto assert_type_mqr(lua_State* L,const int& lvl)
  {
    size_t* ptr=(size_t*)luaL_checkudata(L, lvl, "mqr");
    if(!ptr)
    {
      throw std::system_error(EINVAL,std::system_category(),"not an MQR object");
    }
    return ptr;
  }
  
  LUA_API int destroy_nljson(lua_State *L)
  {
    auto ptr=checktype_nljson(L,1); // Userdata<T>**

    if((*static_cast<UDJSON**>(ptr))->type == SHARED_PTR)
    {
      auto jptr=static_cast<UDJSPTR**>(ptr);
      delete(*jptr);
      (*jptr)=nullptr; // this is for GC to manage
    }
    else
    {
      auto jptr=static_cast<UDJSON**>(ptr);
      delete(*jptr);
      (*jptr)=nullptr; // this is for GC to manage
    }
    return 0;
  }
  
  LUA_API int create(lua_State *L)
  {
     unsigned int argc = lua_gettop(L);
     
     if((argc == 1)&&(lua_isstring(L,argc)))
     {
       std::string qname(lua_tostring(L,argc));
       
       auto udptr=static_cast<size_t*>(lua_newuserdata(L,sizeof(size_t*)));
       
       (*udptr)=LAppS::MQ::QueueRegistry::getInstance()->create(qname);
       
       luaL_getmetatable(L, "mqr");
       lua_setmetatable(L, -2);
       
       return 1;
     }else{ // usage
       lua_pushnil(L);
       lua_pushstring(L,"mqr mqr.create(name) - where name is a string, returns userdata of mqr type");
       return 2;
     }
  }
  
  LUA_API int post(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    
    if(argc==2)
    {
      try{
        size_t* ptr=assert_type_mqr(L,1);
        try{
          auto queue=LAppS::MQ::QueueRegistry::getInstance()->find(*ptr);
          
          try{
            auto udptr=luaL_checkudata(L, 2, "nljson");
            if(udptr)
            {
              
              queue->send(std::move(udptr2jsptr(udptr)));
              lua_pushboolean(L,true);
              return 1;
            }else{
              throw std::system_error(EINVAL, std::system_category(), "not an object of the nljson userdata-type");
            }
          }
          catch(const std::exception& e)
          {
            lua_pushboolean(L,false);
            lua_pushstring(L,e.what());
            return 2;
          }
        }catch(const std::exception& e)
        {
          lua_pushboolean(L,false);
          lua_pushstring(L,e.what());
          return 2;
        }
      }catch(const std::exception& e)
      {
        lua_pushboolean(L,false);
        lua_pushstring(L,e.what());
        return 2;
      }
    }else{
      lua_pushboolean(L,false);
      lua_pushstring(L,"Usage: boolean [,string] mqr_obj:post(nljson_obj)");
      return 2;
    }
  }
  
  LUA_API int try_recv(lua_State *L)
  {
    unsigned int argc=lua_gettop(L);
    
    if(argc==1)
    {
      try{
        auto ptr=assert_type_mqr(L,1);
        try{
          auto queue=LAppS::MQ::QueueRegistry::getInstance()->find(*ptr);
          
          try{
            auto udjsptr=static_cast<UDJSPTR**>(lua_newuserdata(L,sizeof(UDJSPTR*)));
            (*udjsptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
            
            if(queue->tryRecv(*((*udjsptr)->ptr)))
            {
              luaL_getmetatable(L, "nljson");
              lua_pushcfunction(L, destroy_nljson);
              lua_setfield(L, -2, "__gc");
              lua_setmetatable(L, -2);
              return 1;
            }else{
              lua_pop(L,1);
              lua_pushnil(L);
              return 1;
            }
          }
          catch(const std::exception& e)
          {
            lua_pushboolean(L,false);
            lua_pushstring(L,e.what());
            return 2;
          }
        }catch(const std::exception& e)
        {
          lua_pushboolean(L,false);
          lua_pushstring(L,e.what());
          return 2;
        }
      }catch(const std::exception& e)
      {
        lua_pushboolean(L,false);
        lua_pushstring(L,e.what());
        return 2;
      }
    }else{
      lua_pushboolean(L,false);
      lua_pushstring(L,"Usage: boolean [,string] mqr_obj:post(nljson_obj)");
      return 2;
    } 
  }
  
  LUA_API int receive(lua_State *L)
  {
    unsigned int argc=lua_gettop(L);
    
    if(argc==1)
    {
      try{
        auto ptr=assert_type_mqr(L,1);
        try{
          auto queue=LAppS::MQ::QueueRegistry::getInstance()->find(*ptr);
          
          try{
            auto udjsptr=static_cast<UDJSPTR**>(lua_newuserdata(L,sizeof(UDJSPTR*)));
            (*udjsptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
            (*((*udjsptr)->ptr))=std::move(queue->recv());
            
              luaL_getmetatable(L, "nljson");
              lua_pushcfunction(L, destroy_nljson);
              lua_setfield(L, -2, "__gc");
              lua_setmetatable(L, -2);
              return 1;
          }
          catch(const std::exception& e)
          {
            lua_pushboolean(L,false);
            lua_pushstring(L,e.what());
            return 2;
          }
        }catch(const std::exception& e)
        {
          lua_pushboolean(L,false);
          lua_pushstring(L,e.what());
          return 2;
        }
      }catch(const std::exception& e)
      {
        lua_pushboolean(L,false);
        lua_pushstring(L,e.what());
        return 2;
      }
    }else{
      lua_pushboolean(L,false);
      lua_pushstring(L,"Usage: boolean [,string] mqr_obj:post(nljson_obj)");
      return 2;
    } 
  }
  
  LUA_API int mqr_methods(lua_State *L)
  {
    luaL_getmetatable(L, "mqr");
    lua_getfield(L,-1,lua_tostring(L,2));
    
    return 1;
  }
  
  LUA_API int luaopen_mqr(lua_State *L)
  {
    static const struct luaL_reg functions[]= {
      {"create",create},
      {nullptr,nullptr}
    };
    
    static const struct luaL_reg members[] = {
      {"post", post},
      {"try_recv", try_recv},
      {"recv", receive},
      {"__index", mqr_methods},
      {nullptr,nullptr}
    };

    luaL_newmetatable(L,"mqr");
    luaL_openlib(L, NULL, members,0);
    luaL_openlib(L, "mqr", functions,0);

    return 1;
  }
}


#endif /* __MQR_H__ */

