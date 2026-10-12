"""Graph storage contracts: exhaustive small graphs, adapters and snapshots.

Quick: all directed/undirected graphs through n=2, 70 random multigraphs and
100 builder cases. Full: through n=3, 500 random multigraphs and 1000 builder
cases; stress: 4000 random multigraphs and 8000 builder cases. All modes check
every API, extreme weights, input adapters, the fromAdjacency/fromMatrix
builders, vertex labels on CoordinateCompression and the line-graph/induced/
contract/simplify utilities against independent oracles.
The shared driver adds checked precondition probes and ASan/UBSan in full/stress.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('01-graph', ['negative-size', 'add-negative', 'add-end',
        'row-negative', 'row-end', 'csr-row-negative', 'csr-row-end', 'input-negative-size',
        'input-negative-count', 'input-base', 'input-truncated', 'input-weight-truncated',
        'input-low', 'input-high', 'input-extreme-low', 'input-extreme-high',
        'adjacency-negative', 'adjacency-end', 'adjacency-asymmetric', 'adjacency-multiplicity',
        'adjacency-loop-once', 'adjacency-weight', 'adjacency-loop-weight', 'matrix-ragged', 'matrix-asymmetric',
        'induced-negative', 'induced-end', 'induced-duplicate', 'contract-size', 'contract-count',
        'contract-negative', 'contract-end'], strict=True))
