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
 *  $Id: NetworkACL.h July 8, 2018 12:15 PM $
 * 
 **/


#ifndef __NETWORKACL_H__
#  define __NETWORKACL_H__

#include <ipcalc.h>
#include <Singleton.h>
#include <set>

namespace LAppS
{
  enum Network_ACL_Policy: uint8_t { DENY, ALLOW };
  
  struct NetworkACL
  {
    Network_ACL_Policy mPolicy;
    std::set<LAppS::addrinfo> mExcludeNetworks;
    std::set<LAppS::addrinfo> mExcludeAddresses;
    
    explicit NetworkACL(const Network_ACL_Policy _policy)
    : mPolicy(_policy){}
    
    void add(const LAppS::addrinfo& address)
    {
      if(address.is_network())
        mExcludeNetworks.emplace(std::move(address));
      else
        mExcludeAddresses.emplace(std::move(address));
    }
    
    void del(const LAppS::addrinfo& address)
    {
      if(address.is_network())
      {
        auto it=mExcludeNetworks.find(address);
        if(it!=mExcludeNetworks.end())
          mExcludeNetworks.erase(it);
      }else{
        auto it=mExcludeAddresses.find(address);
        if(it!=mExcludeAddresses.end())
          mExcludeAddresses.erase(it);
      }
    }
    
    const bool match(const uint32_t address) const
    {
      const LAppS::addrinfo tmp(address);
      
      auto it=mExcludeNetworks.lower_bound(tmp);
      if(it!=mExcludeNetworks.end())
        return it->within(tmp);

      it=mExcludeAddresses.find(tmp);
      return (it!=mExcludeAddresses.end());
    }
    
    const bool match(const addrinfo& address) const
    {
        auto it=mExcludeNetworks.lower_bound(address);
        if(it!=mExcludeNetworks.end())
          return it->within(address);
        
        it=mExcludeAddresses.find(address);
        return (it!=mExcludeAddresses.end());
    }
  };
  
  typedef itc::Singleton<NetworkACL> GlobalNACL;
}

#endif /* __NETWORKACL_H__ */

