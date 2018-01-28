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
 *  $Id: InboundConnectionsPool.h January 19, 2018 3:48 PM $
 * 
 **/


#ifndef __INBOUNDCONNECTIONSPOOL_H__
#  define __INBOUNDCONNECTIONSPOOL_H__

#include <tsbqueue.h>
#include <abstract/IView.h>
#include <mutex>

class InboundConnections : public itc::abstract::IView<itc::CSocketSPtr>
{
private:
  itc::tsbqueue<itc::CSocketSPtr> mInbound;
public:
 
 void onUpdate(const itc::abstract::IView<itc::CSocketSPtr>::value_type& indata)
 {
   mInbound.send(indata);
 }

 const bool empty()
 {
   return mInbound.empty();
 }
 
 const bool size(int& value)
 {
   return mInbound.size(value);
 }
 
 void recv(itc::abstract::IView<itc::CSocketSPtr>::value_type& sock)
 {
   mInbound.recv(sock);
 }
 const bool tryRecv(itc::abstract::IView<itc::CSocketSPtr>::value_type& sock,const ::timespec& timeout)
 {
   return mInbound.tryRecv(sock,timeout);
 }
 void destroy()
 {
   mInbound.destroy();
 }
 virtual ~InboundConnections()
 {
   destroy();
 }
};

typedef std::shared_ptr<InboundConnections> InboundConnectionsSPtr;
typedef std::weak_ptr<InboundConnections> WeakInboundConnectionsSPtr;

#endif /* __INBOUNDCONNECTIONSPOOL_H__ */

