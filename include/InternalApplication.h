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

#include <InternalAppContext.h>
#include <abstract/Runnable.h>

namespace LAppS
{
  class InternalApplication : public itc::abstract::IRunnable
  {
   private:
    InternalAppContext mContext;
   public:
    explicit InternalApplication(const std::string name) : mContext(name)
    {
      mContext.init();
    }
    InternalApplication(const InternalApplication&)=delete;
    InternalApplication(InternalApplication&)=delete;
    InternalApplication()=delete;
    
    void execute()
    {
      mContext.run();
    }
    ~InternalApplication()
    {
      mContext.stop();
    }
  };
}


#endif /* __INTERNALAPPLICATION_H__ */

