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

#include "TestAdaoExchange.hxx"

#include "AdaoExchangeLayer.hxx"
#include "AdaoExchangeLayerException.hxx"
#include "AdaoModelKeyVal.hxx"
#include "PyObjectRAII.hxx"

#include "py2cpp/py2cpp.hxx"

#include <vector>
#include <iterator>

#include "TestAdaoHelper.cxx"

// Functor a remplacer par un appel a un evaluateur parallele
class NonParallelFunctor
{
public:
  NonParallelFunctor(std::function< std::vector<double>(const std::vector<double>&) > cppFunction):_cpp_function(cppFunction) { }
  PyObject *operator()(PyObject *inp) const
  {
    PyGILState_STATE gstate(PyGILState_Ensure());
    PyObjectRAII iterator(PyObjectRAII::FromNew(PyObject_GetIter(inp)));
    if(iterator.isNull())
      throw AdaoExchangeLayerException("Input object is not iterable !");
    PyObject *item(nullptr);
    PyObjectRAII numpyModule(PyObjectRAII::FromNew(PyImport_ImportModule("numpy")));
    if(numpyModule.isNull())
      throw AdaoExchangeLayerException("Failed to load numpy");
    PyObjectRAII ravelFunc(PyObjectRAII::FromNew(PyObject_GetAttrString(numpyModule,"ravel")));
    std::vector< PyObjectRAII > pyrets;
    while( item = PyIter_Next(iterator) )
      {
        PyObjectRAII item2(PyObjectRAII::FromNew(item));
        {
          PyObjectRAII args(PyObjectRAII::FromNew(PyTuple_New(1)));
          PyTuple_SetItem(args,0,item2.retn());
          PyObjectRAII npyArray(PyObjectRAII::FromNew(PyObject_CallObject(ravelFunc,args)));
          // Waiting management of npy arrays into py2cpp
          PyObjectRAII lolistFunc(PyObjectRAII::FromNew(PyObject_GetAttrString(npyArray,"tolist")));
          PyObjectRAII listPy;
          {
            PyObjectRAII args2(PyObjectRAII::FromNew(PyTuple_New(0)));
            listPy=PyObjectRAII::FromNew(PyObject_CallObject(lolistFunc,args2));
          }
          std::vector<double> vect;
          {
            py2cpp::PyPtr listPy2(listPy.retn());
            py2cpp::fromPyPtr(listPy2,vect);
          }
          //
          PyGILState_Release(gstate);
          std::vector<double> res(_cpp_function(vect));// L'appel effectif est ici
          gstate=PyGILState_Ensure();
          //
          py2cpp::PyPtr resPy(py2cpp::toPyPtr(res));
          PyObjectRAII resPy2(PyObjectRAII::FromBorrowed(resPy.get()));
          pyrets.push_back(resPy2);
        }
      }
    std::size_t len(pyrets.size());
    PyObjectRAII ret(PyObjectRAII::FromNew(PyList_New(len)));
    for(std::size_t i=0;i<len;++i)
      {
        PyList_SetItem(ret,i,pyrets[i].retn());
      }
    //PyObject *tmp(PyObject_Repr(ret));
    // std::cerr << PyUnicode_AsUTF8(tmp) << std::endl;
    PyGILState_Release(gstate);
    return ret.retn();
  }
private:
  std::function< std::vector<double>(const std::vector<double>&) > _cpp_function;
};

PyObjectRAII NumpyToListWaitingForPy2CppManagement(PyObject *npObj)
{
  PyObjectRAII func(PyObjectRAII::FromNew(PyObject_GetAttrString(npObj,"tolist")));
  if(func.isNull())
    throw AdaoExchangeLayerException("input pyobject does not have tolist attribute !");
  PyObjectRAII args(PyObjectRAII::FromNew(PyTuple_New(0)));
  PyObjectRAII ret(PyObjectRAII::FromNew(PyObject_CallObject(func,args)));
  return ret;
}

void AdaoExchangeTest::setUp()
{
}

void AdaoExchangeTest::tearDown()
{
}

void AdaoExchangeTest::cleanUp()
{
}

using namespace AdaoModel;

