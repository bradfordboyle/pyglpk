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

#include "util.h"
#include "kkt.h"
#include "structmember.h"

static void KKT_dealloc(KKTObject *self) {
  if (self->weakreflist != NULL) {
    PyObject_ClearWeakRefs((PyObject*)self);
  }
  Py_TYPE(self)->tp_free((PyObject*)self);
}

/** Create a new parameter collection object. */
KKTObject *KKT_New(void) {
  KKTObject *k = (KKTObject*)PyObject_New(KKTObject, &KKTType);
  if (k==NULL) return k;
  memset(&k->kkt, 0, sizeof(pyglpk_kkt_t));
  k->weakreflist = NULL;
  return k;
}

static inline int quality(double re_max)
{
  int ret;
  if (re_max <= 1e-9)
    ret = 'H';
  else if (re_max <= 1e-6)
    ret = 'M';
  else if (re_max <= 1e-3)
    ret = 'L';
  else
    ret = '?';

  return ret;
}

void pyglpk_kkt_check(glp_prob *lp, int scaling, pyglpk_kkt_t *kkt)
{
#if GLPK_VERSION(4, 49)
  int m = glp_get_num_rows(lp);

  /* check primal equality constraints */
  glp_check_kkt(lp, GLP_SOL, GLP_KKT_PE,
                &(kkt->pe_ae_max), &(kkt->pe_ae_row),
                &(kkt->pe_re_max), &(kkt->pe_re_row));
  kkt->pe_quality = quality(kkt->pe_re_max);

  /* check primal bound constraints */
  glp_check_kkt(lp, GLP_SOL, GLP_KKT_PB,
                &(kkt->pb_ae_max), &(kkt->pb_ae_ind),
                &(kkt->pb_re_max), &(kkt->pb_re_ind));
  kkt->pb_quality = quality(kkt->pb_re_max);

  /* check dual equality constraints */
  glp_check_kkt(lp, GLP_SOL, GLP_KKT_DE,
                &(kkt->de_ae_max), &(kkt->de_ae_col),
                &(kkt->de_re_max), &(kkt->de_re_col));
  kkt->de_ae_col = kkt->de_ae_col == 0 ? 0 : kkt->de_ae_col - m;
  kkt->de_re_col = kkt->de_re_col == 0 ? 0 : kkt->de_re_col - m;
  kkt->de_quality = quality(kkt->de_re_max);

  /* check dual bound constraints */
  glp_check_kkt(lp, GLP_SOL, GLP_KKT_DB,
                &(kkt->db_ae_max), &(kkt->db_ae_ind),
                &(kkt->db_re_max), &(kkt->db_re_ind));
  kkt->db_quality = quality(kkt->db_re_max);
#else
  lpx_check_kkt(lp, scaling, kkt);
#endif
}

