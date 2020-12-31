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
 *  $Id: nljson.h March 23, 2018 1:46 AM $
 * 
 **/

/** 
 * TODO: replace the exceptions with lua_error;
 **/

#ifndef __NLJSON_H__
#  define __NLJSON_H__

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <stdio.h>
}

#include <fstream>
#include <string>
#include <memory>
#include <map>
#include <ext/json.hpp>
#include <system_error>

#include <modules/UserDataAdapter.h>

#include <itc_log_defs.h>

static inline auto checktype_nljson(lua_State *L,const int lvl)
{
    auto udptr=luaL_checkudata(L, lvl, "nljson");
    if(!udptr)
    {
      throw std::system_error(
        EINVAL,
        std::system_category(),
        "checktype_nljson() argument on lua stack at index "+
          std::to_string(lvl)+
          " is not of an nljson type"
      );
    }
    return udptr;
}

enum LuaTableType { ARRAY, MAP };

static inline LuaTableType getTableType(lua_State *L)
{
  double  index=0;
  size_t count=0;
  size_t size=0;
  lua_pushnil(L);
  while(lua_next(L, lua_gettop(L)-1) != 0)
  {
    ++size;
    if(lua_isnumber(L,-2) && (index = lua_tonumber(L, -2)))
    {
      if(floor(index) == index )
        ++count;
      else
      {
          lua_pop(L,1);
          break;
      }
    }
    lua_pop(L,1);
  }
  if(size!=count) // is a MAP
  {
    return MAP;
  }
  else
  {
    return ARRAY;
  }
}


static inline void set_pod_value(lua_State *L,json *sp)
{
  const int value_type(lua_type(L,-1));
  switch(value_type)
  {
    case LUA_TNONE:
    case LUA_TNIL:
    case LUA_TLIGHTUSERDATA:
    case LUA_TTHREAD:
    case LUA_TFUNCTION:
      (*sp)=nullptr;
    break;
    case LUA_TBOOLEAN:
      (*sp)=static_cast<bool>(lua_toboolean(L,-1));
    break;
    case LUA_TNUMBER:
      (*sp)=static_cast<double>(lua_tonumber(L,-1));
    break;
      case LUA_TSTRING:
      (*sp)=static_cast<std::string>(lua_tostring(L,-1));
    break;
    default:
      throw std::system_error(
        EINVAL, 
        std::system_category(),
        "Error in set_pod_value() at line"+
          std::to_string(__LINE__)+
          ": the value on lua stack at level "+
          std::to_string(lua_gettop(L))+
          " is not of a POD type, instead it is of type: "+
          lua_typename(L,lua_gettop(L))+"["+
            std::to_string(value_type)+
          "]"
      );
  }
}

static inline const json get_userdata_value(lua_State *L)
{
  auto valptr=luaL_checkudata(L, -1, "nljson");
  if( valptr != nullptr)
  {
    return ud2json(valptr);
  }
  return json(nullptr);
}

static inline void set_lua_table(lua_State *L,json& jsref)
{
  if(lua_istable(L,lua_gettop(L)))
  {
    switch(getTableType(L))
    {
      case MAP:
      {
        if(!(jsref.is_object()))
        {
          jsref=json::object();
        }

        lua_pushnil(L);
        while(lua_next(L,lua_gettop(L)-1) != 0)
        {
          std::string key=static_cast<std::string>(lua_tostring(L,-2));
          int value_type=lua_type(L,-1);
          try
          {
            switch(value_type)
            {
              case LUA_TUSERDATA:
                jsref[key]=get_userdata_value(L);
              break;
              case LUA_TTABLE:
              {
                set_lua_table(L,jsref[key]);
              }
              break;
              default:
                set_pod_value(L,&(jsref[key]));
              break;
            }
            lua_pop(L,1);
          } 
          catch(const std::exception& e)
          {
            throw std::runtime_error(
              "Exception is caught in nljson module at "+std::to_string(__LINE__)+"\n"+e.what()
            );
          }
        }
      }
      break;
      case ARRAY:
      {
        if(!(jsref.is_array()))
        {
          jsref=json::array();
        }
        lua_pushnil(L);
        while(lua_next(L,lua_gettop(L)-1) != 0)
        {
          const size_t index=lua_tointeger(L,-2)-1;
          int value_type=lua_type(L,-1);
          switch(value_type)
          {
            case LUA_TUSERDATA:
            {
                jsref[index]=get_userdata_value(L);
            }
            break;
            case LUA_TTABLE:
            {
              set_lua_table(L,jsref[index]);
            }
            break;
            default:
                set_pod_value(L,&(jsref[index]));
            break;
          }
          lua_pop(L,1);
        }
      }
      break;
    }
  }
  else
  {
    throw std::system_error(
      EINVAL,
      std::system_category(),
      "Error in nljson::set_lua_table(): the type of lua value on stack is not a table"
    );
  }
}

