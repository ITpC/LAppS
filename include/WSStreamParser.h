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
 *  $Id: WSStreamParser.h July 12, 2018 12:55 AM $
 * 
 **/


#ifndef __WSSTREAMPARSER_H__
#  define __WSSTREAMPARSER_H__

#include <endian.h>

#include <cassert>
#include <vector>
#include <memory>
#include <exception>
#include <queue>
#include <stack>

#include <WSProtocol.h>
#include <WSEvent.h>

#include <sys/atomic_mutex.h>
#include <sys/synclock.h>

/* check for AVX2 */
#include <immintrin.h>
#include <cpuid.h>

#include <WSStreamProcessingCommon.h>

namespace WSStreamProcessing
{
  class WSStreamParser
  {
   protected:
    size_t                                  mPLBytesReady;
    size_t                                  cursor;
    size_t                                  mMaxMSGSize;
    size_t                                  mOutMSGPreSize;
    
    FrameHeader                             mHeader;
    std::stack<WebSocketProtocol::OpCode>   opcodes;
    MSGBufferTypeSPtr                       message;
    MSGBufferTypeSPtr                       messageFrames;
    
    std::queue<MSGBufferTypeSPtr>           mBufferQueue;
    
    itc::sys::AtomicMutex                   mBQMutex;
    
    State                                   mState;
    
    uint64_t                                MASK;
    
    
    void reset()
    {
      mState=State::HEADER_1ST_BYTE;
      mPLBytesReady=0;
      memset(&mHeader,0,sizeof(mHeader));
    }
    
    const Result getHeader1stByte(const uint8_t* stream, const size_t& limit, const size_t& offset)
    {
      cursor=offset;
      if(cursor < limit)
      {
        mHeader.FIN=(stream[cursor]&128) >> 7;
        mHeader.RSV1=(stream[cursor]&64) >> 6;
        mHeader.RSV2=(stream[cursor]&32) >> 5;
        mHeader.RSV3=(stream[cursor]&16) >> 4;
        mHeader.OPCODE=static_cast<WebSocketProtocol::OpCode>(stream[cursor]&0xF);
        
        if((mHeader.RSV1|mHeader.RSV2|mHeader.RSV3) == 1)
        {
          return { 
            cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
            WebSocketProtocol::PROTOCOL_VIOLATION
          };
        }
        
        if(!WebSocketProtocol::WS::isOpCodeSupported(mHeader.OPCODE))
        {
          return { 
            cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
            WebSocketProtocol::NO_COMPRENDE
          };
        }
        ++cursor;
        mState=State::HEADER_2DN_BYTE;
        return { 
          cursor, WSStreamProcessing::Directive::CONTINUE, 
          WebSocketProtocol::NORMAL
        };
      }else{
        cursor=0;
        return { 
          cursor, WSStreamProcessing::Directive::MORE, 
          WebSocketProtocol::NORMAL
        };
      }
    }
    
    const Result getHeader2ndByte(const uint8_t* stream, const size_t& limit, const size_t& offset)
    {
      cursor=offset;
      if(cursor < limit)
      {
        mHeader.MASKFLAG=(stream[cursor]&128) >> 7;
        if(mHeader.MASKFLAG == 0)
        {
          return { 
            cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
            WebSocketProtocol::PROTOCOL_VIOLATION
          };
        }

        mHeader.SMALL_SIZE=stream[cursor]&127;

        if(mHeader.SMALL_SIZE<126)
        {
          mHeader.SIZE_TYPE=WSFrameSizeType::MIN;
          mHeader.MSG_SIZE=mHeader.SMALL_SIZE;
          mState=State::READ_MASK_0;
        }else if(mHeader.SMALL_SIZE == 126)
        {
          mHeader.SIZE_TYPE=WSFrameSizeType::SHORT;
          mState=State::READ_SHORT_SIZE_0;
        }
        else 
        {
          mHeader.SIZE_TYPE=WSFrameSizeType::LONG;
          mState=State::READ_LONG_SIZE_0;
        }
        ++cursor;

        return { 
          cursor, WSStreamProcessing::Directive::CONTINUE, 
          WebSocketProtocol::NORMAL
        };
        
      }else{
        cursor=0;
        return { 
          cursor, WSStreamProcessing::Directive::MORE, 
          WebSocketProtocol::NORMAL
        };
      }
      
    }
    
