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
#include <IOWorker.h>
#include <WSWorkersPool.h>
#include <atomic>
#include <memory>
#include <abstract/Runnable.h>
#include <ext/tsl/robin_map.h>
#include <InQueue.h>

namespace LAppS
{
  template <bool TLSEnable=true, bool StatsEnable=true> class Balancer 
  : public ::itc::TCPListener::ViewType
  {
  private:
    using WorkersPool=itc::Singleton<LAppS::WSWorkersPool<TLSEnable,StatsEnable>>;
    
    float                                             mConnectionWeight;

    const size_t selectWorker(const tsl::robin_map<size_t,IOStats>& current_stats)
    {
      if(current_stats.begin()!=current_stats.end())
      {
        auto it=current_stats.begin();
        size_t choosen=it.key();
        IOStats stats_detail=it.value();

        std::for_each(
          current_stats.begin(),current_stats.end(),
          [this,&stats_detail,&choosen](const auto& kv)
          {
            // no connections limit check, as it is the workers job to announce 403 Forbidden to the client
            size_t m1=(kv.second.mConnections+kv.second.mInQueueDepth)*mConnectionWeight+kv.second.mEventQSize;
            size_t m2=(stats_detail.mConnections+stats_detail.mInQueueDepth)*mConnectionWeight+stats_detail.mEventQSize;
            if((m1)<(m2))
            {
              choosen=kv.first;
              stats_detail=kv.second;
            }
          }
        );
        return choosen;
      }
      throw std::system_error(EINVAL,std::system_category(),"No workers are available");
    }
  public:
   
    void onUpdate(const ::itc::TCPListener::value_type& data)
    {
      //tsl::robin_map<size_t,IOStats> current_stats;
      //LAppS::WStats::getInstance()->getStats(current_stats);
      try{
        // size_t choosen=selectWorker(current_stats);
        auto worker=WorkersPool::getInstance()->next();
        worker->enqueue(data);
      }catch(const std::exception& e)
      {
        // ignore
      }
    }

    void onUpdate(const std::vector<::itc::TCPListener::value_type>& data)
    {
      std::for_each(
        data.begin(),data.end(),
        [this](const auto& value)
        {
          this->onUpdate(value);
        }
      );
    }
    
    explicit Balancer(const float connw=0.7):mConnectionWeight(connw)
    {
    }
    ~Balancer()=default;
  };
}
#endif /* __BALANCER_H__ */

