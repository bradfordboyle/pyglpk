import glpk


def tsp(edges):
    # Maps node to col indices of incident edges.
    node2colnums = {}
    for colnum, edge in enumerate(edges):
        n1, n2, cost = edge
        node2colnums.setdefault(n1, []).append(colnum)
        node2colnums.setdefault(n2, []).append(colnum)

    # empty LP instance
    lp = glpk.LPX()

    # stop annoying messages
    glpk.env.term_on = False

    # A struct var for each edge
    lp.cols.add(len(edges))
    # constraint for each node
    lp.rows.add(len(node2colnums)+1)

    # try to minimize the total costs
    lp.obj[:] = [e[-1] for e in edges]
    lp.obj.maximize = False

    # make all struct variables binary
    for col in lp.cols:
        col.kind = bool

    # for each node, select two edges, i.e.., an arrival and a departure
    for row, edge_column_nums in zip(lp.rows, node2colnums.values()):
        row.matrix = [(cn, 1.0) for cn in edge_column_nums]
        row.bounds = 2

    # we should select exactly (number of nodes) edges total
    lp.rows[-1].matrix = [1.0]*len(lp.cols)
    lp.rows[-1].bounds = len(node2colnums)

    assert lp.simplex() is None
    # if no relaxed sol., no exact sol.
    if lp.status != 'opt':
        return None

    # should not fail this way
    assert lp.integer() is None
    if lp.status != 'opt':
        # could not find integer solution
        return None

    # return the edges whose associated struct var has value 1
    return [edge for edge, col in zip(edges, lp.cols) if col.value > 0.99]
