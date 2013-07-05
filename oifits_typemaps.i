// $Id$

// SWIG Typemaps used to create python wrappers for OIFITSlib


// Helper functions used in typemaps below
%{
  static int *convert_int_array(PyObject *input, int expectedDim)
  {
    #define ERR_MSG_LEN 40
    char errMsg[ERR_MSG_LEN];
    int i;
    int *result;
    if (!PySequence_Check(input)) {
      PyErr_SetString(PyExc_TypeError, "Expecting a sequence");
      return NULL;
    }
    if (PyObject_Length(input) != expectedDim) {
      snprintf(errMsg, ERR_MSG_LEN,
               "Expecting a sequence with %d elements", expectedDim);
      PyErr_SetString(PyExc_ValueError, errMsg);
      return NULL;
    }
    result = malloc(expectedDim*sizeof(*result));
    for (i=0; i<expectedDim; i++) {
      PyObject *o = PySequence_GetItem(input, i);
      if (!PyInt_Check(o)) {
        Py_XDECREF(o);
        PyErr_SetString(PyExc_ValueError, "Expecting a sequence of integers");
        return NULL;
      }
      result[i] = (int) PyInt_AsLong(o);
      Py_DECREF(o);
    }
    return result;
  }

  static float *convert_float_array(PyObject *input, int expectedDim)
  {
    #define ERR_MSG_LEN 40
    char errMsg[ERR_MSG_LEN];
    int i;
    float *result;
    if (!PySequence_Check(input)) {
      PyErr_SetString(PyExc_TypeError, "Expecting a sequence");
      return NULL;
    }
    if (PyObject_Length(input) != expectedDim) {
      snprintf(errMsg, ERR_MSG_LEN,
               "Expecting a sequence with %d elements", expectedDim);
      PyErr_SetString(PyExc_ValueError, errMsg);
      return NULL;
    }
    result = malloc(expectedDim*sizeof(*result));
    for (i=0; i<expectedDim; i++) {
      PyObject *o = PySequence_GetItem(input, i);
      if (!PyFloat_Check(o)) {
        Py_XDECREF(o);
        PyErr_SetString(PyExc_ValueError, "Expecting a sequence of floats");
        return NULL;
      }
      result[i] = (float) PyFloat_AsDouble(o);
      Py_DECREF(o);
    }
    return result;
  }

  static double *convert_double_array(PyObject *input, int expectedDim)
  {
    #define ERR_MSG_LEN 40
    char errMsg[ERR_MSG_LEN];
    int i;
    double *result;
    if (!PySequence_Check(input)) {
      PyErr_SetString(PyExc_TypeError, "Expecting a sequence");
      return NULL;
    }
    if (PyObject_Length(input) != expectedDim) {
      snprintf(errMsg, ERR_MSG_LEN,
               "Expecting a sequence with %d elements", expectedDim);
      PyErr_SetString(PyExc_ValueError, errMsg);
      return NULL;
    }
    result = malloc(expectedDim*sizeof(*result));
    for (i=0; i<expectedDim; i++) {
      PyObject *o = PySequence_GetItem(input, i);
      if (!PyFloat_Check(o)) {
        Py_XDECREF(o);
        PyErr_SetString(PyExc_ValueError, "Expecting a sequence of floats");
        return NULL;
      }
      result[i] = PyFloat_AsDouble(o);
      Py_DECREF(o);
    }
    return result;
  }
%}


// Avoid need for char[] to be zero-padded out to $1_dim0
// Must contain a null-terminated string
%typemap(out) char NULL_TERMINATED [ANY] {
  $result = SWIG_FromCharPtr($1);
}

// Map int[] to python tuple - use for small-dimension arrays
%typemap(out) int TUPLE_OUTPUT [ANY]
{
  int i;
  $result = PyTuple_New($1_dim0);
  for (i=0; i<$1_dim0; i++) {
    PyObject *o = PyInt_FromLong((long) $1[i]);
    PyTuple_SetItem($result, i, o);
  }
}

// Map python sequence to int[] - use for small-dimension arrays
%typemap(in) int TUPLE_INPUT [ANY]
{
  $1 = convert_int_array($input, $1_dim0);
  if (!$1) return NULL;
}

// Free storage allocated by convert_int_array()
%typemap(freearg) int TUPLE_INPUT [ANY]
{
  free($1);
}

