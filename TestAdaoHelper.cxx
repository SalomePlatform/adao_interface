#include <vector>
#include "PyObjectRAII.hxx"

#include <cmath>

/* func pour test3DVar testBlue et testNonLinearLeastSquares*/
std::vector<double> funcBase(const std::vector<double>& vec)
{
  return {vec[0],2.*vec[1],3.*vec[2],vec[0]+2.*vec[1]+3.*vec[2]};
}

double funcCrueInternal(double Q, double K_s)
{
  constexpr double L(5.0e3);
  constexpr double B(300.);
  constexpr double Z_v(49.);
  constexpr double Z_m(51.);
  constexpr double alpha( (Z_m - Z_v)/L );
  double H(pow((Q/(K_s*B*sqrt(alpha))),(3.0/5.0)));
  return H;
}

/* func pour testCasCrue*/
std::vector<double> funcCrue(const std::vector<double>& vec)
{
  double K_s(vec[0]);
  constexpr double Qs[]={10.,20.,30.,40.};
  constexpr size_t LEN(sizeof(Qs)/sizeof(double));
  std::vector<double> ret(LEN);
  for(std::size_t i=0;i<LEN;++i)
    {
      ret[i] = funcCrueInternal(Qs[i],K_s);
    }
  return ret;
}

/* Visitor commun pour test3DVar testBlue et testNonLinearLeastSquares*/
class Visitor2 : public AdaoModel::PythonLeafVisitor
{
public:
  Visitor2(PyObject *context):_context(context)
  {
    std::vector< std::vector<double> > bounds{ {0., 10.}, {3., 13.}, {1.5, 15.5} };
    std::vector< double > Xb{5.,7.,9.};
    py2cpp::PyPtr boundsPy(py2cpp::toPyPtr(bounds));
    _bounds = boundsPy.get();
    Py_XINCREF(_bounds);
    py2cpp::PyPtr XbPy(py2cpp::toPyPtr(Xb));
    _Xb = XbPy.get();
    Py_XINCREF(_Xb);
    std::vector<double> observation{2., 6., 12., 20.};
    py2cpp::PyPtr observationPy(py2cpp::toPyPtr(observation));
    _observation = observationPy.get();
    Py_XINCREF(_observation);
  }
  
  void visit(AdaoModel::MainModel *godFather, AdaoModel::PyObjKeyVal *obj) override
  {
    if(obj->getKey()=="Bounds")
      {
        std::ostringstream oss; oss << "___" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_bounds);
        PyDict_SetItemString(_context,varname.c_str(),_bounds);
        obj->setVarName(varname);
        return ;
      }
    if(godFather->findPathOf(obj)=="Background/Vector")
      {
        std::ostringstream oss; oss << "___" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_Xb);
        PyDict_SetItemString(_context,varname.c_str(),_Xb);
        obj->setVarName(varname);
      }
    if(godFather->findPathOf(obj)=="Observation/Vector")
      {
        std::ostringstream oss; oss << "____" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_observation);
        PyDict_SetItemString(_context,varname.c_str(),_observation);
        obj->setVarName(varname);
      }
  }
private:
  unsigned int _cnt = 0;
  PyObject *_bounds = nullptr;
  PyObject *_Xb = nullptr;
  PyObject *_observation = nullptr;
  PyObject *_context = nullptr;
};

/* Visitor pour testCasCrue */
class VisitorCruePython : public AdaoModel::PythonLeafVisitor
{
public:
  VisitorCruePython(PyObject *context):_context(context)
  {
    {//case.set( 'Background',          Vector=thetaB)
      std::vector< double > Xb{ 20. };//thetaB
      py2cpp::PyPtr XbPy(py2cpp::toPyPtr(Xb));
      _Xb = XbPy.get();
      Py_XINCREF(_Xb);
    }
    {//case.set( 'BackgroundError',     DiagonalSparseMatrix=sigmaTheta )
      std::vector< double > sigmaTheta{ 5.e10 };
      py2cpp::PyPtr sigmaThetaPy(py2cpp::toPyPtr(sigmaTheta));
      _sigmaTheta = sigmaThetaPy.get();
      Py_XINCREF(_sigmaTheta);
    }
    {//case.set( 'Observation',         Vector=Hobs)
      std::vector<double> observation{0.19694513, 0.298513, 0.38073079, 0.45246109};
      py2cpp::PyPtr observationPy(py2cpp::toPyPtr(observation));
      _observation = observationPy.get();
      Py_XINCREF(_observation);
    }
    {//case.set( 'ObservationError',    ScalarSparseMatrix=sigmaH )
      double sigmaH( 0.5);
      py2cpp::PyPtr sigmaHPy(py2cpp::toPyPtr(sigmaH));
      _sigmaH = sigmaHPy.get();
      Py_XINCREF(_sigmaH);
    }
  }


  void visit(AdaoModel::MainModel *godFather, AdaoModel::PyObjKeyVal *obj) override
  {
    if(godFather->findPathOf(obj)=="Background/Vector")
      {
        std::ostringstream oss; oss << "___" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_Xb);
        PyDict_SetItemString(_context,varname.c_str(),_Xb);
        obj->setVarName(varname);
      }
    if(godFather->findPathOf(obj)=="BackgroundError/Matrix")
      {
        std::ostringstream oss; oss << "___" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_sigmaTheta);
        PyDict_SetItemString(_context,varname.c_str(),_Xb);
        obj->setVarName(varname);
      }
    if(godFather->findPathOf(obj)=="Observation/Vector")
      {
        std::ostringstream oss; oss << "____" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_observation);
        PyDict_SetItemString(_context,varname.c_str(),_observation);
        obj->setVarName(varname);
      }
    if(godFather->findPathOf(obj)=="ObservationError/Matrix")
      {
        std::ostringstream oss; oss << "____" << _cnt++;
        std::string varname(oss.str());
        obj->setVal(_sigmaH);
        PyDict_SetItemString(_context,varname.c_str(),_sigmaH);
        obj->setVarName(varname);
      }
  }
private:
  unsigned int _cnt = 0;
  PyObject *_Xb = nullptr;
  PyObject *_sigmaH = nullptr;
  PyObject *_sigmaTheta = nullptr;
  PyObject *_observation = nullptr;
  PyObject *_context = nullptr;
};

