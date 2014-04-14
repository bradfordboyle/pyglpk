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

#include "barcol.h"
#include "bar.h"
#include "util.h"
#include "structmember.h"

#define LP (self->py_lp->lp)

BarObject *BarCol_Bar(BarColObject *self, int index);

/** BAR COLLECTION ITERATOR OBJECT IMPLEMENTATION **/

typedef struct {
  PyObject_HEAD
  int index;
  BarColObject *bc;
  PyObject *weakreflist; // Weak reference list.
} BarColIterObject;

static PyObject *BarCol_Iter(PyObject *bc) {
  BarColIterObject *it;
  if (!BarCol_Check(bc)) {
    PyErr_BadInternalCall();
    return NULL;
  }
  it = PyObject_New(BarColIterObject, &BarColIterType);
  if (it == NULL) return NULL;
  it->weakreflist = NULL;
  it->index = 0;
  Py_INCREF(bc);
  it->bc = (BarColObject *)bc;
  return (PyObject *)it;
}

static void BarColIter_dealloc(BarColIterObject *it) {
  if (it->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)it);
  }
  Py_XDECREF(it->bc);
  it->ob_type->tp_free((PyObject*)it);
}

static Py_ssize_t BarColIter_len(BarColIterObject *it) {
  int len=0;
  len = BarCol_Size(it->bc) - it->index;
  return len >= 0 ? len : 0;
}

static PyObject *BarColIter_next(BarColIterObject *it) {
  if (BarCol_Size(it->bc) - it->index <= 0) return NULL;
  return (PyObject*)BarCol_Bar(it->bc, it->index++);
}

static PySequenceMethods bciter_as_sequence = {
  (lenfunc)BarColIter_len, /* sq_length */
  0, /* sq_concat */
};

PyTypeObject BarColIterType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.BarCollectionIter",		/* tp_name */
  sizeof(BarColIterObject),		/* tp_basicsize */
  0,					/* tp_itemsize */
  (destructor)BarColIter_dealloc,	/* tp_dealloc */
  0,					/* tp_print */
  0,					/* tp_getattr */
  0,					/* tp_setattr */
  0,					/* tp_compare */
  0,					/* tp_repr */
  0,					/* tp_as_number */
  &bciter_as_sequence,			/* tp_as_sequence */
  0,					/* tp_as_mapping */
  0,					/* tp_hash */
  0,					/* tp_call */
  0,					/* tp_str */
  PyObject_GenericGetAttr,		/* tp_getattro */
  0,					/* tp_setattro */
  0,					/* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,			/* tp_flags */
  "Bar collection iterator objects.  Created for iterating over the\n"
  "rows and columns contained with a bar collection.",	/* tp_doc */
  0,					/* tp_traverse */
  0,					/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(BarColIterObject, weakreflist),	/* tp_weaklistoffset */
  PyObject_SelfIter,			/* tp_iter */
  (iternextfunc)BarColIter_next,	/* tp_iternext */
  0,					/* tp_methods */
  0,					/* tp_members */
  0,					/* tp_getset */
  0,					/* tp_base */
  0,					/* tp_dict */
  0,					/* tp_descr_get */
  0,					/* tp_descr_set */
  0,					/* tp_dictoffset */
};

/** BAR COLLECTION OBJECT IMPLEMENTATION **/

static int BarCol_traverse(BarColObject *self, visitproc visit, void *arg) {
  Py_VISIT((PyObject*)self->py_lp);
  //printf("traverse bar col!\n");
  return 0;
}

static int BarCol_clear(BarColObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_CLEAR(self->py_lp);
  //printf("clearing bar col!\n");
  return 0;
}

static void BarCol_dealloc(BarColObject *self) {
  BarCol_clear(self);
  //printf("dealloc bar col!\n");
  self->ob_type->tp_free((PyObject*)self);
}

/** Create a new bar collection object. */
BarColObject *BarCol_New(LPXObject *py_lp, char rows) {
  BarColObject *bc = (BarColObject*)PyObject_GC_New(BarColObject, &BarColType);
  if (bc==NULL) return bc;

  Py_INCREF(py_lp);
  bc->py_lp = py_lp;
  bc->r = rows ? 1 : 0;
  bc->size = -1;
  bc->weakreflist = NULL;

  PyObject_GC_Track(bc);
  return bc;
}

/** Return a bar for a given index. */
BarObject *BarCol_Bar(BarColObject *self, int index) {
  return Bar_New(self, index);
}

