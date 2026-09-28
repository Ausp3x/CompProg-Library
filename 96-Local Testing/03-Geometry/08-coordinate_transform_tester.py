#!/usr/bin/env python3
"""Fraction oracles and geometric invariants for approximate affine maps."""
from decimal import Decimal, localcontext
from fractions import Fraction
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
    count = {'quick': 100, 'full': 3000, 'stress': 20000}[mode]

    def apply(f, p):
        return [sum(x * y for x, y in zip(row, p)) for row in f]

    def matrix(f):
        a, b, c, d, x, y = f
        return [[a, b, x], [c, d, y], [0, 0, 1]]

    def format_value(x):
        x = Fraction(x)
        return str(Decimal(x.numerator) / Decimal(x.denominator))

    with path.open('w') as out, localcontext() as ctx:
        ctx.prec = 80
        for i in range(count):
            scale = [1, 10**9, Fraction(1, 2**20)][i % 3]
            f = [Fraction(rng.randint(-10, 10) * scale) for _ in range(6)]
            g = [rng.randint(-10, 10) for _ in range(6)]
            if i % 5 == 0:
                f[2:4] = f[:2]  # exactly singular, including rank zero cases
            p = [rng.randint(-100, 100) for _ in range(2)] + [1]
            h = [rng.randint(-100, 100) for _ in range(3)]
            if i % 4 == 0:
                h[2] = 0
            m, n = matrix(f), matrix(g)
            det = f[0] * f[3] - f[1] * f[2]
            hp = apply(m, h)
            values = [*f, *g, *p[:2], *h, *apply(m, p)[:2],
                      *apply(m, p[:2] + [0])[:2], *hp,
                      *apply(m, apply(n, p))[:2], det]
            if det:
                x, y = p[0] - f[4], p[1] - f[5]
                values += [(f[3] * x - f[1] * y) / det, (f[0] * y - f[2] * x) / det]
            else:
                values += [0, 0]
            values += [Fraction(h[0], h[2]), Fraction(h[1], h[2])] if h[2] else [0, 0]
            out.write(' '.join(format_value(x) for x in values) + '\n')


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-transform-oracle-') as folder:
        path = Path(folder) / 'fraction.txt'
        fixtures(path, mode, seed)
        os.environ['CP_TRANSFORM_ORACLE'] = str(path)
        raise SystemExit(main('08-coordinate_transform', ['rotation-nan', 'projection-point',
                                                         'reflection-point', 'similarity-point']))
