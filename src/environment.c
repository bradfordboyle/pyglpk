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

#include "environment.h"
#include "structmember.h"
#include "util.h"

static int Environment_traverse(EnvironmentObject *self, visitproc visit,
				void *arg) {
  Py_VISIT(self->version);
  Py_VISIT(self->term_hook);
  return 0;
}

static int Environment_clear(EnvironmentObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_CLEAR(self->version);
  Py_CLEAR(self->term_hook);
  return 0;
}

static void Environment_dealloc(EnvironmentObject *self) {
  Environment_clear(self);
  self->ob_type->tp_free((PyObject*)self);
}

EnvironmentObject* Environment_New(void) {
  EnvironmentObject *env = (EnvironmentObject*)
    PyObject_GC_New(EnvironmentObject, &EnvironmentType);
  if (env==NULL) return env;
  // Initialize data members.
  env->mem_limit = -1;
  env->term_on = 1;
  env->term_hook = NULL;
  env->version = Py_BuildValue("ii", GLP_MAJOR_VERSION, GLP_MINOR_VERSION);
  env->weakreflist = NULL;
  // Now return the structure.
  return env;
}

/****************** GET-SET-ERS ***************/

static PyObject* Environment_getblocks(EnvironmentObject *self,void *closure) {
  int count;
  glp_mem_usage(&count, NULL, NULL, NULL);
  return PyInt_FromLong(count);
}

static PyObject* Environment_getblocks_peak(EnvironmentObject *self,
					   void *closure) {
  int cpeak;
  glp_mem_usage(NULL, &cpeak, NULL, NULL);
  return PyInt_FromLong(cpeak);
}

#if GLPK_VERSION(4,28)
#define GLP_LONG glp_long
#else
#define GLP_LONG glp_ulong
#endif

static PyObject* long2py(GLP_LONG l) {
  if ((l.hi==0 && l.lo>=0) || (l.hi==-1 && l.lo<0))
    return PyInt_FromLong(l.lo);
  PY_LONG_LONG ll = l.hi;
  ll <<= 32;
  ll |= (unsigned int)l.lo;
  return PyLong_FromLongLong(ll);
}

static PyObject* Environment_getbytes(EnvironmentObject *self,void *closure) {
  GLP_LONG b;
  glp_mem_usage(NULL, NULL, &b, NULL);
  return long2py(b);
}

static PyObject* Environment_getbytes_peak(EnvironmentObject *self,
					   void *closure) {
  GLP_LONG b;
  glp_mem_usage(NULL, NULL, NULL, &b);
  return long2py(b);
}

static PyObject* Environment_getmemlimit(EnvironmentObject *self,
					 void *closure) {
  if (self->mem_limit < 0) Py_RETURN_NONE;
  return PyInt_FromLong(self->mem_limit);
}

static int Environment_setmemlimit(EnvironmentObject *self, PyObject *value,
				   void *closure) {
#if GLPK_VERSION(4, 19)
  int limit;
  if (value==NULL || value==Py_None) {
    limit=0x7fffffff;
    self->mem_limit = -1;
  } else if (PyInt_Check(value)) {
    limit=PyInt_AS_LONG(value);
    if (limit<0) {
      PyErr_SetString(PyExc_ValueError, "mem_limit must be non-negative");
      return -1;
    }
    self->mem_limit = limit;
  } else {
    PyErr_SetString(PyExc_TypeError, "mem_limit must be int");
    return -1;
  }
  glp_mem_limit(limit);
  return 0;
#else
  if (value==NULL || value==Py_None) {
    return 0;
  }
  PyErr_SetString(PyExc_NotImplementedError,
		  "memory limits not supported prior to GLPK 4.19");
  return -1;
#endif
}

/**************** TERMINAL BEHAVIOR ***********/

#if !GLPK_VERSION(4, 21)
static int empty_term_hook(void *info, const char *s) { return 1; }
#endif

static int environment_term_hook(EnvironmentObject *env, const char *s) {
  // When this is called, env->term_hook should *never* be NULL.
  PyObject_CallFunction(env->term_hook, "s", s);
  if (PyErr_Occurred()) PyErr_Clear();
  return 1;
}

static PyObject* Environment_gettermon(EnvironmentObject *self,void *closure) {
  if (self->term_on) Py_RETURN_TRUE;
  else Py_RETURN_FALSE;
}