static PyObject *BarCol_add(BarColObject *self, PyObject *args) {
  int n;
  if (!PyArg_ParseTuple(args, "i", &n)) return NULL;
  if (n<1) {
    PyErr_SetString(PyExc_ValueError, "number of added entries must be >0");
    return NULL;
  }
  n = (BarCol_Rows(self) ? glp_add_rows : glp_add_cols)(LP, n)-1;
  self->size = -1;
  return PyInt_FromLong(n);
}

/********** ABSTRACT PROTOCOL FUNCTIONS *******/

int BarCol_Size(BarColObject* self) {
  if (self->size < 0)
    self->size = (BarCol_Rows(self) ? glp_get_num_rows : glp_get_num_cols)(LP);
  return self->size;
}

int BarCol_Index(BarColObject *self, PyObject *obj, int *index, int except){
  int size = (BarCol_Rows(self) ? lpx_get_num_rows : lpx_get_num_cols)(LP);
  if (PyInt_Check(obj)) {
    int i = PyInt_AsLong(obj);
    if (i < 0) i += size;
    if (i < 0 || i >= size) {
      if (except & 2)
	PyErr_Format(PyExc_IndexError, "%s index out of bounds",
		     (BarCol_Rows(self)?"row":"column"));
      return 2;
    }
    *index = i;
    return 0;
  } else if (PyString_Check(obj)) {
    glp_create_index(LP); // No effect if already present.
    char *name = PyString_AsString(obj);
    if (name==NULL) return -1;
    int i = (BarCol_Rows(self) ? glp_find_row : glp_find_col)(LP,name);
    if (i==0) {
      if (except & 4)
	PyErr_Format(PyExc_KeyError, "%s named '%s' does not exist",
		     BarCol_Rows(self)?"row":"col", name);
      return 4;
    }
    *index = i-1;
    return 0;
  }
  if (except & 1)
    PyErr_SetString(PyExc_TypeError, "row/col indices must "
		    "be ints or strings");
  return 1;
}

static int BarCol_contains(BarColObject *self, PyObject *item) {
  int index = 0;
  int inside = BarCol_Index(self, item, &index, 1);
  if (PyErr_Occurred()) return -1;
  return (inside==0 ? 1 : 0);
}

static PyObject* BarCol_subscript(BarColObject *self, PyObject *item) {
  Py_ssize_t size = BarCol_Size(self), i;
  
  if (PyInt_Check(item) || PyString_Check(item)) {
    // They input a single number or string.
    int index = 0;
    if (BarCol_Index(self, item, &index, -1)) return NULL;
    return (PyObject*)BarCol_Bar(self, index);
  } else if (PySlice_Check(item)) {
    // They input a slice.
    Py_ssize_t start, stop, step, subsize;
    PyObject *sublist = NULL;
    if (PySlice_GetIndicesEx((PySliceObject*)item,size,&start,&stop,
			     &step,&subsize)<0) return NULL;
    sublist = PyList_New(subsize);
    if (sublist==NULL) return NULL;
    for (i=0; i<subsize; ++i) {
      PyObject *bar = (PyObject*) BarCol_Bar(self, start + i*step);
      if (bar==NULL) {
	Py_DECREF(sublist);
	return NULL;
      }
      PyList_SET_ITEM(sublist, i, bar);
    }
    return sublist;
  } else if (PyTuple_Check(item)) {
    // They input a tuple.
    Py_ssize_t subsize, i;
    PyObject *sublist = NULL;

    // Allocate the list to return.
    subsize = PyTuple_Size(item);
    sublist = PyList_New(subsize);
    if (sublist==NULL) return NULL;

    for (i=0; i<subsize; ++i) {
      // Try to determine if the item is an integer.
      PyObject *subitem = PyTuple_GetItem(item, i);
      // Get the numeric index.
      int index = 0;
      if (BarCol_Index(self, subitem, &index, -1)) {
	Py_DECREF(sublist);
	return NULL;
      }
      // Get the actual bar now.
      PyObject *bar = (PyObject*) BarCol_Bar(self, index);
      if (bar==NULL) {
	Py_DECREF(sublist);
	return NULL;
      }
      PyList_SET_ITEM(sublist, i, bar);
    }
    return sublist;
  }
  PyErr_SetString(PyExc_TypeError,"bad index type for row/col collection");
  return NULL;
}

