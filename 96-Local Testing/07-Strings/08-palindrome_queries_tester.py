"""Palindrome queries against direct symmetric scans and exhaustive intervals.

Quick/full/stress cover ternary strings through length 5/7/9, 80/500/3000
seeded byte/integer cases, and 5000/200000/1000000-symbol structural cases.
All modes cover exact predicates, both hash variants, leftmost longest witnesses,
empty intervals, all bytes, signed extremes, lifecycle and a known hash false
positive. Full/stress add ASan/UBSan; checked builds run six range probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('08-palindrome_queries', [
        'exact-negative', 'exact-reversed', 'exact-past-end',
        'hash-negative', 'hash-reversed', 'hash-past-end']))
