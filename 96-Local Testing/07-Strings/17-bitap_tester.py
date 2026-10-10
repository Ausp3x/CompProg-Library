"""Bitap: brute window scans, brute Levenshtein over every substring, brute BitsetText.

Every case checks patternMasks (every accepts bit), shiftAnd and shiftOr
(m <= 64), shiftOrMultiword, bitapHamming and bitapEdit for k in
{0, 1, 2, 3, m/2, m-1, m, m+2}, through both the masks and the pattern
overloads. The edit oracle is Sellers' semi-global DP, itself checked against
brute Levenshtein over every substring on small cases. Quick/full/stress
enumerate binary patterns through length 3/4/5 against texts through 5/7/8 and
ternary class patterns (every subset per position) through length 2
against texts through 4/4/5, then 300/1500/6000 random symbol and class
patterns through 200 symbols (one to four words) over alphabets 1..4 and 256
(byte strings too), BitsetText with 100/500/5000 random texts of up to 400
symbols under point updates, range queries, default ranges and foreign
pattern symbols plus a 20000/200000-symbol text, and binary texts of length
50000/100000/500000 with patterns at the word boundaries 1, 63, 64, 65, 130, 1000.
Builds use -Wall -Wextra -Wshadow -Wconversion -Werror. Full/stress add
ASan/UBSan (the checked _GLIBCXX_DEBUG build runs stress at full-mode sizes);
checked builds run nine precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('17-bitap', ['shift-and-long', 'shift-or-long', 'text-symbol', 'pattern-symbol', 'class-symbol',
                                       'negative-k', 'bitset-set', 'bitset-range', 'bitset-negative-r'], strict=True))
