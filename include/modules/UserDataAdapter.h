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
 *  $Id: UserDataAdapter.h March 23, 2018 1:48 AM $
 * 
 **/


#ifndef __USERDATAADAPTER_H__
#  define __USERDATAADAPTER_H__

#include <type_traits>
#include <ext/json.hpp>
#include <memory>

using json = nlohmann::json;
typedef std::shared_ptr<json> JSPTR;

template<typename T, typename U> struct is_same : std::false_type { };
template<typename T> struct is_same<T, T> : std::true_type { };
template<typename T, typename U> constexpr bool isEqual() { return is_same<T, U>::value; }

enum UserdataType { RAW_PTR, SHARED_PTR };
template <typename T> struct UserdataAdapter
{
  UserdataType type;
  T *ptr;
  UserdataAdapter()=delete;
  explicit UserdataAdapter(const UserdataType __type,T* __ptr)
  : type(__type),ptr(__ptr)
  {
    static_assert(
    (isEqual<T,JSPTR>() || isEqual<T,json>()),
    "UserdataAdapter may be used only for T=std::shared_ptr<nlohmann::json> or T=nlohmann::json"
    );
  }
  explicit UserdataAdapter(const UserdataAdapter& ref)
  : type(ref.type),ptr(ref.ptr) {}
  ~UserdataAdapter()
  {
    if(type == SHARED_PTR)
    {
      delete(ptr);
    }
  }
};

typedef UserdataAdapter<JSPTR>       UDJSPTR;
typedef UserdataAdapter<json>        UDJSON;


static inline json& ud2json(void* ptr)
{
  if((*static_cast<UDJSON**>(ptr))->type == SHARED_PTR)
  {
    auto udptr=static_cast<UDJSPTR**>(ptr);
    auto jsptr=(*udptr)->ptr; // std::shared_ptr<json>*
    return **jsptr;
  }
  else
  {
    auto udptr=static_cast<UDJSON**>(ptr);
    auto jsptr=(*udptr)->ptr; // json*
    return *jsptr;
  }
}

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

const json& get_userdata_value(lua_State *L, int index)
{
  auto valptr=luaL_checkudata(L, index, "nljson");
  if( valptr != nullptr)
  {
    return ud2json(valptr);
  }
  throw std::system_error(EINVAL, std::system_category(), "The submitted userdata is not of an nljson type");
}


#endif /* __USERDATAADAPTER_H__ */

