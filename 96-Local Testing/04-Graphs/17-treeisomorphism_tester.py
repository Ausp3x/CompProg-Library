#!/usr/bin/env python3
"""Tree classification, hashing, orbits, isomorphism mappings, canonical strings and automorphisms.

Oracles: naive AHU strings (sorted child strings per root), brute-force
permutation automorphisms, mapping validation (bijection, edges onto edges).
quick: every labelled tree n<=6 with every root, 300 random pairs per n,
200 symmetric families, 20000-vertex path/star/binary/six shapes with
relabelled copies and degree-changing non-isomorphic pairs. full: n<=7,
3000 pairs per n, 2000 families, 200000-vertex shapes, in optimized, checked
and ASan/UBSan builds. stress: n<=8, 10000 families, 1000000-vertex shapes.
The checked (_GLIBCXX_DEBUG, -O0) build caps full and stress at n<=7, 2000
families and 50000-vertex shapes; the ASan/UBSan build caps stress at n<=7,
2000 families and 200000-vertex shapes, to stay within the timeout.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('17-treeisomorphism', ['directed', 'root-range', 'cycle', 'disconnected', 'empty',
        'hash-root', 'iso-root', 'automorphism-root', 'unrooted-cycle'], strict=True))
