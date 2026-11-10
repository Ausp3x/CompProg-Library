#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("36-fastset", ["insert-range", "erase-negative", "contains-range", "enumerate-reversed", "enumerate-past", "size-negative",
                                         "decremental-erase-range", "decremental-contains-negative", "decremental-enumerate-past", "decremental-enumerate-reversed", "decremental-size-negative"]))
