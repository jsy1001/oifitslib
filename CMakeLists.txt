# CMakeLists.txt for OIFITSlib library and command-line utilities
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

cmake_minimum_required(VERSION 2.8)
project(oifitslib C)

enable_testing()

find_package(PkgConfig)
pkg_check_modules(CFITSIO REQUIRED cfitsio)
pkg_check_modules(GLIB2 glib-2.0>=2.16.0)

include_directories(${CFITSIO_INCLUDE_DIRS})
include_directories(${GLIB2_INCLUDE_DIRS})

set(INCFILES chkmalloc.h datemjd.h exchange.h oifile.h oicheck.h oifilter.h oimerge.h oiiter.h)

set(oitable_SOURCES read_fits.c write_fits.c alloc_fits.c free_fits.c chkmalloc.c)
set(oifits_SOURCES ${oitable_SOURCES} datemjd.c oifile.c oifilter.c oicheck.c oimerge.c oiiter.c)

add_library(oitable STATIC ${oitable_SOURCES})

add_executable(demo demo.c)
target_link_libraries(demo oitable ${CFITSIO_LIBRARIES})

install(FILES ${INCFILES} DESTINATION include)
install(TARGETS oitable DESTINATION lib)
install(TARGETS demo DESTINATION bin)

if(GLIB2_FOUND)
  add_library(oifits STATIC ${oifits_SOURCES})
  add_executable(oifits-check oifits-check.c)
  add_executable(oifits-merge oifits-merge.c)
  add_executable(oifits-filter oifits-filter.c)
  add_executable(oifits-upgrade oifits-upgrade.c)
  set(util_LIBRARIES oifits ${CFITSIO_LIBRARIES} ${GLIB2_LIBRARIES} m)
  target_link_libraries(oifits-check ${util_LIBRARIES})
  target_link_libraries(oifits-merge ${util_LIBRARIES})
  target_link_libraries(oifits-filter ${util_LIBRARIES})
  target_link_libraries(oifits-upgrade ${util_LIBRARIES})

  install(TARGETS oifits DESTINATION lib)
  install(TARGETS oifits-check oifits-merge oifits-filter oifits-upgrade DESTINATION bin)

  function(add_unit_test name)
    add_executable(utest_${name} utest_${name}.c)
    target_link_libraries(utest_${name} ${util_LIBRARIES})
    add_test(NAME ${name} WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND utest_${name})
  endfunction()
  add_unit_test(datemjd)
  add_unit_test(oifile)
  add_unit_test(oicheck)
  add_unit_test(oimerge)
  add_unit_test(oifilter)
  add_unit_test(oiiter)
endif()
