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
 *  $Id: Worker.h January 7, 2018 5:27 AM $
 * 
 **/


#ifndef __WORKER_H__
#  define __WORKER_H__

// c headers
#include <stdio.h>

// remove when done with debugging
#include <iostream>


// c++ headers
#include <map>
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>

// ITC headers
#include <sys/synclock.h>
#include <sys/Nanosleep.h>
#include <TCPListener.h>

// Local headers
#include <ePoll.h>
#include <WebSocket.h>
#include <HTTPRequestParser.h>
#include <InboundConnectionsPool.h>

// crypto++
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <set>

#include <tls.h>


/**
 * \@brief To be used as a CancelableThread only
 **/
template <bool TLSEnable=false> class ConnectionsWorker : public itc::abstract::IRunnable
{
private:
 ePoll<> mEPoll;
 std::atomic<bool> doRun;
 std::atomic<bool> canStop;
 std::map<int,std::shared_ptr<WebSocket<TLSEnable>>> mConnections;
 WeakInboundConnectionsSPtr mInbound;
 std::vector<epoll_event> events;
 size_t mMaxConnections;
 std::vector<uint8_t> header;
 itc::sys::Nap mSleep;
 CryptoPP::SHA1 sha1;
 std::vector<uint8_t> inbuffer;
 tls_config* TLSConfig;
 tls*        TLSContext;

public:
  ConnectionsWorker(const size_t maxConnections, const WeakInboundConnectionsSPtr& ic)
  : itc::abstract::IRunnable(), mEPoll(),doRun(true),canStop(false),
    mInbound(ic.lock()), events(maxConnections),mMaxConnections(maxConnections),
    inbuffer(8192)
  {
    bool tlsEnabled=LAppSConfig::getInstance()->getWSConfig()["tls"];
    if(tlsEnabled)
    {
      if(tls_init())
      {
        throw std::system_error(errno,std::system_category(),"TLS: can not initialize. This worker will not start");
      }
      
      if(!(TLSConfig=tls_config_new()))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can not configure. This worker will not start");
      }
      
      if(!(TLSContext=tls_server()))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can not create server context. This worker will not start");
      }
      
      uint32_t protocols=0;
      
      if(tls_config_parse_protocols(&protocols, "secure"))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can not parse 'secure' protocols. This worker will not start");
      }
      if(tls_config_set_protocols(TLSConfig, protocols))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can't set protocols. This worker will not start");
      }
      if(tls_config_set_ciphers(TLSConfig,"secure"))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can't set secure ciphers. This worker will not start");
      }
      if(tls_config_set_ecdhecurves(TLSConfig,"default"))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can't set default ecdhcurves. This worker will not start");
      }
      
      tls_config_prefer_ciphers_server(TLSConfig);
      
      
      std::string certificate_key_file=LAppSConfig::getInstance()->getWSConfig()["tls_server"]["key"];
      
      if(tls_config_set_key_file(TLSConfig,certificate_key_file.c_str()))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can't load certificate key file (PEM). This worker will not start");
      }
      
      std::string certificate_file=LAppSConfig::getInstance()->getWSConfig()["tls_server"]["cert"];
      
      if(tls_config_set_cert_file(TLSConfig,certificate_file.c_str()))
      {
        throw std::system_error(errno,std::system_category(),"TLS: can't load certificate file (PEM). This worker will not start");
      }
      
      if(tls_configure(TLSContext, TLSConfig))
      {
        throw std::system_error(errno,std::system_category(),"TLS: Context configuration has been failed (PEM). This worker will not start");
      }
      
    }
    header.resize(8192,0);
  }
  
