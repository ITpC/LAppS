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
 *  $Id: WSServerMessage.h January 18, 2018 2:38 PM $
 * 
 **/


#ifndef __WSSERVERMESSAGE_H__
#  define __WSSERVERMESSAGE_H__

#include <vector>
#include <string>
#include <WSProtocol.h>

/**
 * Fast and bad implementation of server side messages.
 * To be optimized later. TODO: create header in a separate buffer, then
 * send the buffer and afterwards the payload. 
 **/
namespace WebSocketProtocol
{
  struct MakeMessageHeader
  {
    explicit MakeMessageHeader(std::vector<uint8_t>& out, const WebSocketProtocol::OpCode oc, const size_t size)
    {
      out.clear();
      out.push_back(128|oc);
      WS::putLength(size,out);
    }
    MakeMessageHeader(const MakeMessageHeader&)=delete;
    MakeMessageHeader(MakeMessageHeader&)=delete;
    MakeMessageHeader()=delete;
  };
  struct ServerMessage
  {
    ServerMessage(
      std::vector<uint8_t>& out, 
      const WebSocketProtocol::OpCode oc,
      const MSGBufferTypeSPtr& src
    ){
      out.clear();
      out.push_back(128|oc);
      WS::putLength(src->size(),out);
      size_t offset=out.size();
      out.resize(offset+src->size());
      memcpy(out.data()+offset,src->data(),src->size());
    }
    ServerMessage(
      std::vector<uint8_t>& out, 
      const WebSocketProtocol::OpCode oc,
      const char* src,
      const size_t len
    ){
      out.clear();
      out.push_back(128|oc);
      WS::putLength(len,out);
      size_t offset=out.size();
      out.resize(offset+len);
      memcpy(out.data()+offset,src,len);
    }
    ServerMessage(
      std::vector<uint8_t>& out, 
      const WebSocketProtocol::OpCode oc,
      const std::vector<uint8_t>& src
    ){
      out.clear();
      out.push_back(128|oc);
      WS::putLength(src.size(),out);
      size_t offset=out.size();
      out.resize(offset+src.size());
      memcpy(out.data()+offset,src.data(),src.size());
    }
  };
  
  /**
   * 5.5.1.  Close

   The Close frame contains an opcode of 0x8.

   The Close frame MAY contain a body (the "Application data" portion of
   the frame) that indicates a reason for closing, such as an endpoint
   shutting down, an endpoint having received a frame too large, or an
   endpoint having received a frame that does not conform to the format
   expected by the endpoint.  If there is a body, the first two bytes of
   the body MUST be a 2-byte unsigned integer (in network byte order)
   representing a status code with value /code/ defined in Section 7.4.
   Following the 2-byte integer, the body MAY contain UTF-8-encoded data
   with value /reason/, the interpretation of which is not defined by
   this specification.  This data is not necessarily human readable but
   may be useful for debugging or passing information relevant to the
   script that opened the connection.  As the data is not guaranteed to
   be human readable, clients MUST NOT show it to end users.

   Close frames sent from client to server must be masked as per
   Section 5.3.

   The application MUST NOT send any more data frames after sending a
   Close frame.

   If an endpoint receives a Close frame and did not previously send a
   Close frame, the endpoint MUST send a Close frame in response.  (When
   sending a Close frame in response, the endpoint typically echos the
   status code it received.)  It SHOULD do so as soon as practical.  An
   endpoint MAY delay sending a Close frame until its current message is
   sent (for instance, if the majority of a fragmented message is
   already sent, an endpoint MAY send the remaining fragments before
   sending a Close frame).  However, there is no guarantee that the
   endpoint that has already sent a Close frame will continue to process
   data.
   **/
struct ServerCloseMessage
{
    explicit ServerCloseMessage(
      std::vector<uint8_t>& out,
      const DefiniteCloseCode& c
    ){
      uint16_t beCode=htobe16(c);
      out.clear();
      out.push_back(128|8);
      out.push_back(2);
      out.push_back(static_cast<uint8_t>((beCode<<8)>>8));
      out.push_back(static_cast<uint8_t>(beCode>>8));
    }
    
    explicit ServerCloseMessage(
      std::vector<uint8_t>& out,
      const DefiniteCloseCode& c,
      const std::string src
    ){
      uint16_t beCode=htobe16(c);
      out.clear();
      out.push_back(128|8);
      WS::putLength(src.size()+2,out);
      out.push_back(static_cast<uint8_t>((beCode<<8)>>8));
      out.push_back(static_cast<uint8_t>(beCode>>8));
      
      size_t offset=out.size();
      out.resize(offset+src.size());
      memcpy(out.data()+offset,src.data(),src.size());
    }
  };
  
  struct ServerPongMessage
  {
    explicit ServerPongMessage(
      std::vector<uint8_t>& out,
      const MSGBufferTypeSPtr& src
    ){
      out.clear();
      out.push_back(128|10);
      WS::putLength(src->size(),out);
      size_t offset=out.size();
      out.resize(offset+src->size());
      memcpy(out.data()+offset,src->data(),src->size());
    }
  };
}
#endif /* __WSSERVERMESSAGE_H__ */

