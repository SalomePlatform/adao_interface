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

#include "AdaoModelKeyVal.hxx"
#include "AdaoExchangeLayerException.hxx"

#include <sstream>

using namespace AdaoModel;

const char CostDecrementTolerance::KEY[]="CostDecrementTolerance";

const char StringObserver::KEY[]="String";

const char InfoObserver::KEY[]="Info";

const char StoreSupplKeyVal::KEY[]="StoreSupplementaryCalculations";

const char *StoreSupplKeyVal::DFTL[]={"CostFunctionJAtCurrentOptimum","CostFunctionJoAtCurrentOptimum","CurrentOptimum","SimulatedObservationAtCurrentOptimum","SimulatedObservationAtOptimum"};

const char EnumAlgoKeyVal::KEY[]="Algorithm";

const char ParametersOfAlgorithmParameters::KEY[]="Parameters";

const char Bounds::KEY[]="Bounds";

const char MaximumNumberOfSteps::KEY[]="MaximumNumberOfSteps";

const char VectorBackground::KEY[]="Vector";

const char StoreBackground::KEY[]="Stored";

const char MatrixBackgroundError::KEY[]="Matrix";

const char ScalarSparseMatrixError::KEY[]="ScalarSparseMatrix";

const char DiagonalSparseMatrixError::KEY[]="DiagonalSparseMatrix";

const char OneFunction::KEY[]="OneFunction";

const char DifferentialIncrement::KEY[]="DifferentialIncrement";

const char ObservationOperatorParameters::KEY[]="Parameters";

const char CenteredFiniteDifference::KEY[]="CenteredFiniteDifference";

const char InputFunctionAsMulti::KEY[]="InputFunctionAsMulti";

const char VariableKV::KEY[]="Variable";

const char TemplateKV::KEY[]="Template";

const char AlgorithmParameters::KEY[]="AlgorithmParameters";

const char Background::KEY[]="Background";

const char BackgroundError::KEY[]="BackgroundError";

const double BackgroundError::BACKGROUND_SCALAR_SPARSE_DFT = 1.e10;

const char ObservationError::KEY[]="ObservationError";

const char ObservationOperator::KEY[]="ObservationOperator";

const char Observation::KEY[]="Observation";

const double ObservationError::BACKGROUND_SCALAR_SPARSE_DFT = 1.;

const char ObserverEntry::KEY[]="Observer";

std::string TopEntry::getParamForSet(const GenericKeyVal& entry) const
{
  std::ostringstream oss;
  oss << "case.set(\'" << entry.getKey() << "\' , **" << entry.pyStr() << ")";
  return oss.str();
}

std::string GenericKeyVal::pyStrKeyVal() const
{
  std::string val(this->pyStr());
  if( !val.empty() )
    {
      std::ostringstream oss;
      oss << "\"" << this->getKey() << "\" : " << val;
      return oss.str();
    }
  else
    return std::string();
}

void GenericKeyVal::visitAll(MainModel *godFather, RecursiveVisitor *visitor)
{
  visitor->visit(this);
}

std::string DoubleKeyVal::pyStr() const
{
  std::ostringstream oss;
  oss << std::scientific << _val;
  return oss.str();
}

std::string BoolKeyVal::pyStr() const
{
  return _val?"True":"False";
}

std::string StringKeyVal::pyStr() const
{
  std::ostringstream oss;
  oss << "\"" << _val << "\"";
  return oss.str();
}

std::string NoneKeyVal::pyStr() const
{
  return "None";
}

std::string ListStringsKeyVal::pyStr() const
{
  std::ostringstream oss;
  oss << "[ ";
  std::size_t len(_val.size());
  for(std::size_t i=0;i<len;++i)
    {
      oss << "\"" << _val[i] << "\"";
      if(i!=len-1)
        oss << ", ";
    }
  oss << " ]";
  return oss.str();
}

StoreSupplKeyVal::StoreSupplKeyVal():ListStringsKeyVal(KEY)
{
  _val.insert(_val.end(),DFTL,DFTL+sizeof(DFTL)/sizeof(char *));
}

std::shared_ptr<DictKeyVal> EnumAlgoKeyVal::generateDftParameters() const
{
  switch(_enum)
    {
    case EnumAlgo::ThreeDVar:
    case EnumAlgo::LinearLeastSquares:
    case EnumAlgo::NonLinearLeastSquares:
      {
        return templateForOthers();
      }
    case EnumAlgo::Blue:
      {
        return templateForBlue();
      }
    default:
      throw AdaoExchangeLayerException("EnumAlgoKeyVal::generateDftParameters : Unrecognized Algo !");
    }
}

std::string EnumAlgoKeyVal::getRepresentation() const
{
  switch(_enum)
    {
    case EnumAlgo::ThreeDVar:
      return "3DVAR";
    case EnumAlgo::NonLinearLeastSquares:
      return "NonLinearLeastSquares";
    case EnumAlgo::LinearLeastSquares:
      return "LinearLeastSquares";
    case EnumAlgo::Blue:
      return "Blue";
    default:
      throw AdaoExchangeLayerException("EnumAlgoKeyVal::getRepresentation : Unrecognized Algo !");
    }
}

