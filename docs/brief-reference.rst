===============
Brief Reference
===============

The following table is a brief overview of the functionality of PyGLPK. The
intention was to present functionality in roughly the same order and groupings
that they do in the GLPK reference manual, at least where appropriate. This
section contains a brief description of some functionality, some simple Python
code illustrating the general principles of usage, the related C API function
(for the benefit of those familiar with GLPK; this information can be safely
ignored if you are not), and a link to more detailed documentation on this
subject with the **?** links.

------------------
Problem Attributes
------------------

Create or delete problem object

.. code-block:: python

    lp = glpk.LPX()  # glp_create_prob
    del(lp)          # glp_delete_prob

Reinitialize problem object

.. code-block:: python

    lp.erase()  # glp_erase_prob

Set or get problem name

.. code-block:: python

    lp.name = "pname"  # glp_set_prob_name
    del(lp.name)       # glp_set_prob_name

    lp.name            # glp_get_prob_name

Set or get objective function name

.. code-block:: python

    lp.obj.name = "oname"  # glp_set_obj_name
    del(lp.obj.name)       # glp_set_obj_name

    lp.obj.name            # glp_get_obj_name

Set or get optimization direction

.. code-block:: python

    lp.obj.maximize = True  # glp_set_obj_dir
    lp.obj.maximize         # glp_get_obj_dir

Add new rows or columns to a problem

.. code-block:: python

    lp.rows.add(num_to_add)  # glp_add_rows
    lp.cols.add(num_to_add)  # glp_add_cols

Set or get row or column name

.. code-block:: python

    lp.rows[ri].name = "rname"  # glp_set_row_name
    del(lp.rows[rnum].name)     # glp_set_row_name

    lp.cols[ci].name = "cname"  # glp_set_col_name
    del(lp.cols[cnum].name)     # glp_set_col_name

    lp.rows[ri].name            # glp_get_row_name
    lp.cols[ci].name            # glp_get_col_name

Set or get row or column bounds

.. code-block:: python

    lp.rows[ri].bounds = lower, upper  # glp_set_row_bnds
    lp.rows[ri].bounds = equals        # glp_set_row_bnds

    lp.cols[ci].bounds = lower, upper  # glp_set_col_bnds
    lp.cols[ci].bounds = equals        # glp_set_col_bnds

    lp.rows[ri].bounds                 # glp_get_row_type
                                       # glp_get_row_lb
                                       # glp_get_row_ub

    lp.cols[ci].bounds                 # glp_get_col_type
                                       # glp_get_col_lb
                                       # glp_get_col_ub

Set or get objective coefficient or shift term

.. code-block:: python

    # use index None to get shift term
    lp.obj[ci] = coef  # glp_set_obj_coef
    lp.obj[ci]         # glp_get_obj_coef

Set or get row or column of the constraint matrix

.. code-block:: python

    lp.rows[ri].matrix = [(ci1,val1), (ci2,val2), ...]  # glp_set_mat_row
    lp.cols[ci].matrix = [(ri1,val1), (ri2,val2), ...]  # glp_set_mat_col
    lp.rows[ri].matrix                                  # glp_get_mat_row
    lp.cols[ci].matrix                                  # glp_get_mat_col

Set or get the whole constraint matrix

.. code-block:: python

    lp.matrix = [(ri1,ci1,val1), (ri2,ci2,val2), ...]  # glp_load_matrix
    lp.matrix

Delete rows or columns from problem object

.. code-block:: python

    del(lp.rows[ri1, ri2, ...])  # glp_del_rows
    del(lp.rows[r_lo:r_hi+1])    # glp_del_rows

    del(lp.cols[ci1, ci2, ...])  # glp_del_cols
    del(lp.cols[c_lo:c_hi+1])    # glp_del_cols


Delete problem object

.. code-block:: python

    # or just let garbage collector handle it
    del(lp)  # glp_delete_prob

---------------------------------
Indexing rows and columns by name
---------------------------------

Index row or column by its name

.. code-block:: python

    lp.rows['rowname']  # glp_find_row
    # to check for membership
    'rowname' in lp.rows

    lp.cols['colname']  #
    # to check for membership
    'colname' in lp.cols

The index is created when required.

---------------
Problem Scaling
---------------

Automatically scale or unscale problem data

.. code-block:: python

    lp.scale()    # glp_scale_prob
    lp.unscale()  # glp_unscale_prob

Set or get scaling of row and column data

.. code-block:: python

    lp.rows[i].scale = factor  # glp_set_rii
    lp.cols[i].scale = factor  # glp_set_sjj
    lp.rows[i].scale           # glp_get_rii
    lp.cols[i].scale           # glp_get_sjj

----------------
Basis operations
----------------

Construct trivial initial LP basis

.. code-block:: python

    lp.std_basis()  # glp_std_basis

Construct advanced initial LP basis

.. code-block:: python

    lp.adv_basis()  # glp_adv_basis

