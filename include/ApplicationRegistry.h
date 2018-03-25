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
    std::mutex mMutex;

    std::map<std::string,AppInstances> mApplications;
    std::map<std::string,std::string>  mTarget2Name;

   public:
    void regApp(const ApplicationSPtr& instance)
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
        instances->second.push(
          std::make_shared<ApplicationThread>(instance)
        );
      }
      else
      {
        mApplications.emplace(instance->getName(),AppInstances());
        mApplications[instance->getName()].push(
          std::make_shared<ApplicationThread>(instance)
        );
      }
      mTarget2Name.emplace(instance->getTarget(),instance->getName());
    }

    auto findByName(const std::string& name)
    {
      SyncLock sync(mMutex);
      auto it=mApplications.find(name);
      if(it!=mApplications.end())
      {
        if(it->second.size() > 0)
        {
          const ApplicationThreadSPtr& app=it->second.front(); // round-robin access to instances
          it->second.pop();
          it->second.push(app);
          return app->getRunnable();
        }
        else throw std::system_error(ENOENT, std::system_category(), name+", - no instances available");
      }
      throw std::system_error(ENOENT, std::system_category(), name+", - no such application");
    }
    auto findByTarget(const std::string& target)
    {
      SyncLock sync(mMutex);
      auto target_iter=mTarget2Name.find(target);
      if(target_iter!=mTarget2Name.end())
      {
        auto name=target_iter->second;
        auto it=mApplications.find(name);
        if(it!=mApplications.end())
        {
          if(it->second.size() > 0)
          {
            const ApplicationThreadSPtr& app=it->second.front(); // round-robin access to instances
            it->second.pop();
            it->second.push(app);
            return app->getRunnable();
          }
          else throw std::system_error(ENOENT, std::system_category(), name+", - no instances available");
        }
        throw std::system_error(ENOENT, std::system_category(), name+", - no such application");
      }
      throw std::system_error(ENOENT, std::system_category(), target+", - no such target");
    }
  };
}

typedef itc::Singleton<LAppS::ApplicationRegistry> ApplicationRegistry;

#endif /* __APPLICATIONREGISTRY_H__ */

