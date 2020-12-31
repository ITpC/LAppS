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
 *  $Id: LuaStandaloneService.h September 26, 2018 6:11 PM $
 * 
 **/


#ifndef __LUASTANDALONESERVICE_H__
#  define __LUASTANDALONESERVICE_H__

#include <abstract/Service.h>
#include <abstract/StandaloneService.h>
#include <LuaStandaloneServiceContext.h>


namespace LAppS
{
  class LuaStandaloneService : public abstract::StandaloneService
  {
   private:
    LuaStandaloneServiceContext mContext;
    
   public:
    LuaStandaloneService(const std::string& name)
      :abstract::StandaloneService(), mContext{name,this->getInstanceId()}
    {
    }
      
    void execute()
    {
      try{
        mContext.init();
        try{
          mContext.run();
        }catch(const std::exception& e)
        {
          ITC_ERROR(__FILE__,__LINE__,"Runtime error in service {}: {}",mContext.getName().c_str(),e.what());
        }
      }catch(const std::exception& e)
      {
        ITC_ERROR(__FILE__,__LINE__,"Exception on initialization of service {}: {}",mContext.getName().c_str(),e.what());
      }
      
    }
    
    std::atomic<bool>* get_stop_flag_address()
    {
      return mContext.get_stop_flag_address();
    }
    const bool filterIP(const uint32_t address) const
    {
      return false;
    }
    
    ~LuaStandaloneService()
    {
      this->shutdown();
      while(mContext.isRunning()) itc::sys::sched_yield(5000);
    }
    
    void onCancel()
    {
      this->shutdown();
    }
    
    void shutdown()
    {
      ITC_INFO(__FILE__,__LINE__,"Shutdown is requested for the instance {} of the service {}",this->getInstanceId(), mContext.getName().c_str());
      mContext.stop();
      ITC_INFO(__FILE__,__LINE__,"Instance {} of the service {} is down",this->getInstanceId(), mContext.getName().c_str());
    }
    
    const ::LAppS::ServiceLanguage getLanguage() const
    {
      return ::LAppS::ServiceLanguage::LUA;
    }
    const std::string& getName() const
    {
      return mContext.getName();
    }
    
    const bool isUp() const
    {
      return mContext.isRunning();
    }
    const bool isDown() const
    {
      return !mContext.isRunning();
    }
    const size_t getMaxMSGSize() const
    {
      return 0;
    }
    void enqueue(const AppInEvent&& e)
    {
      throw std::logic_error("Interface method void LuaStandaloneService::enqueue(const AppInEvent&) may not be implemented");
    }
  };
}

#endif /* __LUASTANDALONESERVICE_H__ */

