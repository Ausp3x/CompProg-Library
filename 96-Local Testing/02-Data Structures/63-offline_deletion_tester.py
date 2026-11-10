#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("63-offline_deletion", ["interval-reversed", "interval-range", "query-range", "insert-range", "erase-absent", "erase-early",
                                                  "time-negative", "time-large", "queue-pop-empty"]))
