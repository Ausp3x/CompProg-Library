"""Graph storage contracts: exhaustive small graphs, adapters and snapshots.

Quick: all directed/undirected graphs through n=2 and 70 random multigraphs.
Full: through n=3 and 500 random multigraphs; stress adds 4000 random cases.
All modes check every API, extreme weights, label ordering, input adapters and
the line-graph/induced/contract/simplify utilities against independent oracles.
The shared driver adds checked precondition probes and ASan/UBSan in full/stress.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('01-graph', ['negative-size', 'add-negative', 'add-end',
        'row-negative', 'row-end', 'csr-row-negative', 'csr-row-end', 'input-negative-size',
        'input-negative-count', 'input-base', 'input-truncated', 'input-weight-truncated',
        'input-low', 'input-high', 'input-extreme-low', 'input-extreme-high',
        'label-missing-low', 'label-missing-high', 'label-negative', 'label-end',
        'induced-negative', 'induced-end', 'induced-duplicate', 'contract-size', 'contract-count',
        'contract-negative', 'contract-end'], strict=True))
