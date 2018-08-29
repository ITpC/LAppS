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
      {"listeners",2},
      {"connection_weight", 0.8},
      {"ip","0.0.0.0"},
      {"port",5083},
      {"lapps_config_auto_save", true },
      {"workers",{ {"workers",3}, {"max_connections", 10000 },{"auto_fragment",false}}},
      {"acl", {{"policy", "allow"},{"exclude", {} }}},
#ifdef LAPPS_TLS_ENABLE
      {"tls",true},
#else      
      {"tls",false},
#endif
      {"tls_certificates",{ {"ca","/opt/lapps/etc/ssl/cert.pem"},{"cert", "/opt/lapps/conf/ssl/cert.pem"}, {"key","/opt/lapps/conf/ssl/key.pem" } }}
    }),
    lapps_config({
      {
        "directories",
        {
          {"applications","apps"},
          {"app_conf_dir","etc"},
          {"tmp","tmp"},
          {"workdir","workdir"},
          {"deploy", "deploy" }
        },
      },
      {
        "services", {
          {"placeholder", {
            {"internal", true},
            {"instances", 0},
            {"auto_start" , false }
          }}
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
      
      mEnv.setEnv("LAPPS_APPS_DIR",apps_dir);
      
      auto it=lapps_config["services"].begin();
      
      std::string lua_path(mEnv["LUA_PATH"]);
      
      while(it!=lapps_config["services"].end())
      {
        const std::string app_path=apps_dir+"/"+it.key()+"/?.lua";
        lua_path=lua_path+";"+app_path;
        ++it;
      }
      mEnv.setEnv("LUA_PATH",lua_path);
   }
   const json& getWSConfig() const
   {
     return ws_config;
   }
   
   json& getLAppSConfig()
   {
     return lapps_config;
   }
   
   void save()
   {
     std::ofstream lapps_config_file(mEnv["LAPPS_CONF_DIR"]+"/"+mEnv["LAPPS_CONFIG"]);
     if(lapps_config_file)
     {
        lapps_config_file << lapps_config.dump(2);
        lapps_config_file.close();
     }
   }
  };
}

typedef itc::Singleton<LAppS::Config> LAppSConfig;

#endif /* __CONFIG_H__ */
