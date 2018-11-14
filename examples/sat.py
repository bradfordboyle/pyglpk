import glpk, random, sys

# Conjunction of disjunction instances can defined as a list of tuples
# of integers.  Each integer indexes into some variable (counting from
# 1).  A negative index indicates the "not" of the appropriate
# variables.  The formula is the result of "or"ing all the quantities
# within the tuples, and the "and"ing across all tuples.  For one
# example, a 3-SAT formula would be a case where all the tuples were
# of length 3.

# For example, the expression
#   ( !x2 \/ !x3 \/ x4 ) /\ ( !x1 \/ x2 \/ x4 ) /\ ( x2 \/ !x3 \/ !x4 )
# would have representation
#   [(-2, -3, 4), (1, 2, 4), (2, -3, -4)]

# An interesting problem is the SAT problem, where we try to find an
# assignment of truth values to each variable so that a given
# expression becomes true.  This module has a SAT solver.

def generate_cnf(numvars, numclauses, perclause=3, rgen=random):
    """Generate a random CNF expression.

    Generates a random CNF formula over the given number of variables,
    clauses, and n, using the given random generator.  The argument
    numvars serves as the number of variables.  Numclauses serves as
    the number of clauses that are generated.  Perclause serves as the
    number of literals per clause."""
    if (numvars < 3): raise ValueError("need more than 3 variables")
    return [tuple(rgen.choice((-v,v)) for v in rgen.sample(
        range(1,numvars+1),3)) for c in range(numclauses)]

def solve_sat(expression):
    """Attempts to satisfy a formula of conjunction of disjunctions.

    If there are n variables in the expression, this will return a
    list of length n, all elements booleans.  The truth of element i-1
    corresponds to the truth of variable i.

    If no satisfying assignment could be found, None is returned."""
    # Trivial boundary case.
    if len(expression)==0: return []
    # Now many variables do we have?
    numvars = max(max(abs(v) for v in clause) for clause in expression)

    # Construct an empty linear program.
    lp = glpk.LPX()

    # The output GLPK produces is rather annoying.
    glpk.env.term_on = False

    # We want twice as many columns (LP variables) as there are
    # logical variables in the expression: one column for each
    # positive literal, and one column for each negative literal.  A
    # literal is "true" if its column holds 1, false if 0.
    lp.cols.add(2*numvars)

    # Bound all columns (LP variables) to have value between 0 and 1.
    for col in lp.cols:
        col.bounds = 0.0, 1.0

    # Let us suppose that literals x_i and !x_i correspond to columns
    # 2*i-2 and 2*i-1 respectively.  (So, columns 0 and 1 correspond
    # to the truth of x_1 and !x_1, respectively, columns 2 and 3 to
    # the truth of x_2 and !x_2, etc.)

    # Let's just define a helper function that will perform the
    # mapping of literal identifier to column number.  (So 1 maps to
    # 0, -1 to 1, 2 to 2, -2 to 3, 3 to 4, etc.)
    def lit2col(lit):
        return 2*lit-2 if lit>0 else 2*(-lit)-1

    # Here we do two things: we set the names of the column variables
    # appropriately (not required, but fun), and define a row in a
    # constraint matrix indicating that a literal and its complement
    # must sum to exactly 1 since exactly one of the two literals must
    # be true.
    for i in range(1, numvars+1):
        lp.cols[lit2col( i)].name =  'x_%d'%i
        lp.cols[lit2col(-i)].name = '!x_%d'%i
        lp.rows.add(1)
        lp.rows[-1].matrix = [(lit2col(i), 1.0), (lit2col(-i), 1.0)]
        lp.rows[-1].bounds = 1.0

    # We want to find an assignment of such variables so that each
    # disjunction (or-ing) is true.  A disjunction is true if one of
    # its literals is true.  Literal truth corresponds to 1 in the
    # corresponding column, so we can represent truth of a disjunction
    # by defining a constraint saying the sum of its literals must be
    # at least 1.  We do this for all clauses.
    for clause in expression:
        lp.rows.add(1)
        lp.rows[-1].matrix = [(lit2col(lit), 1.0) for lit in clause]
        lp.rows[-1].bounds = 1, None

    # Now we have the LP built.  Run the simplex algorithm.
    retval = lp.simplex()

    # If our iteration terminated prematurely, or if we do not have an
    # optimal solution, assume we have failed.
    if retval != None: return None
    if lp.status != 'opt': return None

    # Now switch this from a linear continuous problem to a
    # mixed-integer problem, and say we want all column variables
    # integer as well (that is, not just between 0 and 1, but exactly
    # 0 and 1).
    for col in lp.cols:
        col.kind = int

    # Attempt to solve this MIP problem with the MIP solver.
    retval = lp.integer()

    # Again, only returns non-None on failure.
    if retval != None: return None
    if lp.status != 'opt': return None

    # We want to return a list of boolean quantities, where the first
    # element is True iff x_1 is true, the second element is True iff
    # x_2 is true, and so on.  Variable truth corresponds to the
    # literals represented in the even columns.  So, we can just go
    # over all of the even columns (every 2 counting from 0), see if
    # the value is close to 1, and use that as our variable
    # assignment.
    return [col.value > 0.99 for col in lp.cols[::2]]

def verify(expression, assignment):
    """Return the truth of an expression given a variable truth assignment."""
    # Each clause must be true.
    for clause in expression:
        # For a disjunctive clause to be true, at least one of its
        # literals must be true.
        lits_true = 0
        for lit in clause:
            if (lit>0) == (assignment[abs(lit)-1]):
                lits_true += 1
        if lits_true == 0:
            return False
    return True

if __name__=='__main__':
    # If we have arguments, use them to seed the random generator.
    if len(sys.argv) > 1:
        random.seed(tuple(sys.argv))
    expression = generate_cnf(4, 16)
    assignment = solve_sat(expression)

    if assignment:
        print('Assignment found.  It is')
        print('valid.' if verify(expression, assignment) else 'invalid.')
    else:
        print('No assignment found.')
