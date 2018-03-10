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

/**
 * @brief WS handshake
 **/
template <bool TLSEnable=false, bool StatsEnable=false> class Shaker
{
public:
  typedef std::shared_ptr<InSockQueuesRegistry<TLSEnable,StatsEnable>> ISQRegistry;
  typedef WebSocket<TLSEnable,StatsEnable> WSType;
  typedef std::shared_ptr<WSType>          WSSPtr;
  
  Shaker(const ISQRegistry& isqr)
  : mISQR(isqr), mHTTPRParser(), headerBuffer(8192,0),
    enableTLS(),enableStatsUpdate()
  {
    std::cout << "Shaker tls enabled: " << TLSEnable << std::endl;
  }
  
  const bool shakeTheHand(const itc::CSocketSPtr& inbound)
  {
    WSSPtr current=mkWebSocket(inbound,enableTLS);
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
       try{
         auto queue=mISQR->find(mHTTPRParser.getRequestTarget());
         prepareOKResponse(response);
         int sent=current->send(response);

         if(sent != static_cast<int>(response.size()))
         {
           itc::getLog()->debug(
            __FILE__,
            __LINE__,
            "Removing connection on communication errors"
           );
           return false;
         }
         else
         {
           current->setState(WSType::MESSAGING);
           queue->send(current);
           return true;
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
         return false;
       }
      }else{
       static const std::string  forbidden("HTTP/1.1 403 Forbidden");
       current->send(forbidden);
       // ignore the send result.
       return false;
      }
    }
    return false;
  }
private:
  ISQRegistry               mISQR;
  HTTPRequestParser         mHTTPRParser;
  CryptoPP::SHA1            sha1;
  std::vector<uint8_t>      headerBuffer;
  itc::utils::Bool2Type<TLSEnable> enableTLS;
  itc::utils::Bool2Type<StatsEnable> enableStatsUpdate;
  
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

