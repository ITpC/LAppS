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
 *  $Id: MQBroker.h July 18, 2018 5:19 AM $
 * 
 **/


#ifndef __MQREGISTRY_H__
#  define __MQREGISTRY_H__

#include <memory>
#include <string>
#include <map>
#include <functional>

#include <sys/mutex.h>
#include <sys/synclock.h>
#include <Singleton.h>

#include <tsbqueue.h>

#include <ext/json.hpp>


namespace LAppS
{
  namespace MQ
  {
    typedef std::shared_ptr<json>                             MessageType;
    typedef itc::tsbqueue<MessageType,itc::sys::mutex>        QueueType;
    typedef std::shared_ptr<QueueType>                        QueueHolderType;
    
    class Registry
    {
     private:
      itc::sys::mutex                  mMutex;
      std::map<size_t,QueueHolderType> mQueueMap;
      
     public:
      explicit Registry()=default;
      Registry(const Registry&)=delete;
      Registry(Registry&)=delete;
      
      const size_t create(const std::string& name)
      {
        ITCSyncLock sync(mMutex);
        size_t hash=std::hash<std::string>{}(name);
        auto it=mQueueMap.find(hash);
        
        if(it==mQueueMap.end())
        {
          mQueueMap.emplace(hash,std::make_shared<QueueType>());
        }
        return hash;
      }
      
      auto find(const std::string& name)
      {
        ITCSyncLock sync(mMutex);
        size_t hash=std::hash<std::string>{}(name);
        auto it=mQueueMap.find(hash);
        if(it!=mQueueMap.end())
          return it->second;
        else
          throw std::system_error(ENOENT,std::system_category(),name+", - no such queue");
      }
      
      auto find(const size_t& id)
      {
        ITCSyncLock sync(mMutex);
        auto it=mQueueMap.find(id);
        if(it!=mQueueMap.end())
          return it->second;
        else
          throw std::system_error(ENOENT,std::system_category(),std::string("The queue with id ")+std::to_string(id)+std::string(" is not available"));
      }
    };
    
    typedef itc::Singleton<Registry> QueueRegistry;
    
  }
}

#endif /* __MQREGISTRY_H__ */

