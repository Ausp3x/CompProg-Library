#!/usr/bin/env python3
"""Euler witnesses versus independent edge-subset path enumeration.

quick: all looped directed n<=2/undirected n<=3 graphs, 120 random cases,
20000-edge chains/cycles. full: directed n<=3/undirected n<=4, 1500 random
graphs with <=10 edges, all two-node edge multiplicities 0..2, 200000-edge
chains/cycles in optimized, checked and ASan/UBSan builds. stress extends
random to 10000 cases and chains/cycles to 500000 edges.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('10-euleriantrail', ['negative-start', 'high-start', 'empty-start']))
