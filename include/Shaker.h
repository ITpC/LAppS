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
 *  $Id: Shaker.h March 10, 2018 4:14 AM $
 * 
 **/


#ifndef __SHAKER_H__
#  define __SHAKER_H__

#include <memory>

#include <HTTPRequestParser.h>

// crypto++
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>


#include <WebSocket.h>
#include <InSockQueuesRegistry.h>
#include <TLSServerContext.h>
#include <Val2Type.h>
#include <ePoll.h>
#include <Config.h>
#include <ApplicationRegistry.h>

/**
 * @brief WS handshake
 **/
template <bool TLSEnable=false, bool StatsEnable=false> 
class Shaker : public itc::abstract::IRunnable
{
public:
  typedef std::shared_ptr<InSockQueuesRegistry<TLSEnable,StatsEnable>> ISQRegistry;
  typedef WebSocket<TLSEnable,StatsEnable> WSType;
  typedef std::shared_ptr<WSType>          WSSPtr;
  
  Shaker(const ISQRegistry& isqr,const itc::CSocketSPtr& socksptr)
  : enableTLS(),enableStatsUpdate(),mSocketSPtr(socksptr),nlt(5000),
    mISQR(isqr), mHTTPRParser(), headerBuffer(1024,0)
  {
  }
  void onCancel()
  {
    
  }
  void shutdown()
  {
    
  }
  void execute()
  {
    // TODO: add a config view with fast access and without json.
    nlt=LAppSConfig::getInstance()->getWSConfig()["network_latency_tolerance"];
    std::string peer_addr;
    mSocketSPtr->getpeeraddr(peer_addr);
    
    // prevent connections exhaustion DDoS
    // the handshake must be available immediately after connection is 
    // established. 
    // TODO: need to add a network latency tolerance into configuration. 
    // for some connections with high latency this might be a an overkill 
    try
    {
      mPoll.add(mSocketSPtr->getfd());
      int ret=mPoll.poll(events,nlt);
      
      if(ret == 0) // timed out, no IO within required threshold.
      {
        itc::getLog()->info(
          __FILE__,__LINE__,
          "No handshake request was sent from the peer %s within required threshold. Disconnecting and removing connection",
          peer_addr.c_str()
        );
        return;
      }
        
      if(error_bits(events[0].data.fd))
      {
        itc::getLog()->info(
          __FILE__,__LINE__,
          "Communication error before handshake on socket %d from peer %s",
          mSocketSPtr->getfd(),peer_addr.c_str()
        );
        return;
      }
    }
    catch(const std::system_error& e)
    {
      itc::getLog()->error(
        __FILE__,__LINE__,
        "Exception on connection polling before handshake: %s",
        e.what()
      );
      return;
    }
    
    WSSPtr current=mkWebSocket(mSocketSPtr,enableTLS);
    std::vector<uint8_t> response;
    
    mHTTPRParser.clear();
    
    headerBuffer.resize(8192);
    int received=current->recv(headerBuffer,MSG_NOSIGNAL);
    if(received != -1)
    {
      headerBuffer.resize(received+1);
      headerBuffer[received]=0;

      if(parseHeader())
      {
       try
       {
          auto queue=mISQR->find(mHTTPRParser.getRequestTarget());
          current->setApplication(
            ApplicationRegistry::getInstance(
            )->findByTarget(mHTTPRParser.getRequestTarget())
          );
          
          try
          {
            prepareOKResponse(response);
            int sent=current->send(response);

            if(sent != static_cast<int>(response.size()))
            {
              std::string pipaddr;
              current->getPeerIP(pipaddr);
              itc::getLog()->error(
               __FILE__,
               __LINE__,
               "Communication error on connection from peer [%s]. Removing connection",
                pipaddr.c_str()
              );
              return;
            }
            queue->send(current);
          }
          catch (const std::exception& e)
          {
            std::string pipaddr;
            current->getPeerIP(pipaddr);
            itc::getLog()->error(
              __FILE__,__LINE__,
              "Runtime error [%s]: %s",pipaddr.c_str(),e.what()
            );
          }
       }catch(const std::system_error& e)
       {
         std::string pipaddr;
         current->getPeerIP(pipaddr);

         itc::getLog()->error(
          __FILE__,__LINE__,
          "No such queue or target: %s, closing connection from %s",
          mHTTPRParser.getRequestTarget().c_str(),pipaddr.c_str()
         );
         return;
       }
      }else{
       static const std::string  forbidden("HTTP/1.1 403 Forbidden");
       current->send(forbidden);
       // ignore the send result.
       return;
      }
    }
    return;
  }
private:
  itc::utils::Bool2Type<TLSEnable> enableTLS;
  itc::utils::Bool2Type<StatsEnable> enableStatsUpdate;
  itc::CSocketSPtr          mSocketSPtr;
  int                       nlt;
  ePoll<>                   mPoll;
  ISQRegistry               mISQR;
  HTTPRequestParser         mHTTPRParser;
  CryptoPP::SHA1            sha1;
  std::vector<epoll_event>  events;
  std::vector<uint8_t>      headerBuffer;
  
  const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<false> tls_is_disabled)
  {
    return std::make_shared<WSType>(inbound);
  }
  const std::shared_ptr<WSType> mkWebSocket(const itc::CSocketSPtr& inbound,const itc::utils::Bool2Type<true> tls_is_enabled)
  {
    auto tls_server_context=TLS::SharedServerContext::getInstance()->getContext();
    if(tls_server_context)
      return std::make_shared<WSType>(inbound,tls_server_context);
    else throw std::system_error(EINVAL,std::system_category(),"TLS ServerContext is NULL");
  }
  void prepareOKResponse(std::vector<uint8_t>& response)
  {
    static const std::string  UID("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    static const std::string  okResponse("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
    response.resize(okResponse.length());

    memcpy(response.data(),okResponse.data(),okResponse.length());

    std::string replykey(mHTTPRParser["Sec-WebSocket-Key"]+UID);

    byte digest[CryptoPP::SHA1::DIGESTSIZE];
    sha1.CalculateDigest( digest, (const byte*)(replykey.c_str()),replykey.length());

    CryptoPP::Base64Encoder b64;
    b64.Put(digest, CryptoPP::SHA1::DIGESTSIZE);
    b64.MessageEnd();

    size_t response_size=b64.MaxRetrievable();
    response.resize(response.size()+response_size);
    b64.Get(response.data()+okResponse.length(),response_size);

    response.resize(response.size()+3);
    response[response.size()-4]=13;
    response[response.size()-3]=10;
    response[response.size()-2]=13;
    response[response.size()-1]=10;
  }
  bool error_bits(const uint32_t events)
  {
   return (events & (EPOLLRDHUP|EPOLLERR|EPOLLHUP));
  }

  bool in_bit(const uint32_t events)
  {
   return (events & EPOLLIN);
  }
  const bool parseHeader()
  {
   try
   {
      mHTTPRParser.parse(headerBuffer);
      return true;
   }catch(const std::exception& e)
   {
     itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an exception %s, in %s()",this, e.what(),__FUNCTION__);
     return false;
   }
  }
};

#endif /* __SHAKER_H__ */

