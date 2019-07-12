# Copyright (C) 2019 EDF R&D
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#
# Author: Anthony Geay, anthony.geay@edf.fr, EDF R&D

cmake_minimum_required(VERSION 2.6)
project(AdaoExchangeLayer)

##

set(BUILD_SHARED_LIBS TRUE)
set(CONFIGURATION_ROOT_DIR $ENV{CONFIGURATION_ROOT_DIR} CACHE PATH "Path to the Salome CMake configuration files")
if(EXISTS ${CONFIGURATION_ROOT_DIR})
  list(APPEND CMAKE_MODULE_PATH "${CONFIGURATION_ROOT_DIR}/cmake")
  include(SalomeMacros)
  include(SalomeSetupPlatform)
else(EXISTS ${CONFIGURATION_ROOT_DIR})
  message(FATAL_ERROR "We absolutely need the Salome CMake configuration files, please define CONFIGURATION_ROOT_DIR !")
endif(EXISTS ${CONFIGURATION_ROOT_DIR})

##

option(AEL_ENABLE_TESTS "Build tests (default ON)." ON)
if(AEL_ENABLE_TESTS)
  if(EXISTS ${PY2CPP_ROOT_DIR})
    set(PY2CPP_ROOT_DIR $ENV{PY2CPP_ROOT_DIR} CACHE PATH "Path to Py2cpp")
    set(Py2cpp_DIR "${PY2CPP_ROOT_DIR}/lib/cmake/py2cpp")
    find_package(Py2cpp REQUIRED)
    get_target_property(PY2CPP_INCLUDE_DIR py2cpp INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(PY2CPP_LIB py2cpp IMPORTED_LOCATION_RELEASE)
    find_package(SalomeCppUnit)
  else(EXISTS ${PY2CPP_ROOT_DIR})
    message(FATAL_ERROR "PY2CPP declared as enabled whereas not found !")
  endif(EXISTS ${PY2CPP_ROOT_DIR})
endif(AEL_ENABLE_TESTS)

##

find_package(SalomePythonInterp REQUIRED)
find_package(SalomePythonLibs REQUIRED)

##

include_directories(
  ${PYTHON_INCLUDE_DIRS}
  ${PY2CPP_INCLUDE_DIR}
  ${CPPUNIT_INCLUDE_DIRS}
  )
set(adaoexchange_SOURCES AdaoExchangeLayer.cxx AdaoModelKeyVal.cxx)
add_library(adaoexchange ${adaoexchange_SOURCES})
target_link_libraries(adaoexchange ${PYTHON_LIBRARIES})
install(FILES AdaoExchangeLayer.hxx PyObjectRAII.hxx AdaoExchangeLayerException.hxx AdaoModelKeyVal.hxx DESTINATION include)
install(TARGETS adaoexchange DESTINATION lib)

##

if(AEL_ENABLE_TESTS)
  add_executable(TestAdaoExchange TestAdaoExchange.cxx)
  target_link_libraries(TestAdaoExchange adaoexchange ${PY2CPP_LIB} ${CPPUNIT_LIBRARIES})
  install(TARGETS TestAdaoExchange DESTINATION bin)
endif(AEL_ENABLE_TESTS)
