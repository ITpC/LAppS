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

namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false> class WSWorkersPool
  {
   private:
    mutable itc::sys::mutex           mMutex;
    std::vector<WorkerThreadSPtr>     mWorkers;
    size_t                            mCursor;

   public:
    typedef LAppS::IOWorker<TLSEnable,StatsEnable> WorkerType;

    WSWorkersPool():mMutex(),mWorkers(),mCursor(0)
    {
    }
    WSWorkersPool(const WSWorkersPool&)=delete;
    WSWorkersPool(WSWorkersPool&)=delete;

    void spawn(const size_t maxC, const bool auto_fragment)
    {
      ITCSyncLock sync(mMutex);
      auto worker=std::make_shared<WorkerType>(mWorkers.size(),maxC, auto_fragment);
      mWorkers.push_back(
        std::make_shared<WorkerThread>(std::move(worker))
      );
    }
    auto next()
    {
      ITCSyncLock sync(mMutex);
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

    const size_t size() const
    {
      return mWorkers.size();
    }

    auto get(const size_t& wid)
    {
      ITCSyncLock sync(mMutex);

      if(wid<mWorkers.size())
      {
        return mWorkers[wid];
      }
      throw std::system_error(EINVAL,std::system_category(), "There is no worker with ID "+std::to_string(wid));
    }
    
    void  getWorkers(std::vector<std::shared_ptr<::abstract::Worker>>& out) const
    {
      ITCSyncLock sync(mMutex);
      for(auto i: mWorkers)
      {
        out.push_back(i->getRunnable());
      }
    }
    void clear()
    {
      ITCSyncLock sync(mMutex);
      mWorkers.clear();
    }

    ~WSWorkersPool()
    {
      clear();
    }
  };
}



#endif /* __WSWORKERSPOOL_H__ */

