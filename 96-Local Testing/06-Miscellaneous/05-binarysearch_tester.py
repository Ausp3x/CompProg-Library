"""Test the Miscellaneous facade with the canonical search family's complete suite.

Quick: exhaustive integer/minimum boundaries through n=6 and 100 random cases.
Full: through n=8, 3000 random cases, real limits and all three build configurations.
Stress: through n=9 and 30000 random cases. Facade availability is checked before
the canonical test source is included, so accidentally empty facades fail to compile.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('05-binarysearch', [
        'first-order', 'last-order', 'integer-order', 'binary-iterations',
        'ternary-iterations', 'golden-iterations', 'binary-infinite',
        'ternary-nan', 'golden-infinite', 'ternary-order', 'golden-order',
        'negative-absolute', 'negative-relative', 'infinite-tolerance', 'nan-tolerance']))
