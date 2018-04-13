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
 *  $Id: Application.h March 17, 2018 3:53 AM $
 * 
 **/


#ifndef __ABSTRACT_APPLICATION_H__
#  define __ABSTRACT_APPLICATION_H__

#include <abstract/Runnable.h>
#include <WSProtocol.h>
#include <WSEvent.h>
#include <TaggedEvent.h>


namespace abstract
{
  class Application : public itc::abstract::IRunnable
  {
   public:
    enum Protocol { RAW, LAPPS };
    
    Application()=default;
    Application(const Application&)=delete;
    Application(Application&)=delete;
    
    virtual void enqueue(const TaggedEvent&)=0;
    virtual const ::abstract::Application::Protocol getProtocol() const=0;
    virtual const std::string& getName() const=0;
    virtual const std::string& getTarget() const=0;
    virtual ~Application()=default;
  };
}

typedef std::shared_ptr<abstract::Application>            ApplicationSPtr;
typedef itc::sys::CancelableThread<abstract::Application> ApplicationThread;
typedef std::shared_ptr<ApplicationThread>                ApplicationThreadSPtr;
typedef ::abstract::Application::Protocol                 ApplicationProtocol;


#endif /* __ABSTRACT_APPLICATION_H__ */

