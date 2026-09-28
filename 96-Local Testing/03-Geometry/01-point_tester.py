#!/usr/bin/env python3
"""Independent exact, exhaustive ordering and floating primitive checks."""
import os
from pathlib import Path
import random
import sys
import tempfile

from _00_runner import main


def option(name, fallback):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else fallback


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def difference(a, b):
    return tuple(x - y for x, y in zip(a, b))


def cross(a, b):
    if len(a) == 2:
        return a[0] * b[1] - a[1] * b[0]
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def fixtures(path, mode, seed):
    """Python's arbitrary precision checks widening independently of C++ lll."""
    rng = random.Random(seed)
    count = {'quick': 100, 'full': 3000, 'stress': 15000}[mode]
    with path.open('w') as out:
        def emit2(a, b, c):
            delta = difference(a, b)
            det = cross(difference(b, a), difference(c, a))
            values = [2, *a, *b, *c, dot(a, b), cross(a, b), dot(a, a),
                      dot(delta, delta), (det > 0) - (det < 0)]
            out.write(' '.join(map(str, values)) + '\n')

        def emit3(a, b, c, d):
            delta = difference(a, b)
            values = [3, *a, *b, *c, *d, dot(a, b), *cross(a, b), dot(a, a),
                      dot(delta, delta), dot(a, cross(b, c)),
                      dot(difference(b, a), cross(difference(c, a), difference(d, a)))]
            out.write(' '.join(map(str, values)) + '\n')

        for scale in [0, 1, 999999999, 1000000000, 2**31 - 1]:
            for sign in [-1, 1]:
                x = scale * sign
                emit2((x, -x), (-x, x), (x, x))
                emit3((x, x, x), (-x, x, x), (x, -x, x), (x, x, -x))
        emit2((-2**63, 0), (0, 0), (0, 1))
        emit3((-2**63, 0, 0), (0, 0, 0), (0, 1, 0), (0, 0, 1))
        for _ in range(count):
            def point(n):
                return tuple(rng.randint(-10**9, 10**9) for _ in range(n))
            emit2(point(2), point(2), point(2))
            emit3(point(3), point(3), point(3), point(3))


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-point-oracle-') as folder:
        path = Path(folder) / 'exact.txt'
        fixtures(path, mode, seed)
        os.environ['CP_POINT_ORACLE'] = str(path)
        raise SystemExit(main('01-point', ['division-zero-2d', 'division-zero-3d']))
