"""Z suite: direct LCP oracles, exhaustive validation and conversion.

Quick/full/stress: binary strings through 5/8/9, all pattern/text pairs through
3/5/6, 100/1200/8000 random pairs over signed symbols, every range-valid Z array
through 5/8/9, and 10000/200000/1000000-symbol adversaries. Every mode covers
mixed-signedness symbol types against exact-value oracles (200/3000/20000 random
cases), index-only sequences, bytes/NUL, wide integers, aliasing and clearing.
Builds use -Wall -Wextra -Wshadow -Wconversion -Werror. Full/stress run
optimized NDEBUG, checked assertions and ASan/UBSan; three assertion probes.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-z', ['length', 'pattern-length', 'text-length'], strict=True))
