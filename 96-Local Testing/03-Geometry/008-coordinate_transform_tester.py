#!/usr/bin/env python3
"""Fraction oracles and geometric invariants for approximate affine maps."""
from decimal import Decimal, localcontext
from fractions import Fraction
import os
from pathlib import Path
import random
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main


def option(name, fallback):
    return sys.argv[sys.argv.index(name) + 1] if name in sys.argv else fallback


def fixtures(path, mode, seed):
    rng, lines = random.Random(seed), random.Random(f'line-{seed}')  # lines: its own stream, the maps stay unchanged
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
            # applyDirection(p): the exact vector image over its 80-digit square-root length; 0 flags a zero image.
            vx, vy = apply(m, p[:2] + [0])[:2]
            length = (Decimal((vx * vx + vy * vy).numerator) / Decimal((vx * vx + vy * vy).denominator)).sqrt()
            values += [1, Decimal(vx.numerator) / vx.denominator / length,
                       Decimal(vy.numerator) / vy.denominator / length] if vx or vy else [0, 0, 0]
            values += apply(m, h[:2] + [1])[:2]  # applyLinear({p, (h.x, h.y)}) second endpoint
            # applyLine: the line through two integer points z0 != z1 with normal perp(z1 - z0), the exact images of
            # z0, z1 and z2 = z0 + normal (value |normal|^2 on the line's positive side), then the image line or 0.
            z0 = [lines.randint(-100, 100) for _ in range(2)]
            z1 = [lines.randint(-100, 100) for _ in range(2)]
            if z1 == z0:
                z1[0] += 1
            nx, ny = z0[1] - z1[1], z1[0] - z0[0]
            images = [apply(m, z + [1])[:2] for z in (z0, z1, [z0[0] + nx, z0[1] + ny])]
            values += [nx, ny, nx * z0[0] + ny * z0[1], *images[0], *images[1], *images[2]]
            values += [1, *image_line(images, nx * nx + ny * ny)] if det else [0, 0, 0, 0]
            values += [int(f == [Fraction(x) for x in g])]
            out.write(' '.join(format_value(x) for x in values) + '\n')


def image_line(images, side):
    """The line through the first two images, scaled so that the third image has value side (Fraction)."""
    (w0, w1, w2), F = images, Fraction
    n = (F(w0[1] - w1[1]), F(w1[0] - w0[0]))
    k = F(side) / (n[0] * (w2[0] - w0[0]) + n[1] * (w2[1] - w0[1]))
    return k * n[0], k * n[1], k * (n[0] * w0[0] + n[1] * w0[1])


# Largest numerator or denominator Rational<lng> ('L') and Rational<lll> ('X') hold; 'I' is Rational<InfInt>.
LIMIT = {'L': (1 << 63) - 1, 'X': (1 << 127) - 1}


class Watch:
    """A Fraction recording the largest numerator or denominator any result reaches.

    The m_ functions mirror the header's sequence of R operations only to admit a record to a bounded R (every
    intermediate fits); the expected values come from the independent pointwise matrix oracle below.
    """
    peak = 0
    __slots__ = ('v',)

    def __init__(self, v):
        self.v = v = Fraction(v)
        Watch.peak = max(Watch.peak, abs(v.numerator), v.denominator)

    def __add__(self, o):
        return Watch(self.v + o.v)

    def __sub__(self, o):
        return Watch(self.v - o.v)

    def __mul__(self, o):
        return Watch(self.v * o.v)

    def __truediv__(self, o):
        return Watch(self.v / o.v)

    def __neg__(self):
        return Watch(-self.v)


def m_vector(f, v):
    return f[0] * v[0] + f[1] * v[1], f[2] * v[0] + f[3] * v[1]


def m_point(f, p):
    v = m_vector(f, p)
    return v[0] + f[4], v[1] + f[5]


def m_fixing(f, p, u):
    v = m_vector(f, p)
    return (*f[:4], u[0] - v[0], u[1] - v[1])


