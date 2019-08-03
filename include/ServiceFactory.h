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
 *  $Id: ServiceFactory.h October 2, 2018 4:13 PM $
 * 
 **/


#ifndef __SERVICEFACTORY_H__
#  define __SERVICEFACTORY_H__

#include <memory>
#include <string>

#include <ext/json.hpp>

#include <NetworkACL.h>
#include <ContextTypes.h>

#include <abstract/Service.h>
#include <LuaReactiveService.h>
#include <LuaStandaloneService.h>

using json = nlohmann::json;


namespace LAppS
{
  class ServiceFactory
  {
   public:
    explicit ServiceFactory()=default;
    ServiceFactory(const ServiceFactory&)=delete;
    ServiceFactory(ServiceFactory&)=delete;
    ~ServiceFactory() noexcept = default;
    
    static auto get(const ServiceLanguage& lang, const std::string& name)
    {
      switch(lang)
      {
        case ServiceLanguage::LUA:
          return std::make_shared<ServiceInstanceType>(std::make_shared<LuaStandaloneService>(name));
        case ServiceLanguage::PYTHON:
        default:
          throw std::system_error(EINVAL,std::system_category(),"Python services are not implemented yet");
      }
    }
    
    static auto get(
      const ServiceProtocol proto,
      const ServiceLanguage lang, 
      const std::string& name,
      const std::string& target,
      const size_t max_in_msg_sz,
      const Network_ACL_Policy policy,
      const json& exclude_list
    )
    {
      switch(lang)
      {
        case ServiceLanguage::LUA:
          switch(proto)
          {
            case ServiceProtocol::LAPPS:
              return std::make_shared<ServiceInstanceType>(std::make_shared<LuaReactiveService<ServiceProtocol::LAPPS>>(name,target,max_in_msg_sz,policy,exclude_list));
            case ServiceProtocol::RAW:
              return std::make_shared<ServiceInstanceType>(std::make_shared<LuaReactiveService<ServiceProtocol::RAW>>(name,target,max_in_msg_sz,policy,exclude_list));
            case ServiceProtocol::INTERNAL:
              throw std::system_error(EINVAL,std::system_category(),"An attempt to start a Standalone service ["+name+"] as a Reactive one");
          }
        break;
        case ServiceLanguage::PYTHON:
        default:
          throw std::system_error(EINVAL,std::system_category(),"Python services are not implemented yet");
      }
    }
  };
}


#endif /* __SERVICEFACTORY_H__ */

