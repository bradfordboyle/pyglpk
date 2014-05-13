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

#include "util.h"
#include "kkt.h"
#include "structmember.h"

static void KKT_dealloc(KKTObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  self->ob_type->tp_free((PyObject*)self);
}

/** Create a new parameter collection object. */
KKTObject *KKT_New(void) {
  KKTObject *k = (KKTObject*)PyObject_New(KKTObject, &KKTType);
  if (k==NULL) return k;
  bzero((void*)(&(k->kkt)), sizeof(LPXKKT));
  k->weakreflist = NULL;
  return k;
}

/****************** GET-SET-ERS ***************/

static PyObject* KKT_pe_ae_row(KKTObject *self, void *closure) {
  int i=self->kkt.pe_ae_row; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_pe_re_row(KKTObject *self, void *closure) {
  int i=self->kkt.pe_re_row; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_pe_quality(KKTObject *self, void *closure) {
  return PyString_FromFormat("%c", self->kkt.pe_quality); }

static PyObject* KKT_pb_ae_ind(KKTObject *self, void *closure) {
  int i=self->kkt.pb_ae_ind; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_pb_re_ind(KKTObject *self, void *closure) {
  int i=self->kkt.pb_re_ind; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_pb_quality(KKTObject *self, void *closure) {
  return PyString_FromFormat("%c", self->kkt.pb_quality); }

static PyObject* KKT_de_ae_col(KKTObject *self, void *closure) {
  int i=self->kkt.de_ae_col; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_de_re_col(KKTObject *self, void *closure) {
  int i=self->kkt.de_re_col; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_de_quality(KKTObject *self, void *closure) {
  return PyString_FromFormat("%c", self->kkt.de_quality); }

static PyObject* KKT_db_ae_ind(KKTObject *self, void *closure) {
  int i=self->kkt.db_ae_ind; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_db_re_ind(KKTObject *self, void *closure) {
  int i=self->kkt.db_re_ind; return PyInt_FromLong(i?i-1:0); }
static PyObject* KKT_db_quality(KKTObject *self, void *closure) {
  return PyString_FromFormat("%c", self->kkt.db_quality); }

/****************** OBJECT DEFINITION *********/

int KKT_InitType(PyObject *module) {
  return util_add_type(module, &KKTType);
}

static PyMemberDef KKT_members[] = {
  {"pe_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.pe_ae_max),  RO,
   "Largest absolute error."},
  {"pe_re_max", T_DOUBLE, offsetof(KKTObject, kkt.pe_re_max),  RO,
   "Largest relative error."},

  {"pb_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.pb_ae_max),  RO,
   "Largest absolute error."},
  {"pb_re_max", T_DOUBLE, offsetof(KKTObject, kkt.pb_re_max),  RO,
   "Largest relative error."},

  {"de_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.de_ae_max),  RO,
   "Largest absolute error."},
  {"de_re_max", T_DOUBLE, offsetof(KKTObject, kkt.de_re_max),  RO,
   "Largest relative error."},

  {"db_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.db_ae_max),  RO,
   "Largest absolute error."},
  {"db_re_max", T_DOUBLE, offsetof(KKTObject, kkt.db_re_max),  RO,
   "Largest relative error."},
  {NULL}
};

static PyGetSetDef KKT_getset[] = {
  {"pe_ae_row", (getter)KKT_pe_ae_row, (setter)NULL,
   "Index of the row with the largest absolute error."},
  {"pe_re_row", (getter)KKT_pe_re_row, (setter)NULL,
   "Index of the row with the largest relative error."},
  {"pe_quality", (getter)KKT_pe_quality, (setter)NULL,
   "Character representing the quality of the primal solution.\n"
   "'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."},

  {"pb_ae_ind", (getter)KKT_pb_ae_ind, (setter)NULL,
   "Index of the variable with the largest absolute error."},
  {"pb_re_ind", (getter)KKT_pb_re_ind, (setter)NULL,
   "Index of the variable with the largest relative error."},
  {"pb_quality", (getter)KKT_pb_quality, (setter)NULL,
   "Character representing the quality of primal feasibility.\n"
   "'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."},

  {"de_ae_row", (getter)KKT_de_ae_col, (setter)NULL,
   "Index of the column with the largest absolute error."},
  {"de_re_row", (getter)KKT_de_re_col, (setter)NULL,
   "Index of the column with the largest relative error."},
  {"de_quality", (getter)KKT_de_quality, (setter)NULL,
   "Character representing the quality of the primal solution.\n"
   "'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."},

  {"db_ae_ind", (getter)KKT_db_ae_ind, (setter)NULL,
   "Index of the variable with the largest absolute error."},
  {"db_re_ind", (getter)KKT_db_re_ind, (setter)NULL,
   "Index of the variable with the largest relative error."},
  {"db_quality", (getter)KKT_db_quality, (setter)NULL,
   "Character representing the quality of primal feasibility.\n"
   "'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."},

  {NULL}
};

static PyMethodDef KKT_methods[] = {
  {NULL}
};

PyTypeObject KKTType = {
  PyObject_HEAD_INIT(NULL)
  0,					/* ob_size */
  "glpk.KKT",				/* tp_name */
  sizeof(KKTObject),			/* tp_basicsize*/
  0,					/* tp_itemsize*/
  (destructor)KKT_dealloc,		/* tp_dealloc*/
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
  "Karush-Kuhn-Tucker conditions.  This is returned from a check on\n"
  "quality of solutions.  Four types of conditions are stored here:\n"
  "- KKT.PE conditions are attributes prefixed by 'pe' measuring\n"
  "  error in the primal solution.\n"
  "- KKT.PB conditions are attributes prefixed by 'pb' measuring\n"
  "  error in satisfying primal bound constraints, i.e., feasibility.\n"
  "- KKT.DE and KKT.DB are analogous, but for the dual.",
	/* tp_doc */
  0,					/* tp_traverse */
  0,					/* tp_clear */
  0,					/* tp_richcompare */
  offsetof(KKTObject, weakreflist),	/* tp_weaklistoffset */
  0,					/* tp_iter */
  0,					/* tp_iternext */
  KKT_methods,				/* tp_methods */
  KKT_members,				/* tp_members */
  KKT_getset,				/* tp_getset */
};
