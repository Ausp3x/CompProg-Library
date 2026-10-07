#!/usr/bin/env python3
"""Exact 2D and 3D closest pair: exhaustive/index-tie C++ checks plus Python bigint oracles.

Quick: 2D ordered 2x2 lists through length 4, 3D 2x2x2 lists through length 3,
small/random/large smoke cases. Full: 2D lists through length 7, all 3x3 subsets,
1,200 random cases and 50,000-point shapes; 3D lists through length 5, 4,000
3x3x3 subsets, 1,200 random cases and 64,001-point lattices; 500 Python-bigint
cases per dimension; three build configurations. Stress: 2D length 9, 3D length
6, 10,000 random cases, 250,000-point and 343,001-point shapes, 4,000 Python
cases per dimension. All modes include boundary regressions.
"""
import math
import random
import shlex
import subprocess

from _00_runner import main


def reference(points):
    if len(points) < 2:
        return (-1, -1, 0)
    d, i, j = min((sum((a - b) ** 2 for a, b in zip(p, q)), i, j)
                  for i, p in enumerate(points)
                  for j, q in enumerate(points) if i < j)
    return (i, j, d)


def bigint_oracle(binary, args, env):
    check(binary, args, env, 2)
    check(binary, args, env, 3)


def check(binary, args, env, dim):
    rng = random.Random(args.seed + dim)
    lo, hi = -(1 << 63), (1 << 63) - 1
    limit = (1 << 127) - 1
    root = math.isqrt(limit)
    if dim == 2:
        points = [[], [(lo, hi)], [(lo, 0), (lo + root, 0)],
                  [(lo, lo), (-1, -1)], [(hi, hi), (hi, hi)]]
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
    command = [str(binary), '--oracle' if dim == 2 else '--oracle3']
    data = [str(len(points))]
    for p in points:
        data.append(str(len(p)))
        data.extend(' '.join(map(str, q)) for q in p)
    try:
        result = subprocess.run(command, input='\n'.join(data) + '\n', text=True,
                                capture_output=True, env=env, timeout=180)
    except subprocess.TimeoutExpired as error:
        raise RuntimeError(f'Python oracle timeout=180s seed={args.seed} '
                           f'command={shlex.join(command)}') from error
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} seed={args.seed} '
                           f'command={shlex.join(command)}\n{result.stderr}')
    lines = result.stdout.splitlines()
    if len(lines) != len(points):
        raise RuntimeError(f'Python oracle protocol seed={args.seed} expected_lines={len(points)} '
                           f'actual_lines={len(lines)} command={shlex.join(command)}')
    for p, line in zip(points, lines):
        want, got = reference(p), tuple(map(int, line.split()))
        if got != want:
            raise RuntimeError(f'Python bigint oracle seed={args.seed} input={p!r} '
                               f'expected={want} actual={got} command={shlex.join(command)}')
    print(f'PASS closestpair Python bigint oracle dimension={dim} cases={len(points)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('05-closestpair', ['span-x', 'span-y', 'diagonal', 'span-z3', 'diagonal3'], bigint_oracle))
