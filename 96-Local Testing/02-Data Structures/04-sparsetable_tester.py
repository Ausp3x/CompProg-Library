#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('04-sparsetable', ["prod-empty","prod-negative","prod-end","prod-reversed","associative-prod-empty","associative-prod-negative","associative-prod-end","associative-prod-reversed","empty-table","max-right-negative","max-right-end","min-left-negative","min-left-end","max-right-empty-table","grid-empty-rows","grid-empty-columns","grid-row-end","oversize","grid-oversize","grid-reversed-rows","grid-reversed-columns","grid-column-negative","grid-ragged","grid-ragged-empty-first","grid-zero-width"]))
