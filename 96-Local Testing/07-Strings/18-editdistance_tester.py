"""Edit distance and alignment: alignment enumeration, string BFS and recursion oracles.

Small pairs are checked against every alignment (Levenshtein, banded for
every w, weighted, Needleman-Wunsch, semi-global, Gotoh), BFS over strings
with insert/delete/substitute (validating the enumerator) and with adjacent
transpositions (unrestricted Damerau-Levenshtein), the suffix-form OSA
recursion and an all-substring-pairs Smith-Waterman scan; witnesses are
validated column by column. string, string_view, vector<int>, vector<char>
and vector<lng> inputs are covered.
Quick/full/stress: exhaustive ternary pairs through lengths 3/4/4, 150/1500/2000
random small pairs with random scoring (the insert/delete/substitute BFS on every
fourth), 40/300/450 medium pairs (multiword boundaries 63..193, wide integer
symbols) against a prefix-table DP, and large random/near-equal/unary/
unbalanced/skewed pairs of 600/3000/5000 symbols.
Full/stress add ASan/UBSan; checked adds 5 death probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('18-editdistance', ['band-negative', 'threshold-negative', 'weighted-negative', 'semi-positive-gap', 'local-positive-gap'], strict=True))
