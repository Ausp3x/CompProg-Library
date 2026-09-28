"""Arithmetic against exact Python integers; full adds random full-width inputs."""
import math
import random
import subprocess
from _00_runner import main


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    rows = []
    lo, hi, umax = -(1 << 63), (1 << 63) - 1, (1 << 64) - 1
    edges = [lo, lo + 1, -hi // 2, -2, -1, 0, 1, 2, hi // 2, hi - 1, hi]
    count = {'quick': 200, 'full': 5000, 'stress': 30000}[args.mode]

    def add(op, *values):
        rows.append((op, values))

    for a in range(-24, 25):
        for b in range(-24, 25):
            add('g', a, b)
            if b: add('d', a, b)
    for a in edges:
        for b in edges:
            add('g', a, b)
            if b: add('d', a, b)
            for m in (1, 2, 6, 97, hi):
                add('m', a, b, m)
                add('p', a, b, m)
    for a in [-(1 << 127), -(1 << 127) + 1, -1, 0, 1, (1 << 127) - 1]:
        for b in [-(1 << 127), -hi, -1, 1, hi, (1 << 127) - 1]:
            if not (a == -(1 << 127) and b == -1): add('d', a, b)
        for m in (1, 2, 97, hi): add('n', a, m)
    for a in (0, 1, 2, 1 << 63, umax - 1, umax):
        for b in (0, 1, 2, 1 << 63, umax):
            for m in (1, 2, 97, 1 << 63, umax):
                add('u', a, b, m)
                add('q', a, b, m)
    for a in edges + list(range(-12, 13)):
        for b in list(range(66)) + [umax]: add('i', a, b)
    for e in range(2, 64):
        for limit in (hi, -lo):
            l, r = 0, 1 << 32
            while l + 1 < r:
                mid = (l + r) // 2
                if mid ** e <= limit: l = mid
                else: r = mid
            for a in (l - 1, l, l + 1):
                add('i', a, e); add('i', -a, e)
    for _ in range(count):
        a, b, m = rng.randint(lo, hi), rng.randint(lo, hi), rng.randint(1, hi)
        add('g', a, b)
        if b: add('d', a, b)
        add('n', rng.randrange(-(1 << 127), 1 << 127), m)
        add('m', a, b, m); add('p', a, b, m)
        a, b = rng.randrange(-(1 << 127), 1 << 127), rng.randrange(-(1 << 127), 1 << 127)
        if b and not (a == -(1 << 127) and b == -1): add('d', a, b)
        a, b, m = rng.getrandbits(64), rng.getrandbits(64), rng.randint(1, umax)
        add('u', a, b, m); add('q', a, b, m)
    payload = ''.join(op + ' ' + ' '.join(map(str, v)) + '\n' for op, v in rows)
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, v), line in zip(rows, lines):
        got = list(map(int, line.split()))
        a, b = v[:2]
        if op == 'g':
            g = math.gcd(a, b)
            expected = [g, math.lcm(a, b), g]
            ok = got[:3] == expected and a * got[3] + b * got[4] == g
            if g <= hi: ok &= got[5] == g and a * got[6] + b * got[7] == g
        elif op == 'd': expected = [a // b, -(-a // b)]; ok = got == expected
        elif op == 'n': expected = [a % b]; ok = got == expected
        elif op in ('m', 'u'): expected = [a * b % v[2]]; ok = got == expected
        elif op in ('p', 'q'):
            try: expected = [pow(a, b, v[2])]
            except ValueError: expected = [-1]
            ok = got == expected
        else:
            if abs(a) > 1 and b > 64: expected = [0, 123]
            else:
                value = pow(a, b)
                expected = [1, value] if lo <= value <= hi else [0, 123]
            ok = got == expected
        if not ok: raise RuntimeError(f'smallest known reproducer={op} {v}, expected={expected} / Bezout, actual={got}')
    print(f'PASS exact Python arithmetic oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('01-mod_arithmetic', ['gcd-width', 'alias', 'floor-zero', 'ceil-zero',
        'floor-overflow', 'ceil-overflow', 'norm-zero', 'norm-negative', 'mul-zero',
        'mul64-zero', 'pow-zero', 'pow64-zero'], oracle))
