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

#ifndef _BAR_H
#define _BAR_H

#include <Python.h>
#include "lp.h"
#include "barcol.h"

#define Bar_Check(op) PyObject_TypeCheck(op, &BarType)

typedef struct {
  PyObject_HEAD
  BarColObject *py_bc;
  char r;
  int index;
  PyObject *weakreflist; // Weak reference list.
} BarObject;

extern PyTypeObject BarType;

/* Returns a new bar collection object, either over rows or columns. */
BarObject *Bar_New(BarColObject *py_bc, int index);
/* Return non-zero if this bar is a row, 0 if a column. */
static inline int Bar_Row(BarObject *b) {return b->r;};
/* Return the index of this bar object. */
static inline int Bar_Index(BarObject *b) {return b->index;};
/* Return non-zero if this bar is still valid (i.e., its index is
   still in range).  If the argument is non-zero, throw an exception
   if it is not. */
int Bar_Valid(BarObject *b, int except);

/* Returns a list of tuple of (index,value) pairs of non-zero entries
   of this row or column of the constraint matrix. */
PyObject *Bar_GetMatrix(BarObject *self);
/* Attempt to set the matrix terms of this bar according to the passed
   in object.  Return -1 on failure with an exception set, 0 on
   success.  Can be viewed as the complement to the get matrix
   function. */
int Bar_SetMatrix(BarObject *b, PyObject *newvals);
/* Init the type and related types it contains. 0 on success. */
int Bar_InitType(PyObject *module);

#endif // _BAR_H
