import glpk


def solve_sat(expression):
    # trivial case
    if len(expression) == 0:
        return []

    # otherwise count vars
    numvars = max([max([abs(v) for v in clause]) for clause in expression])

    # empty LP instance
    lp = glpk.LPX()

    # stop annoying messages
    glpk.env.term_on = False

    # as many columns as there are literals
    lp.cols.add(2*numvars)
    # literal must be between false and true
    for col in lp.cols:
        col.bounds = 0.0, 1.0

    # function to compute column index
    def lit2col(lit):
        return [2*(-lit)-1, 2*lit-2][lit > 0]

    # ensure "oppositeness" of literals
    for i in xrange(1, numvars+1):
        lp.rows.add(1)
        lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
        # must sum to exactly 1
        lp.rows[-1].bounds = 1.0

    # ensure "trueness" of each clause
    for clause in expression:
        lp.rows.add(1)
        lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
        # at least one literal must be true
        lp.rows[-1].bounds = 1, None

    # try to solve the relaxed problem
    retval = lp.simplex()

    # should not fail in this fashion
    assert retval is None
    # ff no relaxed solution, no exact sol
    if lp.status != 'opt':
        return None

    for col in lp.cols:
        col.kind = int

    # try to solve this integer problem
    retval = lp.integer()
    # should not fail in this fashion
    assert retval is None
    if lp.status != 'opt':
        return None

    return [col.value > 0.99 for col in lp.cols[::2]]

exp = [
    (-1, -3, -4),
    (2, 3, -4),
    (1, -2, 4),
    (1, 3, 4),
    (-1, 2, -3)
]
print(solve_sat(exp))
