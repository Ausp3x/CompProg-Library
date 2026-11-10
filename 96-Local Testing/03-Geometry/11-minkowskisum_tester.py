#!/usr/bin/env python3
"""Convex Minkowski sums, differences, families and hull of nonconvex sums; quick/full/stress."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('11-minkowskisum', ['clockwise', 'collinear-vertex', 'repeated-pair', 'family-nonconvex']))
