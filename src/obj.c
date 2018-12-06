/**************************************************************************
Copyright (C) 2007, 2008 Thomas Finley, tfinley@gmail.com

This file is part of PyGLPK.

PyGLPK is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

PyGLPK is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PyGLPK. If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "2to3.h"

#include "obj.h"
#include "barcol.h"
#include "util.h"
#include "structmember.h"

#define LP (self->py_lp->lp)

/** OBJECTIVE FUNCTION ITERATOR OBJECT IMPLEMENTATION **/

typedef struct {
  PyObject_HEAD
  int index;
  ObjObject *obj;
  PyObject *weakreflist; // Weak reference list.
} ObjIterObject;

static PyObject *Obj_Iter(PyObject *obj) {
  ObjIterObject *it;
  if (!Obj_Check(obj)) {
    PyErr_BadInternalCall();
    return NULL;
  }
  it = PyObject_New(ObjIterObject, &ObjIterType);
  if (it == NULL) return NULL;
  it->weakreflist = NULL;
  it->index = 0;
  Py_INCREF(obj);
  it->obj = (ObjObject *)obj;
  return (PyObject *)it;
}

static void ObjIter_dealloc(ObjIterObject *it) {
  if (it->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)it);
  }
  Py_XDECREF(it->obj);
  Py_TYPE(it)->tp_free((PyObject*)it);
}

static Py_ssize_t ObjIter_len(ObjIterObject *it) {
  int len=0;
  len = Obj_Size(it->obj) - it->index;
  return len >= 0 ? len : 0;
}

static PyObject *ObjIter_next(ObjIterObject *it) {
  if (Obj_Size(it->obj) - it->index <= 0) return NULL;
  it->index++;
  return PyFloat_FromDouble(glp_get_obj_coef(it->obj->py_lp->lp, it->index));
}

static PySequenceMethods objiter_as_sequence = {
    .sq_length = (lenfunc) ObjIter_len,
};

PyDoc_STRVAR(obj_iter_doc,
"Objective function iterator objects, used to cycle over the coefficients of\n"
"the objective function."
);

PyTypeObject ObjIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name           = "glpk.ObjectiveIter",
    .tp_basicsize      = sizeof(ObjIterObject),
    .tp_dealloc        = (destructor)ObjIter_dealloc,
    .tp_as_sequence    = &objiter_as_sequence,
    .tp_getattro       = PyObject_GenericGetAttr,
    .tp_flags          = Py_TPFLAGS_DEFAULT,
    .tp_doc            = obj_iter_doc,
    .tp_weaklistoffset = offsetof(ObjIterObject, weakreflist),
    .tp_iter           = PyObject_SelfIter,
    .tp_iternext       = (iternextfunc) ObjIter_next,
};

/************* OBJECTIVE FUNCTION OBJECT IMPLEMENTATION **********/

static int Obj_traverse(ObjObject *self, visitproc visit, void *arg) {
  Py_VISIT((PyObject*)self->py_lp);
  return 0;
}

static int Obj_clear(ObjObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_CLEAR(self->py_lp);
  return 0;
}

static void Obj_dealloc(ObjObject *self) {
  Obj_clear(self);
  Py_TYPE(self)->tp_free((PyObject*)self);
}

/** Create a new bar collection object. */
ObjObject *Obj_New(LPXObject *py_lp) {
  ObjObject *obj = (ObjObject*)PyObject_GC_New(ObjObject, &ObjType);
  if (obj==NULL) return obj;

  Py_INCREF(py_lp);
  obj->py_lp = py_lp;
  obj->weakreflist = NULL;

  PyObject_GC_Track(obj);
  return obj;
}

/********** ABSTRACT PROTOCOL FUNCTIONS *******/

int Obj_Size(ObjObject* self) {
  // There are as many objective values as there are columns.
  return glp_get_num_cols(LP);
}

