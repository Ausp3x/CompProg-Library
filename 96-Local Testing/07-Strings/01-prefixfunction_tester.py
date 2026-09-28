"""KMP/prefix suite: direct substring/suffix and alphabet-partition oracles.

Quick/full/stress: binary strings through 5/8/9, all pattern/text pairs through
3/5/6, 100/1200/8000 random pairs, all range-valid prefix arrays through 5/8/9,
and 10000/200000/1000000-symbol adversaries. All modes cover every public API,
bytes, wide integer alphabets, accepting/empty states, reset and copying.
Full/stress run optimized NDEBUG, checked assertions and ASan/UBSan.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('01-prefixfunction', ['size-large', 'border-negative', 'border-large',
        'alphabet-negative', 'symbol-negative', 'symbol-large', 'stream-overflow']))
