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

#include "lp.h"
#include "structmember.h"
#include "barcol.h"
#include "obj.h"
#include "kkt.h"
#include "util.h"
#include "tree.h"

#ifdef USEPARAMS
#include "params.h"
#endif

#define LP (self->lp)

static int LPX_traverse(LPXObject *self, visitproc visit, void *arg) {
  Py_VISIT(self->rows);
  Py_VISIT(self->cols);
  Py_VISIT(self->obj);
#ifdef USEPARAMS
  Py_VISIT(self->params);
#endif
  return 0;
}

static int LPX_clear(LPXObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_CLEAR(self->rows);
  Py_CLEAR(self->cols);
  Py_CLEAR(self->obj);
#ifdef USEPARAMS
  Py_CLEAR(self->params);
#endif
  return 0;
}

static void LPX_dealloc(LPXObject *self) {
  LPX_clear(self);
  if (LP) glp_delete_prob(LP);
  self->ob_type->tp_free((PyObject*)self);
}

LPXObject* LPX_FromLP(glp_prob*lp) {
  LPXObject *lpx = (LPXObject*)PyObject_GC_New(LPXObject, &LPXType);
  if (lpx==NULL) return lpx;
  // Start out with null.
  lpx->cols = lpx->rows = NULL;
  lpx->obj = NULL;
  lpx->params = NULL;
  // Try assigning the values.
  if ((lpx->cols = (PyObject*)BarCol_New(lpx, 0))==NULL ||
      (lpx->rows = (PyObject*)BarCol_New(lpx, 1))==NULL ||
#ifdef USEPARAMS
      (lpx->params = (PyObject*)Params_New(lpx))==NULL ||
#endif
      (lpx->obj = (PyObject*)Obj_New(lpx))==NULL) {
    Py_DECREF(lpx);
    return NULL;
  }
  // Finally assign the LP and return the structure.
  lpx->lp = lp;
  return lpx;
}

static PyObject * LPX_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
  LPXObject *self;
  self = (LPXObject*) type->tp_alloc(type, 0);
  if (self != NULL) {
    self->lp = NULL;

    self->rows = NULL;
    self->cols = NULL;
    self->obj = NULL;
#ifdef USEPARAMS
    self->params = NULL;
#endif
    self->weakreflist = NULL;

    self->last_solver = -1;
  }
  return (PyObject*)self;
}

static int LPX_init(LPXObject *self, PyObject *args, PyObject *kwds) {
  char *mps_n=NULL, *freemps_n=NULL, *cpxlp_n=NULL;
  PyObject *model_obj=NULL;
  static char *kwlist[] = {"gmp","mps","freemps","cpxlp",NULL};
  if (!PyArg_ParseTupleAndKeywords
      (args, kwds, "|Osss", kwlist,
       &model_obj, &mps_n, &freemps_n, &cpxlp_n)) {
    return -1;
  }
  int numargs = (mps_n?1:0)+(freemps_n?1:0)+(cpxlp_n?1:0)+(model_obj?1:0);
  if (numargs>1) {
    PyErr_SetString(PyExc_TypeError, "cannot specify multiple data sources");
    return -1;
  }
  if (numargs==0) {
    // No arguments.  Create an empty problem.
    self->lp = glp_create_prob();
  } else {
    // Some of these are pretty straightforward data reading routines.
    if (mps_n) {
#if GLPK_VERSION(4, 29)
      self->lp = glp_create_prob();
      int failure = glp_read_mps(self->lp, GLP_MPS_DECK, NULL, mps_n);
      if (failure) {
	PyErr_SetString(PyExc_RuntimeError, "MPS reader failed");
	return -1;
      }
#else
      self->lp = lpx_read_mps(mps_n);
#endif
    } else if (freemps_n) {
#if GLPK_VERSION(4, 29)
      self->lp = glp_create_prob();
      int failure = glp_read_mps(self->lp, GLP_MPS_FILE, NULL, mps_n);
      if (failure) {
	PyErr_SetString(PyExc_RuntimeError, "Free MPS reader failed");
	return -1;
      }
#else
      self->lp = lpx_read_freemps(freemps_n);
#endif
    } else if (cpxlp_n) {
#if GLPK_VERSION(4, 29)
      self->lp = glp_create_prob();
      int failure = glp_read_lp(self->lp, NULL, mps_n);
      if (failure) {
	PyErr_SetString(PyExc_RuntimeError, "CPLEX LP reader failed");
	return -1;
      }
#else
      self->lp = lpx_read_cpxlp(cpxlp_n);
#endif
    } else if (model_obj) {
      // This one can take a few possible values.
      char *model[] = {NULL,NULL,NULL};
      if (PyString_Check(model_obj)) {
	// Single string object.
	model[0] = PyString_AsString(model_obj);
	if (!model[0]) return -1;
      } else if (PyTuple_Check(model_obj)) {
	// Possibly module arguments.
	int i,size = PyTuple_Size(model_obj);
	if (size < -1) { return -1; }
	if (size >  3) { 
	  PyErr_SetString(PyExc_ValueError, "model tuple must have length<=3");
	  return -1; }
	for (i=0; i<size; ++i) {
	  PyObject *so = PyTuple_GET_ITEM(model_obj,i);
	  if (so==Py_None) continue;
	  model[i] = PyString_AsString(so);
	  if (model[i]==NULL) { return -1; }
	}
      } else {
	PyErr_SetString(PyExc_TypeError, "model arg must be string or tuple");
	return -1;
      }
      // Now, pass in that information.
      if (!model[0]) return -1;
      self->lp = lpx_read_model(model[0], model[1], model[2]);
    }
  }
  // Any of the methods above may have failed, so the LP would be null.
  if (LP == NULL) {
    PyErr_SetString(numargs?PyExc_RuntimeError:PyExc_MemoryError,
		    "could not create problem");
    return -1;
  }
  // Create those rows and cols and things.
  self->cols = (PyObject*)BarCol_New(self, 0);
  self->rows = (PyObject*)BarCol_New(self, 1);
  self->obj = (PyObject*)Obj_New(self);
#ifdef USEPARAMS
  self->params = (PyObject*)Params_New(self);
#endif
  return 0;
}

static PyObject* LPX_Str(LPXObject *self) {
  // Returns a string representation of this object.
  return PyString_FromFormat
    ("<%s %d-by-%d at %p>", self->ob_type->tp_name,
     glp_get_num_rows(LP), glp_get_num_cols(LP), self);
}

