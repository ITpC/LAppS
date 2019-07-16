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
#include <forward_list>

#include <WSProtocol.h>
#include <abstract/WebSocket.h>

namespace LAppS
{
  template <bool TLSEnable=true,bool StatsEnable=true> class Broadcast
  {
   public:
    typedef std::forward_list<std::shared_ptr<::abstract::WebSocket>>  BCastSubscribers;
   private:
    itc::sys::mutex           sMutex;
    itc::sys::mutex           uMutex;
    size_t                    ChannelID;
    BCastSubscribers          mSubscribers;
    BCastSubscribers          mUnsubscribers;
    
    void purge()
    {
      ITCSyncLock syncu(uMutex);
      
      auto uit=mUnsubscribers.begin();
      while(uit!=mUnsubscribers.end())
      {
        for(auto it=mSubscribers.before_begin();it!=mSubscribers.end();++it)
        { 
          auto next=std::next(it);
          if((next!=mSubscribers.end())&&((*next)==(*uit)))
          {
            mSubscribers.erase_after(it);
            break;
          }
        }
        ++uit;
      }
      mUnsubscribers.clear();
    }
    
   public:
    
    explicit Broadcast(const size_t chid) : sMutex(), uMutex(), ChannelID(chid)
    {
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
    
    void subscribe(::abstract::WebSocket* handler)
    {
      ITCSyncLock sync(sMutex);
      mSubscribers.push_front(std::move(handler->get_shared()));
    }
    void unsubscribe(::abstract::WebSocket* handler)
    {
      ITCSyncLock sync(uMutex);
      mUnsubscribers.push_front(std::move(handler->get_shared()));
    }
    
    void bcast(const MSGBufferTypeSPtr& msg)
    {
      ITCSyncLock sync(sMutex);
      purge();
      
      for(auto handler : mSubscribers)
      {
        handler->send(*msg);
      }
    }
  };
}

#endif /* __BROADCAST_H__ */

