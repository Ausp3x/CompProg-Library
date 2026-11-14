"""Graph storage contracts: exhaustive small graphs, adapters and snapshots.

Quick: all directed/undirected graphs through n=2, 70 random multigraphs and
100 builder cases. Full: through n=3, 500 random multigraphs and 1000 builder
cases; stress: 4000 random multigraphs and 8000 builder cases. All modes check
every API, extreme weights, input adapters, the fromAdjacency/fromMatrix
builders, vertex labels on CoordinateCompression, the line-graph/induced/
contract/simplify/edge-subgraph/restricted-view/subdivision utilities, the
path/cycle/star builders, complement, graph power, in-degrees, edgeId,
direction conversions, Mycielskian, disjoint union, join, the Cartesian/tensor/
strong/lexicographic/rooted/corona/modular products (each graph, Graph and
CsrGraph, paired with random second factors on both sides: one in quick, two in
full and stress) and the pair-matched union/intersection/difference/symmetric
difference (against a random overlapping graph, both orders, and itself)
against independent oracles: exact edge sequences built from the definitions,
multiplicity-matrix product identities, Floyd hop distances, count maps, and
hand-computed named fixtures (C4, K4, C5, P4 squared, triangle corona).
The shared driver adds checked precondition probes and ASan/UBSan in full/stress.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('01-graph', ['negative-size', 'add-negative', 'add-end', 'add-past-end',
        'row-negative', 'row-end', 'row-past-end', 'csr-row-negative', 'csr-row-end', 'csr-row-past-end', 'input-negative-size',
        'input-negative-count', 'input-base', 'input-base-negative', 'input-truncated', 'input-weight-truncated',
        'input-low', 'input-high', 'input-extreme-low', 'input-extreme-high',
        'adjacency-negative', 'adjacency-end', 'adjacency-asymmetric', 'adjacency-asymmetric-reverse', 'adjacency-multiplicity',
        'adjacency-loop-once', 'adjacency-weight', 'adjacency-loop-weight', 'matrix-ragged', 'matrix-asymmetric', 'matrix-asymmetric-lower',
        'induced-negative', 'induced-end', 'induced-past-end', 'induced-duplicate', 'contract-size', 'contract-count',
        'contract-negative', 'contract-end', 'contract-past-end', 'cycle-empty', 'path-end', 'star-end', 'edge-subgraph-negative',
        'edge-subgraph-end', 'edge-subgraph-past-end', 'edge-subgraph-duplicate', 'restricted-vertex-negative', 'restricted-vertex-end', 'restricted-vertex-past-end',
        'restricted-edge-negative', 'restricted-edge-end', 'restricted-edge-past-end', 'edge-id-negative', 'edge-id-end', 'edge-id-past-end', 'power-negative',
        'union-direction', 'join-direction', 'cartesian-direction', 'tensor-direction', 'strong-direction',
        'lexicographic-direction', 'rooted-direction', 'rooted-negative', 'rooted-end', 'corona-direction', 'corona-direction-reverse',
        'modular-directed', 'modular-mixed', 'product-overflow', 'corona-overflow', 'corona-overflow-window', 'mycielskian-directed',
        'set-union-size', 'set-union-direction', 'set-intersection-size', 'set-intersection-direction',
        'set-difference-size', 'set-difference-direction', 'set-symmetric-size', 'set-symmetric-direction']))
