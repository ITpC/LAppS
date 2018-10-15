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
 *  $Id: StandaloneService.h September 24, 2018 9:57 AM $
 * 
 **/


#ifndef __STANDALONESERVICE_H__
#  define __STANDALONESERVICE_H__

namespace LAppS
{
  namespace abstract
  {
    class StandaloneService : public LAppS::abstract::Service
    {
     public:
      explicit StandaloneService() : LAppS::abstract::Service() {}
      StandaloneService(const StandaloneService&)=delete;
      StandaloneService(StandaloneService&)=delete;
      virtual ~StandaloneService() noexcept = default;
      
      const bool isReactive() const 
      {
        return false;
      }
      const bool isStandalone() const
      {
        return true;
      }
      const std::string& getTarget() const
      {
        static std::string target;
        return target;
      }
      const ::LAppS::ServiceProtocol getProtocol() const
      {
        return ::LAppS::ServiceProtocol::INTERNAL;
      }
    };
  }
}

#endif /* __STANDALONESERVICE_H__ */