PyObject *LPX_GetMatrix(LPXObject *self) {
  int row, numrows, listi, i, nnz, rownz;
  PyObject *retval;

  numrows = glp_get_num_rows(LP);
  nnz = glp_get_num_nz(LP);
  
  retval = PyList_New(nnz);
  if (nnz==0 || retval==NULL) return retval;

  // We don't really need this much memory, but, eh... 
  int *ind = (int*)calloc(nnz,sizeof(int));
  double *val = (double*)calloc(nnz,sizeof(double));

  listi = 0;
  for (row=1; row<=numrows; ++row) {
    rownz = glp_get_mat_row(LP, row, ind-1, val-1);
    if (rownz==0) continue;
    for (i=0; i<rownz; ++i) {
      PyList_SET_ITEM(retval, listi++, Py_BuildValue
		      ("iid", row-1, ind[i]-1, val[i]));
    }
    // Continue to downscale these vectors, freeing memory in C even
    // as we use more memory in Python.
    nnz -= rownz;
    if (nnz) {
      ind = (int*)realloc(ind, nnz*sizeof(int));
      val = (double*)realloc(val, nnz*sizeof(double));
    }
  }
  free(ind);
  free(val);
  if (PyList_Sort(retval)) {
    Py_DECREF(retval);
    return NULL;
  }
  return retval;
}

int LPX_SetMatrix(LPXObject *self, PyObject *newvals) {
  int len, *ind1, *ind2;
  double*val;

  // Now, attempt to convert the input item.
  if (newvals == NULL || newvals == Py_None) {
    len = 0;
  } else if (!util_extract_iif(newvals, (PyObject*)self, &len,
			       &ind1, &ind2, &val)) {
    return -1;
  }

  // Input the stuff into the LP constraint matrix.
  glp_load_matrix(LP, len, ind1-1, ind2-1, val-1);
  // Free the memory.
  if (len) {
    free(ind1);
    free(ind2);
    free(val);
  }
  return 0;
}

/****************** METHODS ***************/

/*static PyObject* LPX_OrderMatrix(LPXObject *self) {
  glp_order_matrix(LP);
  Py_RETURN_NONE;
  }*/

static PyObject* LPX_Erase(LPXObject *self) {
#if GLPK_VERSION(4, 29)
  glp_erase_prob(LP);
#else
  // Approximate the functionality by deleting and reassigning the
  // underlying pointer.  The Python code shouldn't actually know the
  // difference.
  if (LP) glp_delete_prob(LP);
  self->lp = glp_create_prob();
#endif
  Py_RETURN_NONE;
}

static PyObject* LPX_Scale(LPXObject *self, PyObject*args) {
#if GLPK_VERSION(4, 31)
  int flags=GLP_SF_AUTO;
  PyArg_ParseTuple(args, "|i", &flags);
  glp_scale_prob(LP, flags);
#else
  lpx_scale_prob(LP);
#endif
  Py_RETURN_NONE;
}

static PyObject* LPX_Unscale(LPXObject *self) {
  glp_unscale_prob(LP);
  Py_RETURN_NONE;
}

static PyObject* LPX_basis_std(LPXObject *self) {
#if GLPK_VERSION(4, 31)
  glp_std_basis(LP);
#else
  lpx_std_basis(LP);
#endif
  Py_RETURN_NONE;
}

static PyObject* LPX_basis_adv(LPXObject *self) {
#if GLPK_VERSION(4, 31)
  glp_adv_basis(LP, 0);
#else
  lpx_adv_basis(LP);
#endif
  Py_RETURN_NONE;
}

static PyObject* LPX_basis_cpx(LPXObject *self) {
#if GLPK_VERSION(4, 31)
  glp_cpx_basis(LP);
#else
  lpx_cpx_basis(LP);
#endif
  Py_RETURN_NONE; 
}

static PyObject* LPX_basis_read(LPXObject *self, PyObject *args) {
  char *bas_filename = NULL;
  if (!PyArg_ParseTuple(args, "s", &bas_filename)) {
    return NULL;
  }
  if (lpx_read_bas(LP, bas_filename)) {
    PyErr_SetString(PyExc_RuntimeError, "could not read basis file");
    return NULL;
  }
  Py_RETURN_NONE;
}

/**************** SOLVER METHODS **************/

static PyObject* solver_retval_to_message(int retval) {
  switch (retval) {
  case LPX_E_OK:	Py_RETURN_NONE;
  case LPX_E_FAULT:	return PyString_FromString("fault");
  case LPX_E_OBJLL:	return PyString_FromString("objll");
  case LPX_E_OBJUL:	return PyString_FromString("objul");
  case LPX_E_ITLIM:	return PyString_FromString("itlim");
  case LPX_E_TMLIM:	return PyString_FromString("tmlim");
  case LPX_E_SING:	return PyString_FromString("sing");

  case LPX_E_NOPFS:	return PyString_FromString("nopfs");
  case LPX_E_NODFS:	return PyString_FromString("nodfs");

  case LPX_E_NOFEAS:	return PyString_FromString("nofeas");
  case LPX_E_NOCONV:	return PyString_FromString("noconv");
  case LPX_E_INSTAB:	return PyString_FromString("instab");

  default:		return PyString_FromString("unknown?");
  }
}

static PyObject* glpsolver_retval_to_message(int retval) {
  const char* returnval=NULL;
  switch (retval) {
  case 0: Py_RETURN_NONE;
  case GLP_EBADB: returnval="badb"; break;
  case GLP_ESING: returnval="sing"; break;
  case GLP_ECOND: returnval="cond"; break;
  case GLP_EBOUND: returnval="bound"; break;
  case GLP_EFAIL: returnval="fail"; break;
  case GLP_EOBJLL: returnval="objll"; break;
  case GLP_EOBJUL: returnval="objul"; break;
  case GLP_EITLIM: returnval="itlim"; break;
  case GLP_ETMLIM: returnval="tmlim"; break;
  case GLP_ENOPFS: returnval="nopfs"; break;
  case GLP_ENODFS: returnval="nodfs"; break;
#if GLPK_VERSION(4, 20)
  case GLP_EROOT: returnval="root"; break;
  case GLP_ESTOP: returnval="stop"; break;
#endif
  default: returnval="unknown?"; break;
  }
  return PyString_FromString(returnval);
}

static PyObject* LPX_solver_simplex(LPXObject *self, PyObject *args,
				    PyObject *keywds) {
#if GLPK_VERSION(4, 18)
  glp_smcp cp;
  // Set all to GLPK defaults, except for the message level, which
  // inexplicably has a default "verbose" setting.
  glp_init_smcp(&cp);
  cp.msg_lev = GLP_MSG_OFF;
  // Map the keyword arguments to the appropriate entries.
  static char *kwlist[] =
    {"msg_lev", "meth", "pricing", "r_test", "tol_bnd", "tol_dj", "tol_piv",
     "obj_ll", "obj_ul", "it_lim", "tm_lim", "out_frq", "out_dly", "presolve",
     NULL};
  if (!PyArg_ParseTupleAndKeywords
      (args, keywds, "|iiiidddddiiiii", kwlist, &cp.msg_lev, &cp.meth,
       &cp.pricing, &cp.r_test, &cp.tol_bnd, &cp.tol_dj, &cp.tol_piv,
       &cp.obj_ll, &cp.obj_ul, &cp.it_lim, &cp.tm_lim, &cp.out_frq,
       &cp.out_dly, &cp.presolve)) {
    return NULL;
  }
  cp.presolve = cp.presolve ? GLP_ON : GLP_OFF;
  // Do checking on the various entries.
  switch (cp.msg_lev) {
  case GLP_MSG_OFF: case GLP_MSG_ERR: case GLP_MSG_ON: case GLP_MSG_ALL: break;
  default:
    PyErr_SetString
      (PyExc_ValueError,
       "invalid value for msg_lev (LPX.MSG_* are valid values)");
    return NULL;
  }
  switch (cp.meth) {
  case GLP_PRIMAL: case GLP_DUALP: break;
#if GLPK_VERSION(4, 31)
  case GLP_DUAL: break;
#endif
  default:
    PyErr_SetString
      (PyExc_ValueError,
       "invalid value for meth (LPX.PRIMAL, LPX.DUAL, "
       "LPX.DUALP valid values)");
    return NULL;
  }
  switch (cp.pricing) {
  case GLP_PT_STD: case GLP_PT_PSE: break;
  default:
    PyErr_SetString
      (PyExc_ValueError, 
       "invalid value for pricing (LPX.PT_STD, LPX.PT_PSE valid values)");
    return NULL;
  }
  switch (cp.r_test) {
  case GLP_RT_STD: case GLP_RT_HAR: break;
  default:
    PyErr_SetString
      (PyExc_ValueError, 
       "invalid value for ratio test (LPX.RT_STD, LPX.RT_HAR valid values)");
    return NULL;
  }
  if (cp.tol_bnd<=0 || cp.tol_bnd>=1) {
    PyErr_SetString(PyExc_ValueError, "tol_bnd must obey 0<tol_bnd<1");
    return NULL;
  }
  if (cp.tol_dj<=0 || cp.tol_dj>=1) {
    PyErr_SetString(PyExc_ValueError, "tol_dj must obey 0<tol_dj<1");
    return NULL;
  }
  if (cp.tol_piv<=0 || cp.tol_piv>=1) {
    PyErr_SetString(PyExc_ValueError, "tol_piv must obey 0<tol_piv<1");
    return NULL;
  }
  if (cp.it_lim<0) {
    PyErr_SetString(PyExc_ValueError, "it_lim must be non-negative");
    return NULL;
  }
  if (cp.tm_lim<0) {
    PyErr_SetString(PyExc_ValueError, "tm_lim must be non-negative");
    return NULL;
  }
  if (cp.out_frq<=0) {
    PyErr_SetString(PyExc_ValueError, "out_frq must be positive");
    return NULL;
  }
  if (cp.out_dly<0) {
    PyErr_SetString(PyExc_ValueError, "out_dly must be non-negative");
    return NULL;
  }
  // All the checks are complete.  Call the simplex solver.
  int retval = glp_simplex(LP, &cp);
  if (retval!=GLP_EBADB && retval!=GLP_ESING && retval!=GLP_ECOND
      && retval!=GLP_EBOUND && retval!=GLP_EFAIL)
    self->last_solver = 0;
  return glpsolver_retval_to_message(retval);
#else
  int retval = lpx_simplex(LP);
  if (retval!=LPX_E_FAULT) self->last_solver = 0;
  return solver_retval_to_message(retval);
#endif
}

static PyObject* LPX_solver_exact(LPXObject *self) {
  int retval = lpx_exact(LP);
  if (retval!=LPX_E_FAULT) self->last_solver = 0;
  return solver_retval_to_message(retval);
}

static PyObject* LPX_solver_interior(LPXObject *self) {
  int retval = lpx_interior(LP);
  if (retval!=LPX_E_FAULT) self->last_solver = 1;
  return solver_retval_to_message(retval);
}

#if GLPK_VERSION(4, 20)

struct mip_callback_object {
  PyObject *callback;
  LPXObject *py_lp;
};

static void mip_callback(glp_tree *tree, void *info) {
  struct mip_callback_object *obj = (struct mip_callback_object *)info;
  PyObject *method_name = NULL;
  // Choose the method name for the callback object that is appropriate.
  switch (glp_ios_reason(tree)) {
#if GLPK_VERSION(4, 21)
  case GLP_ISELECT: method_name=PyString_FromString("select"); break;
  case GLP_IPREPRO: method_name=PyString_FromString("prepro"); break;
  case GLP_IBRANCH: method_name=PyString_FromString("branch"); break;
#endif
  case GLP_IROWGEN: method_name=PyString_FromString("rowgen"); break; 
  case GLP_IHEUR:   method_name=PyString_FromString("heur");   break;
  case GLP_ICUTGEN: method_name=PyString_FromString("cutgen"); break;
  case GLP_IBINGO:  method_name=PyString_FromString("bingo");  break;
  default:
    // This should never happen.
    PyErr_SetString(PyExc_RuntimeError, "unrecognized reason for callback");
    glp_ios_terminate(tree);
    return;
  }
  // If there is no method with that name.
  if (!PyObject_HasAttr(obj->callback, method_name)) {
    Py_DECREF(method_name);
    method_name=PyString_FromString("default");
    if (!PyObject_HasAttr(obj->callback, method_name)) {
      Py_DECREF(method_name);
      method_name = NULL;
      return;
    }
  }
  // Try calling the method.
  TreeObject *py_tree = Tree_New(tree, obj->py_lp);
  if (py_tree==NULL) {
    Py_DECREF(method_name);
    glp_ios_terminate(tree);
    return;
  }
  PyObject *retval = NULL;
  retval = PyObject_CallMethodObjArgs
    (obj->callback, method_name, py_tree, NULL);
  py_tree->tree = NULL; // Invalidate the Tree object.
  Py_DECREF(py_tree);
  Py_XDECREF(method_name);
  if (retval==NULL) {
    // This could have failed for any number of reasons.  Perhaps the
    // code within the method failed, perhaps the method does not
    // accept the tree argument, or perhaps the 'method' is not really
    // even a callable method at all.
    glp_ios_terminate(tree);
    return;
  }
  Py_DECREF(retval);
}

#endif // GLPK_VERSION(4, 20)

