#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("24-interval_set", ["set-insert-reversed","set-erase-reversed","set-covers-reversed","map-reversed-universe","map-get-end","map-assign-end","map-assign-reversed","map-apply-negative","map-enumerate-end","map-split-end","map-merge-reversed"]))
