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

#ifndef _PARAMS_H
#define _PARAMS_H

#include <Python.h>
#include "lp.h"

#define Params_Check(op) PyObject_TypeCheck(op, &ParamsType)

typedef struct {
  PyObject_HEAD
  LPXObject *py_lp;
  PyObject *weakreflist; // Weak reference list.
} ParamsObject;

extern PyTypeObject ParamsType;

/* Returns a new parameter collection object for the LP. */
ParamsObject *Params_New(LPXObject *py_lp);
/* Reset all parameters to their defaults. */
void Params_Reset(ParamsObject *params);

/* Init the type and related types it contains. 0 on success. */
int Params_InitType(PyObject *module);

#endif // _PARAMS_H
