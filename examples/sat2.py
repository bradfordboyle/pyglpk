import glpk

def solve_sat(expression):
    if len(expression)==0: return [] # Trivial case.  Otherwise count vars.
    numvars = max([max([abs(v) for v in clause]) for clause in expression])
    lp = glpk.LPX()                  # Construct an empty linear program.
    glpk.env.term_on = False         # Stop the annoying output.
    lp.cols.add(2*numvars)           # As many columns as there are literals.
    for col in lp.cols:              # Literal must be between false and true.
        col.bounds = 0.0, 1.0
    def lit2col(lit):                # Function to compute column index.
        return [2*(-lit)-1,2*lit-2][lit>0]
    for i in range(1, numvars+1):   # Ensure "oppositeness" of literals.
        lp.rows.add(1)
        lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
        lp.rows[-1].bounds = 1.0     # Must sum to exactly 1.
    for clause in expression:        # Ensure "trueness" of each clause.
        lp.rows.add(1)
        lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
        lp.rows[-1].bounds = 1, None # At least one literal must be true.
    retval = lp.simplex()            # Try to solve the relaxed problem.
    assert retval is None            # Should not fail in this fashion.
    if lp.status!='opt': return None # If no relaxed solution, no exact sol.

    for col in lp.cols:
        col.kind = int
    retval = lp.integer()            # Try to solve this integer problem.
    assert retval is None            # Should not fail in this fashion.
    if lp.status != 'opt': return None
    return [col.value > 0.99 for col in lp.cols[::2]]

exp = [(-1, -3, -4), (2, 3, -4), (1, -2, 4), (1, 3, 4), (-1, 2, -3)]
print(solve_sat(exp))
