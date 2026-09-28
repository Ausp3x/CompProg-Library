#!/usr/bin/env python3
"""Exact closest pair: exhaustive/index-tie C++ checks plus Python bigint oracle.

Quick: ordered 2x2 lists through length 4, small/random/large smoke cases.
Full: ordered lists through length 7, all 3x3 subsets, 1,200 random cases and
50,000-point shapes; 500 Python-bigint random cases, three build configurations.
Stress: ordered lists through length 9, 10,000 random cases and 250,000-point
shapes; 4,000 Python-bigint cases. All modes include boundary regressions.
"""
import math
import random
import shlex
import subprocess

from _00_runner import main


def reference(points):
    if len(points) < 2:
        return (-1, -1, 0)
    d, i, j = min(((x - u) ** 2 + (y - v) ** 2, i, j)
                  for i, (x, y) in enumerate(points)
                  for j, (u, v) in enumerate(points) if i < j)
    return (i, j, d)


def bigint_oracle(binary, args, env):
    rng = random.Random(args.seed)
    lo, hi = -(1 << 63), (1 << 63) - 1
    limit = (1 << 127) - 1
    root = math.isqrt(limit)
    points = [[], [(lo, hi)], [(lo, 0), (lo + root, 0)],
              [(lo, lo), (-1, -1)], [(hi, hi), (hi, hi)]]
    rounds = {'quick': 25, 'full': 500, 'stress': 4000}[args.mode]
    for t in range(rounds):
        n = rng.randrange(65)
        if t % 4 == 0:
            # Different squared distances become indistinguishable in float.
            p = [(rng.randrange(-3 * 10 ** 18, 3 * 10 ** 18),
                  rng.randrange(-3 * 10 ** 18, 3 * 10 ** 18)) for _ in range(n)]
        elif t % 4 == 1:
            offset = lo if t % 8 == 1 else hi - 100
            p = [(offset + rng.randrange(100), offset + rng.randrange(100)) for _ in range(n)]
        else:
            p = [(rng.randrange(-9, 10), rng.randrange(-9, 10)) for _ in range(n)]
        points.append(p)
    command = [str(binary), '--oracle']
    data = [str(len(points))]
    for p in points:
        data.append(str(len(p)))
        data.extend(f'{x} {y}' for x, y in p)
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
    print(f'PASS closestpair Python bigint oracle cases={len(points)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('05-closestpair', ['span-x', 'span-y', 'diagonal'], bigint_oracle))
