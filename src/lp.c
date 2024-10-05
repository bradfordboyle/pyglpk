/**************************************************************************
Copyright (C) 2007, 2008 Thomas Finley, tfinley@gmail.com

This file is part of PyGLPK.

PyGLPK is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

PyGLPK is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with PyGLPK. If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <Python.h>
#include <glpk.h>
#include "2to3.h"
#include "lp.h"
#include "structmember.h"
#include "bar.h"
#include "barcol.h"
#include "obj.h"
#include "kkt.h"
#include "util.h"
#include "tree.h"

#ifdef USEPARAMS
#include "params.h"
#endif

#define LP (self->lp)

static PyObject* glpstatus2string(int);
static int unzip(PyObject *, const int, BarObject *[], double []);

static int LPX_traverse(LPXObject *self, visitproc visit, void *arg)
{
	Py_VISIT(self->rows);
	Py_VISIT(self->cols);
	Py_VISIT(self->obj);
#ifdef USEPARAMS
	Py_VISIT(self->params);
#endif
	return 0;
}

static int LPX_clear(LPXObject *self)
{
	if (self->weakreflist != NULL) {
		PyObject_ClearWeakRefs((PyObject*)self);
	}
	Py_CLEAR(self->rows);
	Py_CLEAR(self->cols);
	Py_CLEAR(self->obj);
#ifdef USEPARAMS
	Py_CLEAR(self->params);
#endif
	return 0;
}

static void LPX_dealloc(LPXObject *self)
{
	LPX_clear(self);
	if (LP) glp_delete_prob(LP);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

LPXObject* LPX_FromLP(glp_prob*lp)
{
	LPXObject *lpx = (LPXObject*)PyObject_GC_New(LPXObject, &LPXType);
	if (lpx == NULL)
		return lpx;
	// Start out with null.
	lpx->cols = lpx->rows = NULL;
	lpx->obj = NULL;
	lpx->params = NULL;
	// Try assigning the values.
	if ((lpx->cols = (PyObject*)BarCol_New(lpx, 0)) == NULL || (lpx->rows = (PyObject*)BarCol_New(lpx, 1)) == NULL ||
#ifdef USEPARAMS
			(lpx->params = (PyObject*)Params_New(lpx))==NULL ||
#endif
			(lpx->obj = (PyObject*)Obj_New(lpx))==NULL) {
		Py_DECREF(lpx);
		return NULL;
	}
	// Finally assign the LP and return the structure.
	lpx->lp = lp;
	return lpx;
}

static PyObject * LPX_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	LPXObject *self;
	self = (LPXObject*) type->tp_alloc(type, 0);
	if (self != NULL) {
		self->lp = NULL;

		self->rows = NULL;
		self->cols = NULL;
		self->obj = NULL;
#ifdef USEPARAMS
		self->params = NULL;
#endif
		self->weakreflist = NULL;
		self->last_solver = -1;
	}
	return (PyObject*)self;
}

static int LPX_init(LPXObject *self, PyObject *args, PyObject *kwds)
{
	char *mps_n=NULL, *freemps_n=NULL, *cpxlp_n=NULL, *glp_n=NULL;
	char *model[] = {NULL,NULL,NULL};
	PyObject *model_obj = NULL, *so = NULL;
	static char *kwlist[] = {"gmp","mps","freemps","cpxlp", "glp", NULL};
	Py_ssize_t numargs = 0, model_size = 0;
	int failure = 0, i;
	glp_tran *tran;

	numargs += args ? PyTuple_Size(args) : 0;
	numargs += kwds ? PyDict_Size(kwds) : 0;
	if (numargs>1) {
		PyErr_SetString(PyExc_TypeError, "cannot specify multiple data sources");
		return -1;
	}

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Ossss", kwlist, &model_obj, &mps_n, &freemps_n, &cpxlp_n, &glp_n)) {
		return -1;
	}

	if (model_obj && PyString_Check(model_obj)) {
		model[0] = PyString_AsString(model_obj);
		if (!model[0])
			return -1;
	} else if (model_obj && PyTuple_Check(model_obj)) {
		model_size = PyTuple_Size(model_obj);
		if (model_size < -1)
			return -1;
		if (model_size < 1 || model_size >  3) {
			PyErr_SetString(PyExc_ValueError, "model tuple must have 1<=length<=3");
			return -1;
		}
		for (i = 0; i < model_size; i++) {
			so = PyTuple_GET_ITEM(model_obj,i);
			if (so == Py_None)
				continue;
			model[i] = PyString_AsString(so);
			if (model[i] == NULL)
				return -1;
		}
	} else if (model_obj) {
		PyErr_SetString(PyExc_TypeError, "model arg must be string or tuple");
		return -1;
	}

	// start by create an empty problem
	self->lp = glp_create_prob();
	// Some of these are pretty straightforward data reading routines.
	if (mps_n) {
		failure = glp_read_mps(self->lp, GLP_MPS_DECK, NULL, mps_n);
		if (failure)
			PyErr_SetString(PyExc_RuntimeError, "MPS reader failed");
	} else if (freemps_n) {
		failure = glp_read_mps(self->lp, GLP_MPS_FILE, NULL, freemps_n);
		if (failure)
			PyErr_SetString(PyExc_RuntimeError, "Free MPS reader failed");
	} else if (cpxlp_n) {
		failure = glp_read_lp(self->lp, NULL, cpxlp_n);
		if (failure)
			PyErr_SetString(PyExc_RuntimeError, "CPLEX LP reader failed");
	} else if (glp_n) {
		failure = glp_read_prob(self->lp, 0, glp_n);
		if (failure)
			PyErr_SetString(PyExc_RuntimeError, "GLPK LP/MIP reader failed");
	} else if (model_obj) {
		/* allocate the translator workspace */
		tran = glp_mpl_alloc_wksp();

		/* read model section and optional data section */
		failure = glp_mpl_read_model(tran, model[0], model[1] != NULL);
		if (failure)
		        PyErr_SetString(PyExc_RuntimeError, "GMP model reader failed");

		/* read separate data section, if required */
		if (!failure && model[1] && (failure = glp_mpl_read_data(tran, model[1])))
		        PyErr_SetString(PyExc_RuntimeError, "GMP data reader failed");

		/* generate the model */
		if (!failure && (failure = glp_mpl_generate(tran, model[2])))
		        PyErr_SetString(PyExc_RuntimeError, "GMP generator failed");

		/* build the problem instance from the model */
		if (!failure)
		        glp_mpl_build_prob(tran, self->lp);

		/* free the translator workspace */
		glp_mpl_free_wksp(tran);
	}
	// Any of the methods above may have failed, so the LP would be null.
	if (failure) {
                glp_delete_prob(self->lp);
                self->lp = NULL;
                return -1;
        }

	// Create those rows and cols and things.
	self->cols = (PyObject*)BarCol_New(self, 0);
	self->rows = (PyObject*)BarCol_New(self, 1);
	self->obj = (PyObject*)Obj_New(self);
#ifdef USEPARAMS
	self->params = (PyObject*)Params_New(self);
#endif
	return 0;
}

static PyObject* LPX_Str(LPXObject *self)
{
	// Returns a string representation of this object.
	return PyString_FromFormat("<%s %d-by-%d at %p>",
			Py_TYPE(self)->tp_name, glp_get_num_rows(LP), glp_get_num_cols(LP), self);
}

PyObject *LPX_GetMatrix(LPXObject *self)
{
	int row, numrows, listi, i, nnz, rownz;
	PyObject *retval;

	numrows = glp_get_num_rows(LP);
	nnz = glp_get_num_nz(LP);
	retval = PyList_New(nnz);
	if (nnz == 0 || retval == NULL)
		return retval;

	// We don't really need this much memory, but, eh...
	int *ind = (int*)calloc(nnz, sizeof(int));
	double *val = (double*)calloc(nnz, sizeof(double));

	listi = 0;
	for (row=1; row<=numrows; ++row) {
		rownz = glp_get_mat_row(LP, row, ind-1, val-1);
		if (rownz == 0)
			continue;
		for (i = 0; i < rownz; ++i) {
			PyList_SET_ITEM(retval, listi++, Py_BuildValue("iid", row - 1, ind[i] - 1, val[i]));
		}
		/*
		 * Continue to downscale these vectors, freeing memory in C even
		 * as we use more memory in Python.
		 */
		nnz -= rownz;
		if (nnz) {
			ind = (int*)realloc(ind, nnz*sizeof(int));
			val = (double*)realloc(val, nnz*sizeof(double));
		}
	}
	free(ind);
	free(val);
	if (PyList_Sort(retval)) {
		Py_DECREF(retval);
		return NULL;
	}
		return retval;
}

