"""Aho-Corasick independent dictionary/substring verification.

Quick/full/stress enumerate every subset of binary patterns through length 1/2/2,
all binary texts through length 3/4/5, ordered duplicate dictionaries,
100/1200/6000 seeded random dictionaries and 60/600/3000 random external tries
(fromTrie, shuffled node ids). All modes cover sparse/dense builds, every public
query including sparseStep, callbacks, empty/duplicate IDs, full bytes, lifecycle
and 15 asserted preconditions. Large cases: nested unary patterns (depth
100/1000/2500, text 10000/150000/600000) and a caterpillar trie (spine
2000/50000/200000) plus an adversarial trie whose leaf depths force the
temporary dense link computation (spines 500/5000/20000, small cases m = 3, 20).
Full/stress add ASan/UBSan.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-aho', ['unbuilt', 'unbuilt-sparse', 'unbuilt-sink', 'trie-duplicate-label',
        'trie-unreachable', 'trie-parent-range', 'trie-end-range', 'trie-label-size', 'add-built', 'negative-state',
        'large-state', 'large-sparse-state', 'aggregate-size', 'safe-state', 'output-state'], strict=True))
