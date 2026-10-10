"""Burrows-Wheeler transforms: brute rotation matrices and brute Lyndon/omega sorting.

Every case checks sortCyclicRotations against stably sorted materialized
rotations; bwt and bwtFromSuffixArray against the sorted rotations of s plus a
sentinel keyed below every symbol (its value is arbitrary, sometimes the
largest byte); inverseBwt; psiMapping and lfMapping (sentinel and rotation
forms) against a brute stable sort, LF as the inverse of psi and LF stepping a
row to the rotation one position earlier; bwtPrimary (last column and the
smallest row holding s) and inverseBwtPrimary; bijectiveBwt against brute
Lyndon factors (rotation comparisons) with conjugates sorted by omega order,
inverseBijectiveBwt, and bijectiveBwt(inverseBijectiveBwt(s)) = s.
Quick/full/stress enumerate every string over alphabets 1..3 through length
7/9/10 (ternary two shorter), then 300/3000/6000 random strings up to 40
symbols (a third over 255 bytes, a quarter periodic) each as bytes, shifted
bytes with the 0xFF sentinel, extreme lng and signed int, and random,
period-5, Fibonacci and unary texts of length 30000/200000/500000 (round trips,
SuffixArray-based BWT, rotation order versus SuffixArray of s + s). Builds use
-Wall -Wextra -Wshadow -Wconversion -Werror. Full/stress add ASan/UBSan (the
checked _GLIBCXX_DEBUG build runs stress at full-mode sizes); checked builds
run five precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('29-bwt', ['sentinel-in-text', 'no-sentinel', 'two-sentinels', 'primary-range', 'suffix-array-size'], strict=True))
