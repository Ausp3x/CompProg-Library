#!/usr/bin/env python3
from _00_runner import main

if __name__ == "__main__":
    raise SystemExit(main("63-offline_deletion", ["interval-reversed", "interval-range", "query-range", "insert-range", "erase-absent", "erase-early",
                                                  "time-negative", "time-large", "queue-pop-empty"]))
