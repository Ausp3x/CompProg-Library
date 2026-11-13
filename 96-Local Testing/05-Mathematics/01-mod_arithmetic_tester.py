"""Arithmetic against exact Python integers; full adds random full-width inputs."""
import math
import random
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run

INF64 = 0x3f3f3f3f3f3f3f3f  # 01-template.hpp INF64


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
            add('p', a, b, INF64); add('c', a, b)
    for a in [-(1 << 127), -(1 << 127) + 1, -1, 0, 1, (1 << 127) - 1]:
        for b in [-(1 << 127), -hi, -1, 1, hi, (1 << 127) - 1]:
            if not (a == -(1 << 127) and b == -1): add('d', a, b)
        for m in (1, 2, 97, hi): add('n', a, m)
    wlo, whi = -(1 << 127), (1 << 127) - 1

    def strict(a, b):  # floorDivStrict, ceilDivStrict and divModFloor where b != 0 and each result fits lll
        if not b or not wlo <= a <= whi or (a == wlo and b == -1): return
        add('r', a, b)
        if -(-a // b) - 1 >= wlo: add('s', a, b)
        if a // b + 1 <= whi: add('t', a, b)

    def power(a, m, n, exponents, k=2):  # one FixedBasePower object with k digit levels and its repeated get calls
        add('f', a, m, n, k, len(exponents), *exponents)

    def least_base(n, k):  # least B >= 2 with B^k > n, by bisection on exact integers
        lo_b, hi_b = 1, n + 2
        while hi_b - lo_b > 1:
            mid = (lo_b + hi_b) // 2
            lo_b, hi_b = (lo_b, mid) if mid ** k > n else (mid, hi_b)
        return max(hi_b, 2)

    def around(n, k=2):  # endpoints, both sides of every digit-level boundary, random exponents
        b = least_base(n, k)
        es = [0, 1, n, n - 1] + [rng.randint(0, n) for _ in range(8)]
        for i in range(k):
            for c in sorted({1, 2, b - 1, n // b ** i, n // b ** i - 1} | {rng.randint(1, b) for _ in range(3)}):
                es += [c * b ** i + d for d in (-1, 0, 1)]
        return [e for e in es if 0 <= e <= n]

    wide = [wlo, wlo + 1, wlo + 2, wlo // 2, lo - 1, lo, -3, -2, -1, 0, 1, 2, 3, hi, hi + 1, whi // 2, whi - 2, whi - 1, whi]
    for a in range(-24, 25):
        for b in range(-24, 25): strict(a, b)
    for a in wide:
        for b in wide: strict(a, b)
    for b in (2, 3, 7, hi, -hi, 1 << 64, -(1 << 64), (1 << 126) + 1, whi // 3, wlo // 3):
        for q in (wlo // abs(b), wlo // abs(b) + 1, -2, -1, 0, 1, 2, whi // abs(b) - 1, whi // abs(b)):
            if wlo <= b * q <= whi: strict(b * q, b); strict(b * q + 1, b); strict(b * q - 1, b)
    moduli, bases = [1, 2, 97, 10 ** 9 + 7, hi], [lo, lo + 1, -10 ** 18, -2, -1, 0, 1, 2, 3, hi]
    small = [0, 1, 2, 3] + [k * k + d for k in range(2, 11) for d in (-1, 0, 1)]
    for m in moduli:
        for a in bases:
            for n in small: power(a, m, n, list(range(n + 1)) + [n, 0, n // 2])
            for n in (999, 1000, 10 ** 6 - 1, 10 ** 6, 10 ** 6 + 1): power(a, m, n, around(n))
    large = [(lo, hi, 10 ** 12)] if args.mode == 'quick' else [(lo, hi, 10 ** 12), (-3, 10 ** 9 + 7, 10 ** 12 - 1), (rng.randint(lo, hi), rng.randint(2, hi), 10 ** 12 + 1)]
    for a, m, n in large: power(a, m, n, around(n))
    levels = [(3, 10 ** 6), (3, 10 ** 18), (4, hi), (5, hi - 1), (7, 10 ** 12), (63, hi), (64, hi), (128, 5), (200, hi), (2, 0), (3, 1), (4, 15), (4, 16), (2, 3), (2, 4)]
    for k, n in levels if args.mode != 'quick' else levels[2:3] + levels[7:]:
        for a, m in ((lo, hi), (-3, 10 ** 9 + 7), (rng.randint(lo, hi), rng.randint(1, hi)), (0, 2), (2, 1)):
            power(a, m, n, around(n, k), k)
    for a in (0, -1, 2):
        power(a, 1, 10 ** 8, [0, 10 ** 8]); power(a, 2, 10 ** 8, [0, 1, 10 ** 8])
    for a in (0, 1, 2, 1 << 63, umax - 1, umax):
        for b in (0, 1, 2, 1 << 63, umax):
            for m in (1, 2, 97, 1 << 63, umax):
                add('u', a, b, m)
                add('q', a, b, m)
    for a in [1, 3, 5, umax, umax - 2, (1 << 63) - 1, (1 << 63) + 1] + [2 * k + 1 for k in range(64)]:
        add('v', a)
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
        add('m', a, b, m); add('p', a, b, m); add('p', a, b, INF64); add('c', a, b)
        add('c', rng.randint(-(1 << 32), 1 << 32), rng.randint(lo, hi) >> rng.randrange(64))
        add('v', rng.getrandbits(64) | 1)
        a, b = rng.randrange(-(1 << 127), 1 << 127), rng.randrange(-(1 << 127), 1 << 127)
        if b and not (a == -(1 << 127) and b == -1): add('d', a, b)
        a, b, m = rng.getrandbits(64), rng.getrandbits(64), rng.randint(1, umax)
        add('u', a, b, m); add('q', a, b, m)
        a, b = rng.randrange(wlo, whi + 1), rng.randrange(wlo, whi + 1) >> rng.randrange(128)
        strict(a, b)
        if b: q = rng.randrange(wlo // abs(b) + 1, whi // abs(b) + 1); strict(b * q, b)
        if rng.randrange(10) == 0:
            n = rng.randint(0, 1 << rng.randrange(17))
            power(rng.randint(lo, hi), rng.randint(1, hi), n, around(n) + [rng.randint(0, n) for _ in range(4)])
    payload = ''.join(op + ' ' + ' '.join(map(str, v)) + '\n' for op, v in rows)
    command = [str(binary), '--oracle']
    p = timed_run(f'01-mod_arithmetic oracle {Path(command[0]).name}', command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, v), line in zip(rows, lines):
        got = list(map(int, line.split()))
        a, b = (v + (0,))[:2]
        if op == 'f':
            n, k, base, size = v[2], v[3], got[0], got[1]
            expected = [pow(a, e, b) for e in v[5:]]
            ok = got[2:] == expected and base == least_base(n, k) and size == k * base
        elif op == 's': expected = [-(-a // b) - 1]; ok = got == expected
        elif op == 't': expected = [a // b + 1]; ok = got == expected
        elif op == 'r': expected = list(divmod(a, b)); ok = got == expected
        elif op == 'g':
            g = math.gcd(a, b)
            expected = [g, math.lcm(a, b), g]
            ok = got[:3] == expected and a * got[3] + b * got[4] == g
        elif op == 'd': expected = [a // b, -(-a // b)]; ok = got == expected
        elif op == 'n': expected = [a % b]; ok = got == expected
        elif op in ('m', 'u'): expected = [a * b % v[2]]; ok = got == expected
        elif op in ('p', 'q'):
            try: expected = [pow(a, b, v[2])]
            except ValueError: expected = [-1]
            ok = got == expected
        elif op == 'c':
            clamp = lambda x: min(max(x, lo), hi)
            add_ok, mul_ok = lo <= a + b <= hi, lo <= a * b <= hi
            expected = [int(add_ok), a + b if add_ok else 123, int(mul_ok), a * b if mul_ok else 123, clamp(a + b), clamp(a * b)]
            ok = got == expected
        elif op == 'v': expected = [pow(a, -1, 1 << 64)]; ok = got == expected
        else:
            if abs(a) > 1 and b > 64: expected = [0, 123]
            else:
                value = pow(a, b)
                expected = [1, value] if lo <= value <= hi else [0, 123]
            ok = got == expected
        if not ok: raise RuntimeError(f'smallest known reproducer={op} {v}, expected={expected} / Bezout, actual={got}')
    print(f'PASS exact Python arithmetic oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('01-mod_arithmetic', ['floor-zero', 'ceil-zero',
        'floor-overflow', 'ceil-overflow', 'norm-zero', 'norm-negative', 'mul-zero',
        'mul64-zero', 'pow-zero', 'pow64-zero', 'pow64-zero-exponent', 'inv-even', 'floor-strict-zero', 'ceil-strict-zero',
        'floor-strict-overflow', 'ceil-strict-overflow', 'ceil-strict-overflow-negative', 'divmod-zero', 'divmod-overflow', 'power-mod-zero',
        'power-mod-negative', 'power-n-negative', 'power-get-negative', 'power-get-past', 'power-levels-one'], oracle))
