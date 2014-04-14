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
  it->ob_type->tp_free((PyObject*)it);
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
  (lenfunc)ObjIter_len, /* sq_length */
  0, /* sq_concat */
};

PyTypeObject ObjIterType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.ObjectiveIter",			/* tp_name */
  sizeof(ObjIterObject),		/* tp_basicsize */
  0,					/* tp_itemsize */
  (destructor)ObjIter_dealloc,		/* tp_dealloc */
  0,					/* tp_print */
  0,					/* tp_getattr */
  0,					/* tp_setattr */
  0,					/* tp_compare */
  0,					/* tp_repr */
  0,					/* tp_as_number */
  &objiter_as_sequence,			/* tp_as_sequence */
  0,					/* tp_as_mapping */
  0,					/* tp_hash */
  0,					/* tp_call */
  0,					/* tp_str */
  PyObject_GenericGetAttr,		/* tp_getattro */
  0,					/* tp_setattro */
  0,					/* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,			/* tp_flags */
  "Objective function iterator objects, used to cycle over the\n"
  "coefficients of the objective function.",	
  /* tp_doc */
  0,					/* tp_traverse */
  0,					/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(ObjIterObject, weakreflist),	/* tp_weaklistoffset */
  PyObject_SelfIter,			/* tp_iter */
  (iternextfunc)ObjIter_next,		/* tp_iternext */
  0,					/* tp_methods */
  0,					/* tp_members */
  0,					/* tp_getset */
  0,					/* tp_base */
  0,					/* tp_dict */
  0,					/* tp_descr_get */
  0,					/* tp_descr_set */
  0,					/* tp_dictoffset */
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
  self->ob_type->tp_free((PyObject*)self);
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
  return lpx_get_num_cols(LP);
}

static PyObject* Obj_subscript(ObjObject *self, PyObject *item) {
  int size = Obj_Size(self), index=0;
  BarColObject *bc = (BarColObject*) (self->py_lp->cols);

  if (PySlice_Check(item)) {
    // It's a slice.  (Of zest!!)
    Py_ssize_t start, stop, step, subsize, i;
    PyObject *sublist = NULL;
    if (PySlice_GetIndicesEx((PySliceObject*)item,size,&start,&stop,
			     &step,&subsize)<0) return NULL;
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
    // They input none.  Bastards!
    return PyFloat_FromDouble(glp_get_obj_coef(LP,0));
  }
  
  if (BarCol_Index(bc, item, &index, -1)) return NULL;
  return PyFloat_FromDouble(glp_get_obj_coef(LP, index+1));

  return NULL;
}

// Take a PyObject and a pointer to a double, and modify the double so
// that it is the float interpretation of the Python object and return
// 0.  If this cannot be done, throw an exception and return -1.
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
    if (PySlice_GetIndicesEx((PySliceObject*)item,size,&start,&stop,
			     &step,&subsize)<0) return -1;
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