// Map float[] to python tuple - use for small-dimension arrays
%typemap(out) float TUPLE_OUTPUT [ANY]
{
  int i;
  $result = PyTuple_New($1_dim0);
  for (i=0; i<$1_dim0; i++) {
    PyObject *o = PyFloat_FromDouble((double) $1[i]);
    PyTuple_SetItem($result, i, o);
  }
}

// Map python sequence to float[] - use for small-dimension arrays
%typemap(in) float TUPLE_INPUT [ANY]
{
  $1 = convert_float_array($input, $1_dim0);
  if (!$1) return NULL;
}

// Free storage allocated by convert_float_array()
%typemap(freearg) float TUPLE_INPUT [ANY]
{
  free($1);
}

// Map double[] to python tuple - use for small-dimension arrays
%typemap(out) double TUPLE_OUTPUT [ANY]
{
  int i;
  $result = PyTuple_New($1_dim0);
  for (i=0; i<$1_dim0; i++) {
    PyObject *o = PyFloat_FromDouble($1[i]);
    PyTuple_SetItem($result, i, o);
  }
}

// Map python sequence to double[] - use for small-dimension arrays
%typemap(in) double TUPLE_INPUT [ANY]
{
  $1 = convert_double_array($input, $1_dim0);
  if (!$1) return NULL;
}

// Free storage allocated by convert_double_array()
%typemap(freearg) double TUPLE_INPUT [ANY]
{
  free($1);
}


// Pair of typemaps to return newly-allocated object to python,
// rather than specify output object as argument
%typemap(in, numinputs=0) SWIGTYPE *OUTPUT 
{
  $1 = ($1_ltype) malloc(sizeof(*$1)); // leave for user to destruct
}

%typemap(argout) SWIGTYPE *OUTPUT
{
  PyObject *o, *o2, *o3;
  o = SWIG_NewPointerObj($1, $1_descriptor, 0);
  if ((!$result) || ($result == Py_None)) {
    $result = o;
  } else {
    if (!PyTuple_Check($result)) {
      PyObject *o2 = $result;
      $result = PyTuple_New(1);
      PyTuple_SetItem($result, 0, o2);
    }
    o3 = PyTuple_New(1);
    PyTuple_SetItem(o3, 0, o);
    o2 = $result;
    $result = PySequence_Concat(o2, o3);
    Py_DECREF(o2);
    Py_DECREF(o3);
  }
}


// Pair of typemaps to hide pointer to status variable argument
// and raise python exception on failure
%typemap(in, numinputs=0) STATUS *FITSIO_STATUS (STATUS status)
{
  status = 0;
  $1 = &status;
}

%typemap(argout) STATUS *FITSIO_STATUS
{
  GString *gstr;
  char msg[80];
  if(*$1) {
    gstr = g_string_sized_new(80);
    while(fits_read_errmsg(msg)) {
      g_string_append_printf(gstr, "%s\n", msg);
    }
    PyErr_SetString(PyExc_IOError, gstr->str);
    g_string_free(gstr, TRUE);
    SWIG_fail;
  }
}


// Macro to map python sequence of SWIG-wrapped DATA_TYPE instances to
// named GList *LIST
%define %map_in_glist(LIST, DATA_TYPE)
%typemap(in) GList *LIST
{
  DATA_TYPE *item;
  if (PySequence_Check($input)) {
    int size = PySequence_Size($input);
    int i = 0;
    $1 = NULL;
    for (i=0; i<size; i++) {
      PyObject *o = PySequence_GetItem($input, i);
      if(SWIG_ConvertPtr(o, (void **) &item,
			 SWIGTYPE_p_ ## DATA_TYPE, 0) == -1)
	  return NULL;
      $1 = g_list_append($1, item);
    }
  } else {
    PyErr_SetString(PyExc_TypeError, "Expected a sequence");
    return NULL;
  }
}

%typemap(freearg) GList *LIST
{
  g_list_free($1);
}
%enddef  

// Macro to map named GList *LIST (containing elements of type DATA_TYPE)
// to python list
%define %map_out_glist(LIST, DATA_TYPE)
%typemap(out) GList *LIST {
  $result = PyList_New(g_list_length($1));
  GList *link = $1;
  int i = 0;
  while(link != NULL) {
    PyObject *o = SWIG_NewPointerObj(link->data, SWIGTYPE_p_ ## DATA_TYPE, 0);
    PyList_SetItem($result, i++, o);
    link = link->next;
  }
}
%enddef


// Local Variables:
// mode: C
// End:
