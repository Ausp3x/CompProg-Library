#!/usr/bin/env python3
"""Convex polygon queries, cuts, intersection and inscribed k-gons against exhaustive and clipConvex oracles; quick/full/stress."""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('15-convex_polygon_query', ['clockwise', 'collinear', 'small', 'tangent-inside', 'extreme-zero',
                                                      'cut-degenerate', 'batch-unsorted', 'kgon-k', 'kgon-smawk-k', 'pentagram', 'extreme-bound',
                                                      'intersection-clockwise']))
