#!/usr/bin/env python3
"""Halfplane intersection, clipping, kernels and Chebyshev centre; quick/full/stress."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('10-halfplaneintersection', ['degenerate-plane', 'coordinate-bound', 'clip-clockwise', 'clip-small',
                                                      'kernel-zero-area', 'linear-zero-area', 'linear-spike', 'linear-spike-first',
                                                      'linear-spike-last', 'linear-bound']))
