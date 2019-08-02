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
 *  $Id: ServiceContext.h September 18, 2018 9:55 AM $
 * 
 **/


#ifndef __SERVICECONTEXT_H__
#  define __SERVICECONTEXT_H__

#include <ContextTypes.h>

namespace LAppS
{
  namespace abstract
  {
    class ServiceContext
    {
     private:
      std::string     mName;
      
     public:
      
      explicit ServiceContext(
        const std::string& name
      ): mName{name}
      {
      }
      
      const std::string& getName() const
      {
        return mName;
      }
      
      ServiceContext()=delete;
      ServiceContext(const ServiceContext&)=delete;
      ServiceContext(ServiceContext&)=delete;
      
      virtual void init() = 0;
      virtual void shutdown() = 0;
      
      virtual const ::LAppS::ServiceProtocol getProtocol() const = 0;
      virtual const ::LAppS::ServiceLanguage getLanguage() const = 0;
      virtual std::atomic<bool>* get_stop_flag_address() = 0;
      
      virtual ~ServiceContext() noexcept = default;
      
    };
  }
}

#endif /* __SERVICECONTEXT_H__ */