int LPX_SetMatrix(LPXObject *self, PyObject *newvals)
{
	int len, *ind1 = NULL, *ind2 = NULL;
	double *val = NULL;
	// Now, attempt to convert the input item.
	if (newvals == NULL || newvals == Py_None)
		len = 0;
	else if (!util_extract_iif(newvals, (PyObject*)self, &len, &ind1, &ind2, &val))
		return -1;

	// Input the stuff into the LP constraint matrix.
	glp_load_matrix(LP, len, ind1-1, ind2-1, val-1);
	// Free the memory.
	if (len) {
		free(ind1);
		free(ind2);
		free(val);
	}
	return 0;
}

/****************** METHODS ***************/

/*static PyObject* LPX_OrderMatrix(LPXObject *self) {
  glp_order_matrix(LP);
  Py_RETURN_NONE;
  }*/

static PyObject* LPX_Erase(LPXObject *self)
{
	glp_erase_prob(LP);
	Py_RETURN_NONE;
}

static PyObject* LPX_Copy(LPXObject *self, PyObject *args)
{
	int names = GLP_OFF;
	if (!PyArg_ParseTuple(args, "|i", &names))
		return NULL;
	glp_prob *dest = glp_create_prob();
	glp_copy_prob(dest, LP, names);

	return (PyObject *) LPX_FromLP(dest);
}

static PyObject* LPX_Scale(LPXObject *self, PyObject*args)
{
	int flags = GLP_SF_AUTO;
	PyArg_ParseTuple(args, "|i", &flags);
	glp_scale_prob(LP, flags);
	Py_RETURN_NONE;
}

static PyObject* LPX_Unscale(LPXObject *self)
{
	glp_unscale_prob(LP);
	Py_RETURN_NONE;
}

static PyObject* LPX_basis_std(LPXObject *self)
{
	glp_std_basis(LP);
	Py_RETURN_NONE;
}

static PyObject* LPX_basis_adv(LPXObject *self)
{
	glp_adv_basis(LP, 0);
	Py_RETURN_NONE;
}

static PyObject* LPX_basis_cpx(LPXObject *self)
{
	glp_cpx_basis(LP);
	Py_RETURN_NONE;
}

/**************** SOLVER METHODS **************/

static PyObject* glpsolver_retval_to_message(int retval)
{
	const char* returnval = NULL;
	switch (retval) {
	case 0:
		Py_RETURN_NONE;
	case GLP_EBADB:
		returnval = "badb";
		break;
	case GLP_ESING:
		returnval = "sing"; break;
	case GLP_ECOND:
		returnval = "cond"; break;
	case GLP_EBOUND:
		returnval = "bound"; break;
	case GLP_EFAIL:
		returnval = "fail"; break;
	case GLP_EOBJLL:
		returnval = "objll"; break;
	case GLP_EOBJUL:
		returnval = "objul"; break;
	case GLP_EITLIM:
		returnval = "itlim"; break;
	case GLP_ETMLIM:
		returnval = "tmlim"; break;
	case GLP_ENOPFS:
		returnval = "nopfs"; break;
	case GLP_ENODFS:
		returnval = "nodfs"; break;
	case GLP_EROOT:
		returnval = "root"; break;
	case GLP_ESTOP:
		returnval = "stop"; break;
	case GLP_ENOCVG:
		returnval = "nocvg"; break;
	case GLP_EINSTAB:
		returnval = "instab"; break;
	default:
		returnval = "unknown?"; break;
	}
	return PyString_FromString(returnval);
}

static PyObject* LPX_solver_simplex(LPXObject *self, PyObject *args,
				    PyObject *keywds)
{
	glp_smcp cp;
	/*
	 * Set all to GLPK defaults, except for the message level, which
	 * inexplicably has a default "verbose" setting.
	 */
	glp_init_smcp(&cp);
	cp.msg_lev = GLP_MSG_OFF;
	// Map the keyword arguments to the appropriate entries.
	static char *kwlist[] = {"msg_lev", "meth", "pricing", "r_test",
		"tol_bnd", "tol_dj", "tol_piv", "obj_ll", "obj_ul", "it_lim",
		"tm_lim", "out_frq", "out_dly", "presolve", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, keywds, "|iiiidddddiiiii",
				kwlist, &cp.msg_lev, &cp.meth, &cp.pricing,
				&cp.r_test, &cp.tol_bnd, &cp.tol_dj,
				&cp.tol_piv, &cp.obj_ll, &cp.obj_ul, &cp.it_lim,
				&cp.tm_lim, &cp.out_frq, &cp.out_dly,
				&cp.presolve))
		return NULL;
	cp.presolve = cp.presolve ? GLP_ON : GLP_OFF;
	// Do checking on the various entries.
	switch (cp.msg_lev) {
	case GLP_MSG_OFF:
	case GLP_MSG_ERR:
	case GLP_MSG_ON:
	case GLP_MSG_ALL:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for msg_lev (LPX.MSG_* are valid values)");
		return NULL;
	}
	switch (cp.meth) {
	case GLP_PRIMAL:
	case GLP_DUALP:
		break;
	case GLP_DUAL:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for meth (LPX.PRIMAL, LPX.DUAL, LPX.DUALP valid values)");
		return NULL;
	}
	switch (cp.pricing) {
	case GLP_PT_STD:
	case GLP_PT_PSE:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for pricing (LPX.PT_STD, LPX.PT_PSE valid values)");
		return NULL;
	}
	switch (cp.r_test) {
		case GLP_RT_STD:
		case GLP_RT_HAR:
			break;
		default:
			PyErr_SetString(PyExc_ValueError, "invalid value for ratio test (LPX.RT_STD, LPX.RT_HAR valid values)");
			return NULL;
	}
	if (cp.tol_bnd <= 0 || cp.tol_bnd >= 1) {
		PyErr_SetString(PyExc_ValueError, "tol_bnd must obey 0<tol_bnd<1");
		return NULL;
	}
	if (cp.tol_dj <= 0 || cp.tol_dj >= 1) {
		PyErr_SetString(PyExc_ValueError, "tol_dj must obey 0<tol_dj<1");
		return NULL;
	}
	if (cp.tol_piv <= 0 || cp.tol_piv >= 1) {
		PyErr_SetString(PyExc_ValueError, "tol_piv must obey 0<tol_piv<1");
		return NULL;
	}
	if (cp.it_lim < 0) {
		PyErr_SetString(PyExc_ValueError, "it_lim must be non-negative");
		return NULL;
	}
	if (cp.tm_lim < 0) {
		PyErr_SetString(PyExc_ValueError, "tm_lim must be non-negative");
		return NULL;
	}
	if (cp.out_frq <= 0) {
		PyErr_SetString(PyExc_ValueError, "out_frq must be positive");
		return NULL;
	}
	if (cp.out_dly < 0) {
		PyErr_SetString(PyExc_ValueError, "out_dly must be non-negative");
		return NULL;
	}
	// All the checks are complete. Call the simplex solver.
	int retval = glp_simplex(LP, &cp);
	if (retval != GLP_EBADB && retval != GLP_ESING && retval != GLP_ECOND && retval != GLP_EBOUND && retval != GLP_EFAIL)
		self->last_solver = 0;
	return glpsolver_retval_to_message(retval);
}

static PyObject* LPX_solver_exact(LPXObject *self)
{
	int retval;
	glp_smcp parm;

        //TODO: add kwargs for smcp
	glp_init_smcp(&parm);
	retval = glp_exact(LP, &parm);
	if (!retval)
		self->last_solver = 0;
	return glpsolver_retval_to_message(retval);
}

static PyObject* LPX_solver_interior(LPXObject *self) {
	int retval = glp_interior(LP, NULL);
	if (!retval)
		self->last_solver = 1;
	return glpsolver_retval_to_message(retval);
}

struct mip_callback_object {
	PyObject *callback;
	LPXObject *py_lp;
};

