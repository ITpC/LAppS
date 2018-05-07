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
 *  $Id: InternalApplicationRegistry.h May 7, 2018 1:43 PM $
 * 
 **/


#ifndef __INTERNALAPPLICATIONREGISTRY_H__
#  define __INTERNALAPPLICATIONREGISTRY_H__

#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <queue>

#include <sys/synclock.h>

#include <InternalApplication.h>

namespace LAppS
{
  class InternalApplicationRegistry
  {
   public:
    typename std::shared_ptr<InternalApplication>           IntAppSPtr;
    typedef itc::sys::CancelableThread<InternalApplication> InternalAppThread;
    typedef std::shared_ptr<InternalAppThread>              InternalAppThreadSPtr;
    typedef std::queue<InternalAppThreadSPtr>               InternalAppsQueue;
    
   private:
    std::mutex mMutex;
    std::map<std::string,InternalAppsQueue> mRegistry;
    
   public:
    explicit InternalApplicationRegistry():mMutex(),mRegistry()
    {
    }
    
    InternalApplicationRegistry(const InternalApplicationRegistry&)=delete;
    InternalApplicationRegistry(InternalApplicationRegistry&)=delete;
    
    void startInstance(const std::string& name)
    {
      ::SyncLock sync(mMutex);
      auto it=mRegistry.find(name);
      if(it==mRegistry.end())
      {
        mRegistry.emplace(name,InternalAppsQueue());
      }
      mRegistry[name].emplace(std::make_shared<InternalAppThread>(std::make_shared<InternalApplication>(name)));
    }

    const bool stopInstance(const std::string& name)
    {
      auto it=mRegistry.find(name);
      if(it!=mRegistry.end())
      {
        if(!it->second.empty())
        {
          it->second.pop();
          return true;
        }
        return false;
      }
      return false;
    }
    
    void clear()
    {
      SyncLock sync(mMutex);
      mRegistry.clear();
    }
    
    ~InternalApplicationRegistry()
    {
      this->clear();
    }
  };
}

typedef itc::Singleton<LAppS::InternalApplicationRegistry> InternalAppsRegistry;


#endif /* __INTERNALAPPLICATIONREGISTRY_H__ */

