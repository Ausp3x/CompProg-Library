#!/usr/bin/env python3
"""Advanced shortest paths: spfa, desopoPape, dialDijkstra, radixHeapDijkstra, aStar, bidirectionalDijkstra,
secondShortestPath, top2Dijkstra, shortestPathDag, countShortestPaths, minimaxPath, widestPath, negativeSsspScaling
against Bellman-Ford, saturated walk-count, tight walk-length and threshold-reachability oracles (self-checked by
walk enumeration on small graphs). Quick: fixtures, exhaustive n=2, 150 small, 8 medium
and 4 large random graphs (613 cases). Full adds exhaustive n=2/3 domains and 2000 + 80 + 30 random graphs (15113 cases);
stress adds n=3 directed with loops and 10000 + 400 + 120 random graphs (about 63000 cases).
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('13-shortest_path_advanced', [
        'dial-negative', 'dial-int-max', 'radix-negative', 'radix-bound',
        'astar-negative', 'astar-range', 'bidirectional-negative', 'bidirectional-range',
        'second-negative', 'second-source-range', 'top2-negative', 'top2-source-range',
        'count-mod-zero', 'count-mod-big', 'exists-rank', 'walk-size', 'dag-size',
        'dial-weight-max', 'astar-target-far', 'exists-negative-rank', 'walk-smaller',
    ]))
