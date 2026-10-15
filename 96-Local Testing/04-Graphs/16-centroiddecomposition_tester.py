#!/usr/bin/env python3
"""Centroid decomposition, nearest marked, distance sums, contours and pair counts versus brute force.

quick: 150 random forests (n<=120, six shapes, one to four trees, unit,
signed and 2^40 weights) with structure, distance, mark/unmark, distance-sum,
contour sum/add/fold/apply, frequency and path-count oracles; frequency
against BFS pairs at n=1700; 20000-vertex path/star/random/caterpillar/binary.
full: 1500 forests, frequency n<=3000, 200000-vertex shapes, in optimized,
checked and ASan/UBSan builds. stress: 8000 forests, frequency n<=4000,
1000000-vertex shapes. The checked (_GLIBCXX_DEBUG, -O0) build caps full and
stress at frequency n<=3000 and 50000-vertex shapes, and the ASan/UBSan build
caps stress at 200000-vertex shapes, to stay within the timeout and memory cap.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('16-centroiddecomposition', ['directed', 'cycle', 'distance-level', 'nearest-range',
        'negative-weight', 'sum-range', 'contour-size', 'contour-range', 'add-negative', 'fold-cycle', 'fold-range',
        'apply-range', 'frequency-cycle', 'paths-directed'], strict=True))
