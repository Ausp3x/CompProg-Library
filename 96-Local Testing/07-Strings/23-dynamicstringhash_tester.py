"""Dynamic string hashes: an independent polynomial-hash oracle over mirrored sequences.

For KIND 0, 1 and 2 (default and random bases), every step compares
PointUpdateHash (set, get, reverseGet, lcp, lcs, compare), SequenceHash
(insert, erase, set, reverse, split/merge rotations, range assignment for
KIND != 1, at, size, values, get, reverseGet, lcp, lcs, compare) and DequeHash
(pushFront, pushBack, popFront, popBack, get, size) with a brute hash of a
mirrored vector or deque and brute prefix/suffix scans; HashMonoid is checked
for identity, associativity, reversal, mapping and composition and through
LazySegmentTree range assignment. Quick/full/stress run 30/200/800,
20/120/500 and 20/150/600 random rounds (alphabets 1..3 and full symbol
ranges, empty sequences, high bytes through the string overloads),
100/1000/4000 monoid rounds, and periodic sequences of 20000/100000/300000
symbols with 2000/20000/20000 long-prefix queries and point edits. Builds use
-Wall -Wextra -Wshadow -Wconversion -Werror (-Wshadow is suppressed only
around 06-Miscellaneous/02-customhash.hpp and 02-Data Structures/19-treap.hpp);
full/stress add ASan/UBSan; checked builds run ten precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('23-dynamicstringhash', ['point-set', 'point-range', 'point-symbol', 'point-lcp', 'base-small',
                                                   'base-even', 'deque-pop', 'deque-range', 'sequence-range',
                                                   'sequence-symbol'], strict=True))
