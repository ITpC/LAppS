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
      {"workers",{ {"workers",3}, {"max_connections", 1000 }}},
#ifdef LAPPS_TLS_ENABLE
      {"tls",true},
#else      
      {"tls",false},
#endif
      {"tls_certificates",{ {"ca","/opt/lapps/etc/ssl/cert.pem"},{"cert", "/opt/lapps/conf/ssl/cert.pem"}, {"key","/opt/lapps/conf/ssl/key.pem" } }},
      // {"tls_certificates",{ {"ca","/etc/ssl/cert.pem"},{"cert", "./ssl/cert.pem"}, {"key","./ssl/key.pem" } }},
      {"auto_fragment",true}, // Not yet implemented
      {"max_inbound_message_size",300000} // 300 000 bytes. Not yet implemented.No message limit so far.
    }),
    lapps_config({
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
        "services", {
          {{"echo", {
            {"internal", false},
            {"request_target", "/echo"},
            {"protocol", "raw"},
            {"instances", 3}
          }}},
          {{"echo_lapps", {
            {"internal", false},
            {"request_target", "/echo_lapps"},
            {"protocol", "LAppS"},
            {"instances", 3}
          }}}
          /**
          {{"console", {
            {"internal", false},
            {"request_target","/console"},
            {"protocol", "LAppS"},
            {"workers",{ {"workers",1}, {"max_connections", 100 }}},
            {"instances", 1},
          }}}*/
        }
      }})
   {
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
      // add application files to LUA_PATH
      const std::string apps_subdir=lapps_config["directories"]["applications"];
      const std::string apps_dir(mEnv["LAPPS_HOME"]+"/"+apps_subdir+"/");
      auto it=lapps_config["services"].begin();
      
      std::string lua_path=mEnv["LUA_PATH"];
      
      while(it!=lapps_config["services"].end())
      {
        std::cout << it.value().begin().key() << std::endl;
        const std::string app_path=apps_dir+it.value().begin().key()+"/?.lua";
        lua_path=lua_path+";"+app_path;
        ++it;
      }
      mEnv.setEnv("LUA_PATH",lua_path.c_str());
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
