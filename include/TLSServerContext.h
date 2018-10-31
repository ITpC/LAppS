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
 *  $Id: TLSServerContext.h January 29, 2018 12:27 PM $
 * 
 **/


#ifndef __TLSSERVERCONTEXT_H__
#  define __TLSSERVERCONTEXT_H__

// c++
#include <system_error>
#include <memory>

// libressl
#include <tls.h>
// libcrypt
#include <openssl/crypto.h>

// LAppS
#include <Config.h>

// ITCLib/ITCFramework
#include <Singleton.h>
#include <sys/mutex.h>

typedef void (*locking_function)(int mode, int n, const char *file,	int line);

void openssl_crypt_locking_function_callback(int mode, int n, const char* file, const int line)
{
  static std::vector<std::mutex> locks(CRYPTO_num_locks());
  if(n>=static_cast<int>(locks.size()))
  {
    abort();
  }
  
  if(mode & CRYPTO_LOCK)
    locks[n].lock();
  else
    locks[n].unlock();
}


namespace TLS
{
  class ServerContext
  {
   private:
    tls_config* TLSConfig;
    tls*        TLSContext;
   public:
    
    ServerContext(const ServerContext&)=delete;
    ServerContext(ServerContext&)=delete;
    
    ~ServerContext()
    {
      tls_close(TLSContext);
      tls_free(TLSContext);
      tls_config_free(TLSConfig);
    }
    
    explicit ServerContext()
    {
      bool tlsEnabled=LAppSConfig::getInstance()->getWSConfig()["tls"];
      if(tlsEnabled)
      {
        CRYPTO_set_locking_callback(openssl_crypt_locking_function_callback);
        CRYPTO_set_id_callback(pthread_self);
        if(tls_init())
        {
          throw std::system_error(errno,std::system_category(),"TLS initialization has failed");
        }

        if(!(TLSConfig=tls_config_new()))
        {
          throw std::system_error(errno,std::system_category(),"TLS new configuration can not be instantiated.");
        }

        if(!(TLSContext=tls_server()))
        {
          throw std::system_error(errno,std::system_category(),"TLS creating server context has failed.");
        }

        uint32_t protocols=0;

        if(tls_config_parse_protocols(&protocols, "secure"))
        {
          throw std::system_error(errno,std::system_category(),"TLS 'secure' protocols parsing has failed.");
        }
        if(tls_config_set_protocols(TLSConfig, protocols))
        {
          throw std::system_error(errno,std::system_category(),"TLS: can't set protocols.");
        }
        if(tls_config_set_ciphers(TLSConfig,"secure"))
        {
          throw std::system_error(errno,std::system_category(),"TLS: can't set secure ciphers.");
        }
        if(tls_config_set_ecdhecurves(TLSConfig,"default"))
        {
          throw std::system_error(errno,std::system_category(),"TLS: can't set default ecdhcurves.");
        }

        tls_config_prefer_ciphers_server(TLSConfig);


        std::string certificate_key_file=LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["key"];

        if(tls_config_set_key_file(TLSConfig,certificate_key_file.c_str()))
        {
          throw std::system_error(errno,std::system_category(),"TLS: can't load certificate key file (PEM).");
        }

        std::string certificate_file=LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["cert"];

        if(tls_config_set_cert_file(TLSConfig,certificate_file.c_str()))
        {
          throw std::system_error(errno,std::system_category(),"TLS: can't load certificate file (PEM).");
        }

        if(tls_configure(TLSContext, TLSConfig))
        {
          throw std::system_error(errno,std::system_category(),"TLS: Context configuration has failed (PEM).");
        } 
      }
    }
    
    tls* getContext()
    {
      return TLSContext;
    }
  };
  typedef itc::Singleton<ServerContext>  SharedServerContext;
}


#endif /* __TLSSERVERCONTEXT_H__ */

