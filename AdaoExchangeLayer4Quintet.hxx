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

#include "Python.h"

class AdaoCallbackSt;

namespace AdaoModel
{
  class MainModel;
}

class AdaoExchangeLayer4Quintet
{
  class Internal;
public:
  AdaoExchangeLayer4Quintet();
  ~AdaoExchangeLayer4Quintet();
  void init(AdaoModel::MainModel *model);
  void execute();
  bool next(PyObject *& inputRequested);
  void setResult(PyObject *outputAssociated);
  PyObject *getResult();
private:
  void initPythonIfNeeded();
  void loadTemplate(AdaoModel::MainModel *model);
private:
  Internal *_internal = nullptr;
};
