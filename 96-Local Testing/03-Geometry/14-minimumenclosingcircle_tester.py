#!/usr/bin/env python3
"""Minimum enclosing circle: exact support, centre, radius2 and locate against brute force and the optimality criterion; quick/full/stress.

The Python oracle brute-forces the exact minimum circle with Fractions over every 1-, 2- and 3-point candidate (textbook circumcentre
formula): integer sets up to 10^9, including cocircular lattice sets, against radius2 and center; arbitrary doubles against
minimumEnclosingCircleApprox within 1e-12 of the coordinate scale.
"""
from fractions import Fraction
import random
import shlex
import subprocess
from _00_runner import main

CIRCLE = [(x, y) for x in range(-1105, 1106) for y in range(-1105, 1106) if x * x + y * y == 1105 * 1105]


def brute(pts):
    """Exact minimum enclosing circle (centre, squared radius) of Fraction points."""
    cands = [(p, Fraction(0)) for p in pts]
    for i in range(len(pts)):
        for j in range(i + 1, len(pts)):
            (ax, ay), (bx, by) = pts[i], pts[j]
            cands.append((((ax + bx) / 2, (ay + by) / 2), ((ax - bx) ** 2 + (ay - by) ** 2) / 4))
            for k in range(j + 1, len(pts)):
                cx, cy = pts[k]
                d = 2 * (ax * (by - cy) + bx * (cy - ay) + cx * (ay - by))
                if d == 0:
                    continue
                a2, b2, c2 = ax * ax + ay * ay, bx * bx + by * by, cx * cx + cy * cy
                ux = (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / d
                uy = (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / d
                cands.append(((ux, uy), (ax - ux) ** 2 + (ay - uy) ** 2))
    best = None
    for (ux, uy), r2 in cands:
        if all((x - ux) ** 2 + (y - uy) ** 2 <= r2 for x, y in pts) and (best is None or r2 < best[1]):
            best = ((ux, uy), r2)
    return best


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 40, 'full': 400, 'stress': 4000}[args.mode]
    rows, want = [], []
    for i in range(count):
        n = rng.randint(1, 8)
        if i % 4 == 3:
            pts = [(900000 * x, 900000 * y) for x, y in rng.sample(CIRCLE, min(n, len(CIRCLE)))]
        else:
            s = rng.choice([3, 1000, 10 ** 9])
            pts = [(rng.randint(-s, s), rng.randint(-s, s)) for _ in range(n)]
            if i % 5 == 0:
                pts.append(pts[0])
        rows.append(('E', pts))
    for i in range(count):
        s = rng.choice([1e-3, 1.0, 1e3, 1e9, 1e15])
        n = rng.randint(1, 10)
        pts = [(rng.uniform(-s, s), rng.uniform(-s, s)) if i % 3 else (float(rng.randint(-1000, 1000)), float(rng.randint(-1000, 1000)))
               for _ in range(n)]
        rows.append(('F', pts))
    text = ''.join(f'{k} {len(p)} ' + ' '.join(f'{x!r} {y!r}' for x, y in p) + '\n' for k, p in rows)
    command = [str(binary), '--seed', str(args.seed), '--oracle']
    result = subprocess.run(command, input=text, text=True, capture_output=True, env=env, timeout=600)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    got = result.stdout.splitlines()
    if len(got) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(got)} command={shlex.join(command)}')
    worst = Fraction(0)
    for (kind, pts), line in zip(rows, got):
        f = [(Fraction(x), Fraction(y)) for x, y in pts]
        (ux, uy), r2 = brute(f)
        out = line.split()
        if kind == 'E':
            ok = Fraction(out[0]) == r2 and Fraction(int(out[1]), int(out[3])) == ux and Fraction(int(out[2]), int(out[3])) == uy
        else:
            scale = max(abs(c) for p in f for c in p)
            tol = scale / 10 ** 12
            cx, cy, r = (Fraction(v) for v in out)
            err = max(abs(cx - ux), abs(cy - uy))
            lo = max(Fraction(0), r - tol)
            ok = err <= tol and lo * lo <= r2 <= (r + tol) ** 2
            if scale:
                worst = max(worst, err / scale)
        if not ok:
            raise RuntimeError(f'Python Fraction oracle seed={args.seed} row={kind} {pts}\nexpected centre=({ux}, {uy}) r2={r2}\n'
                               f'actual={line}\ncommand={shlex.join(command)}')
    print(f'PASS Python Fraction oracle cases={len(rows)} seed={args.seed} worst floating centre error={float(worst):.3g} of the scale', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('14-minimumenclosingcircle', ['empty', 'coordinate-bound', 'no-support', 'radius-no-support', 'approx-empty',
                                                        'approx-nonfinite'], oracle))
