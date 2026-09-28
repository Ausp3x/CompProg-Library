"""Canonical graph DSU adapter against independent transitive closure.

Quick exhausts graphs through n=3 and uses 50 random cases; full/stress exhaust
through n=4 and use 500/4000 random cases. Large chains check sparse scaling.
The canonical DSU's richer standalone suite remains owned by Data Structures.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('04-dsu', ['negative-size', 'negative-vertex', 'end-vertex']))
