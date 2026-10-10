"""Minimal rotation: brute force over every materialized rotation.

Every mode checks minRotation, maxRotation, minRotationIndices (first start and
gap), canonicalRotation, minRotation over a SuffixArray of s + s, and
rotationOffset/rotationEquivalent against every rotation, a modified string and
a shorter string. Quick/full/stress enumerate ternary strings through lengths
7/9/10, then 300/3000/20000 random byte strings (some periodic) each also run as
extreme-lng and signed-int vectors, the 256 bytes descending, and unary,
period-3 and random binary strings of length 20000/300000/1000000 (random
compared with the suffix-array variant). Builds use -Wall -Wextra -Wshadow
-Wconversion -Werror. Full/stress add ASan/UBSan (the checked _GLIBCXX_DEBUG build runs stress at full-mode sizes); checked builds run two
precondition probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('15-minrotation', ['suffix-array-odd', 'suffix-array-halves'], strict=True))
