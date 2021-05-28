OIFITSlib
=========

[![CMake](https://github.com/jsy1001/oifitslib/actions/workflows/cmake.yml/badge.svg)](https://github.com/jsy1001/oifitslib/actions/workflows/cmake.yml)
[![codecov](https://codecov.io/gh/jsy1001/oifitslib/branch/main/graph/badge.svg?token=J43UX3RVB8)](https://codecov.io/gh/jsy1001/oifitslib)

OIFITSlib is a C library for input/output, merging, filtering and checking of
optical/IR interferometry datasets in the OIFITS exchange format. The published
specification for OIFITS version 2 is available at
<http://arxiv.org/abs/1510.04556>.

Version 2.x of OIFITSlib supports the OIFITS version 2 standard: the library can
read either v1 or v2 OIFITS files, and writes v2 files. The OIFITS standard has
been designed such that v2 files are backwards-compatible with reading software
intended for v1; hence a capability to write v1 files should not be
needed. However, version 1.x of OIFITSlib is available on GitHub for writing v1
OIFITS files if required.

The program `oifits-upgrade` will convert a version 1 OIFITS file to version 2
of the standard. Other command-line utilities `oifits-merge`, `oifits-filter`
and `oifits-check` are also provided - these provide simple user interfaces to
OIFITSlib routines.

A Python 2.7 interface to OIFITSlib is also included (created using SWIG). To
build this you will need SWIG 1.3 or later.

Installation
------------

The following libraries are required - please install them first:

- CFITSIO (version 3.x recommended):
  <http://heasarc.gsfc.nasa.gov/docs/software/fitsio/fitsio.html>
- GLib (version 2.56 or later required): <http://www.gtk.org/>

CMake (version 3.13 or later required) is now used for building - this should be
more portable than the previous Makefile. For those not familiar with CMake,
instructions can be found at <https://cmake.org/runningcmake/>. If you are using
a Unix-like operating system the following commands should build and install
OIFITSlib:

    cd build
    cmake ..
    make
    sudo make install

Optionally, run `make doc` to generate the API reference documentation.  Note
that the "oitable" library can be built without GLib. If GLib is not detected or
the `pkg-config` utility is not installed, only liboitable and its demonstration
program `oitable-demo` will be built.

OIFITSlib has been tested under Linux (Ubuntu and CentOS) and MacOS X. The
author is interested in hearing about successes or failures under other
operating systems.

Usage
-----

API reference documentation for OIFITSlib is automatically generated from
comments in the C code using doxygen (refer to the build instructions
above). After building the documentation, point your web browser to the file
[doc/oifitslib/html/index.html](doc/oifitslib/html/index.html).

The command-line utilities `oifits-merge`, `oifits-filter`, `oifits-check`, and
`oifits-upgrade` will output brief usage information if invoked with the
`--help` argument.

The `oifits-upgrade` utility converts a valid OIFITS version 1 file to OIFITS
version 2. The input file must include OI_ARRAY table(s) containing all of the
array elements used to obtain the data. Version 2 of OIFITS defines several
mandatory keywords for the primary header. The values for ORIGIN, OBSERVER and
INSMODE must be specified on the `oifits-upgrade` command line. Values for the
other mandatory keywords are obtained from the contents of the input file.

The python interface is documented using python docstrings generated from SWIG
interface files. Use pydoc to view the documentation for the oifits, oicheck,
oimerge and oifilter modules.

History
-------

The table-level input/output code (see [exchange.h](src/oifitslib/exchange.h))
in OIFITSlib is derived from the previously-released "OIFITS example software in
C", and provides the same Application Programming Interface (API). OIFITSlib
provides a file-level API built on top of the table-level code, containing
functions to read and write an entire OIFITS file (see
[oifile.h](src/oifitslib/oifile.h)).

License
-------

OIFITSlib is free software: you can redistribute it and/or modify it under the
terms of the GNU Lesser General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

OIFITSlib is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with OIFITSlib (in the file [lgpl.txt](lgpl.txt)).  If not, see
<http://www.gnu.org/licenses/>.
