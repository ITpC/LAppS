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
 
 const size_t getKVPair(
  const char *header, 
  const size_t limit, 
  const char *compare_with,
  const size_t compare_size
 ){
   size_t cursor=0;
   
   size_t key_last=getKey(header,limit);
   std::string key(header,key_last);
   cursor+=(key_last+1); // skip ':'
   
   if(key.compare(0,compare_size,compare_with)==0)
   {
      size_t val_start=0;
      size_t val_end=0;
      getValue(&header[cursor],limit-cursor,val_start,val_end);
      std::string value(&header[cursor+val_start],val_end-val_start);
      cursor+=val_end;
      
      // Skip CRLF
      ++cursor;
      ++cursor;
      mHeaders.emplace(key,value);
    }else{

     // ignoring useless header
      while(
       (cursor < limit) && 
       ((cursor + 1) < limit) && 
       (!isCRLF(&header[cursor]))
      ) ++cursor;
      while((header[cursor] == '\x0d') || (header[cursor] == '\x0a')) ++cursor;
    }
    return cursor;
  }
 
 // collect minimum required headers for WebSockets, ignore the rest
 size_t collectHeaders(const char* headers,const size_t limit)
 {
   size_t cursor=0;
   while(cursor < limit && headers[cursor])
   {
     // decide if to skip this key value
     switch(headers[cursor])
     {
       case 'C': // Connect
         cursor+=getKVPair(&headers[cursor],limit-cursor,"Connection",10);
       break;
       case 'H': // Host
         cursor+=getKVPair(&headers[cursor],limit-cursor,"Host",4);
       break;
       case 'O': // Origin
         cursor+=getKVPair(&headers[cursor],limit-cursor,"Origin",6);
       break;
       case 'U': // Upgrade
         cursor+=getKVPair(&headers[cursor],limit-cursor,"Upgrade",7);
       break;
       case 'S': // Sec-WebSocket-*
         cursor+=getKVPair(&headers[cursor],limit-cursor,"Sec-WebSocket",13);
       break;
       default: // ignoring useless header parts
         while((cursor+1 < limit) && (!isCRLF(&headers[cursor])))
           ++cursor;
         // Skip CRLF
         ++cursor;
         ++cursor;
         break;
     }
   }
   return cursor;
 }
 size_t getKey(const char* header, const size_t limit)
 {
   size_t i=0;
   while((i<limit) && (header[i] != ':'))
   {
       ++i;
   }
   if(i<limit)
   {
     return i;
   }
   throwMalformedHTTPRequest();
   return 0; // relax the compiler warnings
 }
 void getValue(const char* header, const size_t limit, size_t& start, size_t& stop)
 {
   size_t i=0;
   // skip leading whitespaces
   
   while((i<limit) && ((header[i] == ' ')||(header[i]=='\t'))) ++i;
   
   start=i;
   
   for(;(i<limit)&&(header[i] != '\x0d');++i);

   stop=i;
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
   // (uWebSockets sends malformed requests)
   if(isCRLF((const char*)(ref.data()))) cursor+=2;
   
   
   // is it a get request? - ignoring all other requests
   if((ref.data()[cursor] == 'G') && ((ref.data()[cursor+1]) == 'E') && ((ref.data()[cursor+2]) == 'T'))
   {
     // one space only as per RFC
     if(ref.data()[cursor+3] == ' ')
     {
       // target
       for(size_t i=cursor+4;(i<bufflen&&(ref.data()[i]!=' '));++i)
       {
         RequestTarget+=ref.data()[i];
         cursor=i;
       }

       cursor+=2;

       mMinSize+=(RequestTarget.size()-1);

       throwOnMinSizeViolation(ref);

       for(
         size_t i=cursor;
         ((i<bufflen)&&(!isCRLF((const char*)(&(ref.data()[i])))));
         ++i
       ){
         HTTPVersion+=ref.data()[i];
       }
       cursor+=HTTPVersion.size()+2;

       cursor+=collectHeaders((const char*)(&ref.data()[cursor]),bufflen-cursor);

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
   mHeaders.clear();
   mMinSize=168;
 }
};




#endif /* __HTTPREQUESTPARSER_H__ */

