"""Enumeration: independent recursive object/Cartesian/reflection oracles.

Every mode covers all twenty-nine APIs, immutable borrowed vectors, cancellation at
first/intermediate/last callbacks, exceptions, nesting, default word types,
8/16/32/64/128-bit words, full-width masks and INT_MAX coordinates/radices.
Quick: vector/mask dimensions through 6, product dimensions through 3 over
radices 0..3, 100 random products and 30 sparse masks/width, dimension 1000.
Full: dimensions 10/5, 1500 products, 300 sparse masks/width, dimension 100000.
Stress: dimensions 12/6, 10000 products, 3000 masks/width, dimension 500000.
Multicombinations cover n,k=0..4/6/8 in quick/full/stress respectively.
Supermasks: exhaustive n<=8 per width plus random dense masks (free-bit oracle).
Submask pairs: double-loop filter n<=6/10/12 per width plus a full-width prefix.
Set bits and k-bit submasks: every mask below 2^8/2^10/2^12 per width with every
k (bit loop, popcount-bucketed filter), random words, sparse masks of up to 20
bits (recursive choice of set bits), full-width all-ones with k=0,1,2,W-1,W,W+1.
Revolving-door combinations n<=10/14/14, k<=n+1, both dispatch branches (Knuth's Gamma recurrence,
removed/added checked against the predecessor), n=100 k=2, the loopless branch at
n=100 k=98..100 (top-rows oracle) with a stop after 1000 outputs, and INT_MAX prefixes.
Integer partitions n<=12/22/28 under seven part caps (first-part recursion and
coin-change p(n)); set partitions n<=7/10/11, all and with each k=0..n+1 blocks
(block assignment filtered by block count, Bell and Stirling triangles), Stirling
counts with lexicographic restricted-growth checks for n=20 k=0,1,2,18..21 and
n=60 k=1,59,60, large-dimension k=1,2,n-1,n prefixes;
pairings n<=10/12/13 (smallest-free recursion, double factorials); labelled
trees n<=6/6/7 (connected (n-1)-edge subsets of K_n, naive Pruefer removal, an
ordered-set decoder for the large-dimension prefix);
rank/unrank exhaustive n<=9/13/15 against recursive combinations and Gray
rank/unrank against Knuth's Gamma lists, plus 2000/50000/300000 random round
trips (n<=67 and n up to INT_MAX with k<=2) with the Pascal-sum and Gamma
recurrence rank oracles, k<=2 closed forms, one revolving-door exchange between
consecutive Gray ranks, and first/last ranks at the 2^64 boundary (C(67,33),
C(64,32)), k=0, k=1, k=n-1 and k=n. Cool-lex ranks: every combination of
n<=9/13/15 against its position in the rotation-oracle list (and a Theorem 2
Pascal oracle and k<=2 closed form checked there), the random lexicographic and
Gray combinations against that Pascal oracle (n<=67) or the closed form (k<=2,
n up to INT_MAX), and the first/last strings at the same boundaries.
Compositions and weak compositions n,k=0..6/9/10 (increasing-part recursion,
C(n-1,k-1) and C(n+k-1,k-1)), k=100000 with n=0, n=k, n=k-1 and a weak n=1
prefix, empty k=INT_MAX without allocation, INT_MAX prefixes for k=2,3.
Exact-part partitions n<=14/24/30 with m=0..n+1 and m=INT_MAX (nonincreasing
recursion sorted colexicographically, p(n,m) triangle summed against p(n)),
m=100000 with n=m,m+2, INT_MAX prefixes for m=2,3. Cool-lex masks
n<=10/14/16 per width and cool-lex vectors n<=10/14/16, k=0..n+1 and k=INT_MAX
(the paper's prefix-rotation rule on a bitstring, itself checked against the
ascending k-bit masks or the recursive combinations), vectors also at n=60/200/200 with
k=0,1,2,n-2..n+1 (C(n,k), first/last strings, set against the combinations
or their complements), and outside quick streamed at n=18/20 for every k
against the rotation rule applied in step, with each rank equal to its
position; masks at full width with k=0..4,W-4..W+1: the whole order up to 5000/50000/50000
outputs (sorted against the k-subsets of all bits), else a 2000/20000/20000
output prefix plus, outside quick and up to 10^6 outputs, count, popcount,
distinctness and last mask. The shared runner covers optimized NDEBUG and checked builds;
full/stress add ASan/UBSan with leak checking. Checked builds run 66 probes.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main


if __name__ == '__main__':
    raise SystemExit(main('012-enumeration', [
        'combination-negative-n', 'combination-negative-k',
        'multicombination-negative-n', 'multicombination-negative-k',
        'subset-negative-n', 'product-negative', 'product-zero-negative',
        'gray-product-negative', 'gray-product-zero-negative',
        'subset-mask-negative', 'subset-mask-large', 'combination-mask-negative-n',
        'combination-mask-large-n', 'combination-mask-negative-k',
        'gray-mask-negative', 'gray-mask-large',
        'supermask-outside', 'supermask-outside-stop', 'supermask-large', 'partition-negative', 'partition-negative-capped', 'partition-zero-part',
        'set-partition-negative', 'combination-submask-negative-k', 'submask-pair-negative',
        'submask-pair-large', 'labeled-tree-zero', 'gray-combination-negative-n',
        'gray-combination-negative-k', 'pairing-negative', 'pairing-int-max', 'rank-overflow', 'rank-unsorted', 'rank-range', 'rank-k-large',
        'unrank-k-large', 'unrank-rank-large', 'unrank-overflow', 'gray-rank-overflow',
        'gray-rank-unsorted', 'gray-rank-range', 'gray-rank-above', 'gray-rank-negative', 'gray-rank-k-large', 'gray-unrank-rank-large',
        'gray-unrank-k-large', 'gray-unrank-negative-k', 'gray-unrank-overflow', 'set-partition-k-negative-n',
        'set-partition-k-negative-k', 'composition-negative-n', 'composition-negative-k',
        'weak-composition-negative-n', 'weak-composition-negative-k', 'partition-parts-negative-n',
        'partition-parts-negative-m', 'cool-lex-negative-k', 'cool-lex-negative-n', 'cool-lex-large-n',
        'cool-lex-vector-negative-n', 'cool-lex-vector-negative-k', 'cool-rank-unsorted', 'cool-rank-range',
        'cool-rank-negative', 'cool-rank-k-large', 'cool-rank-overflow']))
