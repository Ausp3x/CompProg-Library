#!/usr/bin/env python3
"""Traversal: Floyd distances/reachability, enumerated colorings, forest rank,
recursive colour-DFS arc labels, Floyd-depth BFS arc labels, Floyd-radius ego graphs.

Every graph case runs on Graph and CsrGraph: BFS/DFS from {}, all, {0},
{n-1}, {n-1,0,n-1} with classifyBfsArcs/classifyArcs on each, dfsForest with
classifyArcs, and egoGraph from every center at radius 0, 1, 2 and INT_MAX.
quick: n<=3 simple graphs, 100 random graphs, 20000-vertex chains;
full: directed n<=3 / undirected n<=4, ternary n=3 multigraphs, 1800
random graphs and 200000-vertex chains, all three build configurations;
stress: directed/undirected n<=4, 12000 random graphs, 500000-vertex chains.
Every mode also runs hand-derived arc labels (parallel edges, forward, cross, loop).
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-traversal', ['result-negative', 'bfs-negative',
        'bfs-large', 'bfs-past-end', 'bfs-empty-source', 'dfs-negative', 'dfs-large', 'dfs-past-end',
        'components-directed', 'bipartite-directed', 'classify-size',
        'classify-bfs-result', 'classify-bfs-size', 'classify-small', 'classify-order-short',
        'classify-bfs-small', 'classify-bfs-dfs-result', 'ego-negative-radius',
        'ego-center-large', 'ego-center-past-end', 'ego-center-negative']))
