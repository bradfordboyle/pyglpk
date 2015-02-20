/***********************************************************************
*  This code is part of GLPK (GNU Linear Programming Kit).
*
*  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008,
*  2009, 2010, 2011, 2013 Andrew Makhorin, Department for Applied
*  Informatics, Moscow Aviation Institute, Moscow, Russia. All rights
*  reserved. E-mail: <mao@gnu.org>.
*
*  GLPK is free software: you can redistribute it and/or modify it
*  under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  GLPK is distributed in the hope that it will be useful, but WITHOUT
*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
*  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
*  License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with GLPK. If not, see <http://www.gnu.org/licenses/>.
***********************************************************************/

#include <float.h>
#include <glpk.h>
#include <limits.h>
#include "lpx.h"

/* forward declare static methods */
static void fill_smcp(LPX *lp, glp_smcp *parm);
static struct LPXCPS *access_parms(LPX *lp);
static void reset_parms(LPX *lp);
static int solve_mip(LPX *lp, int presolve);

/* read LP/MIP model written in GNU MathProg language */
/* glpk-4.45/glplpx01.c:1482--1503 */
LPX *lpx_read_model(const char *model, const char *data, const char *output)
{
	LPX *lp = NULL;
	glp_tran *tran;
	/* allocate the translator workspace */
	tran = glp_mpl_alloc_wksp();
	/* read model section and optional data section */
	if (glp_mpl_read_model(tran, model, data != NULL))
		goto done;
	/* read separate data section, if required */
	/* TODO: compound and statement? */
	if (data != NULL)
		if (glp_mpl_read_data(tran, data))
			goto done;
	/* generate the model */
	if (glp_mpl_generate(tran, output))
		goto done;
	/* build the problem instance from the model */
	lp = glp_create_prob();
	glp_mpl_build_prob(tran, lp);
done:
	/* free the translator workspace */
	glp_mpl_free_wksp(tran);
	/* bring the problem object to the calling program */
	return lp;
}

/* read LP basis in fixed MPS format */
int lpx_read_bas(LPX *lp, const char *fname)
{
#if 0
	return read_bas(lp, fname);
#else
	xassert(lp == lp);
	xassert(fname == fname);
	xerror("lpx_read_bas: operation not supported\n");
	return 0;
#endif
}

/* easy-to-use driver to the exact simplex method */
/* glpk-4.45/glplpx01.c:539--556 */
int lpx_exact(LPX *lp)
{
	glp_smcp parm;
	int ret;
	fill_smcp(lp, &parm);
	ret = glp_exact(lp, &parm);
	switch (ret) {
	case 0:
		ret = LPX_E_OK;
		break;
	case GLP_EBADB:
	case GLP_ESING:
	case GLP_EBOUND:
	case GLP_EFAIL:
		ret = LPX_E_FAULT;
		break;
	case GLP_EITLIM:
		ret = LPX_E_ITLIM;
		break;
	case GLP_ETMLIM:
		ret = LPX_E_TMLIM;
		break;
	default:
		xassert(ret != ret);
	}
	return ret;
}

int lpx_intopt(LPX *lp)
{     /* easy-to-use driver to the branch-and-bound method */
      return solve_mip(lp, GLP_ON);
}

