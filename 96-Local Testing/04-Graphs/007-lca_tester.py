"""LCA variants against independent BFS parent/path oracles.

Quick exhausts labeled forests through n=4, checks 40 random weighted forests
and a 2000-vertex chain. Full exhausts through n=6, checks 350 random forests,
FCB block-boundary shapes and a 200000-vertex chain. Stress adds 2500 random
forests and a 500000-vertex chain. Every small forest/root checks all pairs,
path positions, ancestor distances, rerooted queries, vertex/edge string
path folds and path intersections (all quadruples through n=5 with the
default root, otherwise 100 samples, or 10 per explicit root; folds use every
root through n=4 and roots -1, 0, n-1 for n=5..6). LCA, EulerLCA and
SparseTableLCA (Graph and CsrGraph) share every pair check; meet runs all
triples against the common vertex of the three BFS paths and lcaSet every
vertex subset through n=6 (200 random multisets above), each in given and
reversed-plus-duplicate order, against parent-walk ancestor intersections, on
LCA, LCAFold, EulerLCA and SparseTableLCA; the chain checks both on random
triples and sets. Full/stress include optimized, checked and ASan/UBSan
builds through the shared driver.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('007-lca', ['directed', 'cycle', 'loop', 'parallel',
        'root-negative', 'root-end', 'empty-root', 'adjacency-label', 'query-negative', 'query-end',
        'ancestor-negative', 'jump-negative', 'reroot-end', 'euler-query-end',
        'offline-query-end', 'euler-cycle', 'offline-cycle', 'intersection-end',
        'fold-size', 'fold-query-end', 'sparse-query-end', 'sparse-cycle', 'meet-end', 'set-end',
        'root-beyond', 'euler-query-first', 'intersection-first', 'intersection-second'], timeout=1000))
