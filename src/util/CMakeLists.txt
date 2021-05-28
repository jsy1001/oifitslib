# CMakeLists.txt for OIFITS command-line utilities
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

cmake_minimum_required(VERSION 3.13)
project(util C)

find_package(PkgConfig)
pkg_check_modules(GLIB2 glib-2.0>=2.56.0)
pkg_check_modules(CFITSIO cfitsio)

if(GLIB2_FOUND)

  include_directories(../oifitslib)
  include_directories(${CFITSIO_INCLUDE_DIRS})
  include_directories(${GLIB2_INCLUDE_DIRS})

  link_directories(${CFITSIO_LIBRARY_DIRS})
  link_directories(${GLIB2_LIBRARY_DIRS})

  add_executable(oifits-check oifits-check.c)
  add_executable(oifits-merge oifits-merge.c)
  add_executable(oifits-filter oifits-filter.c)
  add_executable(oifits-upgrade oifits-upgrade.c)
  target_link_libraries(oifits-check oifits)
  target_link_libraries(oifits-merge oifits)
  target_link_libraries(oifits-filter oifits)
  target_link_libraries(oifits-upgrade oifits)
  install(TARGETS oifits-check oifits-merge oifits-filter oifits-upgrade DESTINATION bin)

endif()
