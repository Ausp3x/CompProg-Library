#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("09-rollbackdsu", ["find-range", "find-negative", "unite-range", "undo-empty", "rollback-future", "rollback-negative", "size-negative",
                                             "aggregate-undo-empty", "aggregate-rollback-future", "aggregate-add-range"]))