static void mip_callback(glp_tree *tree, void *info)
{
	struct mip_callback_object *obj = (struct mip_callback_object *)info;
	PyObject *method_name = NULL;
	// Choose the method name for the callback object that is appropriate.
	switch (glp_ios_reason(tree)) {
	case GLP_ISELECT:
		method_name = PyString_FromString("select");
		break;
	case GLP_IPREPRO:
		method_name = PyString_FromString("prepro");
		break;
	case GLP_IBRANCH:
		method_name = PyString_FromString("branch");
		break;
	case GLP_IROWGEN:
		method_name = PyString_FromString("rowgen");
		break;
	case GLP_IHEUR:
		method_name = PyString_FromString("heur");
		break;
	case GLP_ICUTGEN:
		method_name = PyString_FromString("cutgen");
		break;
	case GLP_IBINGO:
		method_name = PyString_FromString("bingo");
		break;
	default:
		// This should never happen.
		PyErr_SetString(PyExc_RuntimeError, "unrecognized reason for callback");
		glp_ios_terminate(tree);
		return;
	}
	// If there is no method with that name.
	if (!PyObject_HasAttr(obj->callback, method_name)) {
		Py_DECREF(method_name);
		method_name=PyString_FromString("default");
		if (!PyObject_HasAttr(obj->callback, method_name)) {
			Py_DECREF(method_name);
			method_name = NULL;
			return;
		}
	}
	// Try calling the method.
	TreeObject *py_tree = Tree_New(tree, obj->py_lp);
	if (py_tree == NULL) {
		Py_DECREF(method_name);
		glp_ios_terminate(tree);
		return;
	}
	PyObject *retval = NULL;
	retval = PyObject_CallMethodObjArgs(obj->callback, method_name, py_tree, NULL);
	py_tree->tree = NULL; // Invalidate the Tree object.
	Py_DECREF(py_tree);
	Py_XDECREF(method_name);
	if (retval == NULL) {
		/* This could have failed for any number of reasons. Perhaps the
		 * code within the method failed, perhaps the method does not
		 * accept the tree argument, or perhaps the 'method' is not
		 * really even a callable method at all.
		 */
		glp_ios_terminate(tree);
		return;
	}
	Py_DECREF(retval);
}

static PyObject* LPX_solver_integer(LPXObject *self, PyObject *args,
		PyObject *keywds)
{
	PyObject *callback = NULL;
	struct mip_callback_object*info = NULL;
	glp_iocp cp;
	glp_init_iocp(&cp);
	cp.msg_lev = GLP_MSG_OFF;
	// Map the keyword arguments to the appropriate entries.
	static char *kwlist[] = {"msg_lev", // int
		"br_tech", 		// int
		"bt_tech", 		// int
		"pp_tech", 		// int
#if GLP_MAJOR_VERSION >= 4 && GLP_MINOR_VERSION >= 57
		"sr_heur",		// int
#endif
		"fp_heur", 		// int
		"ps_heur", 		// int
		"ps_tm_lim",	// int
		"gmi_cuts", 	// int
		"mir_cuts", 	// int
		"cov_cuts", 	// int
		"clq_cuts",		// int
		"tol_int",		// double
		"tol_obj", 		// double 
		"mip_gap", 		// double
		"tm_lim",  		// int
		"out_frq", 		// int
		"out_dly", 		// int
		"callback", 	// void 
		//"cb_info", "cb_size",
		"presolve", 	// int
		"binarize", 	// int
		NULL};
	if (!PyArg_ParseTupleAndKeywords(args, keywds, 
#if GLP_MAJOR_VERSION >=4 && GLP_MINOR_VERSION >= 57
	"|iiiiiiiiiiiidddiiiOii",
#else
	"|iiiiiiiiiiidddiiiOii",
#endif
			kwlist,
			&cp.msg_lev, 
			&cp.br_tech, 
			&cp.bt_tech,
			&cp.pp_tech,
#if GLP_MAJOR_VERSION >= 4 && GLP_MINOR_VERSION >= 57
			&cp.sr_heur,
#endif
			&cp.fp_heur,
			&cp.ps_heur,
			&cp.ps_tm_lim,
			&cp.gmi_cuts,
			&cp.mir_cuts,
			&cp.cov_cuts,
			&cp.clq_cuts,
			&cp.tol_int, 
			&cp.tol_obj, 
			&cp.mip_gap,
			&cp.tm_lim,
			&cp.out_frq, 
			&cp.out_dly, 
			&callback,
			&cp.presolve,
			&cp.binarize)) {
		return NULL;
	}

	// Convert on/off parameters. 
#if GLP_MAJOR_VERSION >= 4 && GLP_MINOR_VERSION >= 57
	cp.sr_heur = cp.sr_heur ? GLP_ON : GLP_OFF;
#endif
	cp.fp_heur = cp.fp_heur ? GLP_ON : GLP_OFF;
	cp.ps_heur = cp.ps_heur ? GLP_ON : GLP_OFF;
	cp.gmi_cuts = cp.gmi_cuts ? GLP_ON : GLP_OFF;
	cp.mir_cuts = cp.mir_cuts ? GLP_ON : GLP_OFF;
	cp.cov_cuts = cp.cov_cuts ? GLP_ON : GLP_OFF;
	cp.clq_cuts = cp.clq_cuts ? GLP_ON : GLP_OFF;
	cp.presolve = cp.presolve ? GLP_ON : GLP_OFF;
	cp.binarize = cp.binarize ? GLP_ON : GLP_OFF;

	if ((cp.presolve == GLP_OFF) && (glp_get_status(LP) != GLP_OPT)) {
		PyErr_SetString(PyExc_RuntimeError, "integer solver without presolve requires existing optimal basic solution");
		return NULL;
	}

	// Do checking on the various entries.
	switch (cp.msg_lev) {
	case GLP_MSG_OFF:
	case GLP_MSG_ERR:
	case GLP_MSG_ON:
	case GLP_MSG_ALL:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for msg_lev (LPX.MSG_* are valid values)");
	    	return NULL;
	}
	switch (cp.br_tech) {
	case GLP_BR_FFV:
	case GLP_BR_LFV:
	case GLP_BR_MFV:
	case GLP_BR_DTH:
	case GLP_BR_PCH:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for br_tech (LPX.BR_* are valid values)");
		return NULL;
	}
	switch (cp.bt_tech) {
	case GLP_BT_DFS:
	case GLP_BT_BFS:
	case GLP_BT_BLB:
	case GLP_BT_BPH:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for bt_tech (LPX.BT_* are valid values)");
		return NULL;
	}
	switch (cp.pp_tech) {
	case GLP_PP_NONE:
	case GLP_PP_ROOT:
	case GLP_PP_ALL:
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "invalid value for pp_tech (LPX.PP_* are valid values)");
		return NULL;
	}

	if (cp.ps_tm_lim < 0) {
		PyErr_SetString(PyExc_ValueError, "ps_tm_lim must be nonnegative");
		return NULL;
	}
	if (cp.tol_int <= 0 || cp.tol_int >= 1) {
		PyErr_SetString(PyExc_ValueError, "tol_int must obey 0<tol_int<1");
		return NULL;
	}
	if (cp.tol_obj <= 0 || cp.tol_obj >= 1) {
		PyErr_SetString(PyExc_ValueError, "tol_obj must obey 0<tol_obj<1");
		return NULL;
	}
	if (cp.mip_gap < 0) {
		PyErr_SetString(PyExc_ValueError, "mip_gap must be non-negative");
		return NULL;
	}
	if (cp.tm_lim < 0) {
		PyErr_SetString(PyExc_ValueError, "tm_lim must be non-negative");
		return NULL;
	}
	if (cp.out_frq <= 0) {
		PyErr_SetString(PyExc_ValueError, "out_frq must be positive");
		return NULL;
	}
	if (cp.out_dly<0) {
		PyErr_SetString(PyExc_ValueError, "out_dly must be non-negative");
		return NULL;
	}

	int retval;
	if (callback != NULL && callback != Py_None) {
		info = (struct mip_callback_object*)
			malloc(sizeof(struct mip_callback_object));
		info->callback = callback;
		info->py_lp = self;
		cp.cb_info = info;
		cp.cb_func = mip_callback;
	}
	retval = glp_intopt(LP, &cp);
	if (info)
		free(info);
	if (PyErr_Occurred()) {
		/* This should happen only if there was a problem within the
		 * callback function, or if the callback was not appropriate.
		 */
		return NULL;
	}
	if (retval!=GLP_EBADB && retval!=GLP_ESING && retval!=GLP_ECOND && retval!=GLP_EBOUND && retval!=GLP_EFAIL)
		self->last_solver = 2;
	return glpsolver_retval_to_message(retval);
}

static PyObject* LPX_solver_intopt(LPXObject *self)
{
        int retval;
        glp_iocp parm;

        //TODO: add kwargs for iocp
        glp_init_iocp(&parm);
	retval = glp_intopt(LP, &parm);
	if (!retval)
		self->last_solver = 2;
	return glpsolver_retval_to_message(retval);
}

