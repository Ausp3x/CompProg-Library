#!/usr/bin/env python3
"""Disk-polygon areas and circle arcs inside polygons; quick/full/stress."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('16-circle_polygon', ['arcs-zero-radius', 'negative-radius']))
