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
 *  $Id: ReactiveService.h March 17, 2018 3:53 AM $
 * 
 **/


#ifndef __ABSTRACT_REAVTIVE_SERVICE_H__
#  define __ABSTRACT_REAVTIVE_SERVICE_H__

#include <vector>

#include <abstract/Service.h>
#include <AppInEvent.h>
#include <ContextTypes.h>


namespace LAppS
{
  namespace abstract
  {
    class ReactiveService : public abstract::Service
    {
     private:
      std::string mTarget;
     public:

      explicit ReactiveService(const std::string& target) 
      : ::LAppS::abstract::Service(),mTarget(target)
      {}

      ReactiveService()=delete;
      ReactiveService(const ReactiveService&)=delete;
      ReactiveService(ReactiveService&)=delete;
      virtual ~ReactiveService() noexcept = default;

      virtual const size_t getMaxMSGSize() const=0;
      virtual const bool filter(const uint32_t)=0;
      
      const std::string& getTarget() const
      {
        return mTarget;
      }
      
      const bool isReactive() const 
      {
        return true;
      }
      const bool isStandalone() const
      {
        return false;
      }
    };
  }
}

#endif /* __ABSTRACT_REAVTIVE_SERVICE_H__ */

