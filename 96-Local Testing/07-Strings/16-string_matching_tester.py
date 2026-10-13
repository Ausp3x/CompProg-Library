"""Exact single-pattern matching: brute-force window scan, brute tables.

Every mode checks naiveSearch, rabinKarp (default and per-case seeds), twoWay,
firstOccurrence, isSubstring and countOccurrences (overlapping and greedy
disjoint) on every case, boyerMoore at the default S = 256 on every byte case,
and boyerMoore, horspool, sunday, both dfaSearch overloads,
badCharacterTable (three prefix lengths) and
goodSuffixTable (brute strong good-suffix definition and the period) on every
coded case. Quick/full/stress enumerate binary patterns through length 4/5/6
against texts through 7/9/10 and ternary patterns through 3/3/4 against
texts through 5/7/6 (alphabet bound S = 100), then 2000/8000/20000 random cases (a fifth over 256
bytes, a third periodic, patterns mostly substrings) each also run as int,
extreme-lng (generic algorithms) and 5-symbol vectors, Rabin-Karp collision
cases (symbols equal modulo 2^61 - 1), and unary hit/miss, period-3 and
random binary texts of length 100000/1000000 with patterns of length
2000/20000 against kmpOccurrences (the unary hit is quadratic for Boyer-Moore
without the Galil rule; verified Rabin-Karp only where hits * m <= 1e8). Builds use -Wall -Wextra -Wshadow -Wconversion
-Werror. Full/stress add ASan/UBSan (the checked _GLIBCXX_DEBUG build runs
stress at full-mode sizes); checked builds run five precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('16-string_matching', ['boyer-moore-symbol', 'horspool-text-symbol', 'bad-character-length',
                                                 'dfa-text-symbol', 'dfa-pattern-symbol'], strict=True))
