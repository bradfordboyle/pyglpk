====================
Object Documentation
====================

This document explains the function of the classes defined in PyGLPK. In
addition to descriptions, for the benefit of those familiar with the C API, it
proceeds by example and analogy to the C API: there is often some C code in one
color followed by some Python code in a different color which tries to do the
same thing, in this spirit:

.. code-block:: c

    printf("Hello world!");
    total = 0;
    for (i=0; i<10; ++i) total += i*i;

.. code-block:: python

    print "Hello world!"
    total = sum(i*i for i in range(10))

Hopefully, in the case of multiline examples, it should be obvious which lines
correspond to each other.

Reading this document through is not necessarily the best way to gain an
understanding of PyGLPK if you are completely new to it. This is organized in
order of object above any other consideration: advanced obscure subjects
relating to LPX objects occur before simple functionality of row and column
objects. Rather, this is intended to be a reference.

--------------
Linear Program
--------------

The most basic object in PyGLPK is the LPX class, just as in the GLPK C API it
is the LPX structure. As a rule, it holds data and methods that are relevant to
the linear problem as a whole. Throughout the code in this document, the token
``lp`` refers to an LPX object.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Creating, Erasing, and Deleting Problems
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One may create the Python instance of an LPX problem with a constructor. An LPX
that has been modified can be restored to its original pristine state with a
call to the ``erase`` method. One may explicitly delete the object. Note that
one does not need to delete the LPX object as it should be automatically
garbage collected, though it may be desirable to do so, just as it is sometimes
desirable to free a Python object without waiting until it falls out of scope.

.. code-block:: c

    lp = glp_create_prob();
    glp_erase_prob(lp);
    glp_delete_prob(lp);

.. code-block:: python

    lp = glpk.LPX()
    lp.erase()
    del(lp)

If you have maintained objects which point back to the LPX object (e.g., rows,
columns, objective function), the LPX can be deleted, but the underlying object
shall not be freed until these references are discarded. In other words, there
is nothing special about deleting the object versus any other Python object.
The point is that ``del(lp)`` will not necessarily result in deallocation of
the LPX structure.

^^^^^^^^^^^^^^^
Naming Problems
^^^^^^^^^^^^^^^

One gets and sets the name for a problem by querying or assigning to the
``name`` attribute. As with the C API, names are limited to 255 characters.
Unset names are ``None``. One unsets a name by assigning ``None`` or deleting
the ``name``.

.. code-block:: c

    glp_set_prob_name(lp, "some name");
    glp_set_prob_name(lp, NULL);
    char *thename = glp_get_prob_name(lp);

.. code-block:: python

    lp.name = "some name"
    del(lp.name)  # Alternately, lp.name = None
    thename = lp.name

^^^^^^^^^^^^^^^^^
Constraint Matrix
^^^^^^^^^^^^^^^^^

One gets and sets the entries of the entire constraint matrix by querying or
assigning the attribute ``matrix``. Assignments to ``matrix`` will remove
previous entries.

Retrieving ``matrix`` will yeild a list of three element ``(int, int, float)``
tuples over the non-zero entries in this matrix, each of the form ``(ri, ci,
value)``, indicating that row ``ri`` and column ``ci`` (counting from 0) hold
value ``value``.

For example, consider if ``lp`` encoded the constraint matrix:

.. math::

    \begin{aligned}
    & p = x_0 + x_1 + x_2 & \\
    & q = 10 x_0 + 4 x_1 + 5 x_2 & \\
    & r = 2 x_0 + 6 x_2 & \\
    \end{aligned}

Then ``print lp.matrix`` outputs ``[(0, 0, 1.0), (0, 1, 1.0), (0, 2, 1.0), (1,
0, 10.0), (1, 1, 4.0), (1, 2, 5.0), (2, 0, 2.0), (2, 2, 6.0)]``.

For setting rather than getting, one may set all non-zero entries of the
constraint matrix by assigning an iterable with similar structure to the
``matrix`` attribute. The iterable must yield values each in one of these two
forms:

- The integer-integer-float tuple ``(ri, ci, value)`` where ``index`` >= 0
  specifies that element ``index`` should have value ``value`` (negative
  indices are permitted in this context if you like)
- The single float item ``value`` which specifies an object equivalent to
  ``(ri, ci+1, value)`` (or ``(ri+1, 0, value)`` if ``ci+1`` goes past the end
  of the column) where ``ri, ci`` was the last location considered. If this
  single-value form is used on the first entry, the location 0, 0 is
  assumed.

Indices out of bounds will result in an ``IndexError`` and duplicate indices
will result in an ``ValueError``. Order does not matter, except of course for
single value entries, as their location depends on the previous entry.

One may set all entries of a row or column in the constraint matrix to zero by
assigning ``None`` to or deleting the ``matrix`` attribute.

Suppose we wanted to set rather than get the earlier matrix.

.. code-block:: c

    int    ia[] = {0+1, 0+1, 0+1,  1+1, 2+1, 1+1, 1+1, 2+1};
    int    ja[] = {0+1, 1+1, 2+1,  0+1, 0+1, 1+1, 2+1, 2+1};
    double ar[] = {1.0, 1.0, 1.0, 10.0, 2.0, 4.0, 5.0, 6.0}
    glp_load_matrix(lp, sizeof(ia), ia, ja, ar);

.. code-block:: python

    lp.matrix = [(0, 0, 1.0), (0, 1, 1.0), (0, 2, 1.0), (1, 0, 10.0),
                (2, 0, 2.0), (1, 1, 4.0), (1, 2, 5.0), (2, 2, 6.0)]

One could also do the following.

.. code-block:: python

    lp.matrix = [ 1.0, 1.0, 1.0,
                10.0, 4.0, 5.0,
                2.0, 0.0, 6.0 ]

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Non-Zero Constraint Matrix Entries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets the number of non-zero constraint matrix entries by querying the
``nnz`` integer attribute.

.. code-block:: c

    int numnonzero = glp_get_num_nz(lp);


.. code-block:: python

    numnonzero = lp.nnz

^^^^^^^^^^^^^^^^
Basis Definition
^^^^^^^^^^^^^^^^

The user may want to define the initial LP basis prior to starting simplex
optimization. There are several automatic ways of constructing this basis.

- ``std_basis`` method constructs a trivial LP basis.
- ``adv_basis`` method constructs an advanced LP basis that tries to have as
  few fixed variables as possible while maintaining the triangularity of the
  basis matrix.
- ``cpx_basis`` method constructs an advanced LP basis as described in R.
  Bixby. "Implementing the Simplex method: The initial basis." *ORSA Journal on
  Computing*, 4(3), 1992.
- ``read_basis`` reads a basis stored in the fixed MPS file format from a given
  file name. If this method fails, it throws a ``RuntimeError``.

.. code-block:: c

    glp_std_basis(lp);
    glp_adv_basis(lp, 0);
    glp_cpx_basis(lp);
    lpx_read_basis(lp, "/path/to/file");

.. code-block:: python

    lp.std_basis()
    lp.adv_basis()
    lp.cpx_basis()
    lp.read_basis("/path/to/file")

