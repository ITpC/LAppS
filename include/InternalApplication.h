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
 *  $Id: InternalApplication.h May 7, 2018 10:38 AM $
 * 
 **/


#ifndef __INTERNALAPPLICATION_H__
#  define __INTERNALAPPLICATION_H__
#include <chrono>
#include <string>
#include <functional>

#include <InternalAppContext.h>
#include <abstract/Runnable.h>
#include <Nonce.h>

namespace LAppS
{
  class InternalApplication : public itc::abstract::IRunnable
  {
   private:
    size_t mInstanceID;
    InternalAppContext mContext;
   public:
    explicit InternalApplication(const std::string name)
    : mInstanceID(
        std::hash<std::string>{}(std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch()
        ).count())+std::to_string(std::hash<size_t>{}(InstanceIdNonce::getInstance()->getNext())))
      ), mContext(name)
    {
    }
    
    InternalApplication(const InternalApplication&)=delete;
    InternalApplication(InternalApplication&)=delete;
    InternalApplication()=delete;
    
    const size_t getInstanceId() const
    {
      return mInstanceID;
    }
    
    void execute()
    {
      mContext.init();
      mContext.run();
    }
    ~InternalApplication()
    {
      this->shutdown();
    }
    void onCancel()
    {
      this->shutdown();
    }
    void shutdown()
    {
      itc::getLog()->info(__FILE__,__LINE__,"Shutdown is requested for an instance %ul  of the internal application %s",mInstanceID, mContext.getName().c_str());
      mContext.stop();
      
      itc::sys::Nap waithere;
      
      while(!mContext.mCanStop.load())
      {
        waithere.usleep(10000);
      }

      itc::getLog()->info(__FILE__,__LINE__,"Instance %ul  of an internal application %s is down",mInstanceID, mContext.getName().c_str());
    }
  };
}


#endif /* __INTERNALAPPLICATION_H__ */

