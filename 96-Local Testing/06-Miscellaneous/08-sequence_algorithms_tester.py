"""Sequence suite: quick smoke, full exhaustive/random/domain/sanitizer, stress expands bounds."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('08-sequence_algorithms', [
        'negative-length', 'reversed-length', 'ragged', 'weight-size',
        'negative-window', 'negative-limit', 'bad-k', 'unsorted', 'average-length']))
