/*
 * Copyright (C) 2018 Pavel Kraynyukhov <pk@new-web.co>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id:  abstract/WebSocket.h June 4, 2018, 6:57 AM $
 */


#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include <vector>
#include <queue>
#include <WSEvent.h>
namespace abstract
{
  class WebSocket : public std::enable_shared_from_this<abstract::WebSocket>
  {
  public:
    enum State { HANDSHAKE=0, MESSAGING=1, CLOSED=2 };
    
    WebSocket()=default;
    
    virtual const int send(const std::vector<uint8_t>&)=0;
    virtual const State getState() const=0;
    virtual const bool mustAutoFragment() const=0;
    virtual std::shared_ptr<abstract::WebSocket> get_shared()=0;
    virtual void returnBuffer(std::remove_reference<const std::shared_ptr<MSGBufferType>&>::type)=0;
    virtual void close()=0;
  protected:
    virtual ~WebSocket()=default;
  };
}

#endif /* WEBSOCKET_H */

