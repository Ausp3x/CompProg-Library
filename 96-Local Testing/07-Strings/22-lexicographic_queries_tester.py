"""Lexicographic queries: brute substring enumeration, identities and the suffix automaton.

Every text checks compareSubstrings, longestCommonSuffix and
DictionaryOfBasicFactors (name, equal, compare, lcp) on all range pairs
(texts up to 6 symbols) or 3000 sampled pairs, occurrenceRange and
occurrenceCount on every range, SubstringOrder (kth with and without
multiplicity, both ranks, out-of-range k) against sorted substring sets,
sortSubstrings, frequencyStatistics, lcpSums, minimalAbsentWords (as a set,
leftmost starts, no duplicates), minimalUniqueSubstrings,
shortestUniqueSubstrings, both per-prefix suffix extrema,
distinctSubstringsPerPrefix and distinctSubstringsByLength against brute
enumeration. Quick/full/stress
enumerate binary texts through 7/9/12 and ternary texts through 4/6/7
symbols, then 30/200/800 random texts up to 25/45/60 symbols (Fibonacci,
Thue-Morse, periodic, alphabets 1..4 and 26) and as many random string
collections (lce, compare, comparePieces against concatenations),
20/100/400 lng-symbol and high-byte texts through the string overloads,
medium texts of 300/1000/3000 symbols for the per-prefix extrema, and large
texts of 20000/50000/300000 symbols checked against the suffix automaton
(kth, occurrences, distinct counts per prefix and per length, MUS, SUS,
MAW), distinct windows of sampled lengths and counting identities. Builds use -Wall -Wextra -Wshadow -Wconversion -Werror;
full/stress add ASan/UBSan; the checked and ASan/UBSan builds run stress at
full-mode sizes (the brute oracles allocate heavily and stress there exceeds
the 180 s limit); checked builds run eleven precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('22-lexicographic_queries', ['compare-range', 'suffix-range', 'collection-range', 'collection-index',
                                                       'dbf-name', 'dbf-range', 'occurrence-range', 'no-rmq', 'maw-symbol',
                                                       'maw-sigma', 'order-range'], strict=True))