void AdaoExchangeTest::test3DVar()
{
  NonParallelFunctor functor(funcBase);
  MainModel mm;
  AdaoExchangeLayer adao;
  adao.init();
  // For bounds, Background/Vector, Observation/Vector
  Visitor2 visitorPythonObj(adao.getPythonContext());
  mm.visitPythonLeaves(&visitorPythonObj);
  //
  adao.loadTemplate(&mm);
  //
  {
    std::string sciptPyOfModelMaker(mm.pyStr());
    //std::cerr << sciptPyOfModelMaker << std::endl;
  }
  adao.execute();
  PyObject *listOfElts( nullptr );
  while( adao.next(listOfElts) )
    {
      PyObject *resultOfChunk(functor(listOfElts));
      adao.setResult(resultOfChunk);
    }
  PyObject *res(adao.getResult());
  PyObjectRAII optimum(PyObjectRAII::FromNew(res));
  PyObjectRAII optimum_4_py2cpp(NumpyToListWaitingForPy2CppManagement(optimum));
  std::vector<double> vect;
  {
    py2cpp::PyPtr obj(optimum_4_py2cpp);
    py2cpp::fromPyPtr(obj,vect);
  }
  CPPUNIT_ASSERT_EQUAL(3,(int)vect.size());
  CPPUNIT_ASSERT_DOUBLES_EQUAL(2.,vect[0],1e-7);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(3.,vect[1],1e-7);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(4.,vect[2],1e-7);
}

