#!/usr/bin/env python3
"""Independent exact, exhaustive ordering and floating primitive checks."""
from fractions import Fraction
import math
import os
from pathlib import Path
import random
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main


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
        signed = [(0.0, 0.0), (-0.0, 0.0), (0.0, -0.0), (-0.0, -0.0), (1.0, -0.0), (1.0, 0.0), (-1.0, 0.0),
                  (-1.0, -0.0), (-0.0, 1.0), (0.0, 1.0), (-0.0, -1.0), (0.0, -1.0), (2.0, -0.0), (-3.0, -0.0)]
        for _ in range(8):
            rng.shuffle(signed)
            approx(out, signed)
        for _ in range({'quick': 40, 'full': 600, 'stress': 4000}[mode]):
            approx(out, approx_points(rng))


def approx_key(x, y):
    """Contract key: angle in (-pi, pi], y = +-0 on the axis, the zero vector at 0."""
    if y == 0:
        return 0.0 if x >= 0 else math.pi
    return math.atan2(y, x)


def direction(x, y):
    """Exact direction class: the vector scaled by its largest |coordinate|; zero joins the +x axis at angle 0."""
    fx, fy = Fraction(x), Fraction(y)
    m = max(abs(fx), abs(fy))
    return (Fraction(1), Fraction(0)) if m == 0 else (fx / m, fy / m)


def approx_points(rng):
    """Random doubles, small integers and signed zeros, with power-of-two scalings, flipped zero signs and duplicates."""
    points = []
    for _ in range(rng.randint(0, 8)):
        kind = rng.randrange(3)
        if kind == 0:
            v = (rng.uniform(-1000, 1000), rng.uniform(-1000, 1000))
        elif kind == 1:
            v = (float(rng.randint(-3, 3)), float(rng.randint(-3, 3)))
        else:
            v = (rng.choice([0.0, -0.0, 1.0, -1.0, 2.5]), rng.choice([0.0, -0.0, 1.0, -1.0, -2.5]))
        points.append(v)
        for _ in range(rng.randint(0, 2)):
            e = rng.randint(-60, 60)
            points.append((math.ldexp(v[0], e), math.ldexp(v[1], e)))
        if rng.random() < 0.3:
            points.append(tuple(-c if c == 0 else c for c in v))
        if rng.random() < 0.2:
            points.append(v)
    rng.shuffle(points)
    return points


def approx(out, points):
    """Record: 4 n, then x y key group rank per point; rank orders a group by exact norm."""
    groups, norms = {}, {}
    for x, y in points:
        d = direction(x, y)
        groups.setdefault(d, len(groups))
        norms.setdefault(d, set()).add(Fraction(x) ** 2 + Fraction(y) ** 2)
    ranks = {d: sorted(n) for d, n in norms.items()}
    values = [4, len(points)]
    for x, y in points:
        d = direction(x, y)
        values += [repr(x), repr(y), repr(approx_key(x, y)), groups[d],
                   ranks[d].index(Fraction(x) ** 2 + Fraction(y) ** 2)]
    out.write(' '.join(map(str, values)) + '\n')


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-point-oracle-') as folder:
        path = Path(folder) / 'exact.txt'
        fixtures(path, mode, seed)
        os.environ['CP_POINT_ORACLE'] = str(path)
        raise SystemExit(main('01-point', ['division-zero-2d', 'division-zero-3d', 'unit-zero', 'arg-zero', 'angle-zero',
                                        'signed-angle-zero', 'angle3-zero', 'canonical-minimum', 'angle-struct-zero',
                                        'angle-step-zero', 'angle-step-through-origin', 'angle-step-back-through-origin', 'segment-angles-turns', 'normal-zero', 'normal3-zero', 'normal3-parallel',
                                        'projection-zero', 'projection3-zero']))