std::string EnumAlgoKeyVal::pyStr() const
{
  std::ostringstream oss;
  oss << "\"" << this->getRepresentation() << "\"";
  return oss.str();
}

std::shared_ptr<DictKeyVal> EnumAlgoKeyVal::templateForBlue() const
{
  std::shared_ptr<DictKeyVal> ret(std::make_shared<ParametersOfAlgorithmParameters>());
  std::shared_ptr<StoreSupplKeyVal> v(std::make_shared<StoreSupplKeyVal>());
  ret->pushBack(std::static_pointer_cast<GenericKeyVal>(v));
  return ret;
}

std::shared_ptr<DictKeyVal> EnumAlgoKeyVal::templateForOthers() const
{
  std::shared_ptr<DictKeyVal> ret(std::make_shared<ParametersOfAlgorithmParameters>());
  std::shared_ptr<Bounds> v0(std::make_shared<Bounds>());
  std::shared_ptr<MaximumNumberOfSteps> v1(std::make_shared<MaximumNumberOfSteps>());
  std::shared_ptr<CostDecrementTolerance> v2(std::make_shared<CostDecrementTolerance>());
  std::shared_ptr<StoreSupplKeyVal> v3(std::make_shared<StoreSupplKeyVal>());
  ret->pushBack(std::static_pointer_cast<GenericKeyVal>(v0));
  ret->pushBack(std::static_pointer_cast<GenericKeyVal>(v1));
  ret->pushBack(std::static_pointer_cast<GenericKeyVal>(v2));
  ret->pushBack(std::static_pointer_cast<GenericKeyVal>(v3));
  return ret;
}

std::string DictKeyVal::pyStr() const
{
  std::vector<std::string> vect;
  std::size_t len(_pairs.size());
  for(std::size_t i=0;i<len;++i)
    {
      const auto& elt(_pairs[i]);
      std::string cont(elt->pyStrKeyVal());
      if( ! cont.empty() )
        vect.push_back(cont);
    }
  len = vect.size();
  std::ostringstream oss;
  oss << "{ ";
  for(std::size_t i=0;i<len;++i)
    {
      oss << vect[i];
      if(i!=len-1)
        oss << ", ";
    }
  oss << " }";
  return oss.str();
}

void DictKeyVal::visitPython(MainModel *godFather, PythonLeafVisitor *visitor)
{
  for(auto elt : _pairs)
    {
      elt->visitPython(godFather, visitor);
    }
}

void DictKeyVal::visitAll(MainModel *godFather, RecursiveVisitor *visitor)
{
  visitor->enterSubDir(this);
  for(auto elt : _pairs)
    {
      elt->visitAll(godFather, visitor);
    }
  visitor->exitSubDir(this);
}

std::string PyObjKeyVal::pyStr() const
{
  std::ostringstream oss;
  oss << _var_name;
  return oss.str();
}

void PyObjKeyVal::visitPython(MainModel *godFather, PythonLeafVisitor *visitor)
{
  visitor->visit(godFather,this);
}

std::string UnsignedIntKeyVal::pyStr() const
{
  std::ostringstream oss;
  oss << _val;
  return oss.str();
}

void InputFunctionAsMulti::setVal(bool val)
{
  if(!val)
    throw AdaoExchangeLayerException("InputFunctionAsMulti : value has to remain to true !");
}

ObservationOperatorParameters::ObservationOperatorParameters():DictKeyVal(KEY)
{
  std::shared_ptr<DifferentialIncrement> v0(std::make_shared<DifferentialIncrement>());
  std::shared_ptr<CenteredFiniteDifference> v1(std::make_shared<CenteredFiniteDifference>());
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,DifferentialIncrement>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,CenteredFiniteDifference>(v1));
}

AlgorithmParameters::AlgorithmParameters():DictKeyVal(KEY)
{
  std::shared_ptr<EnumAlgoKeyVal> v0(std::make_shared<EnumAlgoKeyVal>());
  std::shared_ptr<DictKeyVal> v1(v0->generateDftParameters());
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,EnumAlgoKeyVal>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,DictKeyVal>(v1));
}

Background::Background():DictKeyVal(KEY)
{
  std::shared_ptr<VectorBackground> v0(std::make_shared<VectorBackground>());
  std::shared_ptr<StoreBackground> v1(std::make_shared<StoreBackground>());
  v1->setVal(true);
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,VectorBackground>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,StoreBackground>(v1));
}

GenericError::GenericError(const std::string& key, double dftValForScalarSparseMatrix):DictKeyVal(key)
{
  std::shared_ptr<MatrixBackgroundError> v0(std::make_shared<MatrixBackgroundError>());
  std::shared_ptr<ScalarSparseMatrixError> v1(std::make_shared<ScalarSparseMatrixError>(dftValForScalarSparseMatrix));
  std::shared_ptr<DiagonalSparseMatrixError> v2(std::make_shared<DiagonalSparseMatrixError>());
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,MatrixBackgroundError>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,ScalarSparseMatrixError>(v1));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,DiagonalSparseMatrixError>(v2));
}

