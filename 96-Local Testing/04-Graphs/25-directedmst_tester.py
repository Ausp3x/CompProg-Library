#!/usr/bin/env python3
"""Directed MST (all roots) and maximum branching versus exhaustive search and Chu-Liu/Edmonds.

quick: every simple digraph n<=3 in three weight domains (small signed,
nonnegative, full-range extremes), 400 random multigraphs n<=6 with loops and
parallel arcs, 500 builder arc lists n<=5 with lll weights up to
2^64-1 and custom IDs, 20 medium graphs n<=60 against an O(VE) Chu-Liu/Edmonds for
every root, 3000-vertex witness checks. full: every simple digraph n<=4,
5000 random multigraphs, 5000 builder lists, 200 medium graphs, 100000-vertex random graph and
cycle, in optimized, checked and ASan/UBSan builds. stress: 30000 random,
20000 builder, 600 medium, 300000-vertex shapes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('25-directedmst', ['undirected', 'root-range', 'branching-undirected',
        'arc-range', 'arc-weight', 'arc-weight-64', 'negative-n'], strict=True))
