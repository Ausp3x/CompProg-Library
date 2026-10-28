"""Flows with lower bounds and vertex supplies: feasibleCirculation, feasibleFlow,
max/min s-t values and the min-cost circulation adapter against independent oracles.

Every result is certified: a feasible witness (bounds, conservation, value and
cost recomputed in lll) plus optimality (no s->t or t->s residual path and the
returned stCut tight in lll, value == hi(out) - lo(in) - b(C) for max and
lo(out) - hi(in) - b(C) for min; no negative residual cycle by Bellman-Ford and
complementary slackness under the returned potentials for min cost), or a strict
Hoffman cut b(S) > hi(out) - lo(in) with s and t on one side; stCut and
potentials are empty where not promised. feasibleFlow must agree with max on
feasibility and lie in [min, max]. Every call repeats (explicit zero supplies
when none) and must be identical; circulation feasibility must equal "0 in
[min, max]"; the adapter also runs a tester-side Bellman-Ford successive-
shortest-path backend with Bellman-Ford potentials (m <= 40). Oracles: 13 hand
cases validating the exhaustive integer-flow brute force; every two-vertex
network (loops, parallel, antiparallel, bounds in [-2, 2], cost -2 or 1, three
supply vectors) with m <= 1 (quick) or m <= 2 (full, stress); random tiny
networks n <= 4, m <= 4 against brute force (400/4000/20000 rounds, Cap/Cost
int/int, lng/lng, lll/lll, lng/int, int/lng); W = MAX hand cases and
brute-forced networks scaled by MAX / W for int/int, int/lng, lng/lng and
lll/lng (100/1000/5000 rounds; lll costed runs at MAX / 8, inside MinCostFlow's
bounded()); the cost bound sum |c| * max(|lo|, |hi|) at 2^126 and 2^125 with
lll/lng in all 24 edge orders; medium certified networks n <= 40 cross-checked
across two type pairs (60/600/3000); large uncosted certified networks
n = 2000/30000/100000 for Cap lng and int and n / 4 for lll (random, planted
with supplies, planted circulation, forced chain), costed n = 150/1500/3000
(bounds 20) and n = 100/300/600 (bounds 10^12) (checked build: a tenth).
Optimized, checked (21 assertion probes) and, outside quick, ASan/UBSan builds
through the driver.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('30-flow_with_demands', ['size-negative', 'lo-greater-hi', 'cost-lo-greater-hi',
        'supply-size', 'supply-sum', 'cost-supply-sum', 'vertex-range', 'vertex-negative', 's-equals-t',
        's-range', 't-range', 'w-overflow', 'w-overflow-negative', 'w-overflow-supply', 'w-overflow-lll',
        'w-overflow-cost', 'cap-min-cost', 'cost-min', 'cost-min-lng', 'cost-bound', 'cost-bound-exact'], strict=True))
