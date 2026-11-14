#!/usr/bin/env python3
"""Calipers: exhaustive hulls, 256-bit objective oracles, ties and support witnesses.

farthestPair and farthestPairNaive are checked against an all-pairs scan on clouds with duplicates,
collinear points and ties (rounds as random hulls, each also shuffled).
quick: regressions, 64 small sets, 50 random hulls/clouds and 100-vertex chain.
full: all 512 3x3 sets, 1000 random hulls/clouds, 10000-vertex chain, all builds.
stress: all 65536 4x4 sets, 8000 random hulls/clouds, 30000-vertex chain, all builds.
expectedBoxAreaApprox, in every hull above: bounds D^2 / pi <= E <= D^2 and the minimum-area rectangle, L^2 / pi for
segments, a C++ literal-rotation Gauss-Legendre oracle (hulls up to 64 vertices, lattice circles), cyclic, quarter-turn,
translation and scale invariance, rectangle known answers ab + (a^2 + b^2) / pi; the Python oracle (box_area) adds
needles, tiny and near-collinear 10^9 hulls, lattice circles of radius 65 and 1105 (full +32045, stress +1185665)
and 40/400/2000 random hulls, each with its cyclic, quarter-turn and translated variants.
Once per full or stress run, -fsyntax-only compiles check that farthestPair and farthestPairNaive admit signed integral
coordinates and fail their static_assert on floating ones.
"""
import os
import shlex
import subprocess
import sys
import threading
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run
import math
import random


HEADER = Path(__file__).resolve().parents[2] / '03-Geometry' / '07-rotatingcalipers.hpp'
DOMAIN_CHECKED = False
DOMAIN_LOCK = threading.Lock()
DOMAIN_CASES = [('farthestPair', 'Point2<lng>', True), ('farthestPair', 'Point2<int8_t>', True), ('farthestPair', 'Point2<double>', False),
                ('farthestPairNaive', 'Point2<lng>', True), ('farthestPairNaive', 'Point2<int8_t>', True), ('farthestPairNaive', 'Point2<double>', False)]


def domain(env):
    """Each operation compiles for its documented coordinate types and fails its static_assert just outside them."""
    global DOMAIN_CHECKED
    with DOMAIN_LOCK:
        if DOMAIN_CHECKED:
            return
        command = [*shlex.split(env.get('CXX', os.environ.get('CXX', 'g++'))), '-std=gnu++20', '-fsyntax-only', '-x', 'c++', '-']
        cases = [(f, t, valid, subprocess.Popen(command, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, env=env))
                 for f, t, valid in DOMAIN_CASES]
        for f, t, valid, process in cases:
            source = f'#include "{HEADER}"\nint main() {{ (void){f}(vector<{t}>{{}}); }}\n'
            _, stderr = process.communicate(source, timeout=120)
            ok = process.returncode == 0 if valid else process.returncode != 0 and 'static assertion failed' in stderr
            if not ok:
                raise RuntimeError(f'command={shlex.join(command)} {f}(vector<{t}>) expected {"compile" if valid else "static_assert failure"} '
                                   f'returncode={process.returncode}\n{stderr[-2000:]}')
        DOMAIN_CHECKED = True
    print(f'PASS rotatingcalipers type domains: {len(DOMAIN_CASES)} -fsyntax-only compiles', flush=True)


def oracle(binary, args, env):
    if args.mode != 'quick':
        domain(env)
    rng = random.Random(args.seed)
    limit = (1 << 128) - 1
    fixed = (0, 1, (1 << 32) - 1, 1 << 64, (1 << 127) - 1, limit)
    cases = [(a, b or 1, c, d or 1) for a in fixed for b in fixed for c in fixed for d in fixed]
    cases += [tuple(rng.getrandbits(128) | 1 for _ in range(4))
              for _ in range({'quick': 200, 'full': 2000, 'stress': 20000}[args.mode])]
    command = [str(binary), '--fractions']
    result = timed_run(f'07-rotatingcalipers oracle {Path(command[0]).name}', command, input=''.join(' '.join(map(str, q)) + '\n' for q in cases),
                                                     text=True, capture_output=True, env=env, timeout=180)
    lines = result.stdout.splitlines()
    if result.returncode or len(lines) != len(cases):
        raise RuntimeError(f'fraction oracle command={command} returncode={result.returncode} '
                           f'expected_lines={len(cases)} actual_lines={len(lines)} stderr={result.stderr}')
    for q, line in zip(cases, lines):
        a, b, c, d = q
        x, y, less = line.split()
        actual = (int(x, 16), int(y, 16), int(less))
        expected = (a * d, c * b, int(a * d < c * b))
        if actual != expected:
            raise RuntimeError(f'fraction oracle seed={args.seed} input={q} expected={expected} actual={actual}')
    print(f'PASS rotatingcalipers Python integer oracle: {len(cases)} full-width product/comparison cases', flush=True)
    box_oracle(binary, args, env)