static KKTObject* LPX_kkt(LPXObject *self, PyObject *args)
{
	// Cannot get for undefined primal or dual.
	if (glp_get_prim_stat(LP) == GLP_UNDEF || glp_get_dual_stat(LP) == GLP_UNDEF) {
		PyErr_SetString(PyExc_RuntimeError, "cannot get KKT when primal or dual basic solution undefined");
		return NULL;
	}
	// Check the Python arguments.
	int scaling = 0;
	PyObject *arg = NULL;
	if (!PyArg_ParseTuple(args, "|O", &arg))
		return NULL;
	scaling = ((arg == NULL) ? 0 : PyObject_IsTrue(arg));
	if (scaling == -1)
		return NULL;
	// OK, all done with those checks. Now get the KKT condition.
	KKTObject *kkt = KKT_New();
	if (!kkt)
		return NULL;
	pyglpk_kkt_check(LP, scaling, &(kkt->kkt));
	return kkt;
}

static KKTObject* LPX_kktint(LPXObject *self) {
	KKTObject *kkt = KKT_New();
	if (!kkt)
		return NULL;
	pyglpk_int_check(LP, &(kkt->kkt));
	return kkt;
}

static  PyObject* LPX_warm_up(LPXObject *self) {
	int retval;
	retval = glp_warm_up(LP);
	return glpsolver_retval_to_message(retval);
}

PyObject* convert_and_zip(LPXObject *self, const int len, const int ind[], const double val[]) {
	PyObject *retval;
	int m, n;

	if ((retval = PyList_New(len)) == NULL) {
		return NULL;
	}

	m = glp_get_num_rows(LP);
	n = glp_get_num_cols(LP);
	for (int i = 1; i <= len; i++) {
		BarObject *bar;
		if (1 <= ind[i] && ind[i] <= m) {
			bar = Bar_New((BarColObject *) self->rows, ind[i] - 1);
		} else if (m+1 <= ind[i] && ind[i] <= m+n) {
			bar = Bar_New((BarColObject *) self->cols, ind[i] - m - 1);
		}
		// TODO what if ind[i] outside bounds?
		PyList_SET_ITEM(retval, i - 1, Py_BuildValue("(Od)", bar, val[i]));
	}

	return retval;
}

static PyObject* LPX_transform_row(LPXObject *self, PyObject *arg) {
	int k, len, m, n;
	PyObject *iterator, *retval;

	if ((iterator = PyObject_GetIter(arg)) == NULL) {
		return NULL;
	}

	m = glp_get_num_rows(LP);
	n = glp_get_num_cols(LP);

	BarObject **vars = (BarObject**)calloc(n + 1, sizeof(BarObject*));
	int *ind = (int*)calloc(n + 1, sizeof(int));
	double *val = (double*)calloc(n + 1, sizeof(double));
	len = unzip(iterator, n+1, vars, val);
	Py_DECREF(iterator);
	if (PyErr_Occurred()) {
		retval = NULL;
		goto exit;
	}
	for (int i = 1; i <= len; i++) {
		// check that variable is part of this LP instance
		if (vars[i]->py_bc->py_lp != self) {
			PyErr_SetString(PyExc_ValueError, "variable not associated with this LPX");
			retval = NULL;
			goto exit;
		}
		// check that variable is structural
		if (Bar_Row(vars[i])) {
			PyErr_SetString(PyExc_ValueError, "input variables must be structural");
			retval = NULL;
			goto exit;
		}
		ind[i] = Bar_Index(vars[i]) + 1;
	}

	len = glp_transform_row(LP, len, ind, val);

	retval = convert_and_zip(self, len, ind, val);

	exit: free(val);
	free(ind);

	return retval;
}

static PyObject* LPX_transform_col(LPXObject *self, PyObject *arg) {
	int k, len, i, m, n;
	PyObject *iterator, *retval;

	if ((iterator = PyObject_GetIter(arg)) == NULL) {
		return NULL;
	}

	m = glp_get_num_rows(LP);
	n = glp_get_num_cols(LP);

	BarObject **vars = (BarObject**)calloc(m + 1, sizeof(BarObject*));
	int *ind = (int*)calloc(m + 1, sizeof(int));
	double *val = (double*)calloc(m + 1, sizeof(double));

	len = unzip(iterator, m + 1, vars, val);
	Py_DECREF(iterator);
	if (PyErr_Occurred()) {
		retval = NULL;
		goto exit;
	}
	for (int i = 1; i <= len; i++) {
		if (vars[i]->py_bc->py_lp != self) {
			PyErr_SetString(PyExc_ValueError, "variable not associated with this LPX");
			retval = NULL;
			goto exit;
		}
		// check that variable is auxiliary
		if (!Bar_Row(vars[i])) {
			PyErr_SetString(PyExc_ValueError, "input variables must be auxiliary");
			retval = NULL;
			goto exit;
		}
		ind[i] = Bar_Index(vars[i]) + 1;
	}

	len = glp_transform_col(LP, len, ind, val);
	retval = convert_and_zip(self, len, ind, val);

	exit: free(val);
	free(ind);

	return retval;
}

static PyObject* LPX_prim_rtest(LPXObject *self, PyObject *args) {
	int dir, *ind, len, m, piv;
	double eps, *val;
	PyObject *iterator, *lo;

	if (!PyArg_ParseTuple(args, "O!id", &PyList_Type, &lo, &dir, &eps)) {
		return NULL;
	}

	if (dir != 1 && dir != -1) {
		PyErr_SetString(PyExc_ValueError, "direction must be either +1 (increasing) or -1 (decreasing)");
		return NULL;
	}

	if ((iterator = PyObject_GetIter(lo)) == NULL) {
		return NULL;
	}

	m = glp_get_num_rows(LP);

	BarObject **vars = (BarObject**)calloc(m + 1, sizeof(BarObject*));
	ind = (int *)calloc(m + 1, sizeof(int));
	val = (double *)calloc(m + 1, sizeof(double));
	len = unzip(iterator, m + 1, vars, val);
	Py_DECREF(iterator);
	if (PyErr_Occurred()) {
		return NULL;
	}

	for (int i = 1; i <= len; i++) {
		ind[i] = Bar_Index(vars[i]) + 1 + (Bar_Row(vars[i]) ? 0 : m);
	}

	piv = glp_prim_rtest(LP, len, ind, val, dir, eps);
	free(val);
	free(ind);

	return PyInt_FromLong((long) piv-1);
}

static PyObject* LPX_dual_rtest(LPXObject *self, PyObject *args) {
	int dir, *ind, len, m, n, piv;
	double eps, *val;
	PyObject *iterator, *lo;

	if (!PyArg_ParseTuple(args, "O!id", &PyList_Type, &lo, &dir, &eps)) {
		return NULL;
	}

	if (dir != 1 && dir != -1) {
		PyErr_SetString(PyExc_ValueError, "direction must be either +1 (increasing) or -1 (decreasing)");
		return NULL;
	}

	if ((iterator = PyObject_GetIter(lo)) == NULL) {
		return NULL;
	}

	m = glp_get_num_rows(LP);
	n = glp_get_num_cols(LP);

	BarObject **vars = (BarObject**)calloc(n + 1, sizeof(BarObject*));
	ind = (int *)calloc(n + 1, sizeof(int));
	val = (double *)calloc(n + 1, sizeof(double));
	len = unzip(iterator, n + 1, vars, val);
	Py_DECREF(iterator);
	if (PyErr_Occurred()) {
		return NULL;
	}

	for (int i = 1; i <= len; i++) {
		ind[i] = Bar_Index(vars[i]) + 1 + (Bar_Row(vars[i]) ? 0 : m);
	}

	piv = glp_dual_rtest(LP, len, ind, val, dir, eps);
	free(val);
	free(ind);

	return PyInt_FromLong((long) piv-1);
}

static PyObject* LPX_write(LPXObject *self, PyObject *args, PyObject *keywds)
{
	static char* kwlist[] = {"mps", "freemps", "cpxlp", "glp", "sol", "sens_bnds",
		"ips", "mip", NULL};
	char* fnames[] = {NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL};
	char* fname;
	const char* err_msg = "writer for '%s' failed to write to '%s'";
	int rv;

	rv = PyArg_ParseTupleAndKeywords(args, keywds, "|ssssssss", kwlist,
	fnames,fnames+1,fnames+2,fnames+3, fnames+4,fnames+5,fnames+6,fnames+7);

	if (!rv)
		return NULL;

	fname = fnames[0];
	if (fname != NULL) {
		rv = glp_write_mps(LP, GLP_MPS_DECK, NULL, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[0], fname);
			return NULL;
		}
	}

	fname = fnames[1];
	if (fname != NULL) {
		rv = glp_write_mps(LP, GLP_MPS_FILE, NULL, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[1], fname);
			return NULL;
		}
	}

	fname = fnames[2];
	if (fname != NULL) {
		rv = glp_write_lp(LP, NULL, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[2], fname);
			return NULL;
		}
	}

	fname = fnames[3];
	if (fname != NULL) {
		rv = glp_write_prob(LP, 0, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[3], fname);
			return NULL;
		}
	}

	fname = fnames[4];
	if (fname != NULL) {
		rv = glp_print_sol(LP, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[4], fname);
			return NULL;
		}
	}

	fname = fnames[5];
	if (fname != NULL) {
		if (glp_get_status(LP) == GLP_OPT && !glp_bf_exists(LP))
			glp_factorize(LP);
		rv = glp_print_ranges(LP, 0, NULL, 0, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[5], fname);
			return NULL;
		}
	}

	fname = fnames[6];
	if (fname != NULL) {
		rv = glp_print_ipt(LP, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[6], fname);
			return NULL;
		}
	}

	fname = fnames[7];
	if (fname != NULL) {
		rv = glp_print_mip(LP, fname);
		if (rv != 0) {
			PyErr_Format(PyExc_RuntimeError, err_msg, kwlist[7], fname);
			return NULL;
		}
	}
	Py_RETURN_NONE;
}

