"""Pell equations, Cornacchia, sums of squares, Pythagorean triples, Gauss circle counts and binary quadratic forms.

C++: twoSquares, twoSquaresAll, twoSquaresCount, threeSquares, fourSquares, threeTriangular and
gaussCircleCount for every n <= 5000 / 60000 / 200000 (quick / full / stress) against enumerated
pairs, Legendre's three-square set and prefix sums of r2; random 64-bit n (validity, the 4^a (8b + 7)
exclusion, r2 / 4 + [square] list sizes) and large circles against an independent column sum;
cornacchia for every coprime (d, m) with d <= 12 / 40 / 80 and m <= 2000 / 8000 / 20000 against
enumerated primitive pairs, and random 64-bit (d, m); pythagoreanTriples against brute force and
Euclid's parametrization up to N = 3e6; reducedForms for every D down to -1500 / -12000 / -30000
against scanning; reduce under random SL2(Z) moves, represent (primitive and non-primitive forms)
against the ellipse scan, compose against Dirichlet's united forms for |D| <= 150 / 400 / 800;
group laws and h-th powers for |D| up to 8e7; pellFundamental (lll, iint, mint) against the
equation and a y-scan, pellSolutions against repeated multiplication and a 2 x 2 matrix power with
huge k; pellGeneralized against an exhaustive scan below Nagell's bounds with class grouping, and with iint
for large fundamental units (D = 1021 regression).
Python oracle: constructed n with known factorizations (r2 product formula), class-number-one
forms (w * sum of Kronecker symbols over divisors) under random SL2(Z) moves, primitive counts
2^w and 2^(w - 1) for x^2 + y^2 and x^2 + 2 y^2, class numbers by Dirichlet's analytic formula,
compositions by CRT-united forms with an independent reduction, Pell fundamentals by an
independent continued fraction, generalized Pell planted classes, validity, distinctness and
minimality. 24 assertion probes.
"""
import math
import random
import subprocess
from _00_runner import main

HI = (1 << 63) - 1
SMALLP = [p for p in range(2, 2000) if all(p % d for d in range(2, int(p ** 0.5) + 1))]


def is_prime(n):
    if n < 2: return False
    for p in SMALLP[:13]:
        if n % p == 0: return n == p
    d, s = n - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in SMALLP[:13]:
        x = pow(a, d, n)
        if x in (1, n - 1): continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1: break
        else: return False
    return True


def prime_mod(rng, bits, residues, modulus):
    while True:
        x = rng.getrandbits(bits) | (1 << (bits - 1)) | 1
        if x % modulus in residues and is_prime(x): return x