extern "C" {
  LUA_API int find(lua_State *L);
  LUA_API int destroy(lua_State *L)
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
  
  LUA_API int empty(lua_State *L)
  {
    auto argc=lua_gettop(L);
    if(argc != 1)
    {
      throw std::system_error(EINVAL, std::system_category(), "nljson:empty() method must be used with nljson object/array, with no additional arguments");
    }
    
    auto ptr=checktype_nljson(L,1); // Userdata<T>**

    json& jsref=ud2json(ptr);
    if(jsref.is_array()||jsref.is_object())
    {
      lua_pushboolean(L,jsref.empty());
      return 1;
    }
    lua_pushboolean(L,false);
    return 1;
  }
  
  LUA_API int get_typename(lua_State *L)
  {
    auto argc=lua_gettop(L);
    
    if(argc != 1)
    {
      throw std::system_error(EINVAL, std::system_category(), "nljson.typename(nljson__userdata) method must be used with userdata of type nljson only");
    }
    try{
      auto ptr=checktype_nljson(L,1); // Userdata<T>**

      json& jsref=ud2json(ptr);
      lua_pushstring(L,jsref.type_name());
      return 1;
    }catch(const std::exception& e)
    {
      ITC_ERROR(
        __FILE__,__LINE__,
        "Caught an exception in lua[nljson.typename()]: {}",e.what()
      );
      throw e;
    }
  }
  
  LUA_API int erase(lua_State *L)
  {
    size_t argc=lua_gettop(L);
    if(argc!=2)
    {
      lua_pushboolean(L,false);
      return 1;
    }
    auto ptr=checktype_nljson(L,1);
    auto key_type=lua_type(L,2);
    json& jsref=ud2json(ptr);
    switch(key_type)
    {
      case LUA_TNUMBER:
        if(jsref.is_array())
        {
          size_t idx=lua_tointeger(L,2);
          jsref.erase(idx);
          lua_pushboolean(L,true);
          return 1;
        }
        lua_pushboolean(L,false);
        return 1;
        break;
      case LUA_TSTRING:
        if(jsref.is_object())
        {
          const std::string key(lua_tostring(L,2));
          auto it=jsref.find(key);
          if(it!=jsref.end())
          {
            jsref.erase(it);
            lua_pushboolean(L,true);
            return 1;
          }
          lua_pushboolean(L,false);
          return 1;
        }
        lua_pushboolean(L,false);
        return 1;
        break;
      default:
        lua_pushboolean(L,false);
        return 1;
    }
  }
  LUA_API int decode(lua_State *L)
  {
     unsigned int argc = lua_gettop(L);
     
     if(argc == 1)
     {
        if(lua_isstring(L,argc))
        {
          try {
            // allocate a block to store  an addes to UDJSPTR and return blocks address
            auto udptr=static_cast<UDJSPTR**>(lua_newuserdata(L,sizeof(UDJSPTR*)));
            // allocate UDJSPTR and store its address to allocated block
            (*udptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
            // make JSPTR by parsing the string and store it to *(Userdata->ptr);
            *((*udptr)->ptr)=std::make_shared<json>(json::parse(lua_tostring(L,argc)));
            luaL_getmetatable(L, "nljson");
            lua_pushcfunction(L, destroy);
            lua_setfield(L, -2, "__gc");
            lua_setmetatable(L, -2);
          } catch (const std::exception& e)
          {
            ITC_ERROR(__FILE__,__LINE__,"Exception in nljson::decode(): {}",e.what());
            throw std::runtime_error(std::string("Exception caught in nljson::decode() [json parsing]: ")+e.what());
          }
        }
        else
        {
          throw std::runtime_error("Wrong method usage in lua code, must be: nljson.decode(json_encoded_string), where json_encoded_string is of type string");
        }
     }
     else
     {
        throw std::runtime_error("Wrong method usage in lua code, must be: nljson.decode(json_encoded_string), where json_encoded_string is of type string");
     }
     return 1;
  }

  LUA_API int decode_file(lua_State *L)
  {
     unsigned int argc = lua_gettop(L);
     if(argc == 1)
     {
        if(lua_isstring(L,argc))
        {
          std::string file_name((char*)lua_tostring(L,argc));
          std::ifstream ifs(file_name, std::ifstream::binary);

          if(ifs)
          {
            try{
              // allocate a block to store  an addes to UDJSPTR and return blocks address
              auto udptr=static_cast<UDJSPTR**>(lua_newuserdata(L,sizeof(UDJSPTR*)));
              // allocate UDJSPTR and store its address to allocated block
              (*udptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
              // make shared pointer to json object
              *((*udptr)->ptr)=std::make_shared<json>();
              // parse from stream
              ifs >> (**((*udptr)->ptr));
              luaL_getmetatable(L, "nljson");
              lua_pushcfunction(L, destroy);
              lua_setfield(L, -2, "__gc");
              lua_setmetatable(L, -2);
            } catch ( const std::exception& e)
            {
              throw std::runtime_error(std::string("Exception in nljson::decode_file: ")+e.what());
            }
            ifs.close();
          }
          else
          {
            throw std::system_error(ENOENT, std::system_category(), file_name );
          }
        }
        else
        {
          throw std::system_error(EINVAL, std::system_category(),"Usage: nljson.decode_file(FILE_NAME), where FILE_NAME is of type string");
        }
      }
      else
      {
        throw std::system_error(EINVAL, std::system_category(),"Usage: nljson.decode_file(FILE_NAME), where FILE_NAME is of type string");
      }
     return 1;
  }

  LUA_API int encode_pretty(lua_State *L)
  {
    size_t argc = lua_gettop(L);
    size_t indent=2;
    if(argc>1)
    {
      if(argc==2) indent=lua_tointeger(L,argc);
      if(argc > 2) throw std::system_error(EINVAL, std::system_category(), "Usage: nljson.encode_pretty(nljson_userdata_object, [number indent]).");
      auto ptr=checktype_nljson(L,argc-1);
      const json& jsref=ud2json(ptr);
      std::string json_string( indent ? jsref.dump(indent) : jsref.dump() );
      lua_pushlstring(L,json_string.c_str(),json_string.length());
    }
    else
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson.encode_pretty(nljson_userdata_object, [number indent]).");
    }
    return 1;
  }

  LUA_API int encode(lua_State *L)
  {
     unsigned int argc = lua_gettop(L);
     if(argc == 1)
     {
        lua_pushinteger(L,0);
        encode_pretty(L);
     }
     else
     {
        throw std::system_error(EINVAL, std::system_category(), "Usage: nljson.encode(nljson_userdata_object)");
     }
     return 1;
  }

  void pushvalue(lua_State *L, const json::value_t vtype, const json& value)
  {
    switch(vtype) // what do we return to Lua?
    {
      case json::value_t::null:
      case json::value_t::discarded:
        lua_pushnil(L);
      break;
      case json::value_t::object:
      case json::value_t::array:
      {
        auto udptr=static_cast<UDJSON**>(lua_newuserdata(L,sizeof(UDJSON*)));
        (*udptr)=new UDJSON(RAW_PTR,(json*)(&value));
        luaL_getmetatable(L, "nljson"); // no GC on C++ collections
        lua_setmetatable(L, -2);
      }
      break;
      case json::value_t::string:
      {
        std::string retval=value;
        lua_pushlstring(L,retval.c_str(),retval.length());
      }
      break;
      case json::value_t::boolean:
      {
        bool retval=value;
        lua_pushboolean(L,retval);
      }
      break;
      case json::value_t::number_integer:
      {
        int64_t retval=value;
        lua_pushinteger(L,retval);
      }
      break;
      case json::value_t::number_unsigned:
      {
        uint64_t retval=value;
        lua_pushinteger(L,retval);
      }
      break;
      case json::value_t::number_float:
      {
        double retval=value;
        lua_pushnumber(L,retval);
      }
      break;
    }
  }

  LUA_API int find(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    
    if(argc != 2)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson.find(userdata:nljson,key:string)");
    }

    auto ptr=checktype_nljson(L,argc-1);
    if(!lua_isstring(L,argc))
    {
      throw std::system_error(EINVAL, std::system_category(), "Expecting string argument as a key in nljson.find()");
    }

    std::string key(lua_tostring(L,argc));

    const json& jsref=ud2json(ptr);

    auto it=jsref.find(key);

    if(it!=jsref.end())
    {
        pushvalue(L,it->type(),*it);
    }
    else
    {
        lua_pushnil(L);
    }
    return 1;
  }


  // to be called from LUA_API int tree_traversal(lua_State*) only
  static inline void t_traversal(lua_State *L,const json& node)
  {
    unsigned int argc = lua_gettop(L);

    if(node.type() == json::value_t::array || node.type() == json::value_t::object)
    {
      for(auto it=node.begin();it!=node.end();++it)
      {
        size_t counter=0;
        switch(it->type())
        {
          case json::value_t::array:
          case json::value_t::object:
            t_traversal(L,*it);
          break;
          default:
          break;
        }
        lua_pushvalue(L,argc);
        switch(node.type())
        {
          case json::value_t::array:
            lua_pushinteger(L,++counter);
          break;
          case json::value_t::object:
            lua_pushstring(L,it.key().c_str());
          break;
          break;
          default:
          break;
        }
        pushvalue(L,it.value().type(),it.value());
        if(lua_pcall(L,2,0,0) != 0)
        {
          throw std::system_error(
            EINVAL, 
            std::system_category(), 
            std::string("Error in calling callback function provided as a second argument of  nljson.tree_traversal: ")+
            lua_tostring(L, -1)
          );
        }
      }
    }
  }

  LUA_API int tree_traversal(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    
    if(argc != 2)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson.find(nljson_userdata,function)");
    }

    auto ptr=checktype_nljson(L,argc-1);
    if(!lua_isfunction(L,argc))
    {
      throw std::system_error(EINVAL, std::system_category(), "Expecting a function argument as a second argument in call to tree_traversal()");
    }
    try
    {
      const json& jsref=ud2json(ptr);
      t_traversal(L,jsref);
      lua_pop(L,1);
    }
    catch ( const std::exception& e)
    {
      throw;
    }
    return 0;
  }

  LUA_API int foreach(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    
    if(argc != 2)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson.foreach(userdata:nljson,function)");
    }

    auto ptr=checktype_nljson(L,argc-1);
    if(!lua_isfunction(L,argc))
    {
      throw std::system_error(EINVAL, std::system_category(), "Expecting a function argument as a second argument in call to nljson.foreach()");
    }
    try{
      const json& jsref=ud2json(ptr);
      size_t counter=0;

      for(auto it=jsref.begin();it!=jsref.end();++it)
      {
        lua_pushvalue(L,argc);
        if(jsref.is_object())
        {
          lua_pushstring(L,it.key().c_str());
        }
        else
        { 
          lua_pushinteger(L,++counter);
        }
        pushvalue(L,it.value().type(),it.value());
        if(lua_pcall(L,2,0,0) != 0)
        {
          throw std::system_error(
            EINVAL, 
            std::system_category(), 
            std::string("Error in calling callback function provided as a second argument of  nljson.foreach: ")+
            lua_tostring(L, -1)
          );
        }
      }
      lua_pop(L,1);
    } catch ( const std::exception& e)
    {
      throw;
    }
    return 0;
  }

  LUA_API int getval(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    if(argc != 2)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson_object[index] - array access to nljson userdata type, index is a string or integer");
    }
    auto ptr=checktype_nljson(L,1);
    int idx_type=lua_type(L, 2);
    const json& jsref=ud2json(ptr);

    try {
      switch(idx_type)
      {
        case LUA_TSTRING: // accessing a map?
        {
          const std::string index((char*)lua_tostring(L,argc));
          pushvalue(L,jsref[index].type(),jsref[index]);
        }
        break;
        case LUA_TNUMBER: // accessing a vector?
        {
          const size_t index=(lua_tointeger(L,argc))-1;
          pushvalue(L,jsref[index].type(),jsref[index]);
        }
        break;
      }
    } catch (const std::exception& e)
    {
      throw;
    }
    return 1;
  }

  LUA_API int to_cbor(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    if(argc != 1)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson:to_cbor(nljson_userdata_object), returns string");
    }

    auto ptr=checktype_nljson(L,argc);
    const json& jsref=ud2json(ptr);

    std::vector<uint8_t> cbor=json::to_cbor(jsref);
    lua_pushlstring(L,(const char*)(cbor.data()),cbor.size());

    return 1;
  }

  LUA_API int from_cbor(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    if(argc != 1)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson:from_cbor(string), returns nljson userdata");
    }
    std::vector<uint8_t> cbor(0);
    size_t sz;
    const char *ptr=lua_tolstring(L,argc,&sz);
    cbor.resize(sz);
    memcpy(cbor.data(),ptr,sz);

    auto udjsptr=static_cast<UDJSPTR**>(lua_newuserdata(L,sizeof(UDJSPTR*)));
    // allocate UDJSPTR and store its address to allocated block
    (*udjsptr)=new UDJSPTR(SHARED_PTR,new JSPTR());
    // make JSPTR by parsing the string and store it to *(Userdata->ptr);
    *((*udjsptr)->ptr)=std::make_shared<json>(json::from_cbor(cbor));
    luaL_getmetatable(L, "nljson");
    lua_pushcfunction(L, destroy);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    return 1;
  }
  
  
  LUA_API int setval(lua_State *L)
  {
    unsigned int argc = lua_gettop(L);
    
    if(argc != 3)
    {
      throw std::system_error(EINVAL, std::system_category(), "Usage: nljson_object[index]=value - set value to the userdata type within a map or vector, index is a string or integer");
    }
    // arg 1 - userdata
    // arg 2 - index
    // arg 3 - value
    auto ptr=checktype_nljson(L,argc-2);
    int idx_type=lua_type(L, argc-1);
    int value_type=lua_type(L,argc);

    json& jsref=ud2json(ptr);

    try
    {
      switch(idx_type)
      {
        case LUA_TSTRING: // accessing a map?
        {
          if(jsref.is_object())
          {
            const std::string key((char*)lua_tostring(L,argc-1));
            switch(value_type)
            {
              case LUA_TUSERDATA:
                jsref[key]=get_userdata_value(L);
              break;
              case LUA_TTABLE:
              {
                set_lua_table(L,jsref[key]);
              }
              break;
              default:
                set_pod_value(L,&(jsref[key]));
              break;
            }
            pushvalue(L,jsref[key].type(),jsref[key]);
          }
          else
          {
            throw std::system_error(EINVAL, std::system_category(),"Error in operator[] use of string key for non map object");
          }
        }
        break;
        case LUA_TNUMBER: // accessing a vector?
        {
          if(jsref.is_array())
          {
            const size_t index=(lua_tointeger(L,argc-1))-1;
            switch(value_type)
            {
              case LUA_TUSERDATA:
              {
                jsref[index]=get_userdata_value(L);
              }
              break;
              case LUA_TTABLE:
              {
                set_lua_table(L,jsref[index]);
              }
              break;
              default:
                set_pod_value(L,&(jsref[index]));
              break;
            }             
            pushvalue(L,jsref[index].type(),jsref[index]);
          }
          else
          {
            throw std::system_error(EINVAL, std::system_category(), "Error in operator[] use of numerical index for non array object");
          }
        }
        break;
      }
    } catch (const std::exception& e)
    {
      throw;
    }

    return 1;
  }
  
   
  LUA_API int luaopen_nljson(lua_State *L)
  {
    static const struct luaL_reg functions[]= {
      {"decode", decode},
      {"decode_file", decode_file},
      {"encode", encode },
      {"to_cbor", to_cbor},
      {"from_cbor", from_cbor},
      {"find",find},
      {"foreach",foreach},
      {"traverse",tree_traversal},
      {"encode_pretty", encode_pretty },
      {"erase",erase},
      {"empty",empty},
      {"typename",get_typename},
      {nullptr,nullptr}
    };
    static const struct luaL_reg members[] = {
      {"get",getval},
      {"set",setval},
      {"__tostring",encode},
      {"__index",getval},
      {"__newindex",setval},
      {"__gc",destroy},
      {nullptr,nullptr}
    };
    luaL_newmetatable(L,"nljson");
    luaL_openlib(L, NULL, members,0);
    luaL_openlib(L, "nljson", functions,0);

    return 1;
  }
}



#endif /* __NLJSON_H__ */