/****************** GET-SET-ERS ***************/

static PyObject* LPX_getname(LPXObject *self, void *closure)
{
	const char *name = glp_get_prob_name(LP);
	if (name == NULL)
		Py_RETURN_NONE;
	return PyString_FromString(name);
}

static int LPX_setname(LPXObject *self, PyObject *value, void *closure)
{
	char *name;
	if (value == NULL || value == Py_None) {
		glp_set_prob_name(LP, NULL);
		return 0;
	}
	name = PyString_AsString(value);
	if (name == NULL)
		return -1;
	if (PyString_Size(value) > 255) {
		PyErr_SetString(PyExc_ValueError, "name may be at most 255 chars");
		return -1;
	}
	glp_set_prob_name(LP, name);
	return 0;
}

static PyObject* LPX_getobj(LPXObject *self, void *closure)
{
	Py_INCREF(self->obj);
	return self->obj;
}

static PyObject* LPX_getnumnonzero(LPXObject *self, void *closure)
{
	return PyInt_FromLong(glp_get_num_nz(LP));
}

static PyObject* LPX_getmatrix(LPXObject *self, void *closure)
{
	return LPX_GetMatrix(self);
}

static int LPX_setmatrix(LPXObject *self, PyObject *value, void *closure)
{
	return LPX_SetMatrix(self, value);
}

static PyObject* glpstatus2string(int status)
{
	switch (status) {
	case GLP_OPT:
		return PyString_FromString("opt");
	case GLP_FEAS:
		return PyString_FromString("feas");
	case GLP_INFEAS:
		return PyString_FromString("infeas");
	case GLP_NOFEAS:
		return PyString_FromString("nofeas");
	case GLP_UNBND:
		return PyString_FromString("unbnd");
	case GLP_UNDEF:
		return PyString_FromString("undef");
	default:
		return PyString_FromString("unknown?");
	}
}

static PyObject* LPX_getstatus(LPXObject *self, void *closure)
{
	int status;
	switch (self->last_solver) {
	case -1:
	case 0:
		status=glp_get_status(LP);
		break;
	case 1:
		status=glp_ipt_status(LP);
		break;
	case 2:
		status=glp_mip_status(LP);
		break;
	default:
		PyErr_SetString(PyExc_RuntimeError, "bad internal state for last solver identifier");
		return NULL;
	}
	return glpstatus2string(status);
}

static PyObject* LPX_getspecstatus(LPXObject *self, int(*statfunc)(glp_prob *))
{
	return glpstatus2string(statfunc(LP));
}

static PyObject* LPX_getray(LPXObject *self, void *closure)
{
	int ray = glp_get_unbnd_ray(LP), numrows;
	if (ray == 0)
		Py_RETURN_NONE;
	numrows = glp_get_num_rows(LP);
	ray--;
	if (ray < numrows)
		return PySequence_GetItem(self->rows, ray);
	return PySequence_GetItem(self->cols, ray - numrows);
}

static PyObject* LPX_getkind(LPXObject *self, void *closure)
{
	PyObject *retval = NULL;
	retval = (PyObject*)(glp_get_num_int(LP) ? &PyInt_Type : &PyFloat_Type);
	Py_INCREF(retval);
	return retval;
}

static PyObject* LPX_getnumint(LPXObject *self, void *closure)
{
	return PyInt_FromLong(glp_get_num_int(LP));
}

static PyObject* LPX_getnumbin(LPXObject *self, void *closure)
{
	return PyInt_FromLong(glp_get_num_bin(LP));
}

static int unzip(PyObject *iterator, const int len, BarObject *bars[], double val[]) {
	if (!PyIter_Check(iterator)) {
		PyErr_SetString(PyExc_TypeError, "object not iterable");
		return -1;
	}

	PyObject *item;
	int index = 0;
	while ((item = PyIter_Next(iterator)) && (index < len)) {
		if (!PyTuple_Check(item) || PyTuple_Size(item) != 2) {
			PyErr_SetString(PyExc_TypeError, "item must be two element tuple");
			Py_DECREF(item);
			return -1;
		}

		PyObject *bo, *vo;
		if ((bo = PyTuple_GetItem(item, 0)) == NULL || (vo = PyTuple_GetItem(item, 1)) == NULL) {
			PyErr_SetString(PyExc_RuntimeError, "unable to unpack tuple");
			Py_DECREF(item);
			return -1;
		}

		if (!PyObject_TypeCheck(bo, &BarType) || !PyFloat_Check(vo)) {
			PyErr_SetString(PyExc_TypeError, "tuple must contain glpk.Bar and double");
			Py_DECREF(item);
			return -1;
		}

		index += 1;

		bars[index] = (BarObject *) bo;
		val[index] = PyFloat_AsDouble(vo);

		Py_DECREF(item);
	}

	return index;
}

/****************** OBJECT DEFINITION *********/

int LPX_InitType(PyObject *module)
{
	int retval;
	if ((retval = util_add_type(module, &LPXType))!=0)
		return retval;

	/* Add in constant flag terms used in this class.
	 * The following macro helps set constants.
	 */
#define SETCONST(A) PyDict_SetIntString(LPXType.tp_dict, #A, GLP_ ## A)
	// These are used in the LPX.scale method.
	SETCONST(SF_GM);
	SETCONST(SF_EQ);
	SETCONST(SF_2N);
	SETCONST(SF_SKIP);
	SETCONST(SF_AUTO);
	// These are used in control parameters for solvers.
	SETCONST(MSG_OFF);
	SETCONST(MSG_ERR);
	SETCONST(MSG_ON);
	SETCONST(MSG_ALL);

	SETCONST(PRIMAL);
	SETCONST(DUAL);
	SETCONST(DUALP);

	SETCONST(PT_STD);
	SETCONST(PT_PSE);

	SETCONST(RT_STD);
	SETCONST(RT_HAR);

	SETCONST(BR_FFV);
	SETCONST(BR_LFV);
	SETCONST(BR_MFV);
	SETCONST(BR_DTH);
	SETCONST(BR_PCH);

	SETCONST(BT_DFS);
	SETCONST(BT_BFS);
	SETCONST(BT_BLB);
	SETCONST(BT_BPH);

	SETCONST(PP_NONE);
	SETCONST(PP_ROOT);
	SETCONST(PP_ALL);
#undef SETCONST
	// Add in the calls to the other objects.
	if ((retval = Obj_InitType(module)) != 0)
		return retval;
#ifdef USEPARAMS
	if ((retval = Params_InitType(module)) != 0)
		return retval;
#endif
	if ((retval = BarCol_InitType(module)) != 0)
		return retval;
	if ((retval = KKT_InitType(module)) != 0)
		return retval;
	if ((retval = Tree_InitType(module)) != 0)
		return retval;
	return 0;
}

PyDoc_STRVAR(rows_doc, "Row collection. See the help on class BarCollection.");

PyDoc_STRVAR(cols_doc,
"Column collection. See the help on class BarCollection."
);

PyDoc_STRVAR(params_doc,
"Control parameter collection. See the help on class Params."
);

static PyMemberDef LPX_members[] = {
	{"rows", T_OBJECT_EX, offsetof(LPXObject, rows), READONLY, rows_doc},
	{"cols", T_OBJECT_EX, offsetof(LPXObject, cols), READONLY, cols_doc},
#ifdef USEPARAMS
	{"params", T_OBJECT_EX, offsetof(LPXObject, params), READONLY, params_doc},
#endif
	{NULL}
};

PyDoc_STRVAR(name_doc, "Problem name, or None if unset.");

PyDoc_STRVAR(obj_doc, "Objective function object.");

PyDoc_STRVAR(nnz_doc, "Number of non-zero constraint coefficients.");

PyDoc_STRVAR(matrix_doc,
"The constraint matrix as a list of three element (row index, column index,\n"
"value) tuples across all non-zero elements of the constraint matrix."
);

