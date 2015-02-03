// oicheck python module - SWIG interface definition file
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
"This module provides an interface to the OIFITSlib dataset checking
functions. A number of functions check_XXX() are provided, each of
which takes an oifits.OiFits instance and returns a tuple (level,
result) where level is an integer indicating the severity of any
breach of the OIFITS standard (see OI_BREACH_LEVEL in oicheck.h), and
result is a CheckResult instance. str(result) will return a
human-readable description of the result.

>>> import oifits
>>> from oicheck import *
>>> checks = [check_tables, check_header, check_keywords, check_visrefmap,
...           check_unique_targets, check_targets_present, check_corr_present,
...           check_elements_present, check_flagging, check_t3amp,
...           check_waveorder, check_time, check_spectrum]
>>> o = oifits.OiFits('bigtest2.fits')
>>> for c in checks:
...     level, result = c(o)
...     if level > 0:
...         print result
"
%enddef


%include "oifits_typemaps.i"

%module(docstring=DOCSTRING) oicheck
%pythoncode
%{
def _test():
  import doctest
  doctest.testmod()

if __name__ == '__main__':
  _test()
%}

%{
#include "oicheck.h"
%}

// Apply typemaps from oifits_typemaps.i
%apply SWIGTYPE *OUTPUT {oi_check_result *pResult};


typedef enum {} oi_breach_level;

%rename(CheckResult) oi_check_result;
typedef struct {
  oi_breach_level level;
  char *description;
  int numBreach;
} oi_check_result;

oi_breach_level check_tables(oi_fits *, oi_check_result *pResult);
oi_breach_level check_header(oi_fits *, oi_check_result *pResult);
oi_breach_level check_keywords(oi_fits *, oi_check_result *pResult);
oi_breach_level check_visrefmap(oi_fits *, oi_check_result *pResult);
oi_breach_level check_unique_targets(oi_fits *, oi_check_result *pResult);
oi_breach_level check_targets_present(oi_fits *, oi_check_result *pResult);
oi_breach_level check_corr_present(oi_fits *, oi_check_result *pResult);
oi_breach_level check_elements_present(oi_fits *, oi_check_result *pResult);
oi_breach_level check_flagging(oi_fits *, oi_check_result *pResult);
oi_breach_level check_t3amp(oi_fits *, oi_check_result *pResult);
oi_breach_level check_waveorder(oi_fits *, oi_check_result *pResult);
oi_breach_level check_time(oi_fits *, oi_check_result *pResult);
oi_breach_level check_spectrum(oi_fits *, oi_check_result *pResult);

// Object-oriented interface to result
%extend oi_check_result
 {
  ~oi_check_result()
  {
    free_check_result(self);
  }

  const char *__str__()
  {
    char *str = format_check_result(self);
    if (str != NULL)
      return str;
    else
      return "Check passed\n";
  }
}

// Local Variables:
// mode: C
// End:
