#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("35-persistentarray", ["get-version", "get-range", "get-negative", "set-version", "set-range", "tovector-version", "fill-negative"]))
