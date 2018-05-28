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

static thread_local std::vector<uint8_t> anInBuffer(8192);

namespace io
{
  enum Status : int32_t  { ERROR=-1, POLLIN=-2, POLLOUT=-3 };
  enum NextOp:  uint8_t  { SEND, RECV, ANY };
}


template <bool TLSEnable=false, bool StatsEnable=false> class WebSocket
{
 public:
  enum State { HANDSHAKE, MESSAGING, CLOSED };
  
 private:
  int                                 fd;
  size_t                              mWorkerId;
  State                               mState;
  io::NextOp                          mNextOp;
  
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
  
  std::queue<MSGBufferTypeSPtr>       mOutMessages;
  
  size_t                              mOutCursor;
  
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
  
 public:

  WebSocket(const WebSocket&) = delete;
  WebSocket(WebSocket&) = delete;
  explicit WebSocket(
    const itc::CSocketSPtr&  socksptr, 
    const size_t&            workerid, 
    const SharedEPollType&   ep, 
    struct tls*              tls_context=nullptr
  )
  : fd(socksptr->getfd()),     mWorkerId(workerid),     mState(HANDSHAKE),
    mNextOp(io::NextOp::RECV), enableTLS(),             enableStatsUpdate(),
    TLSContext(tls_context),   TLSSocket(nullptr),      mSocketSPtr(std::move(socksptr)),
    streamProcessor(512),      mEPoll(ep),              mStats{0,0,0,0,512,512}, 
    mOutMessages(),            mOutCursor(0)
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
        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags&(~O_NONBLOCK));

        this->closeSocket(WebSocketProtocol::SHUTDOWN);

        while(sendNextSync());

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

        if(mApplication) 
          mApplication->enqueueDisconnect(mWorkerId,fd);

        mSocketSPtr->close();
      }
      break;  
      case State::CLOSED:
        if(TLSEnable)
        {
          tls_close(TLSSocket);
          tls_free(TLSSocket);
        }
        
        if(mApplication)
          mApplication->enqueueDisconnect(mWorkerId,fd);
        
        mSocketSPtr->close();
        
        break;
      case HANDSHAKE:
        mState=CLOSED;
        if(TLSEnable)
        {
          tls_close(TLSSocket);
          tls_free(TLSSocket);
        }
        mSocketSPtr->close();
        break;
    }
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
  
  void changeFlowToOut()
  {
    mNextOp=io::NextOp::SEND;
    mEPoll->mod_out(fd);
  }
  
  void changeFlowToIn()
  {
    mNextOp=io::NextOp::RECV;
    mEPoll->mod_in(fd);
  }
  
  void pushOutMessage(std::queue<MSGBufferTypeSPtr>& messages)
  {
    if(mState == State::MESSAGING)
    {
      while(!messages.empty())
      {
        mOutMessages.push(std::move(messages.front()));
        messages.pop();
      }
      changeFlowToOut();
      mStats.mOutMessageCount=mOutMessages.size();
    }
  }
  
  void pushOutMessage(const MSGBufferTypeSPtr& msg)
  {
    if(mState == State::MESSAGING)
    {
      mOutMessages.push(msg);
      changeFlowToOut();
      mStats.mOutMessageCount=mOutMessages.size();
    }
  }

  void setApplication(const ApplicationSPtr ptr)
  {
    mApplication=ptr;
    if(mApplication)
    {
      streamProcessor.setMaxMSGSize(mApplication->getMaxMSGSize());
      mNextOp=io::NextOp::RECV;
      int flags = fcntl(fd, F_GETFL, 0);
      fcntl(fd, F_SETFL, flags|O_NONBLOCK);
      mEPoll->mod_in(fd);
    }
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
  
  const io::NextOp getNextOp() const
  {
    return mNextOp;
  }
  
  int recv_sync(std::vector<uint8_t>& buff)
  {
    return this->recv_sync(buff,enableTLS);
  }

  int send_sync(const std::vector<uint8_t>& buff)
  {
    updateOutStats(buff.size());
    return this->send_sync(buff,enableTLS);  
  }
  
  const bool handleIO()
  {
    if(mState == State::MESSAGING)
    {
      switch(mNextOp)
      {
        case io::NextOp::ANY:
          chooseFlow();
          return handleIO();
        case io::NextOp::RECV:
          switch(this->handleInput())
          {
            case io::Status::ERROR:
              return false;
            case io::Status::POLLIN:
            case io::Status::POLLOUT:
              return true;
            default:
              return true;
          }
        case io::NextOp::SEND:
          switch(this->handleOutput())
          {
            case io::Status::ERROR:
              return false;
            case io::Status::POLLIN:
            case io::Status::POLLOUT:
            default:
              return true;
          }
      }
    }
    
    return false;
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
        mNextOp=io::NextOp::RECV;
        mEPoll->mod_in(fd);
        return;
      case WSStreamProcessing::Directive::TAKE_READY_MESSAGE:
        if(onMessage(streamProcessor.getMessage())&&(state.cursor!=input_size))
        {
          cursor=state.cursor;
          goto again; 
        }
        mNextOp=io::NextOp::ANY;
        return;
      case WSStreamProcessing::Directive::CLOSE_WITH_CODE:
        closeSocket(state.cCode);
        return;
    }
  }
  
   /**
   * @brief sending next prepared outstanding WebSocket message
   **/
  const int sendNext()
  {
    if(mOutMessages.empty())
    {
      changeFlowToIn();
      mStats.mOutMessageCount=0;
      return io::Status::POLLIN;
    }
    else
    {
      auto message=mOutMessages.front();
      int ret=this->send(*message);
      if((ret > 0 )&&(message->size() == static_cast<size_t>(ret)))
      {
        mOutMessages.pop();
        if((*message)[0] == (128|8)) // close
        {
          mState=State::CLOSED;
          mStats.mOutMessageCount=0;
          return io::Status::ERROR;
        }
        else
        {
          if(mOutMessages.empty())
          {
            changeFlowToIn();
            mStats.mOutMessageCount=0;
            return ret;
          }
        }
      }
      changeFlowToOut();
      mStats.mOutMessageCount=mOutMessages.size();
      return ret;
    }
  }
  const bool sendNextSync()
  {
    if(mOutMessages.empty())
    {
      return false;
    }
    else
    {
      auto message=mOutMessages.front();
      int ret=this->send(*message);
      if(ret > 0 )
      {
        mOutMessages.pop();
        if((*message)[0] == (128|8)) // close
        {
          mApplication->enqueueDisconnect(mWorkerId,fd);
          return false;
        }
        return true;
      }
      return false;
    }
  }
  int handleInput()
  {
    if(mState == State::MESSAGING)
    {
      int ret=this->recv(anInBuffer);
      
      if(ret>0)
      {
        processInput(anInBuffer,ret);
      }
      return ret;
    }
    return io::Status::ERROR;
  }
  
  int handleOutput()
  {
    if(mState == State::MESSAGING)
    {
      return sendNext();
    }
    return io::Status::ERROR;
  }
  // async IO is private.
  int recv(std::vector<uint8_t>& buff)
  {
    updateInStats(buff.size());
    return this->recv(buff,enableTLS);
  }

  int send(const std::vector<uint8_t>& buff)
  {
    updateOutStats(buff.size());
    
    int ret=this->send(buff,enableTLS);
    
    return ret;
  }
 
  void updateInStats(const size_t sz)
  {
    updateInStats(sz,enableStatsUpdate);
  }
  
  void updateInStats(const size_t sz, const itc::utils::Bool2Type<true>& withStats)
  {
    ++mStats.mInMessageCount;
    mStats.mInCMASize=(sz+(mStats.mInMessageCount-1)*mStats.mInCMASize)/(mStats.mInMessageCount);
    
    // reset stats to avoid size_t overflow
    if(mStats.mInMessageCount+1 == 0xFFFFFFFFFFFFFFFFULL)
    {
      mStats.mInMessageCount=1;
      mStats.mInCMASize=512;
    }
    
    
    if(sz>mStats.mInMessageMaxSize)
        mStats.mInMessageMaxSize=sz;
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
        
    updateInStats(ref.message->size(),enableStatsUpdate);
    
    switch(ref.type)
    {
      case WebSocketProtocol::TEXT:
        if(streamProcessor.isValidUtf8(ref.message->data(),ref.message->size()))
        {
          mApplication->enqueue({mWorkerId,fd,ref});
          return true;          
        }
        else 
        {
          closeSocket(WebSocketProtocol::BAD_DATA);     
          return false;
        }
      case WebSocketProtocol::BINARY:
        mApplication->enqueue({mWorkerId,fd,ref});
        return true;
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
        mNextOp=io::NextOp::RECV;
        mEPoll->mod_in(fd);
        return true;
      default:
        closeSocket(WebSocketProtocol::PROTOCOL_VIOLATION);
        return false;
    }
    return true;
  }
  
  void closeSocket(const WebSocketProtocol::DefiniteCloseCode& ccode)
  {
    mApplication->enqueueDisconnect(mWorkerId,fd);
    auto outBuffer=std::make_shared<MSGBufferType>();
    WebSocketProtocol::ServerCloseMessage(*outBuffer,ccode);
    pushOutMessage(outBuffer);
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
    pushOutMessage(outBuffer);
  }
  
  void updateOutStats(const size_t sz)
  {
    updateOutStats(sz,enableStatsUpdate);
  }
  void updateOutStats(const size_t sz, const itc::utils::Bool2Type<true>& withStats)
  {
    ++mStats.mOutMessageCount;
    mStats.mOutCMASize=(sz+(mStats.mOutMessageCount-1)*mStats.mOutCMASize)/(mStats.mOutMessageCount);
    
    if(mStats.mOutMessageCount+1 == 0xFFFFFFFFFFFFFFFFULL)
    {
      mStats.mOutMessageCount=1;
      mStats.mOutCMASize=512;
    }
    
    if(sz>mStats.mOutMessageMaxSize)
    {
      mStats.mOutMessageMaxSize=sz;
    }
  }
  
  void updateOutStats(const size_t buff_size,const itc::utils::Bool2Type<false>& noStats)
  {
    
  }
  
  void chooseFlow()
  {
    if(mNextOp==io::NextOp::ANY)
    {
      if(mOutMessages.size()>0)
      {
      mNextOp=io::NextOp::SEND;
      mEPoll->mod_out(fd);
      }
      else
      {
        mNextOp=io::NextOp::RECV;
        mEPoll->mod_in(fd);
      }
    }
  }
  
  int recv(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<true> withTLS)
  {
    int ret=tls_read(TLSSocket,buff.data(),buff.size());

    switch(ret)
    {
      case -1:
        if(mApplication) mApplication->enqueueDisconnect(mWorkerId,fd);
        mState=State::CLOSED;
        return io::Status::ERROR;
      case TLS_WANT_POLLIN:
        mEPoll->mod_in(fd);
        return io::Status::POLLIN;
      case TLS_WANT_POLLOUT:
        mEPoll->mod_out(fd);
        return io::Status::POLLOUT;
      default:
        mNextOp=io::NextOp::ANY;
        return ret;
    }
  }

  int recv(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<false> noTLS)
  {
    int ret=::recv(fd,buff.data(),buff.size(),MSG_NOSIGNAL);
    if(ret == -1)
    {
      if((errno == EWOULDBLOCK)||(errno == EAGAIN))
      {
        mEPoll->mod_in(fd);
        return io::Status::POLLIN;
      }
      else
      {
        if(mApplication) mApplication->enqueueDisconnect(mWorkerId,fd);
        mState=State::CLOSED;
        return io::Status::ERROR;
      }
    }
    mNextOp=io::NextOp::ANY;
    return ret;
  }

  const int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<false> noTLS)
  {
    const int result=::send(fd,buff.data()+mOutCursor,buff.size()-mOutCursor,MSG_NOSIGNAL);

    if(result == -1)
    {
      if((errno == EAGAIN)||(errno == EWOULDBLOCK))
      {
        mEPoll->mod_out(fd);
        return io::Status::POLLOUT;
      }
      if(mApplication) mApplication->enqueueDisconnect(mWorkerId,fd);
      mState=State::CLOSED;
      return io::Status::ERROR;
    }
    
    mOutCursor+=result;
    
    if(mOutCursor != buff.size())
    {
      mEPoll->mod_out(fd);
      return io::Status::POLLOUT;
    }

    mOutCursor=0;
    mNextOp=io::NextOp::ANY;
    return buff.size();
  }

  const int send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<true> withTLS)
  {
    const int result=tls_write(TLSSocket,buff.data()+mOutCursor,buff.size()-mOutCursor);
    switch(result)
    {
      case -1:
        if(mApplication) mApplication->enqueueDisconnect(mWorkerId,fd);
        mState=State::CLOSED;
        return io::Status::ERROR;
      case TLS_WANT_POLLIN:
        mEPoll->mod_in(fd);
        return io::Status::POLLIN;
      case TLS_WANT_POLLOUT:
        mEPoll->mod_out(fd);
        return io::Status::POLLOUT;
      default:
        mOutCursor+=result;
        
        if(mOutCursor != buff.size())
        {
          mEPoll->mod_out(fd);
          return io::Status::POLLOUT;
        }
        mOutCursor=0;
        mNextOp=io::NextOp::ANY;
        return buff.size();
    }
  }
  
  int recv_sync(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<false> noTLS)
  {
    while(1)
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
          return io::Status::ERROR;
        }
      }
      return ret;
    }
  }
  
  int recv_sync(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<true> withTLS)
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
   
  const int send_sync(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<false> noTLS)
  {
    size_t outCursor=0;
    do
    {
      const int result=::send(fd,buff.data()+outCursor,buff.size()-outCursor,MSG_NOSIGNAL);

      if(result == -1)
      {
        if((errno == EAGAIN)||(errno == EWOULDBLOCK))
          continue;
        return io::Status::ERROR;
      }

      outCursor+=result;

    } while(outCursor != buff.size());

    return outCursor;
  }

  const int send_sync(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<true> withTLS)
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
        return io::Status::ERROR;
      }
      outCursor+=result;
    }while(outCursor!=buff.size());

    return outCursor;
  }  
};

#endif /* __WEBSOCKET_H__ */

