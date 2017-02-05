------------------------
Reference Manual Example
------------------------

In this section we solve how to define and solve the example given at the
beginning of the GLPK C API reference manual.

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

^^^^^^^^^^^^^^^^^^
The Implementation
^^^^^^^^^^^^^^^^^^

Here is the implementation of that linear program within PyGLPK:

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
        10.0, 4.0, 5.0,                 # constraint matrix. (In this
        2.0, 2.0, 6.0                   # case, all are non-zero.)
    ]
    lp.simplex()                        # Solve this LP with the simplex method
    print 'Z = %g;' % lp.obj.value,     # Retrieve and print obj func value
    print '; '.join(                    # Print struct variable names and
                                        # primal values
        '%s = %g' % (c.name, c.primal) for c in lp.cols
    )

^^^^^^^^^^^^^^^
The Explanation
^^^^^^^^^^^^^^^

We shall now go over this implementation section by section.

.. code:: python

    import glpk

First we need the module of course.

.. code:: python

    lp = glpk.LPX()                     # Create empty problem instance

Here we construct a new linear program object with a call to the LPX
constructor.

.. code:: python

    lp.name = 'sample'                  # Assign symbolic name to problem

Linear program objects have various attributes. One of them is ``name``, which
holds a symbolic name for the program. Initially a program has no name, and
``name`` correspondingly has the value ``None``. Here we name the program
``'sample'``. Programs do not necessarily require names, but a user may wish to
give a program a name nonetheless.

.. code:: python

    lp.obj.maximize = True              # Set this as a maximization problem

Linear program objects contain other less trivial attributes. One of the most
important is ``obj``, an object representing the linear program's objective
function. In this case, we are assigning ``lp.obj``'s ``maximize`` attribute
the value ``True``, informing out linear program that we want to maximize our
objective function.

.. code:: python

    lp.rows.add(3)                      # Append three rows to this instance

Another very important component of ``lp`` is the ``rows`` attribute, holding
an object which indexes over the rows of this linear program. In this case, we
call the ``lp.rows`` method ``add``, telling it to add three rows to the linear
program.

.. code:: python

    for r in lp.rows:                   # Iterate over all rows
        r.name = chr(ord('p')+r.index)  # Name them p, q, and r

The ``lp.rows`` object is also used for accessing particular rows. In this
case, we are iterating over each row. In the course of this iteration, ``r``
holds the first, second, and third row. We want to name these rows ``'p'``,
``'q'``, and ``'r'``, in order.

Note that an individual row is an object in itself. It also has a ``name``
attribute, to which we assign the character with ASCII value of ``p`` plus
whatever the index of this row is. The first row has ``index`` of 0, the next
1, the next and last 2. So, this will give us the desired names.

.. code:: python

    lp.rows[0].bounds = None, 100.0     # Set bound -inf < p <= 100
    lp.rows[1].bounds = None, 600.0     # Set bound -inf < q <= 600
    lp.rows[2].bounds = None, 300.0     # Set bound -inf < r <= 300

In addition to iterating over all rows, we can access a particular row by
indexing the ``lp.rows`` object. In this case we index by the numeric row
index. (Now that we have set their names, we could alternatively index them by
their names!)

In this case, we are using the row's ``bounds`` attribute to set the bounds for
the corresponding auxiliary variable. Bounds consist of a lower and upper
bound. In this case, we are specifying that we always want the lower end
unbounded (by assigning ``None``, indicating no bound in that direction), and
otherwise setting an appropriate upper bound.

.. code:: python

    lp.cols.add(3)                      # Append three columns to this instance

In addition to the ``rows`` object, there is also a ``cols`` object for
creating and accessing columns. Indeed, the two objects have the same type. In
this case, we see we are adding three columns to the linear program.

.. code:: python

    for c in lp.cols:                   # Iterate over all columns
        c.name = 'x%d' % c.index        # Name them x0, x1, and x2
        c.bounds = 0.0, None            # Set bound 0 <= xi < inf

Similar to how we iterated over and assigned names to the rows, in this case we
assign appropriate names to our columns. We also assign bounds to each column's
associated structural variable, though in this case we want each structural
variable to be greater than 0, and have no upper bound.

.. code:: python

    lp.obj[:] = [10.0, 6.0, 4.0]        # Set objective coefficients

There is one objective coefficient for every column. In this, we set all the
coefficients at once to their desired values. Note that these ``lp.obj``
objects act like sequences over the objective coefficient values, just as the
row and column collections do over rows and the columns.

.. code:: python

    lp.matrix = [
        1.0, 1.0, 1.0,                  # Set nonzero entries of the
        10.0, 4.0, 5.0,                 # constraint matrix. (In this
        2.0, 2.0, 6.0                   # case, all are non-zero.)
    ]

We are setting the non-zero entries of the coefficient constraint matrix by
assigning to the linear program's ``matrix`` attribute. Matrix entries are
either (1) values, or (2) tuples specifying the row index, column index, and
value. In the first case, if it is just a value with the indices omitted, it
assumes that the value specified is for the next value in the constraint
matrix, read top to bottom, left to right. We could also have explicitly
defined the indices with this equivalent statement:

.. code:: python

    lp.matrix = [
        (0, 0, 1.0), (0, 1, 1.0), (0, 2, 1.0),
        (1, 0, 10.0), (1, 1, 4.0), (1, 2, 5.0),
        (2, 0, 2.0), (2, 1, 2.0), (2, 2, 6.0)
    ]

But we did not. Let's move on.

.. code:: python

    lp.simplex()                        # Solve this LP with the simplex method

Here we are calling a simplex solver to solve the defined linear program!

.. code:: python

    print 'Z = %g;' % lp.obj.value,     # Retrieve and print obj func value
    print '; '.join(                    # Print struct variable names and
                                        # primal values
        '%s = %g' % (c.name, c.primal) for c in lp.cols
    )

After optimization, we want to print out the value of the objective function
(as stored in ``lp.obj.value``), and the value of the primal variable for each
of the columns (as stored in each column's ``primal`` attribute).

This all results in this output.

.. code::

    Z = 733.333; x0 = 33.3333; x1 = 66.6667; x2 = 0
