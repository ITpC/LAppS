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
 *  $Id: Broadcasts.h May 3, 2018 9:00 AM $
 * 
 **/


#ifndef __BROADCASTS_H__
#  define __BROADCASTS_H__

#include <set>
#include <mutex>
#include <string>
#include <sys/synclock.h>

#include <Broadcast.h>
#include <Singleton.h>

namespace LAppS
{
  template <bool TLSEnable=false,bool StatsEnable=false> class Broadcasts
  {
  public:
   typedef Broadcast<TLSEnable,StatsEnable> BCastValueType;
  private:
    itc::sys::mutex mMutex;
    std::map<size_t,std::shared_ptr<BCastValueType>> mBroadcasts;
    
  public:
   explicit Broadcasts():mMutex(){}
   
   Broadcasts(const Broadcasts&)=delete;
   Broadcasts(Broadcasts&)=delete;
   
   const bool create(const size_t bcastid)
   {
     ITCSyncLock sync(mMutex);
     auto it=mBroadcasts.find(bcastid);
     if(it==mBroadcasts.end())
     {
       mBroadcasts.emplace(bcastid,std::move(std::make_shared<BCastValueType>(bcastid)));
       return true;
     }
     return false;
   }
   
   auto find(const size_t bcastid)
   {
     ITCSyncLock sync(mMutex);
     auto it=mBroadcasts.find(bcastid);
     if(it!=mBroadcasts.end())
     {
       return it->second;
     }
     throw std::runtime_error("Broadcast with id "+std::to_string(bcastid)+" is not found");
   }
  };
  
  #ifdef LAPPS_TLS_ENABLE
  #ifdef STATS_ENABLE
      typedef Broadcasts<true,true> BCastType;
      
  #else
      typedef Broadcasts<true,false> BCastType;
  #endif
#else
  #ifdef STATS_ENABLE
      typedef Broadcasts<false,true> BCastType;
  #else
      typedef Broadcasts<false,false> BCastType;
  #endif
#endif
  typedef itc::Singleton<BCastType> BroadcastRegistry;
}

#endif /* __BROADCASTS_H__ */