static int solve_mip(LPX *lp, int presolve)
{     glp_iocp parm;
      int ret;
      glp_init_iocp(&parm);
      switch (lpx_get_int_parm(lp, LPX_K_MSGLEV))
      {  case 0:  parm.msg_lev = GLP_MSG_OFF;   break;
         case 1:  parm.msg_lev = GLP_MSG_ERR;   break;
         case 2:  parm.msg_lev = GLP_MSG_ON;    break;
         case 3:  parm.msg_lev = GLP_MSG_ALL;   break;
         default: xassert(lp != lp);
      }
      switch (lpx_get_int_parm(lp, LPX_K_BRANCH))
      {  case 0:  parm.br_tech = GLP_BR_FFV;    break;
         case 1:  parm.br_tech = GLP_BR_LFV;    break;
         case 2:  parm.br_tech = GLP_BR_DTH;    break;
         case 3:  parm.br_tech = GLP_BR_MFV;    break;
         default: xassert(lp != lp);
      }
      switch (lpx_get_int_parm(lp, LPX_K_BTRACK))
      {  case 0:  parm.bt_tech = GLP_BT_DFS;    break;
         case 1:  parm.bt_tech = GLP_BT_BFS;    break;
         case 2:  parm.bt_tech = GLP_BT_BPH;    break;
         case 3:  parm.bt_tech = GLP_BT_BLB;    break;
         default: xassert(lp != lp);
      }
      parm.tol_int = lpx_get_real_parm(lp, LPX_K_TOLINT);
      parm.tol_obj = lpx_get_real_parm(lp, LPX_K_TOLOBJ);
      if (lpx_get_real_parm(lp, LPX_K_TMLIM) < 0.0 ||
          lpx_get_real_parm(lp, LPX_K_TMLIM) > 1e6)
         parm.tm_lim = INT_MAX;
      else
         parm.tm_lim =
            (int)(1000.0 * lpx_get_real_parm(lp, LPX_K_TMLIM));
      parm.mip_gap = lpx_get_real_parm(lp, LPX_K_MIPGAP);
      if (lpx_get_int_parm(lp, LPX_K_USECUTS) & LPX_C_GOMORY)
         parm.gmi_cuts = GLP_ON;
      else
         parm.gmi_cuts = GLP_OFF;
      if (lpx_get_int_parm(lp, LPX_K_USECUTS) & LPX_C_MIR)
         parm.mir_cuts = GLP_ON;
      else
         parm.mir_cuts = GLP_OFF;
      if (lpx_get_int_parm(lp, LPX_K_USECUTS) & LPX_C_COVER)
         parm.cov_cuts = GLP_ON;
      else
         parm.cov_cuts = GLP_OFF;
      if (lpx_get_int_parm(lp, LPX_K_USECUTS) & LPX_C_CLIQUE)
         parm.clq_cuts = GLP_ON;
      else
         parm.clq_cuts = GLP_OFF;
      parm.presolve = presolve;
      if (lpx_get_int_parm(lp, LPX_K_BINARIZE))
         parm.binarize = GLP_ON;
      ret = glp_intopt(lp, &parm);
      switch (ret)
      {  case 0:           ret = LPX_E_OK;      break;
         case GLP_ENOPFS:  ret = LPX_E_NOPFS;   break;
         case GLP_ENODFS:  ret = LPX_E_NODFS;   break;
         case GLP_EBOUND:
         case GLP_EROOT:   ret = LPX_E_FAULT;   break;
         case GLP_EFAIL:   ret = LPX_E_SING;    break;
         case GLP_EMIPGAP: ret = LPX_E_MIPGAP;  break;
         case GLP_ETMLIM:  ret = LPX_E_TMLIM;   break;
         default:          xassert(ret != ret);
      }
      return ret;
}

double lpx_get_real_parm(LPX *lp, int parm)
{     /* query real control parameter */
#if 0 /* 17/XI-2009 */
      struct LPXCPS *cps = lp->cps;
#else
      struct LPXCPS *cps = access_parms(lp);
#endif
      double val = 0.0;
      switch (parm)
      {  case LPX_K_RELAX:
            val = cps->relax;
            break;
         case LPX_K_TOLBND:
            val = cps->tol_bnd;
            break;
         case LPX_K_TOLDJ:
            val = cps->tol_dj;
            break;
         case LPX_K_TOLPIV:
            val = cps->tol_piv;
            break;
         case LPX_K_OBJLL:
            val = cps->obj_ll;
            break;
         case LPX_K_OBJUL:
            val = cps->obj_ul;
            break;
         case LPX_K_TMLIM:
            val = cps->tm_lim;
            break;
         case LPX_K_OUTDLY:
            val = cps->out_dly;
            break;
         case LPX_K_TOLINT:
            val = cps->tol_int;
            break;
         case LPX_K_TOLOBJ:
            val = cps->tol_obj;
            break;
         case LPX_K_MIPGAP:
            val = cps->mip_gap;
            break;
         default:
            xerror("lpx_get_real_parm: parm = %d; invalid parameter\n",
               parm);
      }
      return val;
}

int lpx_get_int_parm(LPX *lp, int parm)
{     /* query integer control parameter */
#if 0 /* 17/XI-2009 */
      struct LPXCPS *cps = lp->cps;
#else
      struct LPXCPS *cps = access_parms(lp);
#endif
      int val = 0;
      switch (parm)
      {  case LPX_K_MSGLEV:
            val = cps->msg_lev; break;
         case LPX_K_SCALE:
            val = cps->scale; break;
         case LPX_K_DUAL:
            val = cps->dual; break;
         case LPX_K_PRICE:
            val = cps->price; break;
         case LPX_K_ROUND:
            val = cps->round; break;
         case LPX_K_ITLIM:
            val = cps->it_lim; break;
         case LPX_K_ITCNT:
            val = lp->it_cnt; break;
         case LPX_K_OUTFRQ:
            val = cps->out_frq; break;
         case LPX_K_BRANCH:
            val = cps->branch; break;
         case LPX_K_BTRACK:
            val = cps->btrack; break;
         case LPX_K_MPSINFO:
            val = cps->mps_info; break;
         case LPX_K_MPSOBJ:
            val = cps->mps_obj; break;
         case LPX_K_MPSORIG:
            val = cps->mps_orig; break;
         case LPX_K_MPSWIDE:
            val = cps->mps_wide; break;
         case LPX_K_MPSFREE:
            val = cps->mps_free; break;
         case LPX_K_MPSSKIP:
            val = cps->mps_skip; break;
         case LPX_K_LPTORIG:
            val = cps->lpt_orig; break;
         case LPX_K_PRESOL:
            val = cps->presol; break;
         case LPX_K_BINARIZE:
            val = cps->binarize; break;
         case LPX_K_USECUTS:
            val = cps->use_cuts; break;
         case LPX_K_BFTYPE:
#if 0
            val = cps->bf_type; break;
#else
            {  glp_bfcp parm;
               glp_get_bfcp(lp, &parm);
               switch (parm.type)
               {  case GLP_BF_FT:
                     val = 1; break;
                  case GLP_BF_BG:
                     val = 2; break;
                  case GLP_BF_GR:
                     val = 3; break;
                  default:
                     xassert(lp != lp);
               }
            }
            break;
#endif
         default:
            xerror("lpx_get_int_parm: parm = %d; invalid parameter\n",
               parm);
      }
      return val;
}

