#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("28-disjointsparsetable", ["semigroup-prod-empty", "prod-range", "prod-negative", "prod-reversed", "static-reversed", "static-range"]))
