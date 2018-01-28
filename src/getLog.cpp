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
 *  $Id: getLog.cpp December 29, 2017 10:26 PM $
 * 
 **/

#include <TSLog.h>
#include <Singleton.h>
#include <memory>

#if defined(LOG_FILE)
#define __LOG_FILE_NAME__    LOG_FILE
#else
#define __LOG_FILE_NAME__    "lapps.log"
#endif

#if defined(APP_NAME)
#define __LOG_APP_NAME__      APP_NAME
#else
#define __LOG_APP_NAME__      "LAppS"
#endif


namespace itc
{

  const std::shared_ptr<CurrentLogType>& getLog()
  {
    return itc::Singleton<CurrentLogType>::getInstance(
      itc::Singleton<itc::utils::STDOutLogThreadSafeAdapter>::getInstance(__LOG_FILE_NAME__),
      __LOG_APP_NAME__
    );
  }
}
