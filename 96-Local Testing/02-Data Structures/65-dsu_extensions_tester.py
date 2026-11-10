#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("65-dsu_extensions", ["linked-members-range", "linked-unite-range", "sum-unite-range", "sum-add-negative", "dual-find-range",
                                                "dual-apply-range", "forest-size", "forest-size-large", "forest-union-range", "forest-connect-range", "forest-time-negative",
                                                "forest-node-range", "forest-node-time-range", "forest-merge-time-range"]))
