/*
 * Copyright (C) 2018 Pavel Kraynyukhov <pk@new-web.co>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * $Id: Reader.h Created on June 9, 2018, 10:12 AM $
 */


#ifndef READER_H
#define READER_H
#include <memory>
#include <atomic>

#include <tsbqueue.h>
#include <abstract/Runnable.h>
#include <WebSocket.h>

namespace LAppS
{
  template <bool TLSEnable=true, bool StatsEnable=true> 
  class Reader : public itc::abstract::IRunnable
  {
  public:
    typedef WebSocket<TLSEnable,StatsEnable> WSType;
    typedef std::shared_ptr<WSType> WSSPtrType;
  private:
    std::atomic<bool>               mMayRun;
    std::atomic<bool>               mMayStop;
    
    itc::tsbqueue<WSSPtrType>       mReadQueue;
    
  public:
    explicit Reader() : mMayRun(true),mMayStop(false){}
    Reader(const Reader&)=delete;
    Reader(Reader&)=delete;
    
    void enqueue(const WSSPtrType& ref)
    {
      mReadQueue.send(ref);
    }
    const size_t getQSize() const
    {
      return mReadQueue.size();
    }
    void enqueue(const std::vector<WSSPtrType>& ref)
    {
      mReadQueue.send(std::move(ref));
    }
    void execute()
    {
      sigset_t sigset;
      sigemptyset(&sigset);
      sigaddset(&sigset, SIGQUIT);
      sigaddset(&sigset, SIGINT);
      sigaddset(&sigset, SIGTERM);
      sigaddset(&sigset, SIGPIPE);
      if(pthread_sigmask(SIG_BLOCK, &sigset, NULL) != 0)
        throw std::system_error(errno,std::system_category(),"Can not mask signals [SIGQUIT,SIGINT,SIGTERM,SIGPIPE");
      while(mMayRun)
      {
        try{
          WSSPtrType ws(mReadQueue.recv());
          if(ws->getState() == WSType::State::MESSAGING)
          {
            int ret=ws->handleInput();
            if(ret == -1)
            {
              itc::getLog()->info(__FILE__,__LINE__,"Disconnected: %s",ws->getPeerAddress().c_str());
            }
          }
        }catch(const std::exception& e)
        {
          itc::getLog()->info(__FILE__,__LINE__,"Reader is going down. Reason: event queue is going down.");
          mMayRun.store(false); 
        }
      }
      mMayStop.store(true);
    }
    
    void shutdown()
    {
      mMayRun.store(false);
    }
    
    void onCancel()
    {
      this->shutdown();
    }
    ~Reader()
    {
      this->shutdown();
    }
  };
}

#endif /* READER_H */

