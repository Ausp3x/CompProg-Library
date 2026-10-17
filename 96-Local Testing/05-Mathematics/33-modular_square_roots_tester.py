"""Square, k-th and polynomial roots modulo prime powers and composite moduli.

C++: sqrtModAll, sqrtModComposite, sqrtModPrimePower and rootCount(a, 2, m) for every residue of
every m <= 120 / 400 / 700 (quick / full / stress) plus 64-bit representatives against enumerated
roots; sqrtMod2k exhaustively for k <= 14 and planted or random residues up to k = 64 (least root
bounded by the four known roots, squares characterized by valuation and u mod 8);
kthRootModComposite, kthRootModPrimePower and rootCount for every (a, k <= 10 / 18 / 24 plus six
huge k) and m <= 60 / 150 / 230 against power tables; planted k-th roots modulo prime powers up to
2^63 with p | k and non-units, composite products, rootCount multiplicativity and sqrtModAll class
counts; quadraticCongruenceMod for every m <= 50 / 120 / 180 over a coefficient grid with
prime-power multiples and random 64-bit coefficients, and planted quadratics modulo every large prime power
(Taylor-checked classes, planted roots covered, totals equal to the polynomial lifter); polynomialCongruenceRoots on structured
products (x - r)^k, contents, constants and the zero polynomial for m <= 300 and 2- and 3-power
moduli, and planted roots modulo 2^61 - 1, 2^62 - 57 and squares of 1e9 + 7, classes checked
by every Taylor term. Python oracle: moduli built from small prime powers (brute-force counts),
large primes up to 2^62 (counts from Euler's criterion and gcd(k, p - 1)) and squared medium
primes; every returned class is checked exactly (f(r), f^(j)(r) M^j / j! = 0 mod m), disjoint,
and its total size matches the constructed count; least square roots modulo odd prime powers and
2^k are exact. 15 assertion probes.
"""
import math
import random
import subprocess
from _00_runner import main

HI = (1 << 63) - 1
SMALLP = [p for p in range(2, 1000) if all(p % d for d in range(2, int(p ** 0.5) + 1))]


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


def big_prime(rng, bits):
    while True:
        x = rng.getrandbits(bits) | (1 << (bits - 1)) | 1
        if is_prime(x): return x


