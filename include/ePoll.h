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
 *  $Id: ePoll.h January 7, 2018 5:25 AM $
 * 
 **/


#ifndef __EPOLL_H__
#  define __EPOLL_H__

#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <system_error>

// 
// 
#define ePollINDefaultOps EPOLLIN|EPOLLRDHUP|EPOLLPRI|EPOLLONESHOT
#define ePollINEdge ePollINDefaultOps|EPOLLET|EPOLLEXCLUSIVE

/**
 * \@brief epoll wrapper (inbound messages only), not thread-safe. To embed into
 * workers only.
 * 
 **/
template <uint32_t ops=ePollINDefaultOps> class ePoll
{
private:
  int mPollFD;
  uint32_t maxevents;
public:
  ePoll():mPollFD(epoll_create1(O_CLOEXEC)),maxevents(0)
  {
    if(mPollFD == -1)
      throw std::system_error(errno,std::system_category(),std::string("In ePoll() constructor: "));
  }
  void add(const int fd)
  {
    epoll_event ev;
    ev.events=ops;
    ev.data.fd=fd;
    if(epoll_ctl(mPollFD,EPOLL_CTL_ADD,fd,&ev)==-1)
      throw std::system_error(errno,std::system_category(),std::string("In ePoll::add(): "));
    ++maxevents;
  }
  
  void mod(const int fd)
  {    
    epoll_event ev;
    ev.events=ops;
    ev.data.fd=fd;
    
    if(epoll_ctl(mPollFD,EPOLL_CTL_MOD,fd,&ev)==-1)
    {
      if(errno != ENOENT)
        throw std::system_error(errno,std::system_category(),std::string("In ePoll::mod(): "));
    }
    else ++maxevents;
  }
  // no maxevents decrement here, because of EPOLLONESHOT and del() only called 
  // after poll()
  void del(const int fd)
  {
    epoll_event ev;
    ev.events=ops;
    ev.data.fd=fd;
    
    if(epoll_ctl(mPollFD,EPOLL_CTL_DEL,fd,&ev)==-1)
      throw std::system_error(errno,std::system_category(),std::string("In ePoll::del(): "));
    --maxevents;
  }
  /**
   * \@brief epoll_wait wrapper. Use it within try{}catch(){}. This method
   * decreases maxevents on amount of events returned (EPOLLONESHOT).
   * You have to use mod() to set the fd back under epoll watch.
   * 
   * \@param std::vector<epoll_event>& - available events (if any)
   * 
   * \@return 0 on timeout, -1 on EINTR and throws system_error(errno),
   * or amount of events otherwise.
   **/
  int poll(std::vector<epoll_event>& out)
  {
    //out.resize(maxevents);
    int ret=0;
    while(1)
    {
      ret=epoll_wait(mPollFD,out.data(),out.size(),1);
      if((ret == -1) && (errno != EINTR))
      {
        //out.resize(0);
        throw std::system_error(
          errno,std::system_category(),std::string("In ePoll::poll(): ")
        );
      }
      else{
        //out.resize(ret);
        return ret;
      }
    }
  }
  const uint32_t getMaxEvents() const
  {
    return maxevents;
  }
};


#endif /* __EPOLL_H__ */

