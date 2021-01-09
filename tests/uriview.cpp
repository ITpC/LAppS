/*
 * Copyright (C) 2021 pk
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
 */

/* 
 * File:   uriview.cpp
 * Author: pk
 *
 * Created on January 9, 2021, 4:08 PM
 */

#include <URIView.h>
#include <iostream>
#include <fstream>
#include <fmt/core.h>
#include <string_view>
#include <string>

int main()
{

  std::string uri;
  URIView uriparser;

  std::ifstream cases("data/uris");
  while (std::getline(cases, uri))
  {
    if(uri[0] == '#')
    {
      std::cout << uri << std::endl;
      continue;
    }
    auto pbytes=uriparser.parse({uri.data(),uri.length()});
    std::cout << fmt::format("PASSED({}). Parsed {} bytes of {} for URI [{}]",uriparser.valid, pbytes, uri.length(), uri) << std::endl;
  }
  return 0;
}


