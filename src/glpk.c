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

#include <Python.h>
#include "lp.h"
#include "environment.h"

#if PY_MAJOR_VERSION >= 3

#define MOD_ERROR_VAL NULL
#define MOD_SUCCESS_VAL(val) val
#define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#define MOD_DEF(ob, name, doc, methods) \
        static struct PyModuleDef moduledef = { \
        PyModuleDef_HEAD_INIT, name, doc, -1, methods, }; \
        ob = PyModule_Create(&moduledef);

#else

#define MOD_ERROR_VAL
#define MOD_SUCCESS_VAL(val)
#define MOD_INIT(name) void init##name(void)
#define MOD_DEF(ob, name, doc, methods) \
        ob = Py_InitModule3(name, methods, doc);

#endif

static PyMethodDef GLPKMethods[] = {
  {NULL, NULL, 0, NULL}
};

static char module___doc__[] =
"The PyGLPK module, encapsulating the functionality of the GNU\n\
Linear Programming Kit. Usage of this module will typically\n\
start with the initialization of an LPX instance to define a\n\
linear program, and proceed from there to add data to the\n\
problem and ultimately solve it. See help on the LPX class,\n\
as well as the HTML documentation accompanying your PyGLPK\n\
distribution.";

MOD_INIT(glpk) {
  PyObject *m;
  
  MOD_DEF(m, "glpk", module___doc__, GLPKMethods)
  
  if (m==NULL) return MOD_ERROR_VAL;
  
  PyModule_AddStringConstant(m, "__version__", "0.5.0-SNAPSHOT");

  Environment_InitType(m);

  PyModule_AddObject(m, ENVIRONMENT_INSTANCE_NAME,
		     (PyObject*)Environment_New());

  LPX_InitType(m);

  // Do a quick and dirty version check, so as to warn the user that
  // they should recompile PyGLPK if the underlying glpk shared
  // library has changed.
  char myversion[10];
  snprintf(myversion, 10, "%d.%d", GLP_MAJOR_VERSION, GLP_MINOR_VERSION);
  if (strcmp(myversion, glp_version())) {
    char buf[100];
    snprintf(buf, 100, "PyGLPK compiled on GLPK %s, but runtime is GLPK %s",
	     myversion, glp_version());
#if PY_MAJOR_VERSION == 2
#if PY_MINOR_VERSION >= 5
    PyErr_WarnEx(PyExc_RuntimeWarning, buf, 1);
#else
    PyErr_Warn(PyExc_RuntimeWarning, buf);
#endif
#endif
  }
  
  return MOD_SUCCESS_VAL(m);
}
