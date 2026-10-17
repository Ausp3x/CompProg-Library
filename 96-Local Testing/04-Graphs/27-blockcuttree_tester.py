#!/usr/bin/env python3
"""Block-cut and bridge trees, removal queries, augmentation and incremental blocks versus brute force.

quick: every simple graph n<=4 and 300 random multigraphs n<=7 with loops,
isolated vertices and parallel edges; blocks against cycle-union classes,
every query against vertex/edge deletion BFS and simple-path enumeration,
augmentation validity plus exhaustive minimality (n<=5), incremental blocks
after every insertion; 3000-vertex random/path/ladder shapes against deletion
BFS, the incremental structure and lowlink. full: every simple graph n<=5,
3000 random rounds, 100000-vertex shapes, in optimized, checked and
ASan/UBSan builds. stress: 15000 rounds, 300000-vertex shapes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('27-blockcuttree', ['directed', 'bridge-directed', 'vertex-range',
        'removed-endpoint', 'size-removed', 'edge-range', 'incremental-range'], strict=True))
