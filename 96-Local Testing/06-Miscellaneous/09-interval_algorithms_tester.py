"""Independent interval set oracles, with exact endpoint and witness checks.

Quick: every ordered pair on three coordinates, 100 random inputs, n=1000.
Full: every multiset of <=3 intervals on three coordinates, 2500 random inputs,
n=100000, optimized/checked/ASan-UBSan. Stress: ordered triples, 15000 random
inputs, n=500000, all configurations. Checked large cases cap n at 2000 because
libstdc++ debug partition checks make repeated std::lower_bound linear per call.
All modes cover every public operation.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('09-interval_algorithms', [
        'bounds-reversed', 'merge-reversed', 'events-reversed',
        'counts-reversed', 'stabbing-reversed', 'schedule-reversed',
        'weights-reversed', 'weights-size', 'cover-reversed', 'cover-target-reversed',
        'partition-reversed', 'nested-reversed', 'nested-counts-reversed']))