^^^^^^^
Scaling
^^^^^^^

Prior to optimization, it is often help to scale your problem, in part to avoid
numerical instability. The method ``scale`` tells the linear program to
transform the program into an alternate equivalent formulation with better
numerical properties. **Note that this transformation is transparent to the
user.** This is a matter of internal representation used to help the solver.
This procedure obeys the following flags defined as integers in the LPX class,
which can be ORed together to produce a combination of effects:

``SF_GM``
  perform geometric mean scaling
``SF_EQ``
  perform equilibration scaling
``SF_2N``
  round scale factors to the nearest power of two
``SF_SKIP``
  skip scaling, if the problem is well scaled
``SF_AUTO``
  choose scaling options automatically

By using the ``unscale`` method, one can cancel any previous scaling.

.. code-block:: c

    glp_scale_prob(lp, GLP_SF_AUTO);
    glp_scale_prob(lp, GLP_SF_GM | GLP_SF_2N);
    glp_unscale_prob(lp);

.. code-block:: python

    lp.scale()
    lp.scale(LPX.SF_GM | LPX.SF_2N)
    lp.unscale()

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Problem Kind, Continuous or Mixed Integer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets the kind of problem (the default linear program or mixed integer) by
querying the attribute ``kind``. This will hold either ``float`` if this is a
pure linear program (LP), or ``int`` if this is a mixed integer program (MIP)
by having any integer or binary column variables. A linear program becomes a
mixed integer program by having some of its columns <a href="#rc_mip">assigned
to either ``bin`` or ``int`` kind.

.. code-block:: c

    int thekind = lpx_get_class(lp);

.. code-block:: python

    thekind = lp.kind

^^^^^^^^^^^^^^^^^^^^^^^^^^
Integer and Binary Columns
^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets the number of integer and binary (i.e., integer with 0, 1 bounds)
column variables by querying the ``nint`` and ``nbin`` integer attributes,
respectively. If this is not a mixed integer problem, these attributes always
hold 0.

.. code-block:: c

    int num_int = glp_get_num_int(lp);
    int num_bin = glp_get_num_bin(lp);

.. code-block:: python

    num_int = lp.nint
    num_bin = lp.nbin

^^^^^^^^^^^^^^^^^^^
Solving the Problem
^^^^^^^^^^^^^^^^^^^

When it comes time to actually solving a linear program, one calls a
``lp.solver()`` method, where ``solver`` refers to one of several solver
methods. There are several choices available.

- ``simplex`` is a standard simplex method.
- ``exact`` is an `exact<http://gmplib.org/>`_ simplex method.
- ``interior`` is an interior point method.
- ``integer`` is a method that uses a branch-and-bound based method to solve a
  mixed integer program (MIP). This method requires an existing optimal basic
  solution as acquired through either ``simplex()`` or ``exact().``
- ``intopt`` is a more advanced branch-and-bound MIP solver. This does not
  require an existing optimal basic solution.


Return values are either ``None`` if the solver terminated normally, or a
string denoting one of several possible error messages. See help for each
method to review these possible return values.

Some of these solver routines may accept additional keyword parameters to
control the behavior of the underlying solver. See help for each method to
review possible control parameters and default values.

.. code-block:: c

    glp_simplex(lp, params);
    lpx_exact(lp);
    lpx_interior(lp);
    glp_intopt(lp, params);  // These two may be applied only to MIP problems
    lpx_intopt(lp);

.. code-block:: python

    lp.simplex()
    lp.exact()
    lp.interior()
    lp.integer()  # These two may be applied only to MIP problems
    lp.intopt()

Note, the solver not returning a message simply means that it terminated
without error. **It does not mean that an optimal solution or indeed any
solution was found!** For example, a solver could terminate without error if it
determines that there is no feasible solution.

^^^^^^^^^^^^^^^
Solution Status
^^^^^^^^^^^^^^^

One gets the solution status for the last solver by querying the ``status``
attribute. This takes the form of a string with several possible values.

- ``opt`` meaning the solution is optimal.
- ``undef`` meaning the solution is undefined.
- ``feas`` meaning the solution is feasible, but not necessarily optimal.
- ``infeas`` meaning the solution is infeasible.
- ``nofeas`` meaning the problem has no feasible solution.
- ``unbnd`` meaning the problem has an unbounded solution.

.. code-block:: c

    int stat = glp_get_status(lp);  // or glp_(ipt|mip)_status

.. code-block:: python

    stat = lp.status

Unlike the C API, PyGLPK remembers which solver was used last and retrieves the
corresponding status value. If for whatever reason you wish to retrieve the
status of a solver's solution other than what was used last, you may ask for
``status_s`` (for ``simplex`` and ``exact``), or ``status_i`` (for
``interior``), or ``status_m`` (for ``integer`` or ``intopt``).

Additionally, if one has used the simplex solver, one can get the primal and
dual status with the ``status_primal`` and ``status_dual`` attributes.

.. code-block:: c

    int pstat = glp_get_prim_stat(lp);
    int dstat = glp_get_dual_stat(lp);

.. code-block:: python

    pstat = lp.status_primal
    dstat = lp.status_dual

^^^
Ray
^^^

If, after running a simplex optimizer, your basic solution is unbounded, you
may retrieve the row or column corresponding to the non-basic variable causing
primal unboundedness within the attribute ``ray``. The meaning of this is that
corresponding variable is able to infinitely change in some unbounded direction
to improve the objective function.

.. code-block:: c

    int the_var_index = lpx_get_ray_info(lp);

.. code-block:: python

    row_or_col = lp.ray

^^^^^^^^^^
File Input
^^^^^^^^^^

In addition to programmatically defining a linear problem, there are methods to
read linear programs and MIPs from files. We have seen the empty LPX
constructor employed to create an empty problem. The LPX constructor also has
the ability to accept a single keyword argument: the keywords specifies a file
format, and the argument specifies the filename, as in
``lp=glpk.LPX(format=filename)``. If successfully read, an LPX instance will be
created with the file data.

All formats accept a single string representing the path to the file to be
read. Valid formats include the following.

- ``gmp`` for reading a model and a data file in the GNU MathProg modeling language
- ``mps`` for reading a fixed `MPS<http://en.wikipedia.org/wiki/MPS_(format)>`_
  formatted files
- ``freemps`` for reading a free MPS formatted file
- ``cpxlp`` for reading a CPLEX LP formatted file
- ``glp`` for reading a GNU LP formatted file

The format ``gmp`` (GNU MathProg), in addition to accepting a single string
argument, may optionally accept a three element tuple instead, containing these
elements:

- A file name argument specifying the GMP model file.
- A file name argument specifying the GMP data file. This may optionally be
  ``None`` if the data is included in the model file.
- A file name argument specifying the output file, where the output of any
  "display" statements in the GMP are output. This may optionally be ``None``
  to send output to standard output.

For the ``gmp`` option, if you input a single string ``filename`` instead of a
tuple, it is equivalent to inputing the tuple ``(filename, None, None)``.

