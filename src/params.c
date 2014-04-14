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

#include "params.h"
#include "util.h"
#include "structmember.h"
#include <float.h>

#define LP (self->py_lp->lp)

static int Params_traverse(ParamsObject *self, visitproc visit, void *arg) {
  Py_VISIT((PyObject*)self->py_lp);
  return 0;
}

static int Params_clear(ParamsObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_CLEAR(self->py_lp);
  return 0;
}

static void Params_dealloc(ParamsObject *self) {
  Params_clear(self);
  self->ob_type->tp_free((PyObject*)self);
}

/** Create a new parameter collection object. */
ParamsObject *Params_New(LPXObject *py_lp) {
  ParamsObject *p = (ParamsObject*)PyObject_GC_New(ParamsObject, &ParamsType);
  if (p==NULL) return p;

  Py_INCREF(py_lp);
  p->py_lp = py_lp;
  p->weakreflist = NULL;

  PyObject_GC_Track(p);
  return p;
}

/* Reset all parameters to their defaults. */
void Params_Reset(ParamsObject *self) { lpx_reset_parms(LP); }
PyObject *Params_reset(ParamsObject *self) {
  Params_Reset(self); Py_RETURN_NONE; }

/****************** GET-SET-ERS ***************/

/******** SUPER SPECIAL PARAMETER GET-SET-ERS ***************/

struct param_getsets {
  char*attr_name;  // The name as it should be published, e.g., "msglev".
  char*short_name; // A short descriptive name, e.g., "message level".
  int code;        // The GLPK C API parameter code, e.g., LPX_M_MSGLEV.
  unsigned char type:2, // 0 for bool, 1 for int, 2 for double.
    include_low:1,include_high:1,//Whether low, high admissble. Only for float.
    readonly:1;    // Whether to define only a getter.
  double low,high; // Lowest and highest admissible value.  
  char*doc;        // A documentation string.
};

static PyObject* Params_parameter_get(ParamsObject *self,
				      struct param_getsets *pgs) {
  switch (pgs->type) {
  case 0: // Boolean.
    return PyBool_FromLong(lpx_get_int_parm(LP, pgs->code));
  case 1: // Integer.
    return PyInt_FromLong(lpx_get_int_parm(LP, pgs->code));
  case 2: // Float.
    return PyFloat_FromDouble(lpx_get_real_parm(LP, pgs->code));
  default: // Um, apparently I made a mistake in the PGS definition array.
    PyErr_Format(PyExc_RuntimeError, "parameter type code %d unrecognized",
		 pgs->type);
    return NULL;
  }
}

static int Params_parameter_set(ParamsObject *self, PyObject *value,
				struct param_getsets *pgs) {
  if (value == NULL) {
    // Maybe in future this should restore to default, but I can't be arsed.
    PyErr_Format(PyExc_TypeError,"cannot delete attribute '%s'",
		 pgs->attr_name);
    return -1;
  }

  switch (pgs->type) {
  case 0: { // Boolean.
    int truth = PyObject_IsTrue(value);
    if (truth < 0) return -1;
    lpx_set_int_parm(LP, pgs->code, truth);
    return 0; }
  case 1: { // Integer.
    if (!PyInt_Check(value)) {
      PyErr_Format(PyExc_TypeError, "attribute '%s' requires an int",
		   pgs->attr_name);
      return -1;
    }
    int val = PyInt_AS_LONG(value);
    // Do all that range checking.
    if (pgs->low < pgs->high) {
      if (pgs->low > val) {
	PyErr_Format(PyExc_ValueError,"attribute '%s' must be at least %d",
		     pgs->attr_name, (int)pgs->low);
	return -1;
      }
      if (pgs->high < val) {
	PyErr_Format(PyExc_ValueError,"attribute '%s' must be at most %d",
		     pgs->attr_name, (int)pgs->high);
	return -1;
      }
    }
    // It's in range.  Set it.
    lpx_set_int_parm(LP, pgs->code, val);
    return 0; }
  case 2: { // Float.
    value = PyNumber_Float(value);
    if (value == NULL) {
      PyErr_Format(PyExc_TypeError, "attribute %s requires a float",
		   pgs->attr_name);
      return -1;
    }
    double val = PyFloat_AS_DOUBLE(value);
    Py_DECREF(value);
    // Do the range checking.
    if (pgs->low < pgs->high) {
      if ((!pgs->include_low && pgs->low==val) || pgs->low>val) {
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "attribute '%s' must be %s %g",
		 pgs->attr_name, pgs->include_low?"at least":"greater than",
		 pgs->low);
	PyErr_SetString(PyExc_ValueError, buffer);
	return -1;
      }
      if ((!pgs->include_high && pgs->high==val) || pgs->high<val) {
	char buffer[100];
	snprintf(buffer, sizeof(buffer), "attribute '%s' must be %s %g",
		 pgs->attr_name, pgs->include_high?"at most":"less than",
		 pgs->high);
	PyErr_SetString(PyExc_ValueError, buffer);
	return -1;
      }
    }
    // It's in range.  Set it.
    lpx_set_real_parm(LP, pgs->code, val);
    return 0; }
  default: // Um, apparently I made a mistake in the PGS definition array.
    PyErr_Format(PyExc_RuntimeError, "parameter type code %d unrecognized",
		 pgs->type);
    return -1;
  }
}

