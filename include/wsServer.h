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
#include <InboundConnectionsPool.h>
#include <abstract/Worker.h>
#include <WSWorker.h>
#include <InSockQueuesRegistry.h>
#include <TLSServerContext.h>

// libressl
#include <tls.h>



template <bool TLSEnable=false, bool StatsEnable=false>
class wsServer
{
private:
  typedef WSWorker<TLSEnable,StatsEnable>             WorkerType;
  typedef typename WorkerType::WSMode                 WSMode;
  typedef InSockQueuesRegistry<TLSEnable,StatsEnable> ISQRType;
  typedef std::shared_ptr<ISQRType>                   ISQRegistry;
  typedef InboundConnections<TLSEnable,StatsEnable>   ICType;
  typedef std::shared_ptr<ICType>                     ICTypeSPtr;
  
  itc::utils::Bool2Type<TLSEnable>    enableTLS;
  itc::utils::Bool2Type<StatsEnable>  enableStatsUpdate;
  WorkerStats                         mAllStats;
  
  std::vector<ICTypeSPtr>             mShakers;
  std::vector<WorkerThreadSPtr>       mWorkers;
  std::vector<TCPListenerThreadSPtr>  mListenersPool;
  ISQRegistry                         mISQR;
  
  
  void prepareServices()
  {
    try
    {
      size_t      workers=0;
      size_t      max_connections=0;
      WSMode      aWSMode;
      
      auto it=LAppSConfig::getInstance()->getLAppSConfig().find("services");
      if(it!=LAppSConfig::getInstance()->getLAppSConfig().end())
      {
        for(size_t i=0;i<it.value().size();++i)
        {
          auto service=it.value()[i].begin();
          for(;service != it.value()[i].end();++service)
          {
            std::string proto;
            std::string path;
            
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
              path=found.value();
            }
            else
            {
              path.clear();
            }

            found=service.value().find("workers");
            if(found != service.value().end())
            {
              workers=found.value()["workers"];
              
              if(workers>255)
              {
                throw std::system_error(EINVAL,std::system_category(),"Configuration of service "+service.key()+" error: workers > 255. Max Workers hard limit is 255");
              }
              
              auto max_connections_found=found.value().find("max_connections");
              
              if(max_connections_found != found.value().end())
              {
                max_connections=max_connections_found.value();
              }
              else
              {
                max_connections=100;
              }
            }
            else {
              workers=1;
              max_connections=100;
            }


            if((!path.empty())&&(!proto.empty()))
            {
              std::regex request_target("^[/][[:alpha:][:digit:]_-]*([/]?[[:alpha:][:digit:]_-]+)*$");

              if(std::regex_match(path,request_target))
              {
                if(proto.empty())
                  throw std::system_error(EINVAL,std::system_category(), "Application protocol is empty in configuration of the service "+service.key());
                if(proto == "raw")
                {
                    aWSMode=WSMode::RAW;
                }else if(proto == "LAppS")
                {
                    aWSMode=WSMode::LAPPS;
                }else{
                    throw std::system_error(EINVAL,std::system_category(), "Incorrect protocol is specified for target "+path+" in service "+service.key()+". There are only 2 protocols are available: raw, LAppS");
                }


                mISQR->create(path);

                for(size_t i=0;i<workers;++i)
                {
                  mWorkers.push_back(
                    std::make_shared<WorkerThread>(
                      std::make_shared<WorkerType>(
                        static_cast<const uint8_t>(i),max_connections,aWSMode,
                        mISQR->find(path), path
                      )
                    )
                  );
                }
              } else throw std::system_error(EINVAL,std::system_category(), "Incorrect request target: "+path+" in configuration of service "+service.key());
            }
            
          }
        }
      }
      
      
      size_t max_listeners=LAppSConfig::getInstance()->getWSConfig()["listeners"];
      
      for(size_t i=0;i<max_listeners;++i)
      {
        auto shaker=std::make_shared<ICType>(mISQR);
        mShakers.push_back(shaker);
        mListenersPool.push_back(std::make_shared<TCPListenerThread>(
          std::make_shared<itc::TCPListener>(
            LAppSConfig::getInstance()->getWSConfig()["ip"],
            LAppSConfig::getInstance()->getWSConfig()["port"],
            shaker
          )
        ));
      }
      
      
      
    }catch(const std::exception& e)
    {
      itc::getLog()->fatal(__FILE__,__LINE__,"Configuration error %s. Abort.",e.what());
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
    
    for(auto it=mWorkers.begin();it!=mWorkers.end();++it)
    {
      auto stats=it->get()->getRunnable()->getStats();
      mAllStats.mConnections+=stats.mConnections;
      mAllStats.mInMessageCount+=stats.mInMessageCount;
      mAllStats.mOutMessageCount+=stats.mOutMessageCount;
      
      
      if(mAllStats.mInMessageMaxSize<stats.mInMessageMaxSize)
      {
        mAllStats.mInMessageMaxSize=stats.mInMessageMaxSize;
      }
      if(mAllStats.mOutMessageMaxSize<stats.mOutMessageMaxSize)
      {
        mAllStats.mOutMessageMaxSize=stats.mOutMessageMaxSize;
      }
      mAllStats.mOutCMASize+=stats.mOutCMASize;
      mAllStats.mInCMASize+=stats.mInCMASize;
      
      
    }
    mAllStats.mOutCMASize/=mWorkers.size();
    mAllStats.mInCMASize/=mWorkers.size();
    return mAllStats;
  }
  
  const WorkerStats& collectStats()
  {
    return collectStats(enableStatsUpdate);
  }
  
  wsServer()
  : enableTLS(), enableStatsUpdate(), mAllStats{0,0,0,0,0,0,0}, 
    mISQR(std::make_shared<ISQRType>())
  {
     itc::getLog()->debug(__FILE__,__LINE__,"Starting WS Server");
     
     const bool is_tls_enabled=LAppSConfig::getInstance()->getWSConfig()["tls"];

     if(TLSEnable&&(!is_tls_enabled))
     {
       throw std::system_error(EINVAL,std::system_category(),"WS Server is build TLS support, tls option MUST BE enabled in config to start LAppS");
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
     prepareServices();
  };

  void run()
  {
    
    sigset_t sigset;
    int signo;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGQUIT);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    while(1)
    {
      if(sigwait(&sigset, &signo) == 0)
      {
        itc::getLog()->debug(__FILE__,__LINE__,"Shutdown is initiated by signal %d, - server is going down",signo);
        itc::getLog()->flush();
        mISQR->clear();
        itc::getLog()->flush();
        break;
      }
      else
      {
        throw std::system_error(errno, std::system_category(),"wsServer::run() failed on sigwait()");
      }
    }
  }
  ~wsServer()
  {
    itc::getLog()->debug(__FILE__,__LINE__,"wsServer is down");
  }
};


#endif /* __WSSERVER_H__ */

