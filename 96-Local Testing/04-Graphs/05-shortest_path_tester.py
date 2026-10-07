#!/usr/bin/env python3
"""Shortest paths: quick smoke/random; full exhaustive tiny graphs + all builds;
stress expands exhaustive/random domains. Exact Floyd/simple-path witness oracles.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('05-shortest_path', [
        'source', 'zero-one', 'dijkstra', 'dense', 'dense-maximum', 'dag-undirected',
        'path-index', 'legacy-size', 'legacy-limit', 'legacy-source',
        'legacy-adjacency', 'legacy-negative', 'result-size',
        'relax-range', 'relax-unreached', 'relax-arc',
    ], strict=True))
