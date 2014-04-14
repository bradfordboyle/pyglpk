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

#include "tree.h"
#include "structmember.h"
#include "util.h"
#include <string.h>

#if GLPK_VERSION(4, 20)
;
#define TREE (self->py_tree->tree)
#define CHECKTREE							\
  if (!TREE) {								\
    PyErr_SetString(PyExc_RuntimeError, "tree object no long valid");	\
    return NULL;							\
  }
#define TreeNode_Check(op) PyObject_TypeCheck(op, &TreeNodeType)

/************************ TREE NODE OBJECT IMPLEMENTATION *******************/

typedef struct {
  PyObject_HEAD
  TreeObject *py_tree;
  int subproblem;
  unsigned char active:1;
  PyObject *weakreflist;
} TreeNodeObject;

static PyObject *TreeNode_New(TreeObject *py_tree,int subproblem,int active) {
  TreeNodeObject *tn;
  if (!Tree_Check(py_tree)) {
    PyErr_BadInternalCall();
    return NULL;
  }
  tn = PyObject_New(TreeNodeObject, &TreeNodeType);
  if (tn == NULL) return NULL;
  tn->weakreflist = NULL;
  tn->subproblem = subproblem;
  tn->active = active ? 1 : 0;
  Py_INCREF(py_tree);
  tn->py_tree = (TreeObject *)py_tree;
  return (PyObject *)tn;
}

static void TreeNode_dealloc(TreeNodeObject *tn) {
  if (tn->weakreflist) {
    PyObject_ClearWeakRefs((PyObject*)tn);
  }
  Py_XDECREF(tn->py_tree);
  tn->ob_type->tp_free((PyObject*)tn);
}

