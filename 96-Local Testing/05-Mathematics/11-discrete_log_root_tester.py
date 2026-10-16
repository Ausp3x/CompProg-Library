"""Discrete logs, orders, primitive roots, power-residue symbols and prime-field roots.

C++: multiplicativeOrder, isPrimitiveRoot, primitiveRootExists and primitiveRoot for every
m <= 120 / 320 / 520 (quick / full / stress) and every a in [-m, 2m) plus 64-bit extremes,
against brute order tables; discreteLogEx for every (a, b, k) with m <= 12 / 20 / 26 and
400 / 4000 / 20000 random or planted cases (moduli to 2^14, 64-bit residues) against
iteration; discreteLogMonoid on 150 / 900 / 4000 random functional graphs (every s, t; four
ranges incl. lb near 1e18; identity, constant and parity hashes), modular scalars and 2 x 2
matrices; kroneckerSymbol on [-40, 40]^2 / 120 / 200 plus 64-bit extremes against the
definition; legendreSymbol and sqrtModPrimeCipolla on every residue of every prime up to
150 / 600 / 1500; kthRootModPrime and kthRootExists for every (a, k <= p + 1, large k) and
p <= 50 / 100 / 160 plus 21 primes with high q-adic p - 1; unitRootModPrime for every n and
p <= 150 / 500 / 1000. Python oracle: constructed primes below 2^63 with known p - 1
factorizations (smooth, 2 Q, 2^s r, q^s r, Q^2 r, NTT primes, 2^61 - 1), exact least logs
through indices of a known primitive root, planted logs on composites, order and primitive
root characterizations with minimality, Kronecker by Euler's criterion per prime factor,
square, k-th and unit roots checked with Python pow. 14 assertion probes.
"""
import math
import random
import subprocess
from _00_runner import main

HI = (1 << 63) - 1
LO = -(1 << 63)
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


def trial(n):
    """Factor n by trial division below 2e6; the cofactor must be 1 or prime."""
    f = {}
    for d in range(2, 2_000_000):
        if d * d > n: break
        while n % d == 0: f[d] = f.get(d, 0) + 1; n //= d
    if n > 1:
        assert is_prime(n)
        f[n] = f.get(n, 0) + 1
    return f


def value(f):
    return math.prod(q ** e for q, e in f.items())


def big_prime(rng, bits):
    while True:
        x = rng.getrandbits(bits) | (1 << (bits - 1)) | 1
        if is_prime(x): return x


def make_prime(rng, forced, bits):
    """Prime p <= 2^63 - 1 with p - 1 = 2 * forced * small primes, about `bits` bits; returns (p, factorization of p - 1)."""
    for _ in range(100000):
        f = dict(forced)
        f[2] = f.get(2, 0) + 1
        for i in range(64):
            if i >= 2 and value(f).bit_length() >= bits: break
            q = rng.choice(SMALLP[:40])
            f[q] = f.get(q, 0) + 1
        p = value(f) + 1
        if p <= HI and is_prime(p): return p, f
    raise RuntimeError('no prime found')


def prime_pool(rng, count):
    pool = []
    fixed = [998244353, 1000000007, 2305843009213693951, 469762049, 167772161, 754974721, 3, 5, 7, 65537, 1000000000000000003]
    for p in fixed:
        assert is_prime(p)
        pool.append(('fixed', p, trial(p - 1)))
    for i in range(count):
        kind = i % 5
        if kind == 0: pool.append(('smooth', *make_prime(rng, {}, rng.randint(12, 61))))
        if kind == 1: pool.append(('2Q', *make_prime(rng, {big_prime(rng, rng.randint(20, 52)): 1}, 0)))
        if kind == 2: pool.append(('2^s', *make_prime(rng, {2: rng.randint(18, 46)}, rng.randint(52, 60))))
        if kind == 3:
            q = rng.choice([3, 5, 7, 11, 13, 101, 1009])
            s = max(2, int(rng.randint(15, 42) / math.log2(q)))
            pool.append(('q^s', *make_prime(rng, {q: s}, rng.randint(1, 61))))
        if kind == 4: pool.append(('Q^2', *make_prime(rng, {big_prime(rng, rng.randint(12, 28)): 2}, rng.randint(1, 61))))
    return [(kind, p, f) for kind, p, f in pool if p <= HI]


