#!/usr/bin/env python3
"""HLD structure, LCA, paths, rerooted subtrees and TreeMonoid folds, actions and searches versus brute force.

quick: 150 random forests (n<=120, six shapes, one to six trees with isolated
vertices, root=-1 and explicit roots, Graph and CsrGraph) checking the rooting
fields, heavy-first preorder and head chains against a DFS oracle, getLCA,
distance, kthAncestor, processPath (both modes, call count), subtreeRange,
rerootedSubtreeIntervals and reorderArray, then every TreeMonoid configuration
in vertex and edge mode (AffineComposite<mint>, a stateful string-hash
composite, Sum/Min/Max with COMMUTATIVE true and false, MaxGain,
RangeAssignRangeComposite<mint>, RangeAddRangeSum, RangeAffineRangeSum<mint>)
under random set/get/pathFold/subtreeFold/maxPath/pathApply/subtreeApply mixes
against naive path walks, and the legacy edge painting as treeEdgePainting;
20000-vertex deep and shuffled paths, star, caterpillar, binary, random and a
40-tree broom forest. full: 1500 forests and 200000-vertex shapes, in
optimized, checked and ASan/UBSan builds. stress: 8000 forests and
1000000-vertex shapes. The checked (_GLIBCXX_DEBUG) build caps full and
stress at 50000-vertex shapes and stress at 3000 forests, and the ASan/UBSan
build caps stress at 200000-vertex shapes, to stay within the timeout.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('15-hld', ['directed', 'self-loop', 'cycle', 'multi-edge', 'root-range', 'vertex-range',
        'kth-negative', 'edge-range', 'path-range', 'reroot-components', 'values-size', 'root-far', 'vertex-far',
        'vertex-first', 'edge-far', 'path-second', 'subtree-range', 'reroot-vertex', 'reroot-target', 'reorder-short',
        'reorder-long', 'values-short']))