static struct param_getsets Params_pgs[] = {
  {"msglev", "message level", LPX_K_MSGLEV, 1,1,1,0, 0.0, 3.0,
   "Output level for solver routines:\n"
   "0 -- none\n1 -- error messages\n2 -- normal\n"
   "3 -- full information"},
  {"scale", "scale", LPX_K_SCALE, 1,1,1,0, 0.0, 3.0,
   "Scaling used when the lp.scale() method is called.\n"
   "0 -- no scaling\n1 -- equilibration scaling\n"
   "2 -- geometric mean scaling\n3 -- geometric then equilibration scaling"},
  {"dual", "dual simplex", LPX_K_DUAL, 0,1,1,0, 0.0, 1.0,
   "Whether to, if the initial basic solution is dual feasible, use\n"
   "the dual simplex."},
  {"price", "pricing", LPX_K_PRICE, 1,1,1,0, 0.0, 1.0,
   "Pricing option for primal and dual simplex.\n"
   "0 -- textbook pricing\n1 -- steepest edge pricing"},
  {"relax", "ratio test relaxation", LPX_K_RELAX, 2,1,1,0, 0.0, 1.0,
   "Relaxation parameter used in the ratio test.  If it is zero, the\n"
   "textbook ratio test is used.  If it is non-zero (should be\n"
   "positive), Harris' two-pass ratio test is used.  In the latter\n"
   "case on the first pass of the ratio test basic variables (in the\n"
   "case of primal simplex) or reduced costs of non-basic variables\n"
   "(in the case of dual simplex) are allowed to slightly violate\n"
   "their bounds, but not more than (relax*tolbnd) or (relax*toldj)\n"
   "(thus, relax specifies the mixture of tolbnd or toldj)."},
  {"tolbnd", "tolerance", LPX_K_TOLBND, 2,1,1,0, DBL_EPSILON, 0.001,
   "Relative tolerance used to check the basic solution's primal\n"
   "feasibility.  (Do not change this parameter without detailed\n"
   "understanding of its purpose.)"},
  {"toldj", "tolerance", LPX_K_TOLDJ, 2,1,1,0, DBL_EPSILON, 0.001,
   "Absolute tolerance used to check the basic solution's dual\n"
   "feasibility.  (Do not change this parameter without detailed\n"
   "understanding of its purpose.)"},
  {"tolpiv", "pivot tolerance", LPX_K_TOLPIV, 2,1,1,0, DBL_EPSILON, 0.001,
   "Relative tolerance used to choose eligible pivotal elements of\n"
   "the simplex table.  (Do not change this parameter without\n"
   "detailed understanding of its purpose.)"},
  {"round", "zero rounding", LPX_K_ROUND, 0,1,1,0, 0.0, 1.0,
   "Whether to round tiny primal and dual values to exact zero."},
  {"objll", "obj lower limit", LPX_K_OBJLL, 2,1,1,0, -DBL_MAX, DBL_MAX,
   "Lower limit of the objective function.  If on the phase II\n"
   "the objective function reaches this limit and continues\n"
   "decreasing, the solver stops the search.  (Used in the dual\n"
   "simplex only.)"},
  {"objul", "obj upper limit", LPX_K_OBJUL, 2,1,1,0, -DBL_MAX, DBL_MAX,
   "Lower limit of the objective function.  If on the phase II\n"
   "the objective function reaches this limit and continues\n"
   "increasing, the solver stops the search.  (Used in the dual\n"
   "simplex only.)"},
  {"itlim", "simplex iteration limit", LPX_K_ITLIM, 1,1,1,0, 1.0, -1.0,
   "Simplex iterations limit.  If this value is positive, it is\n"
   "decreased by one each time when one simplex iteration has been\n"
   "performed, and reaching zero value signals the solver to stop\n"
   "the search.  Negative value means no iterations limit."},
  {"itcnt", "simplex iteration count", LPX_K_ITCNT, 1,1,1,1, 1.0, -1.0,
   "Simplex iterations count.  This count is increased by one each\n"
   "time when one simplex iteration has been performed."},
  {"tmlim", "time limit", LPX_K_TMLIM, 2,1,1,0, 1.0, -1.0,
   "Searching time limit, in seconds.  If this value is positive,\n"
   "it is decreased each time when one simplex iteration has been\n"
   "performed by the amount of time spent for the iteration, and\n"
   "reaching zero value signals the solver to stop the search.\n"
   "Negative value means no time limit."},
  {"outfrq", "output frequency", LPX_K_OUTFRQ, 1,1,1,0, 1.0, 1e40,
   "Output frequency, in iterations.  This parameter specifies how\n"
   "frequently the solver sends information about the solution to\n"
   "the standard output.  This must be positive."},
  {"outdly", "output delay", LPX_K_OUTDLY, 2,1,1,0, 1.0, -1.0,
   "Output delay, in seconds.  This parameter specifies how long\n"
   "the solver should delay sending information about the solution\n"
   "to the standard output.  Non-positive value means no delay."},

