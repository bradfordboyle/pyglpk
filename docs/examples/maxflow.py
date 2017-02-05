import glpk


def maxflow(capgraph, s, t):
    # map non-source/sink nodes to row num
    node2rnum = {}
    for nfrom, nto, cap in capgraph:
        if nfrom != s and nfrom != t:
            node2rnum.setdefault(nfrom, len(node2rnum))
        if nto != s and nto != t:
            node2rnum.setdefault(nto, len(node2rnum))

    # empty LP instance
    lp = glpk.LPX()

    # stop annoying messages
    glpk.env.term_on = False

    # as many columns cap-graph edges
    lp.cols.add(len(capgraph))
    # as many rows as non-source/sink nodes
    lp.rows.add(len(node2rnum))

    # net flow for non-source/sink is 0
    for row in lp.rows:
        row.bounds = 0

    # will hold constraint matrix entries
    mat = []

    for colnum, (nfrom, nto, cap) in enumerate(capgraph):
        # flow along edge bounded by capacity
        lp.cols[colnum].bounds = 0, cap

        if nfrom == s:
            # flow from source increases flow value
            lp.obj[colnum] = 1.0
        elif nto == s:
            # flow to source decreases flow value
            lp.obj[colnum] = -1.0

        if nfrom in node2rnum:
            # flow from node decreases its net flow
            mat.append((node2rnum[nfrom], colnum, -1.0))
        if nto in node2rnum:
            # flow to node increases its net flow
            mat.append((node2rnum[nto], colnum, 1.0))

    # want source s max flow maximized
    lp.obj.maximize = True
    # assign 0 net-flow constraint matrix
    lp.matrix = mat

    # this should work unless capgraph bad
    lp.simplex()

    # return edges with assigned flow
    return [
        (nfrom, nto, col.value)
        for col, (nfrom, nto, cap) in zip(lp.cols, capgraph)
    ]

capgraph = [
    ('s', 'a', 4), ('s', 'b', 1),
    ('a', 'b', 2.5), ('a', 't', 1), ('b', 't', 4)
]
print(maxflow(capgraph, 's', 't'))

[('s', 'a', 3.5), ('s', 'b', 1.0), ('a', 'b', 2.5),
 ('a', 't', 1.0), ('b', 't', 3.5)]
