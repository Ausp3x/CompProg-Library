"""Manacher: direct palindrome interval oracle, exhaustive and large shapes.

Every mode covers odd/even radii, all reconstruction methods and smaller radii,
the predicate constructor (equality, and a complement involution checked on even
radii), centerEnd, longestEnding/longestStarting against per-position maxima of
the enumerated palindromes, empty/gap endpoints, integer alphabets, bytes/NUL,
copy/move and repeated builds. Quick/full/stress enumerate ternary strings
through lengths 6/8/9, then run 300/2000/15000 random cases and
unary/alternating lengths 5000/300000/1000000. Builds use -Wall -Wextra
-Wshadow -Wconversion -Werror. Full/stress add ASan/UBSan to optimized NDEBUG
and checked builds; checked builds execute twelve precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('05-manacher', [
        'odd-negative-center', 'odd-end-center', 'odd-empty',
        'even-negative-center', 'even-large-center', 'odd-zero-radius',
        'odd-negative-radius', 'odd-large-radius', 'even-negative-radius',
        'even-large-radius', 'center-end-negative', 'center-end-high'], strict=True))
