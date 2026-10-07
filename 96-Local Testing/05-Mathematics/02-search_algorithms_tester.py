"""Search against exhaustive monotone arrays, brute-force minima and analytic roots.

Quick exhausts small endpoints and unimodal arrays through length 6, then 100
random cases. Full covers length 8 and 3000 random cases; stress covers length 9
and 30000 cases. All modes test full signed endpoints and finite-double extremes,
subnormals, adjacent points, precision/call bounds, and every public API. Full
and stress use optimized, checked and ASan/UBSan builds.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-search_algorithms', [
        'first-order', 'last-order', 'integer-order', 'fib-order', 'binary-iterations',
        'ternary-iterations', 'golden-iterations', 'binary-infinite',
        'ternary-nan', 'golden-infinite', 'ternary-order', 'golden-order',
        'negative-absolute', 'negative-relative', 'infinite-tolerance', 'nan-tolerance']))