static PyObject* TreeNode_richcompare(TreeNodeObject *v, PyObject *w, int op) {
  if (!TreeNode_Check(w)) {
    switch (op) {
    case Py_EQ: Py_RETURN_FALSE;
    case Py_NE: Py_RETURN_FALSE;
    default:
      Py_INCREF(Py_NotImplemented);
      return Py_NotImplemented;
    }
  }
  // Now we know it is a tree node object.
  TreeNodeObject *x = (TreeNodeObject *)w;
  if (v->py_tree != x->py_tree) {
    // "Inherit" the judgement of our containing objects.
    return PyObject_RichCompare
      ((PyObject*)v->py_tree, (PyObject*)x->py_tree, op);
  }
  // Now we know it is a tree node object, and part of the same tree
  // no less.
  switch (op) {
  case Py_EQ:
    if (v->subproblem==x->subproblem) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_NE:
    if (v->subproblem!=x->subproblem) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_LE:
    if (v->subproblem<=x->subproblem) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_GE:
    if (v->subproblem>=x->subproblem) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_LT:
    if (v->subproblem< x->subproblem) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  case Py_GT:
    if (v->subproblem> x->subproblem) Py_RETURN_TRUE; else Py_RETURN_FALSE;
  default:
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static PyObject* TreeNode_getothernode(TreeNodeObject *self, void *closure) {
  int(*othernodefunc)(glp_tree*,int) = (int(*)(glp_tree*,int))closure;
  CHECKTREE;
  if (!self->active) {
    // What is appropriate?  Throw an exception, or just return None?
    // I am unsure, but it seems odd that merely accessing an
    // attribute which is named and appears to exist will throw an
    // exception.
    Py_RETURN_NONE;
  }
  int othersubproblem = othernodefunc(TREE, self->subproblem);
  if (othersubproblem==0) Py_RETURN_NONE;
  return TreeNode_New(self->py_tree, othersubproblem, 1);
}

static PyObject* TreeNode_getupnode(TreeNodeObject *self, void *closure) {
  CHECKTREE;
  int othersubproblem = glp_ios_up_node(TREE, self->subproblem);
  if (othersubproblem==0) Py_RETURN_NONE;
  return TreeNode_New(self->py_tree, othersubproblem, 0);
}

static PyObject* TreeNode_getlevel(TreeNodeObject *self, void *closure) {
  CHECKTREE;
  return PyInt_FromLong(glp_ios_node_level(TREE, self->subproblem));
}

static PyObject* TreeNode_getbound(TreeNodeObject *self, void *closure) {
  CHECKTREE;
  return PyFloat_FromDouble(glp_ios_node_bound(TREE, self->subproblem));
}

static PyObject* TreeNode_getactive(TreeNodeObject *self, void *closure) {
  CHECKTREE;
  return PyBool_FromLong(self->active);
}

static PyObject* TreeNode_Str(TreeNodeObject *self) {
  // Returns a string representation of this object.
  return PyString_FromFormat
    ("<%s, %sactive subprob %d of %s %p>", self->ob_type->tp_name,
     self->active?"":"in",
     self->subproblem, TreeType.tp_name, self->py_tree);
}

static PyMemberDef TreeNode_members[] = {
  {"subproblem", T_INT, offsetof(TreeNodeObject, subproblem), RO,
   "The reference number of the subproblem corresponding to this node."},
  {NULL}
};

static PyGetSetDef TreeNode_getset[] = {
  {"next", (getter)TreeNode_getothernode, (setter)NULL,
   "The next active subproblem node, None if there is no next active\n"
   "subproblem, or if this is not an active subproblem.", glp_ios_next_node},
  {"prev", (getter)TreeNode_getothernode, (setter)NULL,
   "The previous active subproblem node, None if there is no previous\n"
   "active subproblem, or if this is not an active subproblem.",
   glp_ios_prev_node},
  {"up", (getter)TreeNode_getupnode, (setter)NULL,
   "The parent subproblem node, None if this is the root.", NULL},
  {"level", (getter)TreeNode_getlevel, (setter)NULL,
   "The level of the node in the tree, with 0 if this is the root.", NULL},
  {"bound", (getter)TreeNode_getbound, (setter)NULL,
   "The local bound for this node's subproblem.", NULL},
  {"active", (getter)TreeNode_getactive, (setter)NULL,
   "Whether this node represents an active subproblem.", NULL},
  {NULL}
};

static PyMethodDef TreeNode_methods[] = {
  /*{"terminate", (PyCFunction)TreeNode_terminate, METH_NOARGS,
   "terminate()\n\n"
   "Prematurely terminate the MIP solver's search."},*/
  {NULL}
};

PyTypeObject TreeNodeType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.TreeNode",				/* tp_name */
  sizeof(TreeNodeObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)TreeNode_dealloc,		/* tp_dealloc*/
  0,					/* tp_print*/
  0,					/* tp_getattr*/
  0,					/* tp_setattr*/
  0,					/* tp_compare*/
  (reprfunc)TreeNode_Str,		/* tp_repr*/
  0,					/* tp_as_number*/
  0,					/* tp_as_sequence*/
  0,					/* tp_as_mapping*/
  0,					/* tp_hash */
  0,					/* tp_call*/
  (reprfunc)TreeNode_Str,		/* tp_str*/
  0,					/* tp_getattro*/
  0,					/* tp_setattro*/
  0,					/* tp_as_buffer*/
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags*/
  "TreeNode instances represent specific subproblem instances in the\n"
  "search Tree object used by the MIP solver.",
					/* tp_doc */
  0,					/* tp_traverse */
  0,					/* tp_clear */
  (richcmpfunc)TreeNode_richcompare,	/* tp_richcompare */
  offsetof(TreeNodeObject, weakreflist),/* tp_weaklistoffset */
  0,					/* tp_iter */
  0,					/* tp_iternext */
  TreeNode_methods,			/* tp_methods */
  TreeNode_members,			/* tp_members */
  TreeNode_getset,			/* tp_getset */
};

/************************ TREE ITER IMPLEMENTATION ************************/

typedef struct {
  PyObject_HEAD
  int last_subproblem;
  TreeObject *py_tree;
  PyObject *weakreflist; // Weak reference list.
} TreeIterObject;

static PyObject *Tree_Iter(PyObject *py_tree) {
  TreeIterObject *it;
  if (!Tree_Check(py_tree)) {
    PyErr_BadInternalCall();
    return NULL;
  }
  it = PyObject_New(TreeIterObject, &TreeIterType);
  if (it == NULL) return NULL;
  it->weakreflist = NULL;
  it->last_subproblem = 0;
  Py_INCREF(py_tree);
  it->py_tree = (TreeObject *)py_tree;
  return (PyObject *)it;
}

static void TreeIter_dealloc(TreeIterObject *it) {
  if (it->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)it);
  }
  Py_XDECREF(it->py_tree);
  it->ob_type->tp_free((PyObject*)it);
}

