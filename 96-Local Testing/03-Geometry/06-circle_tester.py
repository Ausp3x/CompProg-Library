#!/usr/bin/env python3
"""Circle constructions and measures; exact classifications, residuals, quadrature.

quick: small exhaustive grids and 80 random constructions; full: all public
features, expanded grids (segment/power/relation endpoints and radii through 5,
Pythagorean-direction tangencies with |a| <= 75), 1600 constructions and all three
build configurations; stress: grids through 8 (|a| <= 192) and 16000 constructions.
Seed is printed on every failure.
"""
from _00_runner import main

if __name__ == '__main__':
    raise SystemExit(main('06-circle', ['negative-radius', 'infinite-center',
        'nan-radius', 'same-line-points', 'negative-tolerance', 'nan-point',
        'infinite-sweep', 'negative-segment', 'large-segment', 'nan-segment', 'nan-power',
        'relation-radius', 'relation-bound']))
