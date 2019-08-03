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
 *  $Id: HTTPRequestParser.h January 8, 2018 7:08 AM $
 * 
 **/


#ifndef __HTTPREQUESTPARSER_H__
#  define __HTTPREQUESTPARSER_H__

#include <assert.h>

#include <string>
#include <stdexcept>
#include <map>
#include <iostream>
#include <vector>


/**
 * @brief limited HTTP request parser for WS GET requests
 * and handshake only. 
 **/
class HTTPRequestParser
{
private:
 std::string HTTPVersion;
 std::string RequestTarget;
 size_t mMinSize;
 std::map<std::string,std::string> mHeaders;
 
 // beware nullptr and short strings, they are not checked here
 bool isCRLF(const char* str) const
 {
   if((str[0] == '\x0d') && (str[1] == '\x0a')) 
     return true;
   return false;
 }
 void throwOnMinSizeViolation(const std::string& ref)
 {
   if(ref.size()<mMinSize) throwMalformedHTTPRequest();
 }
 void throwOnMinSizeViolation(const std::vector<uint8_t>& ref)
 {
   if(ref.size()<mMinSize) throwMalformedHTTPRequest();
 }
 void throwMalformedHTTPRequest(const std::string& optional="")
 {
   throw std::logic_error(
       "HTTPRequestParser::HTTPRequestParser() - malformed HTTP request: "+
       optional
   );
 }
 
  void CollectHeaders(const char* buff, const size_t sz, const size_t ini_idx)
  {
    size_t index=ini_idx;
    
    while(index < sz)
    {
      std::string key;
      std::string value;

      while((index<sz)&&(buff[index] != ' ')&&(buff[index] != '\r')&&(buff[index] != '\n')&&(buff[index] != ':'))
      {
        key.append(1,buff[index]);
        ++index;
      }

      while((index<sz)&&((buff[index] == ' ')||(buff[index] == ':')||(buff[index] == '\t'))) ++index;

      while((index<sz)&&(buff[index] != '\r')&&(buff[index] != '\n'))
      {
        value.append(1,buff[index]);
        ++index;
      }

      while((index<sz)&&((buff[index] == ' ')||(buff[index] == '\r')||(buff[index] == '\n'))) ++index;

      mHeaders.emplace(std::move(key),std::move(value));
    }
  }
 
public:
 
 HTTPRequestParser()
 :  mMinSize(168)
 {
 }
 void parse(const std::vector<uint8_t>& ref, const size_t bufflen)
 {
   size_t cursor=0;
   
   // RFC 7230 – Section 3
   
   // shortcut: minimal length
   throwOnMinSizeViolation(ref);
   
   // CRLF at start? Ignoring RFC 7230 if there is no CRLF at start.
   if(isCRLF((const char*)(ref.data()))) cursor+=2;
   
   
   // is it a get request? - ignoring all other requests
   if((ref.data()[cursor] == 'G') && ((ref.data()[cursor+1]) == 'E') && ((ref.data()[cursor+2]) == 'T'))
   {
     // one space only as per RFC
     if(ref.data()[cursor+3] == ' ')
     {
       // target
       cursor=4;
       while((cursor<bufflen)&&(ref[cursor]!=' ')&&(ref[cursor]!='\t'))
       {
         RequestTarget+=ref[cursor];
         ++cursor;
       }
       
       // skip spaces
       
       while((cursor<bufflen)&&((ref[cursor]==' ')||(ref[cursor]=='\t'))) ++cursor;
       
       // HTTP Version
       while((cursor < bufflen)&&(ref[cursor]!='\r')&&(ref[cursor]!='\n')&&(ref[cursor]!=' ')&&(ref[cursor]!='\t'))
       {
         HTTPVersion+=ref[cursor];
         ++cursor;
       }
       
       // skip trailing spaces and CRLF 
       
       while((cursor < bufflen)&&((ref[cursor]=='\r')||(ref[cursor]=='\n')||(ref[cursor]==' ')||(ref[cursor]=='\t'))) ++cursor;
       
       CollectHeaders((const char*)(ref.data()),bufflen,cursor);
       
       if(mHeaders.empty())
         throw std::system_error(EBADMSG, std::system_category(), "HTTPRequestParser::parse() - malformed or unsupported GET Request");
     } else throwMalformedHTTPRequest("No space after MODE[GET]");
   } else throwMalformedHTTPRequest("Only GET requests are accepted");
 }
 
 const std::string& operator[](const std::string& key)
 {
   static const std::string empty("");
   auto it=mHeaders.find(key);
   if(it!=mHeaders.end())
     return it->second;
   return empty;
 }
 const std::string& getRequestTarget() const
 {
   return RequestTarget;
 }
 void clear()
 {
   mHeaders.clear();
   RequestTarget.clear();
   HTTPVersion.clear();
   mMinSize=168;
 }
};




#endif /* __HTTPREQUESTPARSER_H__ */