.. code-block:: c

    lp = lpx_read_mps("/path/to/mps_file")
    lp = lpx_read_freemps("/path/to/free_mps_file")
    lp = lpx_read_cpxlp("/path/to/cplexlp_file")
    lp = lpx_read_model("modelfile", NULL, NULL)
    lp = lpx_read_model("modelfile", "datafile", "output.txt")
    lp = lpx_read_prob("/path/to/gnulp_file")

.. code-block:: python

    lp = glpk.LPX(mps="/path/to/mps_file")
    lp = glpk.LPX(freemps="/path/to/free_mps_file")
    lp = glpk.LPX(cpxlp="/path/to/cplexlp_file")
    lp = glpk.LPX(gmp="modelfile")
    lp = glpk.LPX(gmp=("modelfile", "datafile", "output.txt"))
    lp = glpk.LPX(glp="/path/to/gnulp_file")

^^^^^^^^^^^
File Output
^^^^^^^^^^^

One may export data about a linear program to a file in a variety of formats
conveying a variety of different types of information using the method
``write``. The method accepts a large number of keyword arguments: each keyword
specifies a file format, and the argument a file name, as in
``lp.write(format=filename)``. Upon invocation, the LPX object will attempt to
write the data specified by the format into the indicated file.

Valid formats include the following.

- ``mps`` for problem data in the fixed MPS format.
- ``bas`` for the LP basis in fixed MPS format.
- ``freemps`` for problem data in the free MPS format.
- ``cpxlp`` for problem data in the CPLEX LP format.
- ``glp`` for problem data in the GNU LP format.
- ``prob`` for problem data in a plain text format.
- ``sol`` for basic solution in printable format.
- ``sens_bnds`` for bounds sensitivity information.
- ``ips`` for interior-point solution in printable format.
- ``mip`` for MIP solution in printable format.

Note that you can specify multiple formats and output files in a single call to
``write`` in order to write multiple files in multiple formats in one go. For
example, you might want to simultaneously write out printable problem data,
solutions, and bounds sensitivity information all in one go with something like
``lp.write(prob="foo.prob", sol="foo.sol", sens_bnds="foo.bnds")`` .

.. code-block:: c

    lpx_write_mps(lp, filename)
    lpx_write_bas(lp, filename)
    lpx_write_freemps(lp, filename)
    lpx_write_prob(lp, filename)
    lpx_write_cpxlp(lp, filename)
    lpx_print_prob(lp, filename)
    lpx_print_sol(lp, filename)
    lpx_print_sens_bnds(lp, filename)
    lpx_print_ips(lp, filename)
    lpx_print_mip(lp, filename)

.. code-block:: python

    lp.write(mps=filename)
    lp.write(bas=filename)
    lp.write(freemps=filename)
    lp.write(glp=filename)
    lp.write(cpxlp=filename)
    lp.write(prob=filename)
    lp.write(sol=filename)
    lp.write(sens_bnds=filename)
    lp.write(ips=filename)
    lp.write(mip=filename)

------------------
Objective Function
------------------

A linear program objective function specifies what linear function the LP is
attempting to either minimize or maximize. Correspondingly, the objective
object allows one to set objective function coefficients and the direction of
optimization, and retrieve the objection function value after optimization.

The objective function for an LPX object ``lp`` is contained within ``lp.obj``.
This objects is an instance of the ``Objective`` class. Through this object one
can set the objective coefficients and retrieve the objective value.

^^^^^^^^^^^^^^^^^^^^^^^^^
Naming Objective Function
^^^^^^^^^^^^^^^^^^^^^^^^^

Similar to how one names problems, one gets and sets the name for the objective
function by querying or assigning to the ``name`` attribute. As with the C API,
names are limited to 255 characters. Unset names are ``None``. One unsets a
name by assigning ``None`` or deleting the ``name``.

.. code-block:: c

    glp_set_obj_name(lp, "some name");
    glp_set_obj_name(lp, NULL);
    char *thename = glp_get_obj_name(lp);

.. code-block:: python

    lp.obj.name = "some name"
    del lp.obj.name
    thename = lp.obj.name

^^^^^^^^^^^^^^^^^^^^
Minimize or Maximize
^^^^^^^^^^^^^^^^^^^^

One gets and sets whether this is a minimization or maximization problem by
querying or assigning to the ``maximize`` boolean attribute.

.. code-block:: c

    glp_set_obj_dir(lp, GLP_MIN);
    glp_set_obj_dir(lp, GLP_MAX);
    int ismax = (glp_get_obj_dir(lp) == GLP_MAX);

.. code-block:: python

    lp.obj.maximize = False
    lp.obj.maximize = True
    ismax = lp.obj.maximize

^^^^^^^^^^^^^^^^^^^^^^
Objective Coefficients
^^^^^^^^^^^^^^^^^^^^^^

One gets and sets the objective function coefficients by indexing into the
``obj`` object, e.g., ``lp.obj[index]``. There are as many objective
coefficients as there are columns, so valid indices include ``0`` through
``len(lp.cols)-1`` as well as (for negative indexing) ``-1`` through
``-len(lp.cols)``.

One can access and change these objective coefficients through either a single
index, or access or change multiple coefficients by defining multiple indices
through either a series of indices or a slice.

When assigning new objective coefficients, valid assignments include single
numbers (in which case all indexed coefficients receive this same value) or an
iterable object (in which case all indexed coefficients receive values
specified in turn).

The objective function's constant shift term can be accessed either by using
``None`` as an index, or by accessing the ``shift`` attribute, that is,
``lp.obj.shift``.

.. code-block:: c

    glp_set_obj_coef(lp, 2+1, 3.0);
    for (i=0; i&lt;glp_get_num_cols(lp); ++i)
        glp_set_obj_coef(lp, i+1, 1.0)
    glp_set_obj_coef(lp, 0+1, 3.14159); glp_set_obj_coef(lp, 2+1, -2.0);
    glp_set_obj_coef(lp, 0, 0.5);
    glp_set_obj_coef(lp, glp_get_num_cols(lp), 25.0);
    double c = glp_get_obj_coef(lp, 3+1);
    double c1 = glp_get_obj_coef(lp, 1+1), c2 = glp_get_obj_coef(lp, 2+1);

.. code-block:: python

    lp.obj[2] = 3.0
    lp.obj[:] = 1.0
    lp.obj[0,2] = 3.14159, -2.0
    lp.obj.shift = 0.5  # Alternately, lp.obj[None] = 0.5
    lp.obj[-1] = 25.0
    c = lp.obj[3]
    c1, c2 = lp.obj[1,2]

^^^^^^^^^^^^^^^^^^^^^^^^
Objective Function Value
^^^^^^^^^^^^^^^^^^^^^^^^

One gets the value for the objective function by querying the ``value``
attribute.

.. code-block:: c

    double oval = glp_get_obj_val(lp); //  or glp_(ipt|mip)_obj_val

.. code-block:: python

    oval = lp.obj.value

Unlike the C API, PyGLPK remembers which solver was used last and retrieves the
corresponding objective function value. If for whatever reason you wish to
retrieve an objective function from a solver type different from what you used
last, you can force the issue by asking for ``value_s`` (for ``simplex`` and
``exact``), or ``value_i`` (for ``interior``), or ``value_m`` (for ``integer``
or ``intopt``).