static PyObject* Obj_subscript(ObjObject *self, PyObject *item) {
  int size = Obj_Size(self), index=0;
  BarColObject *bc = (BarColObject*) (self->py_lp->cols);

  if (PySlice_Check(item)) {
    // It's a slice. (Of zest!!)
    Py_ssize_t start, stop, step, subsize, i;
    PyObject *sublist = NULL;
#if PY_MAJOR_VERSION >= 3
    if (PySlice_GetIndicesEx(item,size,&start,&stop,
                 &step,&subsize)<0) return NULL;
#else
    if (PySlice_GetIndicesEx((PySliceObject*)item,size,&start,&stop,
			     &step,&subsize)<0) return NULL;
#endif
    sublist = PyList_New(subsize);
    if (sublist==NULL) return NULL;
    for (i=0; i<subsize; ++i) {
      PyObject *objcoef=PyFloat_FromDouble
	(glp_get_obj_coef(LP,1+start+i*step));
      if (objcoef==NULL) {
	Py_DECREF(sublist);
	return NULL;
      }
      PyList_SET_ITEM(sublist, i, objcoef);
    }
    return sublist;
  }

  if (PyTuple_Check(item)) {
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
      if (subitem==Py_None) {
	index=-1;
      } else if (subitem==NULL || BarCol_Index(bc, subitem, &index, -1)) {
	Py_DECREF(sublist);
	return NULL;
      }
      // Try to get the coefficient.
      PyObject *objcoef=PyFloat_FromDouble(glp_get_obj_coef(LP,index+1));
      if (objcoef==NULL) {
	Py_DECREF(sublist);
	return NULL;
      }
      PyList_SET_ITEM(sublist, i, objcoef);
    }
    return sublist;
  }
  
  if (Py_None == item) {
    // They input none. Bastards!
    return PyFloat_FromDouble(glp_get_obj_coef(LP,0));
  }
  
  if (BarCol_Index(bc, item, &index, -1)) return NULL;
  return PyFloat_FromDouble(glp_get_obj_coef(LP, index+1));

  return NULL;
}

// Take a PyObject and a pointer to a double, and modify the double so
// that it is the float interpretation of the Python object and return
// 0. If this cannot be done, throw an exception and return -1.
static int extract_double(PyObject *py_val, double *val) {
  if (!py_val) {
    PyErr_SetString(PyExc_TypeError, "deletion not supported");
    return -1;
  }
  py_val = PyNumber_Float(py_val);
  if (py_val == NULL) {
    PyErr_SetString(PyExc_TypeError, "a float is required");
    return -1;
  }
  *val = PyFloat_AsDouble(py_val);
  Py_DECREF(py_val);
  return 0;
}

static int Obj_ass_subscript(ObjObject *self,PyObject *item,PyObject *value) {
  int size = Obj_Size(self), index=0;
  double val = 0.0;
  BarColObject *bc = (BarColObject*) (self->py_lp->cols);

  if (value==NULL) {
    PyErr_SetString
      (PyExc_TypeError, "objective function doesn't support item deletion");
    return -1;
  }
  
  if (PySlice_Check(item)) {
    // Sliceness!  Again of zest!
    Py_ssize_t start, stop, step, subsize, i, valsize;
    PyObject *subval = NULL;
#if PY_MAJOR_VERSION >= 3
    if (PySlice_GetIndicesEx(item,size,&start,&stop,
                 &step,&subsize)<0) return -1;
#else
    if (PySlice_GetIndicesEx((PySliceObject*)item,size,&start,&stop,
			     &step,&subsize)<0) return -1;
#endif
    // Repeated single number assignment.
    if (PyNumber_Check(value)) {
      if (extract_double(value, &val)) return -1;
      for (i=0; i<subsize; ++i)
	glp_set_obj_coef(LP, 1+start+i*step, val);
      return 0;
    }
    // Try to get the length...
    if (PyErr_Occurred()) return -1;
    valsize = PyObject_Length(value);
    PyErr_Clear();

    if (valsize != -1 && valsize != subsize) {
      PyErr_Format(PyExc_ValueError, "cannot assign %d values to %d "
		   "objective coefficients", (int)valsize, (int)subsize);
      return -1;
    }
    // Attempt to iterate over stuff.
    value = PyObject_GetIter(value);
    if (value == NULL) return -1;
    for (i=0; i<subsize; ++i) {
      if ((subval = PyIter_Next(value))==NULL) break;
      if (extract_double(subval, &val)) {
	Py_DECREF(subval);
	Py_DECREF(value);
	return -1;
      }
      Py_DECREF(subval);
      glp_set_obj_coef(LP, 1+start+i*step, val);
    }
    Py_DECREF(value);
    // Check if our iteration ended prematurely.
    if (i < subsize) {
      if (PyErr_Occurred()) return -1;
      PyErr_Format(PyExc_ValueError,
		   "iterable returned only %d values of %d requested",
		   (int)i, (int)subsize);
      return -1;
    }
    return 0;
  }

  if (PyTuple_Check(item)) {
    Py_ssize_t subsize, i, valsize;
    PyObject *subitem, *subval;
    int index=0;

    subsize = PyTuple_Size(item);
    if (subsize==-1) return -1;
    
    // Repeated single number assignment.
    if (PyNumber_Check(value)) {
      if (extract_double(value, &val)) return -1;
      for (i=0; i<subsize; ++i) {
	if ((subitem = PyTuple_GET_ITEM(item, i))==NULL) return -1;
	if (BarCol_Index(bc, subitem, &index, -1)) return -1;
	glp_set_obj_coef(LP, index+1, val);
      }
      return 0;
    }
    // Try to get the length...
    if (PyErr_Occurred()) return -1;
    valsize = PyObject_Length(value);
    PyErr_Clear();

    if (valsize != -1 && valsize != subsize) {
      PyErr_Format(PyExc_ValueError, "cannot assign %d values to %d "
		   "objective coefficients", (int)valsize, (int)subsize);
      return -1;
    }
    // Attempt to iterate over the new value, and extract values and indices.
    value = PyObject_GetIter(value);
    if (value == NULL) return -1;
    for (i=0; i<subsize; ++i) {
      if ((subval = PyIter_Next(value))==NULL) break;
      if (extract_double(subval, &val)) {
	Py_DECREF(subval);
	Py_DECREF(value);
	return -1;
      }
      Py_DECREF(subval);
      
      if ((subitem = PyTuple_GET_ITEM(item, i))==NULL) {
	Py_DECREF(value);
	return -1;
      }
      if (subitem==Py_None) {
	index = -1;
      } else if (BarCol_Index(bc, subitem, &index, -1)) {
	Py_DECREF(value);
	return -1;
      }
      glp_set_obj_coef(LP, index+1, val);
    }
    Py_DECREF(value);
    // Check if our iteration ended prematurely.
    if (i < subsize) {
      if (PyErr_Occurred()) return -1;
      PyErr_Format(PyExc_ValueError,
		   "iterable returned only %d values of %d requested",
		   (int)i, (int)subsize);
      return -1;
    }
    return 0;
  }

  if (item == Py_None) {
    if (extract_double(value, &val)) return -1;
    glp_set_obj_coef(LP, 0, val);
    return 0;
  }

  // Last possibility is a single index.
  if (BarCol_Index(bc, item, &index, -1)) return -1;
  if (extract_double(value, &val)) return -1;
  glp_set_obj_coef(LP, index+1, val);
  return 0;
}

