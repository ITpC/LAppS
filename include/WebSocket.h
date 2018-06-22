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
#include <ePoll.h>
#include <Config.h>

// LibreSSL
#include <tls.h>

#include <WSConnectionStats.h>

#include <abstract/WebSocket.h>

static thread_local std::vector<uint8_t> anInBuffer(8192);


template <bool TLSEnable=false, bool StatsEnable=false> class WebSocket
: public abstract::WebSocket
{
 public:
  std::shared_ptr<abstract::WebSocket> get_shared()
  {
    return this->shared_from_this();
  }
  
 private:
  int                                 fd;
  size_t                              mWorkerId;
  State                               mState;
  
  itc::utils::Bool2Type<TLSEnable>    enableTLS;
  itc::utils::Bool2Type<StatsEnable>  enableStatsUpdate;
  
  struct tls*                         TLSContext;
  struct tls*                         TLSSocket;
  
  itc::CSocketSPtr                    mSocketSPtr;
  
  uint32_t                            mPeerIP;
  std::string                         mPeerAddress;
  
  WSStreamParser                      streamProcessor;
  
  ApplicationSPtr                     mApplication;
  
  SharedEPollType                     mEPoll;
  
  WSConnectionStats                   mStats;
  
  size_t                              mOutCursor;
  
  bool                                mAutoFragment;
    
  void init(const itc::utils::Bool2Type<true> tls_is_enabled)
  {
    if(tls_accept_socket(TLSContext,&TLSSocket,fd))
    {
      throw std::system_error(errno,std::system_category(),"TLS: can't accept socket");
    }
  }
  
  void init(const itc::utils::Bool2Type<false> tls_is_not_enabled)
  {
  }
    
  void close()
  {
    if(TLSEnable)
    {
      int ret=0;
      do
      {
        ret=tls_close(TLSSocket);
      }while((ret == TLS_WANT_POLLIN)||(ret == TLS_WANT_POLLOUT));

      tls_free(TLSSocket);
    }
    setState(CLOSED);  

    mSocketSPtr->close();
  }
 public:

  WebSocket(const WebSocket&) = delete;
  WebSocket(WebSocket&) = delete;
  explicit WebSocket(
    const itc::CSocketSPtr&  socksptr, 
    const size_t&            workerid, 
    const SharedEPollType&   ep,
    const bool               auto_fragment,
    struct tls*              tls_context=nullptr
  )
  : fd(socksptr->getfd()),     mWorkerId(workerid),     mState(HANDSHAKE),
    enableTLS(),               enableStatsUpdate(),
    TLSContext(tls_context),   TLSSocket(nullptr),      mSocketSPtr(std::move(socksptr)),
    streamProcessor(512),      mEPoll(ep),              mStats{0,0,0,0,0,0}, 
    mOutCursor(0),             mAutoFragment(auto_fragment)
  {
    init(enableTLS);
    mSocketSPtr->getpeeraddr(mPeerIP);
    mSocketSPtr->getpeeraddr(mPeerAddress);
    mEPoll->add_in(fd);
  }
    
  WebSocket()=delete;
  
  ~WebSocket()
  {
    switch(mState)
    {
      case State::MESSAGING:
      {
        this->closeSocket(WebSocketProtocol::SHUTDOWN);        
        this->close();
      }
      break;  
      case State::CLOSED:
      case State::HANDSHAKE:
        this->close();
        break;
    }
  }
  
  const bool mustAutoFragment() const
  {
    return mAutoFragment;
  }
  
  const struct tls* getTLSSocket()
  {
    return TLSSocket;
  }
  
  const uint32_t getPeerIP() const
  {
    return mPeerIP;
  }
  
  const std::string& getPeerAddress() const
  {
    return mPeerAddress;
  }
  const WSConnectionStats& getStats() const
  {
    return mStats;
  }
    

  void setApplication(const ApplicationSPtr ptr)
  {
    mApplication=ptr;
    if(mApplication)
    {
      streamProcessor.setMaxMSGSize(mApplication->getMaxMSGSize());
      mEPoll->mod_in(fd);
    }
  }
  
  const ApplicationSPtr& getApplication() const
  {
    return mApplication;
  }
  
  bool isValid()
  {
    return mSocketSPtr->isValid();
  }
  void setState(const State state)
  {
    if(((state > mState)&&(state != HANDSHAKE))||(state == mState))
      mState=state;
    else throw std::logic_error(
        "Connection::setState(), - new state is out of order"
    );
  }
  
  const State getState() const
  {
    return mState;
  }
  const int getfd() const
  {
    return fd;
  }
  
  int recv(std::vector<uint8_t>& buff)
  {
    return this->recv(buff,enableTLS);
  }

  const int send(const std::vector<uint8_t>& buff)
  {
    updateOutStats(buff.size());
    int ret=this->send(buff,enableTLS);
    return ret;
  }
  
  
  
  const int handleInput()
  {
    if(mState == State::MESSAGING)
    {
      int ret=this->recv(anInBuffer);

      if(ret>0)
      {
        processInput(anInBuffer,ret);
      }
      mEPoll->mod_in(fd);
      return ret;
    }
    mState = State::CLOSED;
    return -1;
  }
  
private:
 
  void processInput(const std::vector<uint8_t>& input,const size_t input_size)
  {
    if(mState!=State::MESSAGING)
    {
      return;
    }
    
    size_t cursor=0;
    again:
    auto state=streamProcessor.parse(input.data(),input_size,cursor,fd);
    
    switch(state.directive)
    {
      case WSStreamProcessing::Directive::MORE:
        return;
      case WSStreamProcessing::Directive::TAKE_READY_MESSAGE:
      {
        if(onMessage(std::move(streamProcessor.getMessage()))&&(state.cursor!=input_size))
        {
          cursor=state.cursor;
          goto again; 
        }
        return;
      }
      case WSStreamProcessing::Directive::CLOSE_WITH_CODE:
        closeSocket(state.cCode);
        return;
    }
  }
 
  void updateInStats(const size_t sz)
  {
    updateInStats(sz,enableStatsUpdate);
  }
  
  void updateInStats(const size_t sz, const itc::utils::Bool2Type<true>& withStats)
  { 
    if(mStats.mInMessageCount+1 == 0xFFFFFFFFFFFFFFFFULL)
    {
      mStats.mInMessageCount=1;
    }
    else ++mStats.mInMessageCount;
    
    if(sz == 0) return; // exclude 0-size messages;
    int64_t size=sz;
    int64_t cma_size=mStats.mInCMASize;
    cma_size=std::abs(cma_size+((size-cma_size)/static_cast<int64_t>(mStats.mInMessageCount)));
    mStats.mInCMASize=cma_size;
  }
  
  void updateInStats(const size_t sz, const itc::utils::Bool2Type<false>& noStats)
  {
  }
  
  bool onMessage(const WSEvent& ref)
  {
    if(!mApplication)
    {
      throw std::system_error(EINVAL, std::system_category(), "No backend application available yet");
    }

    updateInStats(ref.message->size());
    streamProcessor.setMessageBufferSize(mStats.mInCMASize);
    
    switch(ref.type)
    {
      case WebSocketProtocol::TEXT:
        if(streamProcessor.isValidUtf8(ref.message->data(),ref.message->size()))
        {
         
          if(mApplication) mApplication->enqueue(
            std::move(
              abstract::InEvent{
                WebSocketProtocol::TEXT,
                this->get_shared(),
                std::move(ref.message)
              }
            )
          );
          return true;          
        }
        else 
        {
          closeSocket(WebSocketProtocol::BAD_DATA);     
          return false;
        }
      case WebSocketProtocol::BINARY:
      {
        if(mApplication) mApplication->enqueue(
          std::move(
            abstract::InEvent{
              WebSocketProtocol::OpCode::BINARY,
              this->get_shared(),
              std::move(ref.message)
            }
          )
        );
        return true;
      }
      break;
      case WebSocketProtocol::CLOSE:
        onCloseMessage(ref);
        return false;
      case WebSocketProtocol::PING:
        sendPong(ref);
        return true;
      case WebSocketProtocol::PONG: 
        // RFC 6455: A response to an unsolicited Pong frame is not expected.
        // This server does not requires in PONG at all, so all PONGs are
        // ignored.
        return true;
      default:
        closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
        return false;
    }
    return true;
  }
  
  void closeSocket(const WebSocketProtocol::DefiniteCloseCode& ccode)
  {
    try
    {
      auto outBuffer=std::make_shared<MSGBufferType>();
      WebSocketProtocol::ServerCloseMessage(*outBuffer,ccode);
      
      if(mApplication) mApplication->enqueue(std::move(
          abstract::InEvent{
            WebSocketProtocol::OpCode::CLOSE,
            this->get_shared(),
            outBuffer
          }
        )
      );
    }
    catch(const std::exception& e)
    {
      itc::getLog()->info(
        __FILE__,
        __LINE__,
        "Can not send close frame to %s, due to exception [%s]. Probable cause - peer disconnect.",
        mPeerAddress.c_str(),e.what()
      );
    }
  }
  
  void onCloseMessage(const WSEvent& event)
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
  void sendPong(const WSEvent& event)
  {
    auto outBuffer=std::make_shared<MSGBufferType>();
    WebSocketProtocol::ServerPongMessage(
      *outBuffer,
      event.message
    );
    if(mApplication) mApplication->enqueue(std::move(
        abstract::InEvent{
          WebSocketProtocol::OpCode::PONG,
          this->get_shared(),
          outBuffer
        }
      )
    );;
  }
  
  void updateOutStats(const size_t sz)
  {
    updateOutStats(sz,enableStatsUpdate);
  }
  void updateOutStats(const size_t sz, const itc::utils::Bool2Type<true>& withStats)
  {
    if(mStats.mOutMessageCount+1 == 0xFFFFFFFFFFFFFFFFULL)
    {
      mStats.mOutMessageCount=1;
    }
    
    ++mStats.mOutMessageCount;
    mStats.mOutCMASize=mStats.mOutCMASize+(sz-mStats.mOutCMASize)/mStats.mOutMessageCount;
  }
  
  void updateOutStats(const size_t buff_size,const itc::utils::Bool2Type<false>& noStats)
  {  
  }
  
  int recv(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<false> noTLS)
  {
    while(true)
    {
      int ret=::recv(fd,buff.data(),buff.size(),MSG_NOSIGNAL);
      if(ret == -1)
      {
        if((errno == EWOULDBLOCK)||(errno == EAGAIN))
        {
          continue;
        }
        else
        {
          return -1;
        }
      }
      return ret;
    }
  }
  
  int recv(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<true> withTLS)
  {
    int ret=0;
    
    do{
      ret=tls_read(TLSSocket,buff.data(),buff.size());
    
      if(ret == -1)
      {
        return -1;
      }
    }while((ret == TLS_WANT_POLLIN) || (ret == TLS_WANT_POLLOUT));
    return ret;
  }
  
  const int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<false> noTLS)
  {
    size_t outCursor=0;
    do
    {
      const int result=::send(fd,buff.data()+outCursor,buff.size()-outCursor,MSG_NOSIGNAL);

      if(result == -1)
      {
        if((errno == EAGAIN)||(errno == EWOULDBLOCK))
          continue;
        return -1;
      }

      outCursor+=result;

    } while(outCursor != buff.size());
    
    return outCursor;
  }

  const int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<true> withTLS)
  {
    size_t outCursor=0;
    do
    {
      const int result=tls_write(TLSSocket,buff.data()+outCursor,buff.size()-outCursor);

      if((result == TLS_WANT_POLLIN) || (result == TLS_WANT_POLLOUT))
        continue;

      if(result == -1)
      {
        mState=State::CLOSED;
        itc::getLog()->error(
          __FILE__,__LINE__,
          "TLS Error on fd[%d] with peer %s: %s",
          fd,mPeerAddress.c_str(),tls_error(TLSContext)
        );
        return -1;
      }
      outCursor+=result;
    }while((buff.size()-outCursor)>0);

    return outCursor;
  }  
};

#endif /* __WEBSOCKET_H__ */

