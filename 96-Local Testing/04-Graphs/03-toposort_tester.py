#!/usr/bin/env python3
"""Topological orders, extremes, generations and enumeration versus vertex permutations.

Every sort, topologicalGenerations and allTopologicalSorts are checked against
all valid permutations (smallest, largest, earliest position array, count, the
set of orders, relaxation layers). quick: loopless n<=3, looped n<=2, 100
random graphs, 10000-vertex chains, two 6-chains; full: loopless n<=4, looped
n<=3, 1200 random n<=7 graphs, 200000-vertex chains, two 8-chains in optimized,
checked and ASan/UBSan builds; stress extends random to 10000 cases, looped
n<=4, chains to 500000 vertices and two 9-chains.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('03-toposort', ['kahn-undirected',
        'lexicographic-undirected', 'largest-undirected', 'earliest-undirected', 'dfs-undirected',
        'generations-undirected', 'all-undirected', 'all-negative-limit']))
