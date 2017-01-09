===========================
GLPK Version Specific Notes
===========================

When building PyGLPK, certain functions will be disabled depending on what version of GLPK the build process believes it is using. Below is a list of GLPK versions, and descriptions of what functionality will not work in earlier versions. (Or, in some cases, later versions.)

The examples may use a hypothetical ``glpk.LPX`` instance named ``lp`` for illustrative purposes.


GLPK 4.39
    * The interior point solver often does not detect infeasibility on infeasible problems.

GLPK 4.38
    * The interior point solver often crashes on infeasible problems.

GLPK 4.37 and higher
    * On OS X, the build of the shared library is broken for all versions after and including GLPK 4.37.

GLPK 4.33
    * This release had a bug in the integer-solver routines that make parameters cause GLPK to crash. This was fixed in subsequent versions.

GLPK 4.31
    * Addition of a ``LPX.DUAL`` constant for the ``meth`` keyword parameter of the ``LPX.simplex`` solver method.
    * Addition of the ``LPX.SF_GM``, ``LPX.SF_EQ``, ``LPX.SF_2N``, ``LPX.SF_SKIP``, and ``LPX.SF_AUTO`` constants, for the ``LPX.scale`` method's argument.

GLPK 4.23
    * Addition of the ``gmi_cuts`` keyword parameter for the ``LPX.integer`` solver.

GLPK 4.23
    * Addition of the ``mir_cuts`` keyword parameter for the ``LPX.integer`` solver.

GLPK 4.21
    * The ability of the ``LPX.integer`` callback to have ``select``, ``prepro``, and ``branch`` methods called by the callback procedure. For the same reasons, the ``Tree`` methods ``select``, ``can_branch``, and ``branch_upon`` do not appear until this version.
    * Addition of the ``pp_tech`` keyword parameter for the ``LPX.integer`` solver, and associated constants.

GLPK 4.20
    * The keyword parameters of the ``LPX.integer`` MIP solver method including callbacks, and associated constants for the many keyword parameters, as well as all types associated therein, most notably ``Tree`` and ``TreeNode``.

GLPK 4.19
    * The ``Environment.mem_limit`` attribute for setting the maximum number of megabytes the GLPK will allocate.

