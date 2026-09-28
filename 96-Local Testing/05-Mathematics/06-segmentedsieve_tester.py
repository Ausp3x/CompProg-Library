"""Segmented prime modes and factor tables against independent integer oracles.

Quick exhausts endpoints -4..24 with block sizes 1,7,30, runs 50 seeded intervals
and high endpoint regressions. Full uses -6..64 with block sizes 1,2,7,30,31,64,
500 seeded intervals and larger blocked counts. Stress extends to -8..96 and
3000 random intervals. Every mode checks repeatability, signed/empty/singleton
intervals, independent trial factorization and deterministic Miller-Rabin.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-segmentedsieve', [
        'base-negative', 'base-block', 'stream-order', 'stream-block', 'stream-mode',
        'base-short', 'constructor-order', 'closed-order', 'closed-maximum',
        'closed-base-maximum', 'material-size', 'factor-zero', 'factor-left',
        'factor-right', 'factor-interval-negative', 'reset-order']))