def generator(p, f):
    g = 2 if p > 2 else 1
    while any(pow(g, (p - 1) // q, p) == 1 for q in f): g += 1
    return g


def char_order(a, r, m, primes):
    """r is the multiplicative order of a mod m, given the primes of a multiple of r."""
    return r >= 1 and pow(a, r, m) == 1 % m and all(pow(a, r // q, m) != 1 % m for q in primes if r % q == 0)


def shift(rng, a, m):
    """A representative of a mod m anywhere in the signed 64-bit range."""
    t = rng.randint((LO - a) // m + 1, (HI - a) // m) if rng.random() < 0.5 else 0
    return a + t * m


def kron(a, n, nf):
    """Kronecker (a / n) from the factorization nf of |n|: sign rule, (a / 2) table, Euler's criterion per odd prime."""
    if n == 0: return int(abs(a) == 1)
    res = -1 if n < 0 and a < 0 else 1
    for q, e in nf.items():
        if q == 2: s = 0 if a % 2 == 0 else (1 if a % 8 in (1, 7) else -1)
        else:
            r = pow(a % q, (q - 1) // 2, q)
            s = 0 if r == 0 else (1 if r == 1 else -1)
        res *= s ** e
    return res


def lambda_of(mf):
    """Carmichael lambda and the primes of a multiple of it, from the factorization of m with p - 1 factorizations."""
    lam, primes = 1, set()
    for p, (e, pf) in mf.items():
        v = (p - 1) * p ** (e - 1)
        if p == 2 and e >= 3: v //= 2
        lam = lam * v // math.gcd(lam, v)
        primes |= set(pf)
        if e > 1: primes.add(p)
    return lam, primes


def self_check(rng):
    """Validate the oracle's index formula, Kronecker definition and order characterization by brute force on small primes."""
    for p in [q for q in SMALLP if 2 < q < 200]:
        f = trial(p - 1)
        z = generator(p, f)
        for _ in range(30):
            u, v, w = (rng.randrange(p - 1) for _ in range(3))
            a, b, k = pow(z, u, p), pow(z, v, p), pow(z, w, p)
            g = math.gcd(u, p - 1)
            want = -1 if (v - w) % g else (v - w) // g % ((p - 1) // g) * pow(u // g, -1, (p - 1) // g) % ((p - 1) // g)
            brute = next((x for x in range(p) if k * pow(a, x, p) % p == b), -1)
            assert want == brute, ('index formula', p, a, b, k)
            order = next(t for t in range(1, p) if pow(a, t, p) == 1)
            assert char_order(a, order, p, f) and all(not char_order(a, t, p, f) for t in range(1, order))
        for a in range(-30, 30):
            sq = {x * x % p for x in range(1, p)}
            assert kron(a, p, {p: 1}) == (0 if a % p == 0 else 1 if a % p in sq else -1), ('kron', a, p)


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    self_check(rng)
    count = {'quick': 40, 'full': 300, 'stress': 1200}[args.mode]
    pool = prime_pool(rng, {'quick': 20, 'full': 60, 'stress': 150}[args.mode])
    gens = {p: generator(p, f) for _, p, f in pool}
    small = [(k, p, f) for k, p, f in pool if p < 1 << 36]
    rows = []
    # Discrete logs: exact least solutions through indices of a known primitive root on primes and odd prime powers.
    for it in range(count):
        kind, p, f = rng.choice(small)
        m, phi, z = p, p - 1, gens[p]
        if it % 4 == 0 and p < 2000:
            e = 1
            while m * p < 1 << 36: m *= p; e += 1
            phi = m // p * (p - 1)
            pr = set(f) | ({p} if e > 1 else set())
            z = next(g for g in range(2, m) if math.gcd(g, m) == 1 and all(pow(g, phi // q, m) != 1 for q in pr))
        u, v, w = (rng.randrange(phi) for _ in range(3))
        if it % 3 == 0: u = phi // rng.choice([q for q in f]) * rng.randrange(1, 4) % phi
        a, b, k = pow(z, u, m), pow(z, v, m), pow(z, w, m)
        if it % 10 == 1: v = (w + u * rng.randrange(phi)) % phi; b = pow(z, v, m)
        g = math.gcd(u, phi)
        want = -1 if (v - w) % g else (v - w) // g % (phi // g) * pow(u // g, -1, phi // g) % (phi // g)
        if it % 25 == 2: a, b, k, want = 0, 0, rng.randrange(1, m), 1
        if it % 25 == 3: a, b, k, want = 0, k, k, 0
        if it % 25 == 4: a, k, want = rng.randrange(m), 0, 0 if b == 0 else -1
        rows.append(('L', (shift(rng, a, m), shift(rng, b, m), m, shift(rng, k, m)), ('exact', want)))
    # Planted logs on composite moduli up to 2^36, including shared factors.
    for it in range(count):
        m = 1
        while m < 2 or rng.random() < 0.6:
            q = rng.choice(SMALLP[:25]) if rng.random() < 0.7 else big_prime(rng, rng.randint(8, 20))
            if m * q ** 2 > 1 << 36: break
            m *= q ** rng.randint(1, 3) if m * q ** 3 <= 1 << 36 else q
        a, k = rng.randrange(m), rng.randrange(m)
        x = rng.randrange(4) if it % 4 == 0 else rng.randrange(60) if it % 2 else rng.randrange(1 << 20)
        b = k * pow(a, x, m) % m
        rows.append(('L', (shift(rng, a, m), b, m, k), ('planted', x)))
    # Orders modulo products of known primes.
    for it in range(count):
        mf, m = {}, 1
        for _ in range(rng.randint(1, 3)):
            kind, p, f = rng.choice(pool)
            if p in mf or m * p > HI: continue
            e = 1
            while rng.random() < 0.3 and m * p ** (e + 1) <= HI: e += 1
            mf[p] = (e, f); m *= p ** e
        if rng.random() < 0.3 and 2 not in mf and m * 8 <= HI:
            e2 = rng.randint(1, 3); mf[2] = (e2, {}); m *= 2 ** e2
        lam, primes = lambda_of(mf)
        a = rng.randrange(m)
        if it % 2: a = pow(rng.randrange(m), rng.choice(sorted(primes) or [1]), m)
        rows.append(('O', (shift(rng, a, m), m), ('order', primes, m, a, lam)))
    # Primitive roots: cyclic moduli p, p^e, 2 p^e, small cases, and non-cyclic products.
    for it in range(count):
        kind, p, f = rng.choice(pool)
        c = it % 6
        if c == 0 or p == 2: m, pf = p, set(f)
        elif c in (1, 2):
            m, e = p, 1
            while m * p * (2 if c == 2 else 1) <= HI and rng.random() < 0.7: m *= p; e += 1
            m *= 2 if c == 2 and m * 2 <= HI else 1
            pf = set(f) | ({p} if e > 1 else set())
        elif c == 3:
            m = rng.choice([1, 2, 4, 8, 16, 1 << 62])
            pf = {2} if m == 4 else set() if m < 4 else None
        else:
            q = rng.choice(pool)[1]
            m = p * q if p != q and p * q <= HI else 8 * p if 8 * p <= HI else 12
            pf = None
        cyclic = pf is not None
        g = rng.randint(-m, min(HI, 2 * m - 1)) if it % 2 else None
        rows.append(('P', (m, g if g is not None else 0), ('proot', cyclic, pf, m, g)))
    # Kronecker symbols for factored n, signed, with 2-parts and the 64-bit extremes.
    for it in range(count):
        nf, n = {}, 1
        for _ in range(rng.randint(0, 4)):
            kind, p, f = rng.choice(pool + [('two', 2, {})] * 5)
            e = rng.randint(1, 3)
            if n * p ** e <= HI: nf[p] = nf.get(p, 0) + e; n *= p ** e
        n = -n if rng.random() < 0.4 else n
        if it % 17 == 0: n, nf = LO, {2: 63}
        if it % 19 == 0: n, nf = 0, {}
        a = rng.choice([rng.randint(LO, HI), rng.randint(-50, 50), LO, HI, LO + 1])
        rows.append(('K', (a, n), ('kron', kron(a, n, nf))))
    odd = [(k, p, f) for k, p, f in pool if p > 2]
    # Legendre and Cipolla: planted squares, nonresidues z * x^2, zero.
    for it in range(count):
        kind, p, f = rng.choice(odd)
        x = rng.randrange(1, p)
        c = it % 3
        a = x * x % p if c == 0 else gens[p] * x * x % p if c == 1 else 0
        want = (1, min(x, p - x)) if c == 0 else (-1, -1) if c == 1 else (0, 0)
        rows.append(('S', (shift(rng, a, p), p), ('exact2', want)))
    # k-th roots: structured k against planted powers and planted non-powers z^j with gcd(k, p - 1) not dividing j.
    for it in range(count):
        kind, p, f = rng.choice(pool)
        qs = sorted(f)
        c = it % 5
        if c == 0: k = 0
        elif c == 1: k = rng.randint(0, HI)
        elif c == 2: k = (p - 1) * rng.randint(1, min(1000, HI // (p - 1)))
        else:
            q = rng.choice(qs)
            k = q ** rng.randint(1, f[q])
            k *= rng.randint(1, min(1000, HI // k))
            x = rng.choice(qs) ** rng.randint(0, 3)
            k *= x if c == 4 and k * x <= HI else 1
        g = math.gcd(k, p - 1)
        mode = rng.randrange(4)
        if mode == 0: a, exists = pow(rng.randrange(p), k, p), True
        elif mode == 1 and g > 1:
            j = rng.randrange(p - 1)
            j += 1 if j % g == 0 else 0
            a, exists = pow(gens[p], j, p), False
        elif mode == 2: a, exists = 0, k > 0
        else: a, exists = 1, True
        rows.append(('R', (shift(rng, a, p), k, p), ('kth', exists, k, p, a % p)))
    # Unit roots: random divisors of p - 1 and non-divisors.
    for it in range(count):
        kind, p, f = rng.choice(pool)
        nf = {q: rng.randint(0, e) for q, e in f.items()}
        n = value(nf)
        if it % 3 == 0: n = rng.randint(1, HI)
        rows.append(('U', (n, p), ('unit', n, p, set(f))))
    payload = ''.join(op + ' ' + ' '.join(map(str, a)) + '\n' for op, a, _ in rows)
    command = [str(binary), '--oracle']
    proc = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if proc.returncode: raise RuntimeError(f'command={command} returncode={proc.returncode}\n{proc.stderr}')
    lines = proc.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, a, want), line in zip(rows, lines):
        got = list(map(int, line.split()))
        tag = want[0]
        if tag == 'exact': ok = got == [want[1]]
        elif tag == 'planted':
            x, (aa, b, m, k) = want[1], a
            ok = 0 <= got[0] <= x and k * pow(aa, got[0], m) % m == b % m
        elif tag == 'order':
            primes, m, aa, lam = want[1:]
            ok = got == [-1] if math.gcd(aa, m) != 1 else lam % got[0] == 0 and char_order(aa % m, got[0], m, primes)
        elif tag == 'proot':
            cyclic, pf, m, g = want[1:]
            ex, r, isr = got
            if not cyclic: ok = got == [0, -1, 0]
            else:
                phi = m
                for q in set(trial_small(m)): phi = phi // q * (q - 1)
                prim = lambda h: math.gcd(h, m) == 1 and (m == 1 or all(pow(h, phi // q, m) != 1 for q in pf))
                ok = ex == 1 and 0 <= r < m and prim(r) and not any(prim(h) for h in range(1, r))
                ok &= isr == int(prim((g if g is not None else 0) % m))
        elif tag == 'kron': ok = got == [want[1]]
        elif tag == 'exact2': ok = got == list(want[1])
        elif tag == 'kth':
            exists, k, p, ar = want[1:]
            ok = got[0] == int(exists) and (got[1] == -1 if not exists else 0 <= got[1] < p and pow(got[1], k, p) == ar)
        elif tag == 'unit':
            n, p, pf = want[1:]
            if (p - 1) % n: ok = got == [-1]
            else: ok = 0 < got[0] < p and char_order(got[0], n, p, pf)
        if not ok: raise RuntimeError(f'smallest known reproducer={op} {a} expected={want} actual={got}')
    print(f'PASS exact Python oracle: {len(rows)} cases, {len(pool)} constructed primes seed={args.seed}', flush=True)


def trial_small(m):
    """Distinct primes of m when m is p^e, 2 p^e or tiny (p from the pool is the largest factor)."""
    res, d = [], 2
    while m % 2 == 0: res.append(2); m //= 2
    for e in range(1, 64):
        r = round(m ** (1 / e))
        for c in (r - 1, r, r + 1):
            if c > 1 and c ** e == m and is_prime(c): return res + [c]
    return res if m == 1 else res + list(trial(m))


if __name__ == '__main__':
    raise SystemExit(main('11-discrete_log_root', ['order-mod', 'log-mod', 'monoid-range', 'monoid-negative', 'exists-mod',
        'isroot-mod', 'root-mod', 'legendre-two', 'legendre-even', 'cipolla-mod', 'kth-negative', 'kthexists-mod',
        'unit-order', 'unit-mod'], oracle))
