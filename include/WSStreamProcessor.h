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
 *  $Id: WSStreamProcessor.h January 17, 2018 11:44 PM $
 * 
 **/


#ifndef __WSSTREAMPARSER_H__
#  define __WSSTREAMPARSER_H__

#include <vector>
#include <memory>
#include <exception>
#include <stack>

#include <WSProtocol.h>
#include <WSEvent.h>
#include <queue>

/**
 * replace message and messageFrames allocation with the ring-buffer acquire of
 * next available or new message/frame-buffer.
 **/

namespace WSStreamProcessing
{
  enum State : uint8_t {INIT, HEADER,SIZE_MIN,SIZE_SHORT,SIZE_LARGE,LOAD_MASK,PAYLOAD, DONE, MESSAGE_READY };
  enum PLSizeFieldSize : uint8_t { MIN=0, SHORT=2, LONG=8 };
  enum Directive : uint8_t { MORE, TAKE_READY_MESSAGE, CLOSE_WITH_CODE };
  enum Frame : uint8_t { SINGLE_FRAME, FIRST_FRAME, MIDDLE_FRAME, LAST_FRAME };
  
  struct Result
  {
    size_t cursor;
    Directive directive;
    WebSocketProtocol::DefiniteCloseCode cCode;
    Result(const Result& ref)
    : cursor(ref.cursor),directive(ref.directive),
      cCode(ref.cCode)
    {
    }
    Result(const size_t _cursor, const Directive _directive, const WebSocketProtocol::DefiniteCloseCode _code)
    :cursor(_cursor),directive(_directive),cCode(_code){}
    Result& operator=(const Result& ref)
    {
      cursor=ref.cursor;
      directive=ref.directive;
      cCode=ref.cCode;
      return (*this);
    }
  };
}

class WSStreamParser
{
private:

/**===========================================================**/
 
  WSStreamProcessing::State state;
  WebSocketProtocol::OpCode currentOpCode;
  WSStreamProcessing::Frame frame;
  
  
  uint8_t headerBytesReady;
  uint8_t sizeBytesReady;
  uint8_t maskBytesReady;
  
  uint8_t headerBytes[2];
  uint8_t optionalSizeBytes[8];
  
  size_t cursor;
  
  uint64_t messageSize;
  uint64_t PLReadyBytes;
  
  std::stack<WebSocketProtocol::OpCode> opcodes;
  MSGBufferTypeSPtr message;
  MSGBufferTypeSPtr messageFrames;
  
  uint8_t MASK[4];
  
  
  
/**===========================================================**/
  
  const bool isFIN() const
  {
    return headerBytes[0]&128;
  }
  
  const bool isMASKED() const
  {
    return headerBytes[1]&128;
  }

  
/**===========================================================**/
  
 public:  
  explicit WSStreamParser() : state(WSStreamProcessing::INIT), 
    currentOpCode(WebSocketProtocol::CFRSV5), 
    frame(WSStreamProcessing::SINGLE_FRAME), headerBytesReady(0), 
    sizeBytesReady(0), maskBytesReady(0), headerBytes{0}, optionalSizeBytes{0}, 
    cursor(0), messageSize(0), PLReadyBytes(0), opcodes(), 
    message(std::make_shared<std::vector<uint8_t>>()),
    messageFrames(std::make_shared<std::vector<uint8_t>>()),MASK{0}
  {
    message->reserve(512);
    messageFrames->reserve(512);
  }
  
