"""KMP/prefix suite: direct substring/suffix and alphabet-partition oracles.

Quick/full/stress: binary strings through 5/8/9, all pattern/text pairs through
3/5/6, 100/1200/8000 random pairs, all range-valid prefix arrays through 5/8/9,
and 10000/200000/1000000-symbol adversaries. Every mode checks kmpNext against a
direct strong-border scan, validPrefixFunction witnesses, mixed-signedness
symbol types against exact-value oracles (200/3000/20000 random cases plus the
uint/ulng/char32_t versus int -1 regressions), index-only text sequences, a
Fibonacci-word stream, bytes, wide integer alphabets, accepting/empty states,
reset and copying. Builds use -Wall -Wextra -Wshadow -Wconversion -Werror.
Full/stress run optimized NDEBUG, checked assertions and ASan/UBSan.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('01-prefixfunction', ['size-large', 'border-negative', 'border-large',
        'alphabet-negative', 'symbol-negative', 'symbol-large', 'stream-overflow'], strict=True))
