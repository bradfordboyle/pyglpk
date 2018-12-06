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

#include "2to3.h"

#include "tree.h"
#include "structmember.h"
#include "util.h"
#include <string.h>

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
  Py_TYPE(tn)->tp_free((PyObject*)tn);
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
    ("<%s, %sactive subprob %d of %s %p>", Py_TYPE(self)->tp_name,
     self->active?"":"in",
     self->subproblem, TreeType.tp_name, self->py_tree);
}

PyDoc_STRVAR(subproblem_doc,
"The reference number of the subproblem corresponding to this node."
);

static PyMemberDef TreeNode_members[] = {
  {"subproblem", T_INT, offsetof(TreeNodeObject, subproblem), READONLY,
   subproblem_doc},
  {NULL}
};

PyDoc_STRVAR(next_doc,
"The next active subproblem node, None if there is no next active\n"
"subproblem, or if this is not an active subproblem."
);

PyDoc_STRVAR(prev_doc,
"The previous active subproblem node, None if there is no previous active\n"
"subproblem, or if this is not an active subproblem."
);

PyDoc_STRVAR(up_doc, "The parent subproblem node, None if this is the root." );

PyDoc_STRVAR(level_doc,
"The level of the node in the tree, with 0 if this is the root."
);

PyDoc_STRVAR(bound_doc, "The local bound for this node's subproblem." );

PyDoc_STRVAR(active_doc, "Whether this node represents an active subproblem.");

static PyGetSetDef TreeNode_getset[] = {
  {"next", (getter)TreeNode_getothernode, (setter)NULL, next_doc,
  glp_ios_next_node},
  {"prev", (getter)TreeNode_getothernode, (setter)NULL, prev_doc,
   glp_ios_prev_node},
  {"up", (getter)TreeNode_getupnode, (setter)NULL, up_doc, NULL},
  {"level", (getter)TreeNode_getlevel, (setter)NULL, level_doc, NULL},
  {"bound", (getter)TreeNode_getbound, (setter)NULL, bound_doc, NULL},
  {"active", (getter)TreeNode_getactive, (setter)NULL, active_doc, NULL},
  {NULL}
};

static PyMethodDef TreeNode_methods[] = {
  {NULL}
};

PyDoc_STRVAR(tree_node_doc,
"Represent specific subproblem instances in the search Tree object used by\n"
"the MIP solver."
);

PyTypeObject TreeNodeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name           = "glpk.TreeNode",
    .tp_basicsize      = sizeof(TreeNodeObject),
    .tp_dealloc        = (destructor) TreeNode_dealloc,
    .tp_repr           = (reprfunc) TreeNode_Str,
    .tp_str            = (reprfunc) TreeNode_Str,
    .tp_flags          = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc            = tree_node_doc,
    .tp_richcompare    = (richcmpfunc) TreeNode_richcompare,
    .tp_weaklistoffset = offsetof(TreeNodeObject, weakreflist),
    .tp_methods        = TreeNode_methods,
    .tp_members        = TreeNode_members,
    .tp_getset         = TreeNode_getset,
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
  Py_TYPE(it)->tp_free((PyObject*)it);
}

static PyObject *TreeIter_next(TreeIterObject *self) {
  CHECKTREE;
  int subproblem = glp_ios_next_node(TREE, self->last_subproblem);
  if (subproblem==0) return NULL;
  self->last_subproblem = subproblem;
  return (PyObject*)TreeNode_New(self->py_tree, subproblem, 1);
}

PyDoc_STRVAR(tree_iter_doc,
"Tree iterator objects.\n"
"\n"
"Created for iterating over the active subproblems of the search tree."
);

PyTypeObject TreeIterType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name           = "glpk.TreeIter",
    .tp_basicsize      = sizeof(TreeIterObject),
    .tp_dealloc        = (destructor)TreeIter_dealloc,
    .tp_getattro       = PyObject_GenericGetAttr,
    .tp_flags          = Py_TPFLAGS_DEFAULT,
    .tp_doc            = tree_iter_doc,
    .tp_weaklistoffset = offsetof(TreeIterObject, weakreflist),
    .tp_iter           = PyObject_SelfIter,
    .tp_iternext       = (iternextfunc) TreeIter_next,
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
  Py_TYPE(self)->tp_free((PyObject*)self);
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
  if (j<1 || j>numcols) {
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
  if (j<1 || j>numcols) {
    PyErr_Format(PyExc_IndexError, "index %d out of bound for %d columns",
		 j, numcols);
    return NULL;
  }
  if (!glp_ios_can_branch(TREE, j)) {
    PyErr_SetString(PyExc_RuntimeError, "cannot branch upon this column");
    return NULL;
  }
  switch (select) {
  case 'D':
    glp_ios_branch_upon(TREE, j, GLP_DN_BRNCH);
    Py_RETURN_NONE;
  case 'U':
    glp_ios_branch_upon(TREE, j, GLP_UP_BRNCH);
    Py_RETURN_NONE;
  case 'N':
    glp_ios_branch_upon(TREE, j, GLP_NO_BRNCH);
    Py_RETURN_NONE;
  default:
    PyErr_SetString(PyExc_ValueError, "select argument must be D, U, or N");
    return NULL;
  }
}

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
  case GLP_ISELECT: return PyString_FromString("select"); break;
  case GLP_IPREPRO: return PyString_FromString("prepro"); break;
  case GLP_IBRANCH: return PyString_FromString("branch"); break;
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

PyDoc_STRVAR(lp_doc,
"Problem object used by the MIP solver."
);

static PyMemberDef Tree_members[] = {
  {"lp", T_OBJECT_EX, offsetof(TreeObject, py_lp), READONLY, lp_doc},
  {NULL}
};

