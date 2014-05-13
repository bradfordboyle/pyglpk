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

#ifndef _OBJ_H
#define _OBJ_H

#include <Python.h>
#include "lp.h"

#define Obj_Check(op) PyObject_TypeCheck(op, &ObjType)

typedef struct {
  PyObject_HEAD
  LPXObject *py_lp;
  PyObject *weakreflist; // Weak reference list.
} ObjObject;

extern PyTypeObject ObjType;
extern PyTypeObject ObjIterType;

/* Returns a new objective object. */
ObjObject *Obj_New(LPXObject *py_lp);
/* Return the number of objective coefficients. */
int Obj_Size(ObjObject *bc);
/* Init the type and related types it contains. 0 on success. */
int Obj_InitType(PyObject *module);

#endif // _OBJ_H
