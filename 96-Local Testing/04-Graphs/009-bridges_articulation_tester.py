#!/usr/bin/env python3
"""Lowlink, vertex/edge blocks, Robbins orientation, streaming bridges, even
out-degree orientation and local bridges against independent oracles.

quick: all looped simple graphs n<=3, multiplicity<=2 n<=2, 100 random
n<=7 multigraphs, 20000-vertex chains/cycles/stars, a 20000-vertex stream
(~26000 edges) and 50001-edge dense stream, 10x100 grid with a long tail and wheel local bridges,
and n-copy parallel bundles (closing a triangle; bare beside a 3-edge path);
full: all looped simple graphs n<=4, multiplicity<=2 n<=3, 1500 random
n<=8 multigraphs, 200000-vertex chains/cycles/stars, stream (~260000 edges),
a 1000x100 grid with tail, wheel and 300001-edge dense stream, optimized/checked/ASan-UBSan;
stress: additionally loopless n=6, 12000 random cases, 500000-vertex shapes.
Independent oracles enumerate maximal vertex subsets surviving every vertex
removal, delete each edge/vertex, and close the returned orientation's reachability;
every stream prefix meets an edge-list deletion oracle (large: a tester Tarjan it
validates), orientations meet component edge parity, and local bridges meet
explicit neighbour sets with Bellman-Ford on the graph minus the edge (unit, 0..3
and near-INT64_MAX weights, Graph and CsrGraph).
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('009-bridges_articulation', ['lowlink-directed',
        'lowlink-csr-directed', 'orientation-directed', 'orientation-csr-directed',
        'adjacency-endpoint', 'stream-end', 'even-directed', 'local-directed',
        'local-negative']))
