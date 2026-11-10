#!/usr/bin/env python3
"""2-SAT versus brute-force assignment enumeration with semantic constraints.

quick: every clause set over 1-2 variables, 400 random mixed formulas,
at-most-one families, 20000-variable chains and 3000-variable forced
prefix. full: 6000 random formulas, 400000-variable chains, 20000-variable
forced prefix, in optimized, checked and ASan/UBSan builds. stress: every
clause set over 3 variables, 40000 random formulas, 1000000-variable chains.
"""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main

if __name__ == '__main__':
    raise SystemExit(main('18-twosat', ['literal-high', 'literal-low', 'negative-size',
        'forced-unsat', 'at-most-one-range']))
