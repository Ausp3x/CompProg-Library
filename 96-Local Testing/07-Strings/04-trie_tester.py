"""Trie and TrieDense: independent map oracle, exhaustive multisets, mutation and byte ordering.

Map Trie, TrieDense<26,'a'>, TrieDense<2,'a'>, TrieDense<256,0> and TrieDense<4,'a'>
share one generic suite. Every audit checks size/empty/count/countPrefix/search,
findNode and step walks against node counters, forEachPrefixOf/longestPrefix
against every stored prefix of the query, and ordered forEach enumeration.
Every mode covers empty/NUL/full bytes, out-of-alphabet dense queries,
cancellation, exceptions, nested reads, copy/move/reset, newNode slot reuse and
INT64_MAX multiplicity. Quick/full/stress enumerate 27/2187/2187 multisets per
alphabet variant, perform 1000/7000/40000 seeded random operations per variant and
traverse keys of length 2000/100000/300000 (one tenth for dense 256-way nodes).
Full/stress add ASan/UBSan to optimized NDEBUG and checked builds. Checked builds
execute eight count-domain/overflow/dense-alphabet assertion probes. All oracles
survive NDEBUG.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('04-trie', [
        'insert-zero', 'insert-negative', 'insert-overflow',
        'erase-zero', 'erase-negative', 'dense-symbol-high', 'dense-symbol-low', 'dense-overflow'], strict=True))
