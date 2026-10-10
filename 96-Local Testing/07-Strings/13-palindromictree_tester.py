"""Palindromic tree: brute-force palindrome enumeration and O(n^2) partition DPs.

Every case checks the node set against all distinct palindromic substrings
(first occurrence, len, link, parent, diff, series length and node), step for
every node and symbol (plus an absent one), longestSuffixPalindrome/suffix,
substringSuffixPalindrome for every [l, r), occurrencesAt, occurrenceCounts on
the whole storage, per string and an arbitrary range, palindromicLength,
evenPalindromePartition, palindromicFactorization (count of all partitions and
saturating even minimum) and the minPalindromePartition witness (longest last
piece) against a direct palindrome-table DP, for a single tree, a joint tree
(newString) and build() reuse; the dense tree is compared field for field.
Quick/full/stress enumerate ternary strings through lengths 6/8/9, run 100/800/
5000 random strings (forced palindromes, INT_MIN/INT_MAX symbols), a byte string
with NUL and high bytes, medium random/Thue-Morse strings of length 400/2000/
4000 against O(n^2) oracles, and unary (closed forms, modular even partitions)
and Fibonacci words of length 50000/500000/1000000 (structural invariants).
Builds use -Wall -Wextra -Wshadow -Wconversion -Werror. Full/stress add
ASan/UBSan (the checked _GLIBCXX_DEBUG build runs stress at full-mode sizes);
checked builds run ten precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('13-palindromictree', [
        'palindrome-root', 'palindrome-high', 'suffix-negative', 'suffix-high',
        'substring-reversed', 'substring-high', 'counts-high', 'counts-reversed',
        'dense-below', 'dense-above'], strict=True))