PyDoc_STRVAR(status_doc,
"The status of solution of the last solver.\n"
"\n"
"This takes the form of a string with these possible values:\n"
"\n"
"opt\n"
"  The solution is optimal.\n"
"undef\n"
"  The solution is undefined.\n"
"feas\n"
"  The solution is feasible, but not necessarily optimal.\n"
"infeas\n"
"  The solution is infeasible.\n"
"nofeas\n"
" The problem has no feasible solution.\n"
"unbnd\n"
"  The problem has an unbounded solution."
);

PyDoc_STRVAR(status_s_doc, "The status of the simplex solver's solution.");

PyDoc_STRVAR(status_i_doc,
"The status of the interior point solver's solution."
);

PyDoc_STRVAR(status_m_doc, "The status of the MIP solver's solution.");

PyDoc_STRVAR(status_primal_doc,
"The status of the primal solution of the simplex solver.\n"
"\n"
"Possible values are 'undef', 'feas', 'infeas', 'nofeas' in similar meaning\n"
"to the .status attribute."
);

PyDoc_STRVAR(status_dual_doc,
"The status of the dual solution of the simplex solver.\n"
"\n"
"Possible values are 'undef', 'feas', 'infeas', 'nofeas' in similar meaning\n"
"to the .status attribute."
);

PyDoc_STRVAR(ray_doc,
"A non-basic row or column the simplex solver has identified as causing\n"
"primal unboundness, or None if no such variable has been identified."
);

PyDoc_STRVAR(kind_doc,
"Either the type 'float' if this is a pure linear programming (LP) problem,\n"
"or the type 'int' if this is a mixed integer programming (MIP) problem."
);

PyDoc_STRVAR(nint_doc,
"The number of integer column variables. Always 0 if this is not a mixed\n"
"integer problem."
);

PyDoc_STRVAR(nbin_doc,
"The number of binary column variables, i.e., integer with 0 to 1 bounds.\n"
"Always 0 if this is not a mixed integer problem."
);

static PyGetSetDef LPX_getset[] = {
	{"name", (getter)LPX_getname, (setter)LPX_setname, name_doc, NULL},
	{"obj", (getter)LPX_getobj, (setter)NULL, obj_doc, NULL},
	{"nnz", (getter)LPX_getnumnonzero, (setter)NULL, nnz_doc, NULL},
	{"matrix", (getter)LPX_getmatrix, (setter)LPX_setmatrix, matrix_doc, NULL},
	// Solution status retrieval.
	{"status", (getter)LPX_getstatus, (setter)NULL, status_doc, NULL},
	{"status_s", (getter)LPX_getspecstatus, (setter)NULL, status_s_doc,
	(void*)glp_get_status},
	{"status_i", (getter)LPX_getspecstatus, (setter)NULL, status_i_doc,
	(void*)glp_ipt_status},
	{"status_m", (getter)LPX_getspecstatus, (setter)NULL, status_m_doc,
	(void*)glp_mip_status},
	{"status_primal", (getter)LPX_getspecstatus, (setter)NULL,
	status_primal_doc, (void*)glp_get_prim_stat},
	{"status_dual", (getter)LPX_getspecstatus, (setter)NULL, status_dual_doc,
	(void*)glp_get_dual_stat},
	// Ray info.
	{"ray", (getter)LPX_getray, (setter)NULL, ray_doc, NULL},
	// Setting for MIP.
	{"kind", (getter)LPX_getkind, NULL, kind_doc, NULL},
	{"nint", (getter)LPX_getnumint, (setter)NULL, nint_doc, NULL},
	{"nbin", (getter)LPX_getnumbin, (setter)NULL, nbin_doc, NULL},
	{NULL}
};

PyDoc_STRVAR(erase_doc,
"erase()\n"
"\n"
"Erase the content of this problem, restoring it to the state it was in when\n"
"it was first created.");

PyDoc_STRVAR(copy_doc,
"copy()\n"
"\n"
"Copies the content of this problem into a new problem and returns it.");

PyDoc_STRVAR(scale_doc,
"scale([flags=LPX.SF_AUTO])\n"
"\n"
"Perform automatic scaling of the problem data, in order to improve\n"
"conditioning. The behavior is controlled by various flags, which can be\n"
"bitwise ORed to combine effects. Note that this only affects the internal\n"
"state of the LP representation. These flags are members of the LPX class:\n"
"\n"
"SF_GM\n"
"  perform geometric mean scaling\n"
"SF_EQ\n"
"  perform equilibration scaling\n"
"SF_2N\n"
"  round scale factors to the nearest power of two\n"
"SF_SKIP\n"
"  skip scaling, if the problem is well scaled\n"
"SF_AUTO\n"
"  choose scaling options automatically"
);

PyDoc_STRVAR(unscale_doc,
"unscale()\n"
"\n"
"This unscales the problem data, essentially setting all scale factors to 1."
);

PyDoc_STRVAR(std_basis_doc,
"std_basis()\n"
"\n"
"Construct the standard trivial inital basis for this LP."
);

PyDoc_STRVAR(adv_basis_doc,
"adv_basis()\n"
"\n"
"Construct an advanced initial basis, triangular with as few variables as\n"
"possible fixed."
);

PyDoc_STRVAR(cpx_basis_doc,
"cpx_basis()\n"
"\n"
"Construct an advanced Bixby basis. This basis construction method is\n"
"described in:\n"
"Robert E. Bixby. Implementing the Simplex Method: The Initial Basis. ORSA\n"
"Journal on Computing, Vol. 4, No. 3, 1992, pp. 267-84."
);

PyDoc_STRVAR(simplex_doc,
"simplex([keyword arguments])\n"
"\n"
"Attempt to solve the problem using a simplex method.\n"
"\n"
"This procedure has a great number of optional keyword arguments to control\n"
"the functioning of the solver. We list these here, including descriptions\n"
"of their legal values.\n"
"\n"
"msg_lev\n"
"  Controls the message level of terminal output.\n"
"\n"
"  LPX.MSG_OFF\n"
"    no output (default)\n"
"  LPX.MSG_ERR\n"
"    error and warning messages\n"
"  LPX.MSG_ON\n"
"    normal output\n"
"  LPX.MSG_ALL\n"
"    full informational output\n"
"\n"
"meth\n"
"  Simplex method option\n"
"\n"
"  LPX.PRIMAL\n"
"    use two phase primal simplex (default)\n"
"  LPX.DUAL\n"
"    use two phase dual simplex\n"
"  LPX.DUALP\n"
"    use two phase dual simplex, primal if that fails\n"
"\n"
"pricing\n"
"  Pricing technique\n"
"\n"
"  LPX.PT_STD\n"
"    standard textbook technique\n"
"  LPX.PT_PSE\n"
"    projected steepest edge (default)\n"
"\n"
"r_test\n"
"  Ratio test technique\n"
"\n"
"  LPX.RT_STD\n"
"    standard textbook technique\n"
"  LPX.RT_HAR\n"
"    Harris' two-pass ratio test (default)\n"
"\n"
"tol_bnd\n"
"  Tolerance used to check if the basic solution is primal feasible.\n"
"  (default 1e-7)\n"
"\n"
"tol_dj\n"
"  Tolerance used to check if the basic solution is dual feasible. (default\n"
"  1e-7)\n"
"\n"
"tol_piv\n"
"  Tolerance used to choose pivotal elements of the simplex table. (default\n"
"  1e-10)\n"
"\n"
"obj_ll\n"
"  Lower limit of the objective function. The solver terminates upon\n"
"  reaching this level. This is used only in dual simplex optimization.\n"
"  (default is min float)\n"
"\n"
"obj_ul\n"
"  Upper limit of the objective function. The solver terminates upon\n"
"  reaching this level. This is used only in dual simplex optimization.\n"
"  (default is max float)\n"
"\n"
"it_lim\n"
"  Simplex iteration limit. (default is max int)\n"
"\n"
"tm_lim\n"
"  Search time limit in milliseconds. (default is max int)\n"
"\n"
"out_frq\n"
"  Terminal output frequency in iterations. (default 200)\n"
"\n"
"out_dly\n"
"  Terminal output delay in milliseconds. (default 0)\n"
"\n"
"presolve\n"
"  Use the LP presolver. (default False)\n"
"\n"
"This returns None if the problem was successfully solved. Alternately, on\n"
"failure it will return one of the following strings to indicate failure\n"
"type.\n"
"\n"
"fault\n"
"  There are no rows or columns, or the initial basis is invalid, or the\n"
"  initial basis matrix is singular or ill-conditioned.\n"
"objll\n"
"  The objective reached its lower limit.\n"
"objul\n"
"  The objective reached its upper limit.\n"
"itlim\n"
"  Iteration limited exceeded.\n"
"tmlim\n"
"  Time limit exceeded.\n"
"sing\n"
"  The basis matrix became singular or ill-conditioned.\n"
"nopfs\n"
"  No primal feasible solution. (Presolver only.)\n"
"nodfs\n"
"  No dual feasible solution. (Presolver only.)\n"
);

