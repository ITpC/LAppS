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
#include <TLSServerContext.h>
#include <Val2Type.h>
#include <ePoll.h>
#include <Config.h>
#include <ApplicationRegistry.h>

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
      
      void handshake(const WSSPtr& wssocket,LAppS::ApplicationRegistry& anAppRegistry)
      {
        mHTTPRParser.clear();
        int received=wssocket->recv(headerBuffer);
        if(received != -1)
        {
          if(static_cast<size_t>(received) < headerBuffer.size())
            headerBuffer[received]=0;

          if(parseHeader(received))
          {
            prepareOKResponse(response);
            int sent=wssocket->send(response);
            try {
              if(sent > 0)
              {
                auto app=anAppRegistry.getByTarget(mHTTPRParser.getRequestTarget());
                
                if(app->filter(wssocket->getPeerIP()))
                {
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
              "Shakespeer::handshake() was unsuccessful for peer %s. Closing this WebSocket.",
              wssocket->getPeerAddress().c_str()
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
          return true;
       }catch(const std::exception& e)
       {
         itc::getLog()->error(__FILE__,__LINE__,"Worker %p: an exception %s, in %s()",this, e.what(),__FUNCTION__);
         return false;
       }
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
  };
}

#endif /* __SHAKESPEER_H__ */