def hull(points):
    """Strictly convex CCW hull by monotone chain, collinear and duplicate points dropped."""
    p = sorted(set(points))
    if len(p) < 3:
        return p

    def half(seq):
        h = []
        for q in seq:
            while len(h) > 1 and (h[-1][0] - h[-2][0]) * (q[1] - h[-2][1]) - (h[-1][1] - h[-2][1]) * (q[0] - h[-2][0]) <= 0:
                h.pop()
            h.append(q)
        return h[:-1]
    h = half(p) + half(p[::-1])
    return h if len(h) > 1 else p[:1]


def box_area(h):
    """E[bounding-box area] over a uniform rotation, independent of the header's support-function closed form.

    Critical angles are every outward edge normal plus each multiple of pi / 2; on each interval between them the four
    extreme vertices are found by brute force at the midpoint, and (P . u(t)) * (Q . u(t + pi / 2)) is integrated
    analytically with P, Q the exact integer extent vectors, using cancellation-free sin/cos differences.
    """
    n = len(h)
    if n < 2:
        return 0.0
    q = [(x - h[0][0], y - h[0][1]) for x, y in h]
    cuts = {0.0, 2 * math.pi}
    for i in range(n):
        dx, dy = q[(i + 1) % n][0] - q[i][0], q[(i + 1) % n][1] - q[i][1]
        a = math.atan2(-dx, dy)
        cuts.update((a + k * math.pi / 2) % (2 * math.pi) for k in range(4))
    cuts = sorted(cuts)
    total = 0.0
    for l, r in zip(cuts, cuts[1:]):
        m = (l + r) / 2
        c, s = math.cos(m), math.sin(m)
        u = [x * c + y * s for x, y in q]
        v = [y * c - x * s for x, y in q]
        A, B = q[u.index(max(u))], q[u.index(min(u))]
        C, D = q[v.index(max(v))], q[v.index(min(v))]
        px, py, qx, qy = A[0] - B[0], A[1] - B[1], C[0] - D[0], C[1] - D[1]
        # (px c + py s)(qy c - qx s) = px qy c^2 + (py qy - px qx) c s - py qx s^2
        d = math.sin(r - l)
        dsin2 = 2 * math.cos(r + l) * d          # sin 2r - sin 2l
        dcos2 = 2 * math.sin(r + l) * d          # cos 2l - cos 2r
        cc, ss, cs = (r - l) / 2 + dsin2 / 4, (r - l) / 2 - dsin2 / 4, dcos2 / 4
        total += px * qy * cc + (py * qy - px * qx) * cs - py * qx * ss
    return total / (2 * math.pi)


def circle(r):
    """Every lattice point on x^2 + y^2 = r^2, all strictly convex."""
    pts = []
    for x in range(-r, r + 1):
        y = math.isqrt(r * r - x * x)
        if y * y == r * r - x * x:
            pts += [(x, y), (x, -y)]
    return hull(pts)


