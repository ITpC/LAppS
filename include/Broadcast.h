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
 *  $Id: Broadcast.h May 3, 2018 5:25 AM $
 * 
 **/


#ifndef __BROADCAST_H__
#  define __BROADCAST_H__

#include <sys/synclock.h>
#include <WSWorkersPool.h>
#include <forward_list>
#include <TaggedEvent.h>

#include "WSProtocol.h"

namespace LAppS
{
  template <bool TLSEnable=true,bool StatsEnable=true> class Broadcast
  {
   public:
    typedef std::forward_list<size_t>                         BCastSubscribers;
    typedef std::vector<std::shared_ptr<::abstract::Worker>>  WorkersCache;
   private:
    mutable std::mutex        sMutex;
    mutable std::mutex        uMutex;
    size_t                    ChannelID;
    mutable WorkersCache      mWorkersCache;
    mutable BCastSubscribers  mSubscribers;
    mutable BCastSubscribers  mUnsubscribers;
    
    void purge() const
    {
      SyncLock syncu(uMutex);
      
      auto uit=mUnsubscribers.begin();
      while(uit!=mUnsubscribers.end())
      {
        for(auto it=mSubscribers.before_begin();it!=mSubscribers.end();++it)
        { 
          auto next=std::next(it);
          if(*next==*uit)
          {
            mSubscribers.erase_after(it);
            break;
          }
        }
      }
      mUnsubscribers.clear();
    }
    
   public:
    
    explicit Broadcast(const size_t chid) : sMutex(), uMutex(), ChannelID(chid)
    {
      itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->getWorkers(mWorkersCache);
    }
    
    Broadcast()=delete;
    Broadcast(const Broadcast&)=delete;
    Broadcast(Broadcast&)=delete;
    
    const bool operator>(const size_t& chid) const
    {
      return ChannelID > chid;
    }
    
    const bool operator<(const size_t& chid) const
    {
      return ChannelID < chid;
    }
    
    const bool operator>(const Broadcast& ref) const
    {
      return ChannelID > ref.ChannelID;
    }
    
    const bool operator<(const Broadcast& ref) const
    {
      return ChannelID < ref.ChannelID;
    }
    const bool operator==(const Broadcast& ref) const
    {
      return ChannelID == ref.ChannelID;
    }
    
    operator const size_t() const
    {
      return ChannelID;
    }
    
    void subscribe(const size_t handler) const
    {
      SyncLock sync(sMutex);
      mSubscribers.push_front(handler);
    }
    void unsubscribe(const size_t handler) const
    {
      SyncLock sync(uMutex);
      mSubscribers.push_front(handler);
    }
    
    void bcast(const MSGBufferTypeSPtr& msg) const
    {
      SyncLock sync(sMutex);
      purge();
      for(auto handler : mSubscribers)
      {
        size_t wid=handler>>32;
        int32_t fd=static_cast<int32_t>(handler&0x00000000FFFFFFFF);
        if(wid<mWorkersCache.size())
        {
          mWorkersCache[wid]->submitResponse({wid,fd,{WebSocketProtocol::BINARY,msg}});
        }
        else // second attempt
        {
          mWorkersCache.clear(); 
          itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->getWorkers(mWorkersCache);
          if(wid<mWorkersCache.size())
          {
            mWorkersCache[wid]->submitResponse({wid,fd,{WebSocketProtocol::BINARY,msg}});
          } // not broadcasted, worker is down or never existed.
        }
      }
    }
  };
}

#endif /* __BROADCAST_H__ */

