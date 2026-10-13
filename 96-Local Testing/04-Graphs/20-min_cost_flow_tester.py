"""Min-cost flow (successive shortest paths with potentials): flow, slope, convex
edges, potentials and edge reads against independent oracles.

Every call is checked against an oracle and then certified: edge reads, conservation
to the tracked divergence, the summed edge costs, potentials within [-3B, 2B] giving
every residual arc a nonnegative reduced cost (complementary slackness), and a
legitimate stop (limit, unreachable t, or p[t] - p[s] >= below).
Oracles: exhaustive enumeration of every integral edge flow (min cost per divergence
vector, so resumed calls, other (s, t) pairs, added edges, limits and below are all
exact) on small networks (n <= 5, 300/3000/12000 rounds, int/lng/lll/mixed types);
all three-vertex networks with each ordered pair's cap in {0, 1(, 2)} and cost in
{-1, 2} (4096 quick, 46656 otherwise, minus negative cycles); a unit-step
Bellman-Ford successive-shortest-path oracle on medium networks (n <= 24,
20/300/1000 rounds); capacities scaled below 2^64 and unit costs below 2^58
against the scaled exhaustive curve (60/600/3000); hungarian (min and max) and linearSumAssignment
(per-size curve, any size) on bipartite networks (k = 15/60/120); costs at +-LIM and
capacities at MAX with alternating sinks; large certified networks (a
reverse-labelled DAG chain with negative costs, n = 2000/100000/300000, a cyclic
potential-shifted network with negative arcs added after flow, and convex edges
against their parallel-arc expansion). Full/stress add ASan/UBSan through the driver.
libstdc++ debug mode re-validates the whole heap on every push and pop, so the checked
build runs the large chain at n = 20000, the cyclic network at n = 300, the convex
comparison at n = 80 and the assignment at k = 25 in full and stress modes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('20-min_cost_flow', ['negative-size', 'addEdge-range', 'addEdge-negative',
        'addEdge-cost-high', 'addEdge-cost-low', 'convex-range', 'convex-length', 'convex-order',
        'convex-overflow', 'convex-cost', 'getEdge-range', 'flow-same', 'flow-range', 'flow-limit',
        'slope-same', 'slope-limit', 'negative-cycle', 'negative-loop', 'negative-cycle-after-flow',
        'potentials-cycle', 'path-bound', 'total-bound', 'total-sum-bound', 'potentials-bound'], strict=True))