    const Result readSIZE(const uint8_t* stream, const size_t& limit, const size_t& offset)
    {
      cursor=offset;
      if(cursor < limit)
      {
        switch(mState)
        {
          case State::READ_SHORT_SIZE_0:
            mHeader.MEDIUM_SIZE[0]=stream[cursor];
            mState=State::READ_SHORT_SIZE_1;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_SHORT_SIZE_1:
          {
            mHeader.MEDIUM_SIZE[1]=stream[cursor];
            uint16_t* size_ptr=(uint16_t*)(mHeader.MEDIUM_SIZE);
            mHeader.MSG_SIZE=be16toh(*size_ptr);
            if(mHeader.MSG_SIZE > mMaxMSGSize)
            {
              return {
                cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
                WebSocketProtocol::MESSAGE_TOO_BIG
              };
            }
            mState=State::READ_MASK_0;
            ++cursor;
          }
          if(cursor == limit)
          {
            return {
              cursor, WSStreamProcessing::Directive::MORE, 
              WebSocketProtocol::NORMAL
            };
          }
          break;
          case State::READ_LONG_SIZE_0:
            mHeader.LARGE_SIZE[0]=stream[cursor];
            mState=State::READ_LONG_SIZE_1;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_1:
            mHeader.LARGE_SIZE[1]=stream[cursor];
            mState=State::READ_LONG_SIZE_2;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_2:
            mHeader.LARGE_SIZE[2]=stream[cursor];
            mState=State::READ_LONG_SIZE_3;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_3:
            mHeader.LARGE_SIZE[3]=stream[cursor];
            mState=State::READ_LONG_SIZE_4;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_4:
            mHeader.LARGE_SIZE[4]=stream[cursor];
            mState=State::READ_LONG_SIZE_5;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_5:
            mHeader.LARGE_SIZE[5]=stream[cursor];
            mState=State::READ_LONG_SIZE_6;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_6:
            mHeader.LARGE_SIZE[6]=stream[cursor];
            mState=State::READ_LONG_SIZE_7;
            ++cursor;
            if(cursor == limit)
            {
              return {
                cursor, WSStreamProcessing::Directive::MORE, 
                WebSocketProtocol::NORMAL
              };
            }
          case State::READ_LONG_SIZE_7:
          {
            mHeader.LARGE_SIZE[7]=stream[cursor];
            uint64_t *size_ptr=(uint64_t*)(mHeader.LARGE_SIZE);
            mHeader.MSG_SIZE=be64toh(*size_ptr);
            if(mHeader.MSG_SIZE > mMaxMSGSize)
            {
              return {
                cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
                WebSocketProtocol::MESSAGE_TOO_BIG
              };
            }
            mState=State::READ_MASK_0;
            ++cursor;
          } 
          if(cursor == limit)
          {
            return {
              cursor, WSStreamProcessing::Directive::MORE, 
              WebSocketProtocol::NORMAL
            };
          }
          break;
          default:
            return { 
              cursor, WSStreamProcessing::Directive::CONTINUE, 
              WebSocketProtocol::NORMAL
            };
        }
        return { 
          cursor, WSStreamProcessing::Directive::CONTINUE, 
          WebSocketProtocol::NORMAL
        };
      }else{
        cursor=0;
        return { 
          cursor, WSStreamProcessing::Directive::MORE, 
          WebSocketProtocol::NORMAL
        };
      }
    }
    
    

