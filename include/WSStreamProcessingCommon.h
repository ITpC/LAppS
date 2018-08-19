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
 *  $Id: WSStreamProcessingCommon.h August 19, 2018 9:27 AM $
 * 
 **/


#ifndef __WSSTREAMPROCESSINGCOMMON_H__
#  define __WSSTREAMPROCESSINGCOMMON_H__

namespace WSStreamProcessing
{
  enum State : uint8_t {
    INIT, 
    HEADER_1ST_BYTE,
    HEADER_2DN_BYTE, 
    READ_SHORT_SIZE_0, 
    READ_SHORT_SIZE_1,
    READ_LONG_SIZE_0,
    READ_LONG_SIZE_1, 
    READ_LONG_SIZE_2,
    READ_LONG_SIZE_3,
    READ_LONG_SIZE_4,
    READ_LONG_SIZE_5,
    READ_LONG_SIZE_6,
    READ_LONG_SIZE_7, 
    READ_MASK_0,
    READ_MASK_1,
    READ_MASK_2,
    READ_MASK_3,
    VALIDATE,
    PAYLOAD, 
    DONE, 
    MESSAGE_READY 
  };
  enum WSFrameSizeType : uint8_t { MIN=0, SHORT=2, LONG=8 };
  enum Directive : uint8_t { MORE, TAKE_READY_MESSAGE, CLOSE_WITH_CODE, CONTINUE };
  enum FrameSeq : uint8_t { SINGLE, FIRST, MIDDLE, LAST };
  
  struct FrameHeader
  {
    uint8_t                   PAD;
    uint8_t                   FIN;
    uint8_t                   RSV1;
    uint8_t                   RSV2;
    uint8_t                   RSV3;
    uint8_t                   MASKFLAG;
    uint8_t                   SMALL_SIZE;
    
    WSFrameSizeType           SIZE_TYPE;
    WebSocketProtocol::OpCode OPCODE;
    FrameSeq                  FSEQ;
    
    uint8_t                   MEDIUM_SIZE[2];
    uint8_t                   LARGE_SIZE[8];
    uint8_t                   MASK[4];
    size_t                    MSG_SIZE;
  };
  
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

#endif /* __WSSTREAMPROCESSINGCOMMON_H__ */

