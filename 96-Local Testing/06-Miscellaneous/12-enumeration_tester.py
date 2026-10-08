"""Enumeration: independent recursive object/Cartesian/reflection oracles.

Every mode covers all fourteen APIs, immutable borrowed vectors, cancellation at
first/intermediate/last callbacks, exceptions, nesting, default word types,
8/16/32/64/128-bit words, full-width masks and INT_MAX coordinates/radices.
Quick: vector/mask dimensions through 6, product dimensions through 3 over
radices 0..3, 100 random products and 30 sparse masks/width, dimension 1000.
Full: dimensions 10/5, 1500 products, 300 sparse masks/width, dimension 100000.
Stress: dimensions 12/6, 10000 products, 3000 masks/width, dimension 500000.
Multicombinations cover n,k=0..4/6/8 in quick/full/stress respectively.
Supermasks: exhaustive n<=8 per width plus random dense masks (free-bit oracle).
Integer partitions n<=12/22/28 under seven part caps (first-part recursion and
coin-change p(n)); set partitions n<=7/10/11 (block assignment, Bell triangle);
rank/unrank exhaustive n<=9/13/15 against recursive combinations, plus
2000/50000/300000 random round trips with Pascal, k<=2 closed forms and 2^64
boundaries. The shared runner covers optimized NDEBUG and checked builds;
full/stress add ASan/UBSan with leak checking. Checked builds run 28 probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('12-enumeration', [
        'combination-negative-n', 'combination-negative-k',
        'multicombination-negative-n', 'multicombination-negative-k',
        'subset-negative-n', 'product-negative', 'product-zero-negative',
        'gray-product-negative', 'gray-product-zero-negative',
        'subset-mask-negative', 'subset-mask-large', 'combination-mask-negative-n',
        'combination-mask-large-n', 'combination-mask-negative-k',
        'gray-mask-negative', 'gray-mask-large',
        'supermask-outside', 'supermask-large', 'partition-negative', 'partition-zero-part',
        'set-partition-negative', 'rank-overflow', 'rank-unsorted', 'rank-range', 'rank-k-large',
        'unrank-k-large', 'unrank-rank-large', 'unrank-overflow']))
