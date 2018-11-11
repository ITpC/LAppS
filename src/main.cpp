/**
 *  Copyright 2017-2018, Pavel Kraynyukhov <pavel.kraynyukhov@gmail.com>
 * 
 *  This file is part of LAppS (Lua Application Server).
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
 *  along with LAppS .  If not, see <http://www.gnu.org/licenses/>.
 * 
 *  $Id: main.cpp December 29, 2017 8:10 AM $
 * 
 **/

#include <sys/types.h>
#include <unistd.h>
#include <TSLog.h>
#include <wsServer.h>
#include <sys/Nanosleep.h>
#include <HTTPRequestParser.h>
#include <bz2Compression.h>
#include <ServiceRegistry.h>
#include <NetworkACL.h>

void startWSServer()
{
  
#ifdef LAPPS_TLS_ENABLE
  #ifdef STATS_ENABLE
      LAppS::wsServer<true,true> server;
  #else
      LAppS::wsServer<true,false> server;
  #endif
#else
  #ifdef STATS_ENABLE
      LAppS::wsServer<false,true> server;
  #else
      LAppS::wsServer<false,false> server;
  #endif
#endif

  signal(SIGHUP, SIG_IGN);
  
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
      itc::getLog()->info(__FILE__,__LINE__,"Shutdown is initiated by signal %d, - server is going down",signo);
      itc::getLog()->flush();
      LAppS::SServiceRegistry::getInstance()->clear();
#ifdef LAPPS_TLS_ENABLE
  #ifdef STATS_ENABLE
      itc::Singleton<LAppS::WSWorkersPool<true,true>>::getInstance()->clear();
  #else
      itc::Singleton<LAppS::WSWorkersPool<true,false>>::getInstance()->clear();
  #endif
#else
  #ifdef STATS_ENABLE
      itc::Singleton<LAppS::WSWorkersPool<false,true>>::getInstance()->clear();
  #else
      itc::Singleton<LAppS::WSWorkersPool<false,false>>::getInstance()->clear();
  #endif
#endif
      
      break;
    }
    else
    {
      throw std::system_error(errno, std::system_category(),"LAppS failure on sigwait()");
    }
  }
}

int main(int argc, char** argv)
{ 
  itc::getLog()->info(__FILE__,__LINE__,"Starting LAppS");
  
  if(argc > 1)
  {
    if(strncmp(argv[0],"-d",2)==1)
    {
      auto pid=fork();
      if(pid == 0)
      {
        setsid();
        startWSServer();
      }
    }else{
      startWSServer();
    }
  }else{
    startWSServer();
  }
  
      
  itc::getLog()->info(__FILE__,__LINE__,"LAppS is down");
  itc::getLog()->flush();
  return 0;
}