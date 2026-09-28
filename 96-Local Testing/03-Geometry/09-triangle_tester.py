#!/usr/bin/env python3
"""Triangle exact/Fraction and high-precision Decimal oracles.

quick: 6561 exact grid cases, 100 random constructions, 109 exact fixtures and
79 Heron fixtures; full: 18225 grid cases, 2000 constructions and 2009 exact /
369 Heron fixtures in optimized, checked and ASan/UBSan builds; stress: 390625
grid cases, 12000 constructions and 12009 exact / 1869 Heron fixtures.
"""
from decimal import Decimal, localcontext
from fractions import Fraction
import itertools
import os
from pathlib import Path
import random
import sys
import tempfile

from _00_runner import main


def option(name, fallback):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else fallback


def fixtures(path, mode, seed):
    rng = random.Random(seed)
    count = {'quick': 100, 'full': 2000, 'stress': 12000}[mode]

    def determinant(a, b, c):
        return (a[0] * b[1] + b[0] * c[1] + c[0] * a[1]
                - a[1] * b[0] - b[1] * c[0] - c[1] * a[0])

    def exact(points):
        a, b, c, p = points
        d = determinant(a, b, c)
        w = [determinant(p, b, c), determinant(a, p, c), determinant(a, b, p)]
        center = [(Fraction(a[i]) + b[i] + c[i]) / 3 for i in range(2)]
        values = [*(x for point in points for x in point), d, *w]
        for x in center:
            values.extend([x.numerator, x.denominator])
        out.write('E ' + ' '.join(map(str, values)) + '\n')

    def decimal(x):
        x = Fraction(x)
        return Decimal(x.numerator) / Decimal(x.denominator)

    def heron(sides, precision=160):
        # The ordinary semiperimeter identity in arbitrary precision is an
        # independent reference for the sorted Kahan/exponent-scaled kernel.
        with localcontext() as ctx:
            ctx.prec = precision
            a, b, c = map(decimal, sides)
            s = (a + b + c) / 2
            valid = max(a, b, c) <= s
            area = (s * (s - a) * (s - b) * (s - c)).sqrt() if valid else Decimal(-1)
            # Side values are dyadic with <= 64 significant binary bits, so
            # these 160+-digit decimal strings recover the same binary input.
            values = [str(a), str(b), str(c), str(int(valid)), f'{area:.100E}']
            out.write('H ' + ' '.join(values) + '\n')

    with path.open('w') as out:
        corners = [(-10**9, -10**9), (-10**9, 10**9), (10**9, -10**9), (10**9, 10**9)]
        for a, b, c in itertools.permutations(corners[:3]):
            exact([a, b, c, corners[3]])
        exact([corners[0]] * 4)
        exact([(-10**9, -10**9), (0, 0), (10**9, 10**9), (10**9, -10**9)])
        exact([(-10**9, 1), (10**9, 0), (10**9 - 1, 0), (0, -10**9)])
        for i in range(count):
            bound = [3, 10**9, 100][i % 3]
            points = [tuple(rng.randint(-bound, bound) for _ in range(2)) for _ in range(4)]
            if i % 7 == 0:
                points[2] = points[i % 2]
            exact(points)
        bases = [(0, 0, 0), (0, 3, 3), (1, 2, 3), (1, 1, 3), (3, 4, 5),
                 (5, 5, 6), (1, 1, 1), (2, 1, 1 + Fraction(1, 2**60))]
        for sides in bases:
            for permutation in itertools.permutations(sides):
                heron(permutation)
        for exponent in [-8000, -6000, -2000, -1, 0, 1, 2000, 6000, 8000]:
            scale = Fraction(2) ** exponent
            heron([3 * scale, 4 * scale, 5 * scale])
        for exponent in [4000, 9000]:
            scale = Fraction(2) ** exponent
            for sides in itertools.permutations([scale, scale, 1 / scale]):
                heron(sides, precision=11000)
        for i in range({'quick': 10, 'full': 300, 'stress': 1800}[mode]):
            scale = Fraction(2) ** rng.randint(-7000, 7000)
            a, b = rng.randint(1, 1000), rng.randint(1, 1000)
            c = (a + b - Fraction(1, 2**35)) if i % 3 == 0 else rng.randint(0, a + b + 2)
            sides = [a * scale, b * scale, c * scale]
            rng.shuffle(sides)
            heron(sides)


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-triangle-oracle-') as folder:
        path = Path(folder) / 'oracle.txt'
        fixtures(path, mode, seed)
        os.environ['CP_TRIANGLE_ORACLE'] = str(path)
        raise SystemExit(main('09-triangle', ['exact-coordinate', 'exact-query',
            'floating-nan', 'floating-infinite', 'floating-query', 'weights-nan',
            'weights-infinite', 'excircle-index', 'excircle-negative-index',
            'excircle-rounded-gap', 'negative-side', 'nan-side', 'infinite-side',
            'heron-factor-overflow', 'heron-area-overflow', 'heron-area-underflow']))