.. code-block:: c

    double soval = glp_get_obj_value(lp);
    double ioval = glp_ipt_obj_value(lp);
    double moval = glp_mip_obj_value(lp);

.. code-block:: python

    soval = lp.obj.value_s
    ioval = lp.obj.value_i
    moval = lp.obj.value_m

----------------
Rows and Columns
----------------

In a linear program, rows and columns correspond to variables. Correspondingly,
individual rows and column objects contain methods and data pertaining to
individual variables: bounds, values after optimization, status, relevant
entries of the constraint matrix, and other such objects.

Rows and columns live all live within two objects stored within an LPX object
``lp`` as ``lp.rows`` and ``lp.cols``. Both of these objects is an instance of
the ``BarCollection`` class. Individual rows and columns, all of type ``Bar``,
can be accessed by indexing or iteration over these collections.

^^^^^^^^^^^^^^^^^^^^^^^
Adding Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^

To add rows or columns, call the ``add`` method on either the ``row`` or
``column`` subcontainer. As in the C API, the newly created rows and columns
are initially empty, and the return value of the ``add`` method holds the first
newly valid index.

.. code-block:: c

    int rnew = glp_add_rows(lp, nrs);
    int cnew = glp_add_cols(lp, ncs);

.. code-block:: python

    rnew = lp.rows.add(nrs)
    cnew = lp.cols.add(ncs)

^^^^^^^^^^^^^^^^^^^^^^^^^^
Number of Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets the number of rows or columns by querying the length of the LP's
``row`` and ``column`` containers.

.. code-block:: c

    int nrs = glp_get_num_rows(lp);
    int ncs = glp_get_num_cols(lp);

.. code-block:: python

    nrs = len(lp.rows)
    ncs = len(lp.cols)

^^^^^^^^^^^^^^^^^^^^^^^^^
Indexing Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^^^

One accesses particular rows and columns by indexing into the ``lp.rows`` and
``lp.cols`` collections. For example, ``lp.rows[ri]`` returns the row at index
``ri``. This index may also be a negative index counting backwards from the end
of the collection, e.g., ``lp.cols[-1]`` to get the last column of the LP.

These structures adopt much of the familiar behavior of Python sequences. Among
other implications, this means that unlike in the C API, rows and columns are
indexed from 0.

As we shall see, rows and columns can be named. One may also index named rows
and columns by their names.

.. code-block:: c

    int rownum = glp_find_row(lp, "rowname")

.. code-block:: python

    row = lp.rows["rowname"]

In addition to single integer or string values, one may specify multiple values
in this index to retrieve a list of all specified rows or columns.

.. code-block:: python

    lp.cols[2,5,"bob",6]  # columns 2, 5, one named "bob", 8

Indexing by slicing is supported as well. This will result in a list of all
indices specified by the slice.

.. code-block:: python

    lp.rows[4:9]  # rows 4 through 8
    lp.cols[-3:]  # the last 3 columns
    lp.rows[::2]  # every row with an even index

One may also iterate over the ``lp.rows`` and ``lp.cols`` collections. Here is
a comparative example of setting each column to name ``"x%d"`` so the columns
will be named ``x0``, ``x1``, ``x2``, etc.

.. code-block:: c

    char buff[10];
    for (i=1; i<=glp_get_num_cols(lp); ++i) {
        snprintf(buff, sizeof(buff), "x%d", i-1);
        glp_set_col_name(lp, i, buff);
    }

.. code-block:: python

    for col in lp.cols:
        col.name = "x%d" % col.index

^^^^^^^^^^^^^^^^^^^^^^^
Naming Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^

As with the problem and the objective function, one gets and sets the name for
a row or column by querying or assigning the attribute ``name``.

Note the use of an index into the ``rows`` or ``cols`` collections to retrieve
a particular row or columns. As with the C API, indices are integral, though we
count from 0.

.. code-block:: c

    glp_set_row_name(lp, ri+1, "row name");
    glp_set_col_name(lp, ci+1, "col name");
    char *rname = glp_get_row_name(lp, ri+1);
    char *cname = glp_get_col_name(lp, ci+1);

.. code-block:: python

    lp.rows[ri].name = "row name"
    lp.cols[ci].name = "col name"
    rname = lp.rows[ri].name
    cname = lp.cols[ci].name

After the user names a row or column, they may index this row or column by its
name.

.. code-block:: python

    lp.rows[ri].name = "xi"
    therow = lp.rows["xi"]

^^^^^^^^^^^^^^^^^^^^^^^^^
Bounding Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^^^

One gets and sets the bounds for a row or column by querying or assigning the
attribute ``bounds``. To set bounds, one may assign one or two values to the
``bounds``, where values are either ``None`` or numeric.

One ``None`` (or two ``None``s) sets the row's auxiliary (or column's
structural) variable unbounded. (One may also delete the bounds.)  One numeric
value (or two equal numeric values) sets an equality bound. In the case of two
values, the first is interpreted as a lower bound, the second as an upper
bound, with ``None`` indicating unboundedness in that direction. Setting a
lower bound greater than an upper bound causes a ``ValueError``.

In this code, we see instances of setting free (unbounded), lower, upper,
double, and fixed (equality) bounds, respectively on a row and column.

.. code-block:: c

    glp_set_row_bnds(lp, ri+1, GLP_FR,  0,   0);
    glp_set_row_bnds(lp, ri+1, GLP_LO,  2,   0);
    glp_set_col_bnds(lp, ci+1, GLP_UP,  0,   5);
    glp_set_col_bnds(lp, ci+1, GLP_DB, -1,   3.14159);
    glp_set_row_bnds(lp, ri+1, GLP_FX,  3.4, 3.4);

.. code-block:: python

    lp.rows[ri].bounds = None     # Or, lp.rows[ri].bounds = None, None
                                  # Or, del lp.rows[ri].bounds
    lp.rows[ri].bounds = 2, None
    lp.cols[ci].bounds = None, 5
    lp.cols[ci].bounds = -1, 3.14159
    lp.rows[ri].bounds = 3.4      # Or, lp.rows[ri].bounds = 3.4, 3.4

Accessing bounds always yields two values (again, either ``None`` or numeric)
representing lower and upper bounds respectively, even if the bounds resulted
from either a single value assignment or a deletion. Again, ``None`` represents
unboundedness in that direction.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Matrix Entries of Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets and sets the entries of a row or column in the constraint matrix by
querying or assigning the attribute ``matrix``. Assignments to ``matrix`` will
remove previous entries.

Retrieving ``matrix`` will yield a list of two element tuples over the non-zero
entries in this row or column, each of the form ``(index, value)``. The
``index`` is the index (counting from 0) of the entry holding value ``value``
in this row or column.

If we have an LPX object with :math:`r` rows and :math:`c` columns, then valid
indices for rows are 0 through :math:`c - 1` and valid entries for columns are
0 through :math:`r - 1`.

For example, suppose for an object ``lp`` the row :math:`r_2` (that is, row at
index 2) encodes the constraint:

.. math::

    p = 10x_0 1 3.14159x_1 + 0.5x_3

