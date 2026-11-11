#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('03-segmenttree', ["negative-size","oversize","vector-size","get-negative","get-end","set-end","apply-end","apply-end-full","prod-negative","prod-reversed","prod-end","max-right-index","min-left-index","max-right-identity","min-left-identity","fold-negative","fold-reversed","fold-end","fold-right-negative","fold-right-reversed","fold-right-end","set-many-negative","set-many-end","set-many-past","set-many-decreasing","set-many-empty-tree", "prod-ids-negative", "prod-ids-reversed", "prod-ids-end"]))