static PyObject *TreeIter_next(TreeIterObject *self) {
  CHECKTREE;
  int subproblem = glp_ios_next_node(TREE, self->last_subproblem);
  if (subproblem==0) return NULL;
  self->last_subproblem = subproblem;
  return (PyObject*)TreeNode_New(self->py_tree, subproblem, 1);
}

PyTypeObject TreeIterType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.TreeIter",			/* tp_name */
  sizeof(TreeIterObject),		/* tp_basicsize */
  0,					/* tp_itemsize */
  (destructor)TreeIter_dealloc,		/* tp_dealloc */
  0,					/* tp_print */
  0,					/* tp_getattr */
  0,					/* tp_setattr */
  0,					/* tp_compare */
  0,					/* tp_repr */
  0,					/* tp_as_number */
  0,					/* tp_as_sequence */
  0,					/* tp_as_mapping */
  0,					/* tp_hash */
  0,					/* tp_call */
  0,					/* tp_str */
  PyObject_GenericGetAttr,		/* tp_getattro */
  0,					/* tp_setattro */
  0,					/* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,			/* tp_flags */
  "Tree iterator objects.  Created for iterating over the\n"
  "active subproblems of the search tree.",	/* tp_doc */
  0,					/* tp_traverse */
  0,					/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(TreeIterObject, weakreflist),/* tp_weaklistoffset */
  PyObject_SelfIter,			/* tp_iter */
  (iternextfunc)TreeIter_next,		/* tp_iternext */
  0,					/* tp_methods */
  0,					/* tp_members */
  0,					/* tp_getset */
  0,					/* tp_base */
  0,					/* tp_dict */
  0,					/* tp_descr_get */
  0,					/* tp_descr_set */
  0,					/* tp_dictoffset */
};

/************************ TREE OBJECT IMPLEMENTATION ************************/

#undef TREE
#define LP (self->py_lp->lp)
#define TREE (self->tree)

static void Tree_dealloc(TreeObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_XDECREF(self->py_lp);
  self->ob_type->tp_free((PyObject*)self);
}

/** Create a new parameter collection object. */
TreeObject *Tree_New(glp_tree *tree, LPXObject *py_lp) {
  TreeObject *t = (TreeObject*)PyObject_New(TreeObject, &TreeType);
  if (t==NULL) return t;
  t->weakreflist = NULL;
  t->tree = tree;
  // According to the GLPK documentation, sometimes the glp_prob
  // instance is not the same as the one that invoked the solver if
  // the MIP presolver is employed.  In such an instance, we do not
  // use the py_lp as our LPXObject, but rather one created from this
  // intermediate problem.
  if (py_lp->lp != glp_ios_get_prob(tree)) {
    py_lp = LPX_FromLP(glp_ios_get_prob(tree));
    if (py_lp==NULL) {
      Py_DECREF(t);
      return NULL;
    }
  } else {
    Py_INCREF(py_lp);
  }
  t->py_lp = py_lp;
  t->selected = 0;
  return t;
}

#undef CHECKTREE
#define CHECKTREE							\
  if (!TREE) {								\
    PyErr_SetString(PyExc_RuntimeError, "tree object no long valid");	\
    return NULL;							\
  }

static PyObject *Tree_terminate(TreeObject *self) {
  CHECKTREE;
  glp_ios_terminate(self->tree);
  Py_RETURN_NONE;
}