  {"branch", "branching heuristic", LPX_K_BRANCH, 1,1,1,0, 0.0, 2.0,
   "Branching heuristic option (for MIP only):\n"
   "0 -- branch on first variable\n1 -- branch on last variable\n"
   "2 -- branch using a heuristic by Driebeck and Tomlin"},
  {"btrack", "backtracking heuristic", LPX_K_BTRACK, 1,1,1,0, 0.0, 3.0,
   "Backtracking heuristic option (for MIP only):\n"
   "0 -- depth first search\n1 -- breadth first search\n"
   "2 -- best local bound\n"
   "3 -- backtrack using the best projection heuristic"},
  {"tolint", "tolerance", LPX_K_TOLINT, 2,1,1,0, DBL_EPSILON, 0.001,
   "Absolute tolerance used to check if the current basic solution\n"
   "is integer feasible.  (Do not change this parameter without\n"
   "detailed understanding of its purpose.)"},
  {"tolobj", "tolerance", LPX_K_TOLOBJ, 2,1,1,0, DBL_EPSILON, 0.001,
   "Relative tolerance used to check if the value of the objective\n"
   "function is not better than in the best known integer feasible\n"
   "solution.  (Do not change this parameter without detailed\n"
   "understanding of its purpose.)"},

  {"mpsinfo", "MPS info", LPX_K_MPSINFO, 0,1,1,0, 0.0, 1.0,
   "Whether when writing MPS data, comments containing\n"
   "information on the problem should be written as well."},
  {"mpsobj", "MPS objective writing", LPX_K_MPSOBJ, 1,1,1,0, 0.0, 2.0,
   "How the objective function should be output in MPS writing:\n"
   "0 -- never output objective function row\n"
   "1 -- always output objective function row\n"
   "2 -- output objective function row if the problem has no free rows"},
  {"mpsorig", "MPS naming", LPX_K_MPSORIG, 0,1,1,0, 0.0, 1.0,
   "Whether when writing MPS data, the original symbolic names\n"
   "for rows and columns should be used.  If False, when writing,\n"
   "plain names are generated from the index of the row and column\n"
   "(counting from 1)."},

  {"mpswide", "MPS wide", LPX_K_MPSWIDE, 0,1,1,0, 0.0, 1.0,
   "Whether when writing MIPS, all data fields should be used.\n"
   "If False, fields 5 and 6 are kept empty."},
  {"mpsfree", "MPS free", LPX_K_MPSFREE, 0,1,1,0, 0.0, 1.0,
   "Whether the MIPS writer should omit column and vector names\n"
   "whenever possible (free style).   If False, the writer never\n"
   "omits names (pedantic style)."},
  {"mpsskip", "MPS skip", LPX_K_MPSSKIP, 0,1,1,0, 0.0, 1.0,
   "Whether the MIPS writer should skip empty columns."},

