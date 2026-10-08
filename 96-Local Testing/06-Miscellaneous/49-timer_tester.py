"""Timer and ScopedTimer checked against independent steady_clock brackets.

Every reading (elapsed, elapsedUs, elapsedMs, elapsedSec, expired, remaining,
progress) is bracketed by steady_clock samples taken around the reset and the
query: the reading must lie in [query start - reset end, query end - reset
start], truncated for microseconds, with limits below, at and above the bracket
(negative, zero, fractional, huge). Busy waits draw seeded random lengths;
quick/full/stress run 20/120/600 rounds with waits up to 2/8/20 ms and random
resets. All modes check copies, 100000 monotone readings, a 3 ms wait,
ScopedTimer format (label, three fixed decimals, " ms"), its bracket, untouched
stream flags/precision, nested destruction order and the cerr default.
progress with a non-positive limit is an assertion probe.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('49-timer', ['progress-zero', 'progress-negative']))
