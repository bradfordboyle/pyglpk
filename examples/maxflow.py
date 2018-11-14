import glpk

def maxflow(capgraph, s, t):
    node2rnum = {}                      # Map non-source/sink nodes to row num.
    for nfrom, nto, cap in capgraph:
        if nfrom!=s and nfrom!=t:
            node2rnum.setdefault(nfrom, len(node2rnum))
        if nto!=s and nto!=t:
            node2rnum.setdefault(nto, len(node2rnum))

    lp = glpk.LPX()                     # Empty LP instance.
    glpk.env.term_on = False            # Stop annoying messages.
    lp.cols.add(len(capgraph))          # As many columns cap-graph edges.
    lp.rows.add(len(node2rnum))         # As many rows as non-source/sink nodes

    for row in lp.rows: row.bounds = 0  # Net flow for non-source/sink is 0.

    mat = []                            # Will hold constraint matrix entries.

    for colnum, (nfrom, nto, cap) in enumerate(capgraph):
        lp.cols[colnum].bounds = 0, cap # Flow along edge bounded by capacity.

        if nfrom == s:
            lp.obj[colnum] =  1.0       # Flow from source increases flow value
        elif nto == s:
            lp.obj[colnum] = -1.0       # Flow to source decreases flow value

        if nfrom in node2rnum:          # Flow from node decreases its net flow
            mat.append((node2rnum[nfrom], colnum, -1.0))
        if nto in node2rnum:            # Flow to node increases its net flow
            mat.append((node2rnum[nto], colnum, 1.0))

    lp.obj.maximize = True              # Want source s max flow maximized.
    lp.matrix = mat                     # Assign 0 net-flow constraint matrix.

    lp.simplex()                        # This should work unless capgraph bad.

    return [(nfrom, nto, col.value)     # Return edges with assigned flow.
            for col, (nfrom, nto, cap) in zip(lp.cols, capgraph)]

capgraph = [ ('s','o',3), ('s','p',3), ('o','p',2), ('o','q',3),
             ('p','r',2), ('q','r',4), ('q','t',2), ('r','t',3) ]
print(maxflow(capgraph, 's', 't'))

capgraph = [('s','a',4), ('s','b',1), ('a','b',2.5), ('a','t',1), ('b','t',4)]
print(maxflow(capgraph, 's', 't'))