PyDoc_STRVAR(reason_doc,
"A string with the reason the callback function has been called."
);

PyDoc_STRVAR(num_active_doc,
"The number of active nodes."
);

PyDoc_STRVAR(num_all_doc,
"The number of all nodes, both active and inactive."
);

PyDoc_STRVAR(num_total_doc,
"The total number of nodes, including those already removed."
);

PyDoc_STRVAR(curr_node_doc,
"The node of the current active subproblem. If there is no current active\n"
"subproblem in the tree, this will return None."
);

PyDoc_STRVAR(best_node_doc,
"The node of the current active subproblem with best local bound. If the\n"
"tree is empty, this is None."
);

PyDoc_STRVAR(first_node_doc,
"The node of the first active subproblem. If there is no current active\n"
"subproblem in the tree, this is None."
);

PyDoc_STRVAR(last_node_doc,
"The node of the last active subproblem. If there is no current active\n"
"subproblem in the tree, this is None."
);

PyDoc_STRVAR(gap_doc,
"The relative MIP gap (duality gap), that is, the gap between the best MIP\n"
"solution (best_mip) and best relaxed solution (best_bnd) given by this\n"
"formula:\n"
"\n"
"      |best_mip - best_bnd|\n"
"gap = ---------------------\n"
"      |best_mip| + epsilon \n"
"\n"
);

static PyGetSetDef Tree_getset[] = {
  {"reason", (getter)Tree_getreason, (setter)NULL, reason_doc, NULL},
  {"num_active", (getter)Tree_getnumactive, (setter)NULL, num_active_doc,
  NULL},
  {"num_all", (getter)Tree_getnumall, (setter)NULL, num_all_doc, NULL},
  {"num_total", (getter)Tree_getnumtotal, (setter)NULL, num_total_doc, NULL},
  {"curr_node", (getter)Tree_gettreenode, (setter)NULL, curr_node_doc,
  glp_ios_curr_node},
  {"best_node", (getter)Tree_gettreenode, (setter)NULL, best_node_doc,
  glp_ios_best_node},
  {"first_node", (getter)Tree_getfirstlastnode, (setter)NULL,first_node_doc,
  glp_ios_next_node},
  {"last_node", (getter)Tree_getfirstlastnode, (setter)NULL, last_node_doc,
  glp_ios_prev_node},
  {"gap", (getter)Tree_getgap, (setter)NULL, gap_doc, NULL},
  {NULL}
};

PyDoc_STRVAR(terminate_doc,
"terminate()\n"
"\n"
"Prematurely terminate the MIP solver's search."
);

PyDoc_STRVAR(select_doc,
"select(node)\n"
"\n"
"Selects a tree node to continue search from. Note that this function should\n"
"be called only when the reason member of the tree is 'select'."
);

PyDoc_STRVAR(can_branch_doc,
"can_branch(col_index)\n"
"\n"
"Given the index of a column in the LP, this will return True if one can\n"
"branch upon this column's varible, that is, continue the search with this\n"
"column's variable set as an integer. Note that this function should be\n"
"called only when the reason member of the tree is 'branch'."
);

PyDoc_STRVAR(branch_upon_doc,
"branch_upon(col_index, select='N')\n"
"\n"
"Given the index of a column in the LP, this will add two new subproblems,\n"
"down and up branches (in that order) to the active list, where the down and\n"
"up branches are the problems with the column's variable set to the floor\n"
"and ceil of the value, respectively. The select parameter controls which of\n"
"the two branches is selected to next continue the search with 'D', 'U', and\n"
"'N' corresponding to choosing the down, up, or letting GLPK select a\n"
"branch, respectively."
);

PyDoc_STRVAR(heuristic_doc,
"heuristic(values)\n"
"\n"
"Provide an integer feasible solution of the primal problem, where values is\n"
"an iterable object yielding at least as many float values as there are\n"
"columns in the problem. If the provided solution is better than the\n"
"existing one, the solution is accepted and the problem updated. This\n"
"function returns True or False depending on whether the solution was\n"
"accepted or not. Note that this function should be called only when the\n"
"reason member of the tree is 'heur'."
);

static PyMethodDef Tree_methods[] = {
  {"terminate", (PyCFunction)Tree_terminate, METH_NOARGS, terminate_doc},
  {"select", (PyCFunction)Tree_select, METH_VARARGS, select_doc},
  {"can_branch", (PyCFunction)Tree_canbranch, METH_VARARGS, can_branch_doc},
  {"branch_upon", (PyCFunction)Tree_branchupon, METH_VARARGS, branch_upon_doc},
  {"heuristic", (PyCFunction)Tree_heuristic, METH_O, heuristic_doc},
  {NULL}
};

PyDoc_STRVAR(tree_doc,
"Tree instances are passed to MIP solver callback function.\n"
"\n"
"They are used to indicate the state of the solver at some intermediate\n"
"point in a call to LPX.integer(). There are nodes within the tree,\n"
"instances of TreeNode, corresponding to subproblems within the search tree.\n"
"The currently active subproblem is stored in the curr_node member of an\n"
"instance."
);

PyTypeObject TreeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name           = "glpk.Tree",
    .tp_basicsize      = sizeof(TreeObject),
    .tp_dealloc        = (destructor) Tree_dealloc,
    .tp_flags          = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc            = tree_doc,
    .tp_weaklistoffset = offsetof(TreeObject, weakreflist),
    .tp_iter           = Tree_Iter,
    .tp_methods        = Tree_methods,
    .tp_members        = Tree_members,
    .tp_getset         = Tree_getset,
};
