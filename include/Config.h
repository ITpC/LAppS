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
 *  $Id: Config.h December 29, 2017 10:39 PM $
 * 
 **/


#ifndef __CONFIG_H__
#  define __CONFIG_H__

#include <fstream>

#include <ext/json.hpp>
#include <Env.h>

#include <Singleton.h>

using json = nlohmann::json;

namespace LAppS
{
  class Config
  {
  private:
   environment::LAppSEnv mEnv;
   json ws_config;
   json lapps_config;
  public:
   Config() : 
    mEnv(), 
    ws_config({
      {"listeners",3},
      {"ip","0.0.0.0"},
      {"port",5083},
      {"tls",true},
      // {"tls_certificates",{ {"ca","/etc/ssl/cert.pem"},{"cert", "/apps/${APPNAME}/etc/ssl/cert.pem"}, {"key","/apps/${APPNAME}/etc/ssl/key.pem" } }}
      {"tls_certificates",{ {"ca","/etc/ssl/cert.pem"},{"cert", "./ssl/cert.pem"}, {"key","./ssl/key.pem" } }}
    }),
    lapps_config({
      {
        "thread_pool",
        {
          {"maxThreads", 20},
          {"overcommitThreads",2},
          {"purgeTimeout", 10000},
          {"minThreadsReady", 3}
        }
      },
      {
        "directories",
        {
          {"applications","apps"},
          {"app_conf_dir","etc"},
          {"tmp","tmp"},
          {"workdir","workdir"}
        },
      },
      {
        "services", 
        {
          {{"echo", {
            {"autostart", false},
            {"request_target", "/echo"},
            {"protocol", "raw"},
            {"workers",{ {"workers",3}, {"max_connections", 100 }}}
          }}},
          {{"console", {
            {"internal", false},
            {"request_target","/console"},
            {"protocol", "LAppS"},
            {"workers",{ {"workers",1}, {"max_connections", 100 }}}
          }}}
        }
       }
      }){
     std::ifstream ws_config_file(mEnv["LAPPS_CONF_DIR"]+"/"+mEnv["WS_CONFIG"], std::ifstream::binary);
     if(ws_config_file)
     {
       ws_config_file >> ws_config;
       ws_config_file.close();
     }
     std::ifstream lapps_config_file(mEnv["LAPPS_CONF_DIR"]+"/"+mEnv["LAPPS_CONFIG"], std::ifstream::binary);
     if(lapps_config_file)
     {
       lapps_config_file >> lapps_config;
       lapps_config_file.close();
     }
   }
   const json& getWSConfig() const
   {
     return ws_config;
   } 
   const json& getLAppSConfig() const
   {
     return lapps_config;
   }
  };
}

typedef itc::Singleton<LAppS::Config> LAppSConfig;

#endif /* __CONFIG_H__ */
