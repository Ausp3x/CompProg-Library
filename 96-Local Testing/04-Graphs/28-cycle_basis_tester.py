#!/usr/bin/env python3
"""Cycle and cut bases, cycle-space dimension, GF(2) independence, XOR walks and the cut-space hash versus brute force.

Every mode first self-tests the oracles on hand graphs (including a square whose
reversed adjacency changes the reference BFS forest), then runs special cases
(n=0, n=1, loops, parallel chords, isolated vertices, K1..K9, a tree, medium
multigraphs with m>64, a rank-64 XOR basis with count 2^64, the reversed-
adjacency square against the header's forest, and 64 seeds s, s+1 on an
all-chord graph: determinism, distinct labels, no one-edge label shift). Per
graph: forest against a reference queue BFS that scans g[u] in adjacency order
plus structural proof (roots are component minima, parent at depth-1, every
edge spans <=1 level, n-c edges); chords and cycles as simple closed walks with
brute GF(2) rank; every cut against a forest-split brute force and cycle/cut
duality; Graph and CsrGraph agree; dimension against even-degree subset
enumeration (m<=12) and DSU; isIndependentF2 against subset enumeration or
Gauss-Jordan rank (duplicates, empty sets, k>m, cycles+cuts); XorWalk with
weights < 2^B (B<=4) against a (vertex, xor) BFS for count, every contains
value, min, max and every kth, and with full-range weights against a
stack-search forest and pivot basis (exact dist, component and RREF basis,
enumerated kth for rank<=7, sampled kth monotonicity otherwise); CutSpaceHash
against deletion DSU on every edge subset (m<=8) or 100 random distinct
subsets of <=6 edges, two seeds and the default seed, seed determinism, and
no one-edge label shift between seeds s and s+1 when >=3 chords have
consecutive IDs. Shuffled adjacency (each g.adj[v] permuted before the
operations, so adjacency order differs from edge-ID order): a third of the
random graphs, a third of the exhaustive graphs with n>=3 (checked again after
the unshuffled run), half the mid-size hash graphs and the large sparse shape.
Mid-size CutSpaceHash sets (7..64 distinct edges: uniform random sizes, k=64
exactly, chords of the BFS forest, a fundamental cut whole or minus one edge
padded with chords, a vertex star padded at random) on dense multigraphs
n<=12, m in [20, 80] with loops and multiedges (40/400/1500 graphs, 15 sets
each, a tenth also with the default seed) and 15 (quick) or 30 per large shape,
against deletion DSU: k<=44 must match exactly for fixed seeds (k<=40 for
the default seed; false-true odds below 2^-20 per set); for larger k a false
true has probability up to 2^(k-64) by contract, so only disconnecting sets
are checked (one-sided). Large shapes (path, cycle, random tree with local chords,
random sparse disconnected with loops and multiedges): forest, chords, cycle
walks and duality in full, 8 sampled cuts by brute force, DSU dimension,
XorWalk against the oracle on 2000 (quick) or 6000 random pairs, 64-cycle
independence, CutSpaceHash on 60/40 sampled sets of <=6 edges against deletion
DSU, 5000 analytic path/cycle queries and a 65-edge set. quick: every simple
graph n<=4, 300 random multigraphs n<=7 m<=12 with loops (1% medium n<=44
m<=119), 40 mid-size hash graphs, 3000-vertex shapes; optimized and checked
builds. full: every simple graph n<=5, 3000 random rounds, 400 hash graphs,
200000-vertex shapes, plus ASan/UBSan. stress: 15000 rounds, 1500 hash graphs,
300000-vertex shapes in the optimized and ASan/UBSan builds; the checked build
(-O0 -D_GLIBCXX_DEBUG) keeps full-mode rounds and shapes in stress so it stays
far below the runner's 180 s limit. The checked build runs the 7 assertion
probes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('28-cycle_basis', ['directed', 'independent-range', 'max-disconnected', 'kth-range',
        'disconnects-duplicate', 'disconnects-range', 'count-range'], strict=True))