static int Environment_settermon(EnvironmentObject *self,
				 PyObject *value, void *closure) {
  if (!PyBool_Check(value)) {
    PyErr_SetString(PyExc_TypeError, "term_on must be set with bool");
    return -1;
  }
#if GLPK_VERSION(4, 21)
  glp_term_out(value==Py_True ? GLP_ON : GLP_OFF);
#else
  if (value==Py_True) {
    if (self->term_hook) {
      glp_term_hook((int(*)(void*,const char*))environment_term_hook,
		    (void*)self);
    } else {
      glp_term_hook(NULL, NULL);
    }
  } else {
    // Approximate its functionality with an empty terminal hook.
    glp_term_hook(empty_term_hook, NULL);
  }
#endif
  self->term_on = value==Py_True ? 1 : 0;
  return 0;
}

static PyObject* Environment_gettermhook(EnvironmentObject *self,
					 void *closure) {
  if (self->term_hook) {
    Py_INCREF(self->term_hook);
    return self->term_hook;
  } else {
    Py_RETURN_NONE;
  }
}

static int Environment_settermhook(EnvironmentObject *self,
				   PyObject *value, void *closure) {
  // Set the value of the internal terminal hook variable.
  if (value==NULL || value==Py_None) {
    if (self->term_hook) {
      Py_DECREF(self);
    }
    Py_XDECREF(self->term_hook);
    self->term_hook=NULL;
  } else {
    if (self->term_hook==NULL) {
      Py_INCREF(self);
    }
    Py_INCREF(value);
    Py_XDECREF(self->term_hook);
    self->term_hook = value;
  }
  // Next, set the hook with the glp_set_hook function.
#if !GLPK_VERSION(4, 21)
  if (self->term_on==0) return 0;
#endif
  if (self->term_hook) {
    glp_term_hook((int(*)(void*,const char*))environment_term_hook,
		  (void*)self);
  } else {
    glp_term_hook(NULL, NULL);
  }
  return 0;
}

/****************** OBJECT DEFINITION *********/

int Environment_InitType(PyObject *module) {
  return util_add_type(module, &EnvironmentType);
}

static PyMemberDef Environment_members[] = {
  {"version", T_OBJECT_EX, offsetof(EnvironmentObject, version), RO,
   "Tuple holding the major version and minor version of the GLPK\n"
   "that this PyGLPK module was built upon.  For example, if built\n"
   "against GLPK 4.31, version==(4,31)."},
  {NULL}
};

static PyGetSetDef Environment_getset[] = {
  {"mem_limit",(getter)Environment_getmemlimit,(setter)Environment_setmemlimit,
   "The memory limit in megabytes.  None if no limit is set.", NULL},
  {"blocks", (getter)Environment_getblocks, (setter)NULL,
   "The number of currently allocated memory blocks.", NULL},
  {"blocks_peak", (getter)Environment_getblocks_peak, (setter)NULL,
   "The peak value of the blocks attribute.", NULL},
  {"bytes", (getter)Environment_getbytes, (setter)NULL,
   "The number of currently allocated memory bytes.", NULL},
  {"bytes_peak", (getter)Environment_getbytes_peak, (setter)NULL,
   "The peak value of the bytes attribute.", NULL},

  {"term_on", (getter)Environment_gettermon, (setter)Environment_settermon,
   "Whether or not terminal output for the underlying GLPK\n"
   "procedures is on or off.", NULL},
  {"term_hook",(getter)Environment_gettermhook,(setter)Environment_settermhook,
   "Function to intercept all terminal output.  This should be a\n"
   "callable object that accepts a single string argument, or None\n"
   "to indicate that no hook is set (e.g., all output goes to the\n"
   "terminal, default behavior).  Note that when the function is\n"
   "called, there is no guarantee that the input string will be a\n"
   "full line, or even non-empty.  All exceptions thrown by the\n"
   "function will go ignored and unreported." , NULL},
  {NULL}
};

static PyMethodDef Environment_methods[] = {
  /*{"foo", (PyCFunction)Environment_Foo, METH_NOARGS,
   "foo()\n\n"
   "Dummy function."},*/
  {NULL}
};

PyTypeObject EnvironmentType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.Environment",			/* tp_name */
  sizeof(EnvironmentObject),		/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)Environment_dealloc,	/* tp_dealloc*/
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
  "This represents the PyGLPK environment.  Through this, one may control\n"
  "the global behavior of the GLPK.  One instance of this exists, named\n"
  ENVIRONMENT_INSTANCE_NAME " in the glpk module.",
	/* tp_doc */
  (traverseproc)Environment_traverse,	/* tp_traverse */
  (inquiry)Environment_clear,		/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(EnvironmentObject, weakreflist),	/* tp_weaklistoffset */
  0,					/* tp_iter */
  0,					/* tp_iternext */
  Environment_methods,			/* tp_methods */
  Environment_members,			/* tp_members */
  Environment_getset,			/* tp_getset */
};
