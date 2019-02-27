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

#include "PyObjectRAII.hxx"

#include <string>
#include <memory>
#include <vector>

namespace AdaoModel
{
  enum class Type
  {
      Double,
      UnsignedInt,
      Bool,
      String,
      PyObj,
      EnumAlgo,
      ListStrings,
      None,
      Child
  };

  enum class EnumAlgo
  {
      ThreeDVar,
      Blue,
      NonLinearLeastSquares
  };

  class GenericKeyVal;
  class MainModel;
  
  class TopEntry
  {
  public:
    std::string getParamForSet(const GenericKeyVal& entry) const;
  };
  
  class PythonLeafVisitor;
  class RecursiveVisitor;

  class GenericKeyVal
  {
  public:
    virtual bool isNeeded() const = 0;
    virtual Type getType() const = 0;
    virtual std::string pyStr() const = 0;
    virtual void visitPython(MainModel *godFather, PythonLeafVisitor *visitor) { }
    virtual void visitAll(MainModel *godFather, RecursiveVisitor *visitor);
    virtual ~GenericKeyVal() { }
    std::string pyStrKeyVal() const;
    std::string getKey() const { return _key; }
  protected:
    GenericKeyVal(const std::string& key):_key(key) { }
  private:
    std::string _key;
  };

  class NeededGenericKeyVal : public GenericKeyVal
  {
  protected:
    NeededGenericKeyVal(const std::string& key):GenericKeyVal(key) { }
  public:
    bool isNeeded() const override { return true; }
  };

  class NotNeededGenericKeyVal : public GenericKeyVal
  {
  public:
    bool isNeeded() const override { return false; }
  };

  class DoubleKeyVal : public NeededGenericKeyVal
  {
  public:
    DoubleKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    Type getType() const override { return Type::Double; }
    void setVal(double val) { _val=val; }
    double getVal() const { return _val; }
    std::string pyStr() const override;
  private:
    double _val = 0.;
  };

  class BoolKeyVal : public NeededGenericKeyVal
  {
  public:
    BoolKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    Type getType() const override { return Type::Bool; }
    virtual void setVal(bool val) { _val=val; }
    bool getVal() const { return _val; }
    std::string pyStr() const override;
  private:
    bool _val = false;
  };

  class CostDecrementTolerance : public DoubleKeyVal
  {
  public:
    CostDecrementTolerance():DoubleKeyVal(KEY) { setVal(1e-7); }
  public:
    static const char KEY[];
  };
  
  class StringKeyVal : public NeededGenericKeyVal
  {
  public:
    StringKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    Type getType() const override { return Type::String; }
    std::string pyStr() const override;
    void setVal(const std::string& val) { _val = val; }
    std::string getVal() const { return _val; }
  private:
    std::string _val;
  };

  class NoneKeyVal : public NeededGenericKeyVal
  {
  public:
    NoneKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    Type getType() const override { return Type::None; }
    std::string pyStr() const override;
  };

