/**************************************************************************
Copyright (C) 2007, 2008 Thomas Finley, tfinley@gmail.com

This file is part of PyGLPK.

PyGLPK is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

PyGLPK is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PyGLPK.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "util.h"
#include "barcol.h"
#include "lp.h"
#include <string.h>
#include <stdio.h>

int util_extract_if(PyObject *ob, PyObject *barcol,
		    int *len, int **ind, double **val) {
  PyObject *iter=NULL,*index2value=NULL,*item=NULL,*index=NULL,*value=NULL;
  BarColObject *bc = (BarColObject*)barcol;

  int curr_index = 0, num_zero = 0, size = BarCol_Size(bc);

  if ((iter=PyObject_GetIter(ob)) == NULL) return 0;
  if ((index2value=PyDict_New()) == NULL) return 0;

  while ((item=PyIter_Next(iter))!=NULL) {
    PyObject *fvalue;
    // Check the entry.
    if (PyNumber_Check(item)) {
      // It's a single number.
      // Verify the bounds on curr_index.
      if (curr_index < 0 || curr_index >= size) {
	PyErr_Format(PyExc_IndexError, "vector index %d out of range",
		     curr_index);
	break; // Let our end of iteration handler do the rest.
      }
      value = item;
      Py_INCREF(value); // Same object of course, but treated separately.
      index = PyInt_FromLong(curr_index);
    } else if (PyTuple_Check(item)) {
      // It's a tuple.
      if (PyTuple_Size(item)!=2) {
	PyErr_Format(PyExc_ValueError, "vector entry tuple has length %d; "
		     "2 is required", (int)PyTuple_Size(item));
	break;
      }
      // Ensure that the first element is an int (or null sometimes),
      // the second a number.
      index = PyTuple_GET_ITEM(item, 0);
      value = PyTuple_GET_ITEM(item, 1);

      if (BarCol_Index(bc, index, &curr_index, -1)) {
	break;
      }
      // Check the value real quick.
      if (!PyNumber_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "matrix values must be numbers");
	break;
      }
      
      index = PyInt_FromLong(curr_index);
      Py_INCREF(value);
    } else {
      // Something other than a single number, or a tuple.
      PyErr_SetString(PyExc_TypeError, 
		      "vector entries must be tuple or number");
      break;
    }
    // Now we have an int index and numeric value.  Verify that the
    // numeric is interpretable as a float.
    fvalue = PyNumber_Float(value);
    Py_DECREF(value);
    value = fvalue;
    if (value == NULL) {
      Py_DECREF(index);
      PyErr_SetString(PyExc_TypeError, 
		      "vector values must be interpretable as floats");
      break;
    }
    // Insert it into the dictionary that we are using to check for duplicates.
    if (PyDict_GetItem(index2value, index)!=NULL) {
      Py_DECREF(index);
      Py_DECREF(value);
      PyErr_Format(PyExc_ValueError,"duplicate index %d detected",curr_index);
      break;
    }
    if (PyDict_SetItem(index2value, index, value)) {
      Py_DECREF(index);
      Py_DECREF(value);
      break;
    }
    if (PyFloat_AS_DOUBLE(value)==0.0) num_zero++;
    Py_DECREF(index);
    Py_DECREF(value);
    Py_DECREF(item);
    ++curr_index;
  }
  Py_DECREF(iter);
  if (PyErr_Occurred()) {
    // Something bad happened in iteration.
    Py_XDECREF(item);
    Py_DECREF(index2value);
    return 0;
  }
  // Wow.  Now we have a dictionary of known Int to Float objects.
  // Allocate and copy into our arrays.
  Py_ssize_t ppos = 0;
  int i = 0;
  *len = PyDict_Size(index2value) - num_zero;
  if (*len > 0) {
    *ind = (int*)calloc(*len, sizeof(int));
    *val = (double*)calloc(*len, sizeof(double));
    while (PyDict_Next(index2value, &ppos, &index, &value)) {
      double v = PyFloat_AS_DOUBLE(value);
      if (v==0.0) continue;
      (*ind)[i] = PyInt_AS_LONG(index)+1;
      (*val)[i] = v;
      i++;
    }
  } else {
    *ind = NULL;
    *val = NULL;
  }
  Py_DECREF(index2value);
  return 1;
}

int util_extract_iif(PyObject *ob, PyObject*lpobj,
		  int *len, int **ind1, int **ind2, double **val) {
  PyObject *iter=NULL,*index2value=NULL,*item=NULL,*index=NULL,*value=NULL;
  int curr_index1 = 0, curr_index2 = 0, num_zero = 0;
  LPXObject *lp = (LPXObject*)lpobj;
  int size1 = lpx_get_num_rows(lp->lp), size2 = lpx_get_num_cols(lp->lp);

  if ((iter=PyObject_GetIter(ob)) == NULL) return 0;
  if ((index2value=PyDict_New()) == NULL) return 0;

  while ((item=PyIter_Next(iter))!=NULL) {
    PyObject *fvalue;
    if (PyNumber_Check(item)) {
      // It's a single number.
      if (curr_index1 < 0 || curr_index1 >= size1 ||
	  curr_index2 < 0 || curr_index2 >= size2) {
	PyErr_Format(PyExc_IndexError, "matrix index %d,%d out of range",
		     curr_index1, curr_index2);
	break;
      }
      value = item;
      Py_INCREF(value); // Yes, the same item, but they are treated separately.
      index = Py_BuildValue("ii", curr_index1, curr_index2);
    } else if (PyTuple_Check(item)) {
      // It's a tuple.
      if (PyTuple_Size(item)!=3) {
	PyErr_Format(PyExc_ValueError, "matrix entry tuple has length %d; "
		     "3 is required", (int)PyTuple_Size(item));
	break;
      }

      // Try to get the two indices.
      index = PyTuple_GET_ITEM(item, 0);
      if (BarCol_Index((BarColObject*)(lp->rows), index, &curr_index1, -1))
	break;
      index = PyTuple_GET_ITEM(item, 1);
      if (BarCol_Index((BarColObject*)(lp->cols), index, &curr_index2, -1))
	break;

      // Check the value real quick.
      value = PyTuple_GET_ITEM(item, 2);
      if (!PyNumber_Check(value)) {
	PyErr_SetString(PyExc_TypeError, "matrix values must be numbers");
	break;
      }
      
      index = Py_BuildValue("ii", curr_index1, curr_index2);
      Py_INCREF(value);
      // Done with the tuple code.
    } else {
      PyErr_SetString(PyExc_TypeError, 
		      "vector entries must be tuple or number");
      break;
    }
    // Verify that the value is interpretable as a float.
    fvalue = PyNumber_Float(value);
    Py_DECREF(value);
    value = fvalue;
    if (value == NULL) {
      Py_DECREF(index);
      PyErr_SetString(PyExc_TypeError, 
		      "matrix values must be interpretable as floats");
      break;
    }
    // Insert it into the dictionary that we are using to check for duplicates.
    if (PyDict_GetItem(index2value, index)!=NULL) {
      Py_DECREF(index);
      Py_DECREF(value);
      PyErr_Format(PyExc_ValueError,"duplicate index %d,%d detected",
		   curr_index1, curr_index2);
      break;
    }
    if (PyDict_SetItem(index2value, index, value)) {
      Py_DECREF(index);
      Py_DECREF(value);
      break;
    }
    if (PyFloat_AS_DOUBLE(value)==0.0) num_zero++;
    Py_DECREF(index);
    Py_DECREF(value);
    Py_DECREF(item);
    // Update the current indices.
    curr_index2 = (curr_index2+1)%size2;
    if (curr_index2==0) ++curr_index1;
  }
  Py_DECREF(iter);
  //PyErr_SetString(PyExc_RuntimeError, "Just a useless error!");
  if (PyErr_Occurred()) {
    // Something bad happened in iteration.
    Py_XDECREF(item);
    Py_DECREF(index2value);
    return 0;
  }

  // Allocate and copy into our arrays.
  Py_ssize_t ppos = 0;
  int i = 0;
  *len = PyDict_Size(index2value) - num_zero;
  if (*len > 0) {
    *ind1 = (int*)calloc(*len, sizeof(int));
    *ind2 = (int*)calloc(*len, sizeof(int));
    *val = (double*)calloc(*len, sizeof(double));
    while (PyDict_Next(index2value, &ppos, &index, &value)) {
      double v = PyFloat_AS_DOUBLE(value);
      if (v==0.0) continue;
      (*ind1)[i] = PyInt_AS_LONG(PyTuple_GET_ITEM(index, 0))+1;
      (*ind2)[i] = PyInt_AS_LONG(PyTuple_GET_ITEM(index, 1))+1;
      (*val)[i] = v;
      i++;
    }
  } else {
    *ind1 = NULL;
    *ind2 = NULL;
    *val = NULL;
  }
  Py_DECREF(index2value);
  return 1;
}

int util_add_type(PyObject *module, PyTypeObject *type) {
  if (PyType_Ready(type) < 0) return -1;
  Py_INCREF(type);
#if PY_MAJOR_VERSION == 2 && PY_MINOR_VERSION < 5
  char*last_period = strrchr(type->tp_name, '.');
#else
  const char*last_period = strrchr(type->tp_name, '.');
#endif
  if (last_period==NULL) {
    // Um... we really shouldn't have this happen, but OK.
    last_period = type->tp_name;
  } else {
    // Name of the type is everything past this last period.
    last_period++;
  }
  PyModule_AddObject(module, last_period, (PyObject*)type);
  return 0;
}

/**
 * Inserts an integer for value into the dictionary p with a key.
 * Returns 0 on success, or -1 on failure.
 */
int PyDict_SetIntString(PyObject *p, const char *key, int val) {
  PyObject *i;
  int retval;
  i = PyInt_FromLong(val);
  if (i==NULL) return -1;
  retval = PyDict_SetItemString(p, key, i);
  Py_DECREF(i);
  return retval;
}
