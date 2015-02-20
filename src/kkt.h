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

#ifndef _KKT_H
#define _KKT_H

#include <Python.h>
#include "lp.h"
#include "util.h"

#define KKT_Check(op) PyObject_TypeCheck(op, &KKTType)

#if GLPK_VERSION(4, 49)
typedef struct pyglpk_kkt {
  /* primal equality constraints */
  double pe_ae_max;
  int    pe_ae_row;
  double pe_re_max;
  int    pe_re_row;
  int    pe_quality;

  /* primal bound constraints */
  double pb_ae_max;
  int    pb_ae_ind;
  double pb_re_max;
  int    pb_re_ind;
  int    pb_quality;

  /* dual equality constraints */
  double de_ae_max;
  int    de_ae_col;
  double de_re_max;
  int    de_re_col;
  int    de_quality;

  /* dual bound constraints */
  double db_ae_max;
  int    db_ae_ind;
  double db_re_max;
  int    db_re_ind;
  int    db_quality;
} pyglpk_kkt_t;
#else
typedef struct LPXKKT pyglpk_kkt_t;
#endif

typedef struct {
  PyObject_HEAD
  pyglpk_kkt_t kkt;
  PyObject *weakreflist; // Weak reference list.
} KKTObject;

extern PyTypeObject KKTType;

/* Returns a new KKT object. */
KKTObject *KKT_New(void);

/* Init the type and related types it contains. 0 on success. */
int KKT_InitType(PyObject *module);

void pyglpk_kkt_check(glp_prob *, int, pyglpk_kkt_t *);
void pyglpk_int_check(glp_prob *, pyglpk_kkt_t *);

#endif // _PARAMS_H
