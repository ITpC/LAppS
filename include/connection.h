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
 *  $Id: connection.h January 6, 2021 12:44 PM $
 * 
 **/


#ifndef __CONNECTION_H__
#  define __CONNECTION_H__

#include <net/Socket.h>
#include <wolfSSLLib.h>
#include <Singleton.h>
#include <ePoll.h>
#include <fmt/core.h>
#include <ext/tsl/robin_map.h>


namespace LAppS
{
  enum class AppLP: uint8_t { RAW, TLS, HTTP, WS, HTTP2, HTTP3 };
  
  class connection : public itc::net::Socket
  {
   private:
    bool          mIsTLS;
    AppLP         protocol;
    WOLFSSL*      TLSSocket;
    
   public:

    // default mode - not set, blocking
    connection()
    : Socket(),    mIsTLS{false}, protocol{AppLP::RAW}, TLSSocket{nullptr} {}

    // default mode - non blocking, socket must be open and valid
    connection(int sfd, WOLFSSL_CTX* tlscontext=nullptr)
    : Socket(sfd), mIsTLS{tlscontext ? true : false},
      protocol{tlscontext ? AppLP::TLS : AppLP::RAW}
    {
      setMode(itc::net::SocketMode::SYNC);
      setSide(itc::net::Side::SERVER);
      setKeepAlive();
      if(mIsTLS)
      {
        if(!(TLSSocket=wolfSSL_new(tlscontext)))
        {
          throw std::system_error(
            errno,
            std::system_category(),
            "TLS session creation failed"
          );
        }
        wolfSSL_set_fd(TLSSocket,sfd);
      }
    }
    
    // default mode - non blocking
    connection(const char* addr, uint16_t port,WOLFSSL_CTX* tlscontext=nullptr)
    : mIsTLS{tlscontext ? true: false}, 
      protocol{tlscontext ? AppLP::TLS : AppLP::RAW}
    {
      open<itc::net::Transport::TCP,itc::net::Side::CLIENT>(addr,port);
      setMode(itc::net::SocketMode::SYNC);
      if(mIsTLS)
      {
        if(!(TLSSocket=wolfSSL_new(tlscontext)))
        {
          throw std::system_error(
            errno,
            std::system_category(),
            "TLS session creation failed"
          );
        }
        wolfSSL_set_fd(TLSSocket,getfd());
      }
    }
    connection(const connection&)=delete;
    connection(const connection&&)=delete;
    connection(connection&)=delete;
    connection(connection&&)=delete;
    
    const bool isTLS() const
    {
      return mIsTLS;
    }
    
    const auto getType() const
    {
      return protocol;
    }
    
    /**
     * @brief - returns 0 if poll required, -1 on error, 
     * or amount of data received
     * 
     **/
    const ssize_t rx(void* buff, size_t len)
    {
      switch(protocol)
      {
        case AppLP::RAW:
            return read(buff,len);
        case AppLP::TLS:
          return tls_read(buff,len);
        case AppLP::HTTP:
        case AppLP::WS:
        case AppLP::HTTP2:
          if(isTLS())
          {
            return tls_read(buff,len);
          }
          else
          {
            return read(buff,len);
          }
        case AppLP::HTTP3:
          throw std::runtime_error("HTTP3 is not supported yet, udp recv_from ...");
        default:
          throw std::runtime_error("should not come to this state, blame the programmer.");
      }
    }
    
    /**
     * @brief - returns 0 if poll required, -1 on error, 
     * or amount of data received
     * 
     **/
    const long int tx(const void* buff, size_t len)
    {
      switch(protocol)
      {
        case AppLP::RAW:
            return write(buff,len);
        case AppLP::TLS:
          return tls_write(buff,len);
        case AppLP::HTTP:
        case AppLP::WS:
        case AppLP::HTTP2:
          if(isTLS())
          {
            return tls_write(buff,len);
          }
          else
          {
            return write(buff,len);
          }
        case AppLP::HTTP3:
          throw std::runtime_error("HTTP3 is not supported yet, udp recv_from ...");
        default:
          throw std::runtime_error("should not come to this state, blame the programmer.");
      }
    }
    
    virtual ~connection()
    {
      if(isTLS()&&TLSSocket)
      {
        wolfSSL_free(TLSSocket);
        TLSSocket=nullptr;
      }
      this->close();
    }
    
   private:
    void upgrade(const AppLP lvl)
    {
      if(lvl>protocol)
      {
        protocol=lvl;
      }
    }
    
    const long int read(void* buff, const size_t len)
    {
      const auto ret=recv(buff,len);
      switch(ret)
      {
        case 0:
          return -1; // EOF - peer closed connection
        case -1:
          if((errno == EAGAIN)||(errno == EWOULDBLOCK))
          {
            return 0;
          }
        default:
          return ret;
      }
    }
    
    const ssize_t write(const void* buff,const size_t len)
    {
      ssize_t cursor=0;
      do
      {
        const ssize_t result{send(buff+cursor,len-cursor)};

        if(result == -1)
        {
          if((errno == EAGAIN)||(errno == EWOULDBLOCK))
            continue;
          return -1;
        }

        cursor+=result;

      } while(cursor != len);
      return cursor;
    }
    
    const ssize_t tls_read(void* buff, const size_t len)
    {
      do{
        const auto ret=wolfSSL_read(TLSSocket,buff,len);
        if(ret > 0)
          return ret;
        
//        if(ret == 0)
//          return -1;

        auto error=wolfSSL_get_error(TLSSocket,ret);
        switch(error)
        {
          case SSL_ERROR_WANT_READ:
            return 0;
          case SSL_ERROR_WANT_WRITE:
            continue;
          default:
          {
            char buffer[80];
            wolfSSL_ERR_error_string(error,buffer);
            throw std::runtime_error(
              fmt::format(
                "In {}:{}, wolfSSL error at connection::tls_read(): {}",
                __FILE__,__LINE__,buffer
              )
            );
          }
          return -1;
        }
      }while(true);
    }
    
    
    const ssize_t tls_write(const void* buff, const size_t len)
    {
      ssize_t cursor=0;
      do{
        int ret=wolfSSL_write(TLSSocket,buff+cursor,len-cursor);

 //       if(ret == 0)
 //         return -1;
        
        if(ret <= 0)
        {
          auto error=wolfSSL_get_error(TLSSocket,ret);
          switch(error)
          {
            case SSL_ERROR_WANT_READ:
              return 0;
            case SSL_ERROR_WANT_WRITE:
              continue;
            default:
            {
              char buffer[80];
              wolfSSL_ERR_error_string(error,buffer);
              throw std::runtime_error(
                fmt::format(
                  "In {}:{}, wolfSSL error at connection::tls_write(): {}",
                  __FILE__,__LINE__,buffer
                )
              );
            }
            return -1;
          }
        }
        cursor+=ret;
      }while(cursor != len);
      return cursor;
    }    
  };
}


#endif /* __CONNECTION_H__ */

