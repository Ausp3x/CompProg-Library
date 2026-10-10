#!/usr/bin/env python3
from _00_runner import main

if __name__ == "__main__":
    raise SystemExit(main("30-aggregation_queue", ["queue-pop-empty", "queue-front-empty", "queue-back-empty", "deque-pop-front-empty", "deque-pop-back-empty", "deque-front-empty", "deque-back-empty", "stack-pop-empty", "stack-top-empty"]))
