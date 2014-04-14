                              PyGLPK Release Notes

   Copyright (c) 2007, 2008, Thomas W. Finley

PyGLPK 0.3

   September 22 2008
     * The PyGLPK has been updated to build against GLPK 4.31, and has been
       updated to take advantage of many of the features of the new version.
       Nearly all feature changes in this version are to reflect changes in
       the PyGLPK. This version of PyGLPK will not build against anything
       prior to GLPK 4.18.
     * It is no longer necessary to set the kind of a linear program
       explicitly to int to make it a MIP.
     * The Environment class has been added, with a single instance env at
       the root level of the glpk module. With this class one may monitor and
       control memory usage and terminal output.
     * The simplex and integer solvers now accept many keyword parameters to
       control their functionality.
     * Support for callbacks to control the mixed integer programming
       solution search have been addeed. See the Tree and TreeNode types.
     * The Params class has been removed. Much of its functionality has been
       obviated as parameters are passed directly to the solvers, and the
       functionality of the Environment class. (If you wish to build a
       version of PyGLPK with the Params type as it was in previous versions,
       you may edit useparams = False to be useparams = True.)
     * It is now possible to manually change the scaling of rows and columns.

PyGLPK 0.2

   July 9 2007
     * Added weak reference support to all the PyGLPK objects.
     * Testing code is now more exhaustive, and with more testing of smaller
       components rather than exclusively testing PyGLPK's ability to solve
       full problems.
     * Fixed freeing some Python objects too early: When assigning values to
       the constraint matrix, matrix value reference counts were not
       incremented correctly when using implicit indexing (i.e., when the
       index of an entry was inferred from the previous entry) in matrix
       assignments to both the entire matrix and individual rows/columns.
     * The integer parameter outfrq was erroneously identified as being a
       float parameter in code.

PyGLPK 0.1

   March 28 2007
     * First release.

     ----------------------------------------------------------------------

   Thomas Finley, 2007, 2008
