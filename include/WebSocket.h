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
 *  $Id: WebSocket.h January 17, 2018 11:56 PM $
 * 
 **/


#ifndef __WEBSOCKET_H__
#  define __WEBSOCKET_H__



#include <map>
#include <vector>
#include <string>

#include <net/NSocket.h>
#include <TCPListener.h>
#include <Val2Type.h>

#include <WSStreamProcessor.h>
#include <WSServerMessage.h>

#include <Config.h>

// LibreSSL
#include <tls.h>

template <bool TLSEnable=false> class WebSocket
{
 public:
  enum State { HANDSHAKE, SSL_HANDSHAKE, MESSAGING, CLOSED };
 private:
  State mState;
  itc::CSocketSPtr mSocketSPtr;
  std::map<std::string,std::string> mHTTPHeaders;
  WSStreamParser streamProcessor;
  std::vector<uint8_t> outBuffer;
  
  struct tls* TLSContext;
  struct tls* TLSSocket;
  
  void init(const itc::utils::Bool2Type<true> tls_is_enabled)
  {
    if(tls_accept_socket(TLSContext,&TLSSocket,mSocketSPtr->getfd()))
    {
      throw std::system_error(errno,std::system_category(),"TLS: can't accept socket");
    }
    mState=SSL_HANDSHAKE;
  }
  
  void init(const itc::utils::Bool2Type<false> tls_is_not_enabled)
  {
    mState=HANDSHAKE;
  }
  
  void setState(const State state, itc::utils::Bool2Type<false> fictive)
  {
    if((state > mState)&&(state != SSL_HANDSHAKE))
      mState=state;
    else throw std::logic_error(
        "Connection::setState(), - new state is out of order"
    );
  }
  void setState(const State state, itc::utils::Bool2Type<true> fictive)
  {
    if((state > mState)&&(state != HANDSHAKE))
      mState=state;
    else throw std::logic_error(
        "Connection::setState(), - new state is out of order"
    );
  }
  int recv(std::vector<uint8_t>& buff, const int flags,const itc::utils::Bool2Type<false> fictive)
  {
    return mSocketSPtr.get()->recv((void*)(buff.data()),buff.size(),flags);
  }
  
  int recv(std::vector<uint8_t>& buff, const int flags,const itc::utils::Bool2Type<true> fictive)
  {
    int ret=0;
    
    do{
      ret=tls_read(TLSSocket,buff.data(),buff.size());
    
      if(ret == -1)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Error on reading from TLS socket: %s",tls_error(TLSSocket));
        break;
      }
    }while(ret == TLS_WANT_POLLIN || ret == TLS_WANT_POLLOUT);
    return ret;
  }
    
  int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<false> fictive)
  {
    return mSocketSPtr.get()->write(buff.data(),buff.size());
  }
  int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<true> fictive)
  {
    int ret=0;
    size_t offset=0;
    do
    {
      ret=tls_write(TLSSocket,buff.data()+offset,buff.size()-offset);
      
      if(ret == -1)
        break;
      
      if((ret != TLS_WANT_POLLIN)&&(ret != TLS_WANT_POLLOUT))
        offset+=ret;
      
    }while(offset!=buff.size());
    
    if(ret == -1)
    {
      itc::getLog()->error(__FILE__,__LINE__,"Error on reading from TLS socket: %s",tls_error(TLSSocket));
    }
    return ret;
  }
  int send(const std::string& buff,const itc::utils::Bool2Type<false> fictive)
  {
    return mSocketSPtr.get()->write((const uint8_t*)(buff.data()),buff.size());
  }
  
  int send(const std::string& buff,const itc::utils::Bool2Type<true> fictive)
  {
    int ret=0;
    size_t offset=0;
    do
    {
      ret=tls_write(TLSSocket,(const uint8_t*)(buff.data())+offset,buff.size()-offset);
      if(ret == -1)
        break;
      
      if((ret != TLS_WANT_POLLIN)&&(ret != TLS_WANT_POLLOUT))
        offset+=ret;
      
    }while(offset!=buff.size());
    
    if(ret == -1)
    {
      itc::getLog()->error(__FILE__,__LINE__,"Error on reading from TLS socket: %s",tls_error(TLSSocket));
    }
    return ret;
  }
  
 public:
  bool isValid()
  {
    return mSocketSPtr->isValid();
  }
  WebSocket()=delete;
  
  WebSocket(const itc::CSocketSPtr& socksptr,tls* tls_context=nullptr)
  : mState(CLOSED),mSocketSPtr(socksptr),streamProcessor(),TLSContext(tls_context)
  {
    init(itc::utils::Bool2Type<TLSEnable>());
  }
  ~WebSocket()
  {
    if(mState==MESSAGING)
    {
      if(mSocketSPtr->isValid())
      {
        this->closeSocket(WebSocketProtocol::SHUTDOWN);
        mSocketSPtr.get()->close();
        if(TLSEnable)
        {
          tls_close(TLSSocket);
          tls_free(TLSSocket);
        }
        mState=CLOSED;
        return;
      }
    }
    if((mState==CLOSED) && mSocketSPtr->isValid())
    {
      if(TLSEnable)
      {
         tls_close(TLSSocket);
         tls_free(TLSSocket);
      }
      mSocketSPtr.get()->close();
    }
  }
  WebSocket(const WebSocket&) = delete;
  WebSocket(WebSocket&) = delete;
  
  void processInput(const std::vector<uint8_t>& input,const size_t input_size)
  {
    if(mState!=MESSAGING) return;
    size_t cursor=0;
    again:
    auto state=streamProcessor.parse(input.data(),input_size,cursor,mSocketSPtr->getfd());
    
    switch(state.directive)
    {
      case WSStreamProcessing::Directive::MORE:
        return;
      case WSStreamProcessing::Directive::TAKE_READY_MESSAGE:
        if(onMessage(streamProcessor.getMessage())&&(state.cursor!=input_size))
        {
          cursor=state.cursor;
          // my first goto since 1991, without a butt heart on my side
          // thanks Spectre and Meltdown. [Paranoia rulez]
          goto again; 
        }
        return;
      case WSStreamProcessing::Directive::CLOSE_WITH_CODE:
        closeSocket(state.cCode);
        return;
    }
  }
  // echo 
  bool onMessage(const WSEvent& ref)
  {
    switch(ref.type)
    {
      case WebSocketProtocol::TEXT:
      {
        if(streamProcessor.isValidUtf8(ref.message->data(),ref.message->size()))
        {
          WebSocketProtocol::ServerMessage tmp(outBuffer,WebSocketProtocol::TEXT,ref.message);
          int ret=this->send(outBuffer);
          if(ret == -1) return false;
          else return true;
        }
        else 
        {
          closeSocket(WebSocketProtocol::BAD_DATA);     
          return false;
        }
      }
      break;
      case WebSocketProtocol::BINARY:
      {
        WebSocketProtocol::ServerMessage(outBuffer,WebSocketProtocol::BINARY,ref.message);
        int ret=this->send(outBuffer); // TODO: handle the return code
        if(ret == -1) return false;
        else return true;
      }
      break;
      case WebSocketProtocol::CLOSE:
        closeSocket(ref);
        return false;
        break;
      case WebSocketProtocol::PING:
        return sendPong(ref);
        break;
      case WebSocketProtocol::PONG: 
        // RFC 6455: A response to an unsolicited Pong frame is not expected.
        // This server does not requires PING-PONG at all, so all PONGs are
        // ignored.
        return true;
        break;
      default:
        closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
        return false;
        break;
    }
    return true;
  }
  
  void closeSocket(const WebSocketProtocol::DefiniteCloseCode& ccode)
  {
    WebSocketProtocol::ServerCloseMessage(outBuffer,ccode);
    this->send(outBuffer);
    mState=CLOSED;
  }
  
  void closeSocket(const WSEvent& event)
  {
    if(event.message->size() < 2)
    {
      return closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
    }
    if((event.message->size() > 2)&&(!streamProcessor.isValidUtf8(event.message->data()+2,event.message->size())))
    {
      return closeSocket(WebSocketProtocol::BAD_DATA);
    }
    WebSocketProtocol::DefiniteCloseCode endpoint_close_code=
      static_cast<WebSocketProtocol::DefiniteCloseCode>(
        be16toh(*((uint16_t*)(event.message->data())))
      );
    
    switch(endpoint_close_code)
    {
      case 1000:
      case 1001:
      case 1002:
      case 1003:
      case 1007:
      case 1008:
      case 1009:
      case 1010:
      case 1011:
        closeSocket(WebSocketProtocol::NORMAL);
        break;
      default:
        if((endpoint_close_code > 2999)&&(endpoint_close_code<5000))
        {
          return closeSocket(endpoint_close_code);
        }
        closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
        break;
    }
  }
  
 /**
   * 5.5.3.  Pong

   The Pong frame contains an opcode of 0xA.

   Section 5.5.2 details requirements that apply to both Ping and Pong
   frames.

   A Pong frame sent in response to a Ping frame must have identical
   "Application data" as found in the message body of the Ping frame
   being replied to.

   If an endpoint receives a Ping frame and has not yet sent Pong
   frame(s) in response to previous Ping frame(s), the endpoint MAY
   elect to send a Pong frame for only the most recently processed Ping
   frame.

Fette & Melnikov             Standards Track                   [Page 37]
 
RFC 6455                 The WebSocket Protocol            December 2011


   A Pong frame MAY be sent unsolicited.  This serves as a
   unidirectional heartbeat.  A response to an unsolicited Pong frame is
   not expected.
   **/  
  const bool sendPong(const WSEvent& event)
  {
    WebSocketProtocol::ServerPongMessage(
      outBuffer,
      event.message
    );
    int ret=this->send(outBuffer);
    if(ret == -1) 
      return false;
    else return true;
  }
  
  const State getState() const
  {
    return mState;
  }
  const int getFileDescriptor() const
  {
    return mSocketSPtr.get()->getfd();
  }
  
  void setState(const State state)
  {
    const static itc::utils::Bool2Type<TLSEnable> is_tls_enabled;
    setState(state,is_tls_enabled);
  }
  
  int recv(std::vector<uint8_t>& buff,const int flags=0)
  {
    const static itc::utils::Bool2Type<TLSEnable> selector;
    return this->recv(buff,flags,selector);
  }
  
  int send(const std::vector<uint8_t>& buff)
  {
    const static itc::utils::Bool2Type<TLSEnable> selector;
    return this->send(buff,selector);
  }
  
  int send(const std::string& buff)
  {
    const static itc::utils::Bool2Type<TLSEnable> selector;
    return this->send(buff,selector);
  }
  
  void emplace_header(const std::string& key,const std::string& value)
  {
    mHTTPHeaders.emplace(key,value);
  }
};

#endif /* __WEBSOCKET_H__ */

