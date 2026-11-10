#!/usr/bin/env python3
"""Segment union, coverage histograms, rectangle union area/perimeter/coverage/depth/boundary/rings (integral and floating) and orthogonal intersections; quick/full/stress."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('13-segment_union', ['coordinate-bound', 'coverage-k', 'nan-rectangle', 'nonfinite-segment'], memory=2300, timeout=1500))
