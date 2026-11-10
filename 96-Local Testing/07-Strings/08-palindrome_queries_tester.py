"""Palindrome queries against direct symmetric scans and exhaustive intervals.

Quick/full/stress cover ternary strings through length 5/7/9, 80/500/3000
seeded byte/integer cases, and 5000/200000/1000000-symbol structural cases.
All modes cover exact predicates, all three hash kinds, leftmost longest witnesses,
palindromic substring counts in total and by start/last index,
empty intervals, all bytes, signed extremes, lifecycle and a known hash false
positive. Full/stress add ASan/UBSan; checked builds run six range probes.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main


if __name__ == '__main__':
    raise SystemExit(main('08-palindrome_queries', [
        'exact-negative', 'exact-reversed', 'exact-past-end',
        'hash-negative', 'hash-reversed', 'hash-past-end'], extra=('-Wshadow',)))