  WSStreamParser(const WSStreamParser&)=delete;
  WSStreamParser(WSStreamParser&)=delete;
  
  
  const WSStreamProcessing::Result parse(const uint8_t* stream,const size_t limit, const size_t offset=0, const int fd=0)
  {
    switch(state)
    {
      case WSStreamProcessing::INIT:
        cursor=offset;
        headerBytesReady=0;
        sizeBytesReady=0;
        maskBytesReady=0;
        PLReadyBytes=0;
        messageSize=0;
        memset(optionalSizeBytes,0,8);
        memset(MASK,0,4);
        state=WSStreamProcessing::HEADER;
        currentOpCode=WebSocketProtocol::CFRSV5;
        frame=WSStreamProcessing::SINGLE_FRAME;
        return parse(stream,limit,cursor,fd);
      case WSStreamProcessing::HEADER:
        cursor=offset;
        while((cursor<limit)&&(headerBytesReady<2))
        {
          headerBytes[headerBytesReady]=stream[cursor];
          ++headerBytesReady;++cursor;
        }
        
        if(headerBytesReady==2)
        { 
          /**
           * Fail any RSV so far. No extensions are supported yet.
           **/
          uint64_t RSV=static_cast<uint64_t>((headerBytes[0]&0x70)>>4);
          
          if(RSV>0)
          {
            return { 
              cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
              WebSocketProtocol::PROTOCOL_VIOLATION
            };
          } 
          
          /**
           * Fail connections with unmasked data
           **/
          
          if(!isMASKED()) 
          {
            return { 
              cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
              WebSocketProtocol::PROTOCOL_VIOLATION
            };
          }

          currentOpCode=static_cast<WebSocketProtocol::OpCode>(headerBytes[0]&0x0F);
          
          /**
           * Fail connection on unsupported opcodes.
           **/
          if(!WebSocketProtocol::WS::isOpCodeSupported(currentOpCode))
          {
            return { 
              cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
              WebSocketProtocol::NO_COMPRENDE
            };
          }
          
          
          /* Detecting the frame type */
          bool controlOpCode=(
            (currentOpCode == WebSocketProtocol::CLOSE)||
            (currentOpCode == WebSocketProtocol::PING)||
            (currentOpCode == WebSocketProtocol::PONG)
          );
          
          bool controlFrame=(isFIN() && controlOpCode);
          
          bool badControlFrame = (!isFIN()) && controlOpCode;
          
          /**
           * 5.4.  Fragmentation:
           * 
           * FIN(1) OpCode is not 0 - SINGLE FRAME
           * FIN(0) OpCode(1,2)- START OF A FRAGMENTED MESSAGE
           * FIN(1) OpCode(0) - END OF A FRAGMENTED MESSAGE
           * 
           * Control frames (see Section 5.5) MAY be injected in the middle of
           * a fragmented message.  Control frames themselves MUST NOT be
           * fragmented.
           * 
           * 
           * The fragments of one message MUST NOT be interleaved between the
           * fragments of another message unless an extension has been
           * negotiated that can interpret the interleaving.
           * 
           **/
          
          /**
           * Fail fragmented control frames
           **/
          if(badControlFrame)
          {
            return { 
              cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
              WebSocketProtocol::PROTOCOL_VIOLATION
            };
          }
          
          /** ===========  detect fragmentation ===========**/
          
          bool single_frame=(
            isFIN() && (
              (currentOpCode == WebSocketProtocol::TEXT)||
              (currentOpCode == WebSocketProtocol::BINARY)
            )
          );
          
          if(single_frame||controlFrame)
          {
            if((!controlFrame)&&(!opcodes.empty()))
            {
              return { 
                cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
                WebSocketProtocol::PROTOCOL_VIOLATION
              };
            }
            frame=WSStreamProcessing::SINGLE_FRAME;
          }else {
            bool first_frame=(
              (!isFIN()) && (
                (currentOpCode == WebSocketProtocol::TEXT)||
                (currentOpCode == WebSocketProtocol::BINARY)
              )
            );
            if(first_frame)
            {
              frame=WSStreamProcessing::FIRST_FRAME;
              opcodes.push(currentOpCode); // save the opcode for later
            }else{
              bool middle_frame=(
                (!isFIN()) &&(currentOpCode == WebSocketProtocol::CONTINUE)
              );
              if(middle_frame)
              {
                if(opcodes.empty()) 
                { // there was no beginning to this continuation
                  return { 
                      cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
                      WebSocketProtocol::PROTOCOL_VIOLATION
                    };
                }
                frame=WSStreamProcessing::MIDDLE_FRAME;
              }else{
                bool last_frame=(
                  isFIN() && (currentOpCode == WebSocketProtocol::CONTINUE)
                );
                if(last_frame)
                {
                  if(opcodes.empty()) // no previous continuation frame
                  {
                    return { 
                      cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
                      WebSocketProtocol::PROTOCOL_VIOLATION
                    };
                  }
                  frame=WSStreamProcessing::LAST_FRAME;
                  currentOpCode=opcodes.top(); // restore the opcode
                  opcodes.pop();
                }
                else
                { /* can't determine the frame type. closing connection */
                  return { 
                    cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
                    WebSocketProtocol::PROTOCOL_VIOLATION
                  };
                }
              }
            }
          }
          /** end of fragmentation detection **/
          
          size_t size_first_byte=static_cast<size_t>(headerBytes[1]&127);
          
          // ==== This behavior is not specified by RFC.
          // ==== This is a compliance to Autobahn TestSuite.
          /**
          * Autobahn TestSuite Case 2.8 requires close for unsolicited PONG
          * This behavior violates last sentence of section 5.5.3 of the RFC:
          * A Pong frame MAY be sent unsolicited.  This serves as a
          * unidirectional heartbeat.  A response to an unsolicited Pong 
          * frame is not expected.
          * 
          * So this case is ignored for now. However later on there will be 
          * a method to analyze if the endpoint just spamms server with 
          * unsolicited PONGs and connection will fail with the
          * POLICY_VIOLATION code
          * 
          **/
          if(controlFrame)
          {
            switch(size_first_byte)
            {
              case 0:
                  switch(currentOpCode)
                  {
                    /**
                     * Autobahn TestSuite requires to _Fail the connection and 
                     * send the Close Code 1000 in response to CLOSE frames
                     * with payload length 0. Though it is not specified by RFC
                     * I'll obey here, because it is reasonable and complies
                     * to RFC spec with "MUST _Fail" the connection immediately
                     * in response to Close Control Frames.
                     **/
                    case WebSocketProtocol::CLOSE:
                      return { 
                        cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
                        WebSocketProtocol::NORMAL
                      };
                    default:
                      break;
                  }
              break;
              case 126:
              case 127:
                switch(currentOpCode)
                {
                /** Autobahn TestSuite requires to _Fail the connection and send the 
                 * Close Code 1000 in response to control frames with Payloads 
                 * larger then 125.
                 * However RFC does specify this behavior so sending the
                 * POLICY_VIOLATION to comply to AB TS.
                 **/
                  case WebSocketProtocol::CLOSE:
                  case WebSocketProtocol::PING:
                  case WebSocketProtocol::PONG:
                    return { 
                      cursor, WSStreamProcessing::CLOSE_WITH_CODE, 
                      WebSocketProtocol::PROTOCOL_VIOLATION
                  };
                  break;
                  default:
                    break;
                }
                break;
              default:
                break;

            }
          }
          
          
          auto ST=WebSocketProtocol::WS::getSizeType(headerBytes[1]);
          
          switch(ST)
          {
            case WebSocketProtocol::MIN: // skip size it is embedded in header
              state=WSStreamProcessing::SIZE_MIN;
              return parse(stream,limit,cursor,fd);
            case WebSocketProtocol::SHORT:
              state=WSStreamProcessing::SIZE_SHORT;
              return parse(stream,limit,cursor,fd);
            case WebSocketProtocol::LONG:
              state=WSStreamProcessing::SIZE_LARGE;
              return parse(stream,limit,cursor,fd);
          }
        }
        if(cursor == limit ) cursor = 0;
        return { cursor, WSStreamProcessing::MORE, WebSocketProtocol::NORMAL};
      case WSStreamProcessing::SIZE_MIN:
        messageSize=headerBytes[1]&127;
        state=WSStreamProcessing::LOAD_MASK;
        PLReadyBytes=0;
        return parse(stream,limit,cursor,fd);
      case WSStreamProcessing::SIZE_SHORT:
        while((sizeBytesReady<2)&&(cursor<limit))
        {
          optionalSizeBytes[sizeBytesReady]=stream[cursor];
          ++sizeBytesReady;++cursor;
        }
        if(sizeBytesReady == 2)
        {
          uint16_t tmpMsgSize=0;
          memcpy(&tmpMsgSize,optionalSizeBytes,2);
          messageSize=be16toh(tmpMsgSize);
          state=WSStreamProcessing::LOAD_MASK;
          PLReadyBytes=0;
          return parse(stream,limit,cursor,fd);
        }

        if(cursor == limit) cursor=0;
        return { cursor, WSStreamProcessing::MORE, WebSocketProtocol::NORMAL};
      case WSStreamProcessing::SIZE_LARGE:
        while((sizeBytesReady<8)&&(cursor<limit))
        {
          optionalSizeBytes[sizeBytesReady]=stream[cursor];
          ++sizeBytesReady;++cursor;
        }
        if(sizeBytesReady == 8)
        {
          uint64_t tmpMsgSize=0;
          memcpy(&tmpMsgSize,optionalSizeBytes,8);
          messageSize=be64toh(tmpMsgSize);
          state=WSStreamProcessing::LOAD_MASK;
          PLReadyBytes=0;
          return parse(stream,limit,cursor,fd);
        }
        if(cursor == limit) cursor=0;
        return { cursor, WSStreamProcessing::MORE, WebSocketProtocol::NORMAL};
      case WSStreamProcessing::LOAD_MASK:
        while((maskBytesReady<4)&&(cursor<limit))
        {
          MASK[maskBytesReady]=stream[cursor];
          ++maskBytesReady;++cursor;
        }
        if(maskBytesReady == 4)
        {
          state=WSStreamProcessing::PAYLOAD;
          return parse(stream,limit,cursor,fd);
        }
        if(cursor == limit) cursor = 0;
        return { cursor, WSStreamProcessing::MORE, WebSocketProtocol::NORMAL};
      case WSStreamProcessing::PAYLOAD:
      {
        switch(frame)
        {
          case WSStreamProcessing::SINGLE_FRAME:
          {
            while((PLReadyBytes<messageSize)&&(cursor<limit))
            {
              message->push_back(stream[cursor]^MASK[(message->size())%4]);
              ++cursor;
              ++PLReadyBytes;
            }
            if(PLReadyBytes == messageSize)
            {
              state=WSStreamProcessing::DONE;
              return parse(stream,limit,cursor,fd);
            }
            if(cursor == limit) cursor=0;
            return { cursor, WSStreamProcessing::MORE, WebSocketProtocol::NORMAL};
          } break;
          case WSStreamProcessing::FIRST_FRAME:
          case WSStreamProcessing::MIDDLE_FRAME:
          case WSStreamProcessing::LAST_FRAME:
          {
            while((PLReadyBytes<messageSize)&&(cursor<limit))
            {
              messageFrames->push_back(stream[cursor]^MASK[PLReadyBytes%4]);
              ++cursor;
              ++PLReadyBytes;
            }
            if(PLReadyBytes == messageSize)
            {
              state=WSStreamProcessing::DONE;
              return parse(stream,limit,cursor,fd);
            }
            if(cursor == limit) cursor=0;
            return { cursor, WSStreamProcessing::MORE, WebSocketProtocol::NORMAL};
          }
          break;
        }
      } 
      break;
      case WSStreamProcessing::DONE:
      {
        switch(frame)
        {
          case WSStreamProcessing::SINGLE_FRAME:   
           case WSStreamProcessing::LAST_FRAME:
             state=WSStreamProcessing::MESSAGE_READY;
            return { cursor, WSStreamProcessing::TAKE_READY_MESSAGE, WebSocketProtocol::NORMAL};
          case WSStreamProcessing::FIRST_FRAME:
          case WSStreamProcessing::MIDDLE_FRAME:
            state=WSStreamProcessing::INIT;
            return parse(stream,limit,cursor,fd);
        }
      }
      break;
      default:
        throw std::logic_error("WSStreamParser::parse() - falling out of decision switch. Incorrect state on call to parse()");
        break;
    }
    throw std::logic_error("WSStreamParser::parse() - falling out of decision switch. Incorrect state on call to parse()");
  }
 
