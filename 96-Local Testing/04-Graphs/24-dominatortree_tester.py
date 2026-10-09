#!/usr/bin/env python3
"""Dominator trees, dominance and frontiers versus dataflow dominator sets.

quick: every looped digraph n<=3 with every root, 300 random multigraphs
(n<=14), 20000-vertex chain/ladder/star, 400-vertex Lengauer-Tarjan versus
vertex deletion. full: every looped digraph n<=4, 4000 random multigraphs,
200000-vertex shapes, 3000-vertex comparison, in optimized, checked and
ASan/UBSan builds. stress: 30000 random multigraphs, 1000000-vertex shapes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('24-dominatortree', ['undirected', 'root-range', 'simple-root',
        'frontier-size', 'query-range', 'tree-root', 'tree-range'], strict=True))
