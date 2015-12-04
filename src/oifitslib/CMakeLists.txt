# CMakeLists.txt for OIFITSlib libraries
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

find_package(PkgConfig)
pkg_check_modules(CFITSIO REQUIRED cfitsio)
pkg_check_modules(GLIB2 glib-2.0>=2.16.0)

set(INCFILES chkmalloc.h datemjd.h exchange.h oifile.h oicheck.h oifilter.h oimerge.h oiiter.h)

set(oitable_SOURCES read_fits.c write_fits.c alloc_fits.c free_fits.c chkmalloc.c)
set(oifits_SOURCES ${oitable_SOURCES} datemjd.c oifile.c oifilter.c oicheck.c oimerge.c oiiter.c)

include_directories(${CFITSIO_INCLUDE_DIRS})
include_directories(${GLIB2_INCLUDE_DIRS})

add_library(oitable STATIC ${oitable_SOURCES})
target_link_libraries(oitable PUBLIC ${CFITSIO_LIBRARIES})

add_executable(oitable-demo oitable-demo.c)
target_link_libraries(oitable-demo oitable)

install(FILES ${INCFILES} DESTINATION include)
install(TARGETS oitable DESTINATION lib)
install(TARGETS oitable-demo DESTINATION bin)

if(GLIB2_FOUND)
  add_library(oifits SHARED ${oifits_SOURCES})
  target_link_libraries(oifits PUBLIC ${CFITSIO_LIBRARIES} ${GLIB2_LIBRARIES})
  target_link_libraries(oifits PRIVATE m)
  install(TARGETS oifits DESTINATION lib)
endif()

add_subdirectory(test)
