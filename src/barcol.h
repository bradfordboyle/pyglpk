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

#ifndef _BARCOL_H
#define _BARCOL_H

#include <Python.h>
#include "lp.h"

#define BarCol_Check(op) PyObject_TypeCheck(op, &BarColType)

typedef struct {
  PyObject_HEAD
  LPXObject *py_lp;
  char r;
  int size; // Holds either the size, or -1 if this cached value is invalid.
  PyObject *weakreflist; // Weak reference list.
} BarColObject;

extern PyTypeObject BarColType;
extern PyTypeObject BarColIterType;

/* Init the type and related types it contains. 0 on success. */
int BarCol_InitType(PyObject *module);

/* Returns a new bar collection object, either over rows or columns. */
BarColObject *BarCol_New(LPXObject *py_lp, char rows);

/* Return the number of rows or columns in the bar collection. */
int BarCol_Size(BarColObject *bc);
/* Return non-zero if this bar column object is over rows, 0 if over columns*/
static inline int BarCol_Rows(BarColObject *bc) {return bc->r;};
/* Index into the bar collection using an object, and store the index
   in the provided pointer and return 0.  If this object is neither an
   int or a string return 1 (and raise TypeError if exception flag
   set).  If this object is an int, but is out of range, return 2 (and
   raise IndexError if exception flag set).  If this object is a
   string, but there is no bar with that name, return 4 (and raise
   KeyError if exception flag set).  The exception flag is an int, and
   if the intended return value "and"ed with this exception flag is
   non-zero, the corresponding exception is raised as described. */
int BarCol_Index(BarColObject *self, PyObject *py_index,
		 int *index, int except);

#endif // _BARCOL_H
