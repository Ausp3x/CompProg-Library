#!/usr/bin/env python3
"""Lowlink, vertex/edge blocks and Robbins orientation against deletion oracles.

quick: all looped simple graphs n<=3, multiplicity<=2 n<=2, 100 random
n<=7 multigraphs, 20000-vertex chains/cycles/stars;
full: all looped simple graphs n<=4, multiplicity<=2 n<=3, 1500 random
n<=8 multigraphs, 200000-vertex chains/cycles/stars, optimized/checked/ASan-UBSan;
stress: additionally loopless n=6, 12000 random cases, 500000-vertex shapes.
Independent oracles enumerate maximal vertex subsets surviving every vertex
removal, delete each edge/vertex, and close the returned orientation's reachability.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('09-bridges_articulation', ['lowlink-directed',
        'lowlink-csr-directed', 'orientation-directed', 'orientation-csr-directed',
        'legacy-negative', 'legacy-short', 'legacy-endpoint', 'bridge-dfs-cur',
        'bridge-dfs-prv']))
