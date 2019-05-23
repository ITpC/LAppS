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
 *  $Id: ConnectionsInQueue.h May 6, 2019 1:44 AM $
 * 
 **/


#ifndef __CONNECTIONSINQUEUE_H__
#  define __CONNECTIONSINQUEUE_H__

#include <TCPListener.h>
#include <abstract/IView.h>
#include <cfifo.h>
#include <Singleton.h>
#include <Config.h>

namespace LAppS
{
  using queue_type=::itc::cfifo<::itc::TCPListener::value_type>;
  using ViewType=::itc::TCPListener::ViewType;
  
  class ConnectionsInQueue : public queue_type, public ViewType
  {
  public:
   explicit ConnectionsInQueue(const size_t sz) : queue_type(sz) {}
   explicit ConnectionsInQueue() : queue_type(LAppSConfig::getInstance()->getWSConfig()["cinq_depth"]) {}
   
   void onUpdate(const ::itc::TCPListener::value_type& data)
    {
      this->send(std::move(data));
    }
   
    void onUpdate(const std::vector<::itc::TCPListener::value_type>& data)
    {
      for(size_t i=0;i<data.size();++i)
      {
        this->send(std::move(data[i]));
      }
    }
    
    ~ConnectionsInQueue() override
    {
      this->shutdown();
    }
  };
  typedef itc::Singleton<ConnectionsInQueue> CInQ;
}

#endif /* __CONNECTIONSINQUEUE_H__ */