static void fill_smcp(LPX *lp, glp_smcp *parm)
{
	glp_init_smcp(parm);
	switch (lpx_get_int_parm(lp, LPX_K_MSGLEV)) {
		case 0:
			parm->msg_lev = GLP_MSG_OFF;
			break;
		case 1:
			parm->msg_lev = GLP_MSG_ERR;
			break;
		case 2:
			parm->msg_lev = GLP_MSG_ON;
			break;
		case 3:
			parm->msg_lev = GLP_MSG_ALL;
			break;
		default:
			xassert(lp != lp);
	}
	switch (lpx_get_int_parm(lp, LPX_K_DUAL)) {
		case 0:
			parm->meth = GLP_PRIMAL;
			break;
		case 1:
			parm->meth = GLP_DUAL;
			break;
		default:
			xassert(lp != lp);
	}
  	switch (lpx_get_int_parm(lp, LPX_K_PRICE)) {
		case 0:
			parm->pricing = GLP_PT_STD;
			break;
       		case 1:
			parm->pricing = GLP_PT_PSE;
			break;
       		default:
			xassert(lp != lp);
	}
	if (lpx_get_real_parm(lp, LPX_K_RELAX) == 0.0)
		parm->r_test = GLP_RT_STD;
	else
		parm->r_test = GLP_RT_HAR;
	parm->tol_bnd = lpx_get_real_parm(lp, LPX_K_TOLBND);
	parm->tol_dj  = lpx_get_real_parm(lp, LPX_K_TOLDJ);
	parm->tol_piv = lpx_get_real_parm(lp, LPX_K_TOLPIV);
	parm->obj_ll  = lpx_get_real_parm(lp, LPX_K_OBJLL);
	parm->obj_ul  = lpx_get_real_parm(lp, LPX_K_OBJUL);
	if (lpx_get_int_parm(lp, LPX_K_ITLIM) < 0)
		parm->it_lim = INT_MAX;
	else
		parm->it_lim = lpx_get_int_parm(lp, LPX_K_ITLIM);
	if (lpx_get_real_parm(lp, LPX_K_TMLIM) < 0.0)
		parm->tm_lim = INT_MAX;
	else
		parm->tm_lim =
			(int)(1000.0 * lpx_get_real_parm(lp, LPX_K_TMLIM));
	parm->out_frq = lpx_get_int_parm(lp, LPX_K_OUTFRQ);
	parm->out_dly =
		(int)(1000.0 * lpx_get_real_parm(lp, LPX_K_OUTDLY));
	switch (lpx_get_int_parm(lp, LPX_K_PRESOL)) {
		case 0:
			parm->presolve = GLP_OFF;
			break;
		case 1:
			parm->presolve = GLP_ON;
			break;
		default:
			xassert(lp != lp);
	}
	return;
}

static void reset_parms(LPX *lp)
{     /* reset control parameters to default values */
      struct LPXCPS *cps = lp->parms;
      xassert(cps != NULL);
      cps->msg_lev  = 3;
      cps->scale    = 1;
      cps->dual     = 0;
      cps->price    = 1;
      cps->relax    = 0.07;
      cps->tol_bnd  = 1e-7;
      cps->tol_dj   = 1e-7;
      cps->tol_piv  = 1e-9;
      cps->round    = 0;
      cps->obj_ll   = -DBL_MAX;
      cps->obj_ul   = +DBL_MAX;
      cps->it_lim   = -1;
#if 0 /* 02/XII-2010 */
      lp->it_cnt   = 0;
#endif
      cps->tm_lim   = -1.0;
      cps->out_frq  = 200;
      cps->out_dly  = 0.0;
      cps->branch   = 2;
      cps->btrack   = 3;
      cps->tol_int  = 1e-5;
      cps->tol_obj  = 1e-7;
      cps->mps_info = 1;
      cps->mps_obj  = 2;
      cps->mps_orig = 0;
      cps->mps_wide = 1;
      cps->mps_free = 0;
      cps->mps_skip = 0;
      cps->lpt_orig = 0;
      cps->presol = 0;
      cps->binarize = 0;
      cps->use_cuts = 0;
      cps->mip_gap = 0.0;
      return;
}

static struct LPXCPS *access_parms(LPX *lp)
{     /* allocate and initialize control parameters, if necessary */
      if (lp->parms == NULL)
      {  lp->parms = xmalloc(sizeof(struct LPXCPS));
         reset_parms(lp);
      }
      return lp->parms;
}
