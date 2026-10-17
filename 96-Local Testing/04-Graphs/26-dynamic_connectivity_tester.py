#!/usr/bin/env python3
"""Offline dynamic connectivity, incremental bridges and range edge components versus brute force.

quick: 300 offline rounds (n<=6, T<=10; intervals, insert/erase events with
multiplicity, Sum/Min vertex values, bipartiteness) checked at every queried
time; 2000-vertex insert-only/erase-only sequences against a prefix DSU; 300
incremental-bridge sequences n<=8 with all pairs after each edge and a
3000-vertex comparison with Tarjan lowlink; 300 random multigraphs with every
[l, r) against a DSU, medium m=60, large n=3000 sampled. full: 4000 offline,
50000-vertex sequences, 3000 incremental, 100000-vertex lowlink comparison,
4000 range graphs, medium m=250 all ranges, large n=100000, in optimized,
checked and ASan/UBSan builds. stress: 25000/20000 rounds, 200000-300000
vertices.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('26-dynamic_connectivity', ['odc-vertex', 'odc-time', 'odc-erase',
        'odc-query', 'odc-value', 'ibc-vertex', 'range-query', 'range-directed'], strict=True))
