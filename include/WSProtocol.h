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
 *  $Id: WSProtocol.h January 18, 2018 2:39 PM $
 * 
 **/


#ifndef __WSPROTOCOL_H__
#  define __WSPROTOCOL_H__

#include <vector>
#include <endian.h>

/**
 * RFC 6455                 The WebSocket Protocol            December 2011


   Opcode:  4 bits

      Defines the interpretation of the "Payload data".  If an unknown
      opcode is received, the receiving endpoint MUST _Fail the
      WebSocket Connection_.  The following values are defined.

      *  %x0 denotes a continuation frame

      *  %x1 denotes a text frame

      *  %x2 denotes a binary frame

      *  %x3-7 are reserved for further non-control frames

      *  %x8 denotes a connection close

      *  %x9 denotes a ping

      *  %xA denotes a pong

      *  %xB-F are reserved for further control frames

 **/
namespace WebSocketProtocol
{
  enum OpCode : uint8_t { 
    CONTINUE=0, TEXT=1, BINARY=2, 
    NCRESERVED1=3, NCRESERVED2=4, NCRESERVED3=5, 
    NCRESERVED4=6, NCRESERVED5=7, 
    CLOSE=8, PING=9,PONG=10,
    CFRSV1=11,CFRSV2=12,CFRSV3=13,CFRSV4=14,CFRSV5=15
  };
}

/**
 * 7.4.1.  Defined Status Codes

   Endpoints MAY use the following pre-defined status codes when sending
   a Close frame.

   1000

      1000 indicates a normal closure, meaning that the purpose for
      which the connection was established has been fulfilled.

   1001

      1001 indicates that an endpoint is "going away", such as a server
      going down or a browser having navigated away from a page.

   1002

      1002 indicates that an endpoint is terminating the connection due
      to a protocol error.

   1003

      1003 indicates that an endpoint is terminating the connection
      because it has received a type of data it cannot accept (e.g., an
      endpoint that understands only text data MAY send this if it
      receives a binary message).


Fette & Melnikov             Standards Track                   [Page 45]
 
RFC 6455                 The WebSocket Protocol            December 2011


   1004

      Reserved.  The specific meaning might be defined in the future.

   1005

      1005 is a reserved value and MUST NOT be set as a status code in a
      Close control frame by an endpoint.  It is designated for use in
      applications expecting a status code to indicate that no status
      code was actually present.

   1006

      1006 is a reserved value and MUST NOT be set as a status code in a
      Close control frame by an endpoint.  It is designated for use in
      applications expecting a status code to indicate that the
      connection was closed abnormally, e.g., without sending or
      receiving a Close control frame.

   1007

      1007 indicates that an endpoint is terminating the connection
      because it has received data within a message that was not
      consistent with the type of the message (e.g., non-UTF-8 [RFC3629]
      data within a text message).

   1008

      1008 indicates that an endpoint is terminating the connection
      because it has received a message that violates its policy.  This
      is a generic status code that can be returned when there is no
      other more suitable status code (e.g., 1003 or 1009) or if there
      is a need to hide specific details about the policy.

   1009

      1009 indicates that an endpoint is terminating the connection
      because it has received a message that is too big for it to
      process.

   1010

      1010 indicates that an endpoint (client) is terminating the
      connection because it has expected the server to negotiate one or
      more extension, but the server didn't return them in the response
      message of the WebSocket handshake.  The list of extensions that
 
Fette & Melnikov             Standards Track                   [Page 46]
 
RFC 6455                 The WebSocket Protocol            December 2011


      are needed SHOULD appear in the /reason/ part of the Close frame.
      Note that this status code is not used by the server, because it
      can fail the WebSocket handshake instead.

   1011

      1011 indicates that a server is terminating the connection because
      it encountered an unexpected condition that prevented it from
      fulfilling the request.

   1015

      1015 is a reserved value and MUST NOT be set as a status code in a
      Close control frame by an endpoint.  It is designated for use in
      applications expecting a status code to indicate that the
      connection was closed due to a failure to perform a TLS handshake
      (e.g., the server certificate can't be verified).

7.4.2.  Reserved Status Code Ranges

   0-999

      Status codes in the range 0-999 are not used.

   1000-2999

      Status codes in the range 1000-2999 are reserved for definition by
      this protocol, its future revisions, and extensions specified in a
      permanent and readily available public specification.

   3000-3999

      Status codes in the range 3000-3999 are reserved for use by
      libraries, frameworks, and applications.  These status codes are
      registered directly with IANA.  The interpretation of these codes
      is undefined by this protocol.

   4000-4999

      Status codes in the range 4000-4999 are reserved for private use
      and thus can't be registered.  Such codes can be used by prior
      agreements between WebSocket applications.  The interpretation of
      these codes is undefined by this protocol.
 **/