def m_mul(f, g):
    t = m_point(f, (g[4], g[5]))
    return (f[0] * g[0] + f[1] * g[2], f[0] * g[1] + f[1] * g[3], f[2] * g[0] + f[3] * g[2], f[2] * g[1] + f[3] * g[3], *t)


def m_det(f):
    return f[0] * f[3] - f[1] * f[2]


def m_inverse(f):
    det, zero = m_det(f), Watch(0)
    return None if det.v == 0 else m_fixing((f[3] / det, -f[1] / det, -f[2] / det, f[0] / det, zero, zero), f[4:], (zero, zero))


def m_build(op, w):
    one, zero = Watch(1), Watch(0)
    if op == 'id':
        return one, zero, zero, one, zero, zero
    if op == 'raw':
        return tuple(w)
    if op == 'tr':
        return one, zero, zero, one, w[0], w[1]
    if op in ('sc', 'scc'):
        c = w[2:] or (zero, zero)
        return w[0], zero, zero, w[1], c[0] * (one - w[0]), c[1] * (one - w[1])
    if op in ('ro', 'roc'):
        co, si, c = w[0], w[1], w[2:] or (zero, zero)
        co * co + si * si
        return m_fixing((co, -si, si, co, zero, zero), c, c)
    p, q = w[0:2], w[2:4]
    x, y = q[0] - p[0], q[1] - p[1]
    n = x * x + y * y
    if op in ('pr', 're'):
        f = m_fixing((x * x / n, x * y / n, x * y / n, y * y / n, zero, zero), p, p)
        return f if op == 'pr' else (f[0] + f[0] - one, f[1] + f[1], f[2] + f[2], f[3] + f[3] - one, f[4] + f[4], f[5] + f[5])
    u, v = w[4:6], w[6:8]
    z, t = v[0] - u[0], v[1] - u[1]
    co, si = (x * z + y * t) / n, (x * t - y * z) / n
    return m_fixing((co, -si, si, co, zero, zero), p, u)


def image(op, a, z):
    """The image of point z under the map op(a), straight from the definitions (complex numbers as pairs)."""
    def mul(x, y):
        return x[0] * y[0] - x[1] * y[1], x[0] * y[1] + x[1] * y[0]

    def sub(x, y):
        return x[0] - y[0], x[1] - y[1]

    def add(x, y):
        return x[0] + y[0], x[1] + y[1]
    if op == 'id':
        return z
    if op == 'raw':
        return a[0] * z[0] + a[1] * z[1] + a[4], a[2] * z[0] + a[3] * z[1] + a[5]
    if op == 'tr':
        return add(z, a)
    c = a[2:4] or (0, 0)
    if op in ('sc', 'scc'):
        return c[0] + a[0] * (z[0] - c[0]), c[1] + a[1] * (z[1] - c[1])
    if op in ('ro', 'roc'):
        return add(c, mul(a[0:2], sub(z, c)))
    p, q = a[0:2], a[2:4]
    if op in ('pr', 're'):
        # Implicit line n.z = n.p with normal n; the foot is z minus its signed offset along n.
        n = (p[1] - q[1], q[0] - p[0])
        s = (n[0] * (z[0] - p[0]) + n[1] * (z[1] - p[1])) / (n[0] ** 2 + n[1] ** 2)
        foot = (z[0] - s * n[0], z[1] - s * n[1])
        return foot if op == 'pr' else sub(add(foot, foot), z)
    u, v, d = a[4:6], a[6:8], sub(q, p)
    norm = d[0] ** 2 + d[1] ** 2
    ratio = mul(sub(v, u), (d[0] / norm, -d[1] / norm))
    return add(u, mul(sub(z, p), ratio))


def from_images(f):
    o, e, g = f((0, 0)), f((1, 0)), f((0, 1))
    return [[e[0] - o[0], g[0] - o[0], o[0]], [e[1] - o[1], g[1] - o[1], o[1]], [Fraction(0), Fraction(0), Fraction(1)]]