  const WSEvent getMessage()
  {
    if(state == WSStreamProcessing::MESSAGE_READY)
    {
      state=WSStreamProcessing::INIT;
      if(frame==WSStreamProcessing::SINGLE_FRAME)
      {
        MSGBufferTypeSPtr tmp(message);
        message=std::make_shared<std::vector<uint8_t>>();
        message->reserve(512);
        return { currentOpCode, tmp };
      }
      else
      {
        MSGBufferTypeSPtr tmp(messageFrames);
        messageFrames=std::make_shared<std::vector<uint8_t>>();
        messageFrames->reserve(512);
        return {currentOpCode, tmp};
      }
    }
    throw std::logic_error("WSStreamParser::getMessage() - message is not ready yet");
  }
  
  /**
   * bool isValidUtf8(unsigned char,length) is a copy-paste from uWebSockets,
   * Based on utf8_check.c by Markus Kuhn, 2005
   * https://www.cl.cam.ac.uk/~mgk25/ucs/utf8_check.c
   * Optimized for predominantly 7-bit content by Alex Hultman, 2016
   * Licensed as Zlib.
   **/
  bool isValidUtf8(uint8_t *cs, const size_t length)
  {
    uint8_t *s=cs;
    for(const uint8_t *e = s + length; s != e;)
    {
      if(s + 4 <= e && ((*(uint32_t *) s) & 0x80808080) == 0)
      {
        s += 4;
      } 
      else 
      {
        while (!(*s & 0x80))
        {
          if (++s == e)
          {
            return true;
          }
        }
            
        if ((s[0] & 0x60) == 0x40)
        {
          if (s + 1 >= e || (s[1] & 0xc0) != 0x80 || (s[0] & 0xfe) == 0xc0)
          {
            return false;
          }
          s += 2;
        } else if ((s[0] & 0xf0) == 0xe0)
        {
          if (s + 2 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 ||
              (s[0] == 0xe0 && (s[1] & 0xe0) == 0x80) || (s[0] == 0xed && (s[1] & 0xe0) == 0xa0)) {
            return false;
          }
          s += 3;
        } else if ((s[0] & 0xf8) == 0xf0)
        {
          if (s + 3 >= e || (s[1] & 0xc0) != 0x80 || (s[2] & 0xc0) != 0x80 || (s[3] & 0xc0) != 0x80 ||
             (s[0] == 0xf0 && (s[1] & 0xf0) == 0x80) || (s[0] == 0xf4 && s[1] > 0x8f) || s[0] > 0xf4) {
            return false;
          }
          s += 4;
        } else {
            return false;
        }
    }
    }
    return true;
  }
};  
  

  /**
   * 5.5.2.  Ping

   The Ping frame contains an opcode of 0x9.

   A Ping frame MAY include "Application data".

   Upon receipt of a Ping frame, an endpoint MUST send a Pong frame in
   response, unless it already received a Close frame.  It SHOULD
   respond with Pong frame as soon as is practical.  Pong frames are
   discussed in Section 5.5.3.

   An endpoint MAY send a Ping frame any time after the connection is
   established and before the connection is closed.

   NOTE: A Ping frame may serve either as a keepalive or as a means to
   verify that the remote endpoint is still responsive.
   **/


#endif /* __WSSTREAMPARSER_H__ */

