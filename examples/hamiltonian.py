import glpk

def hamiltonian(edges):
    node2colnums = {} # Maps node to col indices of incident edges.
    for colnum, edge in enumerate(edges):
        n1, n2 = edge
        node2colnums.setdefault(n1, []).append(colnum)
        node2colnums.setdefault(n2, []).append(colnum)

    lp = glpk.LPX()                     # A new empty linear program
    glpk.env.term_on = False            # Stop annoying messages.
    lp.cols.add(len(edges))             # A struct var for each edge
    lp.rows.add(len(node2colnums)+1)    # Constraint for each node

    for col in lp.cols:                 # Go over all struct variables
        col.kind = bool                 # Make binary, not continuous

    # For each node, select at least 1 and at most 2 incident edges.
    for row, edge_column_nums in zip(lp.rows, node2colnums.values()):
        row.matrix = [(cn, 1.0) for cn in edge_column_nums]
        row.bounds = 1, 2

    # We should select exactly (number of nodes - 1) edges total
    lp.rows[-1].matrix = [1.0]*len(lp.cols)
    lp.rows[-1].bounds = len(node2colnums)-1

    assert lp.simplex() is None         # Should not fail this way.
    if lp.status != 'opt': return None  # If no relaxed sol., no exact sol.

    # Return the edges whose associated struct var has value 1.
    return [edge for edge, col in zip(edges, lp.cols) if col.value > 0.99]


#  1----2----3----5
#        \  /
#         \/  Has one H path!
#         4

g1 = [(1,2), (2,3), (3,4), (4,2), (3,5)]
print(hamiltonian(g1))

#  4    5    6
#  |    |    |
#  |    |    | Has no H path!
#  1----2----3

g2 = [(1,2), (2,3), (1,4), (2,5), (3,6)]
print(hamiltonian(g2))

#  4    5----6
#  |    |    |
#  |    |    | Has two H paths!
#  1----2----3

g3 = g2 + [(5,6)]
print(hamiltonian(g3))


def tsp(edges):
    node2colnums = {} # Maps node to col indices of incident edges.
    for colnum, edge in enumerate(edges):
        n1, n2, cost = edge
        node2colnums.setdefault(n1, []).append(colnum)
        node2colnums.setdefault(n2, []).append(colnum)

    lp = glpk.LPX()                     # A new empty linear program
    glpk.env.term_on = False            # Stop annoying messages.
    lp.cols.add(len(edges))             # A struct var for each edge
    lp.rows.add(len(node2colnums)+1)    # Constraint for each node

    lp.obj[:] = [e[-1] for e in edges]  # Try to minimize the total costs.
    lp.obj.maximize = False

    for col in lp.cols:                 # Go over all struct variables
        col.bounds = 0, 1               # Either edge selected (1) or not (0)
        col.kind = int                  # Make binary, not continuous

    # For each node, select two edges, i.e.., an arrival and a departure.
    for row, edge_column_nums in zip(lp.rows, node2colnums.values()):
        row.matrix = [(cn, 1.0) for cn in edge_column_nums]
        row.bounds = 2

    # We should select exactly (number of nodes) edges total
    lp.rows[-1].matrix = [1.0]*len(lp.cols)
    lp.rows[-1].bounds = len(node2colnums)

    assert lp.simplex() is None         # Should not fail this way.
    if lp.status != 'opt': return None  # If no relaxed sol., no exact sol.

    assert lp.integer() is None         # Should not fail this way.
    if lp.status != 'opt': return None  # Count not find integer solution!

    # Return the edges whose associated struct var has value 1.
    return [edge for edge, col in zip(edges, lp.cols) if col.value > 0.99]