    const Result validateHeader()
    {
      bool controlOpCode=(
        (mHeader.OPCODE == WebSocketProtocol::CLOSE)||
        (mHeader.OPCODE == WebSocketProtocol::PING)||
        (mHeader.OPCODE == WebSocketProtocol::PONG)
      );

      bool controlFrame=(mHeader.FIN && controlOpCode);
      bool badControlFrame=((!mHeader.FIN) && controlOpCode);

      if(badControlFrame)
      {
        return { 
          cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
          WebSocketProtocol::PROTOCOL_VIOLATION
        };
      }

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

      if(!controlFrame)
      {
        bool single_frame=mHeader.FIN&&((mHeader.OPCODE==WebSocketProtocol::TEXT)|| (mHeader.OPCODE==WebSocketProtocol::BINARY))&&opcodes.empty();
        bool first_fragment=(!mHeader.FIN)&&((mHeader.OPCODE==WebSocketProtocol::TEXT)|| (mHeader.OPCODE==WebSocketProtocol::BINARY))&&opcodes.empty();
        bool continuation_fragment=(!mHeader.FIN)&&(mHeader.OPCODE==WebSocketProtocol::CONTINUE)&&(!opcodes.empty());
        bool last_fragment=(mHeader.FIN)&&(mHeader.OPCODE==WebSocketProtocol::CONTINUE)&&(!opcodes.empty());
        bool bad_frame=(!single_frame)&&(!first_fragment)&&(!continuation_fragment)&&(!last_fragment);


        if(bad_frame)
        {
          return { 
              cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
              WebSocketProtocol::PROTOCOL_VIOLATION
            };
        }

        if(single_frame) 
          mHeader.FSEQ=FrameSeq::SINGLE;
        else if(first_fragment)
        {
          mHeader.FSEQ=FrameSeq::FIRST;
          opcodes.push(mHeader.OPCODE);
        } 
        else if(continuation_fragment)
        {
          mHeader.FSEQ=FrameSeq::MIDDLE;
        } 
        else if(last_fragment)
        {
          mHeader.FSEQ=FrameSeq::LAST;
          mHeader.OPCODE=std::move(opcodes.top()); // restore the opcode
          opcodes.pop();
        }
      }else{ // work on control frames

        if(mHeader.MSG_SIZE == 0)
        {
          if(mHeader.OPCODE==WebSocketProtocol::OpCode::CLOSE)
            return { 
              cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
              WebSocketProtocol::NORMAL
            };
        }
        else if(mHeader.MSG_SIZE > 125)
        {
          return { 
              cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
              WebSocketProtocol::PROTOCOL_VIOLATION
          };
        }
      }
      mState=State::PAYLOAD;
      return { 
          cursor, WSStreamProcessing::Directive::CONTINUE,
          WebSocketProtocol::NORMAL
      };
    }
    
    auto getBuffer()
    {
      AtomicLock sync(mBQMutex);
      if(mBufferQueue.empty())
      {
        return std::make_shared<MSGBufferType>(mOutMSGPreSize);
      }else{
        auto tmp=std::move(mBufferQueue.front());
        tmp->resize(mOutMSGPreSize);
        mBufferQueue.pop();
        return tmp;
      }
    }
  
   public:
    explicit WSStreamParser(const size_t& presz)
    : mPLBytesReady{0},cursor{0},mMaxMSGSize{0},
      mOutMSGPreSize{presz},mHeader{0},opcodes(),
      message(std::make_shared<std::vector<uint8_t>>(presz)),
      messageFrames(std::make_shared<std::vector<uint8_t>>(presz)),
      mBufferQueue(),mBQMutex(),mState{State::INIT},MASK{0}
    {
    }
    
    WSStreamParser()=delete;
    WSStreamParser(const WSStreamParser&)=delete;
    WSStreamParser(WSStreamParser&)=delete;

    void returnBuffer(std::remove_reference<const std::shared_ptr<MSGBufferType>&>::type buff)
    {
      AtomicLock sync(mBQMutex);
      mBufferQueue.push(std::move(buff));
    }

    void setMaxMSGSize(const size_t& mms)
    {
      mMaxMSGSize=mms;
    }
    
    void setMessageBufferSize(const size_t& bsz)
    {
      mOutMSGPreSize=bsz;
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
}
  




#endif /* __WSSTREAMPARSER_H__ */

