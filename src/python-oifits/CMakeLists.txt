# CMakeLists.txt for unit tests of OIFITSlib python interface
#
# This file is part of OIFITSlib.
#
# OIFITSlib is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# OIFITSlib is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with OIFITSlib.  If not, see
# http://www.gnu.org/licenses/

cmake_minimum_required(VERSION 3.12)
project(python-oifits)

set(TESTDATA_DIR ${PROJECT_SOURCE_DIR}/../../testdata)

find_package(PkgConfig)
pkg_check_modules(GLIB2 glib-2.0>=2.16.0)
pkg_check_modules(CFITSIO cfitsio)

if(GLIB2_FOUND)

  find_package(SWIG)
  if(SWIG_FOUND)

    include(${SWIG_USE_FILE})

    find_package(Python2 COMPONENTS Interpreter Development)

    function(add_swig_test module)
      add_test(NAME python-${module} WORKING_DIRECTORY ${TESTDATA_DIR} COMMAND ${Python2_EXECUTABLE} ${PROJECT_BINARY_DIR}/${module}.py)
    endfunction()

    function(install_swig module)
      install(TARGETS _${module} DESTINATION ${Python2_SITELIB})
      install(FILES ${PROJECT_BINARY_DIR}/${module}.py DESTINATION ${Python2_SITELIB})
    endfunction()

    include_directories(../oifitslib)
    include_directories(${CFITSIO_INCLUDE_DIRS})
    include_directories(${GLIB2_INCLUDE_DIRS})
    include_directories(${Python2_INCLUDE_DIRS})

    link_directories(${GLIB2_LIBRARY_DIRS})

    swig_add_library(oifits LANGUAGE python SOURCES oifits.i)
    swig_add_library(oifilter LANGUAGE python SOURCES oifilter.i)
    swig_add_library(oicheck LANGUAGE python SOURCES oicheck.i)
    swig_add_library(oimerge LANGUAGE python SOURCES oimerge.i)
    swig_link_libraries(oifits oifits ${Python2_LIBRARIES})
    swig_link_libraries(oifilter oifits ${Python2_LIBRARIES})
    swig_link_libraries(oicheck oifits ${Python2_LIBRARIES})
    swig_link_libraries(oimerge oifits ${Python2_LIBRARIES})
    install_swig(oifits)
    install_swig(oifilter)
    install_swig(oicheck)
    install_swig(oimerge)

    add_swig_test(oifits)
    add_swig_test(oifilter)
    add_swig_test(oicheck)
    add_swig_test(oimerge)

  endif()

endif()
