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
 *  $Id: Service.h September 18, 2018 9:35 AM $
 * 
 **/


#ifndef __SERVICE_H__
#  define __SERVICE_H__

#include <string>
#include <chrono>
#include <memory>
#include <Nonce.h>
#include <abstract/Runnable.h>
#include <ContextTypes.h>

#include <sys/CancelableThread.h>
#include <AppInEvent.h>

namespace LAppS
{
  namespace abstract
  {
    class Service : public ::itc::abstract::IRunnable
    {
     private:
      size_t              mInstanceId;
      
     public:
      explicit Service()
      : mInstanceId{
          std::hash<std::string>{}(std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch()
          ).count())+std::to_string(std::hash<size_t>{}(InstanceIdNonce::getInstance()->getNext())))
      }{
      }
      
      Service(const Service&) = delete;
      Service(Service&)=delete;
      
      
      const size_t& getInstanceId() const
      {
        return mInstanceId;
      }
      
      virtual const std::string& getName() const = 0;
      virtual const std::string& getTarget() const = 0;
      virtual const bool isUp() const = 0;
      virtual const bool isDown() const = 0;
      virtual const bool isReactive() const = 0;
      virtual const bool isStandalone() const = 0;
      virtual const ::LAppS::ServiceLanguage getLanguage() const = 0;
      virtual const ::LAppS::ServiceProtocol getProtocol() const = 0;
      virtual const bool filterIP(const uint32_t address) const=0;
      virtual void shutdown() = 0;
      virtual const size_t getMaxMSGSize() const=0;
      virtual void enqueue(const AppInEvent&)=0;
      //virtual const bool try_enqueue(const std::vector<AppInEvent>&)=0;
      
      virtual ~Service() noexcept = default;
    };
  }
  using ServiceSPtrType=::std::shared_ptr<abstract::Service>;
  using ServiceInstanceType=::itc::sys::CancelableThread<abstract::Service>;
  using ServiceInstanceStoreType=::std::shared_ptr<ServiceInstanceType>;
}

#endif /* __SERVICE_H__ */