static PyObject* LPX_solver_integer(LPXObject *self, PyObject *args,
				    PyObject *keywds) {
  if (glp_get_status(LP) != GLP_OPT) {
    PyErr_SetString(PyExc_RuntimeError, "integer solver requires "
		    "existing optimal basic solution");
    return NULL;
  }
#if GLPK_VERSION(4, 20)
  PyObject *callback=NULL;
  struct mip_callback_object*info=NULL;
  glp_iocp cp;
  glp_init_iocp(&cp);
  cp.msg_lev = GLP_MSG_OFF;
  // Map the keyword arguments to the appropriate entries.
  static char *kwlist[] = 
    {"msg_lev", "br_tech", "bt_tech",
#if GLPK_VERSION(4, 21)
     "pp_tech",
#endif // GLPK_VERSION(4, 21)
#if GLPK_VERSION(4, 24)
     "gmi_cuts",
#endif // GLPK_VERSION(4, 24)
#if GLPK_VERSION(4, 23)
     "mir_cuts",
#endif // GLPK_VERSION(4, 23)
     "tol_int", "tol_obj", "tm_lim", "out_frq", "out_dly", 
     "callback", //"cb_info", "cb_size",
     NULL};
  if (!PyArg_ParseTupleAndKeywords
      (args, keywds, "|iii"
#if GLPK_VERSION(4, 21)
       "i"
#endif // GLPK_VERSION(4, 21)
#if GLPK_VERSION(4, 24)
       "i"
#endif // GLPK_VERSION(4, 24)
#if GLPK_VERSION(4, 23)
       "i"
#endif // GLPK_VERSION(4, 23)
       "ddiiiO", kwlist, &cp.msg_lev, &cp.br_tech, &cp.bt_tech,
#if GLPK_VERSION(4, 21)
       &cp.pp_tech,
#endif // GLPK_VERSION(4, 21)
#if GLPK_VERSION(4, 24)
       &cp.gmi_cuts,
#endif // GLPK_VERSION(4, 24)
#if GLPK_VERSION(4, 23)
       &cp.mir_cuts,
#endif // GLPK_VERSION(4, 23)
       &cp.tol_int, &cp.tol_obj, &cp.tm_lim, &cp.out_frq, &cp.out_dly,
       &callback)) {
    return NULL;
  }
#if GLPK_VERSION(4, 24)
  cp.gmi_cuts = cp.gmi_cuts ? GLP_ON : GLP_OFF;
#endif // GLPK_VERSION(4, 24)
#if GLPK_VERSION(4, 23)
  cp.mir_cuts = cp.mir_cuts ? GLP_ON : GLP_OFF;
#endif // GLPK_VERSION(4, 23)
  // Do checking on the various entries.
  switch (cp.msg_lev) {
  case GLP_MSG_OFF: case GLP_MSG_ERR: case GLP_MSG_ON: case GLP_MSG_ALL: break;
  default:
    PyErr_SetString
      (PyExc_ValueError,
       "invalid value for msg_lev (LPX.MSG_* are valid values)");
    return NULL;
  }
  switch (cp.br_tech) {
  case GLP_BR_FFV: case GLP_BR_LFV: case GLP_BR_MFV: case GLP_BR_DTH: break;
  default:
    PyErr_SetString
      (PyExc_ValueError,
       "invalid value for br_tech (LPX.BR_* are valid values)");
    return NULL;
  }
  switch (cp.bt_tech) {
  case GLP_BT_DFS: case GLP_BT_BFS: case GLP_BT_BLB: case GLP_BT_BPH: break;
  default:
    PyErr_SetString
      (PyExc_ValueError,
       "invalid value for bt_tech (LPX.BT_* are valid values)");
    return NULL;
  }
#if GLPK_VERSION(4, 21)
  switch (cp.pp_tech) {
  case GLP_PP_NONE: case GLP_PP_ROOT: case GLP_PP_ALL: break;
  default:
    PyErr_SetString
      (PyExc_ValueError,
       "invalid value for pp_tech (LPX.PP_* are valid values)");
    return NULL;
  }
#endif // GLPK_VERSION(4, 21)
  if (cp.tol_int<=0 || cp.tol_int>=1) {
    PyErr_SetString(PyExc_ValueError, "tol_int must obey 0<tol_int<1");
    return NULL;
  }
  if (cp.tol_obj<=0 || cp.tol_obj>=1) {
    PyErr_SetString(PyExc_ValueError, "tol_obj must obey 0<tol_obj<1");
    return NULL;
  }
  if (cp.tm_lim<0) {
    PyErr_SetString(PyExc_ValueError, "tm_lim must be non-negative");
    return NULL;
  }
  if (cp.out_frq<=0) {
    PyErr_SetString(PyExc_ValueError, "out_frq must be positive");
    return NULL;
  }
  if (cp.out_dly<0) {
    PyErr_SetString(PyExc_ValueError, "out_dly must be non-negative");
    return NULL;
  }
  int retval;
  if (callback != NULL && callback != Py_None) {
    info = (struct mip_callback_object*)
      malloc(sizeof(struct mip_callback_object));
    info->callback = callback;
    info->py_lp = self;
    cp.cb_info = info;
    cp.cb_func = mip_callback;
  }
  retval = glp_intopt(LP, &cp);
  if (info) free(info);
  if (PyErr_Occurred()) {
    // This should happen only if there was a problem within the
    // callback function, or if the callback was not appropriate.
    return NULL;
  }
  if (retval!=GLP_EBADB && retval!=GLP_ESING && retval!=GLP_ECOND
      && retval!=GLP_EBOUND && retval!=GLP_EFAIL)
    self->last_solver = 2;
  return glpsolver_retval_to_message(retval);
#else
  int retval = lpx_integer(LP);
  if (retval!=LPX_E_FAULT) self->last_solver = 2;
  return solver_retval_to_message(retval);
#endif // GLPK_VERSION(4, 20)
}

static PyObject* LPX_solver_intopt(LPXObject *self) {
  int retval = lpx_intopt(LP);
  if (retval!=LPX_E_FAULT) self->last_solver = 2;
  return solver_retval_to_message(retval);
}

static KKTObject* LPX_kkt(LPXObject *self, PyObject *args) {
  // Cannot get for undefined primal or dual.
  if (glp_get_prim_stat(LP)==GLP_UNDEF ||
      glp_get_dual_stat(LP)==GLP_UNDEF) {
    PyErr_SetString(PyExc_RuntimeError, "cannot get KKT when primal or dual "
		    "basic solution undefined");
    return NULL;
  }
  // Check the Python arguments.
  int scaling = 0;
  PyObject *arg = NULL;
  if (!PyArg_ParseTuple(args, "|O", &arg)) return NULL;
  scaling = ((arg==NULL) ? 0 : PyObject_IsTrue(arg));
  if (scaling == -1) return NULL;
  // OK, all done with those checks.  Now get the KKT condition.
  KKTObject *kkt = KKT_New();
  if (!kkt) return NULL;
  lpx_check_kkt(LP, scaling, &(kkt->kkt));
  return kkt;
}

static KKTObject* LPX_kktint(LPXObject *self) {
  KKTObject *kkt = KKT_New();
  if (!kkt) return NULL;
  lpx_check_int(LP, &(kkt->kkt));
  return kkt;
}

