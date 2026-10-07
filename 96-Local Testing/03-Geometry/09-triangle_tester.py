#!/usr/bin/env python3
"""Triangle exact/Fraction and high-precision Decimal oracles.

quick: 6561 exact grid cases, 100 random constructions, 109 exact, 79 Heron and
26 excircle fixtures; full: 18225 grid cases, 2000 constructions and 2009 exact /
369 Heron / 406 excircle fixtures in optimized, checked and ASan/UBSan builds;
stress: 390625 grid cases, 12000 constructions and 12009 / 1869 / 2406 fixtures.
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
        squares = sorted(sum((p[j] - q[j]) ** 2 for j in range(2)) for p, q in [(a, b), (b, c), (c, a)])
        kind = 2 if d == 0 else (squares[0] + squares[1] > squares[2]) - (squares[0] + squares[1] < squares[2])
        values = [*(x for point in points for x in point), d, *w, kind]
        for x in center:
            values.extend([x.numerator, x.denominator])
        out.write('E ' + ' '.join(map(str, values)) + '\n')

    def decimal(x):
        x = Fraction(x)
        return Decimal(x.numerator) / Decimal(x.denominator)

    def heron(sides, precision=160):
        # Exact Fraction classification and Heron factors; Decimal only for the square root.
        a, b, c = map(Fraction, sides)
        factors = [a + b + c, -a + b + c, a - b + c, a + b - c]
        valid = min(factors) >= 0
        with localcontext() as ctx:
            ctx.prec = precision
            area = decimal(factors[0] * factors[1] * factors[2] * factors[3] / 16).sqrt() if valid else Decimal(-1)
            # Sides are dyadic with <= 64 significant bits, so these strings round back exactly.
            values = [*(str(decimal(x)) for x in (a, b, c)), str(int(valid)), f'{area:.100E}']
            out.write('H ' + ' '.join(values) + '\n')

    def excircles(points, precision=200):
        # Direct x + y - z with enough digits that its cancellation stays below the reported 30.
        with localcontext() as ctx:
            ctx.prec = precision
            p = [[decimal(t) for t in q] for q in points]
            values = [str(decimal(t)) for q in points for t in q]
            for i in range(3):
                o, q, r = p[i], p[(i + 1) % 3], p[(i + 2) % 3]
                u, v = [q[0] - o[0], q[1] - o[1]], [r[0] - o[0], r[1] - o[1]]
                x = (v[0] ** 2 + v[1] ** 2).sqrt()
                y = (u[0] ** 2 + u[1] ** 2).sqrt()
                z = ((v[0] - u[0]) ** 2 + (v[1] - u[1]) ** 2).sqrt()
                gap = x + y - z
                values += [f'{o[j] + (u[j] * x + v[j] * y) / gap:.30E}' for j in range(2)]
                values.append(f'{abs(u[0] * v[1] - u[1] * v[0]) / gap:.30E}')
            out.write('X ' + ' '.join(values) + '\n')

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
        for i in range({'quick': 20, 'full': 400, 'stress': 2400}[mode]):
            # Integer coordinates <= 2^29 keep cross/dot exact in long double; thin obtuse triangles
            # put the third vertex next to the midpoint of a long side.
            big, k = rng.randint(1, 2**29 - 1), rng.choice([0, 0, rng.randint(-3000, 3000)])
            if i % 2:
                p, q = rng.randint(-big, big), rng.randint(-big, big)
                pts = [(-p, -q), (p, q), (rng.randint(-3, 3), rng.randint(-3, 3))]
                if pts[2][0] * q == pts[2][1] * p:
                    pts[2] = (pts[2][0] + 1, pts[2][1])
                if pts[0] == pts[1] or (pts[1][0] - pts[0][0]) * (pts[2][1] - pts[0][1]) == (pts[1][1] - pts[0][1]) * (pts[2][0] - pts[0][0]):
                    continue
            else:
                pts = [(rng.randint(-big, big), rng.randint(-big, big)) for _ in range(3)]
                if (pts[1][0] - pts[0][0]) * (pts[2][1] - pts[0][1]) == (pts[1][1] - pts[0][1]) * (pts[2][0] - pts[0][0]):
                    continue
            rng.shuffle(pts)
            excircles([[Fraction(t) * Fraction(2) ** k for t in q] for q in pts])
        for h in [1, 10, 30, 60, 100, 1000]:
            excircles([(0, 0), (2, 0), (1, Fraction(1, 2**h))], precision=200 + h)


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-triangle-oracle-') as folder:
        path = Path(folder) / 'oracle.txt'
        fixtures(path, mode, seed)
        os.environ['CP_TRIANGLE_ORACLE'] = str(path)
        raise SystemExit(main('09-triangle', ['exact-coordinate', 'exact-query', 'angle-kind-degenerate',
            'floating-nan', 'floating-infinite', 'floating-query', 'weights-nan',
            'weights-infinite', 'excircle-index', 'excircle-negative-index',
            'excircle-unrepresentable', 'negative-side', 'nan-side', 'infinite-side',
            'heron-factor-overflow', 'heron-area-overflow', 'heron-area-underflow']))
