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

#include "bar.h"
#include "structmember.h"
#include "util.h"
#include <string.h>

#define LP (self->py_bc->py_lp->lp)

//#define USE_BAR_GC // Cyclic GC on the bar incurs a roughly 10-20% cost.

static int Bar_traverse(BarObject *self, visitproc visit, void *arg) {
  Py_VISIT((PyObject*)self->py_bc);
  return 0;
}

static int Bar_clear(BarObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_CLEAR(self->py_bc);
  return 0;
}

static void Bar_dealloc(BarObject *self) {
#ifdef USE_BAR_GC
  Bar_clear(self);
#else
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_DECREF(self->py_bc);
#endif
  self->ob_type->tp_free((PyObject*)self);
}

/** Create a new bar collection object. */
BarObject *Bar_New(BarColObject *py_bc, int index) {
  BarObject *b = NULL;
  // Check whether the bar is valid.
  if (index < 0 || index >= BarCol_Size(py_bc)) {
    PyErr_SetString(PyExc_IndexError, "row or column index out of bounds");
    return b;
  }
  // Input all these fun structures and things.
#ifdef USE_BAR_GC
  b = (BarObject*)PyObject_GC_New(BarObject, &BarType);
#else
  b = (BarObject*)PyObject_New(BarObject, &BarType);
#endif
  Py_INCREF(py_bc);
  b->weakreflist = NULL;
  b->py_bc = py_bc;
  b->r = py_bc->r;
  b->index = index;
#ifdef USE_BAR_GC
  PyObject_GC_Track(b);
#endif
  return b;
}

static PyObject* Bar_Str(BarObject *self) {
  // Returns a string representation of this object.
  return PyString_FromFormat
    ("<%s, %s %d of %s %p>", self->ob_type->tp_name,
     Bar_Row(self)?"row":"col", Bar_Index(self),
     LPXType.tp_name, self->py_bc->py_lp);
}

int Bar_Valid(BarObject *self, int except) {
  if (self->index < BarCol_Size(self->py_bc)) return 1;
  // It's invalid!  Curses.
  if (except) {
    PyErr_SetString(PyExc_RuntimeError, "row or column no longer valid");
  }
  return 0;
}

PyObject *Bar_GetMatrix(BarObject *self) {
  int nnz, i;
  PyObject *retval;
  int (*get_mat)(glp_prob*,int,int[],double[]);

  if (!Bar_Valid(self, 1)) return NULL;

  get_mat = Bar_Row(self) ? glp_get_mat_row : glp_get_mat_col;
  i = Bar_Index(self)+1;
  nnz = get_mat(LP, i, NULL, NULL);
  retval = PyList_New(nnz);
  if (nnz==0 || retval==NULL) return retval;
  
  int*ind = (int*)calloc(nnz,sizeof(int));
  double*val = (double*)calloc(nnz,sizeof(double));
  nnz = get_mat(LP, i, ind-1, val-1);
  
  for (i=0; i<nnz; ++i) {
    PyList_SET_ITEM(retval, i, Py_BuildValue("id", ind[i]-1, val[i]));
  }
  free(ind);
  free(val);
  if (PyList_Sort(retval)) {
    Py_DECREF(retval);
    return NULL;
  }
  return retval;
}

int Bar_SetMatrix(BarObject *self, PyObject *newvals) {
  int len, *ind;
  double*val;
  if (!Bar_Valid(self, 1)) return -1;
  // Get the alternate length (e.g., col length if this is a row).
  len = (Bar_Row(self) ? lpx_get_num_cols : lpx_get_num_rows)(LP);
  // Now, attempt to convert the input item.
  if (newvals == NULL || newvals == Py_None) {
    len = 0;
  } else {
    PyObject*bc = Bar_Row(self) ?
      self->py_bc->py_lp->cols : self->py_bc->py_lp->rows;
    if (!util_extract_if(newvals, bc, &len, &ind, &val)) return -1;
  }
  // Input the stuff into the LP constraint matrix.
  (Bar_Row(self) ? glp_set_mat_row : glp_set_mat_col)
    (LP, Bar_Index(self)+1, len, ind-1, val-1);
  // Free the memory.
  if (len) {
    free(ind);
    free(val);
  }
  return 0;
}

