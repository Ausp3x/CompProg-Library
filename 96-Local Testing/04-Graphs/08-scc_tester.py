#!/usr/bin/env python3
"""SCC partitions/condensation versus independent transitive closure.

quick: directed loopless n<=3, looped n<=2, 150 random graphs, 20000-node
chains/cycles. full: loopless n<=4, looped n<=3, all two-node graphs with
edge multiplicities 0..2, 1800 random graphs and 200000-node chains/cycles
in optimized, checked and ASan/UBSan builds. stress: looped n<=4, 10000
random graphs and 500000-node chains/cycles.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('08-scc', ['tarjan-undirected', 'kosaraju-undirected',
        'condensation-undirected', 'condensation-size']))
