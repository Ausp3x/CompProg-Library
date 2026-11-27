#!/usr/bin/env python3
"""Triangle exact/Fraction and high-precision Decimal oracles.

quick: 6561 exact grid cases, 100 random constructions, 109 exact, 79 Heron,
26 excircle, about 35 angle, 27 center and 200 orient/locate fixtures; full: 18225
grid cases, 2000 constructions and 2009 exact / 369 Heron / 406 excircle / about
380 angle / 367 center / 4600 orient/locate fixtures in optimized, checked and
ASan/UBSan builds; stress: 390625 grid cases, 12000 constructions and 12009 / 1869 /
2406 / about 2200 / 2200 / 27000 fixtures. Angle, center and locate counts vary with
the seed (rejected repeated-vertex or zero-area draws, out-of-bound queries).
"""
from decimal import Decimal, getcontext, localcontext
from fractions import Fraction
import itertools
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


PI = Decimal('3.14159265358979323846264338327950288419716939937510582097494459230781640628620899')


def atan(x):
    # Halving x -> x / (1 + sqrt(1 + x^2)) below 1e-4, then the alternating series to the context precision.
    halvings = 0
    while x > Decimal('1e-4'):
        x, halvings = x / (1 + (1 + x * x).sqrt()), halvings + 1
    total, power, k, limit = Decimal(0), x, 0, x * Decimal(10) ** -(getcontext().prec + 2)
    while power > limit:
        total += (-1) ** k * power / (2 * k + 1)
        power, k = power * x * x, k + 1
    return total * 2 ** halvings


def validate_atan():
    with localcontext() as ctx:
        ctx.prec = 80
        assert abs(4 * atan(Decimal(1)) - PI) < Decimal('1e-75')
        assert abs(6 * atan(1 / Decimal(3).sqrt()) - PI) < Decimal('1e-75')
        for x in ['1e-30', '0.001', '0.5', '7', '1e20']:
            assert abs(float(atan(Decimal(x))) - math.atan(float(x))) <= 1e-15


