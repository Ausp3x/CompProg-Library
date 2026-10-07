#!/usr/bin/env python3
"""Traversal: Floyd distances/reachability, enumerated colorings, forest rank.

quick: n<=3 simple graphs, 100 random graphs, 20000-vertex chains;
full: directed n<=3 / undirected n<=4, ternary n=3 multigraphs, 1800
random graphs and 200000-vertex chains, all three build configurations;
stress: directed/undirected n<=4, 12000 random graphs, 500000-vertex chains.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-traversal', ['result-negative', 'bfs-negative',
        'bfs-large', 'bfs-empty-source', 'dfs-negative', 'dfs-large',
        'components-directed', 'bipartite-directed'], strict=True))