#if GLPK_VERSION(4, 21)
static PyObject *Tree_select(TreeObject *self, PyObject *args) {
  TreeNodeObject *node=NULL;
  CHECKTREE;
  if (!PyArg_ParseTuple(args, "O!", &TreeNodeType, &node)) {
    return NULL;
  }
  if (glp_ios_reason(TREE) != GLP_ISELECT) {
    PyErr_SetString(PyExc_RuntimeError,
		    "function may only be called during select phase");
    return NULL;
  }
  if (self->selected) {
    PyErr_SetString(PyExc_RuntimeError, "function must be called only once");
    return NULL;
  }
  if (self != node->py_tree) {
    PyErr_SetString(PyExc_ValueError, "node did not come from this tree");
    return NULL;
  }
  if (!node->active) {
    PyErr_SetString(PyExc_ValueError, "node is not active");
    return NULL;
  }
  glp_ios_select_node(TREE, node->subproblem);
  self->selected = 1;
  Py_RETURN_NONE;
}

static PyObject *Tree_canbranch(TreeObject *self, PyObject *args) {
  CHECKTREE;
  if (glp_ios_reason(TREE) != GLP_IBRANCH) {
    PyErr_SetString(PyExc_RuntimeError,
		    "function may only be called during branch phase");
    return NULL;
  }
  int j;
  if (!PyArg_ParseTuple(args, "i", &j)) {
    return NULL;
  }
  int numcols = glp_get_num_cols(LP);
  if (j<0 || j>=numcols) {
    PyErr_Format(PyExc_IndexError, "index %d out of bound for %d columns",
		 j, numcols);
    return NULL;
  }
  if (glp_ios_can_branch(TREE, j)) Py_RETURN_TRUE; else Py_RETURN_FALSE;
}

static PyObject *Tree_branchupon(TreeObject *self, PyObject *args) {
  CHECKTREE;
  if (glp_ios_reason(TREE) != GLP_IBRANCH) {
    PyErr_SetString(PyExc_RuntimeError,
		    "function may only be called during branch phase");
    return NULL;
  }
  int j;
  char select='N';
  if (!PyArg_ParseTuple(args, "i|c", &j, &select)) {
    return NULL;
  }
  int numcols = glp_get_num_cols(LP);
  if (j<0 || j>=numcols) {
    PyErr_Format(PyExc_IndexError, "index %d out of bound for %d columns",
		 j, numcols);
    return NULL;
  }
  if (!glp_ios_can_branch(TREE, j)) {
    PyErr_SetString(PyExc_RuntimeError, "cannot branch upon this column");
    return NULL;
  }
  switch (select) {
  case 'D': case 'U': case 'N':
    glp_ios_branch_upon(TREE, j, select);
    Py_RETURN_NONE;
  default:
    PyErr_SetString(PyExc_ValueError, "select argument must be D, U, or N");
    return NULL;
  }
}
#endif // GLPK_VERSION(4, 21)

static PyObject *Tree_heuristic(TreeObject *self, PyObject *arg) {
  CHECKTREE;
  if (glp_ios_reason(TREE) != GLP_IHEUR) {
    PyErr_SetString(PyExc_RuntimeError,
		    "function may only be called during heur phase");
    return NULL;
  }
  int i, notaccepted, numcols = glp_get_num_cols(LP);
  // Try to get an iterator.
  arg = PyObject_GetIter(arg);
  if (arg==NULL) return NULL;
  double *x = calloc(numcols, sizeof(double));
  for (i=0; i<numcols; ++i) {
    PyObject *item = PyIter_Next(arg);
    if (item==NULL) {
      // We should not be stopping this early...
      free(x);
      Py_DECREF(arg);
      if (PyErr_Occurred()) return NULL;
      PyErr_Format
	(PyExc_ValueError, "iterator had only %d objects, but %d required",
	 i+1, numcols);
      return NULL;
    }
    x[i] = PyFloat_AsDouble(item);
    Py_DECREF(item);
    if (PyErr_Occurred()) {
      free(x);
      Py_DECREF(arg);
      PyErr_SetString(PyExc_TypeError, "iterator must return floats");
      return NULL;
    }
  }
  Py_DECREF(arg);
  notaccepted = glp_ios_heur_sol(TREE, x-1);
  free(x);
  if (notaccepted) Py_RETURN_FALSE; else Py_RETURN_TRUE;
}

