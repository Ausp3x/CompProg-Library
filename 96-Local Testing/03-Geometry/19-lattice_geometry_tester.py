#!/usr/bin/env python3
"""Lattice points under segments, in rational and halfplane polygons and in rational-centre disks; quick/full/stress.

The Python oracle sends rational segments and convex polygons with denominators up to 10^15 (domain V * D <= 4 * 10^18)
and checks them with exact Fraction column scans.
"""
from fractions import Fraction
import math
import random
import shlex
import subprocess
from _00_runner import main


def under(p, q):
    """Sum of floor(l(x)) over integers x in the closed x-range of the segment p q."""
    if q[0] < p[0]:
        p, q = q, p
    res = 0
    for x in range(math.ceil(p[0]), math.floor(q[0]) + 1):
        res += math.floor(p[1] + (q[1] - p[1]) * (x - p[0]) / (q[0] - p[0]))
    return res


def polygon(v, closed):
    """Lattice points of a convex CCW polygon by columns: closed, or strictly inside."""
    xs = [p[0] for p in v]
    lo, hi = min(xs), max(xs)
    res = 0
    for x in range(math.ceil(lo), math.floor(hi) + 1):
        ys = []
        for i in range(len(v)):
            p, q = v[i], v[(i + 1) % len(v)]
            if p[0] == q[0]:
                if p[0] == x:
                    ys += [p[1], q[1]]
            elif min(p[0], q[0]) <= x <= max(p[0], q[0]):
                ys.append(p[1] + (q[1] - p[1]) * (x - p[0]) / (q[0] - p[0]))
        a, b = min(ys), max(ys)
        if closed:
            res += math.floor(b) - math.ceil(a) + 1
        elif lo < x < hi:
            res += max(0, math.ceil(b) - math.floor(a) - 1)
    return res


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 40, 'full': 400, 'stress': 4000}[args.mode]
    rows, want = [], []
    for i in range(count):
        den = [rng.randint(1, 10 ** rng.randint(1, 15)) for _ in range(2)]
        vbound = 4 * 10 ** 18 // max(den)
        span = min(vbound, 1000)
        pts = [(Fraction(rng.randint(-span * d, span * d), d), Fraction(rng.randint(-span * d, span * d), d)) for d in den]
        if pts[0][0] == pts[1][0]:
            continue
        rows.append('U ' + ' '.join(f'{p[0].numerator * (p[1].denominator // math.gcd(p[0].denominator, p[1].denominator))} '
                                      f'{p[1].numerator * (p[0].denominator // math.gcd(p[0].denominator, p[1].denominator))} '
                                      f'{p[0].denominator * p[1].denominator // math.gcd(p[0].denominator, p[1].denominator)}' for p in pts))
        want.append(str(under(*pts)))
    for i in range(count // 4):
        k = rng.randint(3, 6)
        D = 10 ** rng.randint(1, 14)
        R = min(4 * 10 ** 18 // D, 20000) // 2
        angles = sorted(rng.random() * 2 * math.pi for _ in range(k))
        v = []
        for a in angles:
            d = rng.randint(1, D)
            v.append((Fraction(round(R * math.cos(a) * d), d), Fraction(round(R * math.sin(a) * d), d)))
        convex = all((v[(j + 1) % k][0] - v[j][0]) * (v[(j + 2) % k][1] - v[j][1]) - (v[(j + 1) % k][1] - v[j][1]) * (v[(j + 2) % k][0] - v[j][0]) > 0
                     for j in range(k))
        if not convex:
            continue
        for closed in (0, 1):
            body = []
            for p in v:
                l = p[0].denominator * p[1].denominator // math.gcd(p[0].denominator, p[1].denominator)
                body.append(f'{p[0].numerator * (l // p[0].denominator)} {p[1].numerator * (l // p[1].denominator)} {l}')
            rows.append(f'P {closed} {k} ' + ' '.join(body))
            want.append(str(polygon(v, closed)))
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input=''.join(r + '\n' for r in rows), text=True, capture_output=True, env=env, timeout=600)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    got = result.stdout.splitlines()
    if len(got) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(got)} command={shlex.join(command)}')
    for row, a, b in zip(rows, want, got):
        if a != b:
            raise RuntimeError(f'Python Fraction oracle seed={args.seed} row={row}\nexpected={a}\nactual={b}\ncommand={shlex.join(command)}')
    print(f'PASS Python Fraction oracle cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('19-lattice_geometry', ['disk-q', 'disk-n', 'under-vertical', 'polygon-small', 'polygon-flat', 'halfplane-far'], oracle))
