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
 *  $Id: Balancer.h May 2, 2018 1:50 PM $
 * 
 **/


#ifndef __BALANCER_H__
#  define __BALANCER_H__

#include <TCPListener.h>
#include <WSWorkersPool.h>
#include <tsbqueue.h>
#include <sys/CancelableThread.h>
#include <atomic>
#include <memory>
#include <abstract/Runnable.h>

namespace LAppS
{
  
  
  template <bool TLSEnable=true, bool StatsEnable=true> 
  class Balancer 
  : public ::itc::TCPListener::ViewType,
    public ::itc::abstract::IRunnable
  {
  private:
    
    float                                             mConnectionWeight;
    std::atomic<bool>                                 mMayRun;
    itc::tsbqueue<::itc::TCPListener::value_type>     mInbound;
    std::vector<std::shared_ptr<::abstract::Worker>>  mWorkersCache;

  public:
    void onUpdate(const ::itc::TCPListener::value_type& data)
    {
      mInbound.send(data);
    }

    void onUpdate(const std::vector<::itc::TCPListener::value_type>& data)
    {
      mInbound.send(data);
    }
    
    Balancer(const float connw=0.7):mConnectionWeight(connw),mMayRun{true},mWorkersCache(0)
    {
      itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->getWorkers(mWorkersCache);
    }
    void shutdown()
    {
      mMayRun.store(false);
    }
    void onCancel()
    {
      shutdown();
    }
    ~Balancer()
    {
      this->shutdown();
    }
    void execute()
    {
      while(mMayRun)
      {
        try {
          auto inbound_connection=mInbound.recv();

          if(mWorkersCache.size()!=itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->size())
            itc::Singleton<WSWorkersPool<TLSEnable,StatsEnable>>::getInstance()->getWorkers(mWorkersCache);

          if(mWorkersCache.size()>0)
          {
            size_t chosen=0;
            auto stats=mWorkersCache[0]->getStats();
            for(size_t i=1;i<mWorkersCache.size();++i)
            {
              auto stats2=mWorkersCache[i]->getStats();
              if(stats.mConnections>stats2.mConnections) // candidate i
              {
                chosen=i;
                if(stats.mEventQSize > stats2.mEventQSize)
                {
                  stats=stats2;
                  chosen=i;
                }
              }
              else
              {
                if(stats.mEventQSize > stats2.mConnections*mConnectionWeight)
                {
                  stats=stats2;
                  chosen=i;
                }
              }
            }
            mWorkersCache[chosen]->update(inbound_connection);
          }else
          {
            mMayRun.store(false);
          }

        }catch (const std::exception& e)
        {
          mMayRun.store(false);
        }
      }
    }
  };
}
#endif /* __BALANCER_H__ */

