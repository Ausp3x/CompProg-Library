"""Advanced knapsack verified against enumeration, copy expansion and independent closed forms.

All modes check hand-computed fixtures and every operation against an independent
oracle: exact tables (monotone queue, SMAWK small weights, flat and diagonal 2D
embeddings, singleton groups, forest of roots) against copy expansion, itself
validated by multiplicity enumeration over a 78-item alphabet (n <= 2, capacity
<= 5 quick, 7 full/stress); groupKnapsack and twoDimensionalKnapsack against
enumeration; treeKnapsack against closed-set enumeration and an O(n * W^2) merge
DP, plus a long path, a large random forest, and a caterpillar and a star sized so
that a wrong child order or a kept light-child table exceeds the memory cap in
full/stress; RemovableKnapsackCounts after 30
random add/remove steps (lng, ModInt 998244353, ModInt 2) and knapsackCountsFps
(moduli 998244353, 1e9+7, 7, 2) against naive and running-sum products;
kthBestKnapsack and countOptimalSelections against enumerated selections and a
direct copy-number recurrence (bounded counts, zero-weight copies, unlimited
zero-weight items of value <= 0, k up to 60, 31 items at capacity 150/1500/2000)
plus INT_MAX-copy zero-weight fixtures;
knapsackBranchAndBound against 2^n enumeration and a DP with witness checks,
including n=2000/100000/200000 with a search path n items deep;
pisingerSubsetSum against reachability with traced witnesses;
unboundedKnapsackLargeCapacity against a plain DP and the pigeonhole formula for
capacities to INT64_MAX; boundedKnapsackSmallValue against a capacity DP, a full-count
value DP, the pigeonhole formula, all-fit sums and enumeration with weights to 2^62; items of weight above INT_MAX
leave every int-capacity table unchanged (50/300/1000 rounds). Quick/full/stress run
300/3000/20000 seeded rounds per family (group/2D and tree 200/2000/12000,
counting 60/500/3000, Pisinger 500/5000/30000) and grow the large
fixtures (path n=5000/200000/300000, FPS t=2000/30000/100000, Pisinger n up to
10000); the -O0 debug-container build shrinks large fixtures and runs stress as
full, and the ASan build runs stress as full (the runner's 180 s limit); 59 probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('29-knapsack_advanced', [
        'monotone-capacity', 'monotone-capacity-max', 'monotone-weight', 'monotone-count', 'monotone-free',
        'small-weight-capacity', 'small-weight-capacity-negative', 'small-weight-free', 'group-capacity',
        'group-capacity-max', 'group-count',
        'group-unlimited', 'two-capacity', 'two-capacity2', 'two-capacity-max', 'two-capacity2-max', 'two-weight', 'two-free', 'two-count',
        'tree-capacity', 'tree-capacity-max', 'tree-size', 'tree-parent',
        'tree-cycle', 'tree-count', 'removable-capacity', 'removable-capacity-max', 'removable-weight',
        'removable-count', 'kth-capacity', 'kth-capacity-max', 'kth-k', 'kth-count', 'kth-free', 'kth-weight',
        'count-optimal-capacity', 'count-optimal-capacity-max', 'count-optimal-count', 'count-optimal-free', 'count-optimal-weight',
        'fps-modulus', 'fps-free', 'fps-size', 'fps-count', 'fps-weight', 'bnb-capacity', 'bnb-count',
        'pisinger-target', 'pisinger-weight', 'pisinger-width', 'unbounded-capacity', 'unbounded-count',
        'unbounded-weight', 'unbounded-width', 'small-value-capacity',
        'small-value-weight', 'small-value-negative', 'small-value-count',
        'small-value-axis']))
