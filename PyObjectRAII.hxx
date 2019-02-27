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

class PyObjectRAII
{
public:
  static PyObjectRAII FromBorrowed(PyObject *obj) { IncRef(obj); return PyObjectRAII(obj); }
  static PyObjectRAII FromNew(PyObject *obj) { return PyObjectRAII(obj); }
  PyObjectRAII():_obj(nullptr) { }
  PyObjectRAII(PyObjectRAII&& other):_obj(other._obj) { other._obj=nullptr; }
  PyObjectRAII(const PyObjectRAII& other):_obj(other._obj) { incRef(); }
  PyObjectRAII& operator=(PyObjectRAII&& other) { unRef(); _obj=other._obj; other._obj=nullptr; }
  PyObjectRAII& operator=(const PyObjectRAII& other) { if(_obj==other._obj) return *this; unRef(); _obj=other._obj; incRef(); return *this; }
  ~PyObjectRAII() { unRef(); }
  PyObject *retn() { incRef(); return _obj; }
  operator PyObject *() const { return _obj; }
  bool isNull() const { return _obj==nullptr; }
private:
  PyObjectRAII(PyObject *obj):_obj(obj) { }
  void unRef() { Py_XDECREF(_obj); }
  void incRef() { IncRef(_obj); }
  static void IncRef(PyObject *obj) { Py_XINCREF(obj); }
private:
  PyObject *_obj;
};

class AutoGIL
{
public:
  AutoGIL():_gstate(PyGILState_Ensure()) { }
  ~AutoGIL() { PyGILState_Release(_gstate); }
private:
  PyGILState_STATE _gstate;
};

class AutoSaveThread
{
public:
  AutoSaveThread() { unlock(); }
  ~AutoSaveThread() { lock(); }
  void lock() { PyEval_RestoreThread(_tstate); }
  void unlock() { _tstate = PyEval_SaveThread(); }
private:
  PyThreadState *_tstate;
};
