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
 *  $Id: WorkerStats.h March 5, 2018 4:06 PM $
 * 
 **/


#ifndef __WORKERSTATS_H__
#  define __WORKERSTATS_H__

#include <atomic>
#include <ext/tsl/robin_map.h>


namespace LAppS
{
  /**
   * @brief statistics of every worker.
   **/
  struct WorkerMinStats
  {
    size_t mConnections;
    size_t mEventQSize;
  };

  struct IOStats : public WorkerMinStats
  {
    size_t mInMessageCount;
    size_t mOutMessageCount;

    size_t mInMessageMaxSize;
    size_t mOutMessageMaxSize;

    size_t mInCMASize;
    size_t mOutCMASize;
    size_t mInQueueDepth;

    const IOStats& operator=(const IOStats& value)
    {
      mConnections=value.mConnections;
      mEventQSize=value.mEventQSize;
      mInMessageCount=value.mInMessageCount;
      mOutMessageCount=value.mOutMessageCount;
      mInMessageMaxSize=value.mInMessageMaxSize;
      mOutMessageMaxSize=value.mOutMessageMaxSize;
      mInCMASize=value.mInCMASize;
      mOutCMASize=value.mOutCMASize;
      mInQueueDepth=value.mInQueueDepth;
      return *this;
    }
  };

  class WorkersStats 
  {
  private:
    std::atomic<bool> can_update;
    std::atomic<size_t> updaters;
    tsl::robin_map<size_t,IOStats> stats;
    
  public:
    
    WorkersStats(WorkersStats&)=delete;
    WorkersStats(const WorkersStats&)=delete;

    explicit WorkersStats()
    :can_update{false},updaters{0}
    {
      can_update.store(true);
      updaters.store(0);
    }

    const bool try_update(const size_t wid, IOStats& _stats)
    {
      bool expected=true;

      updaters.fetch_add(1);
      if(can_update.compare_exchange_strong(expected,true))
      {
        auto it=stats.find(wid);

        if(it!=stats.end())
        {
          it.value()=_stats;
          updaters.fetch_sub(1);
          return true;
        }
      }
      updaters.fetch_sub(1);
      return false;
    }

    void add_slot(const size_t& wid)
    {
      bool expected=true;

      while(!can_update.compare_exchange_strong(expected,false))
      {
        expected=true;
      }
      size_t cexpected=0;

      while(!updaters.compare_exchange_strong(cexpected,0))
      {
        cexpected=0;
      }

      stats.emplace(wid,IOStats());
      can_update.store(true);
    }

    void del_slot(const size_t wid)
    {
      bool expected=true;

      while(!can_update.compare_exchange_strong(expected,false))
      {
        expected=true;
      }
      size_t cexpected=0;

      while(!updaters.compare_exchange_strong(cexpected,0))
      {
        cexpected=0;
      }

      auto it=stats.find(wid);
      if(it!=stats.end()) stats.erase(it);
      can_update.store(true);
    }

    void getStats(tsl::robin_map<size_t,IOStats>& statmap)
    {
      bool expected=true;

      while(!can_update.compare_exchange_strong(expected,false))
      {
        expected=true;
      }
      for(auto& item : stats)
      {
        statmap.emplace(item);
      }
      can_update.store(true);
    }
  }; 
  typedef itc::Singleton<WorkersStats> WStats;
}

#endif /* __WORKERSTATS_H__ */
