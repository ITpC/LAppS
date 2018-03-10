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

#include <Shaker.h>
#include <InSockQueuesRegistry.h>

typedef itc::abstract::IView<itc::CSocketSPtr> SocketsView;

template <bool TLSEnable=false, bool StatsEnable=false> 
class InboundConnections : public SocketsView
{
private:
  typedef std::shared_ptr<InSockQueuesRegistry<TLSEnable,StatsEnable>> ISQRegistry;
  typedef WebSocket<TLSEnable,StatsEnable> WSType;
  typedef std::shared_ptr<WSType>          WSSPtr;
  
  Shaker<TLSEnable,StatsEnable> mShaker;
  
public:
 explicit InboundConnections(const ISQRegistry& isqr) 
 :  SocketsView(),mShaker(isqr)
 {
 }
 
 InboundConnections(const InboundConnections&)=delete;
 InboundConnections(InboundConnections&)=delete;
 
 void onUpdate(const SocketsView::value_type& indata)
 {
   mShaker.shakeTheHand(indata);
 }

 virtual ~InboundConnections()
 {
 }
};

#endif /* __INBOUNDCONNECTIONSPOOL_H__ */

