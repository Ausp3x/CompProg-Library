#!/usr/bin/env python3
"""Exact 2D and 3D closest pair, the randomized and floating variants and the minimum-perimeter triangle.

closestPair and closestPair3: quick runs 2D ordered 2x2 lists through length 4, 3D 2x2x2 lists through
length 3 and small/random/large smoke cases; full runs 2D lists through length 7, all 3x3 subsets, 1,200
random cases and 50,000-point shapes, 3D lists through length 5, 4,000 3x3x3 subsets, 1,200 random cases and
64,001-point lattices, and 500 Python-bigint cases per dimension; stress runs 2D length 9, 3D length 6,
10,000 random cases, 250,000-point and 343,001-point shapes and 4,000 Python cases per dimension.
closestPairRandomized runs every 2D closestPair case (and every typed case) with seeds 0, ~0 and 1/4/8
drawn ones (quick/full/stress), 4 large agreement clouds and the large shapes, 28/700/5,600 typed rounds
per int8/int16/int/lng, and three seeds per Python-bigint 2D case. closestPairApprox runs fixed
regressions, 24/1,200/8,000 random rounds per float/double/long double, and planted 500/50,000/250,000-point
shapes. minimumPerimeterTriangleApprox runs regressions, ordered 2x2 lists through length 5/7/8 and
collinear 1x3 lists through length 7, all 3x3 subsets (and all 4x4 subsets outside quick), 16/800/6,000
random rounds per int8/int16/int/lng/float/double/long double, and planted large inputs (2,000 points in
quick, 200,000 otherwise). Three build configurations outside quick; all modes include boundary regressions.
closestPairNaive, closestPair3Naive and minimumPerimeterTriangleApproxNaive run beside their fast forms on every
oracle-checked case; the two pair scans also through the Python bigint protocol. closestPairSimple runs on every
oracle-checked 2D case (typed rounds included) against the O(n^2) oracle and closestPair, on the timed large shapes
and agreement clouds, through the Python bigint protocol and in the simple-span-x probe. Every closestPairApprox case
runs with the default seed, the closestPairRandomized seeds and one drawn per case; 14/700/5,600 fallback rounds
(repeated points, span >= 2^40 side, side >= 2^980, subnormal distances) and fixed boundary cases are checked against
the brute force, and four exact-double shapes of 2,000 (quick) or 200,000 points against closestPair, timed.
Once per full or stress run, -fsyntax-only compiles check that every static_assert admits its documented
coordinate types and rejects one type just outside them.
"""
import math
import os
import random
import shlex
import subprocess
import threading

import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run


HEADER = Path(__file__).resolve().parents[2] / '03-Geometry' / '005-closestpair.hpp'
DOMAIN_CHECKED = False
DOMAIN_LOCK = threading.Lock()
DOMAIN_CASES = [('closestPair', 'Point2<int>', True), ('closestPair', 'Point2<lng>', True), ('closestPair', 'Point2<lll>', False),
                ('closestPair', 'Point2<double>', False), ('closestPairRandomized', 'Point2<lng>', True),
                ('closestPairRandomized', 'Point2<lll>', False), ('closestPairRandomized', 'Point2<double>', False),
                ('closestPair3', 'Point3<lng>', True), ('closestPair3', 'Point3<lll>', False), ('closestPair3', 'Point3<double>', False),
                ('closestPairApprox', 'Point2<double>', True), ('closestPairApprox', 'Point2<float>', True),
                ('closestPairApprox', 'Point2<long double>', True), ('closestPairApprox', 'Point2<__float128>', True), ('closestPairApprox', 'Point2<int>', False),
                ('closestPairSimple', 'Point2<int>', True), ('closestPairSimple', 'Point2<lng>', True),
                ('closestPairSimple', 'Point2<int8_t>', True), ('closestPairSimple', 'Point2<lll>', False),
                ('closestPairSimple', 'Point2<double>', False),
                ('minimumPerimeterTriangleApprox', 'Point2<lng>', True), ('minimumPerimeterTriangleApprox', 'Point2<int8_t>', True),
                ('minimumPerimeterTriangleApprox', 'Point2<long double>', True), ('minimumPerimeterTriangleApprox', 'Point2<lll>', False),
                ('closestPairNaive', 'Point2<lng>', True), ('closestPairNaive', 'Point2<lll>', False), ('closestPairNaive', 'Point2<double>', False),
                ('closestPair3Naive', 'Point3<lng>', True), ('closestPair3Naive', 'Point3<lll>', False), ('closestPair3Naive', 'Point3<double>', False), ('minimumPerimeterTriangleApproxNaive', 'Point2<double>', True),
                ('minimumPerimeterTriangleApproxNaive', 'Point2<lll>', False)]


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
    print(f'PASS closestpair type domains: {len(DOMAIN_CASES)} -fsyntax-only compiles', flush=True)