static int Obj_ass_item(ObjObject *self, int index, PyObject *v) {
  printf("obj ass item\n");
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
static PyObject* Obj_getspecvalue(ObjObject *self, double(*objvalfunc)(LPX*)) {
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
  (lenfunc)Obj_Size,			/* sq_length */
  0,					/* sq_concat */
  0,					/* sq_repeat */
  0,					/* sq_item */
  0, //(intintargfunc)svector_slice,	/* sq_slice */
  (ssizeobjargproc)Obj_ass_item,	/* sq_ass_item */
  0,					/* sq_ass_slice */
  0, //(objobjproc)svcontains,		/* sq_contains */
};

static PyMappingMethods Obj_as_mapping = {
  (lenfunc)Obj_Size,			/* mp_length */
  (binaryfunc)Obj_subscript,		/* mp_subscript */
  (objobjargproc)Obj_ass_subscript	/* mp_ass_subscript */
};

static PyMemberDef Obj_members[] = {
  {NULL}
};

static PyGetSetDef Obj_getset[] = {
  {"name", (getter)Obj_getname, (setter)Obj_setname,
   "Objective name, or None if unset.", NULL},
  {"maximize", (getter)Obj_getmaximize, (setter)Obj_setmaximize,
   "True or False depending on whether we are trying to maximize\n"
   "or minimize this objective function, respectively.", NULL},
  {"shift", (getter)Obj_getshift, (setter)Obj_setshift,
   "The constant shift term of the objective function.", NULL},
  // Objective function value getters...
  {"value", (getter)Obj_getvalue, (setter)NULL,
   "The current value of the objective function.", NULL},
  {"value_s", (getter)Obj_getspecvalue, (setter)NULL,
   "The current value of the simplex objective function.",
   (void*)glp_get_obj_val},
  {"value_i", (getter)Obj_getspecvalue, (setter)NULL,
   "The current value of the interior point objective function.",
   (void*)glp_ipt_obj_val},
  {"value_m", (getter)Obj_getspecvalue, (setter)NULL,
   "The current value of the MIP objective function.",
   (void*)glp_mip_obj_val},
  {NULL}
};

static PyMethodDef Obj_methods[] = {
  {NULL}
};

PyTypeObject ObjType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.Objective",			/* tp_name */
  sizeof(ObjObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)Obj_dealloc,		/* tp_dealloc*/
  0,					/* tp_print*/
  0,					/* tp_getattr*/
  0,					/* tp_setattr*/
  0,					/* tp_compare*/
  0,					/* tp_repr*/
  0,					/* tp_as_number*/
  &Obj_as_sequence,			/* tp_as_sequence*/
  &Obj_as_mapping,			/* tp_as_mapping*/
  0,					/* tp_hash */
  0,					/* tp_call*/
  0,					/* tp_str*/
  0,					/* tp_getattro*/
  0,					/* tp_setattro*/
  0,					/* tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags*/
  "Objective function objects for linear programs.  An instance is\n"
  "used either to access objective function values for solutions,\n"
  "or to access or set objective function coefficients.  The same\n"
  "indices valid for a BarCollection object over the columns\n"
  "(that is, column numeric indices, column names, slices,\n"
  "multiple values) are also valid for indexing into this object.\n"
  "The special index None is used to specify the shift term.  If\n"
  "we have an LPX instance lp, we may have:\n\n"
  "lp.obj[0]    --> the first objective coefficient\n"
  "lp.obj[None] --> the shift term\n"
  "lp.obj[-3:]  --> the last three objective coefficients\n\n"
  "lp.obj[1,4]  --> the objective coefficients 1, 4\n"
  "When assigning objective coefficients, for single indices one\n"
  "may assign a single number.  For multiple indices, one may\n"
  "assign a single number to make all indicated coefficients\n"
  "identical, or specify an iterable of equal length to set them\n"
  "all individiaully.  For example:\n\n"
  "lp.obj[0]=2.5        --> set the first objective coef to 2.5\n"
  "lp.obj[-3:]=1.0      --> the last three obj coefs get 1.0\n"
  "lp.obj[1,4]=-2.0,2.0 --> obj coefs 1, 4 get -2.0, 2.0",
  /* tp_doc */
  (traverseproc)Obj_traverse,		/* tp_traverse */
  (inquiry)Obj_clear,			/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(ObjObject, weakreflist),	/* tp_weaklistoffset */
  Obj_Iter,				/* tp_iter */
  0,					/* tp_iternext */
  Obj_methods,				/* tp_methods */
  Obj_members,				/* tp_members */
  Obj_getset,				/* tp_getset */
  //0,					/* tp_base */
  //0,					/* tp_dict */
  //0,					/* tp_descr_get */
  //0,					/* tp_descr_set */
  //0,					/* tp_dictoffset */
  //(initproc)Obj_init,			/* tp_init */
  //0,					/* tp_alloc */
  //Obj_new,				/* tp_new */
};