/****************** GET-SET-ERS ***************/

static PyObject* Tree_getreason(TreeObject *self, void *closure) {
  CHECKTREE;
  switch (glp_ios_reason(TREE)) {
#if GLPK_VERSION (4, 21)
  case GLP_ISELECT: return PyString_FromString("select"); break;
  case GLP_IPREPRO: return PyString_FromString("prepro"); break;
  case GLP_IBRANCH: return PyString_FromString("branch"); break;
#endif
  case GLP_IROWGEN: return PyString_FromString("rowgen"); break; 
  case GLP_IHEUR:   return PyString_FromString("heur");   break;
  case GLP_ICUTGEN: return PyString_FromString("cutgen"); break;
  case GLP_IBINGO:  return PyString_FromString("bingo");  break;
  default:
    // This should never happen.
    PyErr_SetString(PyExc_RuntimeError, "unrecognized reason for callback");
    return NULL;
  }
}

static PyObject* Tree_getnumactive(TreeObject *self, void *closure) {
  CHECKTREE;
  int count;
  glp_ios_tree_size(TREE, &count, NULL, NULL);
  return PyInt_FromLong(count);
}

static PyObject* Tree_getnumall(TreeObject *self, void *closure) {
  CHECKTREE;
  int count;
  glp_ios_tree_size(TREE, NULL, &count, NULL);
  return PyInt_FromLong(count);
}

static PyObject* Tree_getnumtotal(TreeObject *self, void *closure) {
  CHECKTREE;
  int count;
  glp_ios_tree_size(TREE, NULL, NULL, &count);
  return PyInt_FromLong(count);
}

static PyObject* Tree_gettreenode(TreeObject *self, void *closure) {
  int(*nodefunc)(glp_tree*) = (int(*)(glp_tree*))closure;
  CHECKTREE;
  int subproblem = nodefunc(TREE);
  if (subproblem==0) Py_RETURN_NONE;
  return TreeNode_New(self, subproblem, 1);
}

static PyObject* Tree_getfirstlastnode(TreeObject *self, void *closure) {
  int(*othernodefunc)(glp_tree*,int) = (int(*)(glp_tree*,int))closure;
  CHECKTREE;
  int subproblem = othernodefunc(TREE, 0);
  if (subproblem==0) Py_RETURN_NONE;
  return TreeNode_New(self, subproblem, 1);
}

static PyObject* Tree_getgap(TreeObject *self, void *closure) {
  CHECKTREE;
  return PyFloat_FromDouble(glp_ios_mip_gap(TREE));
}

/****************** OBJECT DEFINITION *********/

int Tree_InitType(PyObject *module) {
  int retval;
  if ((retval=util_add_type(module, &TreeType))!=0) return retval;
  if ((retval=util_add_type(module, &TreeNodeType))!=0) return retval;
  if ((retval=util_add_type(module, &TreeIterType))!=0) return retval;
  return 0;
}

static PyMemberDef Tree_members[] = {
  {"lp", T_OBJECT_EX, offsetof(TreeObject, py_lp), RO,
   "Problem object used by the MIP solver."},
  {NULL}
};

