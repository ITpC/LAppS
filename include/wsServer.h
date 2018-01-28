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
#include <Worker.h>
#include <InboundConnectionsPool.h>

// libressl
#include <tls.h>


class wsServer
{
private:
  std::vector<TCPListenerThreadSPtr> mListenersPool;
  std::vector<WorkerThreadSPtr> mWorkers;
  std::vector<TLSWorkerThreadSPtr> mTLSWorkers;
  std::shared_ptr<InboundConnections> mInbound;
public:
  wsServer()
  : mInbound(std::make_shared<InboundConnections>())
  {
    itc::getLog()->debug(__FILE__,__LINE__,"Starting WS Server");
    size_t maxConnectionsPerWorker=LAppSConfig::getInstance()->getWSConfig()["workers"]["maxConnections"];
    size_t max_listeners=LAppSConfig::getInstance()->getWSConfig()["listeners"];
    size_t max_workers=LAppSConfig::getInstance()->getWSConfig()["workers"]["workers"];
    const bool is_tls_enabled=LAppSConfig::getInstance()->getWSConfig()["tls"];
    
    if(is_tls_enabled)
    {
      for(size_t i=0;i<max_workers;++i)
      {
        mTLSWorkers.push_back(
          std::make_shared<TLSWorkerThread>(
            std::make_shared<TLSWorker>(maxConnectionsPerWorker,mInbound)
          )
        );
      }
    }
    else
    {
      for(size_t i=0;i<max_workers;++i)
      {
        mWorkers.push_back(
          std::make_shared<WorkerThread>(
            std::make_shared<nonTLSWorker>(maxConnectionsPerWorker,mInbound)
          )
        );
      }
    }
    
    
    itc::getLog()->debug(__FILE__,__LINE__,"max workers: %d, max listeners %d",max_workers,max_listeners);
    

    for(size_t i=0;i<max_listeners;++i)
    {
      mListenersPool.push_back(std::make_shared<TCPListenerThread>(
        std::make_shared<itc::TCPListener>(
          LAppSConfig::getInstance()->getWSConfig()["ip"],
          LAppSConfig::getInstance()->getWSConfig()["port"],
          mInbound
        )
      ));
    }
  };

  void run()
  {
    int c=getchar();
    std::cout << c << std::endl;
    
    mInbound->destroy();
    itc::getLog()->flush();
  }
  ~wsServer()
  {
    itc::getLog()->debug(__FILE__,__LINE__,"wsServer is down");
  }
};


#endif /* __WSSERVER_H__ */

