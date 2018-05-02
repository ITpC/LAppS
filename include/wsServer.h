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
 *  $Id: wsServer.h January 6, 2018 11:56 AM $
 * 
 **/


#ifndef __WSSERVER_H__
#  define __WSSERVER_H__

// C headers
#include <stdio.h>

// C++ std headers
#include <vector>
#include <memory>
#include <iostream>
#include <thread>
#include <regex>

// ITC Headers
#include <TCPListener.h>
#include <abstract/IView.h>
#include <sys/CancelableThread.h>
#include <ThreadPoolManager.h>
#include <sys/Nanosleep.h>
#include <TSLog.h>

// Extra external headers
#include <ext/json.hpp>

// This app headers

#include <Config.h>
#include <TLSServerContext.h>
#include <abstract/Worker.h>
#include <abstract/Application.h>
#include <ApplicationRegistry.h>
#include <Application.h>
#include <IOWorker.h>
#include <Balancer.h>

// libressl
#include <tls.h>



template <bool TLSEnable=false, bool StatsEnable=false>
class wsServer
{
private:
  typedef LAppS::IOWorker<TLSEnable,StatsEnable>                    WorkerType;
  typedef itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>      WorkersPool;
  
  
  typedef LAppS::Application<TLSEnable,StatsEnable,ApplicationProtocol::LAPPS> LAppLAPPS;
  typedef LAppS::Application<TLSEnable,StatsEnable,ApplicationProtocol::RAW> LAppRAW;
  
  
  itc::utils::Bool2Type<TLSEnable>    enableTLS;
  itc::utils::Bool2Type<StatsEnable>  enableStatsUpdate;
  float                               mConnectionWeight;
  size_t                              mWorkers;
  WorkerStats                         mAllStats;
    
  std::vector<TCPListenerThreadSPtr>  mListenersPool;
  itc::sys::CancelableThread<Balancer<TLSEnable,StatsEnable>> mBalancer;
  
  
  
  void prepareServices()
  {
    try
    {
      auto it=LAppSConfig::getInstance()->getLAppSConfig().find("services");
      if(it!=LAppSConfig::getInstance()->getLAppSConfig().end())
      {
        for(size_t i=0;i<it.value().size();++i)
        {
          
          auto service=it.value()[i].begin();
          std::string service_name=service.key();
          
          for(;service != it.value()[i].end();++service)
          {
            std::string proto;
            std::string app_target;
            
            auto found=service.value().find("protocol");
            if(found != service.value().end())
            {
              proto=found.value();
            }
            else
            {
              proto.clear();
            }
            found=service.value().find("request_target");
            if(found != service.value().end())
            {
              app_target=found.value();
            }
            else
            {
              app_target.clear();
            }

            if((!app_target.empty())&&(!proto.empty()))
            {
              std::regex request_target("^[/][[:alpha:][:digit:]_-]*([/]?[[:alpha:][:digit:]_-]+)*$");

              if(std::regex_match(app_target,request_target))
              {
                if(proto.empty())
                  throw std::system_error(EINVAL,std::system_category(), "Application protocol is empty in configuration of the service "+service_name);
                if(proto == "raw")
                {
                  size_t instances=1;
                  
                  found=service.value().find("instances");
                  if(found != service.value().end())
                  {
                    instances=found.value();
                  }
                  for(size_t i=0;i<instances;++i)
                  {
                    ApplicationRegistry::getInstance()->regApp(
                      std::make_shared<LAppRAW>(service_name,app_target)
                    );
                  }
                }
                else if(proto == "LAppS")
                {
                  size_t instances=1;
                  
                  found=service.value().find("instances");
                  if(found != service.value().end())
                  {
                    instances=found.value();
                  }
                  for(size_t i=0;i<instances;++i)
                  {
                    ApplicationRegistry::getInstance()->regApp(
                      std::make_shared<LAppLAPPS>(service_name,app_target)
                    );
                  }
                }else{
                    throw std::system_error(EINVAL,std::system_category(), "Incorrect protocol is specified for target "+app_target+" in service "+service_name+". Only two protocols are supported: raw, LAppS");
                }
              } else throw std::system_error(EINVAL,std::system_category(), "Incorrect request target: "+app_target+" in configuration of service "+service.key());
            } else throw std::system_error(EINVAL,std::system_category(), "Undefined request_target or protocol keyword which are both mandatory");
          }
        }
      }
      
      size_t max_listeners=LAppSConfig::getInstance()->getWSConfig()["listeners"];
      
      for(size_t i=0;i<max_listeners;++i)
      {
        
        itc::getLog()->trace(__FILE__,__LINE__,"wsServer creating a listener %u",i);
        
        mListenersPool.push_back(std::make_shared<TCPListenerThread>(
          std::make_shared<::itc::TCPListener>(
            LAppSConfig::getInstance()->getWSConfig()["ip"],
            LAppSConfig::getInstance()->getWSConfig()["port"],
            //WorkersPool::getInstance()->next()
            mBalancer.getRunnable()
          )
        ));
      }
      
    }catch(const std::exception& e)
    {
      itc::getLog()->fatal(__FILE__,__LINE__,"Caught an exception: %s. Abort.",e.what());
      itc::getLog()->flush();
      exit(1);
    }
  }
  
  
public:
 
  const WorkerStats& collectStats(const itc::utils::Bool2Type<false> stats_collection_is_disabled)
  {
    return mAllStats;
  }
  const WorkerStats& collectStats(const itc::utils::Bool2Type<true> stats_collection_is_enabled)
  {
    return WorkersPool::getInstance()->getStats();
  }
  
  const WorkerStats& collectStats()
  {
    return collectStats(enableStatsUpdate);
  }
  
  wsServer()
  : enableTLS(), enableStatsUpdate(), mConnectionWeight(
      LAppSConfig::getInstance()->getWSConfig()["connection_weight"]
    ), mWorkers(1), mAllStats{0,0,0,0,0,0,0,0}, 
    mBalancer(std::make_shared<Balancer<TLSEnable, StatsEnable>>(mConnectionWeight))
  {
      itc::getLog()->info(__FILE__,__LINE__,"Starting WS Server");

      const bool is_tls_enabled=LAppSConfig::getInstance()->getWSConfig()["tls"];

      if(TLSEnable&&(!is_tls_enabled))
      {
        throw std::system_error(EINVAL,std::system_category(),"WS Server is build with TLS support, tls option MUST BE enabled in config to start LAppS");
      }

      if(is_tls_enabled&&(!TLSEnable))
       throw std::system_error(EINVAL,std::system_category(),"WS Server is build without TLS support, disable tls option in config to start LAppS (insecure)");
      else if(is_tls_enabled)
      {
        auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
        if(tls_server_context == nullptr)
        {
          throw std::logic_error("Can't continue, TLS Server Context is NULL");
        }
      }
      
      auto found=LAppSConfig::getInstance()->getWSConfig().find("workers");
      size_t max_connections=1000;
      
      if(found != LAppSConfig::getInstance()->getWSConfig().end())
      {
        mWorkers=found.value()["workers"];

        auto max_connections_found=found.value().find("max_connections");

        if(max_connections_found != found.value().end())
        {
          max_connections=max_connections_found.value();
        }
      }
      
      for(size_t i=0;i<mWorkers;++i)
      {
        WorkersPool::getInstance()->spawn(max_connections);
      }
      prepareServices();
  };

 
  ~wsServer()
  {
    itc::getLog()->info(__FILE__,__LINE__,"wsServer is down");
    itc::getLog()->flush();
  }
};


#endif /* __WSSERVER_H__ */

