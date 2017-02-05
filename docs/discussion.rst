=======================
Discussion About PyGLPK
=======================

-------------
Existing Work
-------------

A Python binding to GLPK `already exists
<http://www.ncc.up.pt/~jpp/code/python-glpk/>`_ in the form of Python-GLPK, but
as it is automatically created through `SWIG <http://www.swig.org/>`_ it is not
very `Pythonic <http://faassen.n--tree.net/blog/view/weblog/2005/08/06/0>`_.

-----------------------
Building and Installing
-----------------------

Building this module requires that the user have installed the `GLPK
<http://www.gnu.org/software/glpk/>`_ and `GMP <http://gmplib.org/>`_
libraries. The module builds and appears to work on my simple test files in
Python 2.3, 2.4, and 2.5. Earlier versions of Python will not work.

In the ideal case (e.g., you have GLPK and GMP libraries and headers installed
in default search paths), one would install this through ``make`` and ``make
install``, and be done.

The section on :doc:`Building and Installing <./building-installing>` has more
information.

-------------------------------------
High Level Comparison of C and PyGLPK
-------------------------------------

To use the C API, one would first ``#include "glpk.h"``, create an LPX
structure through ``lpx_create_prob()``, and thence manipulate this structure
with ``lpx_*`` functions to set the data, run the optimization, and retrieve
the desired values.

To use this Python module, one would import the ``glpk`` module, create an LPX
Python object through ``glpk.LPX()``, and thence manipulate this object and the
objects it contains to set the data, run the optimization, and retrieve the
desired values. The Python module calls the C API to support these operations.

-----------------
Design Philosophy
-----------------

PyGLPK has objects floating around everywhere, and very few actual methods.
Wherever possible and sensible, one gets and sets traits of the problem by
accessing a method and directly assigning those traits. An example of this
showing how PyGPLK works and how it does not work might be interesting.

PyGLPK is like this

.. code:: python

    lp.maximize = True
    lp.cols.add(10)
    for col in lp.cols:
        col.name = 'x%d' % col.index
        col.bounds = 0.0, None
        lp.obj[col.index] = 1.0
    del lp.cols[2,4,5]

It isn't like this

.. code:: python

    lp.set_maximize()
    lp.add_cols(10)
    for cnum in xrange(lp.num_cols()):
        lp.set_col_name(cnum, 'x%d' % cnum)
        lp.set_col_bounds(cnum, 0.0, None)
    lp.set_obj_coef(cnum, 1.0)
    lp.del_cols([2,4,5])

Both design strategies would accomplish the same thing, but there are
advantages in the first way. For example, if I tell you only that columns of an
LP are stored in a sequence ``cols``, for free you already know a lot (assuming
you're familiar with Python):


* You know how to get the number of columns in the LP.

* You know how to get a particular column or a range of columns.

* You know they're indexed from 0.

* You know how to delete them.

-----------------------------------------
Differences Between C GLPK API and PyGLPK
-----------------------------------------

^^^^^^^^
Indexing
^^^^^^^^

Unlike the C API, everything is indexed from 0, not 1: the user does not pass
in arrays (or lists!) where the first element is ignored. Further, one indexes
(for example) the first row by asking for row 0, not 1.

In the comparative examples of parallel C and Python code, wherever possible
and appropriate I sprinkle ``+1`` in the C code. Of course, only a lunatic
would really write code that way, but I do this to highlight this difference,
and second, make it more obvious which portions of C and Python correspond to
each other: it's far easier to see the relation between ``[7, 3, 1, 8, 6]`` and
``[7+1, 3+1, 1+1, 8+1, 6+1]``, versus ``[8, 4, 2, 9, 7]``.

PyGLPK also honors Python's quirk of "negative indexing" used to count from the
end of a sequence, e.g., where index -1 refers to the last item, -2 second to
last item, and so forth. This can be convenient. For example, after adding a
row, you can refer to this row by ``lp.rows[-1]`` rather than having to be
aware of its absolute index.

^^^^^^^^^^^^^^
Error Handling
^^^^^^^^^^^^^^

The GLPK's approach to errors in arguments is deeply peculiar. It writes an
error message and terminate the process, in contrast with many APIs that
instead set or return an error code which can be checked. The PyGLPK takes the
more Pythonic approach of throwing catchable exceptions for illegal arguments.
Unfortunately, to avoid the process being terminated, this requires that
absolutely every argument be vetted, requiring that PyGLPK have the additional
overhead of doing sometimes rather detailed checks of arguments (which are, of
course, checked yet again when the GLPK has access to them). It seems unlikely
that GLPK's design will be improved in this area.

==============
Simple Example
==============

To ground you, we show an example of the module's workings. (This example is
covered in more detail in the examples section.)  Taking the introductory
example from the GLPK C API reference manual, we start with the following
example linear program:

.. math::

    \begin{aligned}
    & \underset{\mathbf{x}}{\text{maximize}} & & Z = 10 x_0 + 6 x_1 + 4 x_2 & \\
    & \text{subject to} & & p = x_0 + x_1 + x_2 & \\
    & & & q = 10 x_0 + 4 x_1 + 5 x_2 & \\
    & & & r = 2 x_0 + 2 x_1 + 6 x_2 & \\
    & \text{and bounds of variables} & & - \infty \lt p \leq 100 & 0 \leq x_0 \lt \infty \\
    & & & - \infty \lt q \leq 600 & 0 \leq x_1 \lt \infty \\
    & & & - \infty \lt r \leq 300 & 0 \leq x_2 \lt \infty \\
    \end{aligned}

In the following, we show Python code to define and solve this problem, and
subsequently print out the objective function value as well as the primal
values of the structural variables.

.. code:: python

    import glpk                         # Import the GLPK module

    lp = glpk.LPX()                     # Create empty problem instance
    lp.name = 'sample'                  # Assign symbolic name to problem
    lp.obj.maximize = True              # Set this as a maximization problem
    lp.rows.add(3)                      # Append three rows to this instance
    for r in lp.rows:                   # Iterate over all rows
        r.name = chr(ord('p')+r.index)  # Name them p, q, and r
    lp.rows[0].bounds = None, 100.0     # Set bound -inf < p <= 100
    lp.rows[1].bounds = None, 600.0     # Set bound -inf < q <= 600
    lp.rows[2].bounds = None, 300.0     # Set bound -inf < r <= 300
    lp.cols.add(3)                      # Append three columns to this instance
    for c in lp.cols:                   # Iterate over all columns
        c.name = 'x%d' % c.index        # Name them x0, x1, and x2
        c.bounds = 0.0, None            # Set bound 0 <= xi < inf
    lp.obj[:] = [10.0, 6.0, 4.0]        # Set objective coefficients
    lp.matrix = [
        1.0, 1.0, 1.0,                  # Set nonzero entries of the
        10.0, 4.0, 5.0,                 # constraint matrix.  (In this
        2.0, 2.0, 6.0                   # case, all are non-zero.)
    ]
    lp.simplex()                        # Solve this LP with the simplex method
    print 'Z = %g;' % lp.obj.value,     # Retrieve and print obj func value
    print '; '.join(                    # Print struct variable names and primal values
        '%s = %g' % (c.name, c.primal) for c in lp.cols
    )


This may produce this output.

.. code::

    Z = 733.333; x0 = 33.3333; x1 = 66.6667; x2 = 0
