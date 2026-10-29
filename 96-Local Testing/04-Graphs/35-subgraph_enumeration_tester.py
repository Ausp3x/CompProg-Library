#!/usr/bin/env python3
"""Triangles, 4-cycles, 4-cliques, short paths and the triadic census of 35-subgraph_enumeration.hpp
against independent oracles, every operation on Graph and CsrGraph.

Oracles: an adjacency-matrix brute force (vertex triples; vertex quadruples in
their three cyclic orders; per-edge sums mult(u,w)*mult(w,v) and
mult(v,w)*mult(w,x)*mult(x,u) over distinct w, x; depth-first path enumeration),
itself validated by 3- and 4-edge subset enumeration (k non-loop edges on k
vertices of degree 2, connected) on 13 hand graphs with values derived on paper
(K4, K4 doubled, C4, triangle and square with a doubled edge, two digons and a
bridge, K2,3, bowtie, Petersen, loops only, n=0,1,2) and on every random tiny
multigraph; a codegree oracle in O(sum deg^2 * log) (weighted two-paths from
every vertex, K4 by common neighbours, paths by DFS at 201 sampled starts)
checked against the brute force on every medium graph; closed forms (K_n,
K_a,b, C_n, P_n, stars with plain and doubled spokes plus loops, grids); a
vertex-id bitset triangle counter for dense graphs. triadicCensus: the
NetworkX TRICODES table and _tricode bit convention (networkx/algorithms/
triads.py, main) classify every triple; the table is first checked on all 64
labelled triads against the NetworkX triad_type rules and orbit sizes, on the
16 hand shapes in all 6 labellings (with a repeated arc and a loop) and on the
triadic_census docstring example; large digraphs use a centre-and-neighbour-
pair census validated against the brute force on every random digraph.
Per graph: enumerateTriangles (exact sorted triple list, or count, validity
and uniqueness), countTriangles, triangleProductSum (lng, ulng random and
constant with wraparound, ModInt<998244353>, x=1), countTrianglesPerEdge,
countC4, countC4PerEdge, countC3C4PerVertex, findC4 (witness checked as a
closed walk of 4 distinct vertices along existing arcs; empty iff no 4-cycle),
countK4, countPathsP3P4P5, plus the sum identities (3T, 4*C4, simple per-edge
sums). Metamorphic: repeated calls, vertex relabelling with shuffled edge order
and swapped endpoints, k-fold parallel copies plus loops (simple results equal,
per-edge results times k^2 and k^3), symmetrised digraph census (only 003,
102, 201, 300; 300 = triangles; 201 = open wedges). Dense dispatch:
countTriangles, bitsetTriangles and the sparse loop called directly on
complete and random dense graphs (1% parallel edges, loops) and a density
sweep; both kernels must be exercised. Domain edge (every
mode, m=6*10^6; checked build m=6000, ASan/UBSan the chord and past-domain cases full and the
others at a quarter): countTrianglesPerEdge, countC4PerEdge, countTriangles and
countC4 on a 4-cycle with pair multiplicities (1, 1999998, 2*10^6, 2*10^6) plus
a chord, the equal split ((m-1)/3 per pair with one test edge) and a triangle
(1, 2999999, 3*10^6), each per-edge value against closed-form products in lll
that are checked to be below 2^63 (largest 7999992*10^12). Census boundary:
the largest n with C(n, 3) < 2^63 (3810779) on an empty digraph and with a
3-cycle.
Clustering and enumeration: enumerateTriangleEdges against the exact list of
edge-ID triples (vertex triangles times parallel copies, itself checked against
3-edge subset enumeration on every tiny multigraph; count = sum of per-edge
triangles / 3, endpoints a-b, b-c, c-a); enumerateC4 (canonical rotation and
direction) and enumerateK4 against the brute-force / codegree lists (listed up
to 2*10^6 each, else count plus validity and no repeats among the first 10^6);
NetworkX cluster.py (main) transliterated on the simple graph for
localClustering, transitivity, squareClustering, generalizedDegree and
averageClustering (both count_zeros), compared exactly because both sides divide
the same integers (or the same reals for transitivity, 6T / sum d(d-1) versus
3T / P); directedClustering against _directed_triangles_and_degree_iter, equal
to localClustering on the symmetrised digraph; weightedClustering and
weightedDirectedClustering against the weighted NetworkX iterators with
|difference| <= 1e-10 * the same sum over |terms| (weights 1, random in
[1, 10^6], random in [-5, 5] with max != 0; unit weights exactly equal the
unweighted values). Hand values on paper for K4, C4, K2,3, bowtie, Petersen and
the directed 3-cycle, transitive and complete triangles; closed forms on K_n
(clustering, transitivity 1, square clustering 1 for n >= 4, generalized degree
[0..0, n-1]) and K_a,b (0, 0, square clustering 1 for a, b >= 2). Dense
dispatch calls denseStart from the header: pure sparse (r0 = n), whole graph
(r0 = 0) and proper suffixes must all occur; bitsetTriangles(h, r) for 8 r
against the sparse loop's triangles with lowest rank >= r; K200 + 5000 and
K600 / K640 + 13384, 13385, 15744, 15745, 100000 isolated, paired or randomly
attached vertices (r0 < n, r0 > 0 when the whole graph exceeds DENSE_WORDS).
Past the domain: countC4PerEdge with 2.1*10^6 copies of three pairs plus one
test edge (m = 6300001) equals the lll closed form mod 2^64.
quick: exhaustive simple graphs n<=5 and digraphs n<=3, 300 tiny, 40 medium
rounds, K_n up to 130, K2,2000, cycles/paths/stars n=3000, dense n<=300 and
the boundary, large shapes n=3000 (uniform multigraph m=10^4, dense blocks,
hub), census large n=3000. full: exhaustive n<=6 and digraphs n<=4, 3000 tiny,
400 medium, K_n up to 200 (all operations), K300/K1000/K2000 and random n=1000,
3000 (triangle kernels), K2,50000, shapes n=2*10^5 m=5*10^5 (hub n=5000),
census n=2*10^5 m=5*10^5 and blocks, the census boundary. stress: 20000 tiny,
2000 medium, n=m=3*10^5 (Library Checker counting_c4 scale) plus n=2*10^5
m=10^6, census m=10^6. The checked build (-O0 -D_GLIBCXX_DEBUG, about 50x
slower) runs stress at full size, exhaustive n<=5, K_n up to 40, a tenth of
the large shapes and only the K200 + 5000 dense cores and n<=300 dense graphs;
the ASan/UBSan build (about 10x slower) runs stress at full size, exhaustive
n<=5, K_n up to 66, an eighth of the large shapes and dense graphs up to
n=1000 with the boundary; the optimized build covers the rest. The checked
build runs the 32 assertion probes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('35-subgraph_enumeration', ['directed-enumerate', 'directed-count', 'directed-count-csr',
        'directed-product', 'directed-tri-edge', 'directed-c4', 'directed-c4-edge', 'directed-c3c4-vertex',
        'directed-find-c4', 'directed-k4', 'directed-paths', 'undirected-census', 'undirected-census-csr',
        'product-size-short', 'product-size-long', 'census-overflow', 'census-overflow-4e6',
        'directed-enumerate-c4', 'directed-enumerate-k4', 'directed-local-clustering', 'directed-transitivity',
        'directed-square-clustering', 'directed-generalized-degree', 'undirected-directed-clustering',
        'undirected-directed-clustering-csr', 'directed-enumerate-triangle-edges', 'directed-weighted-clustering',
        'undirected-weighted-directed-clustering', 'weighted-parallel', 'weighted-directed-parallel',
        'weighted-max-zero', 'weighted-directed-max-zero'], strict=True))