#if PY_MAJOR_VERSION == 2
#if PY_MINOR_VERSION < 5
// Stuff that sort of acts like sets...
static PyObject* PySet_New(PyObject*i) { return PyDict_New(); }
static int PySet_Add(PyObject*s, PyObject*k) {
  Py_INCREF(Py_None); return PyDict_SetItem(s, k, Py_None); }
static int PySet_Size(PyObject*s) { return PyDict_Size(s); }
#endif // PY_MINOR_VERSION < 5
#endif // PY_MAJOR_VERSION == 2

static int BarCol_ass_subscript(BarColObject *self, PyObject *item,
				PyObject *value) {
  if (value == NULL) {
    // We're deleting.  Woo hoo.
    int size = BarCol_Size(self);
    Py_ssize_t numtodel, i;
    int *indtodel;
    // Extract which indices we should read.
    if (PyInt_Check(item) || PyString_Check(item)) {
      // They input a single number or string.
      int index=0;
      if (BarCol_Index(self, item, &index, -1)) return -1;
      numtodel = 1;
      indtodel = (int*)calloc(numtodel, sizeof(int));
      indtodel[0] = index;
    } else if (PySlice_Check(item)) {
      // They input a slice.
      Py_ssize_t start, stop, step;
      if (PySlice_GetIndicesEx((PySliceObject*)item,size,&start,&stop,
			       &step,&numtodel)<0) return -1;
      if (numtodel==0) return 0; // Trivial...
      indtodel = (int*)calloc(numtodel, sizeof(int));
      for (i=0; i<numtodel; ++i) {
	indtodel[i] = start + i*step;
      }
    } else if (PyTuple_Check(item)) {
      // They input a tuple, probably as a comma delimited sequence of nums.
      PyObject *set, *py_todel; // For checking on duplicate numbers...
      numtodel = PyTuple_Size(item);
      if (numtodel==0) return 0;

      set = PySet_New(NULL);
      if (set==NULL) return -1;
      indtodel = (int*)calloc(numtodel, sizeof(int));

      for (i=0; i<numtodel; ++i) {
	int todel = 0;
	PyObject *subitem = PyTuple_GetItem(item, i);
	if (!subitem || BarCol_Index(self, subitem, &todel, -1)) {
	  free(indtodel);
	  Py_DECREF(set);
	  return -1;
	}
	// Check for duplicates.
	py_todel = PyInt_FromLong((int)todel);
	if (PySet_Add(set, py_todel)) {
	  free(indtodel);
	  Py_DECREF(set);
	  Py_XDECREF(py_todel);
	  return -1;
	}
	Py_XDECREF(py_todel);
	if (PySet_Size(set)==i) {
	  free(indtodel);
	  Py_DECREF(set);
	  PyErr_SetString(PyExc_ValueError, "duplicate index detected");
	  return -1;
	}
	// End of duplicate detection scheme.
	indtodel[i] = todel;
      }
      Py_DECREF(set); // We've verified, no duplicates...
    } else {
      PyErr_SetString(PyExc_TypeError,"bad index type for row/col collection");
      return -1;
    }
    // OK, now we have that stuff... now check it for boundedness.
    for (i=0; i<numtodel; ++i)
      indtodel[i] += 1;
    // Pass it into the appropriate LP deletion routine.
    (BarCol_Rows(self) ? glp_del_rows : glp_del_cols)
      (LP, numtodel, indtodel-1);
    free(indtodel);
    self->size -= numtodel;

  } else {
    int size = BarCol_Size(self);
    //Py_ssize_t i;
    
    // This is an actual value assignment.
    if (PyInt_Check(item)) {
      // They input a single number.
      int index;
      index = PyInt_AsLong(item);
      if (index < 0) index += size;
      BarObject *bar = BarCol_Bar(self, index);
      if (!bar) return -1;
      index = Bar_SetMatrix(bar, value);
      Py_DECREF(bar);
      return index;
    }
    PyErr_SetString(PyExc_TypeError,"bad index type for row/col collection");
    return -1;
  }

  return 0;
}

static int BarCol_ass_item(BarColObject *self, int index, PyObject *v) {
  printf("bc ass item\n");
  return 0;
}

static PyObject* BarCol_Str(BarColObject *self) {
  // Returns a string representation of this object.
  return PyString_FromFormat
    ("<%s, %s of %s %p>", self->ob_type->tp_name,
     (BarCol_Rows(self)?"rows":"cols"),
     LPXType.tp_name, self->py_lp);
}

