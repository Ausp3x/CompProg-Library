#!/usr/bin/env python3
"""All-pairs shortest paths: floydWarshall, johnson, AllPairsResult, minimaxFloydWarshall,
fixedLengthShortestPath, eccentricities against per-source Bellman-Ford, walk-length DP and threshold-reachability
oracles (self-checked on exhaustive domains). Quick: fixtures, exhaustive n=2, 150 small and 4 medium random graphs,
word boundaries 64/65 (516 cases). Full adds exhaustive n=3 and 2000 + 40 random graphs, boundaries 63..200 (30302 cases);
stress widens all of these (about 281000 cases).
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('12-all_pairs_shortest_path', [
        'ecc-non-square', 'finite-range', 'finite-row', 'finite-row-far', 'finite-column-far', 'fixed-negative-k',
        'fixed-overflow', 'fixed-overflow-far',
        'ecc-empty', 'ecc-negative-infinite', 'path-size',
    ]))
