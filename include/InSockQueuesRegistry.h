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
 *  $Id: InSockQueuesRegistry.h March 10, 2018 4:28 AM $
 * 
 **/


#ifndef __INSOCKQUEUESREGISTRY_H__
#  define __INSOCKQUEUESREGISTRY_H__

#include <string>
#include <map>
#include <mutex>

#include <sys/synclock.h>
#include <tsbqueue.h>

#include <WebSocket.h>

template <bool TLSEnable=false, bool StatsEnable=false> class InSockQueuesRegistry
{
public:
 typedef WebSocket<TLSEnable,StatsEnable> WSType;
 typedef typename WSType::Mode            WSMode;
 typedef std::shared_ptr<WSType>          WSSPtr;
 
 typedef itc::tsbqueue<WSSPtr> SocketsQueue;
 typedef std::shared_ptr<SocketsQueue> SocketsQueueSPtr;
 
 const SocketsQueueSPtr& find(const std::string& qname)
 {
   SyncLock sync(mMutex);
   auto it=mQMap.find(qname);
   if(it!=mQMap.end())
     return it->second;
   throw std::system_error(EINVAL,std::system_category(),"No such queue: "+qname);
 }
 void create(const std::string& name)
 {
   SyncLock sync(mMutex);
   mQMap.emplace(name,std::make_shared<SocketsQueue>());
 }
 void clear()
 {
   SyncLock sync(mMutex);
   mQMap.clear();
 }
private:
 std::mutex mMutex;
 std::map<std::string,SocketsQueueSPtr> mQMap;
};


#endif /* __INSOCKQUEUESREGISTRY_H__ */