def make_box_cases(args):
    rng = random.Random(args.seed ^ 0xB0C5)
    M = 10 ** 9
    known = []      # (hull, closed-form answer)
    o = -M
    for a, b in ((1, 1), (2, 3), (7, 1000), (1, 2 * M), (2 * M, 2 * M), (2 * M - 1, 2 * M)):
        # rectangle a x b: ab + (a^2 + b^2) / pi; square side s: s^2 (1 + 2 / pi)
        known.append(([(o, o), (o + a, o), (o + a, o + b), (o, o + b)], a * b + (a * a + b * b) / math.pi))
    for seg in (((0, 0), (3, 4)), ((-M, -M), (M, M)), ((M, -M), (-M, M - 1)), ((5, 5), (6, 5))):
        known.append((list(seg), ((seg[0][0] - seg[1][0]) ** 2 + (seg[0][1] - seg[1][1]) ** 2) / math.pi))
    base = [[], [(5, -7)], [(-M, M)], [(0, 0), (1, 0), (0, 1)], [(M - 1, -M), (M, -M), (M - 1, -M + 1)],
            hull([(-M, -M), (M, M), (M, M - 1)]), hull([(-M, 0), (M, 1), (M - 1, 1)]), hull([(-3, 0), (M, 0), (-3, 1)]),
            hull([(-M, -M + 1), (M - 1, M - 2), (M, M)]), hull([(0, 0), (1, -4), (4, -5), (7, -4), (9, 0), (6, 6), (1, 5)])]
    radii = {'quick': (65, 1105), 'full': (65, 1105, 32045), 'stress': (65, 1105, 32045, 1185665)}[args.mode]
    base += [circle(r) for r in radii]
    for t in range({'quick': 40, 'full': 400, 'stress': 2000}[args.mode]):
        c = (1, 30, M)[t % 3]
        if t % 4 == 3:      # near-collinear huge hulls
            pts = [(x, x + rng.randint(-30, 30)) for x in (rng.randint(-M + 30, M - 30) for _ in range(30))]
        else:
            pts = [(rng.randint(-c, c), rng.randint(-c, c)) for _ in range(rng.randint(1, 40))]
        base.append(hull(pts))
    for h, want in known:
        d2 = max((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 for a in h for b in h)
        if abs(box_area(h) - want) > 1e-12 * d2:
            raise RuntimeError(f'box oracle self-check: closed form {want} != quadrature {box_area(h)} hull={h}')
    groups = []
    for h, want in known + [(h, None) for h in base]:
        d2 = max(((a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2 for a in h for b in h), default=0)
        want = box_area(h) if want is None else want
        variants = [h]
        if h:
            k = rng.randrange(len(h))
            variants.append(h[k:] + h[:k])
            variants.append([(-y, x) for x, y in h])
            lo = [-M - min(z) for z in zip(*h)]
            hi = [M - max(z) for z in zip(*h)]
            dx, dy = rng.randint(lo[0], hi[0]), rng.randint(lo[1], hi[1])
            variants.append([(x + dx, y + dy) for x, y in h])
        groups.append((variants, want, d2))
    return groups


BOX_CASES = {}


def box_cases(args):
    """(hull variants, expected value, squared diameter) groups, built once per seed and mode for every build."""
    key = (args.seed, args.mode)
    if key not in BOX_CASES:
        BOX_CASES[key] = make_box_cases(args)
    return BOX_CASES[key]


def box_oracle(binary, args, env):
    """expectedBoxAreaApprox against box_area, known answers and invariances.

    Tolerance: 1e-12 * D^2 (D the diameter; the answer lies in [D^2 / pi, D^2]): the float oracle errs by about
    eps * D^2 per critical interval, and the largest error seen (stress seed 42, circles up to 972 vertices) is 2.5e-15 * D^2.
    """
    groups = box_cases(args)
    text = ''.join(f'{len(v)} ' + ' '.join(f'{x} {y}' for x, y in v) + '\n' for g in groups for v in g[0])
    command = [str(binary), '--expected']
    result = timed_run(f'07-rotatingcalipers box oracle {Path(command[0]).name}', command, input=text, text=True,
                       capture_output=True, env=env, timeout=180)
    lines = result.stdout.split()
    total = sum(len(g[0]) for g in groups)
    if result.returncode or len(lines) != total:
        raise RuntimeError(f'box oracle command={command} returncode={result.returncode} expected_lines={total} '
                           f'actual_lines={len(lines)} stderr={result.stderr}')
    at = 0
    for variants, want, d2 in groups:
        for i, v in enumerate(variants):
            got = float(lines[at]); at += 1
            exact_zero = len(v) < 2 and got != 0.0
            if exact_zero or abs(got - want) > 1e-12 * d2:
                raise RuntimeError(f'expectedBoxAreaApprox seed={args.seed} variant={("as given", "cyclic start", "quarter turn", "translation")[i]} '
                                   f'expected={want!r} actual={got!r} tolerance={1e-12 * d2} hull={v}')
    print(f'PASS rotatingcalipers Python box-area oracle: {len(groups)} hulls, {total} calls (cyclic start, quarter turn, translation)', flush=True)

if __name__ == '__main__':
    raise SystemExit(main('07-rotatingcalipers', ['clockwise', 'collinear', 'duplicate', 'range', 'denominator', 'farthest-bound', 'farthest-naive-bound'], oracle))
