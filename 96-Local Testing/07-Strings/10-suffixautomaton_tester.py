"""Suffix automaton: direct substring enumeration and suffix-array differential checks.

Every mode checks online extend against per-prefix substring sets, every
distinct substring's state (endpos classes, len/link, minimalLength,
endposSize), counts, first/last/all occurrences, kth distinct and kth with
multiplicity against sorted (multi)sets, lexicographicWalk order and early
stop, matching statistics, longest common substring, rotation occurrences,
shortest absent string by length-ordered enumeration, the suffix-link tree and
distinctSubstringsOnline, for dense (S=2, 3, 26) and map (all bytes) variants.
Quick/full/stress enumerate ternary texts through lengths 5/7/8 and run
60/600/3000 random binary/ternary/byte rounds; large unary/random/Fibonacci
texts of 3000/200000/700000 symbols are checked against SuffixArray (distinct
count, total length, kth distinct, occurrence counts and positions).
Full/stress add ASan/UBSan; checked adds 10 death probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('10-suffixautomaton', [
        'dense-domain', 'unbuilt-count', 'unbuilt-last', 'unbuilt-occurrences', 'unbuilt-endpos',
        'unbuilt-kth', 'unbuilt-kth-multi', 'unbuilt-cyclic', 'unbuilt-absent', 'stale-after-extend'], strict=True))
