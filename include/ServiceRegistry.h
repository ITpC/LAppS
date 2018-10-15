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
 *  $Id: ServiceRegistry.h October 2, 2018 4:12 PM $
 * 
 **/


#ifndef __SERVICEREGISTRY_H__
#  define __SERVICEREGISTRY_H__

#include <string>
#include <memory>
#include <vector>
#include <map>

#include <sys/CancelableThread.h>
#include <sys/atomic_mutex.h>
#include <sys/synclock.h>

#include <Singleton.h>

#include <abstract/Service.h>

#include <ext/json.hpp>

using json=nlohmann::json;

namespace LAppS
{
  struct ServicesInstanceHolder
  {
  private:
    using Instances=std::vector<::LAppS::ServiceInstanceStoreType>;
   
    mutable size_t current;
    Instances mInstances;
    
  public:
    explicit ServicesInstanceHolder():current{0},mInstances(){}
    ServicesInstanceHolder(const ServicesInstanceHolder&)=delete;
    ServicesInstanceHolder(ServicesInstanceHolder&)=delete;
    
    void push_back(const ServiceInstanceStoreType& instance)
    {
      mInstances.push_back(std::move(instance));
    }
    
    const bool hasNext() const
    {
      return mInstances.size()>0;
    }
    
    const auto& next() const
    {
      if(hasNext())
      {
        if((current+1)<mInstances.size())
        {
          current++;
        }else{
          current=0;
        }
        return mInstances[current]->getRunnable();
      }
      throw std::runtime_error("ServicesInstanceHolder::next() - empty");
    }
    
    const bool pop_back()
    {
      if(hasNext())
      {
        mInstances.pop_back();
        if(current>mInstances.size())
        {
          current=0;
        }
        return true;
      }
      return false;
    }
    
    const auto& findById(const size_t& _id) const
    {
      for(const auto& instance : mInstances)
      {
        if(instance->getRunnable()->getInstanceId() == _id)
        {
          return instance->getRunnable();
        }
      }
      throw std::system_error(EINVAL,std::system_category(),"Instance with Id "+std::to_string(_id)+" is not available");
    }
    
    const auto list() const noexcept
    {
      std::shared_ptr<json> retobj=std::make_shared<json>(json::array());
      
      for(const auto& instance: mInstances)
      {
        json j={
          { "Id", instance->getRunnable()->getInstanceId() },
          { "Name", instance->getRunnable()->getName() },
          { "Target", "" }
        };
        
        if(instance->getRunnable()->isReactive())
        {
          j["Target"] = instance->getRunnable()->getTarget();
          j["Reactive"] = true;
          j["Standalone"] = false;
        }
        else
        {
          j["Reactive"] = false;
          j["Standalone"] = true;
        }
        retobj->push_back(j);
      }
      return retobj;
    }
    
    void clear()
    {
      mInstances.clear();
      current=0;
    }
    ~ServicesInstanceHolder()
    {
      clear();
    }
  };
  
  class ServiceRegistry
  {
   private:
    mutable ::itc::sys::AtomicMutex                 mMutex;
    std::map<std::string,ServicesInstanceHolder>    mServices;
    std::map<std::string,std::string>               mTargets2Names;
    
   public:
    explicit ServiceRegistry()=default;
    ServiceRegistry(ServiceRegistry&)=delete;
    ServiceRegistry(const ServiceRegistry&)=delete;
    
    void clear()
    {
      AtomicLock sync(mMutex);
      mTargets2Names.clear();
      mServices.clear();
    }
    
    ~ServiceRegistry() noexcept
    {
      this->clear();
    }
    
    void reg(const ServiceInstanceStoreType& instance)
    {
      AtomicLock sync(mMutex);
      
      if(instance->getRunnable()->isReactive())
      {
        mTargets2Names[instance->getRunnable()->getTarget()]=instance->getRunnable()->getName();
      }
      
      mServices[instance->getRunnable()->getName()].push_back(std::move(instance));
    }
    
    void unreg(const std::string& name)
    {
      AtomicLock sync(mMutex);
      auto it=mServices.find(name);
      if(it!=mServices.end())
      {
        if(it->second.next()->isReactive())
        {
          auto tit=mTargets2Names.find(it->second.next()->getTarget());

          if(tit!=mTargets2Names.end())
            mTargets2Names.erase(tit);
        }
        
        mServices.erase(it);
      }
    }
    
    const auto& findByName(const std::string& name) const
    {
      AtomicLock sync(mMutex);
      auto it=mServices.find(name);
      if(it!=mServices.end())
      {
        return it->second.next();
      }
      throw std::system_error(EINVAL,std::system_category(),std::string("ServiceRegistry::findByName(")+name+"), - no such service");
    }
    
    const auto& findByTarget(const std::string& target) const
    {
      AtomicLock sync(mMutex);
      auto tit=mTargets2Names.find(target);
      if(tit!=mTargets2Names.end())
      {
        auto it=mServices.find(tit->second);
        if(it!=mServices.end())
        {
          return it->second.next();
        }
        throw std::system_error(EINVAL,std::system_category(),std::string("ServiceRegistry::findByTarget(")+tit->second+"), - no such service");
      }
      throw std::system_error(EINVAL,std::system_category(),std::string("ServiceRegistry::findByTarget(")+target+"), - no such target");
    }
    
    const auto& findById(const size_t& _id) const
    {
      for(const auto& instances: mServices)
      {
        try{
          return instances.second.findById(_id);
        }
        catch(const std::exception& e)
        {
        }
      }
      throw std::system_error(EINVAL,std::system_category(),std::string("ServiceRegistry::findById(")+std::to_string(_id)+"), - no such instance");
    }
    
    const auto list() const noexcept
    {
      AtomicLock sync(mMutex);
      std::shared_ptr<json> retobj=std::make_shared<json>(json::array());
      for(const auto& instances: mServices)
      {
        retobj->push_back(*(instances.second.list()));
      }
      return retobj;
    }
  };
  using SServiceRegistry=itc::Singleton<ServiceRegistry>;
}


#endif /* __SERVICEREGISTRY_H__ */

