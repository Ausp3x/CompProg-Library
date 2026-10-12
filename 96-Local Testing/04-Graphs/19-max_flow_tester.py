"""Max flow engines (Dinic, scaling, push-relabel, Edmonds-Karp), single augmenting
paths, cuts, decomposition and dynamic capacities against independent oracles.

Quick runs 300 random networks (n <= 8, loops, multiedges, reverse capacities,
capacities 1, 6, 2^40, 2^61), all 81 looped two-vertex multiplicity networks
and 2000-vertex certified large cases. Full uses 4000 random networks, all
19683 three-vertex networks and 200000-vertex large cases; stress uses 20000
random networks and 500000 vertices. Small values are compared with a brute
minimum over every s-side vertex subset (max-flow min-cut theorem); large
values are certified by a feasible conserving flow plus an equal-capacity
residual cut. Every small round also checks each augmentPath step against an
independent residual BFS (shortest length, exact bottleneck) up to the maximum,
decomposes an arbitrary valid flow set by changeEdge, and every third round
builds its network from a dense matrix through Graph::fromMatrix; medium rounds
(30/300/1500) certify n in [20, 200).
Full/stress add ASan/UBSan through the driver.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('19-max_flow', ['negative-size', 'addEdge-range', 'addEdge-negative',
        'addEdge-overflow', 'flow-same', 'flow-range', 'flow-limit', 'edmondskarp-same', 'edmondskarp-limit',
        'augmentpath-same', 'augmentpath-range', 'scaling-same', 'pushrelabel-same',
        'pushrelabel-supply', 'getEdge-range', 'changeEdge-flow', 'changeEdge-reverse',
        'changeCapacity-negative', 'changeCapacity-same', 'minCut-range', 'decomposition-same',
        'graph-negative', 'graph-undirected-overflow'], strict=True))
