#!/usr/bin/env python3
"""Topological order/cycle certificates versus enumerated vertex permutations.

quick: loopless n<=3, looped n<=2, 100 random graphs, 20000-vertex chains;
full: loopless n<=4, looped n<=3, 1200 random n<=7 graphs and 200000-vertex
chains in optimized, checked and ASan/UBSan builds; stress extends random to
10000 cases, looped n<=4 and chains to 500000 vertices.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('03-toposort', ['kahn-undirected',
        'lexicographic-undirected', 'dfs-undirected'], strict=True))
