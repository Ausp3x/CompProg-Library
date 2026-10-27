#!/usr/bin/env python3
"""Planar embeddings: faces, genus, drawings, commonFace, dual, tree-cotree and dual min cuts versus brute force.

Every embedding is compared field by field with an independent orbit oracle,
including genus[c] = (2 - (V_c - E_c + F_c)) / 2 from the oracle's per-component
counts (an isolated vertex's empty face counts in its component) and
2 * sum(genus) = 2 * components - eulerCharacteristic(). quick: hand cases
(n=0, n=1, single edge, triangle, loop, parallel pair, bridge, K3,3 with genus
>= 1, triangle + K3,3 with genus [0, >=1] and cuts in the triangle, 4x4 grid);
300 random crossing-free straight-line drawings n<=12 against the orbit and
exact angle oracle (atan2-validated), face-area signs and the lowest-vertex
outer rule, chi = 2c and genus 0 everywhere, input-rotation independence,
commonFace for all pairs, dualGraph, tree-cotree, and planarMinCutDual over
every ordered face-sharing or disconnected pair with undirected capacities
plus small or 2^61-scale directed capacities (zeros included) against brute
subset cuts (value, cut arcs, residual separation, arc direction, flow bounds
and conservation, zero flow outside the s-t component; MaxFlow cross-check);
50 drawings scaled to |x|,|y|<=2^61 (metamorphic to the unscaled embedding)
and extreme corners; 400 random rotations of multigraphs n<=7 with loops and
multiedges (orbits, genus, CsrGraph, commonFace, dual, tree-cotree, brute cuts
on genus-0 rotations and, once per graph, in the genus-0 components of a
mixed-genus rotation); 30 mixed-genus graphs: a random drawing n<=10 plus K3,3
(default or shuffled rotation) or a genus-1 rotation of K4, vertex blocks in
random order and edges interleaved, with genus per component, orbits, CsrGraph,
dual, tree-cotree, every face-sharing pair of the planar part against brute
cuts on the drawing alone and MaxFlow on the whole graph, and cross-component
pairs (either order) giving the zero cut; 3 drawings n<=200 with cuts against
MaxFlow; a 40x40 grid with diagonals and a 20000-vertex random tree and cycle
(path-min cut oracles). full: 4000 drawings, 500 huge, 5000 rotations, 300
mixed, 40 mid drawings, 300x300 grid and 200000-vertex tree and cycle, in
optimized, checked and ASan/UBSan builds; only the checked build
(-D_GLIBCXX_DEBUG) keeps the mid drawings (3) and the grid (40x40) at quick
size, because libstdc++ debug mode re-validates the heap on every push and pop.
stress: 5x rounds with a 450x450 grid and 600000-vertex tree and cycle in the
optimized build; 2x rounds with the full-mode grid and 200000-vertex tree and
cycle in ASan/UBSan, and 2x rounds with the quick-size mid drawings and grid
and 200000-vertex tree and cycle in the checked build. The
checked build runs 17 assertion probes; the genus probes (K3,3, genus-1 K4,
and s, t inside the K3,3 of a triangle + K3,3 graph) die on the genus of s's
component.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('29-planar_graph', ['directed', 'common-same', 'common-range',
        'cut-same', 'cut-range', 'cut-genus-k33', 'cut-genus-k4', 'cut-genus-mixed', 'cut-no-common-face', 'cut-negative',
        'cut-negative-weight', 'cut-cap-size', 'drawing-loop', 'drawing-parallel', 'drawing-coincident',
        'drawing-range', 'drawing-size'], strict=True))
