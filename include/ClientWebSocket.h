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
 *  $Id: ClientWebSocket.h August 7, 2018 12:22 PM $
 * 
 **/


#ifndef __CLIENTWEBSOCKET_H__
#  define __CLIENTWEBSOCKET_H__

#include <memory>
#include <vector>
#include <string>
#include <regex>

#include <net/NSocket.h>
#include <TCPSocketDef.h>
#include <missing.h>
#include <sys/mutex.h>
#include <sys/synclock.h>

#include <iostream>

// crypto++
#include <cryptopp/sha.h>
#include <cryptopp/osrng.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <cryptopp/hex.h>


// LAppS

#include <WSProtocol.h>
#include <WSEvent.h>
#include <WSStreamClientParser.h>

#include <modules/nljson.h>

// wolfSSL
#include <wolfSSLLib.h>


namespace LAppS
{
  
  static thread_local CryptoPP::AutoSeededRandomPool RNG(true);
  static thread_local CryptoPP::Base64Encoder        BASE64;
  static thread_local CryptoPP::SHA1                 SHA1;
  
  static thread_local std::vector<uint8_t> recvBuffer(8192,0);
  
  static const std::regex WSURI{"^ws{1,2}[:][/][/](([[:alnum:]]+([-][[:alnum:]]+)*)+[.]?)+([[:alpha:]]{2,})?([:][[:digit:]]{1,5}){0,1}(([/][[:graph:]]+[^/])+|[/])$"};
  
  
  
  namespace WSClient
  {
    enum URISearchDirective {LOOK_FOR_HOSTNAME,LOOK_FOR_PORT,LOOK_FOR_RTARGET,STOP_LOOKING};  
    enum State { INIT, HANDSHAKE_FAILED, COMM_ERROR, CLOSED, HANDSHAKE, MESSAGING };
    enum InputStatus { FORCE_CLOSE=-2, ERROR=-1, NEED_MORE_DATA=1, MESSAGE_READY_BUFFER_IS_EMPTY=2, MESSAGE_READY_BUFFER_IS_NOT_EMPTY=3, CALL_ONCE_MORE=4, UPGRADED=5, UPGRADED_BUFF_NOT_EMPTY=6};
    typedef WSStreamProcessing::State MessageState;
  }
  
  class ClientWebSocket: public itc::ClientSocket
  {
   private:
    using TLSClientContext=std::shared_ptr<wolfSSLLib<TLS_CLIENT>::wolfSSLContext>;
    size_t                  cursor;
    TLSClientContext        TLSContext;
    WOLFSSL*                TLSSocket;
    
    itc::sys::mutex         mMutex;
    
    bool                    tls;
    WSClient::State         mState;
    
    MSGBufferTypeSPtr       mCurrentMessage;
    
    std::string             mSecWebSocketKey;
    
    WSStreamProcessing::WSStreamClientParser  streamProcessor;
    
    void terminate()
    {
      if(mState != WSClient::State::CLOSED)
      {
        sigset_t sigsetmask;
        sigemptyset(&sigsetmask);
        sigaddset(&sigsetmask, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &sigsetmask, NULL); // ignore if can't mask;

        if(tls)
        {
          wolfSSL_shutdown(TLSSocket);
        }      
        mState=WSClient::State::CLOSED;
      }
    }
    
    const int force_send(const std::string& buff,const itc::utils::Bool2Type<false> noTLS)
    {
      size_t outCursor=0;
      do
      {
        const int result=::send(this->getfd(),buff.data()+outCursor,buff.length()-outCursor,MSG_NOSIGNAL);

        if(result == -1)
        {
          if((errno == EAGAIN)||(errno == EWOULDBLOCK))
            continue;
          return -1;
        }

        outCursor+=result;

      } while(outCursor != buff.length());
      return outCursor;
    }

