"""Fractional programming verified against exhaustive subset enumeration with exact ratios.

All modes cover dinkelbach over explicit (N, D) families with a brute-force argmax
oracle (worst, optimal and random feasible starts and the infeasible -INT64_MAX / 1,
minimization by negation, families with operands within 1 of INT64_MAX),
zeroOneFractionalSelect (two seeds per case) and ratioKnapsack against every subset
with exact cross-multiplied ratio comparison, witness sums, sizes, order and the den=0
infeasible sentinel, values near the 2^62 domain bound, ratioKnapsack weights near
and above INT_MAX, and 16 asserted-precondition probes. Quick/full/stress exhaust
a in {-2,0,1,3}, b in {1,2,3} through n=3/4/5 with every k and W <= 3n+1, add
200/3000/20000 seeded rounds (n <= 12, values up to 1e12), run 150/9000/45000 tangent
families on which Dinkelbach is slow, so the Eppstein-Hirschberg rounds run (n <= 14
by brute force for every k, n <= 98 and n <= 1204 by certificate and seed agreement),
add fixtures with both sums at 2^62 - 1, and certify
large instances (zeroOne n=2000/200000/1000000 over random, duplicate, concurrent and
tangent families, ratioKnapsack n=60/250) by the zero-optimum certificate computed
independently. The debug-container and ASan builds run stress as full.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('55-fractional_programming', [
        'dinkelbach-denominator', 'dinkelbach-oracle-denominator', 'dinkelbach-start', 'dinkelbach-calls',
        'select-k-zero', 'select-k-large', 'select-size', 'select-weight', 'select-sum', 'select-weight-sum',
        'ratio-bound', 'ratio-bound-max', 'ratio-size', 'ratio-weight', 'ratio-sum', 'ratio-weight-sum']))
