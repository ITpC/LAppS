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
 *  $Id: Application.h February 4, 2018 5:58 PM $
 * 
 **/


#ifndef __APPLICATION_H__
#  define __APPLICATION_H__

#include <string>
#include <fstream>
#include <memory>

#include <ThreadPoolManager.h>

#include <ext/json.hpp>

using json = nlohmann::json;

namespace LAppS
{
  class Application
  {
  private:
   typedef itc::sys::CancelableThread<itc::ThreadPoolManager> TPManager;
    std::string mName;
    std::string mWSPath;
    std::string mConfFile;
    json        mAppConfig;
    std::shared_ptr<TPManager>  mTPMan;
    
  public:
    explicit Application(
      const std::string& appName,
      const std::string& appWSPath,
      const std::string& conf_file
    ) : mName(appName),mWSPath(appWSPath),mConfFile(conf_file)
    {
      std::ifstream configStream(mConfFile, std::ifstream::binary);
      if(configStream)
      {
        configStream >> mAppConfig;
        configStream.close();
        
        auto it=mAppConfig.find("TPMan");
        if(it==mAppConfig.end()) // set defailts;
        {
          mAppConfig["TPMan"]["maxThreads"]=2;
          mAppConfig["TPMan"]["overcommitThreads"]=2;
          mAppConfig["TPMan"]["purgeTimeout"]=10000;
          mAppConfig["TPMan"]["minThreadsReady"]=1;
        }
        mTPMan=std::make_shared<TPManager>(
          std::make_shared<itc::ThreadPoolManager>(
            mAppConfig["TPMan"]["maxThreads"],
            mAppConfig["TPMan"]["overcommitThreads"],
            mAppConfig["TPMan"]["purgeTimeout"],
            mAppConfig["TPMan"]["minThreadsReady"]
          )
        );
      }
    }
    Application()=delete;
    Application(const Application&)=delete;
    Application(Application&)=delete;
    
  };
}

#endif /* __APPLICATION_H__ */

