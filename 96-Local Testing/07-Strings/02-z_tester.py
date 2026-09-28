"""Z/ex-KMP/conversion suite with direct LCP and alphabet-partition oracles.

Quick/full/stress: binary strings through 5/8/9, all pattern/text pairs through
3/5/6, 100/1200/8000 random pairs, every range-valid Z array through 5/8/9 and
10000/200000/1000000-symbol adversaries. All modes cover all APIs, bytes, wide
integer symbols, invalid/empty arrays and successful/failed aliased conversions.
Full/stress run optimized NDEBUG, checked assertions and ASan/UBSan.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-z', ['length', 'pattern-length', 'text-length']))