def kronecker(a, p):
    """(a / p) for a prime p: the (a / 2) table or Euler's criterion."""
    if p == 2: return 0 if a % 2 == 0 else (1 if a % 8 in (1, 7) else -1)
    r = pow(a % p, (p - 1) // 2, p)
    return 0 if r == 0 else (1 if r == 1 else -1)


def jacobi_kronecker(D, a):
    """(D / a) for a >= 1 by trial factorization of a (a small)."""
    res, m = 1, a
    for p in SMALLP:
        if p * p > m: break
        while m % p == 0: m //= p; res *= kronecker(D, p)
    if m > 1: res *= kronecker(D, m)
    return res


def reduce_form(a, b, c):
    while True:
        q = (a - b) // (2 * a)
        b, c = b + 2 * a * q, a * q * q + b * q + c
        if a > c: a, b, c = c, -b, a; continue
        if a == c and b < 0: b = -b
        return a, b, c


def sl2_move(rng, f, steps):
    a, b, c = f
    for _ in range(steps):
        q = rng.randint(-3, 3)
        a, b, c = a, b + 2 * a * q, a * q * q + b * q + c
        a, b, c = c, -b, a
    return a, b, c


def is_fundamental(D):
    def squarefree(n): return all(n % (p * p) for p in SMALLP if p * p <= n)
    if D % 4 == 1: return squarefree(-D)
    return D % 4 == 0 and (-D // 4) % 4 in (1, 2) and squarefree(-D // 4)


def pell_fundamental(D):
    s = math.isqrt(D)
    P, Q, a = 0, 1, s
    x0, x1, y0, y1 = 1, a, 0, 1
    while True:
        P = a * Q - P; Q = (D - P * P) // Q; a = (s + P) // Q
        if x1 * x1 - D * y1 * y1 == 1: return x1, y1
        x0, x1, y0, y1 = x1, a * x1 + x0, y1, a * y1 + y0


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 40, 'full': 300, 'stress': 1200}[args.mode]
    rows = []
    # Two squares on constructed n: r2 = 4 prod (e + 1) over p = 1 mod 4, zero when a p = 3 mod 4 has odd exponent.
    for it in range(count):
        n, fac = 1, {}
        for _ in range(rng.randint(0, 5)):
            kind = rng.randrange(4)
            if kind == 0: p, e = 2, rng.randint(1, 6)
            elif kind == 1: p, e = prime_mod(rng, rng.randint(3, 40), {3}, 4), 2 * rng.randint(1, 2) - (it % 7 == 0)
            else: p, e = (prime_mod(rng, rng.randint(3, 30), {1}, 4) if kind == 2 else rng.choice([5, 13, 17, 29])), rng.randint(1, 3)
            if n * p ** e > HI: continue
            n *= p ** e
            fac[p] = fac.get(p, 0) + e
        r2 = 0 if any(p % 4 == 3 and e % 2 for p, e in fac.items()) else 4 * math.prod(e + 1 for p, e in fac.items() if p % 4 == 1)
        rows.append(('A', (n,), ('two', n, r2)))
    # Three and four squares, three triangular numbers: random n up to 2^63 and the excluded forms 4^a (8 b + 7).
    for it in range(count):
        n = rng.randint(0, HI) if it % 3 else rng.randint(0, 1 << rng.randint(1, 62))
        if it % 4 == 0: n = (8 * rng.randint(0, (1 << 50)) + 7) << (2 * rng.randint(0, 4))
        rows.append(('B', (n,), ('three', n)))
    # Cornacchia: x^2 + y^2 and x^2 + 2 y^2 have class number one, so odd m with split primes has 2^w and 2^(w - 1) primitive pairs; random (d, m) for validity.
    for it in range(count):
        d = 1 + it % 2
        m, w = 1, 0
        for _ in range(rng.randint(1, 4)):
            p = prime_mod(rng, rng.randint(5, 28), {1} if d == 1 else {1, 3}, 4 if d == 1 else 8)
            e = rng.randint(1, 2)
            if m * p ** e > HI or m % p == 0: continue
            m *= p ** e; w += 1
        rows.append(('C', (d, m), ('corn', d, m, 2 ** w if d == 1 else 2 ** (w - 1))))
        d2, m2 = rng.randint(1, 1 << rng.randint(1, 40)), rng.randint(1, HI)
        while math.gcd(d2, m2) != 1: m2 = rng.randint(1, HI)
        rows.append(('C', (d2, m2), ('corn', d2, m2, None)))
    # Pell fundamental solutions against an independent continued fraction.
    for it in range(count // 2):
        D = rng.randint(2, 10 ** rng.randint(2, 6))
        if math.isqrt(D) ** 2 == D: D += 1
        rows.append(('P', (D,), ('pell', D)))
    # Generalized Pell: planted solutions must have their class present; every returned solution is valid, distinct in class and minimal.
    for it in range(count // 2):
        D = rng.randint(2, 10 ** rng.randint(1, 4))
        if math.isqrt(D) ** 2 == D: D += 1
        x0, y0 = rng.randint(0, 10 ** rng.randint(1, 6)), rng.randint(0, 10 ** rng.randint(1, 5))
        N = x0 * x0 - D * y0 * y0
        if N == 0 or abs(N) > HI: N, x0, y0 = 1, 1, 0
        rows.append(('G', (D, N), ('gen', D, N, x0, y0)))
    # Representations by the class-number-one forms, moved by random SL2(Z) steps: w * sum over d | n of (D / d).
    ones = [(-3, 6), (-4, 4), (-7, 2), (-8, 2), (-11, 2), (-19, 2), (-43, 2), (-67, 2), (-163, 2)]
    for it in range(count):
        D, w = rng.choice(ones)
        b = D % 2
        f = sl2_move(rng, (1, b, (b * b - D) // 4), rng.randint(0, 3))
        n, fac = 1, {}
        for _ in range(rng.randint(0, 4)):
            p = rng.choice(SMALLP[:30]) if rng.random() < 0.6 else prime_mod(rng, rng.randint(10, 30), {1, 3}, 4)
            e = rng.randint(1, 3)
            if n * p ** e > 10 ** 15: continue
            n *= p ** e
            fac[p] = fac.get(p, 0) + e
        total = math.prod(sum(kronecker(D, p) ** i for i in range(e + 1)) for p, e in fac.items())
        rows.append(('R', (*f, n), ('rep', f, n, w * total)))
    # Class numbers of fundamental discriminants: h = w / (2 |D|) |sum_{a < |D|} (D / a) a|.
    for it in range({'quick': 3, 'full': 12, 'stress': 30}[args.mode]):
        D = -rng.randint(3, {'quick': 20000, 'full': 120000, 'stress': 250000}[args.mode])
        while not is_fundamental(D): D -= 1
        w = 6 if D == -3 else 4 if D == -4 else 2
        h = w * abs(sum(jacobi_kronecker(D, a) * a for a in range(1, -D))) // (-2 * D)
        rows.append(('H', (D,), ('exact', [h])))
    # Composition for large |D| against Dirichlet's united form of forms with coprime leading coefficients.
    for it in range(count):
        D = -rng.randint(3, 10 ** rng.randint(3, 12))
        D -= (D % 4) if D % 4 in (2, 3) else 0
        if D % 4 not in (0, 1) or D >= 0: continue
        forms = []
        while len(forms) < 2:
            p = rng.choice(SMALLP[:150])
            if forms and math.gcd(p, forms[0][0]) != 1: continue
            bs = [x for x in range(2 * p) if (x * x - D) % (4 * p) == 0]
            if bs:
                bb = rng.choice(bs)
                f = (p, bb, (bb * bb - D) // (4 * p))
                if math.gcd(math.gcd(f[0], f[1]), f[2]) == 1: forms.append(f)
        (a1, b1, c1), (a2, b2, c2) = forms
        t = (b2 - b1) // 2 * pow(a1, -1, a2) % a2
        B = b1 + 2 * a1 * t
        assert (B - b2) % (2 * a2) == 0 and (B * B - D) % (4 * a1 * a2) == 0
        want = reduce_form(a1 * a2, B, (B * B - D) // (4 * a1 * a2))
        rows.append(('X', (*sl2_move(rng, forms[0], rng.randint(0, 2)), *sl2_move(rng, forms[1], rng.randint(0, 2))), ('exact', list(want))))
    assert all(-(1 << 63) <= v <= HI for _, a, _ in rows for v in a), 'payload outside the signed 64-bit range'
    payload = ''.join(op + ' ' + ' '.join(map(str, a)) + '\n' for op, a, _ in rows)
    command = [str(binary), '--oracle']
    proc = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=300)
    if proc.returncode: raise RuntimeError(f'command={command} returncode={proc.returncode}\n{proc.stderr}')
    lines = proc.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, a, want), line in zip(rows, lines):
        got = list(map(int, line.split()))
        tag = (op, a)
        try:
            if want[0] == 'two':
                _, n, r2 = want
                x, y, c, k = got[:4]
                pairs = [(got[4 + 2 * i], got[5 + 2 * i]) for i in range(k)]
                assert c == r2, (tag, 'twoSquaresCount', c, r2)
                assert (x, y) == (-1, -1) if r2 == 0 else (x >= y >= 0 and x * x + y * y == n), (tag, 'twoSquares', x, y)
                assert pairs == sorted(set(pairs)) and all(u >= 0 and v >= 0 and u * u + v * v == n for u, v in pairs), (tag, 'twoSquaresAll pairs')
                assert len(pairs) == r2 // 4 + (math.isqrt(n) ** 2 == n), (tag, 'twoSquaresAll count', len(pairs))
            elif want[0] == 'three':
                n = want[1]
                m = n
                while m and m % 4 == 0: m //= 4
                t, f = got[:3], got[3:7]
                if n and m % 8 == 7: assert t == [-1, -1, -1], (tag, 'threeSquares excluded', t)
                else: assert t == sorted(t, reverse=True) and t[2] >= 0 and sum(v * v for v in t) == n, (tag, 'threeSquares', t)
                assert f == sorted(f, reverse=True) and f[3] >= 0 and sum(v * v for v in f) == n, (tag, 'fourSquares', f)
                if len(got) > 7:
                    g = got[7:]
                    assert g == sorted(g, reverse=True) and g[2] >= 0 and sum(v * (v + 1) // 2 for v in g) == n, (tag, 'threeTriangular', g)
            elif want[0] == 'corn':
                _, d, m, cnt = want
                pairs = [(got[1 + 2 * i], got[2 + 2 * i]) for i in range(got[0])]
                assert pairs == sorted(set(pairs)) and all(x >= 0 and y >= 0 and math.gcd(x, y) == 1 and x * x + d * y * y == m for x, y in pairs), (tag, 'cornacchia pairs')
                assert cnt is None or len(pairs) == cnt, (tag, 'cornacchia count', len(pairs), cnt)
            elif want[0] == 'pell': assert tuple(got) == pell_fundamental(want[1]), (tag, 'pellFundamental')
            elif want[0] == 'gen':
                _, D, N, x0, y0 = want
                sols = [(got[1 + 2 * i], got[2 + 2 * i]) for i in range(got[0])]
                ux, uy = pell_fundamental(D)
                an = abs(N)
                assert sols == sorted(sols, key=lambda s: (s[1], s[0])), (tag, 'order')
                for i, (x, y) in enumerate(sols):
                    assert x * x - D * y * y == N and y >= 0, (tag, 'equation', x, y)
                    for s in (1, -1):
                        nx, ny = x * ux + D * y * uy * s, x * uy * s + y * ux
                        assert abs(ny) >= y, (tag, 'not minimal', x, y)
                        assert abs(ny) > y or nx * (1 if ny >= 0 else -1) <= x, (tag, 'tie not resolved to larger x', x, y)
                    for x2, y2 in sols[:i]:
                        assert (x * x2 - D * y * y2) % an or (x * y2 - x2 * y) % an, (tag, 'same class twice', (x, y), (x2, y2))
                assert any((x0 * x - D * y0 * y) % an == 0 and (x0 * y - x * y0) % an == 0 for x, y in sols), (tag, 'planted class missing')
            elif want[0] == 'rep':
                _, (fa, fb, fc), n, cnt = want
                pairs = [(got[1 + 2 * i], got[2 + 2 * i]) for i in range(got[0])]
                assert pairs == sorted(set(pairs)) and all(fa * x * x + fb * x * y + fc * y * y == n for x, y in pairs), (tag, 'represent pairs')
                assert len(pairs) == cnt, (tag, 'represent count', len(pairs), cnt)
            elif want[0] == 'exact': assert got == want[1], (tag, got, want[1])
        except AssertionError as error:
            raise RuntimeError(f'smallest known reproducer={op} {a} expected={want} actual={got[:12]} detail={error}') from None
    print(f'PASS exact Python oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('34-quadratic_congruence', ['corn-d', 'corn-m', 'corn-gcd', 'two-negative', 'twoall-negative',
        'twocount-negative', 'three-negative', 'four-negative', 'tri-negative', 'tri-large', 'pyth-negative', 'gauss-negative',
        'pellgen-square', 'pellgen-small', 'pellgen-min', 'forms-positive', 'forms-residue', 'reduce-indefinite',
        'reduce-negative', 'compose-disc', 'compose-primitive', 'represent-indefinite', 'reduce-disc', 'pellsol-unit'], oracle))
