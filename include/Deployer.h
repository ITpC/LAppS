/*
 * Copyright (C) 2018 Pavel Kraynyukhov <pavel.kraynyukhov@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * $Id: Deployer.h June 19, 2018 10:54 $
 */


#ifndef __DEPLOYER_H__
#define __DEPLOYER_H__
#include <memory>
#include <mutex>
#include <sys/synclock.h>
#include <tsbqueue.h>
#include <sys/inotify.h>
#include <linux/limits.h>
#include <experimental/filesystem>
#include <Config.h>
#include <Env.h>
#include <abstract/Application.h>
#include <Application.h>
#include <ApplicationRegistry.h>
#include <InternalApplication.h>
#include <InternalApplicationRegistry.h>
#include <LAR.h>
#include <ext/json.hpp>

namespace fs = std::experimental::filesystem::v1;
using json = nlohmann::json;

namespace LAppS
{
  template <bool TLSEnable=true, bool StatsEnable=true> class Deployer : itc::abstract::IRunnable
  {
  public:
    typedef ::LAppS::Application<TLSEnable,StatsEnable,::abstract::Application::Protocol::LAPPS> LAppSPROTOApp;
    typedef ::LAppS::Application<TLSEnable,StatsEnable,::abstract::Application::Protocol::RAW>   RAWPROTOApp;
  private:
    std::atomic<bool>           mMayRun;
    std::mutex                  mMutex;
    itc::tsbqueue<std::string>  mDeploymentQueue;
    int                         mInotifyFD;
    environment::LAppSEnv       mEnv;
    fs::path                    mDeployDir;
        
    void deploy_all()
    {
      for(auto& entry : fs::recursive_directory_iterator(mDeployDir))
      {
        itc::getLog()->info(__FILE__,__LINE__,"Deploying: %s",entry.path().u8string().c_str());
        deploy_archive(entry.path());
      }
    }
    
    auto deploy_archive(const fs::path& archive_name)
    {
      if(fs::is_regular_file(archive_name))
      {   
        std::string extension(archive_name.extension().u8string().c_str());
        
        if(extension == ".lar")
        { 
          std::string lapps_tmp_dir=LAppSConfig::getInstance()->getLAppSConfig()["directories"]["tmp"];
          fs::path tempdir(fs::path(static_cast<const std::string&>(mEnv["LAPPS_HOME"])) / lapps_tmp_dir);
          
          if(fs::exists(tempdir))
          {
            fs::remove_all(tempdir);
          }
          
          itc::getLog()->info(__FILE__,__LINE__,"Unpacking archive %s",archive_name.u8string().c_str());
          lar::LAR service_archive;
          try{
            service_archive.unpack(
              archive_name,
              tempdir
            );
          }
          catch(const std::exception& e)
          {
            std::string errmsg(std::string("Error on unpacking ")+archive_name.u8string()+std::string(": ")+std::string(e.what()));
            throw std::system_error(ECANCELED,std::system_category(),std::move(errmsg));
          }
          
          size_t counter=0;
          for(auto& dir : fs::directory_iterator(tempdir))
          {
            if(fs::is_directory(dir.path()))
              ++counter;
          }
          if(counter == 0)
            throw std::system_error(ENOENT,std::system_category(),"An empty archive"+archive_name.u8string());

          if(counter > 1)
            throw std::system_error(EINVAL,std::system_category(),"An invalid LAR file {multiroot/multiservice)");

          for(auto& dir : fs::directory_iterator(tempdir))
          {
            auto service_config_file=dir / std::string(archive_name.stem().u8string()+".json");
            if(fs::exists(service_config_file))
            {
              std::ifstream service_config_stream(service_config_file);
              json service_config;
              service_config_stream >> service_config;
              deploy_service(dir.path(),service_config);
              std::string service_name=service_config["name"];
              return service_name;
            }else{
              throw std::system_error(ENOENT,std::system_category(),std::string(u8"The service-config file ")+service_config_file.u8string()+std::string(u8" is unavailable"));
            }
          }
        }else{
          throw std::system_error(EINVAL,std::system_category(),std::string("Exception: ")+archive_name.u8string().c_str()+std::string(u8" does not have an extension .lar"));
        }
      }
      throw std::system_error(EINVAL,std::system_category(),archive_name.u8string()+std::string(u8" is not a regular file"));
    }
    
    void auto_start()
    {
      auto service_iterator=LAppSConfig::getInstance()->getLAppSConfig()["services"].begin();
      auto end_iterator=LAppSConfig::getInstance()->getLAppSConfig()["services"].end();
      for(;service_iterator!=end_iterator;++service_iterator)
      {
        std::string service_name(service_iterator.key());
        const bool auto_start=service_iterator.value()["auto_start"];
        if(auto_start)
          restart_service(service_name);
      }
    }
    
    void deploy_service(const fs::path& dir, const json& service_config)
    {
      itc::getLog()->info(__FILE__,__LINE__,"Deploying service: %s",dir.u8string().c_str());
      try
      {
        const bool internal{service_config["internal"]};
        const bool auto_start{service_config["auto_start"]};
        const size_t instances{service_config["instances"]};
        const std::string service_name=service_config["name"];
        
        const bool already_exists=(
          LAppSConfig::getInstance()->getLAppSConfig()["services"].find(service_name)
          !=
          LAppSConfig::getInstance()->getLAppSConfig()["services"].end()
        );
        
        std::string app_dir=LAppSConfig::getInstance()->getLAppSConfig()["directories"]["applications"];
        fs::path service_path{fs::path(static_cast<const std::string&>(mEnv["LAPPS_HOME"])) / app_dir / service_name};
        
        if(fs::exists(service_path))
        {
          fs::remove_all(service_path);
        }
        
        if(internal)
        {
          if(already_exists)
          {
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["internal"]=internal;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["instances"]=instances;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["auto_start"]=auto_start;
          }else{
            json sconfig({ service_name , {
              {"internal",internal},
              {"instances",instances},
              {"auto_start",auto_start}
            }});
            LAppSConfig::getInstance()->getLAppSConfig()["services"].push_back(sconfig);
          }
          fs::rename(dir,service_path);          
        }
        else
        { // non internal
          const std::string target=service_config["request_target"];
          const std::string protocol=service_config["protocol"];
          const size_t max_in_msg_size{service_config["max_inbound_message_size"]};

          if(already_exists)
          {
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["internal"]=internal;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["instances"]=instances;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["auto_start"]=auto_start;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["request_target"]=target;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["protocol"]=protocol;
            LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["max_inbound_message_size"]=max_in_msg_size;
          }
          else
          {
            json sconfig{{
              service_name,{
                { "internal", internal },
                { "instances", instances },
                { "auto_start", auto_start },
                { "request_target", target },
                { "protocol", protocol },
                { "max_inbound_message_size", max_in_msg_size }
              }
            }};
            LAppSConfig::getInstance()->getLAppSConfig()["services"].insert(sconfig.begin(),sconfig.end());
          }
          
          fs::rename(dir, service_path);
        }
      }
      catch(const std::exception& e){
        itc::getLog()->error(
          __FILE__,__LINE__,
          "Service is not configured properly (unpacked from archive) into dir [%s]. Exception [%s]",
          dir.u8string().c_str(),e.what()
        );
        itc::getLog()->flush();
        return;
      }
      
      const bool config_auto_save{LAppSConfig::getInstance()->getWSConfig()["lapps_config_auto_save"]};
      
      if(config_auto_save)
      {
        LAppSConfig::getInstance()->save();
      }
    }
  public:
    void stop_service(const std::string& service_name)
    {
      try{
        const bool internal{LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["internal"]};
        if(internal)
        {
          ::InternalAppsRegistry::getInstance()->stopInstances(service_name);
        }else{
          ::ApplicationRegistry::getInstance()->unRegApp(service_name);
        }
      }
      catch(const std::exception& e)
      {
        itc::getLog()->error(
          __FILE__,__LINE__,
          "Can't stop service %s, because of the exception: %s",
          service_name.c_str(), e.what()
        );
        itc::getLog()->flush();
      }
    }
    
    void start_service(const std::string& service_name)
    {
      try
      {
        const bool internal=LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["internal"];
        const size_t instances=LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["instances"];
        
        const std::string apps_dir=LAppSConfig::getInstance()->getLAppSConfig()["directories"]["applications"];
        
        fs::path lua_module_path_extend{fs::path(static_cast<const std::string&>(mEnv["LAPPS_HOME"])) / apps_dir / fs::path(service_name)};
        
        mEnv.setEnv("LUA_PATH", mEnv["LUA_PATH"] + ";" + lua_module_path_extend.u8string() + "/?.lua");
        
        itc::getLog()->info(__FILE__,__LINE__,"Starting service %s with %d instance(s)",service_name.c_str(),instances);    
        if(internal)
        {  
          for(size_t i=0;i<instances;++i)
          {
            ::InternalAppsRegistry::getInstance()->startInstance(service_name);
          }
        }
        else
        {
          const std::string target=LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["request_target"];
          const std::string protocol=LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["protocol"];
          const size_t max_in_msg_size=LAppSConfig::getInstance()->getLAppSConfig()["services"][service_name]["max_inbound_message_size"];

          for(size_t i=0;i<instances;++i)
          {
            if(protocol == "raw")
            {
              ::ApplicationRegistry::getInstance()->regApp(
                  std::make_shared<RAWPROTOApp>(service_name,target,max_in_msg_size),
                  instances
              );
            }
            else if(protocol == "LAppS")
            {
              ::ApplicationRegistry::getInstance()->regApp(
                  std::make_shared<LAppSPROTOApp>(service_name,target,max_in_msg_size),
                  instances
              );
            }else{
              throw std::system_error(EINVAL,std::system_category(),"Unknown protocol is configured for service "+service_name);
            }
          }
        }
      }
      catch(const std::exception& e)
      {
        itc::getLog()->error(
          __FILE__,__LINE__,
          "Service [%s] is configured inappropriately in %s/lapps.json, - exception: %s.",
          service_name.c_str(),
          mEnv["LAPPS_CONF_DIR"].c_str(),
          e.what()
        );
      }
    }
    
    void restart_service(const std::string& service_name)
    {
      stop_service(service_name);
      start_service(service_name);
    }
    explicit Deployer()
    : mMayRun{true},mMutex(),mDeploymentQueue(),
      mInotifyFD(inotify_init()),mEnv()
    {
      const std::string deploy_dir=LAppSConfig::getInstance()->getLAppSConfig()["directories"]["deploy"];
      mDeployDir=fs::path(static_cast<const std::string>(mEnv["LAPPS_HOME"])) / deploy_dir;
      
      if(mInotifyFD == -1)
        throw std::system_error(errno,std::system_category(),"Deployer::Deployer(), exception on inotify_init()");
      
      if(!fs::exists(mDeployDir))
        throw std::system_error(ENOENT,std::system_category(),"Deployer::Deployer() - no such directory "+mDeployDir.u8string() +" to watch for LARs.");

      int ret=inotify_add_watch(mInotifyFD,mDeployDir.u8string().c_str(),IN_CLOSE_WRITE);
      
      if(ret == -1)
        throw std::system_error(errno,std::system_category(),"Deployer::Deployer(), exception on inotify_add_watch()");
      
      itc::getLog()->info(__FILE__,__LINE__,"Deploying all services");
      deploy_all();
      itc::getLog()->info(__FILE__,__LINE__,"Starting all services with auto_start enabled");
      auto_start();
    }
    
    Deployer(const Deployer&)=delete;
    Deployer(Deployer&)=delete;
    ~Deployer(){shutdown();}
    
    void onCancel() {}
    void shutdown() {mMayRun.store(false);}
    void execute()
    {
      size_t buffer_size=sizeof(struct inotify_event) + NAME_MAX + 1;
      std::vector<uint8_t> buffer(buffer_size);

      while(mMayRun.load())
      { 
        int ret=read(mInotifyFD,buffer.data(),buffer_size);
        
        if(ret < 0)
        {
          itc::getLog()->error(__FILE__,__LINE__,"Error on inotify read. Going down.");
          mMayRun.store(false);
          break;
        }

        buffer.resize(ret);
        
        auto p=(inotify_event*)(buffer.data());
        std::string name(p->name,p->len);
        fs::path archive_name(std::move(name));
        try{
          fs::path service_name(std::move(deploy_archive(archive_name)));
          restart_service(service_name.stem().u8string());
        }
        catch(const std::exception& e)
        {
          itc::getLog()->error(__FILE__,__LINE__,"Error on deployment of an archive %s: %s ",archive_name.u8string().c_str(),e.what());
        }
        buffer.resize(buffer_size);
      }
    }
  };
}

#endif /* __DEPLOYER_H__ */