namespace WebSocketProtocol
{
  // ignoring all reserved, use of those on the stream
  // will result in "Close" response with code PROTOCOL_ERROR
  enum DefiniteCloseCode : uint16_t {
    NORMAL = 1000, SHUTDOWN = 1001,
    PROTOCOL_VIOLATION=1002, NO_COMPRENDE=1003,
    BAD_DATA=1007, POLICY_VIOLATION=1008,
    MESSAGE_TOO_BIG=1009, UNSUPPORTED_EXTENSION=1010, 
    UNEXPECTED_CONDITION=1011, RESERVED1=2999, RESERVED2=5000
  };  
}

/**
 *    0                   1                   2                   3
      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     +-+-+-+-+-------+-+-------------+-------------------------------+
     |F|R|R|R| opcode|M| Payload len |    Extended payload length    |
     |I|S|S|S|  (4)  |A|     (7)     |             (16/64)           |
     |N|V|V|V|       |S|             |   (if payload len==126/127)   |
     | |1|2|3|       |K|             |                               |
     +-+-+-+-+-------+-+-------------+ - - - - - - - - - - - - - - - +
     |     Extended payload length continued, if payload len == 127  |
     + - - - - - - - - - - - - - - - +-------------------------------+
     |                               |Masking-key, if MASK set to 1  |
     +-------------------------------+-------------------------------+
     | Masking-key (continued)       |          Payload Data         |
     +-------------------------------- - - - - - - - - - - - - - - - +
     :                     Payload Data continued ...                :
     + - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - +
     |                     Payload Data continued ...                |
     +---------------------------------------------------------------+
**/
namespace WebSocketProtocol
{
  enum PLLength { MIN=0, SHORT=2, LONG=8 };
  
  union MASKType {
    uint32_t intMASK;
    uint8_t byteMASK[4];
  };
  
  struct WS
  {
    static const PLLength getSizeType(const uint8_t secondByte)
    {
      switch(secondByte&127)
      {
        case 127:
          return LONG;
        case 126:
          return SHORT;
        default:
          return MIN;
      }
    }
    static const bool isOpCodeSupported(const uint8_t opcode)
    {
      // Supported opcodes: CONTINUE=0, TEXT=1, BINARY=2,  CLOSE=8, PING=9,PONG=10
      return ( (opcode == 0) || (opcode == 1) || (opcode == 2) || 
          (opcode == 8) || (opcode == 9) || (opcode == 10)
        );
    }
    static const bool isFIN(const uint8_t firstByte)
    {
      return firstByte&128;
    }
    static const bool isMASKED(const uint8_t secondByte)
    {
      return secondByte&128;
    }
    constexpr uint8_t MASKLength() const
    {
      return 4;
    }
    static void putLength(const size_t pllength, std::vector<uint8_t>& out)
    {
      if(pllength > std::numeric_limits<uint16_t>::max())
      {
        uint64_t sz=htobe64(pllength);
        const uint8_t *ptr=reinterpret_cast<const uint8_t*>(&sz);
        
        out.push_back(127); // put second byte
        
        for(auto i=0;i<8;++i) // put 8 bytes of payload length
        {
          out.push_back(ptr[i]);
        }
      }
      else if(pllength>125)
      {
        const uint16_t sz=htobe16(pllength);
        const uint8_t *ptr=reinterpret_cast<const uint8_t*>(&sz);
        out.push_back(126); // size code 126 2 bytes pl
        out.push_back(ptr[0]);
        out.push_back(ptr[1]);
      }else out.push_back(pllength);
    }
    
    static void putMASK(const MASKType& mask, std::vector<uint8_t>& out)
    {
      for(auto i=0;i<4;++i)
      {
        out.push_back(mask.byteMASK[i]);
      }
    }
    
    static const uint32_t MASK2HOST(const uint32_t be32)
    {
      return be32toh(be32);
    }
  };
}


#endif /* __WSPROTOCOL_H__ */

