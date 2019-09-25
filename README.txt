_internal->_py_call_back : Py function intercepting multifunc call

_internal->_decorator_func : Decoration func around _internal->_py_call_back to deal with returned values of _internal->_py_call_back

################# user GIL management in AdaoExchangeLayer

class AdaoExchangeLayer has been developped to be launched from any thread.

AdaoExchangeLayer manage the GIL (using AutoGIL) in its implementation.

Consequence the AdaoExchangeLayer instance should NOT be surrounded by GIL protection.

############## user GIL management in AdaoModel::MainModel

The custom MainModel overloading should be GIL protected by the user.
