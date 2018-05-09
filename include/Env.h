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
 *  $Id: Env.h December 29, 2017 10:39 PM $
 * 
 **/


#ifndef __ENV_H__
#  define __ENV_H__

#include <stdlib.h>
#include <map>
#include <string>
#include <algorithm>

namespace environment
{
  class LAppSEnv
  {
   private:
    std::map<std::string,std::string> config;
    
    auto getEnv(const std::string& variable) const
    {
      return secure_getenv(variable.c_str());
    }
    
   public:
    LAppSEnv(): config({
      {"LAPPS_HOME","/opt/lapps"},
      {"LAPPS_CONF_DIR","/opt/lapps/etc/conf"},
      {"WS_CONFIG","ws.json"},
      {"LAPPS_CONFIG","lapps.json"},
      {"LUA_PATH",""}
    }){
      for(auto var : config)
      {
        auto ptr=this->getEnv(var.first);
        if(ptr)
        {
          config[var.first]=ptr;
        }
      }
    }
    
    void setEnv(const std::string& name, const std::string& value)
    {
      config.emplace(name,value);
      ::setenv(name.c_str(), value.c_str(),1);
    }
    
    std::string& operator[](const std::string& var)
    {
      static const std::string empty_string("");
      
      auto it=config.find(var);
      if(it!=config.end())
      {
        return it->second;
      }else{
        const char *envptr=getEnv(var);
        if(envptr == nullptr)
          config.emplace(var,empty_string);
        else
          config.emplace(var,envptr);
      }
      return config[var];
    }
  };
}

#endif /* __ENV_H__ */

