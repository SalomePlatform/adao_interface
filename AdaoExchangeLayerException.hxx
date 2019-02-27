// Copyright (C) 2019 EDF R&D
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
// Author: Anthony Geay, anthony.geay@edf.fr, EDF R&D

#pragma once

#include <string>
#include <exception>

class AdaoExchangeLayerException : std::exception
{
public:
  AdaoExchangeLayerException(const std::string& what):_what(what) { }
  const char *what() noexcept { return _what.c_str(); }
  ~AdaoExchangeLayerException() noexcept { }
private:
  std::string _what;
};