    const int force_send(const std::string& buff,const itc::utils::Bool2Type<true> withTLS)
    {
      if(TLSSocket)
      {
        size_t outCursor=0;
        do
        {
          const int result=wolfSSL_write(TLSSocket,buff.data()+outCursor,buff.length()-outCursor);

          if(result == -1)
          {
            logWOLFSSLError(result,"ClientWebSocket::force_send(withTLS): ");
            return -1;
          }
          
          outCursor+=result;
          
        }while(outCursor!=buff.length());

        return outCursor;
      }
      return -1;
    }
    const int force_send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<false> noTLS)
    {
      size_t outCursor=0;
      do
      {
        const int result=::send(this->getfd(),buff.data()+outCursor,buff.size()-outCursor,MSG_NOSIGNAL);

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
    
    const int force_send(const std::vector<uint8_t>& buff,const itc::utils::Bool2Type<true> withTLS)
    {
      if(TLSSocket)
      {
        size_t outCursor=0;
        do
        {
          const int result=wolfSSL_write(TLSSocket,buff.data()+outCursor,buff.size()-outCursor);

          if(result == -1)
          {
            logWOLFSSLError(result,"ClientWebSocket::force_send(withTLS): ");
            return -1;
          }
          outCursor+=result;
        }while(outCursor!=buff.size());

        return outCursor;
      }
      return -1;
    }
    
    int force_recv(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<false> noTLS)
    {
      while(true)
      {
        int ret=::recv(this->getfd(),buff.data(),buff.size(),MSG_NOSIGNAL);
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

    int force_recv(std::vector<uint8_t>& buff, const itc::utils::Bool2Type<true> withTLS)
    {
      if(TLSSocket)
      {
        int ret=wolfSSL_read(TLSSocket,buff.data(),buff.size());
          
        if(ret <= 0)
        {
          logWOLFSSLError(ret,"ClientWebSocket::force_recv(withTLS): ");
          return -1;
        }
        
        return ret;
      }
      return -1;
    }
    
    const bool responseOK(const char* buff, size_t sz, const std::string& required_sec_websocket_accept)
    {
      if(sz<113) return false;
      
      if(strncmp(buff,"HTTP/1.1 101 Switching Protocols\r\n",34)==0) // first line of response is correct
      {
        std::map<std::string,std::string> headers;
        size_t index=34;

        while(index < sz)
        {
          std::string key;
          std::string value;

          // skip CRLF
          size_t counter=0;
          while((index<sz)&&((buff[index] == '\r')||(buff[index] == '\n')))
          {
            ++index;
            ++counter;
          }
          
          if(counter == 4)
          {
            if(index != sz)
            {
              cursor=index;
            }
            else
              cursor=0;
            break;
          }
          // read key
          while((index<sz)&&(buff[index] != ':'))
          {
            key.append(1,buff[index]);
            ++index;
          }

          // skip ':' and spaces
          while((index<sz)&&((buff[index] == '\t')||(buff[index] == ' ')||(buff[index] == ':')))
          {
            ++index;
          }
          
          // read value
          while((index<sz)&&(buff[index] != '\r')&&(buff[index] != '\n'))
          {
            value.append(1,buff[index]);
            ++index;
          }
          
          if((!key.empty())&&(!value.empty()))
            headers.emplace(std::move(key),std::move(value));
        }

        if(!headers.empty() && ( itc::utils::toupper(headers["Connection"]) == "UPGRADE") && ( itc::utils::toupper(headers["Upgrade"]) == "WEBSOCKET"))
        {
          if(headers["Sec-WebSocket-Accept"] == required_sec_websocket_accept)
            return true;
          else
            return false;
        }
        else return false;
      }
      return false;
    }
    const int force_recv(std::vector<uint8_t>& buff)
    {
      static const itc::utils::Bool2Type<true> TLSEnabled;
      static const itc::utils::Bool2Type<false> TLSDisabled;
     
      ITCSyncLock sync(mMutex);
      if(tls)
      {
        return force_recv(buff,TLSEnabled);
      }else{
        return force_recv(buff,TLSDisabled);
      }
    }
    
    const int force_send(const std::vector<uint8_t>& buff)
    {
      static const itc::utils::Bool2Type<true> TLSEnabled;
      static const itc::utils::Bool2Type<false> TLSDisabled;
      
      ITCSyncLock sync(mMutex);
      if(tls)
      {
        return force_send(buff,TLSEnabled);
      }else{
        return force_send(buff,TLSDisabled);
      }
    }
    
    
    const int force_send(const std::string& buff)
    {
      static const itc::utils::Bool2Type<true> TLSEnabled;
      static const itc::utils::Bool2Type<false> TLSDisabled;
      
      ITCSyncLock sync(mMutex);
      if(tls)
      {
        return force_send(buff,TLSEnabled);
      }else{
        return force_send(buff,TLSDisabled);
      }
    }
    
    const bool serverResponseOk()
    {
      int ret=force_recv(recvBuffer);
      if(ret == 0) return false; 
          //throw std::system_error(ECONNRESET,std::system_category(),"ClientWebSocket::ClientWebSocket() - error on recv, - peer has shutdown connection");
      if(ret>0)
      {
        std::string accept_key_src(mSecWebSocketKey+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

        CryptoPP::byte digest[CryptoPP::SHA1::DIGESTSIZE];
        SHA1.CalculateDigest(digest,(const CryptoPP::byte*)(accept_key_src.data()),accept_key_src.length());

        BASE64.Initialize();
        BASE64.Put(digest, CryptoPP::SHA1::DIGESTSIZE);
        BASE64.MessageEnd();

        size_t accept_key_length=BASE64.MaxRetrievable()-1; // skip trailing \n

        std::string required_accept_key(accept_key_length,'\0');
        BASE64.Get((CryptoPP::byte*)(required_accept_key.data()),accept_key_length);

        return responseOK((const char*)(recvBuffer.data()),ret,required_accept_key);
      }
      else
      {
        return false;
        //throw std::system_error(EBADE,std::system_category(),"ClientWebSocket handshake failed on read");
      }
    }
    
   public:
    
    explicit ClientWebSocket(const std::string& uri, const bool _noverifycert=false, const bool _noverifyname=false)
    : itc::ClientSocket(), cursor{0}, TLSContext(wolfSSLClient::getInstance()->getContext()), TLSSocket{nullptr}, 
      mMutex(),mState{WSClient::State::INIT},streamProcessor(512)
    {
      if(std::regex_match(uri,WSURI))
      {
        WSClient::URISearchDirective sstate=WSClient::URISearchDirective::LOOK_FOR_HOSTNAME;
        size_t     search_start_idx=0;
        
        if(uri[2] == ':') // NON-TLS
        {
          search_start_idx=5;
          tls=false;
          
        }else{ // TLS wss://
          tls=true;
          search_start_idx=6;
        }
        
        size_t hostname_length=uri.find(':',search_start_idx);
        
        if(hostname_length == std::string::npos)  // no port is specified
        {
          hostname_length=uri.find('/',search_start_idx);
          if(hostname_length == std::string::npos)  // no request target is specified
          {
            hostname_length=uri.length();
            sstate=WSClient::URISearchDirective::STOP_LOOKING;
          }else{
            sstate=WSClient::URISearchDirective::LOOK_FOR_RTARGET;
          }
        }else{
          sstate=WSClient::URISearchDirective::LOOK_FOR_PORT;
        }
        
        hostname_length=hostname_length-search_start_idx;
        
        
        std::string hostname(uri,search_start_idx,hostname_length);
        
        search_start_idx+=hostname_length;
        
        uint32_t    port=0;
        std::string request_target;
        size_t      port_length=0;
        size_t      port_end_idx=0;
        
        
        switch(sstate)
        {
          case WSClient::URISearchDirective::LOOK_FOR_PORT:
          {    
            search_start_idx++;
            port_end_idx=uri.find('/',search_start_idx);
            
            if(port_end_idx == std::string::npos)
            {
              port_end_idx=uri.length();
            }else{
              sstate=WSClient::URISearchDirective::LOOK_FOR_RTARGET;
            }
            
            port_length=port_end_idx-search_start_idx;
            
            std::string str_port(uri,search_start_idx,port_length);
            
            port=atoi(str_port.c_str());
            
            search_start_idx=port_end_idx;            
          }
          case WSClient::URISearchDirective::LOOK_FOR_RTARGET:
            if(sstate==WSClient::URISearchDirective::LOOK_FOR_RTARGET)
            {
              request_target=uri.substr(search_start_idx,uri.length()-port_end_idx);
              sstate=WSClient::URISearchDirective::STOP_LOOKING;
            }
          default:
            break;
        }
        
        if(tls)
        {
          if(port==0) port=443;
        
          this->open(hostname,port);
          
          TLSSocket=wolfSSL_new(TLSContext->raw_context());
          
          if( TLSSocket == nullptr)
          {
            throw std::system_error(errno,std::system_category(),"TLS: can't create new WOLFSSL* object");
          }else{
            wolfSSL_set_fd(TLSSocket,this->getfd());
          }
        }else{
          if(port==0) port=80;
          this->open(hostname,port);
        }
        
        std::string httpUpgradeRequest;
        httpUpgradeRequest.append("GET ");
        httpUpgradeRequest.append(request_target);
        httpUpgradeRequest.append(" ");
        httpUpgradeRequest.append("HTTP/1.1\r\n");
        httpUpgradeRequest.append("Host: ");
        httpUpgradeRequest.append(hostname);
        httpUpgradeRequest.append("\r\n");
        httpUpgradeRequest.append("Upgrade: websocket\r\n");
        httpUpgradeRequest.append("Connection: Upgrade\r\n");
        httpUpgradeRequest.append("User-Agent: LAppS/0.7.0\r\n");
        httpUpgradeRequest.append("Sec-WebSocket-Key: ");
        
        std::vector<uint8_t> ws_sec_key(16,0);
        RNG.GenerateBlock(ws_sec_key.data(),16);
        
        BASE64.Initialize();
        BASE64.Put(ws_sec_key.data(), 16);
        BASE64.MessageEnd();

        size_t base64_str_size=BASE64.MaxRetrievable()-1; // remove trailing \n
        
        mSecWebSocketKey.resize(base64_str_size,'\0');
        BASE64.Get((CryptoPP::byte*)(mSecWebSocketKey.data()),base64_str_size);
        
        httpUpgradeRequest.append(mSecWebSocketKey);
        httpUpgradeRequest.append("\r\n");

        if(tls)
        {
          httpUpgradeRequest.append("Origin: https://");
        }
        else
        {
          httpUpgradeRequest.append("Origin: http://");
        }
        
        httpUpgradeRequest.append(hostname);
        httpUpgradeRequest.append("\r\n");
        httpUpgradeRequest.append("Sec-WebSocket-Version: 13\r\n\r\n");
        
        // prevent SIGPIPE
        
        sigset_t sigsetmask;
        sigemptyset(&sigsetmask);
        sigaddset(&sigsetmask, SIGPIPE);
        pthread_sigmask(SIG_BLOCK, &sigsetmask, NULL);
        
        // handshake

        int ret=force_send(httpUpgradeRequest);
        
        if(ret == 0)
        {
          if(tls&&TLSSocket)
          {
            wolfSSL_free(TLSSocket);
            TLSSocket=nullptr;
          }
          throw std::system_error(ECONNRESET,std::system_category(),"ClientWebSocket::ClientWebSocket() - error on send, - peer has shutdown connection");
        }
        
        if(ret>0)
        {
          mState=WSClient::State::HANDSHAKE;
        }else{
          if(tls&&TLSSocket)
          {
            wolfSSL_free(TLSSocket);
            TLSSocket=nullptr;
          }
          throw std::system_error(EBADE,std::system_category(),"ClientWebSocket handshake failed on write");
        }
      }
      else throw std::system_error(EINVAL,std::system_category(),"Error: "+uri+" is not a WebSockets URI");
    }
      
    void closeSocket(const WebSocketProtocol::DefiniteCloseCode& ccode)
    {
      std::vector<uint8_t> out;
      uint16_t beCode=htobe16(ccode);
      out.push_back(128|8);
      out.push_back(128|2);
      // mask
      out.push_back(0);
      out.push_back(0);
      out.push_back(0);
      out.push_back(0);
      
      out.push_back(static_cast<uint8_t>((beCode<<8)>>8)^0);
      out.push_back(static_cast<uint8_t>(beCode>>8)^0);
      force_send(out);
      terminate();
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
    
    void sendPong(const WSEvent& event)
    {
      send(event.message->data(),event.message->size(),WebSocketProtocol::OpCode::PONG);
    }
    
    ~ClientWebSocket() override
    {
      // prevent SIGPIPE on thread calling destructor.
      sigset_t sigsetmask;
      sigemptyset(&sigsetmask);
      sigaddset(&sigsetmask, SIGPIPE);
      pthread_sigmask(SIG_BLOCK, &sigsetmask, NULL);
      
      
      switch(mState)
      {
        case WSClient::State::MESSAGING:
          closeSocket(WebSocketProtocol::SHUTDOWN);
        case WSClient::State::HANDSHAKE:
          terminate();
        case WSClient::State::CLOSED:
        case WSClient::State::INIT:
        case WSClient::State::HANDSHAKE_FAILED:
        case WSClient::State::COMM_ERROR:
        {
          ITCSyncLock sync(mMutex);
          this->close();
        }
      }
      
      if(tls&&TLSSocket)
      {
        wolfSSL_free(TLSSocket);
        TLSSocket=nullptr;
      }
    }
    
    const int send(const uint8_t* src, const size_t len, const WebSocketProtocol::OpCode& opcode)
    {
      union{
        uint64_t  mask;
        uint8_t   byte_mask[8];
      } double_mask;
      
      uint32_t single_mask=0;
      RNG.GenerateBlock((CryptoPP::byte*)(&single_mask),sizeof(single_mask));
      double_mask.mask=(static_cast<uint64_t>(single_mask)<<32)|single_mask;
      
      std::vector<uint8_t> out;
      out.push_back(128|opcode);
      WebSocketProtocol::WS::putLength(len,out);
      out[1]=128|out[1];
      
      if((src != nullptr)&&(len!=0))
      {
        size_t offset=out.size();
        size_t payload_idx=0;

        out.resize(offset+sizeof(single_mask));

        memcpy(out.data()+offset,&single_mask,sizeof(single_mask));
        
        offset=out.size();
        out.resize(offset+len);
      
        uint64_t *src_ptr=(uint64_t*)(src);
        uint64_t *dst_ptr=(uint64_t*)(&out[offset]);

        while((payload_idx +8)<len)
        {
          *dst_ptr=(*src_ptr)^double_mask.mask;
          ++src_ptr;
          ++dst_ptr;
          offset+=8;
          payload_idx+=8;
        }

        while(payload_idx<len)
        {
          out[offset]=src[payload_idx]^double_mask.byte_mask[payload_idx%4];
          ++offset;
          ++payload_idx;
        }
 
      }
      return force_send(out);
    }
    
    const WSEvent getMessage()
    {
      return streamProcessor.getMessage();
    }
    
    void setCommError()
    {
      mState=WSClient::State::COMM_ERROR;
    }
    
    const WSClient::InputStatus handleInput()
    {
      switch(mState)
      {
        case WSClient::State::CLOSED:
        case WSClient::State::HANDSHAKE_FAILED:
        case WSClient::State::COMM_ERROR:
        case WSClient::State::INIT:
          return WSClient::InputStatus::ERROR;
        case WSClient::State::HANDSHAKE:
        {
          if(serverResponseOk())
          {
            mState=WSClient::State::MESSAGING;
            streamProcessor.setMaxMSGSize(16777216);
            if(cursor == 0)
              return WSClient::InputStatus::UPGRADED;
            else
              return WSClient::InputStatus::UPGRADED_BUFF_NOT_EMPTY;
          }
          else
          {
            mState=WSClient::State::HANDSHAKE_FAILED;
            return WSClient::InputStatus::ERROR;
          }
        }
        break;
        case WSClient::State::MESSAGING:
        {
          int received=0;
          if(cursor == 0)
          {
            received=force_recv(recvBuffer);
          }
          else
          {
            received=recvBuffer.size();
          }
          
          if(received > 0)
          {
            auto state{streamProcessor.parse(recvBuffer.data(),received,cursor)};

            switch(state.directive)
            {
              case WSStreamProcessing::Directive::MORE:
                if( cursor == 0)
                  return WSClient::InputStatus::NEED_MORE_DATA;
                else
                  return WSClient::InputStatus::CALL_ONCE_MORE;
              case WSStreamProcessing::Directive::TAKE_READY_MESSAGE:
                if(state.cursor!=static_cast<size_t>(received))
                  return WSClient::InputStatus::MESSAGE_READY_BUFFER_IS_NOT_EMPTY;
                else{
                  cursor=0;
                  return WSClient::InputStatus::MESSAGE_READY_BUFFER_IS_EMPTY;
                }
              break;
              case WSStreamProcessing::Directive::CLOSE_WITH_CODE:
                cursor=0;
                closeSocket(state.cCode);
                return WSClient::InputStatus::FORCE_CLOSE;
              case WSStreamProcessing::Directive::CONTINUE:
                cursor=state.cursor;
                return WSClient::InputStatus::CALL_ONCE_MORE;
            }
          }
          return WSClient::InputStatus::ERROR;
        }
        break;
      }
      return WSClient::InputStatus::ERROR;
    }
  };
  typedef std::shared_ptr<ClientWebSocket> ClientWebSocketSPtr;
}

#endif /* __CLIENTWEBSOCKET_H__ */

