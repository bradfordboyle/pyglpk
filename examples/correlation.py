import glpk
import random

def pair2index(i, j):
    """Convenience function to get an item pair's variable index."""
    i, j = min(i, j), max(i, j)
    return (j * (j-1)) // 2 + i

def pretty_print(matrix):
    """Outputs a 'matrix' in an easily readable format."""
    for row in matrix[1:]:
        print(' '.join('%.3f'%value for value in row))

def gen_random(items, rgen = random):
    """Generate a random matrix."""
    return [[rgen.uniform(-1,1) for i in range(j)] for j in range(items)]

def correlation_clustering(similarity):
    """Linear program relaxation of correlation clustering."""
    lp = glpk.LPX()                          # Define the linear program.
    items = len(similarity)                  # Get the number of items.
    lp.obj.maximize = True                   # Set as maximization.
    lp.cols.add((items * (items-1)) // 2)    # Each item pair has a var.
    for j in range(items):
        for i in range(j):
            index = pair2index(i, j)         # Get the index for this pair.
            lp.cols[index].bounds = 0, 1     # Each variable in range 0 to 1.
            lp.obj[index] = similarity[j][i] # If 1, this much added to obj.
    for k in range(items):                  # For all triples of items, we
        for j in range(k):                  # want to add in constraints to
            jk = pair2index(j,k)             # enforce respect for the
            for i in range(j):              # triangle inequality.
                ij, ik = pair2index(i,j), pair2index(i,k)
                # We want Xij >= Xik + Xjk - 1.
                # That is, we want 1 >= Xik + Xjk - Xij.
                # Add constraints to enforce this!
                lp.rows.add(3)
                lp.rows[-3].matrix = (ij, 1), (ik, 1), (jk,-1)
                lp.rows[-2].matrix = (ij, 1), (ik,-1), (jk, 1)
                lp.rows[-1].matrix = (ij,-1), (ik, 1), (jk, 1)
    for row in lp.rows:                      # For each row.
        row.bounds = None, 1                 # Each row is <= 1.
    glpk.env.term_on = False                 # Make it shut up.
    lp.simplex()                             # Run the optimization.
    labels = [[lp.cols[pair2index(i,j)].value for i in range(j)]
              for j in range(items)]
    return lp.obj.value, labels

def halfint_correlation_clustering(similarity):
    """Wacky MIP-based half-int constrained correlation clustering."""
    lp = glpk.LPX()                          # Define the linear program.
    items = len(similarity)                  # Get the number of items.
    lp.obj.maximize = True                   # Set as maximization.
    lp.cols.add((items * (items-1)) // 2)    # Each item pair has a var.
    for j in range(items):
        for i in range(j):
            index = pair2index(i, j)         # Get the index for this pair.
            lp.cols[index].bounds = 0, 2     # Each variable in range 0 to 1.
            lp.cols[index].kind = int        # This should be integral.
            lp.obj[index]=similarity[j][i]/2 # If 1, this much added to obj.
    for k in range(items):                  # For all triples of items, we
        for j in range(k):                  # want to add in constraints to
            jk = pair2index(j,k)             # enforce respect for the
            for i in range(j):              # triangle inequality.
                ij, ik = pair2index(i,j), pair2index(i,k)
                # We want Xij >= Xik + Xjk - 1.
                # That is, we want 1 >= Xik + Xjk - Xij.
                # Add constraints to enforce this!
                lp.rows.add(3)
                lp.rows[-3].matrix = (ij, 1), (ik, 1), (jk,-1)
                lp.rows[-2].matrix = (ij, 1), (ik,-1), (jk, 1)
                lp.rows[-1].matrix = (ij,-1), (ik, 1), (jk, 1)
    for row in lp.rows:                      # For each row.
        row.bounds = None, 2                 # Each row is <= 1.
    glpk.env.term_on = False                 # Make it shut up.
    lp.simplex()                             # Find the basic solution.
    lp.integer()                             # Find the integer solution.
    labels = [[lp.cols[pair2index(i,j)].value/2 for i in range(j)]
              for j in range(items)]
    return lp.obj.value, labels

# Run the random test.
for i in range(1000):
    if i%100 == 0: print('At seed', i, '...')
    sim = gen_random(7, random.Random(i))
    objval, labels = correlation_clustering(sim)
    # How far off being half integral are we?
    off_half_integer = sum(abs(2*v - round(2*v)) for r in labels for v in r)
    if off_half_integer > .1:
        # We have found a significantly non-half integral solution!
        print('Found for seed', i)
        print('SIMILARITY MATRIX:')
        pretty_print(sim)
        print('LABELS FROM CORRELATION CLUSTERING (objective=%g):' % objval)
        pretty_print(labels)
        # Find the half-int solution, and report it for comparison.
        halfint_objval, halfint_labels = halfint_correlation_clustering(sim)
        print('LABELS FROM HALF-INT CORRELATION CLUSTERING (objective=%g):' % (
            halfint_objval))
        pretty_print(halfint_labels)
        # We have found what we are looking for, so, stop looking.
        break
