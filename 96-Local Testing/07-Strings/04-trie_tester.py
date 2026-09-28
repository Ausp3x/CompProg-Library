"""Trie: independent map oracle, exhaustive multisets, mutation and byte ordering.

Every mode covers all public operations, empty/NUL/full bytes, cancellation,
exceptions, nested reads, copy/move/reset, slot reuse and INT64_MAX multiplicity.
Quick/full/stress enumerate 27/2187/2187 multisets, perform 1000/7000/40000
seeded random operations and traverse keys of length 2000/100000/300000.
Full/stress add ASan/UBSan to optimized NDEBUG and checked builds. Checked builds
execute five count-domain/overflow assertion probes. All oracles survive NDEBUG.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('04-trie', [
        'insert-zero', 'insert-negative', 'insert-overflow',
        'erase-zero', 'erase-negative']))
