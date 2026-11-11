#!/usr/bin/env python3
"""Independent exact rational fixtures; all builds run the identical corpus.

Quick: all object pairs on five grid sites, four-site extrema, 80 seeded cases.
Full: all pairs on the 3x3 grid, seven-site extrema, 1500 seeded cases.
Stress: full coverage with 12000 seeded arbitrary/collinear cases.
Every mode also writes ccw triples and line pairs on its grid, exact squared
distances for every grid object pair and extremal kind pair, and seeded dyadic
and well-conditioned floating line pairs, and modular-inverse equation lines
(all extreme lng triples, box-edge steps, then 80/1500/12000 seeded rounds).
No nonstandard Python packages are required.
"""
import argparse
from decimal import Decimal, localcontext
from fractions import Fraction as F
import itertools
import math
import os
from pathlib import Path
import random
import tempfile

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main


def equation(s):
    a, b, _ = s
    x, y = a[1] - b[1], b[0] - a[0]
    return x, y, x * a[0] + y * a[1]


def member(s, p):
    a, b, kind = s
    if a == b:
        return p == a
    x, y, z = equation(s)
    if x * p[0] + y * p[1] != z:
        return False
    if kind == 2:
        return True
    axis = a[0] == b[0]
    t = F(p[axis] - a[axis], b[axis] - a[axis])
    return t >= 0 and (kind == 1 or t <= 1)


def reference(s, t):
    """Implicit equations + feasible endpoints and distant unbounded witnesses.

    This avoids the implementation's cross-parametrization and interval merge.
    Kind values: empty=0, point=1, segment=2, ray=3, line=4.
    """
    a, b, _ = s
    c, d, _ = t
    if a == b:
        return (1, a, a) if member(t, a) else (0, (0, 0), (0, 0))
    if c == d:
        return (1, c, c) if member(s, c) else (0, (0, 0), (0, 0))
    u, v = equation(s), equation(t)
    det = u[0] * v[1] - v[0] * u[1]
    if det:
        p = F(u[2] * v[1] - v[2] * u[1], det), F(u[0] * v[2] - v[0] * u[2], det)
        return (1, p, p) if member(s, p) and member(t, p) else (0, (0, 0), (0, 0))
    ends = sorted(p for p in (a, b, c, d) if member(s, p) and member(t, p))
    if not ends:
        return 0, (0, 0), (0, 0)
    low, high = sorted((a, b))
    direction = high[0] - low[0], high[1] - low[1]

    def distant(sign):
        # Endpoints lie in [-1e9,1e9], a nonzero integral direction component
        # has magnitude >=1, so 1e10 passes every finite endpoint.
        p = tuple(a[i] + sign * 10**10 * direction[i] for i in range(2))
        return member(s, p) and member(t, p)

    negative, positive = distant(-1), distant(1)
    if negative and positive:
        return 4, a, b
    if negative or positive:
        p = ends[0] if positive else ends[-1]
        return 3, p, tuple(p[i] + (1 if positive else -1) * direction[i] for i in range(2))
    return (1 if ends[0] == ends[-1] else 2), ends[0], ends[-1]


