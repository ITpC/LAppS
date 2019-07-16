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
 *  $Id: Shakespeer.h April 12, 2018 9:32 AM $
 * 
 **/


#ifndef __SHAKESPEER_H__
#  define __SHAKESPEER_H__

#include <memory>

#include <HTTPRequestParser.h>

// crypto++
#include <cryptopp/sha.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>


#include <WebSocket.h>

#include <wolfSSLLib.h>

#include <Val2Type.h>
#include <ePoll.h>
#include <Config.h>
#include <missing.h>

#include <ContextTypes.h>
#include <ServiceRegistry.h>

#include <modules/nljson.h>

static const std::vector<uint8_t> forbidden{'H','T','T','P','/','1','.','1',' ','4','0','3',' ','F','o','r','b','i','d','d','e','n'};

namespace LAppS
{
  template <bool TLSEnable=false, bool StatsEnable=false> class Shakespeer
  {
    public:
      typedef WebSocket<TLSEnable,StatsEnable> WSType;
      typedef std::shared_ptr<WSType>          WSSPtr;
      
      explicit Shakespeer()
      : headerBuffer(512)
      {
      }
      
      Shakespeer(const Shakespeer&)=delete;
      Shakespeer(Shakespeer&)=delete;
      
      void sendForbidden(const WSSPtr& wssocket)
      {
        wssocket->send(forbidden);
        wssocket->close();
      }
      
      void handshake(const WSSPtr& wssocket,const ServiceRegistry& anAppRegistry)
      {
        mHTTPRParser.clear();
        
        if(!wssocket->is_accepted())
        {
          if(!wssocket->accept())
          {
            wssocket->close();
            return;
          }
        }
        
        int received=wssocket->recv(headerBuffer);
        if(received != -1)
        {
          if(static_cast<size_t>(received) < headerBuffer.size())
            headerBuffer[received]=0;
          
          if(parseHeader(received))
          {
            auto app=anAppRegistry.findByTarget(mHTTPRParser.getRequestTarget());
            
            prepareOKResponse(response,app->getName(),app->getProtocol());
            
            int sent=wssocket->send(response);
            
            try {
              if(sent > 0)
              {
                if(app->filterIP(wssocket->getPeerIP()))
                {
                  itc::getLog()->info(
                    __FILE__,
                    __LINE__,
                    "Connection from %s to %s has been filtered according to ACL",
                    wssocket->getPeerAddress().c_str(),app->getName().c_str()
                  );
                  wssocket->send(forbidden);
                  wssocket->close();
                  return;
                }
                wssocket->setState(WSType::MESSAGING);
                wssocket->setApplication(app);
                return;
              }
              else
              {
                itc::getLog()->error(
                  __FILE__,__LINE__,
                  "Can't send OK response for the protocol upgrade after handshake to peer %s. Closing this WebSocket",
                  wssocket->getPeerAddress().c_str()
                );

                wssocket->close();
                return;   
              }
            }catch(const std::exception& e)
            {
              itc::getLog()->error(
                __FILE__,__LINE__,
                "Exception on handshake with peer %s: %s",
                wssocket->getPeerAddress().c_str(),e.what()
              );
              wssocket->close();
              return;
            }
          }else{
            itc::getLog()->error(
              __FILE__,__LINE__,
              "Shakespeer::handshake() was unsuccessful for peer %s. Received %u bytes. Header Content: %s.",
              wssocket->getPeerAddress().c_str(), received, headerBuffer.data()
            );
            wssocket->send(forbidden);
            wssocket->close();
            return;
          }
        }
        else
        {
          wssocket->close();
          itc::getLog()->info(
            __FILE__,__LINE__,
            "Communication error on handshake with peer on fd %d. Closing this WebSocket",
            wssocket->getfd()
          );
        }
      }
    
    private:
      itc::utils::Bool2Type<TLSEnable>              enableTLS;
      itc::utils::Bool2Type<StatsEnable>            enableStatsUpdate;
      
      HTTPRequestParser                             mHTTPRParser;
      CryptoPP::SHA1                                sha1;
      std::vector<uint8_t>                          headerBuffer;
      std::vector<uint8_t>                          response;
      
      const bool parseHeader(const size_t bufflen)
      {
       try
       {
          mHTTPRParser.parse(headerBuffer,bufflen);
          bool arhap=true;
          
          auto connection_value=itc::utils::toupper(mHTTPRParser["Connection"]);
          arhap=arhap&&(connection_value.find("UPGRADE") != std::string::npos);
          arhap=arhap&&(itc::utils::toupper(mHTTPRParser["Upgrade"])=="WEBSOCKET");
          arhap=arhap&&(mHTTPRParser["Sec-WebSocket-Version"]=="13");
          arhap=arhap&&(!(mHTTPRParser["Sec-WebSocket-Key"].empty()));

          
          if(arhap)
            return true;
          else
          {
            return false;
          }
       }catch(const std::exception& e)
       {
         itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an exception %s, in %s()",this, e.what(),__FUNCTION__);
         return false;
       }
      }
      void prepareOKResponse(std::vector<uint8_t>& response, const std::string& app_name, const LAppS::ServiceProtocol& proto)
      {
        static const std::string  UID("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
        std::string  okResponse("HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nServer: LAppS/0.7.0\r\n");
        
        auto it=LAppSConfig::getInstance()->getLAppSConfig()["services"][app_name].find("extra_headers");
        if(it!=LAppSConfig::getInstance()->getLAppSConfig()["services"][app_name].end())
        {
          auto extra_headers_it=it.value().begin();
          while(extra_headers_it != it.value().end())
          {
            std::string key=extra_headers_it.key();
            std::string value=extra_headers_it.value();

            okResponse.append(key);
            okResponse.append(": ");
            okResponse.append(value);
            okResponse.append("\r\n");
            
            ++extra_headers_it;
          }
        }

        std::string allow_protocols("Sec-WebSocket-Protocol: ");
        
        switch(proto)
        {
          case ::LAppS::ServiceProtocol::RAW:
            allow_protocols.append("raw");
            break;
          case ::LAppS::ServiceProtocol::LAPPS:
            allow_protocols.append("LAppS");
            break;
          default:
            itc::getLog()->error(__FILE__,__LINE__,"Shakespeer::prepareOKResponse(proto), - proto is invalid. Must be one or two: RAW, LAPPS");
            break;
        }

        it=LAppSConfig::getInstance()->getLAppSConfig()["services"][app_name].find("proto_alias");

        if(it!=LAppSConfig::getInstance()->getLAppSConfig()["services"][app_name].end())
        {
          allow_protocols="Sec-WebSocket-Protocol: ";
          std::string proto_alias=it.value();
          allow_protocols.append(proto_alias);
          allow_protocols.append("\r\n");
          okResponse.append(allow_protocols);
        }
        
        okResponse.append("Sec-WebSocket-Accept: ");
        
        std::string replykey(mHTTPRParser["Sec-WebSocket-Key"]+UID);
        
        CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
        sha1.CalculateDigest( digest, (const CryptoPP::byte*)(replykey.c_str()),replykey.length());

        CryptoPP::Base64Encoder b64;
        b64.Put(digest, CryptoPP::SHA1::DIGESTSIZE);
        b64.MessageEnd();

        size_t encoded_key_size=b64.MaxRetrievable();
        size_t respsz=okResponse.size();
        
        okResponse.resize(respsz+encoded_key_size-1);

        b64.Get((CryptoPP::byte*)(okResponse.data()+respsz),encoded_key_size-1);

        okResponse.append("\r\n\r\n");
        
        response.resize(okResponse.size());

        memcpy(response.data(),okResponse.data(),okResponse.size());
      }
  };
}

#endif /* __SHAKESPEER_H__ */

