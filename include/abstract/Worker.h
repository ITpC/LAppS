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
 *  $Id: Worker.h February 22, 2018 6:28 PM $
 * 
 **/


#ifndef __WORKER_H__
#  define __WORKER_H__

#include <string>
#include <memory>
#include <abstract/Runnable.h>
#include <TCPListener.h>
#include <WorkerStats.h>
#include <WSEvent.h>
#include <ext/json.hpp>

using json=nlohmann::json;

namespace abstract
{
  class Worker : public ::itc::abstract::IRunnable
  //, public ::itc::TCPListener::ViewType
  {
   protected:
    const size_t  ID;
    size_t        mMaxConnections;
    bool          auto_fragment;
   public:
    explicit Worker(const size_t id, const size_t maxConnections, const bool af)
    : itc::abstract::IRunnable(), ID(id),mMaxConnections(maxConnections),
      auto_fragment(af)
    {
      sigset_t sigset;
      sigemptyset(&sigset);
      sigaddset(&sigset, SIGQUIT);
      sigaddset(&sigset, SIGINT);
      sigaddset(&sigset, SIGTERM);
      if(pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0)
        throw std::system_error(errno,std::system_category(),"Can not mask signals [SIGQUIT,SIGINT,SIGTERM");
    }
      
    Worker()=delete;
    Worker(const Worker&)=delete;
    Worker(Worker&)=delete;
    
    
    void setMaxConnections(const size_t maxconn)
    {
      mMaxConnections=maxconn;
    }
    
    const size_t getMaxConnections() const
    {
      return mMaxConnections;
    }
    
    const bool mustAutoFragment() const
    {
      return auto_fragment;
    }
    
    const size_t getID() const
    {
      return ID;
    }
    virtual void enqueue(const ::itc::TCPListener::value_type&)=0;
    virtual void deleteConnection(const int32_t)=0;
    virtual void disconnect(const int32_t)=0;
    virtual const bool  isTLSEnabled() const = 0;
   protected:
    virtual ~Worker()=default;
  };
}

typedef typename std::shared_ptr<abstract::Worker> WorkerSPtr;
typedef std::weak_ptr<abstract::Worker> WorkerWPtr;
typedef typename std::vector<WorkerSPtr> WorkersVector;

#endif /* __WORKER_H__ */