Construct advanced initial LP basis with Bixby's algorithm

.. code-block:: python

    lp.cpx_basis()  # glp_cpx_basis

Read initial LP basis from a file

.. code-block:: python

    lp.read_basis(filename)  # lpx_read_bas

Set row or column status

.. code-block:: python

    lp.rows[ri].status = newstatus  # glp_set_row_stat
    lp.cols[ci].status = newstatus  # glp_set_col_stat

---------------------
Basic simplex solvers
---------------------

Solve problem with the simplex method

.. code-block:: python

    lp.simplex()  # glp_simplex

Solve problem with an exact arithmetic using simplex method

.. code-block:: python

    lp.exact()  # lpx_exact

Get generic, primal, or dual status of basic solution

.. code-block:: python

    lp.status         # glp_get_status
    lp.status_s       # to force simplex status
    lp.status_primal  # glp_get_prim_stat
    lp.status_dual    # glp_get_dual_stat

Get objective value

.. code-block:: python

    lp.obj.value    # glp_get_obj_val
    lp.obj.value_s  # to force simplex value

Get row or column status

.. code-block:: python

    # a string, one of 'bs', 'nl','nu','nf','ns'
    lp.rows[ri].status  # glp_get_row_stat

    # a string, one of 'bs', 'nl','nu','nf','ns'
    lp.cols[ci].status  # glp_get_col_stat

Get row or column primal or dual value

.. code-block:: python

    lp.rows[ri].primal    # glp_get_row_prim
    lp.rows[ri].primal_s  # to force simplex value

    lp.rows[ri].dual      # glp_get_row_dual
    lp.rows[ri].dual_s    # to force simplex value

    lp.cols[ci].primal    # glp_get_col_prim
    lp.cols[ci].primal_s  # to force simplex value

    lp.cols[ci].dual      # glp_get_col_dual
    lp.cols[ci].dual_s    # to force simplex value

Get non-basic variable causing unboundness

.. code-block:: python

    # a row or column, or None if none has been identified
    lp.ray ()  # lpx_get_ray_info

Check solution's Karush-Kuhn-Tucker conditions

.. code-block:: python

    lp.kkt()  # lpx_check_kkt

---------------------------------
Manual simplex tableau operations
---------------------------------

Warm-up LP basis

.. code-block:: python

    lp.warm_up()  # glp_warm_up

Compute a row of the simplex tableau

.. code-block:: python

    lp.rows[ri].eval_tab_row()  # glp_eval_tab_row

Compute a column of the simplex tableau

.. code-block:: python

    lp.cols[ci].eval_tab_col()  # glp_eval_tab_col

Transform explicitly specified row

.. code-block:: python

    lp.transform_row()  # glp_transform_row

Transform explicitly specified column

.. code-block:: python

    lp.transform_column()  # glp_transform_col

Perform primal ratio test

.. code-block:: python

    lp.prime_ratio_test()  # glp_prim_ratio_test

Perform dual ratio test

.. code-block:: python

    lp.dual_ratio_test()  # glp_dual_ratio_test

---------------------
Interior-point solver
---------------------

Solve problem with the interior-point method

.. code-block:: python

    lp.interior()  # lpx_interior

Get status of interior-point solution

.. code-block:: python

    lp.status    # glp_ipt_status
    lp.status_i  # to force interior point status

Get objective value

.. code-block:: python

    lp.obj.value    # glp_ipt_obj_val
    lp.obj.value_i  # to force interior point value


Get row or column primal or dual value

.. code-block:: python

    lp.rows[ri].primal    # glp_ipt_row_prim
    lp.rows[ri].primal_i  # to force interior point value

    lp.rows[ri].dual      # glp_ipt_row_dual
    lp.rows[ri].dual_i    # to force interior point value

    lp.cols[ci].primal    # glp_ipt_col_prim
    lp.cols[ci].primal_i  # to force interior point value

    lp.cols[ci].dual      # glp_ipt_col_dual
    lp.cols[ci].dual_i    # to force interior point value


---------------------------------
Mixed-Integer Programming Solvers
---------------------------------

Set or get problem class

.. code-block:: python

    lp.kind  # lpx_get_class

Set or get column kind

.. code-block:: python

    lp.cols[ci].kind = int    # glp_set_col_kind
    lp.cols[ci].kind = bool
    lp.cols[ci].kind = float

    lp.cols[ci].kind          # glp_get_col_kind

Get number of integer columns

.. code-block:: python

    lp.nint  # glp_get_num_int

Get number of binary columns

.. code-block:: python

    lp.nbin  # glp_get_num_bin

Solve MIP problem with the B&B method

.. code-block:: python

    lp.integer()  # glp_intopt

Solve MIP problem with the advanced B&B solver

.. code-block:: python

    lp.intopt()  # lpx_intopt

Get status of MIP solution

.. code-block:: python

    lp.status    # glp_mip_status
    lp.status_m  # to force MIP status

Get objective value