static PyObject* LPX_write(LPXObject *self, PyObject *args, PyObject *keywds) {
  static char* kwlist[] = {"mps", "bas", "freemps", "cpxlp", "prob",
			   "sol", "sens_bnds", "ips", "mip", NULL};
  static int(*writers[])(LPX*,const char*) = {
    lpx_write_mps, lpx_write_bas, 
    lpx_write_freemps, 
    lpx_write_cpxlp,
    lpx_print_prob, lpx_print_sol, lpx_print_sens_bnds, lpx_print_ips,
    lpx_print_mip};
  char* fnames[] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
  int i;
  if (!PyArg_ParseTupleAndKeywords
      (args, keywds, "|sssssssss", kwlist, fnames,fnames+1,fnames+2,fnames+3,
       fnames+4,fnames+5,fnames+6,fnames+7,fnames+8)) {
    return NULL;
  }
  for (i=0; i<10; ++i) {
    if (fnames[i]==NULL) continue;
    if (writers[i]==NULL) {
      PyErr_Format(PyExc_NotImplementedError,
		   "writer for '%s' format absent in this version of GLPK",
		   kwlist[i]);
      return NULL;
    }
    int retval = (writers[i])(LP, fnames[i]);
    if (retval==0) continue;
    PyErr_Format(PyExc_RuntimeError, "writer for '%s' failed to write to '%s'",
		 kwlist[i], fnames[i]);
    return NULL;
  }
  Py_RETURN_NONE;
}

/****************** GET-SET-ERS ***************/

static PyObject* LPX_getname(LPXObject *self, void *closure) {
  const char *name = glp_get_prob_name(LP);
  if (name==NULL) Py_RETURN_NONE;
  return PyString_FromString(name);
}
static int LPX_setname(LPXObject *self, PyObject *value, void *closure) {
  char *name;
  if (value==NULL || value==Py_None) {
    glp_set_prob_name(LP, NULL);
    return 0;
  }
  name = PyString_AsString(value);
  if (name==NULL) return -1;
  if (PyString_Size(value) > 255) {
    PyErr_SetString(PyExc_ValueError, "name may be at most 255 chars");
    return -1;
  }
  glp_set_prob_name(LP, name);
  return 0;
}

static PyObject* LPX_getobj(LPXObject *self, void *closure) {
  Py_INCREF(self->obj);
  return self->obj;
}

static PyObject* LPX_getnumnonzero(LPXObject *self, void *closure) {
  return PyInt_FromLong(glp_get_num_nz(LP));
}

static PyObject* LPX_getmatrix(LPXObject *self, void *closure) {
  return LPX_GetMatrix(self);
}
static int LPX_setmatrix(LPXObject *self, PyObject *value, void *closure) {
  return LPX_SetMatrix(self, value);
}

static PyObject* glpstatus2string(int status) {
  switch (status) {
  case GLP_OPT:    return PyString_FromString("opt");
  case GLP_FEAS:   return PyString_FromString("feas");
  case GLP_INFEAS: return PyString_FromString("infeas");
  case GLP_NOFEAS: return PyString_FromString("nofeas");
  case GLP_UNBND:  return PyString_FromString("unbnd");
  case GLP_UNDEF:  return PyString_FromString("undef");
  default:         return PyString_FromString("unknown?");
  }
}

static PyObject* LPX_getstatus(LPXObject *self, void *closure) {
  int status;
  switch (self->last_solver) {
  case -1:
  case 0: status=glp_get_status(LP); break;
  case 1: status=glp_ipt_status(LP); break;
  case 2: status=glp_mip_status(LP); break;
  default: 
    PyErr_SetString(PyExc_RuntimeError,
		    "bad internal state for last solver identifier");
    return NULL;
  }
  return glpstatus2string(status);
}
static PyObject* LPX_getspecstatus(LPXObject *self, int(*statfunc)(LPX*)) {
  return glpstatus2string(statfunc(LP));
}

static PyObject* LPX_getray(LPXObject *self, void *closure) {
  int ray = lpx_get_ray_info(LP), numrows;
  if (ray==0) Py_RETURN_NONE;
  numrows = glp_get_num_rows(LP);
  ray--;
  if (ray < numrows) return PySequence_GetItem(self->rows, ray);
  return PySequence_GetItem(self->cols, ray - numrows);
}

static PyObject* LPX_getkind(LPXObject *self, void *closure) {
  PyObject *retval=NULL;
  retval = (PyObject*)(glp_get_num_int(LP)?&PyInt_Type:&PyFloat_Type);
  Py_INCREF(retval);
  return retval;
}

static PyObject* LPX_getnumint(LPXObject *self, void *closure) {
  return PyInt_FromLong(glp_get_num_int(LP)); }

static PyObject* LPX_getnumbin(LPXObject *self, void *closure) {
  return PyInt_FromLong(glp_get_num_bin(LP)); }

/****************** OBJECT DEFINITION *********/

int LPX_InitType(PyObject *module) {
  int retval;
  if ((retval=util_add_type(module, &LPXType))!=0) return retval;

  // Add in constant flag terms used in this class.
  // The following macro helps set constants.
#define SETCONST(A) PyDict_SetIntString(LPXType.tp_dict, #A, GLP_ ## A)
#if GLPK_VERSION(4, 31)
  // These are used in the LPX.scale method.
  SETCONST(SF_GM);
  SETCONST(SF_EQ);
  SETCONST(SF_2N);
  SETCONST(SF_SKIP);
  SETCONST(SF_AUTO);
#endif
  // These are used in control parameters for solvers.
  SETCONST(MSG_OFF);
  SETCONST(MSG_ERR);
  SETCONST(MSG_ON);
  SETCONST(MSG_ALL);
  
  SETCONST(PRIMAL);
#if GLPK_VERSION(4, 31)
  SETCONST(DUAL);
#endif
  SETCONST(DUALP);
  
  SETCONST(PT_STD);
  SETCONST(PT_PSE);
  
  SETCONST(RT_STD);
  SETCONST(RT_HAR);

#if GLPK_VERSION(4, 20)
  SETCONST(BR_FFV);
  SETCONST(BR_LFV);
  SETCONST(BR_MFV);
  SETCONST(BR_DTH);

  SETCONST(BT_DFS);
  SETCONST(BT_BFS);
  SETCONST(BT_BLB);
  SETCONST(BT_BPH);
#endif
#if GLPK_VERSION(4, 21)
  SETCONST(PP_NONE);
  SETCONST(PP_ROOT);
  SETCONST(PP_ALL);
#endif
#undef SETCONST
  // Add in the calls to the other objects.
  if ((retval=Obj_InitType(module))!=0) return retval;
#ifdef USEPARAMS
  if ((retval=Params_InitType(module))!=0) return retval;
#endif
  if ((retval=BarCol_InitType(module))!=0) return retval;
  if ((retval=KKT_InitType(module))!=0) return retval;
#if GLPK_VERSION(4, 20)
  if ((retval=Tree_InitType(module))!=0) return retval;
#endif
  return 0;
}