/****************** GET-SET-ERS ***************/

static PyObject* Obj_getname(ObjObject *self, void *closure) {
  const char *name = glp_get_obj_name(LP);
  if (name==NULL) Py_RETURN_NONE;
  return PyString_FromString(name);
}
static int Obj_setname(ObjObject *self, PyObject *value, void *closure) {
  char *name;
  if (value==NULL || value==Py_None) {
    glp_set_obj_name(LP, NULL);
    return 0;
  }
  name = PyString_AsString(value);
  if (name==NULL) return -1;
  if (PyString_Size(value) > 255) {
    PyErr_SetString(PyExc_ValueError, "name may be at most 255 chars");
    return -1;
  }
  glp_set_obj_name(LP, name);
  return 0;
}

static PyObject* Obj_getmaximize(ObjObject *self, void *closure) {
  return PyBool_FromLong(glp_get_obj_dir(LP)==GLP_MAX);
}
static int Obj_setmaximize(ObjObject *self, PyObject *value, void *closure) {
  int tomax = PyObject_IsTrue(value);
  if (tomax < 0) return -1;
  glp_set_obj_dir(LP, tomax ? GLP_MAX : GLP_MIN);
  return 0;
}

static PyObject* Obj_getshift(ObjObject *self, void *closure) {
  return PyFloat_FromDouble(glp_get_obj_coef(LP, 0));
}
static int Obj_setshift(ObjObject *self, PyObject *value, void *closure) {
  double v=0.0;
  if (extract_double(value, &v)) return -1;
  glp_set_obj_coef(LP, 0, v);
  return 0;
}

static PyObject* Obj_getvalue(ObjObject *self, void *closure) {
  switch (self->py_lp->last_solver) {
  case -1:
  case 0: return PyFloat_FromDouble(glp_get_obj_val(LP));
  case 1: return PyFloat_FromDouble(glp_ipt_obj_val(LP));
  case 2: return PyFloat_FromDouble(glp_mip_obj_val(LP));
  default: 
    PyErr_SetString(PyExc_RuntimeError,
		    "bad internal state for last solver identifier");
    return NULL;
  }
}
static PyObject* Obj_getspecvalue(ObjObject *self, double(*objvalfunc)(glp_prob *)) {
  return PyFloat_FromDouble(objvalfunc(LP));
}

