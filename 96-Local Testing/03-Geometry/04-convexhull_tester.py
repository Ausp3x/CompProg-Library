#!/usr/bin/env python3
"""Exhaustive small sets and independent Jarvis supporting-edge hull oracle."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('04-convexhull', ['sorted-unsorted', 'lower-sorted-unsorted', 'upper-sorted-unsorted',
                                       'sorted-unsorted-retain', 'lower-sorted-unsorted-retain', 'upper-sorted-unsorted-retain']))
