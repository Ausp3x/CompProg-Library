"""Suffix arrays: independent suffix sorting, direct LCP and substring witnesses.

Every mode covers bytes/NUL, signed/unsigned extremes, optional RMQ, pattern
ranges, LCE/substrings, repeated/common substrings, ties and object lifecycle.
Quick/full/stress enumerate ternary texts through lengths 5/7/8, all binary
text pairs through lengths 3/5/6, and 150/1500/7000 seeded random cases.
Large unary/periodic inputs have 5000/200000/700000 symbols. Full/stress add
ASan/UBSan to optimized NDEBUG and checked builds; checked adds 10 death probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('07-suffixarray', [
        'negative-lce', 'large-lce', 'negative-second', 'large-second',
        'missing-rmq', 'negative-substring', 'reversed-substring',
        'large-substring', 'reversed-second', 'byte-domain']))