/****************** OBJECT DEFINITION *********/

int Obj_InitType(PyObject *module) {
  int retval;
  if ((retval=util_add_type(module, &ObjType))!=0) return retval;
  if ((retval=util_add_type(module, &ObjIterType))!=0) return retval;
  return 0;
}

static PySequenceMethods Obj_as_sequence = {
    .sq_length = (lenfunc) Obj_Size,
};

static PyMappingMethods Obj_as_mapping = {
    .mp_length        = (lenfunc) Obj_Size,
    .mp_subscript     = (binaryfunc) Obj_subscript,
    .mp_ass_subscript = (objobjargproc) Obj_ass_subscript
};

static PyMemberDef Obj_members[] = {
  {NULL}
};

PyDoc_STRVAR(name_doc, "Objective name, or None if unset.");

PyDoc_STRVAR(maximize_doc,
"True or False depending on whether we are trying to maximize or minimize\n"
"this objective function, respectively."
);

PyDoc_STRVAR(shift_doc, "The constant shift term of the objective function.");

PyDoc_STRVAR(value_doc, "The current value of the objective function.");

PyDoc_STRVAR(value_s_doc,
"The current value of the simplex objective function."
);

PyDoc_STRVAR(value_i_doc,
"The current value of the interior point objective function."
);

PyDoc_STRVAR(value_m_doc, "The current value of the MIP objective function.");

static PyGetSetDef Obj_getset[] = {
  {"name", (getter)Obj_getname, (setter)Obj_setname, name_doc, NULL},
  {"maximize", (getter)Obj_getmaximize, (setter)Obj_setmaximize,
   maximize_doc, NULL},
  {"shift", (getter)Obj_getshift, (setter)Obj_setshift, shift_doc, NULL},
  // Objective function value getters...
  {"value", (getter)Obj_getvalue, (setter)NULL, value_doc, NULL},
  {"value_s", (getter)Obj_getspecvalue, (setter)NULL, value_s_doc,
  (void*)glp_get_obj_val},
  {"value_i", (getter)Obj_getspecvalue, (setter)NULL, value_i_doc,
   (void*)glp_ipt_obj_val},
  {"value_m", (getter)Obj_getspecvalue, (setter)NULL, value_m_doc,
   (void*)glp_mip_obj_val},
  {NULL}
};

static PyMethodDef Obj_methods[] = {
  {NULL}
};

PyDoc_STRVAR(obj_doc,
"Objective function objects for linear programs.\n"
"\n"
"An instance is used either to access objective function values for\n"
"solutions, or to access or set objective function coefficients. The same\n"
"indices valid for a BarCollection object over the columns (that is, column\n"
"numeric indices, column names, slices, multiple values) are also valid for\n"
"indexing into this object.\n"
"The special index None is used to specify the shift term. If we have an LPX\n"
"instance lp, we may have::\n"
"\n"
"    lp.obj[0]    # the first objective coefficient\n"
"    lp.obj[None] # the shift term\n"
"    lp.obj[-3:]  # the last three objective coefficients\n\n"
"    lp.obj[1, 4] # the objective coefficients 1, 4\n"
"\n"
"When assigning objective coefficients, for single indices one may assign a\n"
"single number. For multiple indices, one may assign a single number to make\n"
"all indicated coefficients identical, or specify an iterable of equal\n"
"length to set them all individiaully. For example::\n"
"\n"
"    lp.obj[0] = 2.5          # set the first objective coef to 2.5\n"
"    lp.obj[-3:] = 1.0        # the last three obj coefs get 1.0\n"
"    lp.obj[1, 4] = -2.0, 2.0 # obj coefs 1, 4 get -2.0, 2.0"
);

PyTypeObject ObjType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name           = "glpk.Objective",
    .tp_basicsize      = sizeof(ObjObject),
    .tp_dealloc        = (destructor) Obj_dealloc,
    .tp_as_sequence    = &Obj_as_sequence,
    .tp_as_mapping     = &Obj_as_mapping,
    .tp_flags          = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc            = obj_doc,
    .tp_traverse       = (traverseproc) Obj_traverse,
    .tp_clear          = (inquiry) Obj_clear,
    .tp_weaklistoffset = offsetof(ObjObject, weakreflist),
    .tp_iter           = Obj_Iter,
    .tp_methods        = Obj_methods,
    .tp_members        = Obj_members,
    .tp_getset         = Obj_getset,
};