void pyglpk_int_check(glp_prob *lp, pyglpk_kkt_t *kkt)
{
#if GLPK_VERSION(4, 49)
  /* check primal equality constraints */
  glp_check_kkt(lp, GLP_MIP, GLP_KKT_PE,
                &(kkt->pe_ae_max), &(kkt->pe_ae_row),
                &(kkt->pe_re_max), &(kkt->pe_re_row));
  kkt->pe_quality = quality(kkt->pe_re_max);

  /* check primal bound constraints */
  glp_check_kkt(lp, GLP_MIP, GLP_KKT_PB,
                &(kkt->pb_ae_max), &(kkt->pb_ae_ind),
                &(kkt->pb_re_max), &(kkt->pb_re_ind));
  kkt->pb_quality = quality(kkt->pb_re_max);
#else
  lpx_check_int(lp, kkt);
#endif
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

PyDoc_STRVAR(pa_ae_max_doc, "Largest absolute error.");

PyDoc_STRVAR(pe_re_max_doc, "Largest relative error.");

PyDoc_STRVAR(pb_ae_max_doc, "Largest absolute error.");

PyDoc_STRVAR(pb_re_max_doc, "Largest relative error.");

PyDoc_STRVAR(de_ae_max_doc, "Largest absolute error.");

PyDoc_STRVAR(de_re_max_doc, "Largest relative error.");

PyDoc_STRVAR(db_ae_max_doc, "Largest absolute error.");

PyDoc_STRVAR(db_re_max_doc, "Largest relative error.");

static PyMemberDef KKT_members[] = {
  {"pe_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.pe_ae_max), READONLY,
   pa_ae_max_doc},
  {"pe_re_max", T_DOUBLE, offsetof(KKTObject, kkt.pe_re_max), READONLY,
   pe_re_max_doc},

  {"pb_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.pb_ae_max), READONLY,
   pb_ae_max_doc},
  {"pb_re_max", T_DOUBLE, offsetof(KKTObject, kkt.pb_re_max), READONLY,
   pb_re_max_doc},

  {"de_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.de_ae_max), READONLY,
   de_ae_max_doc},
  {"de_re_max", T_DOUBLE, offsetof(KKTObject, kkt.de_re_max), READONLY,
   de_re_max_doc},

  {"db_ae_max", T_DOUBLE, offsetof(KKTObject, kkt.db_ae_max), READONLY,
   db_ae_max_doc},
  {"db_re_max", T_DOUBLE, offsetof(KKTObject, kkt.db_re_max), READONLY,
   db_re_max_doc},
  {NULL}
};

PyDoc_STRVAR(pe_ae_row_doc,
"Index of the row with the largest absolute error."
);

PyDoc_STRVAR(pe_re_row_doc,
"Index of the row with the largest relative error."
);

PyDoc_STRVAR(pe_quality_doc,
"Character representing the quality of the primal solution.\n"
   "'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."
);

PyDoc_STRVAR(pb_ae_ind_doc,
"Index of the variable with the largest absolute error."
);

PyDoc_STRVAR(pb_re_ind_doc,
"Index of the variable with the largest relative error."
);

PyDoc_STRVAR(pb_quality_doc,
"Character representing the quality of primal feasibility.\n"
"'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."
);

PyDoc_STRVAR(de_ae_row_doc,
"Index of the column with the largest absolute error."
);

PyDoc_STRVAR(de_re_row_doc,
"Index of the column with the largest relative error."
);

PyDoc_STRVAR(de_quality_doc,
"Character representing the quality of the primal solution.\n"
"'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."
);

PyDoc_STRVAR(db_ae_ind_doc,
"Index of the variable with the largest absolute error."
);

PyDoc_STRVAR(db_re_ind_doc,
"Index of the variable with the largest relative error."
);

PyDoc_STRVAR(db_quality_doc,
"Character representing the quality of primal feasibility.\n"
"'H', high, 'M', medium, 'L', low, or '?' wrong or infeasible."
);

static PyGetSetDef KKT_getset[] = {
  {"pe_ae_row", (getter)KKT_pe_ae_row, (setter)NULL, pe_ae_row_doc},
  {"pe_re_row", (getter)KKT_pe_re_row, (setter)NULL, pe_re_row_doc},
  {"pe_quality", (getter)KKT_pe_quality, (setter)NULL, pe_quality_doc},

  {"pb_ae_ind", (getter)KKT_pb_ae_ind, (setter)NULL, pb_ae_ind_doc},
  {"pb_re_ind", (getter)KKT_pb_re_ind, (setter)NULL, pb_re_ind_doc},
  {"pb_quality", (getter)KKT_pb_quality, (setter)NULL, pb_quality_doc},

  {"de_ae_row", (getter)KKT_de_ae_col, (setter)NULL, de_ae_row_doc},
  {"de_re_row", (getter)KKT_de_re_col, (setter)NULL, de_re_row_doc},
  {"de_quality", (getter)KKT_de_quality, (setter)NULL, de_quality_doc},

  {"db_ae_ind", (getter)KKT_db_ae_ind, (setter)NULL, db_ae_ind_doc},
  {"db_re_ind", (getter)KKT_db_re_ind, (setter)NULL, db_re_ind_doc},
  {"db_quality", (getter)KKT_db_quality, (setter)NULL, db_quality_doc},

  {NULL}
};

static PyMethodDef KKT_methods[] = {
  {NULL}
};

PyDoc_STRVAR(kkt_doc,
"Karush-Kuhn-Tucker conditions.\n"
"\n"
"This is returned from a check on quality of solutions. Four types of "
"conditions are stored here:\n"
"\n"
"- KKT.PE conditions are attributes prefixed by 'pe' measuring error in the\n"
"  primal solution.\n"
"- KKT.PB conditions are attributes prefixed by 'pb' measuring error in\n"
"  satisfying primal bound constraints, i.e., feasibility.\n"
"- KKT.DE and KKT.DB are analogous, but for the dual."
);

PyTypeObject KKTType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name           = "glpk.KKT",
    .tp_basicsize      = sizeof(KKTObject),
    .tp_dealloc        = (destructor) KKT_dealloc,
    .tp_flags          = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_doc            = kkt_doc,
    .tp_weaklistoffset = offsetof(KKTObject, weakreflist),
    .tp_methods        = KKT_methods,
    .tp_members        = KKT_members,
    .tp_getset         = KKT_getset,
};
