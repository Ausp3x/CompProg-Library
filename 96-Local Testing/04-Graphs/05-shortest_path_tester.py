#!/usr/bin/env python3
"""Shortest paths: quick smoke/random; full exhaustive tiny graphs + all builds;
stress expands exhaustive/random domains. Exact Floyd/simple-path witness oracles.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('05-shortest_path', [
        'source', 'zero-one', 'dijkstra', 'dense', 'dense-maximum', 'dag-undirected',
        'path-index', 'result-size',
        'relax-range', 'relax-unreached', 'relax-arc',
    ]))
