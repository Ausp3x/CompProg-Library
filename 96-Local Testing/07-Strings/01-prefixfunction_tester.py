"""KMP/prefix suite: direct substring/suffix and alphabet-partition oracles.

Quick/full/stress: binary strings through 5/8/9, all pattern/text pairs through
3/5/6, 100/1200/8000 random pairs, all range-valid prefix arrays through 5/8/9,
and 10000/200000/1000000-symbol adversaries. Every mode checks kmpNext against a
direct strong-border scan, validPrefixFunction witnesses, mixed-signedness
symbol types against exact-value oracles (200/3000/20000 random cases plus the
uint/ulng/char32_t versus int -1 regressions), index-only text sequences, a
Fibonacci-word stream, bytes, wide integer alphabets, accepting/empty states,
reset and copying. KmpMatcher::pushBack grows matchers from the empty pattern and
from half of every tested string (pi, next, reset), streams a grown pattern over
every bounded pair, grows 10000/200000/1000000-symbol near-unary and
2584/75025/317811-symbol Fibonacci patterns, and pushes elements of the
matcher's own vectors after shrink_to_fit (aliasing under reallocation). Builds use -Wall -Wextra -Wshadow -Wconversion -Werror.
Full/stress run optimized NDEBUG, checked assertions and ASan/UBSan; thirteen
assertion probes.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('01-prefixfunction', ['size-large', 'size-huge', 'occurrences-text-large',
        'occurrences-text-huge', 'counts-text-large', 'counts-text-huge', 'border-negative', 'border-large',
        'alphabet-negative', 'symbol-negative', 'symbol-large', 'symbol-far', 'stream-overflow'], extra=('-Wshadow',)))
