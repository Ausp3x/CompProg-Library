"""Sequence suite: quick smoke, full exhaustive/random/domain/sanitizer, stress expands bounds."""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('08-sequence_algorithms', [
        'negative-length', 'reversed-length', 'ragged', 'weight-size',
        'negative-window', 'negative-limit', 'bad-k', 'unsorted', 'legacy-overflow', 'average-length']))
