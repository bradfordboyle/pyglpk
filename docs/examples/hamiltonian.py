import glpk


def hamiltonian(edges):
    # maps node to col indices of incident edges
    node2colnums = {}
    for colnum, edge in enumerate(edges):
        n1, n2 = edge
        node2colnums.setdefault(n1, []).append(colnum)
        node2colnums.setdefault(n2, []).append(colnum)

    # empty LP instance
    lp = glpk.LPX()

    # stop annoying messages
    glpk.env.term_on = False

    # a struct var for each edge
    lp.cols.add(len(edges))
    # constraint for each node
    lp.rows.add(len(node2colnums) + 1)

    # make all struct variables binary
    for col in lp.cols:
        col.kind = bool

    # for each node, select at least 1 and at most 2 incident edges
    for row, edge_column_nums in zip(lp.rows, node2colnums.values()):
        row.matrix = [(cn, 1.0) for cn in edge_column_nums]
        row.bounds = 1, 2

    # we should select exactly (number of nodes - 1) edges total
    lp.rows[-1].matrix = [1.0] * len(lp.cols)
    lp.rows[-1].bounds = len(node2colnums) - 1

    # should not fail this way
    assert lp.simplex() is None
    # if no relaxed sol., no exact sol.
    if lp.status != 'opt':
        return None

    # should not fail this way
    assert lp.integer() is None
    # could not find integer solution
    if lp.status != 'opt':
        return None

    # return edges whose associated struct var has value 1
    return [edge for edge, col in zip(edges, lp.cols) if col.value > 0.99]

"""
1----2----3----5
      \  /
       \/  Has one H path!
       4

Path:
[(1, 2), (3, 4), (4, 2), (3, 5)]
"""

g1 = [(1, 2), (2, 3), (3, 4), (4, 2), (3, 5)]
print(hamiltonian(g1))

"""
4    5    6
|    |    |
|    |    | Has no H path!
1----2----3

Path:
None
"""

g2 = [(1, 2), (2, 3), (1, 4), (2, 5), (3, 6)]
print(hamiltonian(g2))

"""
4    5----6
|    |    |
|    |    | Has two H paths!
1----2----3

Path:
[(1, 2), (1, 4), (2, 5), (3, 6), (5, 6)]
"""

g3 = g2 + [(5, 6)]
print(hamiltonian(g3))
