"""K-best enumeration verified against sorted brute-force enumeration under less and greater.

All modes run every operation under std::less and std::greater (kLargest), and a
quarter of the seeded rounds under stateful comparators (s * x < s * y, s = +-1): pair and
subarray sums against all pairs/ranges sorted by (sum, index) with exact order,
subset, fixed-size subset and sorted-list sums against all 2^n subsets or choice
vectors with witnesses checked valid, sum-consistent and pairwise distinct, and
lawlerKBest against every feasible 0/1 vector of a nonlinear objective with a
brute-force constrained oracle (half the cases returning a random optimum, so a
re-solve may return another vector of equal value) and an oracle-call bound. Quick/full/stress exhaust
the alphabet {-2,0,1,1,3} through n=3/4/5 with every k (n=5: k <= 6 and 31..33), m and L <= R, add
150/2000/12000 seeded rounds (n <= 10, Lawler n <= 6, values up to 1e12), int/double
element types, and large n=k=2000/100000/300000 fixtures with a two-pointer
certificate for pair sums and order/witness checks for the others; the -O0
debug-container build caps the large fixtures at n=1000 and runs stress as full
(its heaps validate the whole range per operation), as does the ASan build (the
runner's 180 s limit); 16 probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('54-kbest_enumeration', [
        'pair-k', 'subarray-k', 'subarray-low', 'subarray-order', 'subset-k',
        'subset-witness', 'subset-witness-negative', 'fixed-m', 'fixed-k',
        'lists-k', 'lists-choice', 'lists-choice-negative', 'lawler-n', 'lawler-k',
        'lawler-resolve', 'lawler-value']))