static PyMemberDef LPX_members[] = {
  {"rows", T_OBJECT_EX, offsetof(LPXObject, rows), RO,
   "Row collection.  See the help on class BarCollection."},
  {"cols", T_OBJECT_EX, offsetof(LPXObject, cols), RO,
   "Column collection.  See the help on class BarCollection."},
#ifdef USEPARAMS
  {"params", T_OBJECT_EX, offsetof(LPXObject, params), RO,
   "Control parameter collection.  See the help on class Params."},
#endif
  {NULL}
};

static PyGetSetDef LPX_getset[] = {
  {"name", (getter)LPX_getname, (setter)LPX_setname,
   "Problem name, or None if unset.", NULL},
  {"obj", (getter)LPX_getobj, (setter)NULL, 
   "Objective function object.", NULL},
  {"nnz", (getter)LPX_getnumnonzero, (setter)NULL,
   "Number of non-zero constraint coefficients.", NULL},
  {"matrix", (getter)LPX_getmatrix, (setter)LPX_setmatrix,
   "The constraint matrix as a list of three element (row index,\n"
   "column index, value) tuples across all non-zero elements of\n"
   "the constraint matrix.", NULL},
  // Solution status retrieval.
  {"status", (getter)LPX_getstatus, (setter)NULL,
   "The status of solution of the last solver.  This takes the\n"
   "form of a string with these possible values.\n\n"
   "opt    -- The solution is optimal.\n"
   "undef  -- The solution is undefined.\n"
   "feas   -- The solution is feasible, but not necessarily optimal.\n"
   "infeas -- The solution is infeasible.\n"
   "nofeas -- The problem has no feasible solution.\n"
   "unbnd  -- The problem has an unbounded solution.", NULL},
  {"status_s", (getter)LPX_getspecstatus, (setter)NULL,
   "The status of the simplex solver's solution.", (void*)glp_get_status},
  {"status_i", (getter)LPX_getspecstatus, (setter)NULL,
   "The status of the interior point solver's solution.",
   (void*)glp_ipt_status},
  {"status_m", (getter)LPX_getspecstatus, (setter)NULL,
   "The status of the MIP solver's solution.", (void*)glp_mip_status},
  {"status_primal", (getter)LPX_getspecstatus, (setter)NULL,
   "The status of the primal solution of the simplex solver.\n"
   "Possible values are 'undef', 'feas', 'infeas', 'nofeas' in\n"
   "similar meaning to the .status attribute.",
   (void*)glp_get_prim_stat},
  {"status_dual", (getter)LPX_getspecstatus, (setter)NULL,
   "The status of the dual solution of the simplex solver.\n"
   "Possible values are 'undef', 'feas', 'infeas', 'nofeas' in\n"
   "similar meaning to the .status attribute.",
   (void*)glp_get_dual_stat},
  // Ray info.
  {"ray", (getter)LPX_getray, (setter)NULL,
   "A non-basic row or column the simplex solver has identified\n"
   "as causing primal unboundness, or None if no such variable\n"
   "has been identified.", NULL},
  // Setting for MIP.
  {"kind", (getter)LPX_getkind, NULL,//(setter)LPX_setkind,
   "Either the type 'float' if this is a pure linear programming\n"
   "(LP) problem, or the type 'int' if this is a mixed integer\n"
   "programming (MIP) problem.", NULL},
  {"nint", (getter)LPX_getnumint, (setter)NULL,
   "The number of integer column variables.  Always 0 if this is\n"
   "not a mixed integer problem.", NULL},
  {"nbin", (getter)LPX_getnumbin, (setter)NULL,
   "The number of binary column variables, i.e., integer with 0\n"
   "to 1 bounds.  Always 0 if this is not a mixed integer problem.", NULL},
  {NULL}
};