  class StringObserver : public NoneKeyVal
  {
  public:
    StringObserver():NoneKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class InfoObserver : public NoneKeyVal
  {
  public:
    InfoObserver():NoneKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class ListStringsKeyVal : public NeededGenericKeyVal
  {
  protected:
    ListStringsKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
  public:
    Type getType() const override { return Type::ListStrings; }
    std::string pyStr() const override;
  protected:
    std::vector< std::string > _val;
  };

  class StoreSupplKeyVal : public ListStringsKeyVal
  {
  public:
    StoreSupplKeyVal();
  public:
    static const char *DFTL[];
    static const char KEY[];
  };

  class DictKeyVal;
  
  class EnumAlgoKeyVal : public NeededGenericKeyVal
  {
  public:
    EnumAlgoKeyVal():NeededGenericKeyVal(KEY),_enum(EnumAlgo::ThreeDVar) { }
    Type getType() const override { return Type::EnumAlgo; }
    std::shared_ptr<DictKeyVal> generateDftParameters() const;
    std::string getRepresentation() const;
    std::string pyStr() const override;
    void setVal(EnumAlgo newAlgo) { _enum = newAlgo; }
    EnumAlgo getVal() const { return _enum; }
  private:
    std::shared_ptr<DictKeyVal> templateForBlue() const;
    std::shared_ptr<DictKeyVal> templateForOthers() const;
  private:
    static const char KEY[];
    EnumAlgo _enum;
  };
  
  class DictKeyVal : public NeededGenericKeyVal
  {
  public:
    DictKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    void pushBack(std::shared_ptr<GenericKeyVal> elt) { _pairs.push_back(elt); }
    Type getType() const override { return Type::Child; }
    std::string pyStr() const override;
    void visitPython(MainModel *godFather, PythonLeafVisitor *visitor) override;
    void visitAll(MainModel *godFather, RecursiveVisitor *visitor) override;
  protected:
    std::vector< std::shared_ptr<GenericKeyVal> > _pairs;
  };

  class ParametersOfAlgorithmParameters : public DictKeyVal
  {
  public:
    ParametersOfAlgorithmParameters():DictKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class PyObjKeyVal : public NeededGenericKeyVal
  {
  public:
    PyObjKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    Type getType() const override { return Type::PyObj; }
    void setVal(PyObject *obj) { _val = PyObjectRAII::FromBorrowed(obj); }
    // borrowed ref
    PyObject *getVal() const { return _val; }
    std::string pyStr() const;
    void setVarName(const std::string& vn) { _var_name = vn; }
    void visitPython(MainModel *godFather, PythonLeafVisitor *visitor) override;
  private:
    PyObjectRAII _val;
    std::string _var_name;
  };

  class Bounds : public PyObjKeyVal
  {
  public:
    Bounds():PyObjKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class UnsignedIntKeyVal : public NeededGenericKeyVal
  {
  public:
    UnsignedIntKeyVal(const std::string& key):NeededGenericKeyVal(key) { }
    void setVal(unsigned int val) { _val=val; }
    unsigned int getVal() const { return _val; }
    Type getType() const override { return Type::UnsignedInt; }
    std::string pyStr() const;
  private:
    unsigned int _val = -1;
  };
  
  class MaximumNumberOfSteps : public UnsignedIntKeyVal
  {
  public:
    MaximumNumberOfSteps():UnsignedIntKeyVal(KEY) { setVal(100); }
  public:
    static const char KEY[];
  };

  class VectorBackground : public PyObjKeyVal
  {
  public:
    VectorBackground():PyObjKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class StoreBackground : public BoolKeyVal
  {
  public:
    StoreBackground():BoolKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class MatrixBackgroundError : public PyObjKeyVal
  {
  public:
    MatrixBackgroundError():PyObjKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class ScalarSparseMatrixError : public DoubleKeyVal
  {
  public:
    ScalarSparseMatrixError(double val):DoubleKeyVal(KEY) { setVal(val); }
  public:
    static const char KEY[];
  };

  class DiagonalSparseMatrixError : public PyObjKeyVal
  {
  public:
    DiagonalSparseMatrixError():PyObjKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class OneFunction : public PyObjKeyVal
  {
  public:
    OneFunction():PyObjKeyVal(KEY) { }
  public:
    static const char KEY[];
  };

  class DifferentialIncrement : public DoubleKeyVal
  {
  public:
    DifferentialIncrement():DoubleKeyVal(KEY) { setVal(0.0001); }
  public:
    static const char KEY[];
  };

  class ObservationOperatorParameters : public DictKeyVal
  {
  public:
    ObservationOperatorParameters();
  public:
    static const char KEY[];
  };

  class CenteredFiniteDifference : public BoolKeyVal
  {
  public:
    CenteredFiniteDifference():BoolKeyVal(KEY) { setVal(false); }
  public:
    static const char KEY[];
  };

  class InputFunctionAsMulti : public BoolKeyVal
  {
  public:
    InputFunctionAsMulti():BoolKeyVal(KEY) { BoolKeyVal::setVal(true); }
    void setVal(bool val) override;
  public:
    static const char KEY[];
  };

  class VariableKV : public StringKeyVal
  {
  public:
    VariableKV():StringKeyVal(KEY) { setVal("CurrentState"); }
  public:
    static const char KEY[];
  };

  class TemplateKV : public StringKeyVal
  {
  public:
    TemplateKV():StringKeyVal(KEY) { setVal("CurrentState"); }
  public:
    static const char KEY[];
  };

  //////////////

  class AlgorithmParameters : public DictKeyVal, public TopEntry
  {
  public:
    AlgorithmParameters();
  public:
    static const char KEY[];
  };

  class Background : public DictKeyVal, public TopEntry
  {
  public:
    Background();
  public:
    static const char KEY[];
  };

  class GenericError : public DictKeyVal, public TopEntry
  {
  protected:
    GenericError(const std::string& key, double dftValForScalarSparseMatrix);
  };

  class BackgroundError : public GenericError
  {
  public:
    BackgroundError():GenericError(KEY,BACKGROUND_SCALAR_SPARSE_DFT) { }
  public:
    static const char KEY[];
    static const double BACKGROUND_SCALAR_SPARSE_DFT;
  };

  class ObservationError : public GenericError
  {
  public:
    ObservationError():GenericError(KEY,BACKGROUND_SCALAR_SPARSE_DFT) { }
  public:
    static const char KEY[];
    static const double BACKGROUND_SCALAR_SPARSE_DFT;
  };

  class ObservationOperator : public DictKeyVal, public TopEntry
  {
  public:
    ObservationOperator();
  public:
    static const char KEY[];
  };

  class Observation : public DictKeyVal, public TopEntry
  {
  public:
    Observation();
  public:
    static const char KEY[];
  };

  class ObserverEntry : public DictKeyVal, public TopEntry
  {
  public:
    ObserverEntry();
  public:
    static const char KEY[];
  };

  ///////////////

  class PythonLeafVisitor
  {
  public:
    virtual void visit(MainModel *godFather, PyObjKeyVal *obj) = 0;
    virtual ~PythonLeafVisitor() { }
  };

  class RecursiveVisitor
  {
  public:
    virtual ~RecursiveVisitor() { }
    virtual void visit(GenericKeyVal *obj) = 0;
    virtual void enterSubDir(DictKeyVal *subdir) = 0;
    virtual void exitSubDir(DictKeyVal *subdir) = 0;
  };

  class MainModel
  {
  public:
    MainModel();
    std::string findPathOf(GenericKeyVal *elt);
    std::string pyStr() const;
    std::vector< std::shared_ptr<GenericKeyVal> > toVect() const;
    void visitPythonLeaves(PythonLeafVisitor *visitor);
    void visitAll(RecursiveVisitor *visitor);
  private:
    std::shared_ptr<AlgorithmParameters> _algo;
    std::shared_ptr<Background> _bg;
    std::shared_ptr<BackgroundError> _bg_err;
    std::shared_ptr<Observation> _obs;
    std::shared_ptr<ObservationError> _obs_err;
    std::shared_ptr<ObservationOperator> _observ_op;
    std::shared_ptr<ObserverEntry> _observ_entry;
  };
}
