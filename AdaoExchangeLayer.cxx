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

#include "AdaoExchangeLayer.hxx"
#include "AdaoExchangeLayerException.hxx"
#include "AdaoModelKeyVal.hxx"
#include "PyObjectRAII.hxx"
#include "Python.h"

#include "py2cpp/py2cpp.hxx"

#include <semaphore.h>

#include <iostream>
#include <sstream>
#include <clocale>
#include <cstdlib>
#include <thread>
#include <future>

struct DataExchangedBetweenThreads // data written by subthread and read by calling thread
{
public:
  DataExchangedBetweenThreads();
  ~DataExchangedBetweenThreads();
public:
  sem_t _sem;
  sem_t _sem_result_is_here;
  volatile bool _finished = false;
  volatile PyObject *_data = nullptr;
};

/////////////////////////////////////////////

struct AdaoCallbackSt
{
  PyObject_HEAD
  DataExchangedBetweenThreads *_data;
};

static PyObject *adaocallback_call(AdaoCallbackSt *self, PyObject *args, PyObject *kw)
{
  if(!PyTuple_Check(args))
    throw AdaoExchangeLayerException("Input args is not a tuple as expected !");
  if(PyTuple_Size(args)!=1)
    throw AdaoExchangeLayerException("Input args is not a tuple of size 1 as expected !");
  PyObjectRAII zeobj(PyObjectRAII::FromBorrowed(PyTuple_GetItem(args,0)));
  if(zeobj.isNull())
    throw AdaoExchangeLayerException("Retrieve of elt #0 of input tuple has failed !");
  volatile PyObject *ret(nullptr);
  PyThreadState *tstate(PyEval_SaveThread());// GIL is acquired (see ExecuteAsync). Before entering into non python section. Release lock
  {
    self->_data->_finished = false;
    self->_data->_data = zeobj;
    sem_post(&self->_data->_sem);
    sem_wait(&self->_data->_sem_result_is_here);
    ret = self->_data->_data;
  }
  PyEval_RestoreThread(tstate);//End of parallel section. Reaquire the GIL and restore the thread state
  return (PyObject *)ret;
}

static int adaocallback___init__(PyObject *self, PyObject *args, PyObject *kwargs) { return 0; }

static PyObject *adaocallback___new__(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
  return (PyObject *)( type->tp_alloc(type, 0) );
}

static void adaocallback_dealloc(PyObject *self)
{
  Py_TYPE(self)->tp_free(self);
}

PyTypeObject AdaoCallbackType = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "adaocallbacktype",
  sizeof(AdaoCallbackSt),
  0,
  adaocallback_dealloc,       /*tp_dealloc*/
  0,                          /*tp_print*/
  0,                          /*tp_getattr*/
  0,                          /*tp_setattr*/
  0,                          /*tp_compare*/
  0,                          /*tp_repr*/
  0,                          /*tp_as_number*/
  0,                          /*tp_as_sequence*/
  0,                          /*tp_as_mapping*/
  0,                          /*tp_hash*/
  (ternaryfunc)adaocallback_call,  /*tp_call*/
  0,                          /*tp_str*/
  0,                          /*tp_getattro*/
  0,                          /*tp_setattro*/
  0,                          /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
  0,                          /*tp_doc*/
  0,                          /*tp_traverse*/
  0,                          /*tp_clear*/
  0,                          /*tp_richcompare*/
  0,                          /*tp_weaklistoffset*/
  0,                          /*tp_iter*/
  0,                          /*tp_iternext*/
  0,                          /*tp_methods*/
  0,                          /*tp_members*/
  0,                          /*tp_getset*/
  0,                          /*tp_base*/
  0,                          /*tp_dict*/
  0,                          /*tp_descr_get*/
  0,                          /*tp_descr_set*/
  0,                          /*tp_dictoffset*/
  adaocallback___init__,      /*tp_init*/
  PyType_GenericAlloc,        /*tp_alloc*/
  adaocallback___new__,       /*tp_new*/
  PyObject_GC_Del,            /*tp_free*/
};

/////////////////////////////////////////////

