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
 *  $Id: LAppS::ipcalc.h July 8, 2018 3:50 AM $
 * 
 **/


#ifndef __IPCALC_H__
#  define __IPCALC_H__

#include <string>
#include <memory>

#include <arpa/inet.h>

namespace LAppS
{
  struct ipcalc
  {
  private:
    static const uint32_t saddr2uint(const std::string& address)
    {
       struct in_addr dst;
       if(inet_pton(AF_INET, address.c_str(), &dst))
       {
         return dst.s_addr;
       }
       throw std::system_error(EAFNOSUPPORT,std::system_category(),"LAppS::ipcalc::saddr2uint(const std::string&) - inappropriate string address is provided as an argument: "+address);
    }

  public:
   // accepts addresses in network order only.
    static uint8_t default_ipv4_prefix(const uint32_t& address) 
    {
      uint8_t first_octet=htonl(address)>>24;
      
      if(first_octet == 0)
        return 0;
      
      // Class A
      if(first_octet<128)
      {
        return 8;
      }
      // Class B
      if((first_octet >= 128) && (first_octet < 192))
      {
        return 16;
      }
      // Class C
      if((first_octet >= 192) && (first_octet < 224))
      {
        return 24;
      }
      // Single host
      return 32;
    }
    
    static uint32_t prefix2mask(const uint8_t prefix)
    {
      return htonl(uint32_t(~((1 << (32 - prefix)) - 1)));
    }
    
    static void parse(const std::string& saddress, struct in_addr& address, uint32_t& mask)
    {
      auto pos=saddress.find('/');
      if(pos == 0)
      {
        throw std::system_error(EINVAL,std::system_category(),"LAppS::ipcalc::parse(const std::string&): inappropriate IP address is provided as an argument: "+saddress);
      }
      
      if(pos == std::string::npos) // without mask
      {
        address.s_addr=std::move(saddr2uint(saddress));
        mask=prefix2mask(default_ipv4_prefix(address.s_addr));
        return;
      }
      else // with mask
      {
        std::string sprefix=saddress.substr(pos+1);
        if(sprefix.length()>2)
          throw std::system_error(EINVAL,std::system_category(),"LAppS::ipcalc::parse(const std::string&): inappropriate prefix in "+saddress);
        
        uint8_t prefix=std::atoi(sprefix.c_str());
        
        if(prefix>32)
          throw std::system_error(EINVAL,std::system_category(),"LAppS::ipcalc::parse(const std::string&): inappropriate prefix in "+saddress);
        
        address.s_addr=std::move(saddr2uint(saddress.substr(0,pos)));
        mask=prefix2mask(prefix);
      }
    }
  };
  
  struct addrinfo
  {
    struct in_addr  address;
    uint32_t        mask;
    
    const char* c_str() const
    {
      return inet_ntoa(address);
    }
    
    const std::string cidr() const
    {
      return std::string(c_str())+'/'+std::to_string(__builtin_popcount(mask));
    }
    addrinfo(const std::string& addr)
    {
      LAppS::ipcalc::parse(addr,this->address,this->mask);
    }
    
    addrinfo(const uint32_t addr)
    {
      address.s_addr=addr;
      mask=ipcalc::prefix2mask(ipcalc::default_ipv4_prefix(address.s_addr));
    }
    
    addrinfo(const uint32_t addr,const uint32_t _mask)
    {
      address.s_addr=addr;
      mask=_mask;
    }
    
    const uint32_t network() const
    {
      return mask&address.s_addr;
    }
    
    const bool is_network() const
    {
      return (network() == address.s_addr) || ((address.s_addr >> 24) == 0);
    }
    
    const bool within(const LAppS::addrinfo& addr) const
    {
      if(is_network())
      {
        return (addr.address.s_addr&mask) == network();
      }
      else if(addr.is_network())
      {
        return (addr.mask&address.s_addr) == addr.network();
      }
      else {
        return addr.address.s_addr == address.s_addr;
      }
    }
    
    const bool operator<(const LAppS::addrinfo& _ainfo) const
    {
      return network() < (_ainfo.address.s_addr&mask);
    }
  };
  
  
}

#endif /* __IPCALC_H__ */

