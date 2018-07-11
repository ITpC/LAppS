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

#include <memory>
#include <vector>
#include <chrono>

#include <abstract/Worker.h>
#include <abstract/Runnable.h>

#include <WSProtocol.h>
#include <WSEvent.h>
#include <TaggedEvent.h>
#include <abstract/WebSocket.h>
#include <Sequence.h>
#include <Singleton.h>

typedef itc::Singleton<itc::Sequence<uint64_t,true>> InstanceIdNonce;

namespace abstract
{
  typedef std::shared_ptr<::abstract::WebSocket> WSSPtrType;
  
  
  struct AppInEvent
  {
    WebSocketProtocol::OpCode opcode;
    WSSPtrType                websocket;
    MSGBufferTypeSPtr         message;
  };
  
  class Application : public itc::abstract::IRunnable
  {
   private:
    size_t        mInstanceID;
   public:
    enum Protocol { RAW, LAPPS };
    
    Application(): mInstanceID(
      std::hash<std::string>{}(std::to_string(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::time_point_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now()).time_since_epoch()
      ).count())+std::to_string(std::hash<size_t>{}(InstanceIdNonce::getInstance()->getNext())))
    ){};
    
    Application(const Application&)=delete;
    Application(Application&)=delete;
    
    const size_t getInstanceId() {
      return mInstanceID;
    }
    
    virtual const bool isUp() const=0;
    virtual void enqueue(const AppInEvent&)=0;
    virtual void enqueue(const std::vector<AppInEvent>&)=0;
    virtual const bool try_enqueue(const std::vector<AppInEvent>&)=0;
    virtual const ::abstract::Application::Protocol getProtocol() const=0;
    virtual const std::string& getName() const=0;
    virtual const std::string& getTarget() const=0;
    virtual const size_t getMaxMSGSize() const=0;
    virtual const bool filter(const uint32_t)=0;
    
    virtual ~Application() noexcept = default;
  };
}

typedef std::shared_ptr<abstract::Application>            ApplicationSPtr;
typedef itc::sys::CancelableThread<abstract::Application> ApplicationThread;
typedef std::shared_ptr<ApplicationThread>                ApplicationThreadSPtr;
typedef ::abstract::Application::Protocol                 ApplicationProtocol;


#endif /* __ABSTRACT_APPLICATION_H__ */

