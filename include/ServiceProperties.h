/*
 * Copyright (C) 2018 Pavel Kraynyukhov <pk@new-web.co>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * $Id: ServiceProperties.h Created on June 7, 2018, 11:39 AM $
 */

#ifndef SERVICEPROPERTIES_H
#define SERVICEPROPERTIES_H
namespace LAppS
{
  struct ServiceProperties
  {
    const std::string service_name;
    const std::string target;
    ApplicationProtocol proto;
    const size_t max_inbound_message_size;
    const size_t instances;
  };
}
#endif /* SERVICEPROPERTIES_H */

