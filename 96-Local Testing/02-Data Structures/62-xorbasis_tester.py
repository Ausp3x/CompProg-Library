#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("62-xorbasis", ["kth-empty", "kth-out-of-range", "kth-size-63",
                                          "range-query-l-gt-r", "range-query-r-gt-n", "range-query-l-negative", "range-query-empty-array",
                                          "prefix-max-l-gt-r", "prefix-max-r-gt-n", "prefix-max-l-negative",
                                          "prefix-contains-l-gt-r", "prefix-contains-r-gt-n", "prefix-contains-l-negative",
                                          "prefix-push-wide-b1", "prefix-push-wide-b3", "prefix-push-wide-b63", "prefix-build-wide"]))