Then ``print lp.rows[2].matrix`` outputs ``[(0, 10.0), (1, -3.14159), (3,
0.5)]``.

One may set all non-zero entries of a row or column by assigning an iterable
with similar structure to the ``matrix`` attribute. Suppose our LPX object has
:math:`numr` rows and :math:`numc` columns. The iterable must yield values each
in one of these two forms:

- The integer-float tuple ``(index, value)`` which specifies that element
  ``index`` should have value ``value`` (note that negative indices are
  permitted in this context if you like)

- The single float item ``value`` which specifies an object equivalent to
  ``(index+1, value)`` where ``index`` was the last index used in this
  iterable, or 0 if this is the first object in the iterable

For example, where one interested in defining (rather than simply retrieving)
the entries of the constraint row used in the example above, if there are four
columns, all of the following are equivalent:

.. code-block:: python

    # Define constraint p = 10*x0 - 3.14159*x1 + 0.5*x3
    lp.rows[2].matrix = [(0, 10), (1, -3.14159), (3, 0.5)]
    lp.rows[2].matrix = [(0, 10), (1, -3.14159), (-1, 0.5)]
    lp.rows[2].matrix = [(1, -3.14159), (0, 10), (3, 0.5)]
    lp.rows[2].matrix = [10, -3.14159, 0, 0.5]
    lp.rows[2].matrix = [10, -3.14159, (-1, 0.5)]
    lp.rows[2].matrix = [10, -3.14159, (3, 0.5)]

Indices out of bounds will result in an ``IndexError`` and duplicate indices
will result in an ``ValueError``. Order does not matter, except of course for
single value entries, as their index depends on the previous entry.

One may set all entries of a row or column in the constraint matrix to zero by
assigning ``None`` to or deleting the ``matrix`` attribute.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Number of Non-Zero Constraint Row and Column Entries
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets the number of non-zero constraint elements within a row or a column by
querying the ``nnz`` integer attribute.

.. code-block:: c

    int rnnz = glp_get_mat_row(lp, ri+1, NULL, NULL);
    int cnnz = glp_get_mat_col(lp, ci+1, NULL, NULL);

.. code-block:: python

    rnnz = lp.row[ri].nnz
    cnnz = lp.col[ci].nnz

^^^^^^^^^^^^^^^^^^^^^^^^^
Deleting Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^^^

To delete rows or columns, delete as one would from a typical Python list. Note
the methods of indexing into the row and column collections. Accepted indices
include single values, lists of values, or slices.

.. code-block:: c

    int indices1[] = { 2+1 };
    glp_del_cols(lp, 1, indices1-1);
    int indices2[] = { 2+1,5+1,6+1 };
    glp_del_rows(lp, 3, indices2-1);
    int indices3[] = { 3+1, 4+1, 5+1, 6+1 };
    glp_del_cols(lp, 4, indices3-1);

.. code-block:: python

    del lp.cols[2]      # Remove col indexed at 2
    del lp.rows[2,5,6]  # Remove rows indexed at 2,5,6
    del lp.cols[3:7]    # Remove cols indexed at 3,4,5,6

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Row and Column Scaling Factors
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The constraint matrix :math:`A` undergoes a linear transformation with diagonal
positive matrices :math:`R` and :math:`S` (row and column scaling matrices,
respectively) to come up with an implicit new constraint matrix
:math:`\tilde{A} = RAS`. The transformed matrix has entries
:math:`\tilde{a}_{ij}=r_{ii}a_{ij}s_{jj}`. Though most users may wish to set
this <a href="#lp_scale">scaling automatically, one may set and get the row and
column scaling factors manually with the ``scale`` attribute. Changing the
scaling factor for row :math:`i` or column :math:`j` corresponds to changing
element :math:`r_{ii}` or :math:`s_{jj}` in the diagonal scaling matrices,
respectively.

.. code-block:: c

    glp_set_rii(lp, 2, 3.14159);
    glp_set_sjj(lp, 4, 2.0);
    double row3scale = glp_get_rii(lp, 3);
    double col3scale = glp_get_sjj(lp, 3);

.. code-block:: python

    lp.rows[2].scale = 3.14159
    lp.cols[4].scale = 2.0
    row3scale = lp.rows[3].scale
    col3scale = lp.cols[3].scale

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Special Attributes of Rows and Columns
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

As is clear from the previous examples, row and column collections (e.g., as
accessed by ``lp.rows``) and rows and columns (e.g., as accessed by
``lp.rows[i]``) as well as the rows and columns themselves are bona fide
objects. (This was a design choice: rather than having only the LPX class where
one defines a hundred or so get and set methods, as the C API must, one
retrieves the rows and columns and operates on them instead.)

As a point of implementation, these row and column objects do not contain the
row and column data. In reality, they just contain a pointer back to the LPX
and an index. We shall see consequences of this in this subsection.

Aside from attributes which have obvious analogies to functions in the C API
(e.g., ``name`` with the ``glp_[gs]et_(row|col)_name`` functions), rows and
columns have other special attributes that do not have analogies in the C API
which are exposed to Python users in the hope they may find them useful.

``index`` is an integer attribute containing the index of this row or column.

``valid`` is a boolean attribute containing whether this row or column is
valid. A row or column may become invalid if its index points to somewhere
beyond the current size of the LPX. This is mostly useless: one can track the
size of the program, and even if you do not, using an out of date row or column
safely throws exceptions.

``isrow`` and ``iscol`` are boolean attributes indicating whether this is a row
or a column. Naturally these two attributes are inverses of each other.

Example usage of these principles to elucidate these implementations is
illustrated in this example. All assertions in this snippet are satisfied.

.. code-block:: python

    lp = glpk.LPX()
    lp.rows.add(3)
    lp.rows[0].name, lp.rows[1].name, lp.rows[2].name = 'p', 'q', 'r'
    row1, row2 = lp.rows[1], lp.rows[2]
    assert row1.name == 'q' and row2.name == 'r'
    del lp.rows[1]
    assert row1.name == 'r' and row1.valid and not row2.valid
    assert row1.isrow

^^^^^^^^^^^^^^^^^^^^^^^^^^^
Row and Column Basis Status
^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets and sets the current basis status for a row or column by querying or
assigning the attribute ``status``. This is a two-character string with the
following possible values.

- ``bs`` meaning this row/column is basic.

- ``nl`` meaning this row/column is non-basic.

- ``nu`` meaning this row/column is non-basic and set to the upper bound. On
  assignment, if this row/column is not double bounded, this is equivalent to
  ``nl``.

- ``nf`` meaning this row/column is non-basic and free. On assignment this is
  equivalent to ``nl``.

- ``ns`` meaning this row/column is non-basic and fixed. On assignment this is
  equivalent to ``nl``.

.. code-block:: c

    if (glp_get_row_stat(lp, ri+1) == GLP_BS)
        printf("row is basic\n");
    else
        printf("row is non-basic\n");

.. code-block:: python

    if lp.rows[ri].status == "bs":
        print "row is basic"
    else:
        print "row is non-basic"


As an example of setting the status, the user may wish to assign to this
attribute in order to manually define the initial basis and not rely upon the
automatic basis definition methods ``lp.*_basis()``. To illustrate this, here
is the code within the GLPK standard basis code in both C and Python versions.

