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
 *  $Id: ApplicationRegistry.h March 25, 2018 7:49 AM $
 * 
 **/


#ifndef __APPLICATIONREGISTRY_H__
#  define __APPLICATIONREGISTRY_H__

#include <memory>
#include <string>
#include <mutex>
#include <map>
#include <queue>


#include <abstract/Application.h>
#include <sys/synclock.h>
#include <Singleton.h>


namespace LAppS
{
  class ApplicationRegistry
  {
    typedef std::queue<ApplicationThreadSPtr> AppInstances;
    mutable std::mutex mMutex;

    std::map<std::string,AppInstances> mApplications;
    std::map<std::string,std::string>  mTarget2Name;
    std::map<size_t, ApplicationSPtr> mInstanceID2Instance;

   public:
    void clear()
    {
      SyncLock sync(mMutex);
      mTarget2Name.clear();
      for(auto it=mApplications.begin();it!=mApplications.end();++it)
      {
        while(!it->second.empty())
        {
          it->second.pop();
        }
      }
      mApplications.clear();
    }
    void listApps(std::queue<std::string>& out)
    {
      SyncLock sync(mMutex);
      for(auto it=mApplications.begin();it!=mApplications.end();++it)
      {
        out.push(it->first);
      }
    }
    void unRegApp(const std::string name)
    {
      SyncLock sync(mMutex);
      auto it=mApplications.find(name);
      if(it!=mApplications.end())
      {
        auto target=it->second.front()->getRunnable()->getTarget();
        auto instanceid=it->second.front()->getRunnable()->getInstanceId();
        auto target_it=mTarget2Name.find(target);
        if(target_it != mTarget2Name.end())
        {
          mTarget2Name.erase(target_it);
        }
        mApplications.erase(it);
        auto iidit=mInstanceID2Instance.find(instanceid);
        if(iidit!=mInstanceID2Instance.end())
        {
          mInstanceID2Instance.erase(iidit);
        }
      }
    }
    
    void regApp(const ApplicationSPtr& instance,const size_t maxInstances)
    {
      SyncLock sync(mMutex);

      auto already_mapped=mTarget2Name.find(instance->getTarget());

      if(already_mapped!=mTarget2Name.end())
      {
        if(already_mapped->second!=instance->getName())
        {
          throw std::system_error(EINVAL,std::system_category(),"Can't register more then one application per target");
        }
      }

      auto instances=mApplications.find(instance->getName());

      if(instances!=mApplications.end())
      {
        if(instances->second.size()<maxInstances){
          instances->second.push(
            std::make_shared<ApplicationThread>(instance)
          );  
        }
      }
      else
      {
        mApplications.emplace(instance->getName(),AppInstances());
        mApplications[instance->getName()].push(
          std::make_shared<ApplicationThread>(instance)
        );
      }
      mTarget2Name.emplace(instance->getTarget(),instance->getName());
      mInstanceID2Instance.emplace(instance->getInstanceId(),instance);
    }

    auto getByInstanceId(const size_t& instanceid)
    {
      SyncLock sync(mMutex);
      auto it=mInstanceID2Instance.find(instanceid);
      if(it!=mInstanceID2Instance.end())
      {
        return it->second;
      }
      throw std::runtime_error(std::string("The instance with id ")+std::to_string(instanceid)+std::string(" is not registered"));
    }
    
    auto getByName(const std::string& name)
    {
      SyncLock sync(mMutex);
      auto it=mApplications.find(name);
      if(it!=mApplications.end())
      {
        while(it->second.size() > 0)
        {
          ApplicationThreadSPtr app=it->second.front(); // round-robin access to instances
          it->second.pop();
          if(app->getRunnable()->isUp())
          {
            it->second.push(app);
            return app->getRunnable();
          }
        }
        throw std::system_error(ENOENT, std::system_category(), name+", - no instances available");
      }
      throw std::system_error(ENOENT, std::system_category(), name+", - no such application");
    }
    auto getByTarget(const std::string& target)
    {
      SyncLock sync(mMutex);
      auto target_iter=mTarget2Name.find(target);
      if(target_iter!=mTarget2Name.end())
      {
        auto name=target_iter->second;
        auto it=mApplications.find(name);
        if(it!=mApplications.end())
        {
          while(it->second.size() > 0)
          {
            ApplicationThreadSPtr app=it->second.front(); // round-robin access to instances
            it->second.pop();
            if(app->getRunnable()->isUp())
            {
              it->second.push(app);
              return app->getRunnable();
            }
          }
          throw std::system_error(ENOENT, std::system_category(), name+", - no instances available");
        }
        throw std::system_error(ENOENT, std::system_category(), name+", - no such application");
      }
      throw std::system_error(ENOENT, std::system_category(), target+", - no such target");
    }
    const size_t countInstances(const std::string& name) const
    {
      SyncLock sync(mMutex);
      auto it=mApplications.find(name);
      if(it!=mApplications.end())
      {
        return it->second.size();
      }
      return 0;
    }
  };
}

typedef itc::Singleton<LAppS::ApplicationRegistry> ApplicationRegistry;

#endif /* __APPLICATIONREGISTRY_H__ */

