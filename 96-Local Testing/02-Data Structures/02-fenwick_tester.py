#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('02-fenwick', ["negative-size","add-negative","add-end","add-past","prefix-negative","prefix-end","sum-negative","sum-reversed","sum-end","get-end","set-end","max-right-end","max-right-false","min-left-end","min-left-false","timestamp-negative-size","timestamp-add-negative","timestamp-add-end","timestamp-add-past","timestamp-prefix-negative","timestamp-prefix-end","timestamp-sum-negative","timestamp-sum-reversed","timestamp-sum-end"]))
