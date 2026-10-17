"""Continued fractions, rational approximation, Stern-Brocot and Farey operations against brute force and Python integers.

C++ (optimized, checked and, outside quick, ASan/UBSan builds): every p/q with |p|, q <= 60
(quick 25) and seeded full-width p/q for continuedFraction, convergents (prefix evaluation,
determinant (-1)^(i-1)) and semiconvergents (definition); sqrt(D) for D <= 3000 (period
ends in 2 a0, palindromic body) and minimal preperiod and primitive period for seeded
(P, D, Q); brute force over all bounded fractions for bestRationalApproximation and
fractionBinarySearch (x, y <= 30, n <= 12) and rationalInInterval (all terms <= 12);
complete certificates for 62-bit inputs (bracket, Stern-Brocot adjacency, mediant out of
bounds, interval inside the result's subtree, O(log(n)) predicate calls, an irrational
threshold); the Stern-Brocot tree by BFS to depth 11 (quick 9) for every operation and
62-bit certificates; Farey sequences, successors, k-th terms and counts by enumeration
for n <= 60 (quick 25), counts against an O(n log(n)) divisor sieve and k-th terms by
rank up to n = 200000, and n = 10^9 ranks and successors. Rounds scale by mode.
Python oracle (quick 400, full 3000, stress 20000 rows): Euclid and Fraction evaluation
for expansions and convergents; quadratic irrationals expanded exactly as (x + y sqrt(D)) / z
with integer square roots; paths, ranges, children, ancestors and lca from the
continued-fraction formulas; interval and approximation certificates; Farey brute force.
"""
import math
import random
import shlex
import subprocess
from fractions import Fraction
from _00_runner import main

LO, HI = -(1 << 63), (1 << 63) - 1


def cf(p, q):
    out = []
    while q:
        a, r = divmod(p, q)
        out.append(a)
        p, q = q, r
    return out


def convergents(a):
    res = []
    for i in range(len(a)):
        v = Fraction(a[i])
        for t in reversed(a[:i]):
            v = t + 1 / v
        res.append((v.numerator, v.denominator))
    return res


def semiconvergents(a):
    c = convergents(a)
    out = [c[0]]
    for i in range(1, len(a)):
        p2 = c[i - 2] if i >= 2 else (1, 0)
        out += [(t * c[i - 1][0] + p2[0], t * c[i - 1][1] + p2[1]) for t in range(1, a[i] + 1)]
    return out


def quadratic_terms(P, D, Q, count):
    """Leading terms of (P + sqrt(D)) / Q for non-square D, as (x + y sqrt(D)) / z kept in lowest terms."""
    x, y, z, out = P, 1, Q, []
    for _ in range(count):
        k = math.isqrt(y * y * D) if y >= 0 else -math.isqrt(y * y * D) - 1
        a = (x + k) // z if z > 0 else (x + k + 1) // z
        out.append(a)
        x, y, z = z * (x - a * z), -z * y, (x - a * z) ** 2 - y * y * D
        g = math.gcd(x, y, z) * (1 if z > 0 else -1)
        x, y, z = x // g, y // g, z // g
    return out


def path(p, q):
    a = cf(p, q)
    a[-1] -= 1
    return [(('R', 'L')[i % 2], t) for i, t in enumerate(a) if t]


def walk(runs):
    lo, hi = (0, 1), (1, 0)
    for c, t in runs:
        if c == 'R':
            lo = (lo[0] + t * hi[0], lo[1] + t * hi[1])
        else:
            hi = (hi[0] + t * lo[0], hi[1] + t * lo[1])
    return (lo[0] + hi[0], lo[1] + hi[1])


def subtree(p, q):
    """Open interval of the subtree of p/q: between p_{n-1}/q_{n-1} and (p_n - p_{n-1})/(q_n - q_{n-1})."""
    a = cf(p, q)
    if len(a) == 1:
        return (a[0] - 1, 1), (1, 0)
    c = convergents(a)
    u, v = c[-2], (c[-1][0] - c[-2][0], c[-1][1] - c[-2][1])
    return (u, v) if Fraction(*u) < Fraction(*v) else (v, u)


def less(a, b):
    return a[0] * b[1] < b[0] * a[1]


def check(row, line):
    op, args = row[0], row[1:]
    parts = line.split('|')
    if op == 'cf':
        p, q = args
        a = list(map(int, parts[0].split()))
        if a != cf(p, q):
            return 'continuedFraction'
        flat = lambda s: list(zip(*[iter(map(int, s.split()))] * 2))
        if flat(parts[1]) != convergents(a):
            return 'convergents'
        if sum(a[1:]) <= 3000 and flat(parts[2]) != semiconvergents(a):
            return 'semiconvergents'
    elif op == 'quad':
        P, D, Q = args
        pre, period = (list(map(int, s.split())) for s in parts)
        s = math.isqrt(D)
        if s * s == D:
            num, den = (P + s, Q) if Q > 0 else (-P - s, -Q)
            return None if not period and pre == cf(num, den) else 'rational quadratic'
        if not period or (pre and pre[-1] == period[-1]):
            return 'minimal preperiod'
        if any(len(period) % d == 0 and period[d:] == period[:-d] for d in range(1, len(period))):
            return 'primitive period'
        n = len(pre) + 2 * len(period) + 3
        if quadratic_terms(P, D, Q, n) != (pre + period * (n // len(period) + 1))[:n]:
            return 'quadratic terms'
    elif op == 'sb':
        p, q, k = args
        runs = parts[0].split()
        if list(zip(runs[0::2], map(int, runs[1::2]))) != path(p, q):
            return 'sternBrocotEncode'
        lo0, lo1, hi0, hi1, cl0, cl1, cr0, cr1, an0, an1 = map(int, parts[1].split())
        lo, hi = subtree(p, q)
        if ((lo0, lo1), (hi0, hi1)) != (lo, hi):
            return 'sternBrocotRange'
        if (cl0, cl1, cr0, cr1) != (lo[0] + p, lo[1] + q, p + hi[0], q + hi[1]):
            return 'sternBrocotChildren'
        prefix, left = [], k
        for c, t in path(p, q):
            if left:
                prefix.append((c, min(t, left)))
                left -= min(t, left)
        if (an0, an1) != ((-1, -1) if left else walk(prefix)):
            return 'sternBrocotAncestor'
    elif op == 'lca':
        p, q, r, s = args
        x, y, common = path(p, q), path(r, s), []
        for (c, t), (d, u) in zip(x, y):
            if c != d:
                break
            common.append((c, min(t, u)))
            if t != u:
                break
        if tuple(map(int, line.split())) != walk(common):
            return 'sternBrocotLca'
    elif op == 'interval':
        a, b, c, d = args
        p, q = map(int, line.split())
        lo, hi = subtree(p, q)
        inside = less((a, b), (p, q)) and (d == 0 or less((p, q), (c, d)))
        if not (inside and math.gcd(p, q) == 1 and not less((a, b), lo) and (hi[1] == 0 or (d and not less(hi, (c, d))))):
            return 'rationalInInterval certificate'
        if b + d <= 400:
            want = next((pp, qq) for qq in range(1, b + d + 1) for pp in range(0, a + c + 1)
                        if a * qq < pp * b and (d == 0 or pp * d < c * qq))
            if (p, q) != want:
                return 'rationalInInterval brute'
    elif op == 'best':
        x, y, n = args
        l0, l1, h0, h1 = map(int, line.split())
        v = Fraction(x, y)
        if v.numerator <= n and v.denominator <= n:
            return None if (l0, l1, h0, h1) == (v.numerator, v.denominator) * 2 else 'best exact'
        ok = (0 <= l0 <= n and 1 <= l1 <= n and math.gcd(l0, l1) == 1 and Fraction(l0, l1) < v
              and ((h0, h1) == (1, 0) or (0 <= h0 <= n and 1 <= h1 <= n and math.gcd(h0, h1) == 1 and v < Fraction(h0, h1)))
              and h0 * l1 - l0 * h1 == 1 and (l0 + h0 > n or l1 + h1 > n))
        if not ok:
            return 'bestRationalApproximation certificate'
    elif op == 'farey':
        n, k, a, b, c, d = args
        terms = sorted({Fraction(p, q) for q in range(1, n + 1) for p in range(q + 1)})
        x, y, u, v, cnt = line.split()
        t = terms[k]
        if (int(x), int(y)) != (t.numerator, t.denominator):
            return 'fareyKth'
        if k + 1 < len(terms) and (int(u), int(v)) != (terms[k + 1].numerator, terms[k + 1].denominator):
            return 'fareyNext'
        want = sum(1 for q in range(1, n + 1) for p in range(0, (c * q + d - 1) // d + 1)
                   if math.gcd(p, q) == 1 and Fraction(a, b) <= Fraction(p, q) < Fraction(c, d))
        if int(cnt) != want:
            return 'countRationalsInRange'
    return None


def oracle(binary, args, env):
    assert quadratic_terms(0, 2, 1, 5) == [1, 2, 2, 2, 2] and quadratic_terms(1, 5, 2, 4) == [1, 1, 1, 1]
    assert path(3, 5) == [('L', 1), ('R', 1), ('L', 1)] and walk(path(3, 5)) == (3, 5) and subtree(3, 5) == ((1, 2), (2, 3))
    rng = random.Random(args.seed)
    count = {'quick': 400, 'full': 3000, 'stress': 20000}[args.mode]
    coprime = lambda bits: next((p // math.gcd(p, q), q // math.gcd(p, q)) for p, q in iter(lambda: (rng.randint(1, 1 << bits), rng.randint(1, 1 << bits)), None))
    rows = []
    for i in range(count):
        kind = i % 7
        if kind == 0:
            p = rng.choice([LO, HI, 0, rng.randint(LO, HI) >> rng.randint(0, 62)])
            rows.append(('cf', p, rng.choice([1, HI, rng.randint(1, HI) >> rng.randint(0, 62) or 1])))
        elif kind == 1:
            rows.append(('quad', rng.randint(-40, 40), rng.randint(0, 3000), rng.choice([-1, 1]) * rng.randint(1, 40)))
        elif kind == 2:
            p, q = coprime(rng.randint(1, 61))
            rows.append(('sb', p, q, rng.randint(0, 200)))
        elif kind == 3:
            rows.append(('lca', *coprime(rng.randint(1, 61)), *coprime(rng.randint(1, 61))))
        elif kind == 4:
            bits = rng.randint(1, 61)
            a, b, c, d = (rng.randint(0, 1 << bits) for _ in range(4))
            b, d = max(b, 1), max(d, 1)
            if a * d == c * b:
                c += 1
            if a * d > c * b:
                a, b, c, d = c, d, a, b
            rows.append(('interval', a, b, *((1, 0) if rng.random() < 0.1 else (c, d))))
        elif kind == 5:
            bits = rng.randint(1, 62)
            rows.append(('best', rng.randint(0, 1 << bits), rng.randint(1, 1 << rng.randint(1, 62)), rng.randint(1, (1 << bits) - 1)))
        else:
            n = rng.randint(1, 40 if args.mode == 'quick' else 120)
            size = 1 + sum(1 for q in range(1, n + 1) for p in range(1, q + 1) if math.gcd(p, q) == 1)
            a, b, c, d = rng.randint(0, 9), rng.randint(1, 9), rng.randint(0, 30), rng.randint(1, 9)
            if a * d > c * b:
                a, b, c, d = c, d, a, b
            rows.append(('farey', n, rng.randrange(size), a, b, c, d))
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input=''.join(' '.join(map(str, row)) + '\n' for row in rows),
                            text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    for i, (row, line) in enumerate(zip(rows, actual)):
        failure = check(row, line)
        if failure:
            raise RuntimeError(f'Python oracle seed={args.seed} failing row #{i}={row} check={failure}\nactual={line}\ncommand={shlex.join(command)}')
    print(f'PASS Python Fraction, exact quadratic and formula oracle cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('20-continued_fraction', ['cf-q', 'quad-q', 'quad-d', 'convergents-term', 'convergents-overflow', 'semi-term',
                                                    'best-y', 'interval-order', 'search-n', 'encode-coprime', 'decode-char', 'ancestor-k',
                                                    'range-zero', 'farey-n', 'next-p', 'kth-k', 'count-order'], oracle))