.. code-block:: python

    lp.obj.value    # glp_mip_obj_val
    lp.obj.value_m  # to force MIP value


Get row or column value

.. code-block:: python

    lp.rows[ri].value    # glp_mip_row_val
    lp.rows[ri].value_m  # to force MIP value

    lp.cols[ci].value    # glp_mip_col_val
    lp.cols[ci].value_m  # to force MIP value

Check solution's integer feasibility conditions

.. code-block:: python

    lp.kktint()  # lpx_check_int

-----------------------------------
MIP Branch & Cut Advanced Interface
-----------------------------------

Access the problem object

.. code-block:: python

    tree.lp  # glp_ios_get_prob

Determine the size of the branch and bound tree

.. code-block:: python

    tree.num_active  # glp_ios_tree_size
    tree.num_all     # tree.num_total

Determine current active subproblem

.. code-block:: python

    tree.curr_node  # glp_ios_curr_node

Determine first active subproblem

.. code-block:: python

    tree.first_node  # glp_ios_next_node

Determine last active subproblem

.. code-block:: python

    tree.last_node  # glp_ios_prev_node

Determine next active subproblem

.. code-block:: python

    node.next  # glp_ios_next_node

Determine previous active subproblem

.. code-block:: python

    node.prev  # glp_ios_prev_node

Determine parent subproblem

.. code-block:: python

    node.up  # glp_ios_up_node

Determine subproblem level

.. code-block:: python

    node.level  # glp_ios_node_level

Determine subproblem local bound

.. code-block:: python

    node.bound  # glp_ios_node_bound

Find active subproblem with best local bound

.. code-block:: python

    tree.best_node  # glp_ios_best_node

Compute relative MIP gap

.. code-block:: python

    tree.gap  # glp_ios_mip_gap

Select subproblem to continue the search

.. code-block:: python

    tree.select(node)  # glp_ios_select_node

Provide solution found by heuristic

.. code-block:: python

    tree.heuristic(values)  # glp_ios_heur_sol

Check if can branch upon specified variable

.. code-block:: python

    tree.can_branch(colnum)  # glp_ios_can_branch

Choose variable to branch upon

.. code-block:: python

    tree.branch_upon(colnum, 'D')  # glp_ios_branch_upon

Terminate the solution process

.. code-block:: python

    tree.terminate()  # glp_ios_terminate

-----------
Environment
-----------

Get GLPK version

.. code-block:: python

    glpk.env.version  # glp_version

Monitor memory usage

.. code-block:: python

    glpk.env.blocks       # glp_mem_usage
    glpk.env.blocks_peak
    glpk.env.bytes
    glpk.env.bytes_peak

Limit memory usage

.. code-block:: python

    # max_megabytes should be an integer
    glpk.env.mem_limit = max_megabytes  # glp_mem_limit

Control terminal output

.. code-block:: python

    glpk.env.term_on = True           # glp_term_out
    glpk.env.term_hook = output_func  # glp_term_hook

---------------
Problem readers
---------------

Read fixed MPS format file

.. code-block:: python

    lp = glpk.LPX(mps=filename)  # lpx_read_mps

Read free MPS format file

.. code-block:: python

    lp = glpk.LPX(freemps=filename)  # lpx_read_freemps

Read GNU LP format file

.. code-block:: python

    lp = glpk.LPX(glp=filename)  # lpx_read_prob

Read CPLEX LP format file

.. code-block:: python

    lp = glpk.LPX(cpxlp=filename)  # lpx_read_cpxlp

Read GNU MathProg model file

.. code-block:: python

    lp = glpk.LPX(gmp=filename)  # lpx_read_model
    lp = glpk.LPX(gmp=(model_file, data_file, output_file))


------------------------
Problem and data writers
------------------------

Write problem to fixed MPS format file

.. code-block:: python

    lp.write(mps=filename)  # lpx_write_mps

Write problem to free MPS format file

.. code-block:: python

    lp.write(freemps=filename)  # lpx_write_freemps

Write problem to GNU LP format file

.. code-block:: python

    lp.write(glp=filename)  # lpx_write_prob

Write problem to CPLEX LP format file

.. code-block:: python

    lp.write(cpxlp=filename)  # lpx_write_cpxlp

Write LP basis to fixed MPS format file

.. code-block:: python

    lp.write(bas=filename)  # lpx_write_bas

Write problem to plain text file

.. code-block:: python

    lp.write(prob=filename)  # lpx_print_prob

Write basic solution to plain text file

.. code-block:: python

    lp.write(sol=filename)  # lpx_print_sol

Write bounds sensitivity information to plain text file

.. code-block:: python

    lp.write(sens_bnds=filename)  # lpx_print_sens_bnds

Write interior point solution to plain text file

.. code-block:: python

    lp.write(ips=filename)  # lpx_print_ips

Write MIP solution to plain text file

.. code-block:: python

    lp.write(mip=filename)  # lpx_print_mip
