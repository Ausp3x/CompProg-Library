"""Suffix arrays: independent suffix sorting, direct LCP and substring witnesses.

SA-IS construction and the static prefix-doubling builder are both checked
against sorted suffixes and direct LCP/LCE. Every mode covers bytes/NUL,
signed/unsigned 64- and 128-bit extremes, both sides of the dense-code boundary
(max - min < max(4n, 256)), direct build, optional RMQ, pattern ranges (vector, string_view and brace lists starting with 0), LCE/substrings,
longest repeated (every k for n <= 24) and disjoint repeats, common substrings,
ties and object lifecycle.
Quick/full/stress enumerate ternary texts through lengths 5/7/8, all binary
text pairs through lengths 3/5/6, and 150/1500/7000 seeded random cases.
Large unary/periodic (bytes, doubling, sparse lng) inputs have 5000/200000/700000
symbols. Full/stress add
ASan/UBSan to optimized NDEBUG and checked builds; checked adds 10 death probes.
Builds use -Wall -Wextra -Wshadow -Wconversion -Werror, including T=bool.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('07-suffixarray', [
        'negative-lce', 'large-lce', 'negative-second', 'large-second',
        'missing-rmq', 'negative-substring', 'reversed-substring',
        'large-substring', 'reversed-second', 'repeat-count'], strict=True))
