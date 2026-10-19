"""Offline suffix tree: brute-force compacted suffix trie and suffix-array differential checks.

Every mode checks, against the explicit nodes (root, suffixes, right-branching
substrings) enumerated directly: node count, pre-order ids, Node l/r/depth/
parent/end, children, child, edgeLabel, leafSuffix, leaf, occurrences, the
locus of every substring (locate, isSubstring, step with present and absent
symbols), lca of every node pair (brute ancestor walk, validated against the
common prefix of the node strings), weightedAncestor and one weightedAncestors
batch for every node and length and for every s[i, j), suffixLink, maximal
repeats by maximal pairs (Gusfield), supermaximal repeats by containment,
distinctSubstrings, and agreement of the vector, string, SuffixArray and
rebuild construction paths. Quick/full/stress enumerate ternary texts through
lengths 6/7/8 and binary through 9/11/12, run 80/800/1600 random int/bool/lng/
signed char/byte rounds and 2/16/30 medium Fibonacci, periodic and ternary
texts (60-140 symbols, 60-200 in stress); large unary/random/Fibonacci/
periodic/Thue-Morse texts of 3000/200000/700000 symbols are checked against the
verified SuffixArray with RMQ (interval LCEs, parent depths, pattern ranges,
loci, steps, every suffix link, lca of random and rank-adjacent pairs, a
weightedAncestors batch of min(n, 200000) queries, occurrences, distinct count)
and by the reversal symmetry of maximal repeats. Every mode first checks
hand-derived repeats and counts (banana, mississippi, abab, xabxac, aaaa, empty).
Full/stress add ASan/UBSan; checked adds 18 death probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('11-suffixtree', [
        'edge-root', 'link-root', 'ancestor-deep', 'ancestor-negative', 'ancestor-node',
        'step-root-length', 'step-noncanonical', 'step-deep', 'leaf-range', 'occurrences-range',
        'depth-range', 'children-range', 'child-range', 'lca-range', 'lca-negative',
        'ancestors-deep', 'ancestors-negative', 'ancestors-node'], strict=True))
