#!/usr/bin/env python3
"""DAG path covers, chain covers with Dilworth antichains and weighted antichains versus brute force.

quick: every labelled DAG n<=3 and 300 random DAG multigraphs n<=10 against
successor backtracking (path cover) and subset enumeration (width, weighted
antichain, weights 0 to 2^62); 300-vertex random, dense, grid and two-chain
DAGs with self-checking Dilworth certificates, the unit-weight flow antichain
and a tester-side Kuhn matching; 5000-vertex path cover partition. full:
every labelled DAG n<=4, 3000 random rounds, 2000-vertex shapes, 200000-vertex
path cover, in optimized, checked and ASan/UBSan builds. stress: 15000
rounds, 4000-vertex shapes, 400000-vertex path cover.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('31-path_cover', ['undirected', 'path-cyclic', 'chain-cyclic',
        'weighted-cyclic', 'weight-size', 'weight-negative'], strict=True))
