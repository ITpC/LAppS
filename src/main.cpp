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
#include <TSLog.h>
#include <wsServer.h>
#include <sys/Nanosleep.h>
#include <HTTPRequestParser.h>


int main(int argc, char** argv)
{
  
  itc::getLog()->debug(__FILE__,__LINE__,"Starting LAppS");

#ifdef LAPPS_TLS_ENABLE
  #ifdef STATS_ENABLE
      wsServer<true,true> server;
  #else
      wsServer<true,false> server;
  #endif
#else
  #ifdef STATS_ENABLE
      wsServer<false,true> server;
  #else
      wsServer<false,false> server;
  #endif
#endif
 
  
  server.run();
  itc::getLog()->debug(__FILE__,__LINE__,"LAppS is down");
  return 0;
}