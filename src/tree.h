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

#ifndef _TREE_H
#define _TREE_H

#include <Python.h>
#include "lp.h"
#include "util.h"

#if GLPK_VERSION(4, 20)

#define Tree_Check(op) PyObject_TypeCheck(op, &TreeType)

typedef struct {
  PyObject_HEAD
  glp_tree *tree;
  unsigned char selected:1;
  LPXObject *py_lp;
  PyObject *weakreflist; // Weak reference list.
} TreeObject;

extern PyTypeObject TreeType;
extern PyTypeObject TreeIterType;
extern PyTypeObject TreeNodeType;

/* Returns a new bar collection object, either over rows or columns. */
TreeObject *Tree_New(glp_tree *tree, LPXObject *py_lp);
/* Init the type and related types it contains. 0 on success. */
int Tree_InitType(PyObject *module);

#endif // GLPK_VERSION(4, 20)

#endif // _TREE_H
