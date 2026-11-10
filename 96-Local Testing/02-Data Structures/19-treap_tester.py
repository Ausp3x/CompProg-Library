#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("19-treap", ["treap-kth-range", "treap-kth-negative", "treap-kth-empty", "treap-merge-order", "treap-build-unsorted", "implicit-insert-range", "implicit-erase-range", "implicit-at-range", "implicit-at-negative", "implicit-set-range", "implicit-split-range", "implicit-split-negative", "implicit-reverse-reversed", "implicit-apply-range", "implicit-prod-range", "implicit-maxright-identity", "implicit-maxright-range", "implicit-minleft-range"]))