static PyMethodDef LPX_methods[] = {
  {"erase", (PyCFunction)LPX_Erase, METH_NOARGS,
   "erase()\n\n"
   "Erase the content of this problem, restoring it to the state\n"
   "it was in when it was first created."},
  {"scale", (PyCFunction)LPX_Scale, METH_VARARGS,
   "scale([flags=LPX.SF_AUTO])\n\n"
   "Perform automatic scaling of the problem data, in order to.\n"
   "improve conditioning.  The behavior is controlled by various\n"
   "flags, which can be bitwise ORed to combine effects.  Note\n"
   "that this only affects the internal state of the LP\n"
   "representation.  These flags are members of the LPX class:\n\n"
   "SF_GM   -- perform geometric mean scaling\n"
   "SF_EQ   -- perform equilibration scaling\n"
   "SF_2N   -- round scale factors to the nearest power of two\n"
   "SF_SKIP -- skip scaling, if the problem is well scaled\n"
   "SF_AUTO -- choose scaling options automatically"
  },
  {"unscale", (PyCFunction)LPX_Unscale, METH_NOARGS,
   "unscale()\n\n"
   "This unscales the problem data, essentially setting all\n"
   "scale factors to 1."
  },
  // Basis construction techniques for simplex solvers.
  {"std_basis", (PyCFunction)LPX_basis_std, METH_NOARGS,
   "std_basis()\n\n"
   "Construct the standard trivial inital basis for this LP."},
  {"adv_basis", (PyCFunction)LPX_basis_adv, METH_NOARGS,
   "adv_basis()\n\n"
   "Construct an advanced initial basis, triangular with as few\n"
   "variables as possible fixed."},
  {"cpx_basis", (PyCFunction)LPX_basis_cpx, METH_NOARGS,
   "cpx_basis()\n\n"
   "Construct an advanced Bixby basis.\n\n"
   "This basis construction method is described in:\n"
   "Robert E. Bixby. Implementing the Simplex Method: The Initial\n"
   "Basis.  ORSA Journal on Computing, Vol. 4, No. 3, 1992,\n"
   "pp. 267-84."},
  {"read_basis", (PyCFunction)LPX_basis_read, METH_VARARGS,
   "read_basis(filename)\n\n"
   "Reads an LP basis in the fixed MPS format from a given file."},
  // Solver routines.
  {"simplex", (PyCFunction)LPX_solver_simplex, METH_VARARGS|METH_KEYWORDS,
   "simplex([keyword arguments])\n\n"
   "Attempt to solve the problem using a simplex method.\n\n"
   "This procedure has a great number of optional keyword arguments\n"
   "to control the functioning of the solver.  We list these here,\n"
   "including descriptions of their legal values.\n\n"
   "msg_lev : Controls the message level of terminal output.\n"
   "  LPX.MSG_OFF -- no output (default)\n"
   "  LPX.MSG_ERR -- error and warning messages\n"
   "  LPX.MSG_ON  -- normal output\n"
   "  LPX.MSG_ALL -- full informational output\n"
   "meth    : Simplex method option\n"
   "  LPX.PRIMAL  -- use two phase primal simplex (default)\n"
#if GLPK_VERSION(4, 31)
   "  LPX.DUAL    -- use two phase dual simplex\n"
#endif
   "  LPX.DUALP   -- use two phase dual simplex, primal if that fails\n"
   "pricing : Pricing technique\n"
   "  LPX.PT_STD  -- standard textbook technique\n"
   "  LPX.PT_PSE  -- projected steepest edge (default)\n"
   "r_test  : Ratio test technique\n"
   "  LPX.RT_STD  -- standard textbook technique\n"
   "  LPX.RT_HAR  -- Harris' two-pass ratio test (default)\n"
   "tol_bnd : Tolerance used to check if the basic solution is primal\n"
   "  feasible. (default 1e-7)\n"
   "tol_dj  : Tolerance used to check if the basic solution is dual\n"
   "  feasible. (default 1e-7)\n"
   "tol_piv : Tolerance used to choose pivotal elements of the simplex\n"
   "  table. (default 1e-10)\n"
   "obj_ll  : Lower limit of the objective function.  The solver\n"
   "  terminates upon reaching this level.  This is used only in\n"
   "  dual simplex optimization. (default is min float)\n"
   "obj_ul  : Upper limit of the objective function.  The solver\n"
   "  terminates upon reaching this level.  This is used only in\n"
   "  dual simplex optimization. (default is max float)\n"
   "it_lim  : Simplex iteration limit. (default is max int)\n"
   "tm_lim  : Search time limit in milliseconds. (default is max int)\n"
   "out_frq : Terminal output frequency in iterations. (default 200)\n"
   "out_dly : Terminal output delay in milliseconds. (default 0)\n"
   "presolve: Use the LP presolver. (default False)\n\n"
   "This returns None if the problem was successfully solved.\n"
   "Alternately, on failure it will return one of the following\n"
   "strings to indicate failure type.\n\n"
   "fault   -- There are no rows or columns, or the initial basis\n"
   "           is invalid, or the initial basis matrix is singular\n"
   "           or ill-conditioned.\n"
   "objll   -- The objective reached its lower limit.\n"
   "objul   -- The objective reached its upper limit.\n"
   "itlim   -- Iteration limited exceeded.\n"
   "tmlim   -- Time limit exceeded.\n"
   "sing    -- The basis matrix became singular or ill-conditioned.\n"
   "nopfs   -- No primal feasible solution. (Presolver only.)\n"
   "nodfs   -- No dual feasible solution. (Presolver only.)\n" },

  {"exact", (PyCFunction)LPX_solver_exact, METH_NOARGS,
   "exact()\n\n"
   "Attempt to solve the problem using an exact simplex method.\n\n"
   "This returns None if the problem was successfully solved.\n"
   "Alternately, on failure it will return one of the following\n"
   "strings to indicate failure type.\n\n"
   "fault   -- There are no rows or columns, or the initial basis\n"
   "           is invalid, or the initial basis matrix is singular\n"
   "           or ill-conditioned.\n"
   "itlim   -- Iteration limited exceeded.\n"
   "tmlim   -- Time limit exceeded." },

  {"interior", (PyCFunction)LPX_solver_interior, METH_NOARGS,
   "interior()\n\n"
   "Attempt to solve the problem using an interior-point method.\n\n"
   "This returns None if the problem was successfully solved.\n"
   "Alternately, on failure it will return one of the following\n"
   "strings to indicate failure type.\n\n"
   "fault   -- There are no rows or columns.\n"
   "nofeas  -- The problem has no feasible (primal/dual) solution.\n"
   "noconv  -- Very slow convergence or divergence.\n"
   "itlim   -- Iteration limited exceeded.\n"
   "instab  -- Numerical instability when solving Newtonian system." },

  {"integer", (PyCFunction)LPX_solver_integer, METH_VARARGS|METH_KEYWORDS,
   "integer()\n\n"
   "MIP solver based on branch-and-bound.\n\n"
#if GLPK_VERSION(4, 20)
   "This procedure has a great number of optional keyword arguments\n"
   "to control the functioning of the solver.  We list these here,\n"
   "including descriptions of their legal values.\n\n"
   "msg_lev : Controls the message level of terminal output.\n"
   "  LPX.MSG_OFF -- no output (default)\n"
   "  LPX.MSG_ERR -- error and warning messages\n"
   "  LPX.MSG_ON  -- normal output\n"
   "  LPX.MSG_ALL -- full informational output\n"
   "br_tech : Branching technique option.\n"
   "  LPX.BR_FFV  -- first fractional variable\n"
   "  LPX.BR_LFV  -- last fractional variable\n"
   "  LPX.BR_MFV  -- most fractional variable\n"
   "  LPX.BR_DTH  -- heuristic by Driebeck and Tomlin (default)\n"
   "bt_tech : Backtracking technique option.\n"
   "  LPX.BT_DFS  -- depth first search\n"
   "  LPX.BT_BFS  -- breadth first search\n"
   "  LPX.BT_BLB  -- best local bound (default)\n"
   "  LPX.BT_BPH  -- best projection heuristic\n"
#if GLPK_VERSION(4, 21)
   "pp_tech : Preprocessing technique option.\n"
   "  LPX.PP_NONE -- disable preprocessing\n"
   "  LPX.PP_ROOT -- perform preprocessing only on the root level\n"
   "  LPX.PP_ALL  -- perform preprocessing on all levels (default)\n"
#endif
#if GLPK_VERSION(4, 24)
   "gmi_cuts: Use Gomory's mixed integer cuts (default False)\n"
#endif
#if GLPK_VERSION(4, 23)
   "mir_cuts: Use mixed integer rounding cuts (default False)\n"
#endif
   "tol_int : Tolerance used to check if the optimal solution to the\n"
   "  current LP relaxation is integer feasible.\n"
   "tol_obj : Tolerance used to check if the objective value in the\n"
   "  optimal solution to the current LP is not better than the best\n"
   "  known integer feasible solution.\n"
   "tm_lim  : Search time limit in milliseconds. (default is max int)\n"
   "out_frq : Terminal output frequency in milliseconds. (default 5000)\n"
   "out_dly : Terminal output delay in milliseconds. (default 10000)\n"
   "callback: A callback object the user may use to monitor and control\n"
   "  the solver.  During certain portions of the optimization, the\n"
   "  solver will call methods of callback object. (default None)\n\n"

   "The last parameter, callback, is worth its own discussion.  During\n"
   "the branch-and-cut algorithm of the MIP solver, at various points\n"
   "callback hooks are invoked which allow the user code to influence\n"
   "the proceeding of the MIP solver.  The user code may influence the\n"
   "solver in the hook by modifying and operating on a Tree instance\n"
   "passed to the hook.  These hooks have various codes, which we list\n"
   "here.\n"
   "    select - request for subproblem selection\n"
   "    prepro - request for preprocessing\n"
   "    rowgen - request for row generation\n"
   "    heur   - request for heuristic solution\n"
   "    cutgen - request for cut generation\n"
   "    branch - request for branching\n"
   "    bingo  - better integer solution found\n"
   "During the invocation of a hook with a particular code, the\n"
   "callback object will have a method of the same name as the hook\n"
   "code called, with the Tree instance.  For instance, for the\n"
   "'cutgen' hook, it is equivalent to\n"
   "    callback.cutgen(tree)\n"
   "being called with tree as the Tree instance.  If the method does\n"
   "not exist, then instead the method 'default' is called with the\n"
   "same signature.  If neither the named hook method nor the default\n"
   "method exist, then the hook is ignored.\n\n"
#else
   "This procedure has a great number of optional keyword arguments\n"
   "if built against GLPK 4.20 or later.  Your PyGLPK was built against\n"
   "an earlier version.\n\n"
#endif
   "This method requires a mixed-integer problem where an optimal\n"
   "solution to an LP relaxation (either through simplex() or\n"
   "exact()) has already been found.  Alternately, try intopt().\n\n"
   "This returns None if the problem was successfully solved.\n"
   "Alternately, on failure it will return one of the following\n"
   "strings to indicate failure type.\n\n"
   "fault   -- There are no rows or columns, or it is not a MIP\n"
   "           problem, or integer variables have non-int bounds.\n"
   "nopfs   -- No primal feasible solution.\n"
   "nodfs   -- Relaxation has no dual feasible solution.\n"
   "itlim   -- Iteration limited exceeded.\n"
   "tmlim   -- Time limit exceeded.\n"
   "sing    -- Error occurred solving an LP relaxation subproblem." },

  {"intopt", (PyCFunction)LPX_solver_intopt, METH_NOARGS,
   "intopt()\n\n"
   "More advanced MIP branch-and-bound solver than integer(). This\n"
   "variant does not require an existing LP relaxation.\n\n"
   "This returns None if the problem was successfully solved.\n"
   "Alternately, on failure it will return one of the following\n"
   "strings to indicate failure type.\n\n"
   "fault   -- There are no rows or columns, or it is not a MIP\n"
   "           problem, or integer variables have non-int bounds.\n"
   "nopfs   -- No primal feasible solution.\n"
   "nodfs   -- Relaxation has no dual feasible solution.\n"
   "itlim   -- Iteration limited exceeded.\n"
   "tmlim   -- Time limit exceeded.\n"
   "sing    -- Error occurred solving an LP relaxation subproblem." },

  {"kkt", (PyCFunction)LPX_kkt, METH_VARARGS,
   "kkt([scaled=False])\n\n"
   "Return an object encapsulating the results of a check on the\n"
   "Karush-Kuhn-Tucker optimality conditions for a basic (simplex)\n"
   "solution.  If the argument 'scaled' is true, return results \n"
   "of checking the internal scaled instance of the LP instead."},
  {"kktint", (PyCFunction)LPX_kktint, METH_NOARGS,
   "kktint()\n\n"
   "Similar to kkt(), except analyzes solution quality of an\n"
   "mixed-integer solution.  Note that only the primal components\n"
   "of the KKT object will have meaningful values."},

  // Data writing
  {"write", (PyCFunction)LPX_write, METH_VARARGS | METH_KEYWORDS,
   "write(format=filename)\n\n"
   "Output data about the linear program into a file with a given\n"
   "format.  What data is written, and how it is written, depends\n"
   "on which of the format keywords are used.  Note that one may\n"
   "specify multiple format and filename pairs to write multiple\n"
   "types and formats of data in one call to this function.\n\n"
   "mps       -- For problem data in the fixed MPS format.\n"
   "bas       -- The current LP basis in fixed MPS format.\n"
   "freemps   -- Problem data in the free MPS format.\n"
   "cpxlp     -- Problem data in the CPLEX LP format.\n"
   "glp       -- Problem data in the GNU LP format.\n"
   "prob      -- Problem data in a plain text format.\n"
   "sol       -- Basic solution in printable format.\n"
   "sens_bnds -- Bounds sensitivity information.\n"
   "ips       -- Interior-point solution in printable format.\n"
   "mip       -- MIP solution in printable format."},
  
  {NULL}
};

