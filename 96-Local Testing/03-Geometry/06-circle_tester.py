#!/usr/bin/env python3
"""Circle constructions and measures; exact classifications, residuals, quadrature.

quick: small exhaustive grids and 80 random constructions; full: all public
features, expanded grids, 1600 constructions and all three build configurations;
stress: larger grids and 16000 constructions. Seed is printed on every failure.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-circle', ['negative-radius', 'infinite-center',
        'nan-radius', 'same-line-points', 'negative-tolerance', 'nan-point',
        'infinite-sweep', 'negative-segment', 'large-segment']))
