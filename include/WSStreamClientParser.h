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
 *  $Id: WSStreamClientParser.h August 20, 2018 2:20 PM $
 * 
 **/


#ifndef __WSSTREAMCLIENTPARSER_H__
#  define __WSSTREAMCLIENTPARSER_H__

#include <WSStreamParser.h>

namespace WSStreamProcessing
{
  class WSStreamClientParser : public WSStreamParser
  {
  private:
   const Result getHeader2ndByte(const uint8_t* stream, const size_t& limit, const size_t& offset)
    {
      cursor=offset;
      if(cursor < limit)
      {
        mHeader.MASKFLAG=(stream[cursor]&128) >> 7;
        // server may not send masked data
        if(mHeader.MASKFLAG == 1)
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
    const Result processPayload(const uint8_t* stream, const size_t& limit,const size_t& offset)
    {
      cursor=offset;
      
      switch(mHeader.FSEQ)
      {
        case FrameSeq::SINGLE:
        {
         if(mPLBytesReady == 0)
            message->resize(mHeader.MSG_SIZE);
           
          size_t offs=limit-cursor;
          
          if((mPLBytesReady+offs)>mHeader.MSG_SIZE)
          {
            size_t copy_bytes= mHeader.MSG_SIZE-mPLBytesReady;
            memcpy(message->data()+mPLBytesReady,stream+cursor,copy_bytes);
            cursor+=copy_bytes;
            mPLBytesReady+=copy_bytes;
          }
          else{
            memcpy(message->data()+mPLBytesReady,stream+cursor,offs);
            mPLBytesReady+=(offs);
            cursor+=offs;
          }
          
          if(mPLBytesReady == mHeader.MSG_SIZE)
          {
            mState=WSStreamProcessing::State::DONE;
            return { 
              cursor, 
              WSStreamProcessing::Directive::CONTINUE, 
              WebSocketProtocol::DefiniteCloseCode::NORMAL
            };
          }
          
          if(cursor==limit) cursor=0;
          
          return { 
            cursor, 
            WSStreamProcessing::Directive::MORE, 
            WebSocketProtocol::DefiniteCloseCode::NORMAL
          };
        } 
        case FrameSeq::FIRST:
          if(mPLBytesReady == 0)
            messageFrames->resize(mHeader.MSG_SIZE);
        case FrameSeq::MIDDLE:
        case FrameSeq::LAST:
        {  
          if(mHeader.FSEQ != FrameSeq::FIRST)
          {
            if((mPLBytesReady == 0)&&(cursor!=limit)) 
            {
              if((messageFrames->size()+mHeader.MSG_SIZE)>mMaxMSGSize)
              return {
                cursor, WSStreamProcessing::Directive::CLOSE_WITH_CODE, 
                WebSocketProtocol::DefiniteCloseCode::MESSAGE_TOO_BIG
              };
              messageFrames->resize(messageFrames->size()+mHeader.MSG_SIZE);
            }
          }
          
          const size_t used=messageFrames->size()-mHeader.MSG_SIZE;
          
          size_t offs=limit-cursor;
          
          if((mPLBytesReady+(limit-cursor))>mHeader.MSG_SIZE)
          {
            size_t copy_bytes= mHeader.MSG_SIZE-mPLBytesReady;
            memcpy(messageFrames->data()+used+mPLBytesReady,stream+cursor,copy_bytes);
            cursor+=copy_bytes;
            mPLBytesReady+=copy_bytes;
          }else{
            memcpy(messageFrames->data()+used+mPLBytesReady,stream+cursor,offs);
            mPLBytesReady+=offs;
            cursor+=offs;
          }
          
          if(mPLBytesReady == mHeader.MSG_SIZE)
          {
            mState=WSStreamProcessing::State::DONE;
            return { 
              cursor, 
              WSStreamProcessing::Directive::CONTINUE, 
              WebSocketProtocol::DefiniteCloseCode::NORMAL
            };
          }
          
          if(cursor == limit) cursor=0;
          
          return { 
            cursor, 
            WSStreamProcessing::Directive::MORE, 
            WebSocketProtocol::DefiniteCloseCode::NORMAL
          };
        }
      }
      throw std::logic_error("WSStreamParser::processPayload() switch is out of options, never should have happened. Blame the programmer");
    }
    
    const Result decideOnDone(const uint8_t* stream, const size_t& limit, const size_t& offset)
    {
      cursor=offset;
      switch(mHeader.FSEQ)
      {
        case WSStreamProcessing::SINGLE:   
         case WSStreamProcessing::LAST:
           mState=WSStreamProcessing::State::MESSAGE_READY;
          return {
            cursor,
            WSStreamProcessing::Directive::TAKE_READY_MESSAGE,
            WebSocketProtocol::DefiniteCloseCode::NORMAL
          };
        case WSStreamProcessing::FIRST:
        case WSStreamProcessing::MIDDLE:
          mState=WSStreamProcessing::State::INIT;
          return parse(stream,limit,cursor);
      }
      throw std::logic_error("WSStreamParser::decideOnDone() switch is out of options, never should have happened. Blame the programmer");
    }
    
  public:
    const Result parse(const uint8_t* stream, const size_t& limit, const size_t& offset)
    {
      cursor=offset;
      
      switch(mState)
      {
        case WSStreamProcessing::State::INIT:
          reset();
        case WSStreamProcessing::State::HEADER_1ST_BYTE:
        {
          auto result{getHeader1stByte(stream,limit,cursor)};
          if(result.directive != WSStreamProcessing::CONTINUE )
          {
            return result;
          }
        }
        case WSStreamProcessing::State::HEADER_2DN_BYTE:
        {
          auto result{getHeader2ndByte(stream,limit,cursor)};
          if(result.directive != WSStreamProcessing::Directive::CONTINUE)
          {
            return result;
          }
        }
        case WSStreamProcessing::State::READ_SHORT_SIZE_0:
        case WSStreamProcessing::State::READ_SHORT_SIZE_1:
        case WSStreamProcessing::State::READ_LONG_SIZE_0:
        case WSStreamProcessing::State::READ_LONG_SIZE_1:
        case WSStreamProcessing::State::READ_LONG_SIZE_2:
        case WSStreamProcessing::State::READ_LONG_SIZE_3:
        case WSStreamProcessing::State::READ_LONG_SIZE_4:
        case WSStreamProcessing::State::READ_LONG_SIZE_5:
        case WSStreamProcessing::State::READ_LONG_SIZE_6:
        case WSStreamProcessing::State::READ_LONG_SIZE_7:
        {
          auto result{readSIZE(stream,limit,cursor)};
          if(result.directive != WSStreamProcessing::Directive::CONTINUE)
          {
            return result;
          }
        }
        case WSStreamProcessing::State::READ_MASK_0:
        case WSStreamProcessing::State::READ_MASK_1:
        case WSStreamProcessing::State::READ_MASK_2:
        case WSStreamProcessing::State::READ_MASK_3:
          mState=WSStreamProcessing::State::VALIDATE;
        case WSStreamProcessing::State::VALIDATE:
        {
          auto result{validateHeader()};
          if(result.directive != WSStreamProcessing::Directive::CONTINUE)
          {
            return result;
          }
        }
        case WSStreamProcessing::State::PAYLOAD:
        {
          auto result{processPayload(stream,limit,cursor)};
          if(result.directive != WSStreamProcessing::Directive::CONTINUE)
          {
            return result;
          }
        }
        case WSStreamProcessing::State::DONE:
          return decideOnDone(stream,limit,cursor);
        default:
          throw std::logic_error("WSStreamParser::parse() - switch case is out of options. Never should have happened. Blame the programmer");
      }
    }
    
    const WSEvent getMessage()
    {
      if(mState == WSStreamProcessing::MESSAGE_READY)
      {
        mState=WSStreamProcessing::INIT;
      }
      
        if(mHeader.FSEQ==WSStreamProcessing::FrameSeq::SINGLE)
        {
          MSGBufferTypeSPtr tmp(std::move(message));
          message=getBuffer();
          return { mHeader.OPCODE, std::move(tmp) };
        }
        else
        {
          MSGBufferTypeSPtr tmp(std::move(messageFrames));
          messageFrames=getBuffer();
          return { mHeader.OPCODE, std::move(tmp)};
        }
//      }
//      throw std::logic_error("WSStreamParser::getMessage() - message is not ready yet");
    }
    
    explicit WSStreamClientParser(const size_t presize=512) : WSStreamParser(presize)
    {
    }
    
    WSStreamClientParser()=delete;
    WSStreamClientParser(const WSStreamClientParser&)=delete;
    WSStreamClientParser(WSStreamClientParser&)=delete;
  };
}

#endif /* __WSSTREAMCLIENTPARSER_H__ */

