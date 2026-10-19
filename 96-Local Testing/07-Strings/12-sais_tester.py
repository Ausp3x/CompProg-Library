"""SA-IS: comparison-sorted suffixes, direct LCP and prefix-doubling differential checks.

Every mode checks sais(vector<int>, upper) at the default threshold, NAIVE=1
(pure SA-IS, all recursion levels) and NAIVE=4, the generic vector<T>
overload with compressAlphabet (int, lng extremes, ulng, signed char, string
symbols), the string/string_view byte overload in unsigned order with NUL and
the full byte alphabet, and lcpArray for every overload.
Quick/full/stress enumerate ternary texts through lengths 7/10/11, binary
texts of lengths 13..16/18/18 (every mask through 16, every 7th above), and
300/3000/15000 random rounds; large unary, period-3, Fibonacci, Thue-Morse,
random and sparse-alphabet texts of 5000/200000/700000 symbols are checked
against the verified prefix-doubling SuffixArray<int>::doubling (suffix order) and binary
lifting over exact names of power-of-two blocks, a hash map keyed by the
pair of half-block names (lcpArray; the lifting oracle is itself checked against
direct LCP on the exhaustive ternary and random integer texts).
Full/stress add ASan/UBSan; checked adds 4 death probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('12-sais', ['negative-upper', 'symbol-above', 'negative-symbol', 'lcp-size'], strict=True))
