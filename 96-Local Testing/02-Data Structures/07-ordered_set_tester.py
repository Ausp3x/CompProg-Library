#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    # -D_GLIBCXX_ASSERTIONS: its PBDS debug_map_base default-constructs the diagnostic comparator, which disagrees with valid stateful comparators, and validates whole trees on every operation.
    raise SystemExit(main('07-ordered_set', ["id-exhaustion","rebuild-id-exhaustion","decode-end","decode-past-end","decode-negative","range-order","split-self","join-self","join-self-empty","join-overlap","join-equal-above","join-equal-below","offline-unknown-insert","offline-kth-end","offline-kth-negative","offline-size-limit","sorted-max-size","multiset-rebuild-max-size"], checked='-D_GLIBCXX_ASSERTIONS'))