DataExchangedBetweenThreads::DataExchangedBetweenThreads()
{
  if(sem_init(&_sem,0,0)!=0)// put value to 0 to lock by default
    throw AdaoExchangeLayerException("Internal constructor : Error on initialization of semaphore !");
  if(sem_init(&_sem_result_is_here,0,0)!=0)// put value to 0 to lock by default
    throw AdaoExchangeLayerException("Internal constructor : Error on initialization of semaphore !");
}

DataExchangedBetweenThreads::~DataExchangedBetweenThreads()
{
  sem_destroy(&_sem);
  sem_destroy(&_sem_result_is_here);
}

class AdaoCallbackKeeper
{
public:
  void assign(AdaoCallbackSt *pt, DataExchangedBetweenThreads *data)
  {
    release();
    _pt = pt;
    _pt->_data = data;
  }
  PyObject *getPyObject() const { return reinterpret_cast<PyObject*>(_pt); }
  ~AdaoCallbackKeeper() { release(); }
private:
  void release() { if(_pt) { Py_XDECREF(_pt); } }
private:
  AdaoCallbackSt *_pt = nullptr;
};

class AdaoExchangeLayer::Internal
{
public:
  Internal():_context(PyObjectRAII::FromNew(PyDict_New()))
  {
    AutoGIL agil;
    PyObject *mainmod(PyImport_AddModule("__main__"));
    PyObject *globals(PyModule_GetDict(mainmod));
    PyObject *bltins(PyEval_GetBuiltins());
    PyDict_SetItemString(_context,"__builtins__",bltins);
  }
public:
  PyObjectRAII _context;
  PyObjectRAII _generate_case_func;
  PyObjectRAII _decorator_func;
  PyObjectRAII _adao_case;
  PyObjectRAII _execute_func;
  AdaoCallbackKeeper _py_call_back;
  std::future< void > _fut;
  PyThreadState *_tstate = nullptr;
  DataExchangedBetweenThreads _data_btw_threads;
};

wchar_t **ConvertToWChar(int argc, const char *argv[])
{
  wchar_t **ret(new wchar_t*[argc]);
  for(int i=0;i<argc;++i)
    {
      std::size_t len(strlen(argv[i])+1);
      wchar_t *elt(new wchar_t[len]);
      ret[i]=elt;
      std::mbstowcs(elt, argv[i], len);
    }
  return ret;
}

void FreeWChar(int argc, wchar_t **tab)
{
  for(int i=0;i<argc;++i)
    delete [] tab[i];
  delete [] tab;
}

AdaoExchangeLayer::AdaoExchangeLayer()
{
}

AdaoExchangeLayer::~AdaoExchangeLayer()
{
  delete _internal;
}

void AdaoExchangeLayer::init()
{
  initPythonIfNeeded();
}

PyObject *AdaoExchangeLayer::getPythonContext() const
{
  if(!_internal)
    throw AdaoExchangeLayerException("getPythonContext : not initialized !");
  return _internal->_context;
}

std::string AdaoExchangeLayer::printContext() const
{
  AutoGIL agil;
  PyObject *obj(this->getPythonContext());
  if(!PyDict_Check(obj))
    throw AdaoExchangeLayerException("printContext : not a dict !");
  PyObject *key(nullptr), *value(nullptr);
  Py_ssize_t pos(0);
  std::ostringstream oss;
  while( PyDict_Next(obj, &pos, &key, &value) )
    {
      if(!PyUnicode_Check(key))
        throw AdaoExchangeLayerException("printContext : not a string as key !");
      oss << PyUnicode_AsUTF8(key) << " = ";
      PyObjectRAII reprOfValue(PyObjectRAII::FromNew(PyObject_Repr(value)));
      oss << PyUnicode_AsUTF8(reprOfValue);
      oss << std::endl;
    }
  return oss.str();
}

/*!
 * AdaoExchangeLayer is based on multithreaded paradigm.
 * Master thread (thread calling this method) and slave thread (thread calling ADAO algo)
 * are calling both python interpretor. Consequence all python calls have to be surrounded with AGIL.
 *
 * User consequence : To avoid deadlocks this method release GIL. The downstream python calls must be with AGIL.
 * 
 * This method initialize python interpretor if not already the case.
 * At the end of this method the lock is released to be ready to perform RAII on GIL
 * easily. 
 */
