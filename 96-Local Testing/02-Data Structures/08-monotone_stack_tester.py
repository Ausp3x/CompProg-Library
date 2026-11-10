#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('08-monotone_stack', ["minimum-zero-window","minimum-negative-window","maximum-zero-window","negative-height","late-negative-height","binary-value","binary-negative-cell","binary-large-cell","binary-short-row","binary-long-row","legacy-ragged"]))