PyDoc_STRVAR(exact_doc,
"exact()\n"
"\n"
"Attempt to solve the problem using an exact simplex method.\n"
"\n"
"This returns None if the problem was successfully solved. Alternately, on\n"
"failure it will return one of the following strings to indicate failure\n"
"type:\n"
"\n"
"fault\n"
"  There are no rows or columns, or the initial basis is invalid, or the\n"
"  initial basis matrix is singular or ill-conditioned.\n"
"itlim\n"
"  Iteration limited exceeded.\n"
"tmlim\n"
"  Time limit exceeded."
);

PyDoc_STRVAR(interior_doc,
"interior()\n"
"\n"
"Attempt to solve the problem using an interior-point method.\n"
"\n"
"This returns None if the problem was successfully solved. Alternately, on\n"
"failure it will return one of the following strings to indicate failure\n"
"type:\n"
"\n"
"fault\n"
"  There are no rows or columns.\n"
"nofeas\n"
"  The problem has no feasible (primal/dual) solution.\n"
"noconv\n"
"  Very slow convergence or divergence.\n"
"itlim\n"
"  Iteration limited exceeded.\n"
"instab\n"
"  Numerical instability when solving Newtonian system."
);

PyDoc_STRVAR(integer_doc,
"integer()\n"
"\n"
"MIP solver based on branch-and-bound.\n"
"\n"
"This procedure has a great number of optional keyword arguments to control\n"
"the functioning of the solver. We list these here, including descriptions\n"
"of their legal values:\n"
"\n"
"msg_lev\n"
"  Controls the message level of terminal output.\n"
"\n"
"  LPX.MSG_OFF\n"
"    no output (default)\n"
"  LPX.MSG_ERR\n"
"    error and warning messages\n"
"  LPX.MSG_ON\n"
"    normal output\n"
"  LPX.MSG_ALL\n"
"    full informational output\n"
"\n"
"br_tech\n"
"  Branching technique option.\n"
"\n"
"  LPX.BR_FFV\n"
"    first fractional variable\n"
"  LPX.BR_LFV\n"
"    last fractional variable\n"
"  LPX.BR_MFV\n"
"    most fractional variable\n"
"  LPX.BR_DTH\n"
"    heuristic by Driebeck and Tomlin (default)\n"
"  LPX.BR_PCH\n"
"    hybrid pseudo-cost heuristic\n"
"\n"
"bt_tech\n"
"  Backtracking technique option.\n"
"\n"
"  LPX.BT_DFS\n"
"    depth first search\n"
"  LPX.BT_BFS\n"
"    breadth first search\n"
"  LPX.BT_BLB\n"
"    best local bound (default)\n"
"  LPX.BT_BPH\n"
"    best projection heuristic\n"
"\n"
"pp_tech\n"
"  Preprocessing technique option.\n"
"\n"
"  LPX.PP_NONE\n"
"    disable preprocessing\n"
"  LPX.PP_ROOT\n"
"    perform preprocessing only on the root level\n"
"  LPX.PP_ALL\n"
"    perform preprocessing on all levels (default)\n"
"\n"
"sr_heur\n"
"  Simple rounding heuristic (default True)\n"
"  (requires glpk >= 4.57.0)\n"
"\n"
"fp_heur\n"
"  Feasibility pump heurisic (default False)\n"
"\n"
"ps_heur\n"
"  Proximity search heuristic (default False)\n"
"\n"
"ps_tm_lim\n"
"  Proximity search time limit in milliseconds (default 60000)\n"
"\n"
"gmi_cuts\n"
"  Use Gomory's mixed integer cuts (default False)\n"
"\n"
"mir_cuts\n"
"  Use mixed integer rounding cuts (default False)\n"
"\n"
"cov_cuts\n"
"  Use mixed cover cuts (default False)\n"
"\n"
"clq_cuts\n"
"  Use clique cuts (default False)\n"
"\n"
"tol_int\n"
"  Tolerance used to check if the optimal solution to the current LP\n"
"  relaxation is integer feasible.\n"
"\n"
"tol_obj\n"
"  Tolerance used to check if the objective value in the optimal solution to\n"
"  the current LP is not better than the best known integer feasible\n"
"  solution.\n"
"\n"
"mip_gap\n"
"  elative mip gap tolerance (default 0.0)\n"
"\n"
"tm_lim\n"
"  Search time limit in milliseconds. (default is max int)\n"
"\n"
"out_frq\n"
"  Terminal output frequency in milliseconds. (default 5000)\n"
"\n"
"out_dly\n"
"  Terminal output delay in milliseconds. (default 10000)\n"
"\n"
"presolve\n"
"  MIP presolver (default False)\n"
"\n"
"binarize\n"
"  Binarization option, used only if presolver is enabled (default False)\n"
"\n"
"callback\n"
"  A callback object the user may use to monitor and control the solver.\n"
"  During certain portions of the optimization, the solver will call methods\n"
"  of callback object. (default None)\n"
"\n"
"The last parameter, callback, is worth its own discussion. During the \n"
"branch-and-cut algorithm of the MIP solver, at various points callback\n"
"hooks are invoked which allow the user code to influence the proceeding of\n"
"the MIP solver. The user code may influence the solver in the hook by\n"
"modifying and operating on a Tree instance passed to the hook. These hooks\n"
"have various codes, which we list here:\n"
"\n"
"select\n"
"  request for subproblem selection\n"
"\n"
"prepro\n"
"  request for preprocessing\n"
"\n"
"rowgen\n"
"  request for row generation\n"
"\n"
"heur\n"
"  request for heuristic solution\n"
"\n"
"cutgen\n"
"  request for cut generation\n"
"\n"
"branch\n"
"  request for branching\n"
"\n"
"bingo\n"
"  better integer solution found\n"
"\n"
"During the invocation of a hook with a particular code, the callback object\n"
"will have a method of the same name as the hook code called, with the Tree\n"
"instance. For instance, for the 'cutgen' hook, it is equivalent to::\n"
"\n"
"    callback.cutgen(tree)\n"
"\n"
"being called with tree as the Tree instance. If the method does not exist,\n"
"then instead the method 'default' is called with the same signature. If\n"
"neither the named hook method nor the default method exist, then the hook\n"
"is ignored.\n"
"\n"
"This method requires a mixed-integer problem where an optimal solution to\n"
"an LP relaxation (either through simplex() or exact()) has already been\n"
"found. Alternately, try intopt().\n"
"\n"
"This returns None if the problem was successfully solved. Alternately, on\n"
"failure it will return one of the following strings to indicate failure\n"
"type.\n"
"\n"
"fault\n"
"  There are no rows or columns, or it is not a MIP problem, or integer\n"
"  variables have non-int bounds.\n"
"nopfs\n"
"  No primal feasible solution.\n"
"nodfs\n"
"  Relaxation has no dual feasible solution.\n"
"itlim\n"
"  Iteration limited exceeded.\n"
"tmlim\n"
"  Time limit exceeded.\n"
"sing\n"
"  Error occurred solving an LP relaxation subproblem."
);

PyDoc_STRVAR(intopt_doc,
"intopt()\n"
"\n"
"More advanced MIP branch-and-bound solver than integer(). This variant does\n"
"not require an existing LP relaxation.\n"
"\n"
"This returns None if the problem was successfully solved. Alternately, on\n"
"failure it will return one of the following strings to indicate failure\n"
"type.\n"
"\n"
"fault\n"
"  There are no rows or columns, or it is not a MIP problem, or integer\n"
"  variables have non-int bounds.\n"
"\n"
"nopfs\n"
"  No primal feasible solution.\n"
"\n"
"nodfs\n"
"  Relaxation has no dual feasible solution.\n"
"\n"
"itlim\n"
"  Iteration limited exceeded.\n"
"\n"
"tmlim\n"
"  Time limit exceeded.\n"
"\n"
"sing\n"
"  Error occurred solving an LP relaxation subproblem."
);

PyDoc_STRVAR(kkt_doc,
"kkt([scaled=False])\n"
"\n"
"Return an object encapsulating the results of a check on the\n"
"Karush-Kuhn-Tucker optimality conditions for a basic (simplex) solution. If\n"
"the argument 'scaled' is true, return results of checking the internal\n"
"scaled instance of the LP instead."
);

