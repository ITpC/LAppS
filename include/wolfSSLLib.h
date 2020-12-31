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
 *  $Id: wolfSSLContext.h June 23, 2019 2:10 AM $
 * 
 **/


#ifndef __WOLFSSLLIB_H__
#  define __WOLFSSLLIB_H__

#include <Val2Type.h>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/error-crypt.h>
#include <atomic>
#include <Singleton.h>
#include <Config.h>
#include <memory>


 #define logWOLFSSLError(x,y)\
      char buffer[80];\
      auto err=wolfSSL_get_error(TLSSocket,x);\
      ITC_ERROR(__FILE__,__LINE__,y" %d - {}", err, wolfSSL_ERR_error_string(err,buffer));
      

enum TLSContextType : bool { TLS_SERVER, TLS_CLIENT };

static std::atomic<bool> wolfSSLInitialized{false};

template <TLSContextType CONTEXT_TYPE> class wolfSSLLib
{
public:
 class wolfSSLContext
 {
  private:
   WOLFSSL_CTX* mContext;
  public:
   explicit wolfSSLContext(WOLFSSL_CTX* _ctx) : mContext{_ctx}
   {
     if(!mContext)
       throw std::system_error(EFAULT, std::system_category(), "wolfSSLContext(CTX), - CTX may not be NULL");
   };
   wolfSSLContext(wolfSSLContext&)=delete;
   wolfSSLContext(const wolfSSLContext&)=delete;
   
   auto raw_context()
   {
     return mContext;
   }
   
   ~wolfSSLContext()
   {
     wolfSSL_CTX_free(mContext);
   }
 };
 
 class wolfSSLServerContext : public wolfSSLContext
 {
 private:
  const size_t getConfig_TLS_server_version()
  {
    auto it=LAppSConfig::getInstance()->getWSConfig().find(std::string("tls_server_version"));
    if(it!=LAppSConfig::getInstance()->getWSConfig().end())
    {
      size_t version=it.value();
      if(version == 3 || version == 4)
        return version;
    }
    return 4;
  }
 public:
   explicit wolfSSLServerContext():wolfSSLContext(
    getConfig_TLS_server_version() == 3 ? 
      wolfSSL_CTX_new(wolfTLSv1_2_server_method()) : 
      wolfSSL_CTX_new(wolfTLSv1_3_server_method())
   )
   { 
     std::string ca{LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["ca"]};
     std::string kfile{LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["key"]};
     std::string cert{LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["cert"]};
     
     auto ret=wolfSSL_CTX_load_verify_locations(this->raw_context(),ca.c_str(),nullptr);
     
     std::string errmsg("Error on verification of the CA file ["+ca+"]: ");
     
     if( ret != SSL_SUCCESS)
     {
       switch(ret)
       {
         case SSL_FAILURE:
           errmsg.append("ctx is NULL, or if both file and path are NULL");
           break;
         case SSL_BAD_FILETYPE:
           errmsg.append("the file is in the wrong format");
           break;
         case SSL_BAD_FILE:
           errmsg.append("file doesn’t exist, can’t be read, or is corrupted");
           break;
         case MEMORY_E:
           errmsg.append("out of memory");
           break;
         case ASN_INPUT_E:
           errmsg.append("Base16 decoding fails on the file");
           break;
         case ASN_BEFORE_DATE_E:
           errmsg.append("current date is before the `before' date, this CA file is not yet valid");
           break;
         case ASN_AFTER_DATE_E:
           errmsg.append("current date is after the `after' date, this CA file is already invalid");
           break;
         case BUFFER_E:
           errmsg.append("the chain buffer is bigger than the receiving buffer");
           break;
         case BAD_PATH_ERROR:
           errmsg.append("opendir() fails to open path");
           break;
         default:
           errmsg.append("unknown error");
           break;
       }
       
       ITC_ERROR(__FILE__,__LINE__,"{}",errmsg.c_str());
       
       throw std::system_error(EINVAL,std::system_category(),errmsg);
     }
     
     if(wolfSSL_CTX_use_PrivateKey_file(this->raw_context(),kfile.c_str(), SSL_FILETYPE_PEM) == SSL_SUCCESS)
     {
       if(wolfSSL_CTX_use_certificate_file(this->raw_context(),cert.c_str(),SSL_FILETYPE_PEM) != SSL_SUCCESS)
       {
         throw std::system_error(ENOENT,std::system_category(),cert);
       }
     }
     else
     {
       throw std::system_error(ENOENT,std::system_category(),kfile);
     }
   }
   wolfSSLServerContext(wolfSSLServerContext&)=delete;
   wolfSSLServerContext(const wolfSSLServerContext&)=delete;
 };
 
 class wolfSSLClientContext : public wolfSSLContext
 {
 private:
  const size_t getConfig_TLS_client_version()
  {
    auto it=LAppSConfig::getInstance()->getWSConfig().find(std::string("tls_client_version"));
    if(it!=LAppSConfig::getInstance()->getWSConfig().end())
    {
      size_t version=it.value();
      if(version == 3 || version == 4)
        return version;
    }
    return 4;
  }
 public:
   explicit wolfSSLClientContext()
   : wolfSSLContext(
     getConfig_TLS_client_version() == 3 ? 
       wolfSSL_CTX_new(wolfTLSv1_2_client_method()) : 
       wolfSSL_CTX_new(wolfTLSv1_3_client_method())
   )
   { 
     std::string ca{LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["ca"]};
     
     if(wolfSSL_CTX_load_verify_locations(this->raw_context(),ca.c_str(),nullptr) != SSL_SUCCESS)
     {
       throw std::system_error(ENOENT,std::system_category(),ca);
     }
   }
   wolfSSLClientContext(wolfSSLClientContext&)=delete;
   wolfSSLClientContext(const wolfSSLClientContext&)=delete;   
 };
private:
  std::shared_ptr<wolfSSLContext> getContext(itc::utils::Bool2Type<TLS_SERVER> fictive)
  {
    return std::make_shared<wolfSSLServerContext>();
  }
  
  std::shared_ptr<wolfSSLContext> getContext(itc::utils::Bool2Type<TLS_CLIENT> fictive)
  {
    return std::make_shared<wolfSSLClientContext>();
  }  
  
public:
 
  explicit wolfSSLLib()
  {
    bool test=false;
    if(wolfSSLInitialized.compare_exchange_strong(test,true))
    {
      wolfSSL_Init();
    }
  }
  wolfSSLLib(wolfSSLLib&)=delete;
  wolfSSLLib(const wolfSSLLib&)=delete;
  
  ~wolfSSLLib()
  {
    bool test=true;
    if(wolfSSLInitialized.compare_exchange_strong(test,false))
    {
      wolfSSL_Cleanup();
    }
  }

  constexpr auto getContext()
  {
    return this->getContext(itc::utils::Bool2Type<CONTEXT_TYPE>());
  }
  
};

typedef itc::Singleton<wolfSSLLib<TLS_SERVER>> wolfSSLServer;
typedef itc::Singleton<wolfSSLLib<TLS_CLIENT>> wolfSSLClient;


#endif /* __WOLFSSLLIB_H__ */

