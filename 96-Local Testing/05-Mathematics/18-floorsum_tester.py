"""Floor sums, modular linear sequences and quotient blocks against brute force and Python integers.

C++ (every mode, optimized, checked and, outside quick, ASan/UBSan builds): exhaustive
n <= 12, m <= 12, |a|, |b| <= 15 (quick: 7, 7, 8) for every operation, every [l, r) and
the first hit by scanning a period; signed64 extreme a, b with moduli 1..2^63 - 1 by
direct 128-bit loops; seeded loops of length up to 2000 (quick 200) with any modulus;
identities over |a|, |b| up to 2^126 modulo 2^128 (additivity in n, a + m, b + m, the
ceilSum mirror) and cross-operation certificates for min, max and firstModInRange;
quotient blocks exhaustively for n <= 3000 and by tiling, maximality and the
2 sqrt(n) count up to 10^12; joint blocks against per-x quotient tuples.
Rounds: quick 300, full 3000, stress 30000.
Python oracle (quick 300, full 3000, stress 20000 rows): the OI Wiki floor-sum and
f, g, h recursions on exact integers, min and max by binary search on the value with
floor-sum counts and the least argument by a modular inverse, and the first hit by
binary search on the prefix count; the oracle is validated against brute force first.
"""
import math
import random
import shlex
import subprocess
from _00_runner import main

LO, HI = -(1 << 63), (1 << 63) - 1
P = 998244353
W = 1 << 128


def fsum(a, b, c, n):
    """Sum of floor((a i + b) / c) for i = 0..n (OI Wiki), any integers a, b, c > 0."""
    if n < 0:
        return 0
    qa, a = divmod(a, c)
    qb, b = divmod(b, c)
    res = qa * n * (n + 1) // 2 + qb * (n + 1)
    if a == 0:
        return res
    m = (a * n + b) // c
    return res + n * m - fsum(c, c - b - 1, a, m - 1)


def fgh(a, b, c, n):
    """(sum f, sum i f, sum f^2) over i = 0..n for f = floor((a i + b) / c) (OI Wiki)."""
    if n < 0:
        return 0, 0, 0
    qa, a = divmod(a, c)
    qb, b = divmod(b, c)
    s1, s2 = n * (n + 1) // 2, n * (n + 1) * (2 * n + 1) // 6
    if a == 0:
        f = g = h = 0
    else:
        m = (a * n + b) // c
        f2, g2, h2 = fgh(c, c - b - 1, a, m - 1)
        f = n * m - f2
        g = (m * n * (n + 1) - h2 - f2) // 2
        h = n * m * (m + 1) - 2 * g2 - 2 * f2 - f
    return (f + qa * s1 + qb * (n + 1), g + qa * s2 + qb * s1,
            h + 2 * qb * f + 2 * qa * g + qa * qa * s2 + qb * qb * (n + 1) + 2 * qa * qb * s1)


def floor_sum(n, m, a, b):
    return fsum(a, b, m, n - 1)


def below(n, m, a, b, v):
    """Number of x in [0, n) with (a x + b) mod m < v, for 0 <= v <= m."""
    return floor_sum(n, m, a, b) - floor_sum(n, m, a, b - v)


