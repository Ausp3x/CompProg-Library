"""Subsequence automaton: brute subsequence sets and an independent reference.

Both variants (sparse SubsequenceAutomaton, dense SubsequenceAutomatonDense)
are checked on every case: next and prev for every state and symbol, step on
every state including the dead state with foreign symbols, the distinct
subsequence counts per state as lng, saturated int and a wrapping ring,
kthDistinctSubsequence for boundary, saturated and random k,
shortestAbsentSubsequence, isSubsequence on random ranges and patterns with
foreign symbols, and countSubsequenceOccurrences as lng, saturated int and
ring. The reference scans the text for next/prev (backward table above 3e7
steps), sums counts over symbols with saturation, solves the absent word by an
O(n * S) DP and counts occurrences by the O(n * m) DP. Small cases add brute
subsequence sets by bitmask: every kth index, per-state counts, the absent
word by enumeration, every pattern against every range [l, r), and
occurrence counts by mask. Quick/full/stress enumerate every string over
alphabets 1..3 through length 6/7/8 (ternary one shorter), then 200/1000/2000
random strings up to 400 symbols over alphabets 1..5 and 256 (byte strings
too), and a ternary text of 20000/200000 symbols (both variants, absent word
validity) plus a 1e6-symbol alphabet and 128-bit saturation on 200
distinct symbols. Builds use -Wall -Wextra -Wshadow
-Wconversion -Werror. Full/stress add ASan/UBSan (the checked _GLIBCXX_DEBUG
build runs stress at full-mode sizes); checked builds run six precondition
probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('20-subsequence_automaton', ['build-symbol', 'next-index', 'prev-symbol', 'range', 'range-negative', 'step-state'], strict=True))
