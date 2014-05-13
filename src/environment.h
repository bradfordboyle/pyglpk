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

#ifndef _ENVIRONMENT_H
#define _ENVIRONMENT_H

#include <Python.h>
#include "glpk.h"

#define ENVIRONMENT_INSTANCE_NAME "env"

#define Environment_Check(op) PyObject_TypeCheck(op, &EnvironmentType)

typedef struct {
  PyObject_HEAD
  int mem_limit;
  unsigned int term_on:1;
  PyObject *term_hook;
  PyObject *version;
  PyObject *weakreflist; // Weak reference list.
} EnvironmentObject;

extern PyTypeObject EnvironmentType;

/* Creates a new Environment object for controlling the GLPK environment. */
EnvironmentObject* Environment_New(void);
/* Init the type and related types it contains. 0 on success. */
int Environment_InitType(PyObject *module);

#endif // _ENVIRONMENT_H