void AdaoExchangeTest::testBlue()
{
  class TestBlueVisitor : public RecursiveVisitor
  {
  public:
    void visit(GenericKeyVal *obj)
    {
      EnumAlgoKeyVal *objc(dynamic_cast<EnumAlgoKeyVal *>(obj));
      if(objc)
        objc->setVal(EnumAlgo::Blue);
    }
    void enterSubDir(DictKeyVal *subdir) { }
    void exitSubDir(DictKeyVal *subdir) { }
  };

  NonParallelFunctor functor(funcBase);
  MainModel mm;
  //
  TestBlueVisitor vis;
  mm.visitAll(&vis);
  //
  AdaoExchangeLayer adao;
  adao.init();
  // For bounds, Background/Vector, Observation/Vector
  Visitor2 visitorPythonObj(adao.getPythonContext());
  mm.visitPythonLeaves(&visitorPythonObj);
  //
  adao.loadTemplate(&mm);
  //
  {
    std::string sciptPyOfModelMaker(mm.pyStr());
    //std::cerr << sciptPyOfModelMaker << std::endl;
  }
  adao.execute();
    PyObject *listOfElts( nullptr );
    while( adao.next(listOfElts) )
      {
        PyObject *resultOfChunk(functor(listOfElts));
        adao.setResult(resultOfChunk);
      }
    PyObject *res(adao.getResult());
    PyObjectRAII optimum(PyObjectRAII::FromNew(res));
    PyObjectRAII optimum_4_py2cpp(NumpyToListWaitingForPy2CppManagement(optimum));
    std::vector<double> vect;
    {
      py2cpp::PyPtr obj(optimum_4_py2cpp);
      py2cpp::fromPyPtr(obj,vect);
    }
    CPPUNIT_ASSERT_EQUAL(3,(int)vect.size());
    CPPUNIT_ASSERT_DOUBLES_EQUAL(2.,vect[0],1e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(3.,vect[1],1e-7);
    CPPUNIT_ASSERT_DOUBLES_EQUAL(4.,vect[2],1e-7);
}

void AdaoExchangeTest::testNonLinearLeastSquares()
{
  class TestNonLinearLeastSquaresVisitor : public RecursiveVisitor
  {
  public:
    void visit(GenericKeyVal *obj)
    {
      EnumAlgoKeyVal *objc(dynamic_cast<EnumAlgoKeyVal *>(obj));
      if(objc)
        objc->setVal(EnumAlgo::NonLinearLeastSquares);
    }
    void enterSubDir(DictKeyVal *subdir) { }
    void exitSubDir(DictKeyVal *subdir) { }
  };
  NonParallelFunctor functor(funcBase);
  MainModel mm;
  //
  TestNonLinearLeastSquaresVisitor vis;
  mm.visitAll(&vis);
  //
  AdaoExchangeLayer adao;
  adao.init();
  // For bounds, Background/Vector, Observation/Vector
  Visitor2 visitorPythonObj(adao.getPythonContext());
  mm.visitPythonLeaves(&visitorPythonObj);
  //
  adao.loadTemplate(&mm);
  //
  {
    std::string sciptPyOfModelMaker(mm.pyStr());
    //std::cerr << sciptPyOfModelMaker << std::endl;
  }
  adao.execute();
  PyObject *listOfElts( nullptr );
  while( adao.next(listOfElts) )
    {
      PyObject *resultOfChunk(functor(listOfElts));
      adao.setResult(resultOfChunk);
    }
  PyObject *res(adao.getResult());
  PyObjectRAII optimum(PyObjectRAII::FromNew(res));
  PyObjectRAII optimum_4_py2cpp(NumpyToListWaitingForPy2CppManagement(optimum));
  std::vector<double> vect;
  {
    py2cpp::PyPtr obj(optimum_4_py2cpp);
    py2cpp::fromPyPtr(obj,vect);
  }
  CPPUNIT_ASSERT_EQUAL(3,(int)vect.size());
  CPPUNIT_ASSERT_DOUBLES_EQUAL(2.,vect[0],1e-7);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(3.,vect[1],1e-7);
  CPPUNIT_ASSERT_DOUBLES_EQUAL(4.,vect[2],1e-7);
}

void AdaoExchangeTest::testCasCrue()
{
  NonParallelFunctor functor(funcCrue);
  MainModel mm;
  AdaoExchangeLayer adao;
  adao.init();
  // For bounds, Background/Vector, Observation/Vector
  VisitorCruePython visitorPythonObj(adao.getPythonContext());
  mm.visitPythonLeaves(&visitorPythonObj);
  //
  adao.loadTemplate(&mm);
  //
  {
    std::string sciptPyOfModelMaker(mm.pyStr());
    //std::cerr << sciptPyOfModelMaker << std::endl;
  }
  adao.execute();
  PyObject *listOfElts( nullptr );
  while( adao.next(listOfElts) )
    {
      PyObject *resultOfChunk(functor(listOfElts));
      adao.setResult(resultOfChunk);
    }
  PyObject *res(adao.getResult());
  PyObjectRAII optimum(PyObjectRAII::FromNew(res));
  PyObjectRAII optimum_4_py2cpp(NumpyToListWaitingForPy2CppManagement(optimum));
  std::vector<double> vect;
  {
    py2cpp::PyPtr obj(optimum_4_py2cpp);
    py2cpp::fromPyPtr(obj,vect);
  }
  CPPUNIT_ASSERT_EQUAL(1,(int)vect.size());
  CPPUNIT_ASSERT_DOUBLES_EQUAL(25.,vect[0],1e-3);
}

CPPUNIT_TEST_SUITE_REGISTRATION( AdaoExchangeTest );

#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextTestRunner.h>

int main(int argc, char* argv[])
{
  // --- Create the event manager and test controller
  CPPUNIT_NS::TestResult controller;

  // ---  Add a listener that collects test result
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );        

  // ---  Add a listener that print dots as test run.
#ifdef WIN32
  CPPUNIT_NS::TextTestProgressListener progress;
#else
  CPPUNIT_NS::BriefTestProgressListener progress;
#endif
  controller.addListener( &progress );      

  // ---  Get the top level suite from the registry

  CPPUNIT_NS::Test *suite =
    CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();

  // ---  Adds the test to the list of test to run

  CPPUNIT_NS::TestRunner runner;
  runner.addTest( suite );
  runner.run( controller);

  // ---  Print test in a compiler compatible format.
  std::ofstream testFile;
  testFile.open("test.log", std::ios::out | std::ios::app);
  testFile << "------ ADAO exchange test log:" << std::endl;
  CPPUNIT_NS::CompilerOutputter outputter( &result, testFile );
  outputter.write(); 

  // ---  Run the tests.

  bool wasSucessful = result.wasSuccessful();
  testFile.close();

  // ---  Return error code 1 if the one of test failed.

  return wasSucessful ? 0 : 1;
}