def homogeneous(p):
    x, y = map(F, p)
    d = math.lcm(x.denominator, y.denominator)
    return x.numerator * (d // x.denominator), y.numerator * (d // y.denominator), d


def write_intersection(out, s, t):
    kind, a, b = reference(s, t)
    rational = homogeneous(a) if kind == 1 else (0, 0, 1)
    if kind == 1:
        a = b = (0, 0)
    u, v = equation(s), equation(t)
    parallel = u[0] * v[1] == v[0] * u[1]
    line = s if s[0] != s[1] else t
    coeff = equation(line)
    collinear = all(coeff[0] * p[0] + coeff[1] * p[1] == coeff[2] for p in (s[0], s[1], t[0], t[1]))
    booleans = [parallel, collinear]
    for p in (s[0], s[1], t[0], t[1], (0, 0)):
        booleans += [member(s, p), member((s[0], s[1], 0), p)]
    us, ut = (s[1][0] - s[0][0], s[1][1] - s[0][1]), (t[1][0] - t[0][0], t[1][1] - t[0][1])
    booleans.append(us[0] * ut[0] + us[1] * ut[1] == 0)
    bits = sum(int(value) << i for i, value in enumerate(booleans))
    values = (*s[0], *s[1], s[2], *t[0], *t[1], t[2], kind, *rational, *a, *b, bits)
    out.write('I ' + ' '.join(map(str, values)) + '\n')


def squared_distance(p, a, b):
    ux, uy = b[0] - a[0], b[1] - a[1]
    vx, vy = p[0] - a[0], p[1] - a[1]
    d, n = ux * ux + uy * uy, ux * vx + uy * vy
    if not d or n <= 0:
        return F(vx * vx + vy * vy)
    if n >= d:
        return F((p[0] - b[0])**2 + (p[1] - b[1])**2)
    # Exact Pythagorean subtraction; implementation uses perpendicular height.
    return F(vx * vx + vy * vy) - F(n * n, d)


def decimal(q):
    q = F(q)
    return Decimal(q.numerator) / Decimal(q.denominator)


def write_metric(out, p, a, b, c):
    pd = decimal(squared_distance(p, a, b)).sqrt()
    sd = min(squared_distance(p, b, c), squared_distance(a, b, c),
             squared_distance(b, p, a), squared_distance(c, p, a))
    sd = Decimal(0) if reference((p, a, 0), (b, c, 0))[0] else decimal(sd).sqrt()
    px = py = rx = ry = ld = Decimal(0)
    if a != b:
        ux, uy = b[0] - a[0], b[1] - a[1]
        vx, vy = p[0] - a[0], p[1] - a[1]
        parameter = F(ux * vx + uy * vy, ux * ux + uy * uy)
        px, py = decimal(a[0] + ux * parameter), decimal(a[1] + uy * parameter)
        rx, ry = 2 * px - p[0], 2 * py - p[1]
        ld = Decimal(ux * vy - uy * vx) / Decimal(ux * ux + uy * uy).sqrt()
    values = (*p, *a, *b, *c, pd, sd, px, py, rx, ry, ld)
    out.write('M ' + ' '.join(map(str, values)) + '\n')


def ccw_reference(a, b, c):
    """Side of the implicit equation, then the rational parameter along one axis."""
    x, y, z = equation((a, b, 0))
    side = x * c[0] + y * c[1] - z
    if side:
        return 1 if side > 0 else -1
    axis = a[0] == b[0]
    t = F(c[axis] - a[axis], b[axis] - a[axis])
    return 2 if t < 0 else -2 if t > 1 else 0


def write_ccw(out, a, b, c):
    out.write('C ' + ' '.join(map(str, (*a, *b, *c, ccw_reference(a, b, c)))) + '\n')


def line_reference(a, b, c, d):
    """Cramer on the implicit equations; kind values: empty=0, point=1, line=4."""
    u, v = equation((a, b, 2)), equation((c, d, 2))
    det = u[0] * v[1] - v[0] * u[1]
    if det:
        return 1, (F(u[2] * v[1] - v[2] * u[1], det), F(u[0] * v[2] - v[0] * u[2], det))
    return (4 if u[0] * c[0] + u[1] * c[1] == u[2] else 0), (0, 0)


def write_line(out, a, b, c, d, real=False):
    """Tag L: integral endpoints; tag R: exactly representable doubles (repr round-trips)."""
    kind, p = line_reference(*(tuple(map(F, q)) for q in (a, b, c, d)))
    coords = [repr(float(x)) if real else str(x) for q in (a, b, c, d) for x in q]
    out.write(('R ' if real else 'L ') + ' '.join(coords + [str(kind), str(decimal(p[0])), str(decimal(p[1]))]) + '\n')


def dot(u, v):
    return u[0] * v[0] + u[1] * v[1]


def distance_reference(s, t):
    """Minimum of the convex |a + x u - c - y v|^2 over the parameter box, face by face (KKT).

    Each finite bound pins a variable; a free variable takes its stationary value on
    that face. A minimizer lies in the relative interior of some face and minimizes the
    face's affine hull; only the free-free face of parallel directions has a non-unique
    minimizer, whose line of minimizers meets a pinned face unless both are lines, when
    pinning y = 0 is exact. Every candidate offered is feasible. Independent of intersect.
    """
    (a, b, i), (c, d, j) = s, t
    u, v, w = (b[0] - a[0], b[1] - a[1]), (d[0] - c[0], d[1] - c[1]), (a[0] - c[0], a[1] - c[1])
    boxes = [((0, 0) if z == (0, 0) else ((0, 1), (0, None), (None, None))[k]) for z, k in ((u, i), (v, j))]
    uu, uv, vv, wu, wv = dot(u, u), dot(u, v), dot(v, v), dot(w, u), dot(w, v)

    def faces(box):
        return sorted({e for e in box if e is not None}) + ([None] if box != (0, 0) else [])

    def feasible(z, box):
        return (box[0] is None or z >= box[0]) and (box[1] is None or z <= box[1])

    best = None
    for p, q in itertools.product(faces(boxes[0]), faces(boxes[1])):
        x, y = p, q
        if x is None and y is None:
            det = uv * uv - uu * vv
            if det:
                x, y = F(wu * vv - uv * wv, det), F(uv * wu - uu * wv, det)
            elif boxes[1] == (None, None):
                y = 0
            else:
                continue
        if x is None:
            x = F(y * uv - wu, uu)
        elif y is None:
            y = F(wv + x * uv, vv)
        if feasible(x, boxes[0]) and feasible(y, boxes[1]):
            value = sum((w[k] + x * u[k] - y * v[k]) ** 2 for k in range(2))
            best = value if best is None else min(best, value)
    return F(best)


def write_distance(out, s, t):
    q = distance_reference(s, t)
    assert (q == 0) == (reference(s, t)[0] != 0), (s, t, q)
    values = (*s[0], *s[1], s[2], *t[0], *t[1], t[2], q.numerator, q.denominator, decimal(q).sqrt())
    out.write('D ' + ' '.join(map(str, values)) + '\n')


def lines_and_points(out, rng, sites, rounds):
    """ccw and line-pair grids, seeded wide, collinear, dyadic and floating cases."""
    for a, b, c in itertools.product(sites, repeat=3):
        if a != b:
            write_ccw(out, a, b, c)
    for a, b, c, d in itertools.product(sites, repeat=4):
        if a != b and c != d:
            write_line(out, a, b, c, d)
    wide = lambda: tuple(rng.randint(-10**9, 10**9) for _ in range(2))
    dyadic = lambda: tuple(F(rng.randint(-512, 512), 8) for _ in range(2))
    for _ in range(rounds):
        a, b, c, d = wide(), wide(), wide(), wide()
        if a != b:
            write_ccw(out, a, b, c)
            if c != d:
                write_line(out, a, b, c, d)
            u = (b[0] - a[0], b[1] - a[1])
            g = math.gcd(*u)
            k = rng.choice((-1, 0, 1, g // 2, g, g + 1, -2 * g))
            e = (a[0] + k * (u[0] // g), a[1] + k * (u[1] // g))
            if max(map(abs, e)) <= 10**9:
                write_ccw(out, a, b, e)
        u = tuple(rng.randint(-3, 3) for _ in range(2))
        if u != (0, 0):
            a = tuple(rng.randint(-10**4, 10**4) for _ in range(2))
            b, c, d = [tuple(a[i] + rng.randint(-10, 10) * u[i] for i in range(2)) for _ in range(3)]
            if a != b:
                write_ccw(out, a, b, c)
                if c != d:
                    write_line(out, a, b, c, d)
                    write_line(out, a, b, (c[0] + u[1], c[1] - u[0]), (d[0] + u[1], d[1] - u[0]))
        small = [tuple(rng.randint(-6, 6) for _ in range(2)) for _ in range(4)]
        write_distance(out, (*small[:2], rng.randrange(3)), (*small[2:], rng.randrange(3)))
        a, b, c = dyadic(), dyadic(), dyadic()
        if a != b:
            r = [F(rng.randint(-8, 8), 2) for _ in range(3)]
            u = (b[0] - a[0], b[1] - a[1])
            write_line(out, a, b, c, dyadic(), True)
            if r[0]:
                write_line(out, a, b, c, (c[0] + r[0] * u[0], c[1] + r[0] * u[1]), True)
            if r[1] != r[2]:
                write_line(out, a, b, *((a[0] + x * u[0], a[1] + x * u[1]) for x in r[1:]), True)
        a, b, c, d = [tuple(F(rng.uniform(-1000, 1000)) for _ in range(2)) for _ in range(4)]
        u, v = (b[0] - a[0], b[1] - a[1]), (d[0] - c[0], d[1] - c[1])
        if dot(u, u) > 1 and dot(v, v) > 1 and (u[0] * v[1] - u[1] * v[0]) ** 2 > dot(u, u) * dot(v, v) / 100:
            write_line(out, a, b, c, d, True)


def lattice_brute(a, b):
    """Enumerate the bounding box; independent of gcd."""
    return sum(1 for x in range(min(a[0], b[0]), max(a[0], b[0]) + 1)
               for y in range(min(a[1], b[1]), max(a[1], b[1]) + 1)
               if (b[0] - a[0]) * (y - a[1]) == (b[1] - a[1]) * (x - a[0]))


def cells_brute(a, b):
    """Cells whose open interior meets the closed segment, by exact parameter intervals."""
    count = 0
    for i in range(min(a[0], b[0]) - 1, max(a[0], b[0]) + 1):
        for j in range(min(a[1], b[1]) - 1, max(a[1], b[1]) + 1):
            lo, hi = F(0), F(1)
            for c, p, q in ((i, a[0], b[0]), (j, a[1], b[1])):
                if p == q:
                    if not c < p < c + 1:
                        lo, hi = F(1), F(0)
                    continue
                x, y = sorted((F(c - p, q - p), F(c + 1 - p, q - p)))
                lo, hi = max(lo, x), min(hi, y)
            count += lo < hi
    return count


def write_grid(out, a, b):
    out.write('G ' + ' '.join(map(str, (*a, *b, lattice_brute(a, b), cells_brute(a, b)))) + '\n')


def unit(v):
    n = (Decimal(v[0]) ** 2 + Decimal(v[1]) ** 2).sqrt()
    return Decimal(v[0]) / n, Decimal(v[1]) / n


def write_construction(out, p, a, b, c):
    """Lines ab and pc, bisectors of ab and of angle a-p-b, direction p reflected across ab."""
    u, v = (b[0] - a[0], b[1] - a[1]), (c[0] - p[0], c[1] - p[1])
    if u[0] * v[1] - u[1] * v[0]:
        ld = Decimal(0)
    else:
        ld = abs(Decimal(u[0] * (p[1] - a[1]) - u[1] * (p[0] - a[0]))) / Decimal(u[0] ** 2 + u[1] ** 2).sqrt()
    mx, my = Decimal(a[0] + b[0]) / 2, Decimal(a[1] + b[1]) / 2
    x, y = (a[0] - p[0], a[1] - p[1]), (b[0] - p[0], b[1] - p[1])
    ux, uy = unit(x)
    vx, vy = unit(y)
    if x[0] * y[1] == x[1] * y[0] and x[0] * y[0] + x[1] * y[1] < 0:
        w = (-uy, ux)  # straight angle: counterclockwise normal of p->a
    else:
        w = unit((ux + vx, uy + vy)) if (ux + vx, uy + vy) != (0, 0) else (-uy, ux)
    k = F(p[0] * u[0] + p[1] * u[1], u[0] ** 2 + u[1] ** 2)
    rx, ry = decimal(2 * k * u[0] - p[0]), decimal(2 * k * u[1] - p[1])
    values = (*p, *a, *b, *c, ld, mx, my, mx - u[1], my + u[0], w[0], w[1], rx, ry)
    out.write('X ' + ' '.join(map(str, values)) + '\n')


def constructions(out, rng, sites, rounds):
    for p, a, b, c in itertools.product(sites, repeat=4):
        if a != b and a != p and b != p:
            write_construction(out, p, a, b, c)
    for a, b in itertools.product(sites, repeat=2):
        write_grid(out, a, b)
    for _ in range(rounds):
        a, b = [tuple(rng.randint(-12, 12) for _ in range(2)) for _ in range(2)]
        write_grid(out, a, b)
        p, a, b = [tuple(rng.randint(-10**9, 10**9) for _ in range(2)) for _ in range(3)]
        if a == b or a == p or b == p:
            continue
        k = rng.randint(-3, 3)
        c = (p[0] + k * (b[0] - a[0]), p[1] + k * (b[1] - a[1]))
        if max(map(abs, c)) <= 10**9:
            write_construction(out, p, a, b, c)
        write_construction(out, p, a, b, tuple(rng.randint(-10**9, 10**9) for _ in range(2)))
        o, s = tuple(rng.randint(-10**4, 10**4) for _ in range(2)), rng.randint(1, 10**4)
        d = tuple(rng.randint(-10**4, 10**4) for _ in range(2))
        if d != (0, 0):
            near = (o[0] - s * d[0] + rng.randint(-1, 1), o[1] - s * d[1] + rng.randint(-1, 1))
            if near != o:
                write_construction(out, o, (o[0] + d[0], o[1] + d[1]), near, o)


def equation_reference(a, b, c, bound=10**9):
    """Two lexicographically smallest lattice points of a x + b y = c with |coordinate| <= bound, else None.

    Particular solution by modular inverse, then the integer parameter range by exact floors.
    """
    g = math.gcd(a, b)
    if c % g:
        return None
    a, b, c = a // g, b // g, c // g
    x = c * pow(a, -1, abs(b)) % abs(b) if b else c * a
    y = (c - a * x) // b if b else 0
    step = (b, -a) if b > 0 or (b == 0 and a < 0) else (-b, a)
    lo, hi = -math.inf, math.inf
    for z, s in ((x, step[0]), (y, step[1])):
        if s:
            # -bound <= z + t s <= bound, both ends divided by s and swapped when s < 0.
            l, h = sorted((F(-bound - z, s), F(bound - z, s)))
            lo, hi = max(lo, math.ceil(l)), min(hi, math.floor(h))
        elif abs(z) > bound:
            return None
    if hi < lo + 1:
        return None
    return tuple((x + t * step[0], y + t * step[1]) for t in (lo, lo + 1))


def write_equation(out, a, b, c):
    r = equation_reference(a, b, c)
    p, q = r or ((0, 0), (0, 0))
    out.write('E ' + ' '.join(map(str, (a, b, c, int(r is not None), *p, *q))) + '\n')


def equations(out, rng, rounds):
    """Extreme lng triples, box-edge steps, scaled lines through box points, axis lines, random and huge triples."""
    lo, hi, B = -2**63, 2**63 - 1, 10**9
    fits = lambda *v: all(lo <= z <= hi for z in v)
    special = [lo, lo + 1, -2**62, -B - 1, -B, -1, 0, 1, B, B + 1, 2**62, hi]
    for a, b, c in itertools.product(special, repeat=3):
        if a or b:
            write_equation(out, a, b, c)
    # Steps of 2e9 and 2e9 + 1 fit 2 and 1 points; diagonals through corners fit 1 or 2.
    steps = [(1, 0), (0, 1), (1, 1), (1, -1), (2 * B, 1), (2 * B, -1), (2 * B + 1, 1), (2 * B - 1, 1), (1, 2 * B),
             (1, 2 * B + 1), (-1, 2 * B), (2 * B, 2 * B - 1), (2 * B - 1, 2 * B), (B, B + 1), (B + 1, -B)]
    starts = [(x, y) for x in (-B, -B + 1, 0, B - 1, B, B + 1) for y in (-B - 1, -B, 0, B)]
    for (dx, dy), (x, y) in itertools.product(steps, starts):
        for m in (1, -1, 3, -2**33):
            if fits(m * dy, m * dx, m * (dy * x - dx * y)):
                write_equation(out, m * dy, -m * dx, m * (dy * x - dx * y))
    for _ in range(rounds):
        a, b = rng.randint(-6, 6), rng.randint(-6, 6)
        if a or b:
            write_equation(out, a, b, rng.randint(-40, 40))
        g = rng.randint(1, 10**rng.randint(0, 17))
        a, b = g * rng.randint(-50, 50), g * rng.randint(-50, 50)
        k = rng.randint(-60 * B, 60 * B) if rng.random() < .5 else rng.randint(-50, 50)
        c = g * k + rng.choice((0, 0, rng.randint(0, g - 1)))
        if (a or b) and fits(c):
            write_equation(out, a, b, c)
        p, q = [tuple(rng.randint(-B, B) for _ in range(2)) for _ in range(2)]
        if p != q:
            a, b = q[1] - p[1], p[0] - q[0]
            c = a * p[0] + b * p[1]
            m = rng.randint(1, hi // max(abs(a), abs(b), abs(c))) * rng.choice((1, -1))
            write_equation(out, m * a, m * b, m * c)
            write_equation(out, a, b, c + rng.choice((-1, 1)))
        m = rng.choice((1, -1, 5, -B, 2**40, lo, hi))
        k = rng.choice((-B - 1, -B, -1, 0, 1, rng.randint(-B, B), B, B + 1))
        for a, b in ((m, 0), (0, m)):
            for c in (m * k, m * k + 1):
                if fits(c):
                    write_equation(out, a, b, c)
        a, b = rng.randint(lo, hi), rng.randint(lo, hi)
        if a or b:
            write_equation(out, a, b, rng.randint(lo, hi))


def fixtures(path, mode, seed):
    # Hand-worked oracle regressions, independent of generated C++ outputs.
    assert reference(((0, 0), (2, 2), 2), ((0, 1), (2, -1), 2)) == (1, (F(1, 2), F(1, 2)), (F(1, 2), F(1, 2)))
    assert reference(((0, 0), (1, 0), 1), ((2, 0), (1, 0), 1)) == (2, (0, 0), (2, 0))
    assert reference(((0, 0), (-1, 0), 1), ((2, 0), (3, 0), 1))[0] == 0
    assert reference(((0, 0), (0, 0), 2), ((0, 1), (0, 1), 2))[0] == 0
    assert reference(((0, 0), (1, 0), 0), ((1, 0), (2, 0), 0)) == (1, (1, 0), (1, 0))
    assert squared_distance((3, 7), (-5, -2), (10, 4)) == 29
    assert squared_distance((2, 1), (0, 0), (1, 0)) == 2
    assert (lattice_brute((0, 0), (4, 6)), cells_brute((0, 0), (4, 6))) == (3, 8)
    assert (lattice_brute((0, 0), (0, 3)), cells_brute((0, 0), (0, 3))) == (4, 0)
    assert (lattice_brute((1, 1), (1, 1)), cells_brute((1, 1), (1, 1))) == (1, 0)
    assert cells_brute((0, 0), (1, 1)) == 1 and cells_brute((0, 0), (2, 1)) == 2
    assert [ccw_reference((0, 0), (2, 0), c) for c in ((1, 1), (1, -1), (-1, 0), (3, 0), (0, 0), (1, 0), (2, 0))] == [1, -1, 2, -2, 0, 0, 0]
    assert ccw_reference((0, 0), (0, 2), (0, 5)) == -2
    assert line_reference((0, 0), (2, 2), (0, 2), (2, 0)) == (1, (1, 1))
    assert line_reference((0, 0), (1, 0), (5, 1), (7, 1))[0] == 0 and line_reference((0, 0), (1, 0), (7, 0), (5, 0))[0] == 4
    assert distance_reference(((0, 0), (2, 0), 0), ((1, 1), (1, 1), 0)) == 1
    assert distance_reference(((0, 0), (1, 0), 1), ((-1, 1), (-1, 1), 2)) == 2
    assert distance_reference(((0, 0), (1, 0), 0), ((3, 1), (4, 5), 0)) == 5
    assert distance_reference(((0, 1), (1, 0), 2), ((0, 0), (0, 0), 1)) == F(1, 2)
    assert distance_reference(((0, 0), (1, 0), 2), ((5, 3), (9, 3), 2)) == 9
    assert distance_reference(((0, 0), (1, 0), 1), ((-2, 1), (-3, 1), 1)) == 5
    assert distance_reference(((0, 0), (1, 0), 1), ((-2, 1), (-1, 1), 1)) == 1
    assert distance_reference(((0, 0), (1, 1), 2), ((3, 0), (4, 1), 2)) == F(9, 2)
    B = 10**9
    assert equation_reference(1, 0, B) == ((B, -B), (B, -B + 1)) and equation_reference(0, -2, 2 * B + 2) is None
    assert equation_reference(-1, 2 * B, B) == ((-B, 0), (B, 1)) and equation_reference(-1, 2 * B + 1, B) is None
    assert equation_reference(1, 1, 2 * B) is None and equation_reference(1, 1, 2 * B - 1) == ((B - 1, B), (B, B - 1))
    assert equation_reference(6, 4, 3) is None and equation_reference(-6, -4, -2) == ((-666666665, 999999998), (-666666663, 999999995))
    # Validates the oracle against enumeration in a small box.
    for a, b in itertools.product(range(-4, 5), repeat=2):
        for c in range(-9, 10):
            if a or b:
                pts = sorted((x, y) for x in range(-3, 4) for y in range(-3, 4) if a * x + b * y == c)
                assert equation_reference(a, b, c, 3) == (tuple(pts[:2]) if len(pts) > 1 else None), (a, b, c)
    rng = random.Random(seed)
    sites = [(0, 0), (-1, 0), (0, -1), (1, 0), (0, 1)]
    if mode != 'quick':
        sites += [(-1, -1), (-1, 1), (1, -1), (1, 1)]
    objects = list(itertools.product(sites, sites, range(3)))
    with path.open('w') as out, localcontext() as ctx:
        ctx.prec = 80
        for s, t in itertools.combinations_with_replacement(objects, 2):
            write_intersection(out, s, t)
            write_distance(out, s, t)
        rounds = {'quick': 80, 'full': 1500, 'stress': 12000}[mode]
        for i in range(rounds):
            a, b, c, d = [tuple(rng.randint(-10**9, 10**9) for _ in range(2)) for _ in range(4)]
            if i % 10 == 0:
                b = a
            if i % 11 == 0:
                d = c
            write_intersection(out, (a, b, i % 3), (c, d, (i // 3) % 3))
            write_metric(out, a, b, c, d)
            u = tuple(rng.randint(-50, 50) for _ in range(2))
            ends = []
            for _ in range(4):
                k = rng.randint(-50, 50)
                ends.append(tuple(k * x for x in u))
            write_distance(out, (a, b, i % 3), (c, d, (i // 3) % 3))
            write_intersection(out, (ends[0], ends[1], i % 3), (ends[2], ends[3], (i // 3) % 3))
            write_distance(out, (ends[0], ends[1], i % 3), (ends[2], ends[3], (i // 3) % 3))
        extreme = [(-10**9, -10**9), (-10**9, 10**9), (10**9, -10**9), (10**9, 10**9)]
        if mode != 'quick':
            extreme += [(999999999, 1000000000), (999999998, 999999999), (0, 0)]
        for a, b, c, d in itertools.product(extreme, repeat=4):
            for i, j in itertools.product(range(3), repeat=2):
                write_intersection(out, (a, b, i), (c, d, j))
                write_distance(out, (a, b, i), (c, d, j))
            if a != b:
                write_ccw(out, a, b, c)
                if c != d:
                    write_line(out, a, b, c, d)
            write_metric(out, a, b, c, d)
        write_intersection(out, ((0, 0), (1000000000, 999999999), 2), ((0, 1), (999999999, 999999999), 2))
        # Near-parallel and parallel extremal pairs; numerators reach 10^37.
        near = [((-10**9, -10**9), (10**9, 10**9 - 1), (-10**9, -10**9 + 1), (10**9, 10**9)),
                ((-10**9, -10**9), (10**9, 10**9 - 1), (-10**9 + 1, -10**9), (10**9, 10**9 - 2)),
                ((-10**9, 10**9), (10**9, -10**9), (-10**9, 10**9 - 1), (10**9 - 1, -10**9)),
                ((0, 0), (10**9, 1), (0, 1), (10**9, 1)), ((-10**9, 0), (10**9, 0), (10**9, 1), (-10**9, 1))]
        for a, b, c, d in near:
            for i, j in itertools.product(range(3), repeat=2):
                write_distance(out, (a, b, i), (c, d, j))
                write_distance(out, (b, a, i), (d, c, j))
            write_line(out, a, b, c, d)
        lines_and_points(out, rng, sites, {'quick': 80, 'full': 1500, 'stress': 12000}[mode])
        constructions(out, rng, sites, {'quick': 60, 'full': 600, 'stress': 4000}[mode])
        corners = [(-10**9, -10**9), (10**9, 10**9), (10**9, -10**9), (999999999, 10**9)]
        for p, a, b, c in itertools.product(corners, repeat=4):
            if a != b and a != p and b != p:
                write_construction(out, p, a, b, c)
        write_metric(out, (3, 7), (-5, -2), (10, 4), (8, 8))
        equations(out, rng, {'quick': 80, 'full': 1500, 'stress': 12000}[mode])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--mode', default=os.environ.get('CP_TEST_MODE', 'full'), choices=('quick', 'full', 'stress'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '20260927')))
    args, _ = parser.parse_known_args()
    with tempfile.TemporaryDirectory(prefix='cp-line-oracle-') as directory:
        path = Path(directory) / 'fixtures.txt'
        print(f'RUN independent Python Fraction/Decimal oracle mode={args.mode} seed={args.seed}', flush=True)
        fixtures(path, args.mode, args.seed)
        os.environ['CP_LINE_ORACLE'] = str(path)
        raise SystemExit(main('02-line_segment', [
            'zero-denominator', 'minimum-numerator', 'minimum-y', 'minimum-denominator',
            'large-coordinate', 'small-coordinate', 'invalid-kind', 'invalid-kind-negative', 'parallel-large', 'orthogonal-large', 'collinear-large', 'projection-singleton',
            'reflection-singleton', 'distance-singleton', 'metric-large-coordinate',
            'metric-infinity', 'metric-nan', 'metric-overflow', 'metric-underflow',
            'canonical-singleton', 'lattice-large', 'cells-large', 'line-distance-singleton',
            'line-distance-large', 'bisector-singleton', 'angle-bisector-singleton', 'reflect-direction-singleton',
            'ccw-singleton', 'ccw-large-coordinate', 'ccw-large-turn','line-intersection-singleton', 'line-intersection-second-singleton',
            'line-intersection-large', 'line-intersection-nan', 'line-intersection-underflow',
            'line-intersection-overflow', 'line-intersection-far', 'linear-distance-large', 'line-equation-zero',
        ]))