/****************** GET-SET-ERS ***************/


/****************** OBJECT DEFINITION *********/

int BarCol_InitType(PyObject *module) {
  int retval;
  if ((retval=util_add_type(module, &BarColType))!=0) return retval;
  if ((retval=util_add_type(module, &BarColIterType))!=0) return retval;
  if ((retval=Bar_InitType(module))!=0) return retval;
  return 0;
}

static PySequenceMethods BarCol_as_sequence = {
  (lenfunc)BarCol_Size,			/* sq_length */
  0,					/* sq_concat */
  0,					/* sq_repeat */
  (ssizeargfunc)BarCol_Bar,		/* sq_item */
  0, //(intintargfunc)svector_slice,	/* sq_slice */
  (ssizeobjargproc)BarCol_ass_item,	/* sq_ass_item */
  0,					/* sq_ass_slice */
  (objobjproc)BarCol_contains,		/* sq_contains */
};

static PyMappingMethods BarCol_as_mapping = {
  (lenfunc)BarCol_Size,			/* mp_length */
  (binaryfunc)BarCol_subscript,		/* mp_subscript */
  (objobjargproc)BarCol_ass_subscript	/* mp_ass_subscript */
};

static PyMemberDef BarCol_members[] = {
  {NULL}
};

static PyGetSetDef BarCol_getset[] = {
  {NULL}
};

static PyMethodDef BarCol_methods[] = {
  {"add", (PyCFunction)BarCol_add, METH_VARARGS,
   "add(n)\n\n"
   "Add n more rows (constraints) or columns (struct variables).\n"
   "Returns the index of the first added entry."},
  {NULL}
};

PyTypeObject BarColType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.BarCollection",			/* tp_name */
  sizeof(BarColObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)BarCol_dealloc,		/* tp_dealloc*/
  0,					/* tp_print*/
  0,					/* tp_getattr*/
  0,					/* tp_setattr*/
  0,					/* tp_compare*/
  0,					/* tp_repr*/
  0,					/* tp_as_number*/
  &BarCol_as_sequence,			/* tp_as_sequence*/
  &BarCol_as_mapping,			/* tp_as_mapping*/
  0,					/* tp_hash */
  0,					/* tp_call*/
  (reprfunc)BarCol_Str,			/* tp_str*/
  0,					/* tp_getattro*/
  0,					/* tp_setattro*/
  0,					/* tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags*/
  "Bar collection objects.  An instance is used to index into either\n"
  "the rows and columns of a linear program.  They exist as the 'rows'\n"
  "and 'cols' attributes of LPX instances.\n\n"
  "One accesses particular rows or columns by indexing the appropriate\n"
  "bar collection object, or iterating over it.  Valid indices include\n"
  "particular row and column names (a user defined string) or numbers\n"
  "(counting from 0), a slice specifying a range of numeric elements,\n"
  "or a series of individual indices.  For example, for an LPX instance\n"
  "lp, we may have:\n\n"
  "lp.rows[0]          --> the first row\n"
  "lp.rows[-1]         --> the last row\n"
  "lp.cols[:3]         --> the first three columns\n"
  "lp.cols[1,'name',5] --> column 1, a column named 'name', and column 5\n\n"
  "One may also query the length of this sequence to get the number of\n"
  "rows or columns, and del to get rid of rows or columns, e.g.:\n\n"
  "len(lp.cols)        --> the number of columns in the problem\n"
  "del lp.rows['arow'] --> deletes a row named 'arow'\n"
  "del lp.rows[-2:]    --> deletes the last two rows\n\n",
	/* tp_doc */
  (traverseproc)BarCol_traverse,	/* tp_traverse */
  (inquiry)BarCol_clear,		/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(BarColObject, weakreflist),	/* tp_weaklistoffset */
  BarCol_Iter,				/* tp_iter */
  0,					/* tp_iternext */
  BarCol_methods,			/* tp_methods */
  BarCol_members,			/* tp_members */
  BarCol_getset,			/* tp_getset */
  //0,					/* tp_base */
  //0,					/* tp_dict */
  //0,					/* tp_descr_get */
  //0,					/* tp_descr_set */
  //0,					/* tp_dictoffset */
  //(initproc)BarCol_init,		/* tp_init */
  //0,					/* tp_alloc */
  //BarCol_new,				/* tp_new */
};