def nonresidue(rng, p):
    while True:
        z = rng.randrange(2, p)
        if pow(z, (p - 1) // 2, p) == p - 1: return z


def crt(rs, ms):
    x, m = 0, 1
    for r, n in zip(rs, ms):
        t = (r - x) * pow(m, -1, n) % n
        x, m = x + m * t, m * n
    return x % m


def components(rng, kinds, medium_e=(2, 3)):
    """1-3 coprime components (kind, p, e, P) with product below 2^63, kinds drawn from `kinds`."""
    while True:
        comps, used, m = [], set(), 1
        for _ in range(rng.randint(1, 3)):
            kind = rng.choice(kinds)
            if kind == 'small':
                p = rng.choice(SMALLP[:12])
                e = rng.randint(1, max(1, int(math.log(4096, p))))
            elif kind == 'large': p, e = big_prime(rng, rng.randint(20, 62)), 1
            elif kind == 'two': p, e = 2, rng.randint(13, 40)
            else: p, e = big_prime(rng, rng.randint(10, 20)), rng.choice(medium_e)
            if p in used or m * p ** e > HI: continue
            used.add(p); comps.append((kind, p, e, p ** e)); m *= p ** e
        if comps: return comps, m


def shift(rng, a, m):
    """A representative of a mod m anywhere in the signed 64-bit range."""
    lo, hi = -(1 << 63), HI
    return a + rng.randint((lo - a) // m + 1, (hi - a) // m) * m if rng.random() < 0.5 else a


def taylor(f, r):
    """Coefficients of f(x + r) (exact integers)."""
    h = list(f)
    n = len(h)
    for i in range(n):
        for j in range(n - 2, i - 1, -1): h[j] += r * h[j + 1]
    return h


def check_classes(cls, m, f, count, tag):
    """Classes (r, M): shape, sorted, each class entirely roots of f mod m, pairwise disjoint, total size `count`."""
    assert cls == sorted(cls), (tag, 'unsorted')
    for r, M in cls:
        assert M >= 1 and m % M == 0 and 0 <= r < M, (tag, 'shape', r, M)
        assert all(c * M ** j % m == 0 for j, c in enumerate(taylor(f, r))), (tag, 'class not all roots', r, M)
    for i in range(len(cls)):
        for j in range(i):
            assert (cls[i][0] - cls[j][0]) % math.gcd(cls[i][1], cls[j][1]), (tag, 'overlap', cls[i], cls[j])
    assert sum(m // M for _, M in cls) == count, (tag, 'count', sum(m // M for _, M in cls), count)


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 60, 'full': 400, 'stress': 1500}[args.mode]
    rows = []
    # Square roots: per component planted squares, nonresidues and zero; counts from brute force or Euler's criterion.
    for it in range(count):
        comps, m = components(rng, ['small', 'large', 'large', 'medium'])
        rs, want = [], 1
        for kind, p, e, P in comps:
            c = rng.randrange(4)
            if kind == 'small':
                a = rng.randrange(P) if c else rng.randrange(P) ** 2 % P
                want *= sum(1 for x in range(P) if x * x % P == a)
            elif c == 0 and kind == 'large': a = 0
            elif c == 1 and p > 2: a = nonresidue(rng, p) * rng.randrange(1, p) ** 2 % P; want *= 0
            else:
                x = rng.randrange(1, p) + p * rng.randrange(P // p)
                a = x * x % P
            if kind != 'small' and not (c == 1 and p > 2): want *= 1 if a == 0 else 2
            rs.append(a)
        a = crt(rs, [P for *_, P in comps])
        rows.append(('S', (shift(rng, a, m), m), ('sqrt', a, m, want)))
    # k-th roots: planted powers, non-powers and zero per component; counts from tables, gcd(k, p - 1) and the cyclic group order.
    for it in range(count):
        comps, m = components(rng, ['small', 'large', 'large', 'medium'])
        c = it % 6
        k = [0, 1, 2, 3, rng.randint(4, 60), rng.randint(1, HI)][c]
        if c == 3 and rng.random() < 0.5:
            q = rng.choice([p - 1 for _, p, _, _ in comps])
            k = q * rng.randint(1, min(50, HI // q))
        rs, want, unknown = [], 1, False
        for kind, p, e, P in comps:
            z = rng.randrange(3)
            if kind == 'small':
                tab = [pow(x, k, P) for x in range(P)]
                a = rng.randrange(P) if z else rng.choice(tab)
                want *= tab.count(a)
            elif k == 0:
                a = 1 if z else rng.randrange(2, P)
                want *= P if a == 1 else 0
            else:
                phi = P // p * (p - 1)
                g = math.gcd(k, phi)
                if z == 0 and kind == 'large': a = 0; want *= 1
                elif z == 1 and g > 1 and kind == 'large':
                    a = nonresidue(rng, p) if g % 2 == 0 else rng.randrange(2, p)
                    want *= g if pow(a, phi // g, P) == 1 else 0
                else:
                    x = rng.randrange(1, P)
                    while x % p == 0: x = rng.randrange(1, P)
                    a = pow(x, k, P)
                    want *= g
            rs.append(a)
        a = crt(rs, [P for *_, P in comps])
        rows.append(('K', (shift(rng, a, m), k, m), ('kth', a, k, m, want)))
    # Quadratic congruences: per-component coefficients, counts from brute force or the discriminant; 2^e and p^2, p^3 components get two planted simple roots (or, odd p, a nonresidue discriminant).
    for it in range(count):
        comps, m = components(rng, ['small', 'large', 'large', 'two', 'medium'])
        co, want = [[], [], []], 1
        for kind, p, e, P in comps:
            c = it % 5
            a, b, cc = rng.randrange(P), rng.randrange(P), rng.randrange(P)
            if c == 1: a = 0
            if c == 2: x, y = rng.randrange(P), rng.randrange(P); b, cc = -a * (x + y) % P, a * x * y % P
            if c == 3: x = rng.randrange(P); b, cc = -2 * a * x % P, a * x * x % P
            if c == 4: a, b, cc = p * a % P, p * b % P, p * cc % P
            if kind in ('two', 'medium'):
                a = rng.randrange(1, P)
                while a % p == 0: a = rng.randrange(1, P)
                x = rng.randrange(P)
                y = (x + 1 + rng.randrange(p - 1) + p * rng.randrange(P // p)) % P if p > 2 else (x + 1 + 2 * rng.randrange(P // 2)) % P
                if c == 1 and p > 2: b, cc = 0, -a * nonresidue(rng, p) % P
                else: b, cc = -a * (x + y) % P, a * x * y % P
                want *= 0 if c == 1 and p > 2 else 2
            elif kind == 'small': want *= sum(1 for x in range(P) if (a * x * x + b * x + cc) % P == 0)
            elif a % p:
                d = (b * b - 4 * a * cc) % p
                want *= 1 if d == 0 else 2 if pow(d, (p - 1) // 2, p) == 1 else 0
            elif b % p: want *= 1
            else: want *= P if cc % p == 0 else 0
            for i, v in enumerate((a, b, cc)): co[i].append(v)
        ms = [P for *_, P in comps]
        a, b, c = (crt(v, ms) for v in co)
        rows.append(('Q', (shift(rng, a, m), shift(rng, b, m), shift(rng, c, m), m), ('poly', [c, b, a], m, want)))
    # Polynomials: brute-force small components, planted roots times a rootless quadratic modulo large p, and squared medium primes with a double root.
    for it in range(count):
        comps, m = components(rng, ['small', 'large', 'medium'], (2,))
        deg = rng.randint(3, 7)
        polys, want = [], 1
        for kind, p, e, P in comps:
            if kind == 'small':
                f = [rng.randrange(P) for _ in range(deg + 1)]
                want *= sum(1 for x in range(P) if sum(c * x ** i for i, c in enumerate(f)) % P == 0)
            else:
                n = nonresidue(rng, p) if p > 2 else 1
                roots = [rng.randrange(p) for _ in range(rng.randint(1, deg - 2))]
                # (x - r1)^2 (x - r2) (x^2 - n) has p + 1 roots modulo p^2: every x = r1 mod p and the simple root above r2.
                if kind == 'medium': roots = roots[:1] * 2 + [(roots[0] + 1 + rng.randrange(p - 1)) % p]
                f = [(-n) % P, 0, 1]
                for r in roots:
                    f = [((f[i - 1] if i else 0) - r * (f[i] if i < len(f) else 0)) % P for i in range(len(f) + 1)]
                want *= p + 1 if kind == 'medium' else len(set(roots))
            polys.append(f)
        width = max(len(f) for f in polys)
        polys = [f + [0] * (width - len(f)) for f in polys]
        ms = [P for *_, P in comps]
        f = [crt([g[i] for g in polys], ms) for i in range(width)]
        rows.append(('F', (m, it, width, *[shift(rng, c, m) for c in f]), ('poly', f, m, want)))
    # Prime powers: least square roots of planted units, sqrtMod2k least roots of planted odd squares, k-th roots with p | k.
    for it in range(count):
        kind, p, e, P = components(rng, ['large', 'medium'])[0][0]
        x = rng.randrange(1, P)
        while x % p == 0: x = rng.randrange(1, P)
        if it % 2: rows.append(('P', (x * x % P, p, e), ('exact', [min(x, P - x)])))
        else: rows.append(('P', (nonresidue(rng, p) * x * x % P, p, e), ('exact', [-1])))
        k = rng.randint(3, 64)
        y = rng.getrandbits(k) | 1
        h = 1 << (k - 1)
        rows.append(('T', ((y * y % (1 << k)) - (1 << 64 if (y * y % (1 << k)) >> 63 else 0), k), ('exact', [min(y % (1 << k), -y % (1 << k), (y + h) % (1 << k), (h - y) % (1 << k))])))
        q = rng.choice([2, 3, 5, 7])
        e2 = rng.randint(1, int(62 / math.log2(q)))
        kk = q ** rng.randint(1, 5) * rng.randint(1, 30)
        z = rng.randrange(q ** e2)
        rows.append(('R', (pow(z, kk, q ** e2), kk, q, e2), ('kthpp', kk, q ** e2, pow(z, kk, q ** e2))))
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
            if want[0] == 'sqrt':
                _, av, m, cnt = want
                r, cls = got[0], [(got[2 + 2 * i], got[3 + 2 * i]) for i in range(got[1])]
                assert (r == -1) if cnt == 0 else (0 <= r < m and (r * r - av) % m == 0), (tag, 'sqrtModComposite', r)
                check_classes(cls, m, [-av, 0, 1], cnt, tag)
            elif want[0] == 'kth':
                _, av, k, m, cnt = want
                r, c = got
                assert (r == -1) if cnt == 0 else (0 <= r < m and pow(r, k, m) == av % m), (tag, 'kthRootModComposite', r)
                assert c == cnt, (tag, 'rootCount', c, cnt)
            elif want[0] == 'poly':
                _, f, m, cnt = want
                check_classes([(got[1 + 2 * i], got[2 + 2 * i]) for i in range(got[0])], m, f, cnt, tag)
            elif want[0] == 'exact': assert got == want[1], (tag, got, want[1])
            elif want[0] == 'kthpp':
                _, k, P, av = want
                assert len(got) == 1 and 0 <= got[0] < P and pow(got[0], k, P) == av, (tag, got)
        except AssertionError as error:
            raise RuntimeError(f'smallest known reproducer={op} {a} expected={want} actual={got} detail={error}') from None
    print(f'PASS exact Python oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('33-modular_square_roots', ['sqrtpp-overflow', 'sqrtpp-exponent', 'sqrtpp-prime', 'sqrt2k-range',
        'sqrt2k-negative', 'sqrtcomp-mod', 'sqrtall-mod', 'kthpp-negative', 'kthpp-overflow', 'kthcomp-mod', 'kthcomp-negative',
        'count-mod', 'count-negative', 'poly-mod', 'quad-mod'], oracle))
