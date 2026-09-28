#!/usr/bin/env python3
"""Calipers: exhaustive hulls, 256-bit objective oracles, ties and support witnesses.

quick: regressions, 64 small sets, 50 random hulls and 100-vertex chain.
full: all 512 3x3 sets, 1000 random hulls, 10000-vertex chain, all builds.
stress: all 65536 4x4 sets, 8000 random hulls, 30000-vertex chain, all builds.
"""
from _00_runner import main
import random
import subprocess


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    limit = (1 << 128) - 1
    fixed = (0, 1, (1 << 32) - 1, 1 << 64, (1 << 127) - 1, limit)
    cases = [(a, b or 1, c, d or 1) for a in fixed for b in fixed for c in fixed for d in fixed]
    cases += [tuple(rng.getrandbits(128) | 1 for _ in range(4))
              for _ in range({'quick': 200, 'full': 2000, 'stress': 20000}[args.mode])]
    command = [str(binary), '--fractions']
    result = subprocess.run(command, input=''.join(' '.join(map(str, q)) + '\n' for q in cases),
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

if __name__ == '__main__':
    raise SystemExit(main('07-rotatingcalipers', ['clockwise', 'collinear', 'duplicate', 'range', 'denominator'], oracle))
