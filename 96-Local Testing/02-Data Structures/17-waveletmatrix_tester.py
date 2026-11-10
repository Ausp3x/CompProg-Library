#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    # -D_GLIBCXX_ASSERTIONS: debug lower_bound checks partitioning in O(n) per call and every query binary-searches its alphabet.
    raise SystemExit(main("17-waveletmatrix", ["wavelet-access-range", "wavelet-access-negative", "wavelet-reversed", "wavelet-end", "wavelet-countrange-end", "wavelet-kth-range", "wavelet-kth-empty", "wavelet-kthmax-negative", "wavelet-maxleq-range", "wavelet-mingeq-range", "wavelet-select-negative", "wavelet-rank-range", "wavelet-sum-unweighted", "wavelet-kthsum-unweighted", "wavelet-kthsum-range", "wavelet-descend-end", "wavelet-descend-rank", "wavelet-descend-negative", "wavelet-weights-size", "rectangle-size", "rectangle-sum-unweighted", "monoid-size", "monoid-set-range", "monoid-apply-negative", "monoid-get-range"], checked='-D_GLIBCXX_ASSERTIONS'))