.. code-block:: c

    int i, j, m, n, type;
    double lb, ub;
    // all auxiliary variables are basic
    m = glp_get_num_rows(lp);
    for (i = 1; i <= m; i++)
        glp_set_row_stat(lp, i, GLP_BS);
    // all structural variables are non-basic
    n = glp_get_num_cols(lp);
    for (j = 1; j <= n; j++) {
        type = glp_get_col_type(lp, j);
        lb = glp_get_col_lb(lp, j);
        ub = glp_get_col_ub(lp, j);
        if (type != GLP_DB || fabs(lb) <= fabs(ub))
            glp_set_col_stat(lp, j, GLP_NL);
        else
            glp_set_col_stat(lp, j, GLP_NU);
    }

.. code-block:: python

    # all auxiliary variables are basic
    for row in lp.rows:
        row.status = "bs"
    # all structural variables are non-basic
    for col in lp.cols:
        lb, ub = col.bounds
        if lb==None or ub==None or abs(lb)<=abs(ub):
            col.status = "nl"
        else:
            col.status = "nu"

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Column Variable Kind, Continuous, Integer, or Binary
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets and sets the kind of variable (the default continuous, or integer) by
querying or assigning the attribute ``kind``. This will hold either ``float``
if this is a continuous variable, ``int`` if this is an integer variable, or
``bool`` if this is a binary variable.

.. code-block:: c

    glp_set_col_kind(lp, ci+1, GLP_CV);
    glp_set_col_kind(lp, ci+1, GLP_IV);
    glp_set_col_kind(lp, ci+1, GLP_BV);
    int kind = glp_get_col_kind(lp, ci+1);

.. code-block:: python

    lp.cols[ci].kind = float
    lp.cols[ci].kind = int
    lp.cols[ci].kind = bool
    kind = lp.cols[ci].kind

Note that PyGLPK and GLPK do not make any distinction between setting a column
as binary, versus setting the column as integral with [0, 1] bounds.

Another note, rows must be continuous. As a matter of implementation, because
they are the same type of object as columns, they may also be queried and
assigned to in this fashion. However, their ``kind`` attribute always returns
and only accepts ``float``.

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Row and Column Variable Values
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

One gets a row or column's variable value by querying the ``primal``, ``dual``,
or ``value`` attribute.

.. code-block:: c

    double pval = glp_get_row_prim(ri+1); // if simplex
    double dval = glp_get_col_dual(ci+1);

.. code-block:: python

    pval = lp.rows[ri].primal
    dval = lp.cols[ci].dual

The two attributes ``primal`` and ``value`` are interchangeable. The term value
is used to refer to the solutions of the MIP since it only has one type of
value (no dual). However, since it is the same type of value (loosely
speaking), this "link" between the two was established.

Note that unlike the C API, this remembers which solver was used last and
retrieves the appropriate corresponding variable value. If for whatever reason
you wish to retrieve a variable function from a solver type different from what
you used last (e.g., reviewing the relaxed basic solution after calling
``integer()``), you can ask for ``primal_s`` or ``dual_s`` (for primals and
duals from ``simplex`` and ``exact``), or ``primal_i`` or ``dual_i`` (for
primals and duals from ``interior``), or ``value_m`` (for values from
``integer`` or ``intopt``).

.. code-block:: c

    double prim_sim_row = glp_get_row_prim(lp, ri+1);
    double prim_sim_col = glp_get_col_prim(lp, ci+1);
    double dual_sim_row = glp_get_row_dual(lp, ri+1);
    double dual_sim_col = glp_get_col_dual(lp, ci+1);
    double prim_ipt_row = glp_ipt_row_prim(lp, ri+1);
    double prim_ipt_col = glp_ipt_col_prim(lp, ci+1);
    double dual_ipt_row = glp_ipt_row_dual(lp, ri+1);
    double dual_ipt_col = glp_ipt_col_dual(lp, ci+1);
    double valu_mip_row = glp_mip_row_val (lp, ri+1);
    double valu_mip_col = glp_mip_col_val (lp, ci+1);

.. code-block:: python

    prim_sim_row, prim_sim_col = lp.rows[ri].primal_s, lp.cols[ci].primal_s
    dual_sim_row, dual_sim_col = lp.rows[ri].dual_s,   lp.cols[ci].dual_s
    prim_ipt_row, prim_ipt_col = lp.rows[ri].primal_i, lp.cols[ci].primal_i
    dual_ipt_row, dual_ipt_col = lp.rows[ri].dual_i,   lp.cols[ci].dual_i
    valu_mip_row, valu_mip_col = lp.rows[ri].value_m,  lp.cols[ci].value_m

------------------------------
MIP Callbacks and Search Trees
------------------------------

In this section we describe the MIP solver callback interface, and the ``Tree``
and ``TreeNode`` objects supporting this interface for affecting the MIP
solver.

^^^^^^^^^^^^^^^^
Callback Objects
^^^^^^^^^^^^^^^^

One of the more esoteric parts of the GLPK mixed integer programming solver is
the use of callbacks to let the user code affect the flow of the search
process. Within PyGLPK, one can define a callback object which will be invoked
at various parts of the algorithm, through the use of the optional ``callback``
keyword parameter to the MIP solver, whose argument we will term ``cb``:

.. code-block:: python

    lp.integer(callback=cb)

What this ``cb`` callback object is is not strictly defined, but this object
``cb`` should respond to calls of the form ``cb.method(tree)``, where
``method`` is one of ``select``, ``prepro``, ``rowgen``, ``heur``,
``cutgen``, ``branch``, or ``bingo``. These different methods represent the MIP
solver seeking the callback object's input at various phases of the input. (If
a method does not exist, PyGLPK will try the ``default`` method instead, and if
that does not exist, it will ignore the callback for that method.)

The ``tree`` argument to these methods is a ``Tree`` instance, a representation
of the search tree of the method. The Tree instance contains data about the
problem being solved.

.. code-block:: c

    void callback_func(glp_tree *tree, void *info) {
        switch (glp_ios_reason(tree)) {
        case GLP_ISELECT: // some code to select subproblems here...
        case GLP_IPREPRO: // some code for preprocessing here...
        case GLP_IROWGEN: // some code for providing constraints here...
        case GLP_IHEUR:   // some code for providing heuristic solutions here...
        case GLP_ICUTGEN: // some code for providing constraints here...
        case GLP_IBRANCH: // some code to choose a variable to branch on here...
        case GLP_IBINGO:  // some code to monitor the situation here...
        }
    }
    ...
    glp_iocp parm;
    glp_init_iocp(&parm);
    parm.cb_func = callback_func;
    glp_intopt(lp, &parm);

.. code-block:: python

    class Callback:
        def select(self, tree):
            # some code to select subproblems here...
        def prepro(self, tree):
            # some code for preprocessing here...
        def rowgen(self, tree):
            # some code for providing constraints here...
        def heur(self, tree):
            # some code for providing heuristic solutions here...
        def cutgen(self, tree):
            # some code for providing constraints here...
        def branch(self, tree):
            # some code to choose a variable to branch on here...
        def bingo(self, tree):
            # some code to monitor the situation here...

    lp.integer(callback=Callback())

