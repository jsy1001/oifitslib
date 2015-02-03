// oimerge module - interface definition for SWIG
//
// Copyright (C) 2007, 2015 John Young
//
//
// This file is part of OIFITSlib.
//
// OIFITSlib is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// OIFITSlib is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with OIFITSlib.  If not, see
// http://www.gnu.org/licenses/


%define DOCSTRING
"This module provides a simple interface to the OIFITSlib merge
functionality. Two functions are provided, both of which take two or
more oifits.OiFits instances, and return a new OiFits instance
containing the merged dataset.

merge() merges the OiFits instances supplied as its arguments; mergelist()
merges instances supplied in a list.

>>> import oifits, oimerge
>>> o1 = oifits.OiFits('testdata.fits')
>>> o2 = oifits.OiFits('test/OIFITS2/bigtest2.fits')
>>> merged = oimerge.merge(o1, o2)
>>> print len(merged.vis2List)
3
>>> print len(merged.t3List)
3
"
%enddef


%include "oifits_typemaps.i"

%module(docstring=DOCSTRING) oimerge
%feature("autodoc", "1");
%pythoncode
%{
def _test():
  import doctest
  doctest.testmod()

if __name__ == '__main__':
  _test()
%}

%{
#include "oimerge.h"
%}

// Apply typemaps from oifits_typemaps.i
%apply SWIGTYPE *OUTPUT {oi_fits *pOutput};
%map_in_glist(inList, oi_fits);


%pythoncode
%{
def merge(*args):
  """merge(OiFits a, OiFits b, ...) -> OiFits merged"""
  return mergelist(list(args))
%}


%feature("autodoc", "mergelist([OiFits a, OiFits b, ...]) -> OiFits merged")
 merge_oi_fits_list;

%rename(mergelist) merge_oi_fits_list;
void merge_oi_fits_list(const GList *inList, oi_fits *pOutput);


// Local Variables:
// mode: C
// End:
