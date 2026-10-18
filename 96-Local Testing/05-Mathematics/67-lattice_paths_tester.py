"""Lattice-path counts against grid and walk DP, enumeration and exact Python closed forms.

C++ (every mode, optimized, checked and, outside quick, ASan/UBSan builds): every gridPaths offset in
[-2, 10]^2, dyckPaths and motzkinPaths for n <= 20 and heights <= 10, pathsBelowDiagonal and
pathsBetweenLines for all endpoints in [0, 7]^2 and strips [lo, hi] in [-3, 3], pathsBelowLine and
ballotGeneral (weak and strict) for slopes mu <= 3, catalanConvolution as powers of the Catalan series,
catalanTriangle, narayana by enumerating Dyck words with k peaks, the Motzkin, Schroeder (large and small,
the small ones as large paths without diagonal steps on the diagonal) and Delannoy tables, delannoy and
bizleyPaths for slopes b / a with a, b <= 4, all against grid or walk DP (quick shrinks every bound);
random obstacles (duplicates, origin, target, outside points), rectangles (clipped, negative) and
boundary flows on rectangles up to 14 x 14 against blocked-grid DP; increasingSequencesBetweenBounds on
arbitrary bounds against an O(n m) prefix-sum DP for n, m <= 12 and up to 1500 x 1500, also modulo
1000003; the LGV determinant against the signed enumeration of vertex-disjoint path systems over every
endpoint permutation for k <= 3; the any-ring triangle in ulng and the tables modulo 1000003; the
convolution paths (bounds, Bizley, rectangle transfer) modulo 2^64 - 59. Rounds:
quick 200, full 2000, stress 20000.
Python oracle (quick 60, full 400, stress 2000 rows): exact closed forms with big integers (Motzkin,
Schroeder and Delannoy sums, Narayana, k/(2n + k) C(2n + k, n), the slope-mu ballot fractions, Bizley's
exponential formula in exact rationals) and Python DP for Dyck walks, strips and lines at larger sizes.
"""
from fractions import Fraction
from math import comb
import random
import shlex
import subprocess
from _00_runner import main

P = 998244353


def motzkin(n):
    return sum(comb(n, 2 * k) * comb(2 * k, k) // (k + 1) for k in range(n // 2 + 1))


def schroder(n):
    large = sum(comb(n, k) * comb(n + k, k) // (k + 1) for k in range(n + 1))
    return large, 1 if n == 0 else large // 2


def delannoy(n):
    return sum(comb(n, k) * comb(n + k, k) for k in range(n + 1))


def narayana(n, k):
    if n == 0:
        return int(k == 0)
    return comb(n, k) * comb(n, k - 1) // n if 1 <= k <= n else 0


def catconv(n, k):
    return int(n == 0) if k == 0 else k * comb(2 * n + k, n) // (2 * n + k)


def ballot(c, d, mu, strict):
    if c == 0 and d == 0:
        return 1
    if strict:
        return (c - mu * d) * comb(c + d, d) // (c + d) if c > mu * d else 0
    return (c - mu * d + 1) * comb(c + d, d) // (c + 1) if c >= mu * d else 0


def bizley(a, b, n):
    """[t^n] exp(sum_j C((a + b) j, a j) / ((a + b) j) t^j) for coprime a, b via n f_n = sum j g_j f_(n-j)."""
    from math import gcd
    g0 = gcd(a, b)
    a, b, n = a // g0, b // g0, n * g0
    g = [Fraction(0)] + [Fraction(comb((a + b) * j, a * j), (a + b) * j) for j in range(1, n + 1)]
    f = [Fraction(1)]
    for m in range(1, n + 1):
        f.append(sum(j * g[j] * f[m - j] for j in range(1, m + 1)) / m)
    assert f[n].denominator == 1
    return int(f[n])


def walk(n, h1, h2):
    d = {h1: 1}
    for _ in range(n):
        e = {}
        for h, v in d.items():
            for s in (-1, 1):
                if h + s >= 0:
                    e[h + s] = e.get(h + s, 0) + v
        d = e
    return d.get(h2, 0)


def grid(x1, y1, x2, y2, ok):
    if x2 < x1 or y2 < y1:
        return 0
    row = [0] * (y2 - y1 + 1)
    for x in range(x1, x2 + 1):
        for j, y in enumerate(range(y1, y2 + 1)):
            if not ok(x, y):
                row[j] = 0
            elif x == x1 and y == y1:
                row[j] = 1
            else:
                row[j] += row[j - 1] if j else 0
    return row[-1]


def validate_oracle():
    assert [motzkin(n) for n in range(8)] == [1, 1, 2, 4, 9, 21, 51, 127]
    assert [schroder(n) for n in range(5)] == [(1, 1), (2, 1), (6, 3), (22, 11), (90, 45)]
    assert [delannoy(n) for n in range(5)] == [1, 3, 13, 63, 321]
    assert [bizley(1, 1, n) for n in range(6)] == [1, 1, 2, 5, 14, 42]
    for a in range(1, 4):
        for b in range(1, 4):
            for n in range(4):
                assert bizley(a, b, n) == grid(0, 0, a * n, b * n, lambda x, y: a * y <= b * x), (a, b, n)
    for c in range(9):
        for d in range(5):
            for mu in range(3):
                assert ballot(c, d, mu, False) == grid(0, 0, c, d, lambda x, y: x >= mu * y)
    for n in range(10):
        for k in range(4):
            assert catconv(n, k) == grid(0, 0, n + k - 1, n, lambda x, y: x >= y) if k else True


def oracle(binary, args, env):
    validate_oracle()
    rng = random.Random(args.seed)
    count = {'quick': 60, 'full': 400, 'stress': 2000}[args.mode]
    rows, want = [], []
    for i in range(count):
        c = i % 10
        if c == 0:
            n = rng.randint(0, 2000)
            rows.append(f'motzkin {n}')
            want.append((motzkin(n) % P,) * 2)
        elif c == 1:
            n = rng.randint(0, 2000)
            rows.append(f'schroder {n}')
            want.append(tuple(v % P for v in schroder(n)))
        elif c == 2:
            n = rng.randint(0, 2000)
            rows.append(f'delannoy {n}')
            want.append((delannoy(n) % P,) * 2)
        elif c == 3:
            n = rng.randint(0, 3000)
            k = rng.randint(-1, n + 1)
            rows.append(f'narayana {n} {k}')
            want.append((narayana(n, k) % P,))
        elif c == 4:
            n, k = rng.randint(0, 3000), rng.randint(0, 3000)
            rows.append(f'catconv {n} {k}')
            want.append((catconv(n, k) % P,))
        elif c == 5:
            mu, d = rng.randint(0, 6), rng.randint(0, 2000)
            cc = max(0, mu * d + rng.randint(-3, 3000))
            st = rng.randint(0, 1)
            rows.append(f'ballot {cc} {d} {mu} {st}')
            want.append((ballot(cc, d, mu, st) % P,))
        elif c == 6:
            a, b, n = rng.randint(1, 6), rng.randint(1, 6), rng.randint(0, 25)
            rows.append(f'bizley {a} {b} {n}')
            want.append((bizley(a, b, n) % P,))
        elif c == 7:
            n, h1, h2 = rng.randint(0, 120), rng.randint(0, 30), rng.randint(0, 30)
            rows.append(f'dyck {n} {h1} {h2}')
            want.append((walk(n, h1, h2) % P,))
        elif c == 8:
            lo = rng.randint(-6, 0)
            hi = rng.randint(lo, 6)
            x1, y1 = rng.randint(0, 5), rng.randint(0, 5)
            x2, y2 = x1 + rng.randint(0, 60), y1 + rng.randint(0, 60)
            rows.append(f'strip {x1} {y1} {x2} {y2} {lo} {hi}')
            want.append((grid(x1, y1, x2, y2, lambda x, y: lo <= y - x <= hi) % P,))
        else:
            mu, b = rng.randint(0, 4), rng.randint(0, 8)
            a = mu * b + rng.randint(0, 10)
            d = b + rng.randint(0, 25)
            cc = max(a, mu * d) + rng.randint(0, 40)
            rows.append(f'line {a} {b} {cc} {d} {mu}')
            want.append((grid(a, b, cc, d, lambda x, y: x >= mu * y) % P,))
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input='\n'.join(rows) + '\n', text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    for i, (row, expect, line) in enumerate(zip(rows, want, actual)):
        if tuple(line.split()) != tuple(map(str, expect)):
            raise RuntimeError(f'Python oracle seed={args.seed} failing row #{i}={row}\nexpected={expect}\nactual={line}\n'
                               f'command={shlex.join(command)}')
    print(f'PASS Python exact closed forms and DP (validated by grid DP) cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('67-lattice_paths', ['grid-table', 'dyck-negative', 'ballot-negative', 'convolution-table', 'line-mu',
                                               'strip-order', 'motzkin-table', 'transfer-table', 'sequences-size',
                                               'sequences-negative', 'bizley-zero', 'lgv-size', 'table-negative'], oracle))