  {"presol", "simplex presolver", LPX_K_PRESOL, 0,1,1,0, 0.0, 1.0,
   "Whether the simplex solver should use the LP presolver."},

  {"usecuts", "cutting planes", LPX_K_USECUTS, 1,1,1,0, 0.0, 255,
   "A bitstring representing what cuts should be used in the\n"
   "intopt() solver.  For a current value 'val', the truth of\n"
   "these indicates which cuts will be used (multiple cuts\n"
   "may be used):\n"
   "0x01 & val -- mixed cover cuts\n"
   "0x02 & val -- clique cuts\n"
   "0x04 & val -- Gomory's mixed integer cuts\n"
   "For example, 0x07 or 0xFF or the like would use all cuts,\n"
   "and 0 would mean no cuts will be used."},

  {NULL}
};

int Params_InitType(PyObject *module) {
  int retval, i, existing_count=0, new_count=0;
  // Count the existing get-seters, and how many new ones we need.
  PyGetSetDef *gsdef = ParamsType.tp_getset;
  if (gsdef) {
    for (existing_count = 0; gsdef[existing_count].name; existing_count++);
  }
  for (new_count = 0; Params_pgs[new_count].attr_name; new_count++);
  // We need as many get setters as these combined, plus a null terminator.
  gsdef = (PyGetSetDef*)calloc(new_count+existing_count+1,sizeof(PyGetSetDef));
  gsdef[new_count+existing_count] = ParamsType.tp_getset[existing_count];
  // Copy over all existing non-trivial entries...
  for (i=0; i<existing_count; ++i)
    gsdef[i] = ParamsType.tp_getset[i];
  // Define the new entries.
  for (i=0; i<new_count; ++i) {
    struct param_getsets *pgs = Params_pgs+i;
    PyGetSetDef gs = {pgs->attr_name, (getter)Params_parameter_get,
		      (setter) (pgs->readonly ? NULL : Params_parameter_set),
		      pgs->doc, (void*)pgs };
    gsdef[existing_count+i] = gs;
  }
  // Finally, set it up...
  ParamsType.tp_getset = gsdef;
  retval=util_add_type(module, &ParamsType);
  //free(gsdef); // Why the fuck would Python want to keep *this* around?!!
  return retval;
}

/****************** OBJECT DEFINITION *********/

static PyMemberDef Params_members[] = {
  {NULL}
};

static PyGetSetDef Params_getset[] = {
  {NULL}
};

static PyMethodDef Params_methods[] = {
  {"reset", (PyCFunction)Params_reset, METH_NOARGS,
   "reset()\n\n"
   "Resets all control parameters to their default values."},
  {NULL}
};

PyTypeObject ParamsType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.Params",			/* tp_name */
  sizeof(ParamsObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)Params_dealloc,		/* tp_dealloc*/
  0,					/* tp_print*/
  0,					/* tp_getattr*/
  0,					/* tp_setattr*/
  0,					/* tp_compare*/
  0,					/* tp_repr*/
  0,					/* tp_as_number*/
  0,					/* tp_as_sequence*/
  0,					/* tp_as_mapping*/
  0,					/* tp_hash */
  0,					/* tp_call*/
  0,					/* tp_str*/
  0,					/* tp_getattro*/
  0,					/* tp_setattro*/
  0,					/* tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,/* tp_flags*/
  "Parameter and statistics collection objects.  An instance of\n"
  "this exists as the 'params' attribute of LPX instances.  By\n"
  "changing parameters here, one may affect the behavior of\n"
  "the solvers, data readers, writers, verbosity, etc.",
	/* tp_doc */
  (traverseproc)Params_traverse,	/* tp_traverse */
  (inquiry)Params_clear,		/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(ParamsObject, weakreflist),	/* tp_weaklistoffset */
  0,					/* tp_iter */
  0,					/* tp_iternext */
  Params_methods,			/* tp_methods */
  Params_members,			/* tp_members */
  Params_getset,			/* tp_getset */
};
