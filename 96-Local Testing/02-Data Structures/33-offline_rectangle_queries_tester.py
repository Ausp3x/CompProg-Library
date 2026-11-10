#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    # -D_GLIBCXX_ASSERTIONS: debug lower_bound checks partitioning in O(n) per call and the sweep binary-searches its compressed y coordinates for every event.
    raise SystemExit(main("33-offline_rectangle_queries", ["rectangle-reversed-x", "rectangle-reversed-y", "query-reversed", "point-max-x", "point-query-max-y", "point-max-y", "point-query-max-x", "query-reversed-x"], checked='-D_GLIBCXX_ASSERTIONS'))