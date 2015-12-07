OIFITSlib
=========

John Young <jsy1001@cam.ac.uk>

WHAT IS OIFITSlib?
------------------

OIFITSlib is a C library for input/output, merging, filtering and
checking of optical/IR interferometry datasets in the OIFITS exchange
format. A preprint of the draft specification for OIFITS version 2 is
available at <http://arxiv.org/abs/1510.04556>.

The table-level input/output code (see src/oifitslib/exchange.h) in
OIFITSlib is derived from the previously-released "OIFITS example
software in C", and provides the same Application Programming
Interface (API). OIFITSlib provides a file-level API built on top of
the table-level code, containing functions to read and write an entire
OIFITS file (see src/oifitslib/oifile.h).

Command-line utilities `oifits-merge`, `oifits-filter` and
`oifits-check` are also provided - these provide simple user
interfaces to OIFITSlib routines.

A python interface to OIFITSlib is also included (created using
SWIG). To build this you will need SWIG 1.3 or later.


LICENSING
---------

OIFITSlib is free software: you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OIFITSlib is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with OIFITSlib (in the file lgpl.txt).  If not, see
<http://www.gnu.org/licenses/>.
 

GETTING THE SOFTWARE
--------------------

The source code for OIFITSlib is available from github:
<https://github.com/jsy1001/oifitslib>


BUILDING THE SOFTWARE
---------------------

The following libraries are required - please install them first:
- CFITSIO (version 3.x recommended):
  <http://heasarc.gsfc.nasa.gov/docs/software/fitsio/fitsio.html>
- GLib    (version 2.16 or later required):
  <http://www.gtk.org/>

CMake is now used for building - this should be more portable than the
previous Makefile. For those not familiar with CMake, instructions can
be found at <https://cmake.org/runningcmake/>. If you are using a
Unix-like operating system the following commands should build and
install OIFITSlib:

    cd build
    cmake ..
    make
    sudo make install

Optionally, run `make doc` to generate the API reference documentation.

Note that the "oitable" library can be built without GLib. If GLib is
not detected or the `pkg-config` utility is not installed, only
liboitable and its demonstration program `oitable-demo` will be built.

OIFITSlib has been tested under Linux (Ubuntu and CentOS) and
MacOS X. The author is interested in hearing about successes or failures
under other operating systems.


DOCUMENTATION
-------------

The API reference documentation for OIFITSlib is supplied in HTML
format. Point your web browser to the file doc/oifitslib/html/index.html

The HTML documentation is automatically generated from comments
in the C code using doxygen.

The command-line utilities `oifits-merge`, `oifits-filter`,
`oifits-check`, and `oifits-upgrade` will output brief usage
information if invoked with the `--help` argument.

The python interface is documented using python docstrings generated
from SWIG interface files. Use pydoc to view the documentation for the
oifits, oicheck, oimerge and oifilter modules.