def fixtures(path, mode, seed):
    validate_atan()
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
        values.append(locate(a, b, c, p))
        out.write('E ' + ' '.join(map(str, values)) + '\n')

    def on_segment(u, v, p):
        return (determinant(u, v, p) == 0 and min(u[0], v[0]) <= p[0] <= max(u[0], v[0])
                and min(u[1], v[1]) <= p[1] <= max(u[1], v[1]))

    def locate(a, b, c, p):
        # Signs of the exact barycentric weights; a zero-area triangle is the union of its edges. Both agree on edges.
        d = determinant(a, b, c)
        edge = on_segment(a, b, p) or on_segment(b, c, p) or on_segment(c, a, p)
        if d == 0:
            return 0 if edge else 1
        low = min((x > 0) - (x < 0) for x in (determinant(p, b, c) * d, determinant(a, p, c) * d, determinant(a, b, p) * d))
        where = 1 if low < 0 else -1 if low > 0 else 0
        assert (where == 0) == edge, (a, b, c, p)
        return where

    def located(points):
        a, b, c, p = points
        if max(abs(t) for q in points for t in q) > 10**9:
            return
        d = determinant(a, b, c)
        out.write('L ' + ' '.join(map(str, [*(x for q in points for x in q), (d > 0) - (d < 0), locate(a, b, c, p)])) + '\n')

    def angles(points):
        # Law of cosines on exact squared sides: tan = sqrt(4 b^2 c^2 - n^2) / |n| with n = b^2 + c^2 - a^2 (Heron's 16 * area^2).
        with localcontext() as ctx:
            ctx.prec = 80
            values = [str(t) for q in points for t in q]
            for i in range(3):
                o, q, r = points[i], points[(i + 1) % 3], points[(i + 2) % 3]
                b2, c2 = sum((o[j] - q[j]) ** 2 for j in range(2)), sum((o[j] - r[j]) ** 2 for j in range(2))
                n = b2 + c2 - sum((q[j] - r[j]) ** 2 for j in range(2))
                heron = 4 * b2 * c2 - n * n
                assert heron >= 0 and b2 and c2
                angle = PI / 2 if n == 0 else atan(Decimal(heron).sqrt() / abs(n))
                values.append(f'{angle if n >= 0 else PI - angle:.40E}')
            out.write('A ' + ' '.join(values) + '\n')

    def centers(points):
        # Cramer on the perpendicular bisectors 2 (q - a) . O = |q|^2 - |a|^2 and the altitudes (H - a) . (b - c) = 0, (H - b) . (c - a) = 0.
        a, b, c = points
        assert determinant(a, b, c)

        def solve(m, r):
            det = m[0][0] * m[1][1] - m[0][1] * m[1][0]
            return Fraction(r[0] * m[1][1] - m[0][1] * r[1], det), Fraction(m[0][0] * r[1] - r[0] * m[1][0], det)

        def canonical(p):
            d = math.lcm(p[0].denominator, p[1].denominator)
            x, y = int(p[0] * d), int(p[1] * d)
            g = math.gcd(x, y, d)
            return [x // g, y // g, d // g]

        sq = lambda p: p[0] ** 2 + p[1] ** 2
        dot = lambda p, q: p[0] * q[0] + p[1] * q[1]
        sub = lambda p, q: (p[0] - q[0], p[1] - q[1])
        o = solve([[2 * (b[0] - a[0]), 2 * (b[1] - a[1])], [2 * (c[0] - a[0]), 2 * (c[1] - a[1])]], [sq(b) - sq(a), sq(c) - sq(a)])
        h = solve([sub(b, c), sub(c, a)], [dot(a, sub(b, c)), dot(b, sub(c, a))])
        r2 = sq(sub(o, a))
        assert sq(sub(o, b)) == r2 == sq(sub(o, c)) and dot(sub(h, c), sub(a, b)) == 0
        values = [*(t for q in points for t in q), *canonical(o), *canonical(h), r2.numerator, r2.denominator]
        out.write('C ' + ' '.join(map(str, values)) + '\n')

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
        # Direct x + y - z with enough digits that its cancellation stays below the reported 40 (binary128 long double needs 34).
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
                values += [f'{o[j] + (u[j] * x + v[j] * y) / gap:.40E}' for j in range(2)]
                values.append(f'{abs(u[0] * v[1] - u[1] * v[0]) / gap:.40E}')
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
        big = 10**9
        for points in itertools.permutations([(0, 0), (4, 0), (0, 3)]):
            angles(points)
        for points in [corners[:3], corners[1:], [(-big, 0), (big, 0), (big, 1)], [(-big, -big), (big, big - 1), (big, big)],
                       [(-big, 0), (big, 0), (0, 1)], [(-big, 0), (0, 0), (big, 0)], [(-big, -big), (big, big), (0, 0)],
                       [(0, 0), (1, 0), (0, big)], [(-big, big), (big, -big), (big - 1, -big + 1)]]:
            angles(points)
        for i in range({'quick': 20, 'full': 400, 'stress': 2400}[mode]):
            bound = [3, 100, big][i % 3]
            pick = lambda: (rng.randint(-bound, bound), rng.randint(-bound, bound))
            if i % 4 == 0:
                # Right angle at the first vertex: perpendicular integer legs.
                a, (x, y), m = pick(), pick(), rng.randint(1, 3)
                pts = [a, (a[0] + x, a[1] + y), (a[0] - m * y, a[1] + m * x)]
            elif i % 4 == 1:
                # Thin: a vertex next to the midpoint of a long side, or collinear on it.
                (x, y), (e, f) = pick(), (rng.randint(-2, 2), rng.randint(-2, 2))
                pts = [(-x, -y), (x, y), (e, f)]
            else:
                pts = [pick() for _ in range(3)]
            rng.shuffle(pts)
            if len(set(pts)) == 3 and max(abs(t) for q in pts for t in q) <= big:
                angles(pts)
        for points in [corners[:3], corners[1:], [(-big, -big), (big, big), (big, big - 1)], [(-big, -big + 1), (big, big), (-big, -big)],
                       [(-big, 0), (big, 0), (0, 1)], [(-big, big), (big, -big), (big - 1, -big)], [(0, 0), (4, 0), (0, 3)],
                       [(-big, -big), (big, -big), (0, big)], [(big, big), (-big, big - 2), (-big + 1, big - 1)]]:
            centers(points)
        for i in range({'quick': 20, 'full': 400, 'stress': 2400}[mode]):
            bound = [3, 100, big][i % 3]
            pick = lambda: (rng.randint(-bound, bound), rng.randint(-bound, bound))
            if i % 4 == 0:
                a, (x, y), m = pick(), pick(), rng.randint(1, 3)
                pts = [a, (a[0] + x, a[1] + y), (a[0] - m * y, a[1] + m * x)]
            elif i % 4 == 1:
                (x, y), (e, f) = pick(), (rng.randint(-2, 2), rng.randint(-2, 2))
                pts = [(-x, -y), (x, y), (e, f)]
            else:
                pts = [pick() for _ in range(3)]
            rng.shuffle(pts)
            if determinant(*pts) and max(abs(t) for q in pts for t in q) <= big:
                centers(pts)
        for a, b, c in itertools.permutations(corners[:3]):
            for p in [corners[3], (0, 0), (0, -big), (-big, 0), (1, 0), (-1, 0), *corners[:3]]:
                located([a, b, c, p])
        for i in range({'quick': 40, 'full': 1000, 'stress': 6000}[mode]):
            # Edges with a large gcd: a + m * d for small direction d, so exact edge points lie between the vertices.
            a = (rng.randint(-big, big), rng.randint(-big, big))
            steps = [(rng.randint(-50, 50), rng.randint(-50, 50)) for _ in range(2)]
            if i % 3 == 0:
                steps[1] = steps[0]
            scale = [rng.randint(-big // 50, big // 50) for _ in range(2)]
            if i % 5 == 0:
                scale[rng.randrange(2)] = 0
            pts = [a, *((a[0] + m * x, a[1] + m * y) for m, (x, y) in zip(scale, steps))]
            rng.shuffle(pts)
            u, v = rng.sample(pts, 2)
            dx, dy = v[0] - u[0], v[1] - u[1]
            g = max(math.gcd(dx, dy), 1)
            k = rng.randint(-2, g + 2)
            p = (u[0] + k * dx // g, u[1] + k * dy // g)
            for e in [(0, 0), (1, 0), (-1, 0), (0, 1), (0, -1)]:
                located([*pts, (p[0] + e[0], p[1] + e[1])])
            located([*pts, (rng.randint(-big, big), rng.randint(-big, big))])
            located([*pts, rng.choice(pts)])


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-triangle-oracle-') as folder:
        path = Path(folder) / 'oracle.txt'
        fixtures(path, mode, seed)
        os.environ['CP_TRIANGLE_ORACLE'] = str(path)
        raise SystemExit(main('09-triangle', ['exact-coordinate', 'excircle-gap-overflow', 'excircle-gap-subnormal', 'exact-coordinate-b', 'exact-coordinate-c', 'floating-third', 'exact-query', 'locate-query', 'angle-kind-degenerate', 'angles-repeated-vertex',
            'floating-nan', 'floating-infinite', 'floating-query', 'weights-nan',
            'weights-infinite', 'excircle-index', 'excircle-negative-index',
            'excircle-unrepresentable', 'negative-side', 'nan-side', 'infinite-side',
            'heron-factor-overflow', 'heron-area-overflow', 'heron-area-underflow', 'transform-overflow',
            'circumcenter-degenerate', 'circumradius2-degenerate', 'orthocenter-degenerate',
            'altitudes-repeated-vertex', 'altitudes-side-overflow', 'altitude-feet-side-overflow', 'altitude-feet-repeated-vertex', 'altitudes2-repeated-vertex', 'exact-altitude-feet-repeated-vertex']))