void AdaoExchangeLayer::initPythonIfNeeded()
{
  if (!Py_IsInitialized())
    {
      const char *TAB[]={"AdaoExchangeLayer"};
      wchar_t **TABW(ConvertToWChar(1,TAB));
      // Python is not initialized
      Py_SetProgramName(const_cast<wchar_t *>(TABW[0]));
      Py_Initialize(); // Initialize the interpreter
      PySys_SetArgv(1,TABW);
      FreeWChar(1,TABW);
      PyEval_InitThreads();
      delete _internal;
      _internal = new Internal;
      _internal->_tstate=PyEval_SaveThread(); // release the lock acquired in AdaoExchangeLayer::initPythonIfNeeded by PyEval_InitThreads()
    }
  else
    {
      delete _internal;
      _internal = new Internal;
      if( PyGILState_Check() )// is the GIL already acquired (typically by a PyEval_InitThreads) ?
        _internal->_tstate=PyEval_SaveThread(); // release the lock acquired upstream
    }
}

class Visitor1 : public AdaoModel::PythonLeafVisitor
{
public:
  Visitor1(PyObjectRAII func, PyObject *context):_func(func),_context(context)
  {
  }
  
  void visit(AdaoModel::MainModel *godFather, AdaoModel::PyObjKeyVal *obj) override
  {
    if(obj->getKey()=="Matrix" || obj->getKey()=="DiagonalSparseMatrix")
      {
        std::ostringstream oss; oss << "__" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(Py_None);
        PyDict_SetItemString(_context,varname.c_str(),Py_None);
        obj->setVarName(varname);
        return ;
      }
    if(obj->getKey()=="OneFunction")
      {
        std::ostringstream oss; oss << "__" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_func);
        PyDict_SetItemString(_context,varname.c_str(),_func);
        obj->setVarName(varname);
        return ;
      }
  }
private:
  unsigned int _cnt = 0;
  PyObjectRAII _func;
  PyObject *_context = nullptr;
};

void AdaoExchangeLayer::setFunctionCallbackInModel(AdaoModel::MainModel *model)
{
  AutoGIL agil;
  const char DECORATOR_FUNC[]="def DecoratorAdao(cppFunc):\n"
      "    def evaluator( xserie ):\n"
      "        import numpy as np\n"
      "        yserie = [np.array(elt) for elt in cppFunc(xserie)]\n"
      "        return yserie\n"
      "    return evaluator\n";
  this->_internal->_py_call_back.assign(PyObject_GC_New(AdaoCallbackSt,&AdaoCallbackType),
      &this->_internal->_data_btw_threads);
  PyObject *callbackPyObj(this->_internal->_py_call_back.getPyObject());
  //
  {
      PyObjectRAII res(PyObjectRAII::FromNew(PyRun_String(DECORATOR_FUNC,Py_file_input,this->_internal->_context,this->_internal->_context)));
      PyObjectRAII decoratorGenerator( PyObjectRAII::FromBorrowed(PyDict_GetItemString(this->_internal->_context,"DecoratorAdao")) );
      if(decoratorGenerator.isNull())
        throw AdaoExchangeLayerException("Fail to locate DecoratorAdao function !");
      PyObjectRAII args(PyObjectRAII::FromNew(PyTuple_New(1)));
      { PyTuple_SetItem(args,0,callbackPyObj); Py_XINCREF(callbackPyObj); }
      this->_internal->_decorator_func = PyObjectRAII::FromNew(PyObject_CallObject(decoratorGenerator,args));
      if(this->_internal->_decorator_func.isNull())
        throw AdaoExchangeLayerException("Fail to generate result of DecoratorAdao function !");
  }
  //
  Visitor1 visitor(this->_internal->_decorator_func,this->_internal->_context);
  model->visitPythonLeaves(&visitor);
}

