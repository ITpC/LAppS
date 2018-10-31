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
 *  $Id: ServerStats.h September 9, 2018 1:08 PM $
 * 
 **/


#ifndef __SERVERSTATS_H__
#  define __SERVERSTATS_H__

#include <Singleton.h>
#include <WSWorkersPool.h>
#include <ServiceRegistry.h>
#include <Val2Type.h>

#include <ext/json.hpp>

using json=nlohmann::json;

namespace LAppS
{
  template <const bool StatsEnable=true> class ServerStats
  {
    private:
     itc::utils::Bool2Type<StatsEnable> mStatsEnabled;
     
     
     json getStats(const itc::utils::Bool2Type<false>& stats_disabled)
     {
     }
     
     json getStats(const itc::utils::Bool2Type<true>& stats_enabled)
     {
     }
     
    public:
     auto getStats()
     {
       return getStats(mStatsEnabled);
     }
  };
}

#endif /* __SERVERSTATS_H__ */

