# CMakeLists.txt for unit tests of OIFITSlib libraries
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
project(oifitslib-test C)

set(TESTDATA_DIR ${PROJECT_SOURCE_DIR}/../../../testdata)

find_package(PkgConfig)
pkg_check_modules(GLIB2 glib-2.0>=2.56.0)
pkg_check_modules(CFITSIO cfitsio)

if(GLIB2_FOUND)

  function(add_unit_test name)
    add_executable(utest_${name} utest_${name}.c)
    target_link_libraries(utest_${name}
      PRIVATE oifits m)
    add_test(NAME ${name} WORKING_DIRECTORY ${TESTDATA_DIR} COMMAND utest_${name})
  endfunction()

  add_unit_test(chkmalloc)
  add_unit_test(datemjd)
  add_unit_test(oifile)
  add_unit_test(oicheck)
  add_unit_test(oimerge)
  add_unit_test(oifilter)
  add_unit_test(oiiter)

endif()
