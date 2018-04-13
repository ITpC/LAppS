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

#include <WSEvent.h>
#include <abstract/Application.h>
#include <WSStreamProcessor.h>
#include <WSServerMessage.h>
#include <Config.h>

// LibreSSL
#include <tls.h>

#include <WSConnectionStats.h>


template <bool TLSEnable=false, bool StatsEnable=false> class WebSocket
{
 public:
  enum State { HANDSHAKE, MESSAGING, CLOSED };
  
 private:
  State mState;
  itc::CSocketSPtr mSocketSPtr;
  WSStreamParser streamProcessor;
  
  struct tls* TLSContext;
  struct tls* TLSSocket;
  
  ApplicationSPtr  mApplication;
  
  uint8_t          mWorkerId;
  
  WSConnectionStats mStats;
  
  itc::utils::Bool2Type<TLSEnable>   enableTLS;
  itc::utils::Bool2Type<StatsEnable> enableStatsUpdate;
  
  
  std::vector<uint8_t> outBuffer;
  
  std::queue<TaggedEvent> mOutMessages;
  
  
  void init(const itc::utils::Bool2Type<true> tls_is_enabled)
  {
    if(tls_accept_socket(TLSContext,&TLSSocket,mSocketSPtr->getfd()))
    {
      throw std::system_error(errno,std::system_category(),"TLS: can't accept socket");
    }
  }
  
  void init(const itc::utils::Bool2Type<false> tls_is_not_enabled)
  {
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
    
  int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<false> noTLS)
  {
    if(mSocketSPtr)
    {
      auto ret=mSocketSPtr->write(buff.data(),buff.size());
      return ret;
    }
    else
    {
      itc::getLog()->error(__FILE__,__LINE__,"Socket Shared pointer is invalid");
      return -1;
    }
  }
  int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<true> withTLS)
  {
    int ret=0;
    size_t offset=0;
    do
    {
      ret=tls_write(TLSSocket,buff.data()+offset,buff.size()-offset);
      
      if(ret == -1)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Error on reading from TLS socket: %s",tls_error(TLSSocket));
        return ret;
      }
      
      if((ret != TLS_WANT_POLLIN)&&(ret != TLS_WANT_POLLOUT))
        offset+=ret;
      
    }while(offset!=buff.size());
    
    return offset;
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
  /**
   * @brief all out messages must be already prepared WebSocket messages
   **/
  void enqueueOutMessage(const TaggedEvent& e)
  {
    mOutMessages.push(e);
  }
  /**
   * @brief sending next prepared outstanding WebSocket message
   **/
  const bool sendNext()
  {
    if(mState != MESSAGING)
      return false;
    
    if(mOutMessages.empty())
      return true;
    else
    {
      auto out=mOutMessages.front();
      mOutMessages.pop();
      int ret=this->send(*out.event.message);
      if(ret == -1)
      {
        itc::getLog()->error(__FILE__,__LINE__,"Communication error on WebSocket::sendNext(). Closing this connection.");
        this->setState(CLOSED);
        return false;
      }else{
        if(static_cast<size_t>(ret) != out.event.message->size())
        {
          itc::getLog()->error(__FILE__,__LINE__,"Incomplete message send on WebSocket::sendNext(). Should never happen. Closing this connection.");
          closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
          return false;
        }
        return true;
      }
    }
  }
  
  void setApplication(const ApplicationSPtr ptr)
  {
    mApplication=ptr;
  }
  
  void setWorkerId(const uint8_t id)
  {
    mWorkerId=id;
  }
  
  bool isValid()
  {
    return mSocketSPtr->isValid();
  }
  void setState(const State state)
  {
    if((state > mState)&&(state != HANDSHAKE))
      mState=state;
    else throw std::logic_error(
        "Connection::setState(), - new state is out of order"
    );
  }
  WebSocket()=delete;
  
  explicit WebSocket(const itc::CSocketSPtr socksptr,tls* tls_context=nullptr)
  : mState(HANDSHAKE),mSocketSPtr(socksptr),streamProcessor(),TLSContext(tls_context),
    mStats{0,0,0,0,0,0},enableTLS(),enableStatsUpdate()
  {
    init(enableTLS);
  }
  void getPeerIP(uint32_t& peeraddr)
  {
    mSocketSPtr->getpeeraddr(peeraddr);
  }
  
  void getPeerIP(std::string& peeraddr)
  {
    mSocketSPtr->getpeeraddr(peeraddr);
  }
  const WSConnectionStats& getStats() const
  {
    return mStats;
  }
  ~WebSocket()
  {
    switch(mState)
    {
      case MESSAGING:
        if((mSocketSPtr->isValid()))
        {
          this->closeSocket(WebSocketProtocol::SHUTDOWN);
          if(TLSEnable)
          {
            tls_close(TLSSocket);
            tls_free(TLSSocket);
          }
          mSocketSPtr.get()->close();
          setState(CLOSED);
        }
        break;
      case CLOSED:
        if((mSocketSPtr->isValid()))
        {
          if(TLSEnable)
          {
            tls_close(TLSSocket);
            tls_free(TLSSocket);
          }
          mSocketSPtr.get()->close();
        }
        break;
      case HANDSHAKE:
        mState=CLOSED;
        if((mSocketSPtr->isValid()))
        {
          if(TLSEnable)
          {
            tls_close(TLSSocket);
            tls_free(TLSSocket);
          }
          mSocketSPtr.get()->close();
        }
        break;
    }
  }
  WebSocket(const WebSocket&) = delete;
  WebSocket(WebSocket&) = delete;
  
  void processInput(const std::vector<uint8_t>& input,const size_t input_size)
  {
    if(mState!=MESSAGING)
    {
      return;
    }
    //sendNext();
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
          goto again; 
        }
        return;
      case WSStreamProcessing::Directive::CLOSE_WITH_CODE:
        closeSocket(state.cCode);
        return;
    }
  }
  
  void updateInStats(const size_t sz, const itc::utils::Bool2Type<true> fictive)
  {
    // reset stats to avoid size_t overflow
    
    if(mStats.mInMessageCount+1 == 0xFFFFFFFFFFFFFFFFULL)
    {
      mStats.mInMessageCount=1;
      mStats.mInCMASize=0;
    }
    
    
    if(sz>mStats.mInMessageMaxSize)
      mStats.mInMessageMaxSize=sz;
    
    
    
    ++mStats.mInMessageCount;
    mStats.mInCMASize=(sz+(mStats.mInMessageCount-1)*mStats.mInCMASize)/(mStats.mInMessageCount);
  }
  
  void updateInStats(const size_t sz, const itc::utils::Bool2Type<false> fictive)
  {
  }
  // echo 
  bool onMessage(const WSEvent& ref)
  {
    if(mApplication.get() == nullptr)
    {
      throw std::system_error(EINVAL, std::system_category(), "No backend application available yet");
    }
        
    updateInStats(ref.message->size(),enableStatsUpdate);
    
    switch(ref.type)
    {
      case WebSocketProtocol::TEXT:
      {
        if(streamProcessor.isValidUtf8(ref.message->data(),ref.message->size()))
        {
          mApplication->enqueue({mWorkerId,this->getFileDescriptor(),ref});
          return true;
          
          /*
          WebSocketProtocol::ServerMessage tmp(outBuffer,WebSocketProtocol::TEXT,ref.message);
          
          int ret=this->send(outBuffer);
          if(ret == -1) return false;
          else return true;
          */
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
        
        mApplication->enqueue({mWorkerId,this->getFileDescriptor(),ref});
        return true;
        
        /*
        WebSocketProtocol::ServerMessage(outBuffer,WebSocketProtocol::BINARY,ref.message);
        int ret=this->send(outBuffer);
        if(ret == -1) return false;
        else return true;
        */
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
        // This server does not requires in PONG at all, so all PONGs are
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
    else
      return true;
  }
  
  const State getState() const
  {
    return mState;
  }
  const int getFileDescriptor() const
  {
    return mSocketSPtr.get()->getfd();
  }
    
  int recv(std::vector<uint8_t>& buff,const int flags=0)
  {
    return this->recv(buff,flags,enableTLS);
  }
  
  void updateOutStats(const size_t& buff_size,const itc::utils::Bool2Type<true> fictive)
  {
    if(buff_size>mStats.mOutMessageMaxSize)
      mStats.mOutMessageMaxSize=buff_size;
    
    if(mStats.mOutMessageCount+1 == 0xFFFFFFFFFFFFFFFFULL)
    {
      mStats.mOutMessageCount=1;
      mStats.mOutCMASize=0;
    }
    ++mStats.mOutMessageCount;
    mStats.mOutCMASize=(buff_size+(mStats.mOutMessageCount-1)*mStats.mOutCMASize)/(mStats.mOutMessageCount);
  }
  
  void updateOutStats(const size_t& buff_size,const itc::utils::Bool2Type<false> fictive)
  {
    
  }
  
  int send(const std::vector<uint8_t>& buff)
  {
    int ret=this->send(buff,enableTLS);
    
    updateOutStats(buff.size(),enableStatsUpdate);
    
    return ret;
  }
  
  int send(const std::string& buff)
  {
    int ret=this->send(buff,enableTLS);
    
    updateOutStats(buff.size(),enableStatsUpdate);
    
    return ret;
  } 
};

#endif /* __WEBSOCKET_H__ */

