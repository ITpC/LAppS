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
 *  $Id: InboundConnectionsAdapter.h January 19, 2018 3:48 PM $
 * 
 **/


#ifndef __INBOUNDCONNECTIONSADAPTER_H__
#  define __INBOUNDCONNECTIONSADAPTER_H__

#include <tsbqueue.h>
#include <abstract/IView.h>
#include <mutex>

#include <Shaker.h>
#include <InSockQueuesRegistry.h>
#include <abstract/Application.h>
#include <ThreadPoolManager.h>
#include <Singleton.h>

typedef itc::abstract::IView<itc::CSocketSPtr> SocketsView;

template <bool TLSEnable=false, bool StatsEnable=false> 
class InboundConnectionsAdapter : public SocketsView
{
private:
  typedef InSockQueuesRegistry<TLSEnable,StatsEnable> ISQRegistryDetail;
  typedef std::shared_ptr<ISQRegistryDetail>          ISQRegistry;
  typedef WebSocket<TLSEnable,StatsEnable>            WSType;
  typedef std::shared_ptr<WSType>                     WSSPtr;
  typedef itc::Singleton<itc::ThreadPoolManager>      ThreadPoolManager;
  
  
  ISQRegistry mISQR;
  
public:
 explicit InboundConnectionsAdapter(const ISQRegistry& isqr) 
 :  SocketsView(),mISQR(isqr)
 {
 }
 
 InboundConnectionsAdapter(const InboundConnectionsAdapter&)=delete;
 InboundConnectionsAdapter(InboundConnectionsAdapter&)=delete;
 
 void onUpdate(const SocketsView::value_type& indata)
 {
    ThreadPoolManager::getInstance()->enqueueRunnable(
      std::make_shared<Shaker<TLSEnable,StatsEnable>>(mISQR,indata)
    );    
 }

 virtual ~InboundConnectionsAdapter()
 {
 }
};

#endif /* __INBOUNDCONNECTIONSADAPTER_H__ */

