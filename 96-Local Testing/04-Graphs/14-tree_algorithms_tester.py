#!/usr/bin/env python3
"""Tree diameter, eccentricities, centroid, distance sums, Euler tour, BFS numbering, rerooting, subtree DPs, virtual tree and Pruefer codes versus brute force.

Oracles: all-pairs DFS distances, removal BFS for centroids, an independent
iterative preorder/BFS rooting with parent-walk subtree checks, BFS balls,
naive LCA closure, a naive BFS-order DP from every root (height, exact
canonical text, labelled weighted hash; ordered text and hash for
subtreeDp), subset enumeration (max-plus, connected sets), Pascal rows,
naive smallest-leaf Pruefer encode/decode.
quick: 300 random trees n<=120 (six shapes, six weight classes including
full-range and +-2^62) plus 300 forests n<=120 with one to four trees and
random roots, exhaustive Pruefer codes n<=6 and 300 random codes,
20000-vertex deep paths (identity unit, shuffled full-range) and
random/star/caterpillar/binary/broom trees. full: 3000 rounds, Pruefer n<=7
and 3000 codes, 200000-vertex shapes, in optimized, checked and ASan/UBSan
builds. stress: 12000 rounds, Pruefer n<=8, 500000-vertex shapes. To stay
within the timeout the checked (_GLIBCXX_DEBUG, -O0) build caps full at 1000
rounds, stress at 2000 rounds and all shapes at 50000 vertices; the ASan/UBSan
build caps full at 1500 rounds, stress at 2000 rounds, the deep paths at
200000 vertices and the other shapes at 50000 vertices.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('14-tree_algorithms', ['diameter-directed', 'diameter-cycle', 'diameter-forest',
        'diameter-empty', 'eccentricity-cycle', 'eccentricity-empty', 'centroid-forest', 'centroid-empty', 'distances-directed', 'euler-root', 'euler-cycle',
        'euler-multi-edge', 'subtree-range', 'bfs-loop', 'level-range', 'disk-negative', 'rerooting-forest',
        'rerooting-nonadjacent', 'subtree-dp-root', 'merge-negative-k', 'virtual-range', 'virtual-cycle',
        'prufer-single', 'prufer-forest', 'prufer-value', 'prufer-negative',
        'subtree-range-far', 'level-range-far', 'disk-range', 'rerooting-get-range', 'rerooting-parent-far', 'prufer-value-far'], strict=True))
