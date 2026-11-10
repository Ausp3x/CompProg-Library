#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    raise SystemExit(main("30-aggregation_queue", ["queue-pop-empty", "queue-front-empty", "queue-back-empty", "deque-pop-front-empty", "deque-pop-back-empty", "deque-front-empty", "deque-back-empty", "stack-pop-empty", "stack-top-empty"]))