The ``Tree`` instance passed along to the function contains active subproblems
being searched, where each subproblem corresponds to a ``TreeNode`` instance.

Each of these seven phases of GLPK's implementation of the branch and cut
algorithm correspond to these seven methods. By calling a particular method,
the GLPK indicates that it desires some sort of input from the user. While a
full description of the branch and cut algorithm is beyond the scope of this
document (see the "Branch-and-cut interface routines" section of the "GNU
Linear Programming Kit Reference Manual" that came with you GLPK distribution),
and unfortunately a full understanding of what to do in each instance is beyond
this author, we briefly describe the phases here, and what may be helpful in
each instance to do what GLPK wants us to do.

Note that all operations are optional. One does not need to implement each
method, and it is fine for a method to not do what is being requested: even
just having a ``pass`` statement in a method is fine. The GLPK has default
behavior for all of the methods in case the user does not choose to affect the
solution process.

``select``, request for subproblem selection
  There is no current node, so set one of the active subproblems as the current
  node with the ``tree.select`` method. The default behavior of the GLPK is to
  select the node with the best local bound, equivalent to this:

  .. code-block:: python

      def select(self, tree):
          tree.select(tree.best_node)

``prepro``, request for preprocessing
  The GLPK manual suggests that one may take advantage of this to perform
  preprocessing, perhaps of the form of tightening or loosening bounds of some
  variables, through modification of the ``tree.lp`` program object.

``rowgen``, request for row generation
  When the current subproblem has been solved to optimality and the LP
  relaxation has been solved with a solution better than the best known integer
  feasible solution, this procedure may be called upon to add "lazy"
  constraints to the ``tree.lp``, which is done as one normally adds rows
  (``tree.lp.rows.add(...)``, and so on).

``heur``, request for a heuristic solution
  When the current subproblem being solved to optimality is integer infeasible
  (i.e., some integer problems are fractional), though with a better objective
  value than the best known integer solution, one may call
  ``tree.heuristic(newsol)`` where ``newsol`` is some iterable object (like a
  list, or an iterator) which can yield at least ``len(tree.lp.cols)`` float
  values (with integral values for integral columns), to serve as the new
  primal values. (The method will check to see if it is better.)  Note that
  feasibility of this solution is not checked by the method, so use caution.

``cutgen``, request for cut generation
  Similar to ``rowgen``, called when the subproblem being solved is integer
  infeasible but better than the best known integer solution, with the intent
  being that one adds constraints to cut off the current solution.

``branch``, request for branching
  In the case of integer infeasibility, we have some integer variable (e.g.,
  column) with non-integer value :math:`V`. Branching is the process of
  splitting this process by adding two subproblems to the active list with the
  column's value set to :math:`\lfloor V \rfloor` and :math:`\lceil V \rceil`.
  For some column index ``j``, which we have confirmed we can branch upon with
  the ``tree.can_branch`` method, we call the ``tree.branch`` method with that
  index to add the two corresponding subproblems.

  .. code-block:: python

      def branch(self, tree):
          # Find the first fractional integer variable, and branch on it
          for j in xrange(len(tree.lp.cols)):
              if tree.can_branch(j):
                  tree.branch(j)
                  break

``bingo``, better integer solution found
  When the LP relaxation finds an integer feasible solution, this method is
  called. This is intended only for informational purposes, and should not
  modify any problem data.


^^^^^^^^^^^^^^^^^^^
Tree Linear Program
^^^^^^^^^^^^^^^^^^^

One may retrieve the ``LPX`` problem object used by the MIP solver with the
``lp`` member, i.e., ``tree.lp``. This object is not necessarily the same
``LPX`` instance as that for which we called ``lp.integer()`` if some
preprocessing was performed by the MIP solver.

Modification of the underlying LPX object is an important part of the callback
procedures, especially in the ``rowgen``, ``cutgen``, and ``prepro`` methods.
It is important to note that not all operations you may perform on LPX objects
within this callback are necessarily safe: modifying the problem object out
from under the procedure in the middle of optimization might cause problems.
However, it is difficult to distinguish a "problematic" change versus one which
is helpful for, say, preprocessing, or computing cuts, or what have you.

In order to aid ease of use, the PyGLPK implements "generic" solution retrieval
methods. For instance, an LP potentially has multiple objective function
values: one for the last simplex solution ``lp.obj.value_s``, one for the last
interior point solution ``lp.obj.value_i``, and one for the last MIP solution
``lp.obj.value_m``. However, there is also a ``lp.obj.value``, which sensibly
supposes that a user is interested in the solution the solver (which covers the
vast majority of use cases). However, in the midst of integer solution, the MIP
solver hasn't yet become the "last" solver, so be sure to be explicit when
retrieving values of columns and objective function values.

.. code-block:: c

    glp_prob *lp = glp_ios_get_prob(tree);

.. code-block:: python

    lp = tree.lp

^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Tree Methods for Affecting Search
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The ``Tree`` instance has many methods that allow one to affect the search
process, as described earlier: ``can_branch`` and ``branch_upon`` to choose a
column to set as integer, ``heuristic`` to set a new integer feasible solution,
``select`` to select an active subproblem for expansion, and ``terminate`` to
just terminate the solution outright.

.. code-block:: c

    glp_ios_can_branch(tree, 5+1);
    glp_ios_branch_upon(tree, 5+1, 'D');
    double *values;
    glp_ios_heur_sol(tree, values)
    glp_ios_select_node(tree, node_num);
    glp_ios_terminate(tree);

.. code-block:: python

    tree.can_branch(5)
    tree.branch_upon(5, 'D')
    tree.heuristic(values)
    tree.select(node)
    tree.terminate()


^^^^^^^^^^^^^^^^^^^
Tree Node Traversal
^^^^^^^^^^^^^^^^^^^

The tree instance contains an active subproblem list, corresponding to current
entries in the search tree which have not yet been explored. The tree contains
several members that let one access the tree nodes, corresponding to
subproblems in the list:

- ``curr_node``, the current active subproblem's node, which will be ``None``
  in the selection phase when there is no current subproblem.

- ``first_node``, the first node in the active subproblem list.

- ``last_node``, the last node in the active subproblem list.

- ``best_node``, the node whose active subproblem has the best local bound.

For instance:

.. code-block:: c

    int node_num;
    node_num = glp_ios_curr_node(tree);
    node_num = glp_ios_next_node(tree, 0);
    node_num = glp_ios_prev_node(tree, 0);
    node_num = glp_ios_best_node(tree);

.. code-block:: python

    node = tree.curr_node
    node = tree.first_node
    node = tree.last_node
    node = tree.best_node


One may also iterate over ``tree`` to get all of the nodes in the active list.

In addition to ``Tree`` members, each ``TreeNode`` has members to connect them
to other ``TreeNode`` objects in the tree.

- ``next``, which gets the next active subproblem's node, or ``None`` if this
  is the last active node or an inactive node.

- ``prev``, which gets the previous active subproblem's node, or ``None`` if
  this is the first active node or an inactive node.

- ``up``, which gets the parent node that generated this node, or ``None`` if
  this is the root node.

- ``level``, which is this node's distance from the root, ``0`` if this is the
  root node

- ``subproblem``, which is the integral subproblem number, assigned in order
  from 1 onwards.

.. code-block:: c

    int other_node_num, lev;
    other_node_num = glp_ios_next_node(tree, node_num);
    other_node_num = glp_ios_prev_node(tree, node_num);
    other_node_num = glp_ios_up_node(tree, node_num);
    lev = glp_ios_node_level(tree, node_num);
    // node_num is the subproblem id number

.. code-block:: python

    other_node = node.next
    other_node = node.prev
    other_node = node.up
    other_node = node.level
    subproblem_num = node.subproblem

^^^^^^^^^
Tree Size
^^^^^^^^^

One may get the tree size through the use of various members.

- ``num_active``, the number of active nodes.

- ``num_all``, the number of active and inactive nodes in the tree.

- ``num_total``, the number of nodes which were generated, active, inactive,
  and those nodes which have already been removed.

.. code-block:: c

    int count;
    glp_ios_tree_size(tree, &count, NULL, NULL);
    glp_ios_tree_size(tree, NULL, &count, NULL);
    glp_ios_tree_size(tree, NULL, NULL, &count);

.. code-block:: python

    count = tree.num_active
    count = tree.num_all
    count = tree.num_total


^^^^^^^^^^^^
Tree MIP Gap
^^^^^^^^^^^^

One may get the current relative gap between the integer and relaxed solution
with the ``gap`` member.

.. code-block:: c

    double gap = glp_ios_mip_gap(tree);

.. code-block:: python

    gap = tree.gap

^^^^^^^^^^^^^^^
Tree Node Bound
^^^^^^^^^^^^^^^

One may get the lower (in minimization) or upper (in maximization) bound on the
integer optimal solution to a node's subproblem with the ``bound`` member of a
``TreeNode`` instance.

.. code-block:: c

    double bound = glp_ios_node_bound(tree, node_num);

.. code-block:: python

    bound = node.bound


^^^^^^^^^^^^^^^^^^^^
Tree Callback Reason
^^^^^^^^^^^^^^^^^^^^

The ``Tree`` member ``reason`` holds a string indicating the reason why the
callback was invoked. While the reason for the callback is the same as the
method name called in the callback (e.g., ``select`` is called only if
``tree.reason='select'``), if one implements the ``default`` method in lieu of
specific methods, one may wish to extract the reason with this member.

.. code-block:: c

    int reasoncode = glp_ios_reason(tree);

.. code-block:: python

    reason = tree.reason

-----------
Environment
-----------

Contained within ``glpk.env`` object is an ``Environment`` instance, through
which one controls the global behavior of the GLPK routines.

^^^^^^^
Version
^^^^^^^

In the environment is a tuple ``version`` that reflects the version of GLPK
that the build process believed it was linking against at compilation time. For
example, if the module believed it was linking against GLPK 4.31, the tuple
would be ``(4, 31)``.

.. code-block:: c

    printf("GLPK version is %s\n", glp_version());

.. code-block:: python

    print 'GLPK version is %d.%d\n' % glpk.env.version

^^^^^^
Memory
^^^^^^

The GLPK monitors its own memory use, and this information can be retrieved
from these members of the ``glpk.env`` object. The ``blocks`` member holds the
current number of allocated memory blocks, while ``blocks_peak`` holds the
maximum this ever reached. The ``bytes`` and ``bytes_peak`` members are
similar, except for bytes. Note that blocks are not a particular size, but are
multiple

.. code-block:: c

    int blocks, blocks_peak;
    glp_long bytes, bytes_peak;
    glp_mem_usage(&blocks, &blocks_peak, &bytes, &bytes_peak);

.. code-block:: python

    blocks = glpk.env.blocks
    blocks_peak = glpk.env.blocks_peak
    bytes = glpk.env.bytes
    bytes_peak = glpk.env.bytes_peak

One may also set the maximum number of megabytes with the ``mem_limit`` member.

.. code-block:: c

    glp_mem_limit(50);

.. code-block:: python

    glpk.env.mem_limit = 50

^^^^^^^^^^^^^^^
Terminal Output
^^^^^^^^^^^^^^^

The GLPK has many functions that produce output. The user may at their option
turn off or on the output by setting the environment's ``term_on`` attribute to
``False`` or ``True``.

.. code-block:: c

    glp_term_out(GLP_OFF);
    glp_term_out(GLP_ON);

.. code-block:: python

    glpk.env.term_on = False
    glpk.env.term_on = True


In addition to turning it on and off, one may enable more fine grained control
by intercepting all terminal output with a function hook. The function will be
called with a single string argument whenever the GLPK chooses to print
something. Note that this will intercept only that output which is produced by
the GLPK itself -- other output from Python will be completely unaffected.

.. code-block:: c

    int term_hook(void *info, const char *output) {
        FILE *logfile = (FILE *)info;
        fputs(output, logfile);
        return 1;
    }
    ...
    FILE *lf = fopen("glpk_logfile.txt", "w");
    glp_term_hook(term_hook, (void*)lf);

.. code-block:: python

    logfile = file('glpk_logfile.txt', 'w')
    def term_hook(output):
        file.write(output)
    glpk.env.term_hook = term_hook

One may remove the terminal hook function (e.g., resume default terminal
output) by assigning the value ``None``.

.. code-block:: c

    glp_term_hook(NULL, NULL);

.. code-block:: python

    glpk.env.term_hook = None

-----------------------------
Karush-Kuhn-Tucker Conditions
-----------------------------

The linear program object has the ability to return a ``KKT`` type objects with
which the user may evaluate the fitness of either simplex or MIP solutions.

^^^^^^^^^^^^^^^^^^^^^^
Retrieving KKT Objects
^^^^^^^^^^^^^^^^^^^^^^

One can compute and retrieve these conditions for simplex solvers with the
``kkt`` and for integer solvers with the ``kktint`` methods. The ``kkt`` method
has an optional argument that allows one to specify whether one wants to
compute the conditions for the internally scaled version of the problem (by
default false).

.. code-block:: c

    LPXKKT kkt, skkt, ikkt;
    lpx_check_kkt(lp, 0,  &kkt);  // unscaled simplex KKT
    lpx_check_kkt(lp, 1, &skkt);  // scaled simplex KKT
    lpx_check_int(lp,    &ikkt);  // integer conditions

.. code-block:: python

    kkt  = lp.kkt()     # unscaled simplex KKT
    skkt = lp.kkt(True) # scaled simplex KKT
    ikkt = lp.kktint()  # integer conditions

These objects have KKT statistics about the absolute and relative errors and
worst rows and columns in the primal and (in the case of non-integer problems)
dual solutions. See the inline help for more information about these fields.

-------------------
Miscellaneous Notes
-------------------

^^^^
Help
^^^^

In addition to this documentation, like most Python objects, the objects have
built in inline help, accessible from an interactive Python session. For
example, to access the built in documentation for the ``glpk`` module:

.. code-block:: python

    help(glpk)
