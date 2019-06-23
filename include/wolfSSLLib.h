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


#ifndef __WOLFSSLCONTEXT_H__
#  define __WOLFSSLCONTEXT_H__
#include <Val2Type.h>
#include <wolfssl/ssl.h>
#include <atomic>
#include <Singleton.h>
#include <Config.h>
#include <memory>

enum TLSContextType : bool { TLS_SERVER, TLS_CLIENT };

class wolfSSLLib
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
   explicit wolfSSLServerContext():wolfSSLContext(wolfSSL_CTX_new(wolfTLSv1_3_server_method()))
   { 
     
     std::string kfile{LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["key"]};
     std::string cert{LAppSConfig::getInstance()->getWSConfig()["tls_certificates"]["cert"]};
     
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
   explicit wolfSSLClientContext():wolfSSLContext({wolfSSL_CTX_new(wolfTLSv1_3_client_method())})
   { 
   }
   wolfSSLClientContext(wolfSSLClientContext&)=delete;
   wolfSSLClientContext(const wolfSSLClientContext&)=delete;
   
 };
private:
  static std::atomic<bool> initialized{false};
  
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
    if(initialized.compare_exchange_strong(test,true))
    {
      wolfSSL_Init();
    }
  }
  wolfSSLLib(wolfSSLLib&)=delete;
  wolfSSLLib(const wolfSSLLib&)=delete;
  
  ~wolfSSLLib()
  {
    bool test=true;
    if(initialized.compare_exchange_strong(test,false))
    {
      wolfSSL_Cleanup();
      
    }
  }

  auto getContext(const TLSContextType& _ctype)
  {
    itc::utils::Bool2Type<_ctype> context_type;
    return this->getContext(context_type);
  }
  
};




#endif /* __WOLFSSLCONTEXT_H__ */

