#!/usr/bin/env python3
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == "__main__":
    # -D_GLIBCXX_ASSERTIONS: PBDS debug checks validate whole trees on every operation, which exceeds the timeout on its full corpus.
    raise SystemExit(main("16-mergesorttree", ["static-count-reversed", "static-count-end", "static-countrange-end", "static-kth-k", "static-kth-empty", "static-maxleq-end", "static-mingeq-negative", "dynamic-negative-size", "dynamic-insert-end", "dynamic-erase-negative", "dynamic-set-multi", "dynamic-set-empty", "dynamic-kth-k", "dynamic-count-reversed", "dynamic-countrange-reversed", "frequency-get-end", "frequency-set-negative", "frequency-count-reversed"], checked='-D_GLIBCXX_ASSERTIONS'))