def mmul(f, g):
    return [[sum(f[i][k] * g[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def happly(f, z, w):
    return tuple(f[i][0] * z[0] + f[i][1] * z[1] + f[i][2] * w for i in range(2))


def coefficients(f):
    return [f[0][0], f[0][1], f[1][0], f[1][1], f[0][2], f[1][2]]


def solve(m):
    """Gauss-Jordan on [m | I]: the determinant and the inverse, or None when singular."""
    rows = [[Fraction(x) for x in r] + [Fraction(int(i == j)) for j in range(3)] for i, r in enumerate(m)]
    det = Fraction(1)
    for col in range(3):
        pivot = next((r for r in range(col, 3) if rows[r][col]), None)
        if pivot is None:
            return Fraction(0), None
        if pivot != col:
            rows[col], rows[pivot], det = rows[pivot], rows[col], -det
        pv = rows[col][col]
        det *= pv
        rows[col] = [x / pv for x in rows[col]]
        for r in range(3):
            if r != col and rows[r][col]:
                k = rows[r][col]
                rows[r] = [x - k * y for x, y in zip(rows[r], rows[col])]
    return det, [r[3:] for r in rows]


def solve_any(m, t):
    """Some solution of the possibly singular 2x2 system m x = t (free unknowns 0), or None when it has none."""
    rows, pivots = [[Fraction(m[i][0]), Fraction(m[i][1]), Fraction(t[i])] for i in range(2)], []
    for col in range(2):
        k = next((i for i in range(len(pivots), 2) if rows[i][col]), None)
        if k is None:
            continue
        r = len(pivots)
        rows[r], rows[k] = rows[k], rows[r]
        rows[r] = [x / rows[r][col] for x in rows[r]]
        for i in range(2):
            if i != r and rows[i][col]:
                rows[i] = [x - rows[i][col] * y for x, y in zip(rows[i], rows[r])]
        pivots.append(col)
    if any(rows[i][2] for i in range(len(pivots), 2)):
        return None
    x = [Fraction(0), Fraction(0)]
    for i, col in enumerate(pivots):
        x[col] = rows[i][2]
    return x


def members(M):
    """isTranslation ... isSimilarity of the 3x3 map M, decided constructively: solve for the factory's arguments, then
    rebuild the map pointwise from image() and compare. The linear part of each factory is fixed by its arguments."""
    (a, b, tx), (c, d, ty) = M[0], M[1]

    def same(op, args):
        return from_images(lambda z: image(op, args, z)) == M

    def fixed():  # a point p with M p = p, that is (I - L) p = t
        return solve_any(((1 - a, -b), (-c, 1 - d)), (tx, ty))

    def column(m):  # the line direction of a rank-one v v^T / |v|^2: any nonzero column
        return next(((m[0][j], m[1][j]) for j in range(2) if m[0][j] or m[1][j]), None)

    def axis(k, t):  # center coordinate x with (1 - k) x = t
        return t / (1 - k) if k != 1 else Fraction(0) if t == 0 else None
    res = [same('tr', [tx, ty])]
    center = [axis(a, tx), axis(d, ty)]
    res.append(None not in center and same('scc', [a, d, *center]))
    p = fixed()
    res.append(a * a + c * c == 1 and p is not None and same('roc', [a, c, *p]))
    for m, op in (([[a, b], [c, d]], 'pr'), ([[(a + 1) / 2, b / 2], [c / 2, (d + 1) / 2]], 're')):
        v = column(m)
        res.append(v is not None and p is not None and same(op, [*p, p[0] + v[0], p[1] + v[1]]))
    res.append(same('si', [*map(Fraction, (0, 0, 1, 0)), tx, ty, tx + a, ty + c]))
    return res


def m_predicates(f):
    """Per predicate, the largest numerator or denominator its R operations reach, short-circuited as in the header."""
    def run(k):
        Watch.peak = 0
        a, b, c, d, tx, ty, one = *f, Watch(1)
        if k == 2 and a.v == d.v and b.v == (-c).v:
            a * a + c * c
        if k == 3 and b.v == c.v and (a + d).v == 1 and (a * d).v == (b * b).v and (a * tx + b * ty).v == 0:
            c * tx + d * ty
        if k == 4 and a.v == (-d).v and b.v == c.v and (a * a + b * b).v == 1 and ((a + one) * tx + b * ty).v == 0:
            c * tx + (d + one) * ty
        if k == 5:
            -c
        return Watch.peak
    return [run(k) for k in range(6)]


def m_line(f, inv, l, p):
    """applyLine's R operations, then the value of p on the line and of its image on the image line."""
    x, y = inv[0] * l[0] + inv[2] * l[1], inv[1] * l[0] + inv[3] * l[1]
    k = l[2] + x * f[4] + y * f[5]
    q = m_point(f, p)
    x * q[0] + y * q[1] - k, l[0] * p[0] + l[1] * p[1] - l[2]


def pythagorean(m, n, sign=(1, 1), swap=False):
    h = m * m + n * n
    co, si = Fraction(m * m - n * n, h), Fraction(2 * m * n, h)
    co, si = (si, co) if swap else (co, si)
    return [co * sign[0], si * sign[1]]


def exact_record(tag, f, g, p, u, v, strict):
    """One fixture line: the narrowest type from tag up whose bounded Rational holds every value, then the inputs and
    the Fraction expectations."""
    Watch.peak = 0

    def w(xs):
        return [Watch(x) for x in xs]
    mf, mg, wp, wu, wv = m_build(f[0], w(f[1])), m_build(g[0], w(g[1])), w(p), w(u), w(v)
    m_point(m_build('id', []), wp)
    m_vector(mf, wv)
    m_point(m_fixing(mf, wp, wu), wp)
    m_point(m_mul(mf, mg), wp)
    m_mul(mg, mf)
    m_point(mf, m_point(mg, wp))
    inv = m_inverse(mf)
    if inv:
        m_mul(mf, inv), m_mul(inv, mf)
    if f[0] in ('pr', 're'):
        m_mul(mf, mf)
    if f[0] == 'roc':
        m_point(mf, w(f[1][2:]))
    if f[0] == 'si':
        m_point(mf, w(f[1][0:2])), m_point(mf, w(f[1][2:4]))
    F, G = (from_images(lambda z, h=h: image(*h, z)) for h in (f, g))
    det, inverse = solve(F)
    fix = from_images(lambda z: tuple(x + y for x, y in zip(happly(F, (z[0] - p[0], z[1] - p[1]), 0), u)))
    values = [*coefficients(F), *coefficients(G), *happly(F, p, 1), *happly(F, v, 0), det, *coefficients(fix),
              *coefficients(mmul(F, G)), *coefficients(mmul(G, F)), *happly(mmul(F, G), p, 1)]
    values += [1, *coefficients(inverse)] if inverse else [0] * 7
    # applyLine's input: the line through u and v with normal perp(v - u), or x = u.x when u == v.
    a, b = (u[1] - v[1], v[0] - u[0]) if u != v else (Fraction(1), Fraction(0))
    line = [a, b, a * u[0] + b * u[1]]
    w(values + line)
    order = 'LXI'
    final = next(t for t in order[order.index(tag):] if t == 'I' or Watch.peak <= LIMIT[t])
    assert not strict or final == tag, (tag, final, f, g, Watch.peak.bit_length())
    peak = Watch.peak

    def fits(run):  # the new operations run only where their R operations fit the record's type: flag 2 skips
        Watch.peak = 0
        run()
        return final == 'I' or Watch.peak <= LIMIT[final]
    extra = list(line)
    if inverse is None:
        extra += [0, 0, 0, 0]
    elif fits(lambda: m_line(mf, inv, w(line), wp)):
        z = (line[2] / a, Fraction(0)) if a else (Fraction(0), line[2] / b)
        zs = (z, (z[0] - b, z[1] + a), (z[0] + a, z[1] + b))
        extra += [1, *image_line([happly(F, q, 1) for q in zs], a * a + b * b)]
    else:
        extra += [2, 0, 0, 0]
    extra += [1, *happly(F, u, 1)] if fits(lambda: m_point(mf, wu)) else [2, 0, 0]
    extra += [int(coefficients(F) == coefficients(G))]
    for M, m in ((F, mf), (G, mg), (mmul(F, G), m_mul(mf, mg)), (mmul(G, F), m_mul(mg, mf))):
        extra += [int(x) if final == 'I' or k <= LIMIT[final] else 2 for x, k in zip(members(M), m_predicates(m))]
    return final, peak, ' '.join([final, f[0], *map(str, f[1]), g[0], *map(str, g[1]),
                                  *map(str, (*p, *u, *v)), *map(str, values), *map(str, extra)])


def exact_fixtures(path, mode, seed):
    rng = random.Random(f'exact-{seed}')  # its own stream: the approximate fixtures above stay unchanged
    # Numerator bits, denominator bits and Pythagorean parameter bits per size class.
    sizes = {'small': (5, 2, 3), 'medium': (20, 8, 10), 'large': (50, 25, 30), 'huge': (180, 140, 80)}
    F = Fraction

    def generate(size):
        nb, db, pb = sizes[size]

        def val():
            num = 0 if rng.random() < 0.1 else rng.randint(-(1 << nb), 1 << nb)
            return F(num, 1 if rng.random() < 0.3 else rng.randint(1, 1 << db))

        def pts(k):
            return [val() for _ in range(2 * k)]

        def distinct():
            a = pts(2)
            if a[:2] == a[2:]:
                a[2] += 1
            return a
        op = rng.choice(('id', 'raw', 'raw', 'tr', 'sc', 'scc', 'ro', 'roc', 'pr', 're', 'si', 'si'))
        if op == 'raw':
            a, k = pts(3), rng.random()
            if k < 0.2:
                m = val()
                a[2], a[3] = a[0] * m, a[1] * m  # rank at most one
            elif k < 0.3:
                a[:4] = [F(0)] * 4  # rank zero
            return op, a
        if op == 'tr':
            return op, pts(1)
        if op in ('sc', 'scc'):
            s = pts(1)
            if rng.random() < 0.3:
                s[rng.randrange(2)] = F(0)
            if rng.random() < 0.1:
                s = [F(0), F(0)]
            return op, s + (pts(1) if op == 'scc' else [])
        if op in ('ro', 'roc'):
            if rng.random() < 0.2:
                unit = list(map(F, rng.choice(((1, 0), (0, 1), (-1, 0), (0, -1)))))
            else:
                m = rng.randint(1, 1 << pb)
                unit = pythagorean(m, rng.randint(0, m), (rng.choice((-1, 1)), rng.choice((-1, 1))), rng.random() < 0.5)
            return op, unit + (pts(1) if op == 'roc' else [])
        if op in ('pr', 're'):
            return op, distinct()
        if op == 'si':
            a, u = distinct(), pts(1)
            return op, a + u + (list(u) if rng.random() < 0.2 else pts(1))
        return op, []

    big, odd = 2 ** 126 - 1, F(3 ** 120, 2 ** 150 + 1)
    hp = [F(2 ** 160 + 3), F(-(3 ** 101)), F(5 ** 70, 7), F(2 ** 150 - 1)]
    fixed = [
        ('X', ('tr', [F(big), F(-(big - 2))]), ('id', []), [F(5), F(-7)], [F(1), F(2)], [F(3), F(4)]),
        ('X', ('ro', pythagorean(2 ** 30 - 35, 2 ** 29 + 17)), ('roc', pythagorean(2 ** 30 - 3, 2 ** 28 + 1) + [F(2), F(-3)]),
         [F(7), F(-5)], [F(1), F(1)], [F(-2), F(9)]),
        ('X', ('scc', [F(big), F(1, 2 ** 125), F(1), F(-1)]), ('tr', [F(0), F(1, 3)]), [F(1), F(1)], [F(0), F(1)], [F(1), F(0)]),
        ('I', ('raw', [odd, F(2 ** 130 + 1), F(-(3 ** 90)), odd * 7, F(1, 2 ** 140), F(-(5 ** 60))]),
         ('raw', [F(2 ** 129), F(1), F(-1), odd, F(11), F(-13, 2 ** 135)]), [F(3), F(-1, 2)], [F(1, 3), F(5)], [F(2 ** 200), F(-1)]),
        ('I', ('roc', pythagorean(2 ** 90 + 1, 2 ** 89 + 7, (-1, 1)) + hp[:2]), ('si', hp + [F(1), F(2), F(-(2 ** 140)), F(3)]),
         hp[2:], [F(1), F(2 ** 131)], [F(-1), F(1)]),
        ('I', ('pr', hp), ('re', hp[1:] + [F(1)]), hp[:2], hp[2:], [F(1), F(-1)]),
    ]
    for tag in 'LXI':  # rank-zero and rank-one maps in every type, composed with invertible ones
        fixed += [
            (tag, ('scc', [F(0), F(0), F(2), F(-3)]), ('ro', [F(3, 5), F(4, 5)]), [F(1), F(2)], [F(5), F(6)], [F(-1), F(4)]),
            (tag, ('sc', [F(0), F(7, 2)]), ('tr', [F(1), F(-1)]), [F(3), F(-2)], [F(0), F(0)], [F(1), F(1)]),
            (tag, ('pr', [F(1), F(2), F(4), F(6)]), ('sc', [F(2), F(-3)]), [F(-7), F(5)], [F(1), F(0)], [F(2), F(9)]),
            (tag, ('re', [F(1), F(2), F(4), F(6)]), ('pr', [F(0), F(0), F(1), F(1)]), [F(-7), F(5)], [F(1), F(0)], [F(2), F(9)]),
            (tag, ('si', [F(0), F(0), F(1), F(1), F(3), F(3), F(3), F(3)]), ('id', []), [F(2), F(5)], [F(1), F(1)], [F(1), F(0)]),
            (tag, ('raw', [F(2), F(4), F(1), F(2), F(5), F(6)]), ('roc', [F(-4, 5), F(3, 5), F(1), F(1)]), [F(1), F(1)], [F(0), F(2)], [F(3), F(1)]),
            (tag, ('raw', [F(0), F(0), F(0), F(0), F(1), F(-1)]), ('si', [F(0), F(0), F(2), F(0), F(1), F(1), F(1), F(3)]),
             [F(4), F(4)], [F(1), F(1)], [F(1), F(2)]),
        ]
    def raw(*maps):  # the coefficients of the composition of maps, the last applied first
        res = from_images(lambda z: z)
        for h in maps:
            res = mmul(res, from_images(lambda z, h=h: image(*h, z)))
        return 'raw', coefficients(res)
    P, Q, half = [F(1), F(2), F(4), F(6)], [F(3, 2), F(2)], F(1, 2)
    special = [  # predicate boundaries: each pair is f and g, so f * g and g * f are checked too
        (('raw', [F(1), F(0), F(0), F(1), F(0), F(0)]), ('tr', [F(3), F(-2)])),  # identity, pure translation
        (('raw', [F(0)] * 6), ('raw', [F(0), F(0), F(0), F(0), F(2), F(-3)])),  # zero and constant maps
        (('raw', [F(1), F(0), F(0), F(3), F(5), F(0)]), ('raw', [F(2), F(0), F(0), F(1), F(0), F(7)])),  # factor 1, t != 0
        (('raw', [F(2), F(0), F(0), F(1), F(4), F(0)]), ('raw', [F(1), F(0), F(0), F(-1), F(0), F(0)])),  # axis scaling, x-axis reflection
        (raw(('tr', Q), ('re', P)), raw(('tr', Q), ('pr', P))),  # glide reflection, projection moved along its line
        (raw(('tr', [F(-4), F(3)]), ('pr', P)), raw(('tr', [F(-8), F(6)]), ('re', P))),  # moved along the normal: still members
        (('raw', [F(1), F(1), F(0), F(0), F(0), F(0)]), ('raw', [F(1), F(0), F(1), F(0), F(0), F(5)])),  # oblique idempotents
        # Axis-parallel lines moved along themselves: only one of the two fixed-line equations fails.
        (('raw', [F(1), F(0), F(0), F(0), F(5), F(0)]), ('raw', [F(0), F(0), F(0), F(1), F(0), F(5)])),
        (('raw', [F(-1), F(0), F(0), F(1), F(0), F(3)]), ('raw', [F(1), F(0), F(0), F(-1), F(3), F(0)])),
        (('raw', [F(-1), F(0), F(0), F(1), F(4), F(0)]), ('raw', [F(0), F(0), F(0), F(1), F(2), F(0)])),  # members
        (('raw', [half, half, half, half, F(1), F(-1)]), ('raw', [half, F(0), F(0), half, F(0), F(0)])),  # trace 1: rank 1 and 2
        (('raw', [F(-1), F(0), F(0), F(-1), F(4), F(6)]), ('raw', [F(0), F(-1), F(1), F(0), F(5), F(5)])),  # half and quarter turns
        (('raw', [F(3, 5), F(4, 5), F(4, 5), F(-3, 5), F(1), F(2)]), ('raw', [F(3, 5), F(4, 5), F(4, 5), F(-3, 5), F(-1, 5), F(2, 5)])),
        (('raw', [F(2), F(0), F(0), F(-2), F(0), F(0)]), ('raw', [F(1), F(-1), F(1), F(1), F(3), F(4)])),  # non-unit forms
        (('raw', [F(2), F(0), F(0), F(2), F(1), F(1)]), ('raw', [F(1), F(1), F(1), F(1), F(0), F(0)])),
        (('re', [F(0), F(0), F(1), F(0)]), ('re', [F(0), F(0), F(1), F(1)])),  # reflections composing to a rotation
        (('re', [F(0), F(0), F(1), F(0)]), ('re', [F(0), F(1), F(1), F(1)])),  # and to a translation
    ]
    for tag in 'LXI':
        fixed += [(tag, f, g, [F(1), F(1)], [F(0), F(2)], [F(3), F(1)]) for f, g in special]
    intent = {'small': 'LXI', 'medium': 'LX', 'large': 'X', 'huge': 'I'}
    peaks = {'L': 0, 'X': 0, 'I': 0}
    with path.open('w') as out:
        for item in fixed:
            tag, peak, line = exact_record(*item, strict=True)
            peaks[tag] = max(peaks[tag], peak)
            out.write(line + '\n')
        for _ in range({'quick': 120, 'full': 3000, 'stress': 12000}[mode]):
            size = rng.choice(('small', 'small', 'medium', 'large', 'huge'))
            f, g = generate(size), generate(size)
            p, u, v = ([F(rng.randint(-50, 50), rng.randint(1, 9)) for _ in range(2)] for _ in range(3))
            tag, peak, line = exact_record(rng.choice(intent[size]), f, g, p, u, v, strict=False)
            peaks[tag] = max(peaks[tag], peak)
            out.write(line + '\n')
    # Huge-but-safe Rational<lll> records reach 2^120; InfInt records pass 2^127.
    assert peaks['X'] >= 1 << 120 and peaks['I'] > LIMIT['X'], {t: x.bit_length() for t, x in peaks.items()}


if __name__ == '__main__':
    mode = option('--mode', os.environ.get('CP_TEST_MODE', 'full'))
    seed = int(option('--seed', os.environ.get('CP_TEST_SEED', '20260927')))
    with tempfile.TemporaryDirectory(prefix='cp-transform-oracle-') as folder:
        path = Path(folder) / 'fraction.txt'
        fixtures(path, mode, seed)
        os.environ['CP_TRANSFORM_ORACLE'] = str(path)
        exact = Path(folder) / 'exact.txt'
        exact_fixtures(exact, mode, seed)
        os.environ['CP_TRANSFORM_EXACT'] = str(exact)
        raise SystemExit(main('008-coordinate_transform', ['rotation-nan', 'projection-point',
                                                         'reflection-point', 'similarity-point', 'rotate-approx-nan',
                                                         'exact-rotation-nonunit', 'exact-rotation-over', 'projection-infinite', 'similarity-infinite', 'exact-projection-point',
                                                         'exact-reflection-point', 'exact-similarity-point',
                                                         'exact-overflow', 'direction-zero']))
