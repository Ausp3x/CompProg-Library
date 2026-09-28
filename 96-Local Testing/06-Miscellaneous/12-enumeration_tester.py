"""Enumeration: independent recursive object/Cartesian/reflection oracles.

Every mode covers all nine APIs, immutable borrowed vectors, cancellation at
first/intermediate/last callbacks, exceptions, nesting, default word types,
8/16/32/64/128-bit words, full-width masks and INT_MAX coordinates/radices.
Quick: vector/mask dimensions through 6, product dimensions through 3 over
radices 0..3, 100 random products and 30 sparse masks/width, dimension 1000.
Full: dimensions 10/5, 1500 products, 300 sparse masks/width, dimension 100000.
Stress: dimensions 12/6, 10000 products, 3000 masks/width, dimension 500000.
Multicombinations cover n,k=0..4/6/8 in quick/full/stress respectively.
The shared runner covers optimized NDEBUG and checked builds; full/stress add
ASan/UBSan with leak checking. Checked builds execute 16 precondition probes.
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
        'gray-mask-negative', 'gray-mask-large']))