PyDoc_STRVAR(kktint_doc,
"kktint()\n"
"\n"
"Similar to kkt(), except analyzes solution quality of an mixed-integer\n"
"solution. Note that only the primal components of the KKT object will have\n"
"meaningful values."
);

PyDoc_STRVAR(write_doc,
"write(format=filename)\n"
"\n"
"Output data about the linear program into a file with a given format. What\n"
"data is written, and how it is written, depends on which of the format\n"
"keywords are used. Note that one may specify multiple format and filename\n"
"pairs to write multiple types and formats of data in one call to this\n"
"function.\n"
"\n"
"mps\n"
"  For problem data in the fixed MPS format.\n"
"\n"
"bas\n"
"  The current LP basis in fixed MPS format.\n"
"\n"
"freemps\n"
"  Problem data in the free MPS format.\n"
"\n"
"cpxlp\n"
"  Problem data in the CPLEX LP format.\n"
"\n"
"glp\n"
"  Problem data in the GNU LP format.\n"
"\n"
"sol\n"
"  Basic solution in printable format.\n"
"\n"
"sens_bnds\n"
"  Bounds sensitivity information.\n"
"\n"
"ips\n"
"  Interior-point solution in printable format.\n"
"\n"
"mip\n"
"  MIP solution in printable format."
);

PyDoc_STRVAR(warm_up__doc__,
"LPX.warm_up() -> string\n\n"
"Warms up the LP basis.\n"
"\n"
"Returns None if successful, otherwise one of the following error strings:\n"
"\n"
"badb\n"
"  the basis matrix is invalid\n"
"sing\n"
"  the basis matrix is singular\n"
"cond\n"
"  the basis matrix is ill-conditioned\n"
);

PyDoc_STRVAR(transform_row__doc__,
"LPX.transform_row([(glpk.Bar, float), ...]) -> [(glpk.Bar, float), ...]\n"
"\n"
"Transforms the explicitly specified row\n"
"\n"
"The row to be transformed is given as a list of tuples, with each tuple\n"
"containing a variable (i.e., an instance of glpk.Bar) and a coefficient.\n"
"The input variables should be structural variables (i.e., elements of\n"
"LPX.cols).\n"
"\n"
"The row is returned as a list of tuples containing a reference to a\n"
"non-basic variable and the corresponding coefficient from the simplex\n"
"tableau."
);

PyDoc_STRVAR(transform_col__doc__,
"LPX.transform_col([(glpk.Bar, float), ...]) -> [(glpk.Bar, float), ...]\n"
"\n"
"Transforms the explicitly specified column\n"
"\n"
"The column to be transformed is given as a list of tuples, with each tuple\n"
"containing a variable (i.e., an instance of glpk.Bar) and a coefficient.\n"
"The input variables should be auxiliary variables (i.e., elements of\n"
"LPX.rows).\n"
"\n"
"The column is returned as a list of tuples containing a reference to a\n"
"basic variable and the corresponding coefficient from the simplex\n"
"tableau."
);

PyDoc_STRVAR(prime_ratio_test__doc__,
"LPX.prime_ratio_test([(glpk.Bar, float), int, float]) -> int\n"
"\n"
"Perform primal ratio test using an explicitly specified column of the\n"
"simplex tableau.\n"
"\n"
"The column of the simplex tableau is given as a list of tuples, with each\n"
"tuple containing a basic variable and a coefficient.\n"
"The second argument is an integer specifying the direction in which the\n"
"variable changes when entering the basis: +1 means increasing, -1 means\n"
"decreasing.\n"
"The third argument is an absolute tolerance used by the routine to skip\n"
"small coefficients.\n"
"\n"
"Returns the index of the input column corresponding to the pivot element."
);

PyDoc_STRVAR(dual_ratio_test__doc__,
"LPX.dual_ratio_test([(glpk.Bar, float), int, float]) -> int\n"
"\n"
"Perform dual ratio test using an explicitly specified row of the simplex\n"
"tableau.\n"
"\n"
"The row of the simplex tableau is given as a list of tuples, with each\n"
"tuple containing a basic variable and a coefficient.\n"
"The second argument is an integer specifying the direction in which the\n"
"variable changes when entering the basis: +1 means increasing, -1 means\n"
"decreasing.\n"
"The third argument is an absolute tolerance used by the routine to skip\n"
"small coefficients.\n"
"\n"
"Returns the index of the input row corresponding to the pivot element."
);

static PyMethodDef LPX_methods[] = {
	{"erase", (PyCFunction)LPX_Erase, METH_NOARGS, erase_doc},
	{"copy", (PyCFunction)LPX_Copy, METH_VARARGS, copy_doc},
	{"scale", (PyCFunction)LPX_Scale, METH_VARARGS, scale_doc},
	{"unscale", (PyCFunction)LPX_Unscale, METH_NOARGS, unscale_doc},
	// Basis construction techniques for simplex solvers.
	{"std_basis", (PyCFunction)LPX_basis_std, METH_NOARGS, std_basis_doc},
	{"adv_basis", (PyCFunction)LPX_basis_adv, METH_NOARGS, adv_basis_doc},
	{"cpx_basis", (PyCFunction)LPX_basis_cpx, METH_NOARGS, cpx_basis_doc},
	// Solver routines.
	{"simplex", (PyCFunction)LPX_solver_simplex, METH_VARARGS|METH_KEYWORDS,
	simplex_doc},
	{"exact", (PyCFunction)LPX_solver_exact, METH_NOARGS, exact_doc},
	{"interior", (PyCFunction)LPX_solver_interior, METH_NOARGS, interior_doc},
	{"integer", (PyCFunction)LPX_solver_integer, METH_VARARGS|METH_KEYWORDS,
	integer_doc},
	{"intopt", (PyCFunction)LPX_solver_intopt, METH_NOARGS, intopt_doc},
	{"kkt", (PyCFunction)LPX_kkt, METH_VARARGS, kkt_doc},
	{"kktint", (PyCFunction)LPX_kktint, METH_NOARGS, kktint_doc},
	// Data writing
	{"write", (PyCFunction)LPX_write, METH_VARARGS | METH_KEYWORDS, write_doc},
	{"warm_up", (PyCFunction)LPX_warm_up, METH_NOARGS, warm_up__doc__},
	{"transform_row", (PyCFunction)LPX_transform_row, METH_O, transform_row__doc__},
	{"transform_col", (PyCFunction)LPX_transform_col, METH_O, transform_col__doc__},
	{"prime_ratio_test", (PyCFunction)LPX_prim_rtest, METH_VARARGS, prime_ratio_test__doc__},
	{"dual_ratio_test", (PyCFunction)LPX_dual_rtest, METH_VARARGS, dual_ratio_test__doc__},
	{NULL}
};

PyDoc_STRVAR(lpx_doc,
"LPX() -> empty linear program\n"
"LPX(gmp=filename) -> linear program with data read from a GNU MathProg file\n"
"    containing model and data\n"
"LPX(mps=filename) -> linear program with data read from a datafile in fixed\n"
"    MPS format\n"
"LPX(freemps=filename) -> linear program with data read from a datafile in\n"
"    free MPS format\n"
"LPX(cpxlp=filename) -> linear program with data read from a datafile in\n"
"    fixed CPLEX LP format\n"
"LPX(glp=filename) -> linear program with data read from a datafile in GNU\n"
"    LP format\n"
"LPX(gmp=(model_filename[, data_filename[, output_filename]]) -> linear\n"
"    program from GNU MathProg input files. The first element is a path to\n"
"    model second, the second to the data section. If the second element is\n"
"    omitted or is None then the model file is presumed to also hold the\n"
"    data. The third element holds the output data file to write display\n"
"    statements to. If omitted or None, the output is instead put through\n"
"    to standard output.\n"
"\n"
"This represents a linear program object. It holds data and offers methods\n"
"relevant to the whole of the linear program. There are many members in this\n"
"class, but the most important are:\n"
"obj -> represents the objective function\n"
"rows -> a collection over which one can access rows\n"
"cols -> same, but for columns\n"
);

PyTypeObject LPXType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name      = "glpk.LPX",
    .tp_basicsize = sizeof(LPXObject),
    .tp_dealloc   = (destructor)LPX_dealloc,
    .tp_repr      = (reprfunc)LPX_Str,
    .tp_str       = (reprfunc)LPX_Str,
    .tp_flags     = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_doc       = lpx_doc,
    .tp_traverse  = (traverseproc)LPX_traverse,
    .tp_clear     = (inquiry)LPX_clear,
    .tp_weaklistoffset = offsetof(LPXObject, weakreflist),
    .tp_methods   = LPX_methods,
    .tp_members   = LPX_members,
    .tp_getset    = LPX_getset,
    .tp_init      = (initproc)LPX_init,
    .tp_new       = LPX_new,
};
