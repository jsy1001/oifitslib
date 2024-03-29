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

cmake_minimum_required(VERSION 3.13)
project(oifitslib C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CFITSIO REQUIRED IMPORTED_TARGET cfitsio)
pkg_check_modules(GLIB2 IMPORTED_TARGET glib-2.0>=2.56.0)

set(INCFILES chkmalloc.h datemjd.h exchange.h oifile.h oicheck.h oifilter.h oimerge.h oiiter.h)

set(oitable_SOURCES read_fits.c write_fits.c alloc_fits.c free_fits.c chkmalloc.c)
set(oifits_SOURCES ${oitable_SOURCES} datemjd.c oifile.c oifilter.c oicheck.c oimerge.c oiiter.c)

add_library(oitable SHARED ${oitable_SOURCES})
target_include_directories(oitable PUBLIC .)
target_link_libraries(oitable
  PUBLIC PkgConfig::CFITSIO
  PRIVATE m)

add_executable(oitable-demo oitable-demo.c)
target_link_libraries(oitable-demo
  PRIVATE oitable)

install(FILES ${INCFILES} DESTINATION include)
install(TARGETS oitable DESTINATION lib)
install(TARGETS oitable-demo DESTINATION bin)

if(GLIB2_FOUND)
  add_library(oifits SHARED ${oifits_SOURCES})
  target_include_directories(oifits PUBLIC .)
  target_link_libraries(oifits
    PUBLIC PkgConfig::CFITSIO PkgConfig::GLIB2
    PRIVATE m)

  if(CODE_COVERAGE AND CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    # Add required flags (GCC & LLVM/Clang)
    target_compile_options(oifits PRIVATE
      -O0        # no optimization
      -g         # generate debug info
      --coverage # sets all required flags
      )
    target_link_options(oifits PRIVATE --coverage)
  endif()

  install(TARGETS oifits DESTINATION lib)

  configure_file(oitable.pc.in oitable.pc @ONLY)
  configure_file(oifitslib.pc.in oifitslib.pc @ONLY)
  install(FILES ${PROJECT_BINARY_DIR}/oitable.pc ${PROJECT_BINARY_DIR}/oifitslib.pc DESTINATION lib/pkgconfig)
endif()

add_subdirectory(test)
