#!/usr/bin/env python3
"""Polygon cells, winding, signed moments, lattice and holes; quick/full/stress."""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('03-polygon', ['pick-empty', 'pick-collinear', 'holes-empty', 'holes-orientation', 'lattice-bound']))
