#!/usr/bin/env python3
"""Halfplane intersection, clipping, kernels and Chebyshev centre; quick/full/stress."""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('10-halfplaneintersection', ['degenerate-plane', 'coordinate-bound', 'clip-clockwise', 'clip-small',
                                                      'kernel-zero-area', 'linear-zero-area', 'linear-spike', 'linear-spike-first',
                                                      'linear-spike-last', 'linear-bound']))
