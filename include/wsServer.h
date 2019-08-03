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
#include <limits>

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

#include <abstract/Worker.h>
#include <Deployer.h>
#include <NetworkACL.h>
#include <WSWorkersPool.h>

#include <Balancer.h>

//wolfSSL
#include <wolfSSLLib.h>


namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false>
  class wsServer
  {
  private:
    using WorkerType=LAppS::IOWorker<TLSEnable,StatsEnable>;
    using WorkersPool=itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>;
    using DeployerType=LAppS::Deployer<TLSEnable,StatsEnable>;
    using DeployerThread=itc::sys::CancelableThread<DeployerType>;
    using BalacersPool=std::vector<std::shared_ptr<LAppS::Balancer<TLSEnable,StatsEnable>>>;
    
    itc::utils::Bool2Type<TLSEnable>    enableTLS;
    itc::utils::Bool2Type<StatsEnable>  enableStatsUpdate;
    float                               mConnectionWeight;
    size_t                              mWorkers;
    std::shared_ptr<LAppS::NetworkACL>  mNACL;
    DeployerThread                      mDeployer;
    BalacersPool                        mBalancersPool;
    std::vector<TCPListenerThreadSPtr>  mListenersPool;
    
    void loadNACL()
    {
      std::string default_policy=LAppSConfig::getInstance()->getWSConfig()["acl"]["policy"];
      if(default_policy == "allow")
      {
        mNACL=LAppS::GlobalNACL::getInstance(LAppS::Network_ACL_Policy::ALLOW);
      } else if(default_policy == "deny")
      {
        mNACL=LAppS::GlobalNACL::getInstance(LAppS::Network_ACL_Policy::DENY);
      } else {
        throw std::system_error(EINVAL,std::system_category(), "Invalid value for ws_config.acl.policy: "+default_policy+", must be one of: deny, allow");
      }
      auto it=LAppSConfig::getInstance()->getWSConfig()["acl"]["exclude"].begin();
      auto eit=LAppSConfig::getInstance()->getWSConfig()["acl"]["exclude"].end();
      
      for(;it!=eit;++it)
      {
        std::string addr=it.value();
        mNACL->add(std::move(LAppS::addrinfo(std::move(addr))));
      }
    }

    void startListeners()
    {
      loadNACL();
      
      try
      {
        size_t max_listeners=LAppSConfig::getInstance()->getWSConfig()["listeners"];

        for(size_t i=0;i<max_listeners;++i)
        {

          itc::getLog()->trace(__FILE__,__LINE__,"wsServer creating a listener %zu",i);

          auto balancer=std::make_shared<LAppS::Balancer<TLSEnable,StatsEnable>>(LAppSConfig::getInstance()->getWSConfig()["connection_weight"]);
          mBalancersPool.push_back(balancer);
          
          mListenersPool.push_back(std::make_shared<TCPListenerThread>(
            std::make_shared<::itc::TCPListener>(
              LAppSConfig::getInstance()->getWSConfig()["ip"],
              LAppSConfig::getInstance()->getWSConfig()["port"],
              balancer,
              [this](const uint32_t address)
              {
                switch(this->mNACL->mPolicy)
                {
                  case LAppS::Network_ACL_Policy::ALLOW:
                  {
                    if(this->mNACL->match(address))
                      return true;
                    return false;
                  } break;
                  case LAppS::Network_ACL_Policy::DENY:
                  {
                    if(this->mNACL->match(address))
                      return false;
                    return true;
                  } break;
                }
                return false;
              }
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
    wsServer()
    : enableTLS(), enableStatsUpdate(), mConnectionWeight(
        LAppSConfig::getInstance()->getWSConfig()["connection_weight"]
      ), mWorkers(1), mDeployer{std::make_shared<DeployerType>()}
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
          /*
          auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
          if(tls_server_context == nullptr)
          {
            throw std::logic_error("Can't continue, TLS Server Context is NULL");
          }
          */
        }

        auto found=LAppSConfig::getInstance()->getWSConfig().find("workers");
        size_t max_connections=100000;
        bool auto_fragment=false;

        if(found != LAppSConfig::getInstance()->getWSConfig().end())
        {
          mWorkers=found.value()["workers"];

          auto max_connections_found=found.value().find("max_connections");

          if((max_connections_found != found.value().end())&&max_connections_found.value().is_number_integer())
          {
            max_connections=max_connections_found.value();
          }
          else
          {
            itc::getLog()->error(__FILE__,__LINE__,"workers.max_connections option is not set or of inappropriate type, using default value: %zu",max_connections);
          }
          auto auto_fragment_found=found.value().find("auto_fragment");
          if((auto_fragment_found!=found.value().end())&&(auto_fragment_found.value().is_boolean()))
          {
            auto_fragment=auto_fragment_found.value();
          }
          else
          {
            itc::getLog()->error(__FILE__,__LINE__,"workers.auto_fragment option is not set or of inappropriate type, using default value: false");
          }
        }
        
        for(size_t i=0;i<mWorkers;++i)
        {
          WorkersPool::getInstance()->spawn(max_connections,auto_fragment);
        }
        startListeners();
    };


    ~wsServer()
    {
      itc::getLog()->info(__FILE__,__LINE__,"wsServer is down");
      itc::getLog()->flush();
    }
  };
}

#endif /* __WSSERVER_H__ */