def reference(points):
    if len(points) < 2:
        return (-1, -1, 0)
    d, i, j = min((sum((a - b) ** 2 for a, b in zip(p, q)), i, j)
                  for i, p in enumerate(points)
                  for j, q in enumerate(points) if i < j)
    return (i, j, d)


def bigint_oracle(binary, args, env):
    if args.mode != 'quick':
        domain(env)
    check(binary, args, env, 2, '--oracle', 1)
    check(binary, args, env, 2, '--oracle-randomized', 3)
    check(binary, args, env, 3, '--oracle3', 1)
    check(binary, args, env, 2, '--oracle-naive', 1)
    check(binary, args, env, 2, '--oracle-simple', 1)
    check(binary, args, env, 3, '--oracle3-naive', 1)


def check(binary, args, env, dim, flag, copies):
    rng = random.Random(args.seed + dim)
    lo, hi = -(1 << 63), (1 << 63) - 1
    limit = (1 << 127) - 1
    root = math.isqrt(limit)
    if dim == 2:
        points = [[], [(lo, hi)], [(lo, 0), (lo + root, 0)],
                  [(lo, lo), (-1, -1)], [(hi, hi), (hi, hi)],
                  [(lo, 0), (lo + 1, 0), (lo + root, 0)], [(5, lo), (5, lo + root - 3), (6, lo + 1)],
                  [(hi - root + 3, hi), (hi, hi - 3), (hi - 1, hi - 2), (hi - 1, hi - 2)], [(0, 0), (hi, hi), (hi - 1, hi)]]
    else:
        third = math.isqrt(limit // 3)
        points = [[], [(lo, hi, lo)], [(lo, 0, 0), (lo + root, 0, 0)],
                  [(0, 0, 0), (third, third, third)], [(hi, hi, hi), (hi, hi, hi)]]
    rounds = {'quick': 25, 'full': 500, 'stress': 4000}[args.mode]
    for t in range(rounds):
        n = rng.randrange(65)
        if t % 4 == 0:
            # Different squared distances become indistinguishable in float.
            p = [tuple(rng.randrange(-3 * 10 ** 18, 3 * 10 ** 18) for _ in range(dim)) for _ in range(n)]
        elif t % 4 == 1:
            offset = lo if t % 8 == 1 else hi - 100
            p = [tuple(offset + rng.randrange(100) for _ in range(dim)) for _ in range(n)]
        else:
            p = [tuple(rng.randrange(-9, 10) for _ in range(dim)) for _ in range(n)]
        points.append(p)
    command = [str(binary), flag]
    data = [str(len(points))]
    for p in points:
        data.append(str(len(p)))
        data.extend(' '.join(map(str, q)) for q in p)
    try:
        result = timed_run(f'005-closestpair oracle {Path(command[0]).name}', command, input='\n'.join(data) + '\n', text=True,
                                                    capture_output=True, env=env, timeout=180)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f'Python oracle timeout=180s seed={args.seed} '
                           f'command={shlex.join(command)}') from error
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} seed={args.seed} '
                           f'command={shlex.join(command)}\n{result.stderr}')
    lines = result.stdout.splitlines()
    if len(lines) != copies * len(points):
        raise RuntimeError(f'Python oracle protocol seed={args.seed} expected_lines={copies * len(points)} '
                           f'actual_lines={len(lines)} command={shlex.join(command)}')
    for k, line in enumerate(lines):
        p = points[k // copies]
        want, got = reference(p), tuple(map(int, line.split()))
        if got != want:
            raise RuntimeError(f'Python bigint oracle seed={args.seed} input={p!r} '
                               f'expected={want} actual={got} command={shlex.join(command)}')
    print(f'PASS closestpair Python bigint oracle {flag} dimension={dim} cases={len(points)} answers={len(lines)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('005-closestpair', ['span-x', 'span-y', 'diagonal', 'randomized-span-x', 'randomized-span-y',
                                     'randomized-diagonal', 'span-z3', 'diagonal3', 'naive-span-x', 'naive-diagonal3',
                                     'simple-span-x', 'approx-nan', 'triangle-nan'], bigint_oracle))
