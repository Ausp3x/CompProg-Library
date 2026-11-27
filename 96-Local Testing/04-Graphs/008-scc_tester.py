#!/usr/bin/env python3
"""SCC partitions/condensation versus independent transitive closure.

Every case also checks attracting components, semiconnectivity, the
strong-connectivity augmentation (size max(sources, sinks), closure of the
augmented graph), the maximum-weight walk (weights up to 2^62, versus
reachability chains of components, and its exact documented witness) and
the reachability realization (closure validity; minimum versus the
weak-component count formula, which an exhaustive search over all arc
subsets confirms for n<=4; the exact documented arc order, with a fixed
pair of 2-cycles whose closing arcs sort differently by first and last).

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
    raise SystemExit(main('008-scc', ['tarjan-undirected', 'kosaraju-undirected',
        'condensation-undirected', 'condensation-size', 'attracting-undirected', 'attracting-size',
        'semi-undirected', 'semi-size', 'augment-undirected', 'augment-size', 'walk-negative', 'walk-size',
        'realize-undirected', 'condensation-size-large', 'attracting-size-large', 'semi-size-large',
        'walk-size-large']))