def min_mod(n, m, a, b):
    lo, hi = 0, m - 1
    while lo < hi:
        mid = (lo + hi) // 2
        if below(n, m, a, b, mid + 1):
            hi = mid
        else:
            lo = mid + 1
    g = math.gcd(a, m)
    period = m // g
    x = ((lo - b) // g * pow(a // g, -1, period)) % period if period > 1 else 0
    return lo, x


def first_mod(m, a, b, l, r):
    count = lambda x: below(x, m, a, b, r) - below(x, m, a, b, l)
    if l >= r or count(m) == 0:
        return -1
    lo, hi = 1, m
    while lo < hi:
        mid = (lo + hi) // 2
        if count(mid):
            hi = mid
        else:
            lo = mid + 1
    return lo - 1


def signed128(x):
    x %= W
    return x - W if x >= W // 2 else x


def expected(row):
    n, m, a, b, l, r = row
    f = floor_sum(n, m, a, b)
    out = [signed128(f), signed128(-floor_sum(n, m, -a, -b)),
           (a % m) * n * (n - 1) // 2 + (b % m) * n - m * floor_sum(n, m, a % m, b % m),
           below(n, m, a, b, r) - below(n, m, a, b, l), first_mod(m, a, b, l, r)]
    if n >= 1:
        v, x = min_mod(n, m, a % m, b % m)
        V, X = min_mod(n, m, -a % m, (m - 1 - b) % m)
        out += [v, x, m - 1 - V, X]
    else:
        out += ['-'] * 4
    moments = fgh(a, b, m, n - 1)
    out += [t % W for t in moments] + [t % P for t in moments]
    return tuple(map(str, out))


def validate_oracle():
    for n in range(0, 9):
        for m in range(1, 8):
            for a in range(-9, 10):
                for b in range(-9, 10):
                    ys = [a * i + b for i in range(n)]
                    vals = [y % m for y in ys]
                    want = (sum(y // m for y in ys), sum(i * (y // m) for i, y in enumerate(ys)), sum((y // m) ** 2 for y in ys))
                    assert fgh(a, b, m, n - 1) == want and floor_sum(n, m, a, b) == want[0], (n, m, a, b)
                    if n:
                        v = min(vals)
                        assert min_mod(n, m, a % m, b % m) == (v, vals.index(v)), (n, m, a, b)
                    for l in range(m + 1):
                        for r in range(l, m + 1):
                            hit = next((x for x in range(m) if l <= (a * x + b) % m < r), -1)
                            assert first_mod(m, a, b, l, r) == hit, (m, a, b, l, r)


def oracle(binary, args, env):
    validate_oracle()
    rng = random.Random(args.seed)
    count = {'quick': 300, 'full': 3000, 'stress': 20000}[args.mode]
    edges = [LO, LO + 1, -1, 0, 1, HI - 1, HI]
    rows = []
    for i in range(count):
        n = rng.choice([0, 1, 2, rng.randint(0, 100), rng.randint(0, 1 << rng.randint(1, 62)), HI])
        m = rng.choice([1, 2, rng.randint(1, 1000), rng.randint(1, 1 << rng.randint(1, 62)), HI - 1, HI])
        a, b = (rng.choice(edges) if rng.random() < 0.2 else rng.randint(LO, HI) >> rng.randint(0, 62) for _ in range(2))
        l = rng.randint(0, m)
        r = rng.randint(l, m)
        rows.append((n, m, a, b, l, r))
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input=''.join(' '.join(map(str, row)) + '\n' for row in rows),
                            text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    for i, (row, line) in enumerate(zip(rows, actual)):
        want, got = expected(row), tuple(line.split())
        if got != want:
            raise RuntimeError(f'Python integer oracle seed={args.seed} failing row #{i}={row}\n'
                               f'expected={want}\nactual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python exact-integer oracle (validated by brute force) cases={len(rows)} seed={args.seed}', flush=True)
    wide = []
    for i in range(count):
        n = rng.choice([0, 1, 2, rng.randint(0, 1000), rng.randint(0, 1 << rng.randint(1, 62)), HI])
        m = rng.choice([1, 2, rng.randint(1, 1 << rng.randint(1, 126)), (1 << 127) - 1, (1 << 126) + rng.randint(0, 1 << 125)])
        a, b = (rng.choice([-(1 << 127), (1 << 127) - 1, -1, 0, 1]) if rng.random() < 0.15 else rng.randint(-(1 << 127), (1 << 127) - 1) >> rng.randint(0, 126) for _ in range(2))
        wide.append((n, m, a, b))
    command = [str(binary), '--oracle-wide']
    result = subprocess.run(command, input=''.join(' '.join(map(str, row)) + '\n' for row in wide),
                            text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python wide oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(wide):
        raise RuntimeError(f'Python wide oracle output count expected={len(wide)} actual={len(actual)} command={shlex.join(command)}')
    for i, (row, line) in enumerate(zip(wide, actual)):
        want = str(signed128(floor_sum(*row)))
        if line != want:
            raise RuntimeError(f'Python wide oracle seed={args.seed} failing row #{i}={row}\nexpected={want}\nactual={line}\ncommand={shlex.join(command)}')
    print(f'PASS Python exact-integer oracle for floorSumWide cases={len(wide)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('18-floorsum', ['floorsum-m', 'floorsum-n', 'floorsumwide-m', 'count-range', 'count-top', 'moments-m', 'min-empty',
                                          'max-m', 'first-range', 'quotient-negative', 'joint-negative'], oracle))
