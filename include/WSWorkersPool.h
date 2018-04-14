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
 *  $Id: WSWorkersPool.h April 7, 2018 4:33 AM $
 * 
 **/


#ifndef __WSWORKERSPOOL_H__
#  define __WSWORKERSPOOL_H__

#include <memory>
#include <mutex>
#include <sys/synclock.h>
#include <IOWorker.h>
#include <ePollController.h>

template <bool TLSEnable=false, bool StatsEnable=false> class WSWorkersPool
{
 private:
  std::mutex                    mMutex;
  std::vector<WorkerThreadSPtr> mWorkers;
  size_t                        mCursor;
 public:
  typedef LAppS::IOWorker<TLSEnable,StatsEnable>             WorkerType;
  
  WSWorkersPool():mMutex(),mWorkers(),mCursor(0)
  {
  }
  WSWorkersPool(const WSWorkersPool&)=delete;
  WSWorkersPool(WSWorkersPool&)=delete;
  
  void spawn(const size_t maxC)
  {
    SyncLock sync(mMutex);
    auto worker=std::make_shared<WorkerType>(mWorkers.size(),maxC);
    mWorkers.push_back(
      std::make_shared<WorkerThread>(worker)
    );
    worker->getEPollController()->setView(worker);
  }
  auto next()
  {
    SyncLock sync(mMutex);
    if(mWorkers.size() == 0)
      throw std::logic_error("WSWorkersPool::next() - the pool is empty, there is no next worker");
    
    if(mCursor<mWorkers.size())
    {
      auto ret=mWorkers[mCursor]->getRunnable();
      ++mCursor;
      return ret;
    }else {
      mCursor=0;
      auto ret=mWorkers[mCursor]->getRunnable();
      ++mCursor;
      return ret;
    }
  }
  const WorkerStats getStats()
  {
    WorkerStats mAllStats;
    
    for(auto it=mWorkers.begin();it!=mWorkers.end();++it)
    {
      auto stats=it->get()->getRunnable()->getStats();
      mAllStats.mConnections+=stats.mConnections;
      mAllStats.mInMessageCount+=stats.mInMessageCount;
      mAllStats.mOutMessageCount+=stats.mOutMessageCount;
      
      
      if(mAllStats.mInMessageMaxSize<stats.mInMessageMaxSize)
      {
        mAllStats.mInMessageMaxSize=stats.mInMessageMaxSize;
      }
      if(mAllStats.mOutMessageMaxSize<stats.mOutMessageMaxSize)
      {
        mAllStats.mOutMessageMaxSize=stats.mOutMessageMaxSize;
      }
      mAllStats.mOutCMASize+=stats.mOutCMASize;
      mAllStats.mInCMASize+=stats.mInCMASize;
      
      
    }
    mAllStats.mOutCMASize/=mWorkers.size();
    mAllStats.mInCMASize/=mWorkers.size();
    return mAllStats;
  }
  auto get(const size_t& wid)
  {
    SyncLock sync(mMutex);
    
    if(wid<mWorkers.size())
    {
      return mWorkers[wid];
    }
    throw std::system_error(EINVAL,std::system_category(), "There is no worker with ID "+std::to_string(wid));
  }
  ~WSWorkersPool()
  {
    SyncLock sync(mMutex);
    mWorkers.clear();
  }
};


#endif /* __WSWORKERSPOOL_H__ */