private:
 bool parseHeader(std::vector<uint8_t>& response)
 {
   static const std::string UID("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
   try
   {
    HTTPRequestParser parse(header);
    static const std::string resp("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
    response.resize(resp.length());
    
    memcpy(response.data(),resp.data(),resp.length());
    
    std::string replykey(parse["Sec-WebSocket-Key"]+UID);
    
    byte digest[CryptoPP::SHA1::DIGESTSIZE];
    sha1.CalculateDigest( digest, (const byte*)(replykey.c_str()),replykey.length());
    
    CryptoPP::Base64Encoder b64;
    b64.Put(digest, CryptoPP::SHA1::DIGESTSIZE);
    b64.MessageEnd();
    
    size_t response_size=b64.MaxRetrievable();
    response.resize(response.size()+response_size);
    b64.Get(response.data()+resp.length(),response_size);
    
    response.resize(response.size()+3);
    response[response.size()-4]=13;
    response[response.size()-3]=10;
    response[response.size()-2]=13;
    response[response.size()-1]=10;
    
    std::string tmp;
    tmp.resize(response.size());
    memcpy((void*)(tmp.data()),response.data(),response.size());
   }catch(const std::exception& e)
   {
     itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an exception %s, in %s()",this, e.what(),__FUNCTION__);
     return false;
   }
   return true;
 }
 
 bool error_bits(const uint32_t events)
 {
   return (events & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
 }
 
 bool in_bit(const uint32_t events)
 {
   return (events & EPOLLIN);
 }
 
 void waitForInbound()
 {
   this->waitForInbound(itc::utils::Bool2Type<TLSEnable>());
 }
 
 void waitForInbound(const itc::utils::Bool2Type<false> fictive)
 {
  if(!doRun.load()) return;
  if(auto inbound=mInbound.lock())
  {
    itc::CSocketSPtr tmp;
    try{
      inbound->recv(tmp);
     
      if(tmp->isValid())
      {
        int sockfd=tmp.get()->getfd();
        mConnections.emplace(sockfd,std::make_shared<WebSocket<TLSEnable>>(tmp));
        mEPoll.add(sockfd);
      }
    }catch(const std::exception& e)
    {
      itc::getLog()->error(
        __FILE__,__LINE__,
        "Worker %p, in %s() an inbound connections queue is down [%s], exiting ...",
        this,__FUNCTION__,e.what()
      );
      doRun.store(false);
    }
  }
 }
 
 void waitForInbound(const itc::utils::Bool2Type<true> fictive)
 {
  if(!doRun.load()) return;
  if(auto inbound=mInbound.lock())
  {
    itc::CSocketSPtr tmp;
    try{
      inbound->recv(tmp);
     
      if(tmp->isValid())
      {
        int sockfd=tmp.get()->getfd();
        mConnections.emplace(
          sockfd,std::make_shared<WebSocket<TLSEnable>>(
            tmp,TLSContext
          )
        );
        mEPoll.add(sockfd);
      }
    }catch(const std::exception& e)
    {
      itc::getLog()->error(
        __FILE__,__LINE__,
        "Worker %p, in %s() an inbound connections queue is down [%s], exiting ...",
        this,__FUNCTION__,e.what()
      );
      doRun.store(false);
    }
  }
 }
 
 void collectInbound()
 {
   this->collectInbound(itc::utils::Bool2Type<TLSEnable>());
 }
 
 void collectInbound(const itc::utils::Bool2Type<false> fictive)
 {
   if(auto inbound=mInbound.lock())
   {
     try
     {
       if(!inbound->empty())
       {
         itc::CSocketSPtr tmp;

         inbound->recv(tmp);
         int sockfd=tmp.get()->getfd();
         mConnections.emplace(sockfd,std::make_shared<WebSocket<TLSEnable>>(tmp));
         mEPoll.add(sockfd);
       }
      }catch(const std::exception& e){
        itc::getLog()->error(
          __FILE__,__LINE__,
          "Worker %p, in %s() an inbound connections queue is down [%s], exiting ...",
          this,__FUNCTION__,e.what()
        );
        doRun.store(false);
      }
    }
 }
 void collectInbound(const itc::utils::Bool2Type<true> fictive)
 {
   if(auto inbound=mInbound.lock())
   {
     try
     {
       if(!inbound->empty())
       {
         itc::CSocketSPtr tmp;

         inbound->recv(tmp);
         int sockfd=tmp.get()->getfd();
         mConnections.emplace(
          sockfd,std::make_shared<WebSocket<TLSEnable>>(
            tmp,TLSContext
          )
         );
         mEPoll.add(sockfd);
       }
      }catch(const std::exception& e){
        itc::getLog()->error(
          __FILE__,__LINE__,
          "Worker %p, in %s() an inbound connections queue is down [%s], exiting ...",
          this,__FUNCTION__,e.what()
        );
        doRun.store(false);
      }
    }
 }
 
 int pollForEvents()
 {
    events.clear();
    // poll the connections for events
    int ret=0;
    try {     
      events.resize(mConnections.size());
      ret=mEPoll.poll(events);
      return ret;
    }catch(const std::exception& e)
    {
      itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an exception %s, in %s()",this, e.what(),__FUNCTION__);
      doRun.store(false);
      return 0;
    }   
 }
 
 void onHandShake(std::shared_ptr<WebSocket<TLSEnable>>& current)
 {
  bool remove_this_connection=false;
  header.resize(8192);
  int received=current->recv(header,0);
  if(received != -1)
  {
    header.resize(received+1);
    header[received]=0;
    std::vector<uint8_t> response;
    if(parseHeader(response))
    {
      int sent=current->send(response);

      if(sent != static_cast<int>(response.size()))
      {
        remove_this_connection=true;
      }
      else
      {
        current->setState(WebSocket<TLSEnable>::MESSAGING);
      }
    }
    else remove_this_connection=true;
  }else{
    if(errno != EAGAIN)
      remove_this_connection=true;
  }

  if(remove_this_connection)
  {
    itc::getLog()->debug(
      __FILE__,
      __LINE__,
      "Connection was removed due to comm errors"
    );
    std::string forbidden("HTTP/1.1 403 Forbidden");
    current->send(forbidden);
    current->setState(WebSocket<TLSEnable>::CLOSED);
  }
}
 
 void onMessaging(std::shared_ptr<WebSocket<TLSEnable>>& current)
 {
  // inbuffer.resize(130);
  int received=current->recv(inbuffer,0);
  switch(received)
  {
    case -1:
    {
      switch(errno)
      {
        case EAGAIN:

          break;
        default:
          itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an error %s, in %s()",this, strerror(errno),__FUNCTION__);
          break;
      }
    }
    break;
    case 0:
      break;
    default:
      // inbuffer.resize(received);
      current->processInput(inbuffer,received);
    break;
  }
}
 
public:
  
  void cleanClosed()
  {
    if(!mConnections.empty())
    {
      std::queue<int> toDelete;
      for(auto it=mConnections.begin();it!=mConnections.end();++it)
      {
        if(it->second->getState()==WebSocket<TLSEnable>::CLOSED)
        {
          toDelete.push(it->first);
        }
      }
      while(!toDelete.empty())
      {
        auto it=mConnections.find(toDelete.front());
        toDelete.pop();
        if(it!=mConnections.end())
        {
          mConnections.erase(it);
        }
      }
    }
  }
  void execute()
  {
    sigset_t oldset, newset;
    
    sigemptyset(&newset);
    sigaddset(&newset, SIGPIPE);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);
    while(doRun)
    {
      cleanClosed();
      size_t connections=mConnections.size();
      
      if(connections > 0 )
      {
        if(connections < mMaxConnections)
        {
          collectInbound(); 
        }
        int ret=pollForEvents();

        if(ret>0)
        {
          for(size_t i=0;(i<events.size())&&(i<static_cast<unsigned int>(ret));++i)
          {
            if(error_bits(events[i].events))
            {
              auto it=mConnections.find(events[i].data.fd);
              if(it!=mConnections.end())
              {
                it->second->setState(WebSocket<TLSEnable>::CLOSED);
                mConnections.erase(it);
              }
            }
            else if(in_bit(events[i].events)) 
            {
              std::shared_ptr<WebSocket<TLSEnable>> current(mConnections[events[i].data.fd]);

              switch(current->getState())
              {
                case WebSocket<TLSEnable>::HANDSHAKE:
                  onHandShake(current);
                  mEPoll.mod(events[i].data.fd);
                break;
                case WebSocket<TLSEnable>::MESSAGING: 
                  onMessaging(current);
                  if(current->getState()!=WebSocket<TLSEnable>::CLOSED)
                  {
                    mEPoll.mod(events[i].data.fd);
                  }
                break;
                case WebSocket<TLSEnable>::SSL_HANDSHAKE:
                  onHandShake(current);
                  mEPoll.mod(events[i].data.fd);
                break;
                default:
                  itc::getLog()->error(__FILE__,__LINE__,"Worker %p is not suppose to take this state.",this);
                break;
              }
            }
          }
        }
      } 
      else {
        waitForInbound();
        if(doRun.load() == false)
          break;
      }
    }
    canStop.store(true);
    itc::getLog()->info("Worker %p is stopped.",this);
    pthread_sigmask(SIG_SETMASK, &oldset, 0);
  }
  void shutdown()
  {
    doRun.store(false);
    while(!canStop.load())
      sched_yield();
    
    mConnections.clear();
    
    if(TLSEnable)
    {
      // using these two commented calls, producing SIGABRT ...
      // tls_close(TLSContext);
      // tls_free(TLSContext);
      tls_config_free(TLSConfig);
    }
  }
  void onCancel()
  {
    this->shutdown();
  }
  ~ConnectionsWorker()
  {
    this->shutdown();
  }
};
typedef ConnectionsWorker<true> TLSWorker;
typedef ConnectionsWorker<false> nonTLSWorker;
typedef itc::sys::CancelableThread<TLSWorker> TLSWorkerThread;
typedef std::shared_ptr<TLSWorkerThread> TLSWorkerThreadSPtr;
typedef itc::sys::CancelableThread<nonTLSWorker> WorkerThread;
typedef std::shared_ptr<WorkerThread> WorkerThreadSPtr;


#endif /* __WORKER_H__ */