Observation::Observation():DictKeyVal(KEY)
{
  std::shared_ptr<VectorBackground> v0(std::make_shared<VectorBackground>());
  std::shared_ptr<StoreBackground> v1(std::make_shared<StoreBackground>());
  v1->setVal(false);
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,VectorBackground>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,StoreBackground>(v1));
}

ObservationOperator::ObservationOperator():DictKeyVal(KEY)
{
  std::shared_ptr<OneFunction> v0(std::make_shared<OneFunction>());
  std::shared_ptr<ObservationOperatorParameters> v1(std::make_shared<ObservationOperatorParameters>());
  std::shared_ptr<InputFunctionAsMulti> v2(std::make_shared<InputFunctionAsMulti>());
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,OneFunction>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,ObservationOperatorParameters>(v1));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,InputFunctionAsMulti>(v2));
}

ObserverEntry::ObserverEntry():DictKeyVal(KEY)
{
  std::shared_ptr<VariableKV> v0(std::make_shared<VariableKV>());
  std::shared_ptr<TemplateKV> v1(std::make_shared<TemplateKV>());
  std::shared_ptr<StringObserver> v2(std::make_shared<StringObserver>());
  std::shared_ptr<InfoObserver> v3(std::make_shared<InfoObserver>());
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,VariableKV>(v0));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,TemplateKV>(v1));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,StringObserver>(v2));
  _pairs.push_back(std::static_pointer_cast<GenericKeyVal,InfoObserver>(v3));
}

MainModel::MainModel():_algo(std::make_shared<AlgorithmParameters>()),
    _bg(std::make_shared<Background>()),
    _bg_err(std::make_shared<BackgroundError>()),
    _obs(std::make_shared<Observation>()),
    _obs_err(std::make_shared<ObservationError>()),
    _observ_op(std::make_shared<ObservationOperator>()),
    _observ_entry(std::make_shared<ObserverEntry>())
{
}

std::string MainModel::pyStr() const
{
  std::ostringstream oss;
  oss << "from adao import adaoBuilder" << std::endl;
  oss << "case = adaoBuilder.New()" << std::endl;
  oss << _algo->getParamForSet(*_algo) << std::endl;
  oss << _bg->getParamForSet(*_bg) << std::endl;
  oss << _bg_err->getParamForSet(*_bg_err) << std::endl;
  oss << _obs->getParamForSet(*_obs) << std::endl;
  oss << _obs_err->getParamForSet(*_obs_err) << std::endl;
  oss << _observ_op->getParamForSet(*_observ_op) << std::endl;
  oss << _observ_entry->getParamForSet(*_observ_entry) << std::endl;
  return oss.str();
}

std::vector< std::shared_ptr<GenericKeyVal> > MainModel::toVect() const
{
  return {
        std::static_pointer_cast<GenericKeyVal,AlgorithmParameters>(_algo),
        std::static_pointer_cast<GenericKeyVal,Background>(_bg),
        std::static_pointer_cast<GenericKeyVal,BackgroundError>(_bg_err),
        std::static_pointer_cast<GenericKeyVal,Observation>(_obs),
        std::static_pointer_cast<GenericKeyVal,ObservationError>(_obs_err),
        std::static_pointer_cast<GenericKeyVal,ObservationOperator>(_observ_op),
        std::static_pointer_cast<GenericKeyVal,ObserverEntry>(_observ_entry)
  };
}

class MyAllVisitor : public RecursiveVisitor
{
public:
  MyAllVisitor(GenericKeyVal *elt):_elt_to_find(elt) { }
  void visit(GenericKeyVal *elt) override
      {
      if(_found) return ;
      if(elt != _elt_to_find) return ;
      _path.push_back(elt->getKey());
      _found=true;
      }
  void enterSubDir(DictKeyVal *subdir) override
      {
      if(_found) return ;
      _path.push_back(subdir->getKey());
      }
  void exitSubDir(DictKeyVal *subdir) override
      {
      if(_found) return ;
      _path.pop_back();
      }
  std::string getPath() const
  {
    std::ostringstream oss;
    std::size_t len(_path.size()),ii(0);
    for(auto elt : _path)
      {
        oss << elt;
        if(ii != len-1)
          oss << "/";
        ii++;
      }
    return oss.str();
  }
private:
  GenericKeyVal *_elt_to_find = nullptr;
  std::vector<std::string> _path;
  bool _found = false;
};

std::string MainModel::findPathOf(GenericKeyVal *elt)
{
  MyAllVisitor vis(elt);
  this->visitAll(&vis);
  return vis.getPath();
}

void MainModel::visitPythonLeaves(PythonLeafVisitor *visitor)
{
  std::vector< std::shared_ptr<GenericKeyVal> > sons(toVect());
  for(auto elt : sons)
    {
      elt->visitPython(this, visitor);
    }
}

void MainModel::visitAll(RecursiveVisitor *visitor)
{
  std::vector< std::shared_ptr<GenericKeyVal> > sons(toVect());
  for(auto elt : sons)
    {
      elt->visitAll(this, visitor);
    }
}
