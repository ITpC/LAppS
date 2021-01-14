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
#define ePollDefaultInOps   EPOLLIN|EPOLLRDHUP|EPOLLPRI|EPOLLONESHOT
#define ePollDefaultOutOps  EPOLLOUT|EPOLLRDHUP|EPOLLPRI|EPOLLONESHOT
#define ePollDefaultBothOps EPOLLIN|EPOLLOUT|EPOLLRDHUP|EPOLLPRI|EPOLLONESHOT

/**
 * \@brief epoll wrapper (inbound messages only), not thread-safe. To embed into
 * workers only.
 * 
 **/
class ePoll
{
private:
  int mPollFD;
  
public:
  explicit ePoll():mPollFD(epoll_create1(O_CLOEXEC))
  {
    if(mPollFD == -1)
      throw std::system_error(errno,std::system_category(),"Exception in ePoll::ePoll(): ");
  }
  
  ePoll(const ePoll&)=delete;
  ePoll(ePoll&)=delete;
  
  ~ePoll()
  {
    close(mPollFD);
  }
   
  void add_in(const int fd)
  {
    epoll_event ev;
    ev.events=ePollDefaultInOps;
    ev.data.fd=fd;
    if(epoll_ctl(mPollFD,EPOLL_CTL_ADD,fd,&ev)==-1)
      throw std::system_error(errno,std::system_category(),"Exception in epoll_ctl() in ePoll::add_in(): ");
  }
  
  void add_out(const int fd)
  {
    epoll_event ev;
    ev.events=ePollDefaultOutOps;
    ev.data.fd=fd;
    if(epoll_ctl(mPollFD,EPOLL_CTL_ADD,fd,&ev)==-1)
      throw std::system_error(errno,std::system_category(),"Exception in epoll_ctl() in ePoll::add_out(): ");
  }
  
 
  
  void mod_in(const int fd)
  {    
    epoll_event ev;
    ev.events=ePollDefaultInOps;
    ev.data.fd=fd;
    
    if(epoll_ctl(mPollFD,EPOLL_CTL_MOD,fd,&ev)==-1)
    {
      if(errno != ENOENT)
        throw std::system_error(errno,std::system_category(),"Exception in epoll_ctl() in ePoll::mod_in(): ");
      else
        this->add_in(fd);
    }
  }
  
  void mod_out(const int fd)
  {    
    epoll_event ev;
    ev.events=ePollDefaultOutOps;
    ev.data.fd=fd;
    
    if(epoll_ctl(mPollFD,EPOLL_CTL_MOD,fd,&ev)==-1)
    {
      if(errno != ENOENT)
        throw std::system_error(errno,std::system_category(),"Exception in epoll_ctl() in ePoll::mod_out(): ");
      else
        this->add_out(fd);
    }
  }
  void mod_both(const int fd)
  {    
    epoll_event ev;
    ev.events=ePollDefaultBothOps;
    ev.data.fd=fd;
    
    if(epoll_ctl(mPollFD,EPOLL_CTL_MOD,fd,&ev)==-1)
    {
      if(errno != ENOENT)
        throw std::system_error(errno,std::system_category(),"Exception in epoll_ctl() in ePoll::mod_both(): ");
      else
        this->add_out(fd);
    }
  }
  
  void del(const int fd)
  {
    epoll_event ev;
    ev.events=ePollDefaultBothOps;
    ev.data.fd=fd;
    
    if(epoll_ctl(mPollFD,EPOLL_CTL_DEL,fd,&ev)==-1)
    {
      if(errno != ENOENT)
        throw std::system_error(errno,std::system_category(),"Exception in epoll_ctl() in ePoll::del(): ");
    }
  }
  /**
   * \@brief epoll_wait wrapper. Use it within try{}catch(){}. This method
   * decreases maxevents on amount of events returned (EPOLLONESHOT).
   * You have to use mod() to set the fd back under epoll watch.
   * 
   * \@param std::vector<epoll_event>& - available events (if any)
   * 
   * \@return 0 on timeout, -1 on errors other then EINTR and throws system_error(errno),
   * or amount of events otherwise.
   **/
  int poll(std::vector<epoll_event>& out, const int timeout=10)
  {
    int ret=0;
    while(true)
    {
      ret=epoll_wait(mPollFD,out.data(),out.size(),timeout);
      if(ret == -1)
      {
        if(errno != EINTR)
        throw std::system_error(
          errno,std::system_category(),"Exception in epoll_wait() in ePoll::poll(): "
        );
        ret=0;
      }
      return ret;
    }
  }
};

typedef std::shared_ptr<ePoll> SharedEPollType;

#endif /* __EPOLL_H__ */

