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
 *  $Id: WSClientsPool.h August 13, 2018 4:20 PM $
 * 
 **/


#ifndef __WSCLIENTSPOOL_H__
#  define __WSCLIENTSPOOL_H__

#include <map>
#include <memory>
#include <string>

#include <ClientWebSocket.h>
#include <ePoll.h>

namespace LAppS
{
  class WSClientPool
  {
   private:
    ePoll                                 mEPoll;
    std::map<int32_t,ClientWebSocketSPtr> mPool;
    
   public:
    explicit WSClientPool():mEPoll(), mPool() {}
    WSClientPool(const WSClientPool&)=delete;
    WSClientPool(WSClientPool&)=delete;
    
    ~WSClientPool()
    {
      mPool.clear();
    }
    
    const int32_t create(const std::string& uri)
    {
      auto tmp=std::make_shared<ClientWebSocket>(uri,true,true);
      mPool.emplace(tmp->getfd(),tmp);
      mEPoll.add_in(tmp->getfd());
      return tmp->getfd();
    }
    
    void remove(const int32_t idx)
    {
      auto it=mPool.begin();
      if(it != mPool.end())
        mPool.erase(it);
    }
    
    auto poll(std::vector<epoll_event>& out)
    {
      return mEPoll.poll(out,1);
    }
    
    auto operator[](const int32_t idx)
    {
      return mPool[idx];
    }
    
    void mod_in(const int32_t fd)
    {
      mEPoll.mod_in(fd);
    }
    
    auto find(const int32_t fd)
    {
      return mPool.find(fd);
    }
    auto begin()
    {
      return mPool.begin();
    }
    auto end()
    {
      return mPool.end();
    }
  };
}

#endif /* __WSCLIENTSPOOL_H__ */