PyTypeObject LPXType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.LPX",				/* tp_name */
  sizeof(LPXObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)LPX_dealloc,		/* tp_dealloc*/
  0,					/* tp_print*/
  0,					/* tp_getattr*/
  0,					/* tp_setattr*/
  0,					/* tp_compare*/
  (reprfunc)LPX_Str,			/* tp_repr*/
  0,					/* tp_as_number*/
  0,					/* tp_as_sequence*/
  0,					/* tp_as_mapping*/
  0,					/* tp_hash */
  0,					/* tp_call*/
  (reprfunc)LPX_Str,			/* tp_str*/
  0,					/* tp_getattro*/
  0,					/* tp_setattro*/
  0,					/* tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags*/
  "LPX()                 --> Empty linear program.\n"
  "LPX(gmp=filename)     --> Linear program with data read from a\n"
  "    GNU MathProg file containing model and data.\n"
  "LPX(mps=filename)     --> Linear program with data read from a\n"
  "    datafile in fixed MPS format.\n"
  "LPX(freemps=filename) --> Linear program with data read from a\n"
  "    datafile in free MPS format.\n"
  "LPX(cpxlp=filename)   --> Linear program with data read from a\n"
  "    datafile in fixed CPLEX LP format.\n"
  "LPX(glp=filename)     --> Linear program with data read from a\n"
  "    datafile in GNU LP format.\n"
  "LPX(gmp=(model_filename,[data_filename,[output_filename]])-->\n"
  "    Linear program from GNU MathProg input files.  The first\n"
  "    element is a path to the model second, the second to the\n"
  "    data section.  If the second element is omitted or is None\n"
  "    then the model file is presumed to also hold the data.\n"
  "    The third elment holds the output data file to write\n"
  "    display statements to.  If omitted or None, the output\n"
  "    is instead put through to standard output.\n"
  "\n"
  "This represents a linear program object.  It holds data and\n"
  "offers methods relevant to the whole of the linear program.\n"
  "There are many members in this class, but the most important\n"
  "are:\n"
  "  obj     Represents the objective function.\n"
  "  rows    A collection over which one can access rows.\n"
  "  cols    Same, but for columns.\n"
#ifdef USEPARAMS
  "  params  Holds control parameters and statistics."
#endif
  , /* tp_doc */
  (traverseproc)LPX_traverse,		/* tp_traverse */
  (inquiry)LPX_clear,			/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(LPXObject, weakreflist),	/* tp_weaklistoffset */
  0,					/* tp_iter */
  0,					/* tp_iternext */
  LPX_methods,				/* tp_methods */
  LPX_members,				/* tp_members */
  LPX_getset,				/* tp_getset */
  0,					/* tp_base */
  NULL,					/* tp_dict */
  0,					/* tp_descr_get */
  0,					/* tp_descr_set */
  0,					/* tp_dictoffset */
  (initproc)LPX_init,			/* tp_init */
  0,					/* tp_alloc */
  LPX_new,				/* tp_new */
};
