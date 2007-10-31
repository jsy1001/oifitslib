// $Id$

// oifits module - interface definition file for SWIG

%define DOCSTRING
"This module provides full read-only and limited read-write access
to OIFITS data, by means of an OiFits class. OiFits instances have
a comprehensive set of data attributes (mirroring those in the
OIFITSlib C API) plus a few methods.

Use the OiFits constructor to read an OIFITS file from disk:
>>> from math import *
>>> import oifits
>>> o = oifits.OiFits('bigtest2.fits')

The resulting OiFits instance may be passed to functions in the
oicheck, oimerge, and oifilter modules. Functions in the latter two
modules create a new OiFits instance, which may be written to an
OIFITS file using the write() method:
>>> o.write('!junk.oifits') # leading ! deletes existing file

We use various sorts of mappings from the C API here. These are
enumerated in the following paragraphs, with accompanying examples of
how to access OiFits attributes from python.

Linked lists of objects are converted to python lists:
>>> print len(o.vis2List[:2])
2
>>> o.vis2List = []
>>> print len(o.vis2List)
0

Dictionaries of objects are not mapped, instead we've added methods such 
as get_array() to the top-level class:
>>> print o.get_element('CHARA_2004Jan', 2).sta_index
2
>>> print ceil(1e9*o.get_eff_wave('IOTA_IONIC_PICNIC')[0])
1650.0

Arrays of objects such as data records are mapped to indexable custom
objects (to avoid making a copy of the data). These are read-only:
>>> rec = o.visList[0].record[2]
>>> print rec.target_id
2

Flag and data primitive-type (e.g. double) arrays are mapped to
indexable custom objects, using carrays.i. Elementwise assignment is
possible (but note there is no bounds checking):
>>> o.t3List[0].record[1].flag[0] = 1
>>> print o.t3List[0].record[1].flag[0]
1

(small) Fixed-dimension primitive-type arrays are converted to python
tuples. Python tuples are immutable, but the entire tuple may be replaced:
>>> o.t3List[0].record[0].sta_index = (1, 2, 3)
>>> print o.t3List[0].record[0].sta_index
(1, 2, 3)
"
%enddef
// :TODO: interface for adding data tables
// :TODO: indent style


%include <exception.i>
%include <carrays.i>
%include "oifits_typemaps.i"


// Macro to allow arrays of type TYPE to be accessed using [i] notation
// Must be invoked after class declaration
// :TODO: how to extend arrays?
%define %add_array_access(TYPE)
%extend TYPE
{
  TYPE *__getitem__(int i)
  {
    return self+i;
  }
  // :TODO: __setitem__ ?
}
%enddef

%module(docstring=DOCSTRING) oifits
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
#include "oifile.h"
%}

// Apply typemaps and macros from oifits_typemaps.i
%apply char NULL_TERMINATED [ANY] {char [ANY]};
%apply int TUPLE_INPUT [ANY] {int [2]}; // sta_index
%apply int TUPLE_OUTPUT [ANY] {int [2]};
%apply int TUPLE_INPUT [ANY] {int [3]}; // sta_index in oi_t3
%apply int TUPLE_OUTPUT [ANY] {int [3]};
%apply double TUPLE_INPUT [ANY] {double [3]}; // staxyz
%apply double TUPLE_OUTPUT [ANY] {double [3]};
%map_in_glist(arrayList, oi_array);
%map_out_glist(arrayList, oi_array);
%map_in_glist(wavelengthList, oi_wavelength);
%map_out_glist(wavelengthList, oi_wavelength);
%map_in_glist(visList, oi_vis);
%map_out_glist(visList, oi_vis);
%map_in_glist(vis2List, oi_vis2);
%map_out_glist(vis2List, oi_vis2);
%map_in_glist(t3List, oi_t3);
%map_out_glist(t3List, oi_t3);


// :BUG: $result maps to 'resultObj' but want raw return from new_oi_fits
//%exception oi_fits
%exception dontuse
{
  char msg[80];
  $action; //need something else here?

  printf("exception handler: %p\n", $result);
  if($result == NULL)
  {
    fits_read_errmsg(msg);
    SWIG_exception(SWIG_IOError, msg);
  }
}


// Exclude few attributes that can't be wrapped sensibly
// We use new methods to plug the gaps
%ignore wavelengthHash; // use get_eff_wave() etc. instead
%ignore arrayHash; // use get_element() etc. instead

%array_class(float, waveArray) // used by get_eff_*() method
%array_class(double, doubleArray)
%array_class(signed char, boolArray)

// Trick to ensure desired struct members get mapped to proxy class instances
#define DATA doubleArray
#define BOOL boolArray

%include "exchange.h"
%include "oifile.h"

// %add_array_access is read-only
// could add __setitem__
%add_array_access(element);
%add_array_access(target);
%add_array_access(oi_vis_record);
%add_array_access(oi_vis2_record);
%add_array_access(oi_t3_record);


// Object-oriented interface to oi_fits struct
%rename(OiFits) oi_fits;
%extend oi_fits
{
  oi_fits(char *filename=NULL) 
  {
    oi_fits *self;
    int status;
    self = (oi_fits *) malloc(sizeof(oi_fits));
    if(filename != NULL)
    {
      status = 0;
      read_oi_fits(filename, self, &status);
      if(status) {
        free(self);
        printf("*** CFITSIO Error %d\n", status); //
        return NULL;
      }
    }
    else 
    {
      init_oi_fits(self);
    }
    return self;
  }

  ~oi_fits() 
  {
    free_oi_fits($self);
  }
  
  const char *__str__()
  {
    return format_oi_fits_summary($self);
  }

  void write(const char *filename)
  {
    int status;
    status = 0;
    write_oi_fits(filename, *$self, &status);
  }

  // synthesised attribute
  int numTargets;

  target *get_target(int targetId)
  {
    return oi_fits_lookup_target($self, targetId);
  }

  int get_nelement(char *arrname)
  {
    return oi_fits_lookup_array($self, arrname)->nelement;
  }
  
  element *get_element(char *arrname, int staIndex)
  {
    return oi_fits_lookup_element($self, arrname, staIndex);
  }

  int get_nwave(char *insname)
  {
    return oi_fits_lookup_wavelength($self, insname)->nwave;
  }

  waveArray *get_eff_wave(char *insname)
  {
    return waveArray_frompointer(
      (oi_fits_lookup_wavelength($self, insname)->eff_wave));
  }

  waveArray *get_eff_band(char *insname)
  {
    return waveArray_frompointer(
      (oi_fits_lookup_wavelength($self, insname)->eff_band));
  }
  
};

%{
// Methods to synthesise numTargets attribute
int oi_fits_numTargets_get(oi_fits *self)
{
  return self->targets.ntarget;
}

void oi_fits_numTargets_set(oi_fits *self, int ntarget)
{
  self->targets.ntarget = ntarget;
}
%}

// Local Variables:
// mode: C
// End:
