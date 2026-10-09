#!/usr/bin/env python3
"""Bipartite matching, covers, edge status, DM and b-matching versus enumeration.

quick: simple graphs up to 2x3, 300 random multigraphs (nl, nr <= 6),
60 dynamic add/erase sequences, 20000-vertex paths, dense 300x300.
full: every simple graph up to 3x3, 4000 random multigraphs, 600 dynamic
sequences, 200000-vertex paths and sparse graphs, dense 1500x1500, in
optimized, checked and ASan/UBSan builds. stress: 30000 random multigraphs,
4000 dynamic sequences, 500000-vertex paths.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('21-matching_bipartite', ['edge-range', 'negative-size', 'erase-twice',
        'erase-range', 'cover-not-maximum', 'bmatching-size', 'bmatching-negative'], strict=True))
