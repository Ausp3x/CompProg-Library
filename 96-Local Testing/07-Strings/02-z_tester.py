"""Z suite: direct LCP oracles, exhaustive validation and conversion.

Quick/full/stress: binary strings through 5/8/9, all pattern/text pairs through
3/5/6, 100/1200/8000 random pairs over signed symbols, every range-valid Z array
through 5/8/9, and 10000/200000/1000000-symbol adversaries. Every mode covers
mixed-signedness symbol types against exact-value oracles (200/3000/20000 random
cases), index-only sequences, bytes/NUL, wide integers, aliasing and clearing.

DistinctSubstringsDeque: every binary push/pop sequence through 6/8/9 operations
against a substring set (which validates the sorted-suffix LCP oracle), 20/150/800
random sequences of up to 200 operations per symbol type (signed int, lng with
LLONG_MIN/LLONG_MAX, an equality-only type) over alphabets of 1-4 symbols, the
full byte alphabet with NUL and negative chars drained to empty from both ends,
copy independence, and 1500/10000/30000-symbol unary, near-unary and alternating
sequences against closed forms. State and count are checked after every operation.
Symbol comparisons of zFunction and extendedZ are counted on unary, near-unary,
periodic, Fibonacci and random texts of 2000/20000/100000 symbols against the
linear bounds 2n and 2(n + m).
Builds use -Wall -Wextra -Wshadow -Wconversion -Werror. Full/stress run
optimized NDEBUG, checked assertions and ASan/UBSan; seven assertion probes.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-z', ['length', 'pattern-length', 'text-length', 'length-huge', 'text-length-huge', 'deque-pop-back-empty', 'deque-pop-front-empty'], extra=('-Wshadow',)))
