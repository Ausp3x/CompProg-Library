#!/usr/bin/env python3
"""Partial functional graphs versus explicit orbit and pair-state enumeration.

quick: all partial maps through n=3, 80 random maps and 20000-node chains/
cycles. full: all 8477 partial maps through n=5, 700 random maps and
200000-node chains/cycles in optimized, checked and ASan/UBSan builds.
stress: all maps through n=6, 5000 random maps and 500000-node chains/cycles.
All modes test full unsigned-64 jump/fold counts, noncommutative string and
matrix folds, cycle rotations, Graph/CSR adapters and assertion contracts.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('11-functionalgraph', [
        'successor-negative', 'successor-high', 'undirected', 'two-successors',
        'parallel-successors', 'csr-undirected', 'csr-parallel', 'jump-negative',
        'jump-high', 'distance-negative', 'distance-high', 'reachable-high',
        'meeting-negative', 'meeting-high', 'empty-jump', 'orbit-negative',
        'orbit-high', 'aggregate-size', 'rotated-size',
        'rotated-high', 'fold-size', 'fold-negative', 'fold-high', 'fold-cycle-high',
        'segment-negative', 'segment-high', 'segment-unavailable',
        'step-high', 'step-identity']))
