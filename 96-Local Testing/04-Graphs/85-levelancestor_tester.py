"""LevelAncestor and offlineLevelAncestor against independent BFS parent-walk and root-path DFS oracles.

Every checked graph also verifies the Forest fields (parent, dep, root), that each bottom is a deepest
leaf, the ladder tiling with ladder.size() <= 2n, and jumps.size() = sum of bit_width(dep) over leaves.
Quick: n=0/1 boundaries, many queries on one vertex, snapshot/copy semantics; every labeled forest
through n=4 (counts checked against A001858) with every root in {-1, 0..n-1} and every k in [0, n+1]
plus INT_MAX, 2^40, LLONG_MAX, online, CSR and offline (all pairs, shuffled with duplicates, empty);
60 random forests (n <= 160, several components, shuffled edges); path, star, binary, caterpillar,
broom, recursive, comb, spider, path-binary, deep-random and forest shapes at n in {2..128} near powers
of two, natural and relabeled, roots -1, random, 0, n-1, n/2, every k in [0, dep+1]; 20000 sampled
queries on a 2000-vertex path (roots -1, n-1, n/2, closed forms) and random recursive tree.
Full: labeled forests through n=6, 600 random forests, shapes through n=513 (extra roots only for the
path above n=128), 200000 sampled queries on 200000-vertex path, caterpillar, recursive, deep-random,
path-binary, comb and forest. Stress: labeled forests through n=7, 4000 random forests, two rounds per
large random shape, plus a 500000-vertex path (three roots), caterpillar and recursive.
Builds: optimized (-DNDEBUG, also negative k returns -1), checked, and ASan/UBSan in full/stress.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('85-levelancestor', ['directed', 'cycle', 'loop', 'parallel',
        'root-negative', 'root-end', 'empty-root', 'query-negative', 'query-end', 'k-negative',
        'offline-directed', 'offline-cycle', 'offline-root-end', 'offline-query-negative',
        'offline-query-end', 'offline-k-negative'], strict=True))