static PyGetSetDef Tree_getset[] = {
  {"reason", (getter)Tree_getreason, (setter)NULL,
   "A string with the reason the callback function has been called.", NULL},
  {"num_active", (getter)Tree_getnumactive, (setter)NULL,
   "The number of active nodes.", NULL},
  {"num_all", (getter)Tree_getnumall, (setter)NULL,
   "The number of all nodes, both active and inactive.", NULL},
  {"num_total", (getter)Tree_getnumtotal, (setter)NULL,
   "The total number of nodes, including those already removed.", NULL},
  {"curr_node", (getter)Tree_gettreenode, (setter)NULL,
   "The node of the current active subproblem.  If there is no current\n"
   "active subproblem in the tree, this will return None.", glp_ios_curr_node},
  {"best_node", (getter)Tree_gettreenode, (setter)NULL,
   "The node of the current active subproblem with best local bound.\n"
   "If the tree is empty, this is None.", glp_ios_best_node},
  {"first_node", (getter)Tree_getfirstlastnode, (setter)NULL,
   "The node of the first active subproblem.  If there is no current\n"
   "active subproblem in the tree, this is None.", glp_ios_next_node},
  {"last_node", (getter)Tree_getfirstlastnode, (setter)NULL,
   "The node of the last active subproblem.  If there is no current\n"
   "active subproblem in the tree, this is None.", glp_ios_prev_node},
  {"gap", (getter)Tree_getgap, (setter)NULL,
   "The relative MIP gap (duality gap), that is, the gap between the \n"
   "best MIP solution (best_mip) and best relaxed solution (best_bnd)\n"
   "given by this formula:\n"
   "      |best_mip - best_bnd|\n"
   "gap = ---------------------\n"
   "      |best_mip|+epsilon", NULL},
  {NULL}
};

static PyMethodDef Tree_methods[] = {
  {"terminate", (PyCFunction)Tree_terminate, METH_NOARGS,
   "terminate()\n\n"
   "Prematurely terminate the MIP solver's search."},
#if GLPK_VERSION(4, 21)
  {"select", (PyCFunction)Tree_select, METH_VARARGS,
   "select(node)\n\n"
   "Selects a tree node to continue search from.  Note that this\n"
   "function should be called only when the reason member of the\n"
   "tree is 'select'."},
  {"can_branch", (PyCFunction)Tree_canbranch, METH_VARARGS,
   "can_branch(col_index)\n\n"
   "Given the index of a column in the LP, this will return True\n"
   "if one can branch upon this column's varible, that is,\n"
   "continue the search with this column's variable set as an\n"
   "integer.  Note that this function should be called only when\n"
   "the reason member of the tree is 'branch'."},
  {"branch_upon", (PyCFunction)Tree_branchupon, METH_VARARGS,
   "branch_upon(col_index, select='N')\n\n"
   "Given the index of a column in the LP, this will add two\n"
   "new subproblems, down and up branches (in that order) to the\n"
   "active list, where the down and up branches are the problems\n"
   "with the column's variable set to the floor and ceil of the\n"
   "value, respectively.  The select parameter controls which\n"
   "of the two branches is selected to next continue the search\n"
   "with 'D', 'U', and 'N' corresponding to choosing the down,\n"
   "up, or letting GLPK select a branch, respectively."},
#endif // GLPK_VERSION(4, 21)
  {"heuristic", (PyCFunction)Tree_heuristic, METH_O,
   "heuristic(values)\n\n"
   "Provide an integer feasible solution of the primal problem,\n"
   "where values is an iterable object yielding at least as many\n"
   "float values as there are columns in the problem.  If the\n"
   "provided solution is better than the existing one, the\n"
   "solution is accepted and the problem updated.  This function\n"
   "returns True or False depending on whether the solution was\n"
   "accepted or not.  Note that this function should be called\n"
   "only when the reason member of the tree is 'heur'."},
  {NULL}
};

PyTypeObject TreeType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.Tree",				/* tp_name */
  sizeof(TreeObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)Tree_dealloc,		/* tp_dealloc*/
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
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags*/
  "Tree instances are passed to MIP solver callback function.  They\n"
  "are used to indicate the state of the solver at some intermediate\n"
  "point in a call to LPX.integer().  There are nodes within the\n"
  "tree, instances of TreeNode, corresponding to subproblems within\n"
  "the search tree.  The currently active subproblem is stored in\n"
  "the curr_node member of an instance.",
					/* tp_doc */
  0,					/* tp_traverse */
  0,					/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(TreeObject, weakreflist),	/* tp_weaklistoffset */
  Tree_Iter,				/* tp_iter */
  0,					/* tp_iternext */
  Tree_methods,				/* tp_methods */
  Tree_members,				/* tp_members */
  Tree_getset,				/* tp_getset */
};

#endif // GLPK_VERSION(4, 20)