void AdaoExchangeLayer::loadTemplate(AdaoModel::MainModel *model)
{
  AutoGIL agil;
  {
    std::string sciptPyOfModelMaker(model->pyStr());
    PyObjectRAII res(PyObjectRAII::FromNew(PyRun_String(sciptPyOfModelMaker.c_str(),Py_file_input,this->_internal->_context,this->_internal->_context)));
    PyErr_Print();
    _internal->_adao_case = PyObjectRAII::FromNew(PyDict_GetItemString(this->_internal->_context,"case"));
  }
  if(_internal->_adao_case.isNull())
    throw AdaoExchangeLayerException("Fail to generate ADAO case object !");
  //
  _internal->_execute_func=PyObjectRAII::FromNew(PyObject_GetAttrString(_internal->_adao_case,"execute"));
  if(_internal->_execute_func.isNull())
    throw AdaoExchangeLayerException("Fail to locate execute function of ADAO case object !");
}

void ExecuteAsync(PyObject *pyExecuteFunction, DataExchangedBetweenThreads *data)
{
  {
    AutoGIL gil; // launched in a separed thread -> protect python calls
    PyObjectRAII args(PyObjectRAII::FromNew(PyTuple_New(0)));
    PyObjectRAII nullRes(PyObjectRAII::FromNew(PyObject_CallObject(pyExecuteFunction,args)));// go to adaocallback_call
    PyErr_Print();
  }
  data->_finished = true;
  data->_data = nullptr;
  sem_post(&data->_sem);
}

void AdaoExchangeLayer::execute()
{
  _internal->_fut = std::async(std::launch::async,ExecuteAsync,_internal->_execute_func,&_internal->_data_btw_threads);
}

bool AdaoExchangeLayer::next(PyObject *& inputRequested)
{
  sem_wait(&_internal->_data_btw_threads._sem);
  if(_internal->_data_btw_threads._finished)
    {
      inputRequested = nullptr;
      return false;
    }
  else
    {
      inputRequested = (PyObject *)_internal->_data_btw_threads._data;
      return true;
    }
}

void AdaoExchangeLayer::setResult(PyObject *outputAssociated)
{
  _internal->_data_btw_threads._data = outputAssociated;
  _internal->_data_btw_threads._finished = false;
  sem_post(&_internal->_data_btw_threads._sem_result_is_here);
}

PyObject *AdaoExchangeLayer::getResult()
{
  _internal->_fut.wait();
  if(_internal->_tstate)
    PyEval_RestoreThread(_internal->_tstate);
  AutoGIL gil;
  // now retrieve case.get("Analysis")[-1]
  PyObjectRAII get_func_of_adao_case(PyObjectRAII::FromNew(PyObject_GetAttrString(_internal->_adao_case,"get")));
  if(get_func_of_adao_case.isNull())
    throw AdaoExchangeLayerException("Fail to locate \"get\" method from ADAO case !");
  PyObjectRAII all_intermediate_results;
  {// retrieve return data from case.get("Analysis")
    PyObjectRAII args(PyObjectRAII::FromNew(PyTuple_New(1)));
    PyTuple_SetItem(args,0,PyUnicode_FromString("Analysis"));
    all_intermediate_results=PyObjectRAII::FromNew(PyObject_CallObject(get_func_of_adao_case,args));
    if(all_intermediate_results.isNull())
      throw AdaoExchangeLayerException("Fail to retrieve result of case.get(\"Analysis\") !");
  }
  PyObjectRAII optimum;
  {
    PyObjectRAII param(PyObjectRAII::FromNew(PyLong_FromLong(-1)));
    optimum=PyObjectRAII::FromNew(PyObject_GetItem(all_intermediate_results,param));
    if(optimum.isNull())
      throw AdaoExchangeLayerException("Fail to retrieve result of last element of case.get(\"Analysis\") !");
  }
  /*PyObjectRAII code(PyObjectRAII::FromNew(Py_CompileString("case.get(\"Analysis\")[-1]","retrieve result",Py_file_input)));
  if(code.isNull())
    throw AdaoExchangeLayerException("Fail to compile code to retrieve result after ADAO computation !");
    PyObjectRAII res(PyObjectRAII::FromNew(PyEval_EvalCode(code,_internal->_context,_internal->_context)));*/
  return optimum.retn();
}
