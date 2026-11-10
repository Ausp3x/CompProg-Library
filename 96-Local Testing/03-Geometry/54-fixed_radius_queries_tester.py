#!/usr/bin/env python3
"""Grid-hash and sorted-grid fixed-radius neighbours, pairs and cover counts; exact maximum points in a disk; quick/full/stress."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('54-fixed_radius_queries', ['grid-r', 'grid-coordinate', 'grid-index', 'sorted-r', 'sorted-coordinate', 'sorted-index', 'disk-r2']))
