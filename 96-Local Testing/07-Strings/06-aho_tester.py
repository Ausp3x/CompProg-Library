"""Aho-Corasick independent dictionary/substring verification.

Quick/full/stress enumerate every subset of binary patterns through length 1/2/2,
all binary texts through length 3/4/5, ordered duplicate dictionaries, and
100/1200/6000 seeded random dictionaries. All modes cover sparse/dense builds,
all public queries, callbacks, empty/duplicate IDs, full bytes, lifecycle and
asserted preconditions. Full/stress add ASan/UBSan and larger suffix-heavy counts.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-aho', ['unbuilt', 'add-built', 'negative-state',
        'large-state', 'aggregate-size', 'safe-state', 'unbuilt-sink']))