static PyObject* Bar_richcompare(BarObject *v, PyObject *w, int op) {
  if (!Bar_Check(w)) {
    switch (op) {
    case Py_EQ: Py_RETURN_FALSE;
    case Py_NE: Py_RETURN_FALSE;
    default:
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
    }
  }
  // Now we know it is a bar object.
  BarObject *x = (BarObject *)w;
  if (v->py_bc != x->py_bc) {
    // "Inherit" the judgement of our containing objects.
    return PyObject_RichCompare((PyObject*)v->py_bc, (PyObject*)x->py_bc, op);
  }
  // Now we know it is a bar object, and part of the same bar
  // collection no less.
  switch (op) {
  case Py_EQ: if (v->index==x->index) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_NE: if (v->index!=x->index) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_LE: if (v->index<=x->index) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_GE: if (v->index>=x->index) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_LT: if (v->index< x->index) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_GT: if (v->index> x->index) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  default:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

/********** ABSTRACT PROTOCOL FUNCTIONS *******/

/*int Bar_Size(BarObject* self) {
  return (Bar_Row(self) ? lpx_get_num_rows : lpx_get_num_cols)(LP); }
static PyObject* Bar_subscript(BarObject *self, PyObject *item) {
  printf("bar subscript\n"); Py_RETURN_NONE;}
static int Bar_ass_subscript(BarObject *self,PyObject *item,PyObject *value) {
 printf("bar ass subscript\n"); return 0; }
static PyObject* Bar_item(BarObject *self, int index) {
  printf("bar item\n"); Py_RETURN_NONE; }
static int Bar_ass_item(BarObject *self, int index, PyObject *v) {
  printf("bar ass item\n"); return 0; }*/

/****************** GET-SET-ERS ***************/
static PyObject* Bar_getname(BarObject *self, void *closure) {
  if (!Bar_Valid(self, 1)) return NULL;
  const char *name = (Bar_Row(self) ? glp_get_row_name : glp_get_col_name)
    (LP, Bar_Index(self)+1);
  if (name==NULL) Py_RETURN_NONE;
  return PyString_FromString(name);
}
static int Bar_setname(BarObject *self, PyObject *value, void *closure) {
  char *name;
  if (!Bar_Valid(self, 1)) return -1;
  if (value==NULL || value==Py_None) {
    (Bar_Row(self) ? glp_set_row_name : glp_set_col_name)
      (LP, Bar_Index(self)+1, NULL);
    return 0;
  }
  name = PyString_AsString(value);
  if (name==NULL) return -1;
  if (PyString_Size(value) > 255) {
    PyErr_SetString(PyExc_ValueError, "name may be at most 255 chars");
    return -1;
  }
  (Bar_Row(self) ? lpx_set_row_name : lpx_set_col_name)
    (LP, Bar_Index(self)+1, name);
  return 0;
}

static PyObject* Bar_getvalid(BarObject *self, void *closure) {
  return PyBool_FromLong(Bar_Valid(self, 0));
}

static PyObject* Bar_getbounds(BarObject *self, void *closure) {
  double lb, ub;
  int i;
  if (!Bar_Valid(self, 1)) return NULL;
  
  i = Bar_Index(self)+1;
  lb = (Bar_Row(self) ? glp_get_row_lb : glp_get_col_lb)(LP, i);
  ub = (Bar_Row(self) ? glp_get_row_ub : glp_get_col_ub)(LP, i);

  switch ((Bar_Row(self) ? glp_get_row_type : glp_get_col_type)(LP, i)) {
  case GLP_FR: return Py_BuildValue("OO", Py_None, Py_None);
  case GLP_LO: return Py_BuildValue("fO", lb, Py_None);
  case GLP_UP: return Py_BuildValue("Of", Py_None, ub);
  case GLP_DB: case GLP_FX: return Py_BuildValue("ff", lb, ub);
  }
  // We should never be here.
  PyErr_SetString(PyExc_SystemError, "unrecognized bound type");
  return NULL;
}
static int Bar_setbounds(BarObject *self, PyObject *value, void *closure) {
  int i;
  double lb=0.0, ub=0.0;
  PyObject *lo, *uo;

  void (*bounder)(glp_prob*,int,int,double,double) = NULL;
  if (!Bar_Valid(self, 1)) return -1;

  i = Bar_Index(self)+1;
  bounder = Bar_Row(self) ? glp_set_row_bnds : glp_set_col_bnds;

  if (value==NULL || value==Py_None) {
    // We want it unbounded and free.
    bounder(LP, i, GLP_FR, 0.0, 0.0);
    return 0;
  }

  if (PyNumber_Check(value)) {
    // We want an equality fixed bound.
    value = PyNumber_Float(value);
    if (!value) return -1;
    lb = PyFloat_AsDouble(value);
    Py_DECREF(value);
    if (PyErr_Occurred()) return -1;
    bounder(LP, i, GLP_FX, lb, lb);
    return 0;
  }

  char t_error[] = "bounds must be set to None, number, or pair of numbers";
  if (!PyTuple_Check(value) || PyTuple_GET_SIZE(value)!=2) {
    PyErr_SetString(PyExc_TypeError, t_error);
    return -1;
  }
  
  // Get the lower and upper object.  These references are borrowed.
  lo = PyTuple_GetItem(value, 0);
  uo = PyTuple_GetItem(value, 1);

  if ((lo!=Py_None && !PyNumber_Check(lo)) || 
      (uo!=Py_None && !PyNumber_Check(uo))) {
    PyErr_SetString(PyExc_TypeError, t_error);
    return -1;
  }
  if (lo==Py_None) lo=NULL; else lb=PyFloat_AsDouble(lo);
  if (PyErr_Occurred()) return -1;
  if (uo==Py_None) uo=NULL; else ub=PyFloat_AsDouble(uo);
  if (PyErr_Occurred()) return -1;
  
  if (!lo && !uo)	bounder(LP, i, GLP_FR, 0.0, 0.0);
  else if (!uo)		bounder(LP, i, GLP_LO, lb, 0.0);
  else if (!lo)		bounder(LP, i, GLP_UP, 0.0, ub);
  else if (lb<=ub)	bounder(LP, i, lb==ub ? GLP_FX : GLP_DB, lb, ub);
  else {
    PyErr_SetString(PyExc_ValueError, "lower bound cannot exceed upper bound");
    return -1;
  }
  return 0;
}

static PyObject* Bar_getnumnonzero(BarObject *self, void *closure) {
  if (!Bar_Valid(self, 1)) return NULL;
  return PyInt_FromLong((Bar_Row(self) ? glp_get_mat_row : glp_get_mat_col)
			(LP, Bar_Index(self)+1, NULL, NULL));
}

static PyObject* Bar_getmatrix(BarObject *self, void *closure) {
  return Bar_GetMatrix(self);
}
static int Bar_setmatrix(BarObject *self, PyObject *value, void *closure) {
  return Bar_SetMatrix(self, value);
}

static PyObject* Bar_getisrow(BarObject *self, void *closure) {
  return PyBool_FromLong(Bar_Row(self));
}
static PyObject* Bar_getiscol(BarObject *self, void *closure) {
  return PyBool_FromLong(!(Bar_Row(self)));
}

static PyObject* Bar_getscale(BarObject *self, void *closure) {
  if (!Bar_Valid(self, 1)) return NULL;
  int index;
  double scale;
  index = Bar_Index(self);
  scale = (Bar_Row(self) ? glp_get_rii : glp_get_sjj)(LP,index+1);
  return PyFloat_FromDouble(scale);
}

static int Bar_setscale(BarObject *self, PyObject *value, void *closure){
  if (!Bar_Valid(self, 1)) return -1;
  if (value==NULL) {
    PyErr_SetString(PyExc_AttributeError, "cannot delete scale");
    return -1;
  }
  if (PyNumber_Check(value)) {
    int index;
    double scale;
    index = Bar_Index(self);
    scale = PyFloat_AsDouble(value);
    if (PyErr_Occurred()) return -1;
    if (scale <= 0.0) {
      PyErr_SetString(PyExc_ValueError, "scale factors must be positive");
      return -1;
    }
    (Bar_Row(self) ? glp_set_rii : glp_set_sjj)(LP, index+1, scale);
  } else {
    PyErr_SetString(PyExc_TypeError, "scale factors must be numeric");
    return -1;
  }
  return 0;
}

static PyObject* Bar_getstatus(BarObject *self, void *closure) {
  if (!Bar_Valid(self, 1)) return NULL;
  int index, status;
  index = Bar_Index(self);
  status = (Bar_Row(self) ? glp_get_row_stat : glp_get_col_stat)(LP,index+1);
  switch (status) {
  case GLP_BS: return PyString_FromString("bs");
  case GLP_NL: return PyString_FromString("nl");
  case GLP_NU: return PyString_FromString("nu");
  case GLP_NF: return PyString_FromString("nf");
  case GLP_NS: return PyString_FromString("ns");
  default:
    PyErr_Format(PyExc_RuntimeError, "unknown status %d detected", status);
    return NULL;
  }
}
static int Bar_setstatus(BarObject *self, PyObject *value, void *closure) {
  if (!Bar_Valid(self, 1)) return -1;
  if (value==NULL) {
    PyErr_SetString(PyExc_AttributeError, "cannot delete status");
    return -1;
  }
  char *sstr = PyString_AsString(value);
  if (sstr == NULL) return -1;
  if (sstr[0]==0 || sstr[1]==0 || sstr[2]!=0) {
    PyErr_SetString(PyExc_ValueError, "status strings must be length 2");
    return -1;
  }
  int status;
  // Whee...
  if      (!strncmp("bs", sstr, 2)) status=GLP_BS;
  else if (!strncmp("nl", sstr, 2)) status=GLP_NL;
  else if (!strncmp("nu", sstr, 2)) status=GLP_NU;
  else if (!strncmp("nf", sstr, 2)) status=GLP_NF;
  else if (!strncmp("ns", sstr, 2)) status=GLP_NS;
  else {
    PyErr_Format
      (PyExc_ValueError, "status string value '%s' unrecognized", sstr);
    return -1;
  }
  (Bar_Row(self) ? lpx_set_row_stat : lpx_set_col_stat)
    (LP, Bar_Index(self)+1, status);
  return 0;
}

static PyObject* Bar_getkind(BarObject *self, void *closure) {
  PyObject *retval=NULL;
  int kind = GLP_CV;
  if (!Bar_Valid(self, 1)) return NULL;
  if (!Bar_Row(self)) {
    kind = glp_get_col_kind(LP, Bar_Index(self)+1);
  }
  switch (kind) {
  case GLP_CV: retval = (PyObject*)&PyFloat_Type; break;
  case GLP_IV: retval = (PyObject*)&PyInt_Type;   break;
  case GLP_BV: retval = (PyObject*)&PyBool_Type;  break;
  default:
    PyErr_SetString(PyExc_RuntimeError,
		    "unexpected variable kind encountered");
    return NULL;
  }
  Py_INCREF(retval);
  return retval;
}
static int Bar_setkind(BarObject *self, PyObject *value, void *closure) {
  if (value==(PyObject*)&PyFloat_Type) {
    // Float indicates a continuous variables.
    if (Bar_Row(self)) return 0;
    glp_set_col_kind(LP, Bar_Index(self)+1, GLP_CV);
    return 0;
  } else if (value==(PyObject*)&PyInt_Type) {
    // Integer indicates an integer variable.
    if (Bar_Row(self)) {
      PyErr_SetString(PyExc_ValueError, "row variables cannot be integer");
      return -1;
    }
    glp_set_col_kind(LP, Bar_Index(self)+1, GLP_IV);
    return 0;
  } else if (value==(PyObject*)&PyBool_Type) {
    // Boolean indicates a binary variable.
    if (Bar_Row(self)) {
      PyErr_SetString(PyExc_ValueError, "row variables cannot be binary");
      return -1;
    }
    glp_set_col_kind(LP, Bar_Index(self)+1, GLP_BV);
    return 0;
  } else {
    PyErr_SetString(PyExc_ValueError,
		    "either the type float, int, or bool is required");
    return -1;
  }
}

/****************** THE DUAL/PRIMAL GETTING CODE **********/

// Indexed by (last_solver*4 + isdual*2 + isrow)
static double(*rowcol_primdual_funcptrs[])(glp_prob*,int) = {
  glp_get_col_prim, glp_get_row_prim, glp_get_col_dual, glp_get_row_dual,
  glp_ipt_col_prim, glp_ipt_row_prim, glp_ipt_col_dual, glp_ipt_row_dual,
  glp_mip_col_val,  glp_mip_row_val,   NULL,            NULL };

static PyObject* Bar_getvarval(BarObject *self, void *closure) {
  if (!Bar_Valid(self, 1)) return NULL;
  // Compute what we need to index to get the appropriate function pointer.
  int isrow = Bar_Row(self) ? 1 : 0;
  int isdual = closure==NULL ? 0 : 1;
  int last = self->py_bc->py_lp->last_solver;
  if (last < 0) last = 0; // If no solver called yet, assume simplex is OK.
  if (last > 2) {
    PyErr_Format(PyExc_RuntimeError,
		 "bad internal state for last solver identifier: %d", last);
    return NULL;
  }
  // Get and verify that function pointer.
  double(*valfunc)(glp_prob*,int) =
    rowcol_primdual_funcptrs[last*4 + isdual*2 + isrow];
  if (valfunc==NULL) {
    PyErr_SetString(PyExc_RuntimeError,
		    "dual values do not exist for MIP solver");
    return NULL;
  }
  // Get whatever sort of variable this is and return it.
  return PyFloat_FromDouble(valfunc(LP, Bar_Index(self)+1));
}

static PyObject* Bar_getspecvarval(BarObject *self,
				   double(*valfuncs[])(glp_prob*, int)) {
  if (!Bar_Valid(self, 1)) return NULL;
  double(*valfunc)(glp_prob*, int) = valfuncs[Bar_Row(self) ? 1 : 0];
  return PyFloat_FromDouble(valfunc(LP, Bar_Index(self)+1));
}

static PyObject* Bar_getspecvarvalm(BarObject *self,
				    double(*valfuncs[])(glp_prob*, int)) {
  if (!Bar_Valid(self, 1)) return NULL;
  if (lpx_get_class(LP)!=LPX_MIP) {
    PyErr_SetString(PyExc_TypeError, 
		    "MIP values require mixed integer problem");
    return NULL;
  }
  double(*valfunc)(glp_prob*, int) = valfuncs[Bar_Row(self) ? 1 : 0];
  return PyFloat_FromDouble(valfunc(LP, Bar_Index(self)+1));
}

/****************** OBJECT DEFINITION *********/

//static PySequenceMethods Bar_as_sequence = {
//  (lenfunc)Bar_Size,			/* sq_length */
//  0,					/* sq_concat */
//  0,					/* sq_repeat */
//  (ssizeargfunc)Bar_item,		/* sq_item */
//  0, //(intintargfunc)svector_slice,	/* sq_slice */
//  (ssizeobjargproc)Bar_ass_item,	/* sq_ass_item */
//  0,					/* sq_ass_slice */
//  0, //(objobjproc)svcontains,		/* sq_contains */
//};

//static PyMappingMethods Bar_as_mapping = {
//  (lenfunc)Bar_Size,			/* mp_length */
//  (binaryfunc)Bar_subscript,		/* mp_subscript */
//  (objobjargproc)Bar_ass_subscript	/* mp_ass_subscript */
//};

int Bar_InitType(PyObject *module) {
  int retval;
  if ((retval=util_add_type(module, &BarType))!=0) return retval;

  // These are used in setting the stat

  return 0;
}

static PyMemberDef Bar_members[] = {
  {"index", T_INT, offsetof(BarObject, index), READONLY,
   "The index of the row or column this object refers to."},
  {NULL}
};

static PyGetSetDef Bar_getset[] = {
  {"name", (getter)Bar_getname, (setter)Bar_setname,
   "Row/column symbolic name, or None if unset.", NULL},
  {"bounds", (getter)Bar_getbounds, (setter)Bar_setbounds,
   "The lower and upper bounds, where None signifies unboundedness.", NULL},
  {"valid", (getter)Bar_getvalid, (setter)NULL,
   "Whether this row or column has a valid index in its LP.", NULL},
  {"matrix", (getter)Bar_getmatrix, (setter)Bar_setmatrix,
   "Non-zero constraint coefficients in this row/column vector\n"
   "as a list of two-element (index, value) tuples.",
   NULL},
  {"nnz", (getter)Bar_getnumnonzero, (setter)NULL,
   "Number of non-zero constraint elements in this row/column.", NULL},
  {"isrow", (getter)Bar_getisrow, (setter)NULL,
   "Whether this is a row.", NULL},
  {"iscol", (getter)Bar_getiscol, (setter)NULL,
   "Whether this is a column.", NULL},

  {"scale", (getter)Bar_getscale, (setter)Bar_setscale,
   "The scale for the row or column.  This is a factor which one may\n"
   "set to improve conditioning in the problem.  Most users will want\n"
   "to use the LPX.scale() method rather than setting these directly.\n"
   "The resulting constraint matrix is such that the entry at row i\n"
   "and column j is (for the purpose of optimization) (ri)*(aij)*(sj)\n"
   "where ri and sj are the row and column scaling factors, and aij\n"
   "is the entry of the constraint matrix."
   , NULL},

  {"status", (getter)Bar_getstatus, (setter)Bar_setstatus,
   "Row/column basis status.  This is a two character string with\n"
   "the following possible values:\n\n"
   "bs -- This row/column is basic.\n"
   "nl -- This row/column is non-basic.\n"
   "nu -- This row/column is non-basic and set to the upper bound.\n"
   "      On assignment, if this row/column is not double bounded,\n"
   "      this is equivalent to 'nl'.\n"
   "nf -- This row/column is non-basic and free.\n"
   "      On assignment this is equivalent to 'nl'.\n"
   "ns -- This row/column is non-basic and fixed.\n"
   "      On assignment this is equivalent to 'nl'.", NULL},

  {"value", (getter)Bar_getvarval, (setter)NULL,
   "The value of this variable by the last solver.", NULL},
  {"primal", (getter)Bar_getvarval, (setter)NULL,
   "The primal value of this variable by the last solver.", NULL},
  {"dual", (getter)Bar_getvarval, (setter)NULL,
   "The dual value of this variable by the last solver.", (void*)0x1},

  {"primal_s", (getter)Bar_getspecvarval, (setter)NULL,
   "The primal value of this variable by the simplex solver.",
   (void*)(rowcol_primdual_funcptrs)},
  {"dual_s", (getter)Bar_getspecvarval, (setter)NULL,
   "The dual value of this variable by the simplex solver.",
   (void*)(rowcol_primdual_funcptrs+2)},

  {"primal_i", (getter)Bar_getspecvarval, (setter)NULL,
   "The primal value of this variable by the interior-point solver.",
   (void*)(rowcol_primdual_funcptrs+4)},
  {"dual_i", (getter)Bar_getspecvarval, (setter)NULL,
   "The dual value of this variable by the interior-point solver.",
   (void*)(rowcol_primdual_funcptrs+6)},

  {"value_m", (getter)Bar_getspecvarvalm, (setter)NULL,
   "The value of this variable by the MIP solver.",
   (void*)(rowcol_primdual_funcptrs+8)},

  {"kind", (getter)Bar_getkind, (setter)Bar_setkind,
   "Either the type 'float' if this is a continuous variable, 'int'\n"
   "if this is an integer variable, or 'bool' if this is a binary\n"
   "variable.", NULL},

  {NULL}
};

static PyMethodDef Bar_methods[] = {
  /*{"add", (PyCFunction)Bar_add, METH_VARARGS, "add(n)\n\n"
   "Add n more rows (constraints) or columns (struct variables).\n"
   "Returns the index of the first added entry."},*/
  {NULL}
};

PyTypeObject BarType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.Bar",				/* tp_name */
  sizeof(BarObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)Bar_dealloc,		/* tp_dealloc*/
  0,					/* tp_print*/
  0,					/* tp_getattr*/
  0,					/* tp_setattr*/
  0,					/* tp_compare*/
  (reprfunc)Bar_Str,			/* tp_repr*/
  0,					/* tp_as_number*/
  0, //&Bar_as_sequence,		/* tp_as_sequence*/
  0, //&Bar_as_mapping,			/* tp_as_mapping*/
  0,					/* tp_hash */
  0,					/* tp_call*/
  (reprfunc)Bar_Str,			/* tp_str*/
  0,					/* tp_getattro*/
  0,					/* tp_setattro*/
  0,					/* tp_as_buffer*/
#ifdef USE_BAR_GC
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags*/
#else
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags*/
#endif
  "Bar objects are used to refer to a particular row or column of\n"
  "a linear program.  Rows and columns may be retrieved by\n"
  "indexing into the rows and cols sequences of LPX instances.",
  /* tp_doc */
  (traverseproc)Bar_traverse,		/* tp_traverse */
  (inquiry)Bar_clear,			/* tp_clear */
  (richcmpfunc)Bar_richcompare,		/* tp_richcompare */
  offsetof(BarObject, weakreflist),	/* tp_weaklistoffset */
  0,					/* tp_iter */
  0,					/* tp_iternext */
  Bar_methods,				/* tp_methods */
  Bar_members,				/* tp_members */
  Bar_getset,				/* tp_getset */
  //0,					/* tp_base */
  //0,					/* tp_dict */
  //0,					/* tp_descr_get */
  //0,					/* tp_descr_set */
  //0,					/* tp_dictoffset */
  //(initproc)Bar_init,			/* tp_init */
  //0,					/* tp_alloc */
  //Bar_new,				/* tp_new */
};
