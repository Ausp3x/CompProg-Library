#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('08-monotone_stack', ["minimum-zero-window","minimum-negative-window","maximum-zero-window","negative-height","late-negative-height","binary-value","binary-negative-cell","binary-large-cell","binary-short-row","binary-long-row","legacy-ragged","legacy-short-row",
        "maximal-histogram-negative","maximal-histogram-late-negative","maximal-binary-value","maximal-binary-cell","maximal-binary-ragged",
        "square-value","square-cell","square-ragged","count-squares-value","count-squares-cell","count-squares-ragged",
        "count-rectangles-value","count-rectangles-cell","count-rectangles-ragged","total-value","total-cell","total-ragged",
        "binary-value-negative","maximal-binary-value-negative","square-value-negative","count-squares-value-two",
        "count-rectangles-value-negative","total-value-negative","area-at-most-value","area-at-most-value-negative",
        "area-at-most-cell","area-at-most-ragged","min-sum-negative","min-sum-late-negative","min-sum-overflow",
        "min-sum-2d-negative","min-sum-2d-short-row","min-sum-2d-long-row","min-sum-2d-overflow",
        "checker-square-cell","checker-square-ragged","checker-rectangle-cell","checker-rectangle-ragged","next-smaller-max-size"]))
