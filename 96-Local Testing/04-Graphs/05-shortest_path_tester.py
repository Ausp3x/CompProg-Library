#!/usr/bin/env python3
"""Shortest paths: quick smoke/random; full exhaustive tiny graphs + all builds;
stress expands exhaustive/random domains. Exact Floyd/simple-path witness oracles cover
every engine, the shortestPaths dispatcher, zeroOneBfs on {0, c} weights, voronoiCells,
egoGraphWeighted, criticalPath and dagLongestPath (path enumeration on small DAGs).
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('05-shortest_path', [
        'source', 'source-past', 'zero-one', 'zero-one-smaller', 'dijkstra', 'dense', 'dense-maximum', 'dag-undirected',
        'finite-past', 'path-index', 'path-small-graph', 'path-large-graph', 'result-size',
        'relax-range', 'relax-to-past', 'relax-from-end', 'relax-from-past', 'relax-unreached', 'relax-arc',
        'zero-one-negative', 'voronoi-negative', 'voronoi-center',
        'ego-negative-radius', 'ego-center', 'ego-negative',
        'critical-undirected', 'dag-longest-undirected',
    ]))
