"""Primality and factorization against independent C++ brute force and Python exact integers.

Every check of millerRabin64, isPrime, pollardRhoBrent and factorize also runs on its
dependency-free Compact twin (no Montgomery), against the same independent oracles.
C++ (all modes): exhaustive primality of every n through 100000 (quick), 2000000 (full) or
10000000 (stress) against a separate sieve for isPrime, millerRabin64 and isPrimeSpf (table
below n/2, fallback above), isPrimeTrial through the factor bound; every n through 20000 /
200000 / 1000000 for factorize, factorizeTrial, primeFactors, divisors (both overloads),
divisorCount, divisorSum, isPrimePower, isSquarefree, radical, pollardRhoBrent and
maxDivisorCount (every n through 20000, then every 101st) against trial division, a separate
divisor enumeration and a divisor-count sieve; every divisor of each Miller-Rabin base, named
strong pseudoprimes and Carmichael numbers, Chernick Carmichael numbers (30 / 400), 32-bit
prime squares and semiprimes, the three largest 64-bit primes, all prime powers of seven
bases, rho budget/seed reproducibility, 200 / 3000 / 20000 random coprimeBase lists (up to 7 factors per value) against
structural properties, and the known highly composite answers for 10^18 and 2^64 - 1.
Python oracle: random and structured 64-bit inputs (semiprimes of 20- to 32-bit primes,
prime powers, Carmichael products, smooth numbers with full divisor lists only when d(n) <= 4096,
boundaries) checked with a separate
13-base Miller-Rabin, exact product reconstruction and divisor generation from the verified
factorization. 11 assertion probes run in the checked build.
"""
import random
import subprocess
from _00_runner import main

BASES = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41)


def is_prime(n):
    """Deterministic for n < 3.3e24 with the first 13 prime bases."""
    if n < 2: return False
    for p in BASES:
        if n % p == 0: return n == p
    d, s = n - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in BASES:
        x = pow(a, d, n)
        if x in (1, n - 1): continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1: break
        else: return False
    return True


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 300, 'full': 4000, 'stress': 20000}[args.mode]
    top = (1 << 64) - 1
    def prime(bits):
        while True:
            x = rng.getrandbits(bits) | (1 << (bits - 1)) | 1
            if is_prime(x): return x
    rows = []
    edge = [0, 1, 2, 3, 4, top, top - 1, top - 58, 1 << 63, (1 << 63) - 1, (1 << 62) + 1, (1 << 32) - 5, (1 << 32) + 15, 4759123141]
    for n in edge:
        rows.append(('p', n))
        if n: rows.append(('f', n))
    for _ in range(count):
        rows.append(('p', rng.getrandbits(64)))
        rows.append(('p', rng.getrandbits(rng.randint(2, 64))))
        rows.append(('f', rng.getrandbits(64)))
        rows.append(('t', rng.getrandbits(36)))
        p, q = prime(rng.randint(20, 32)), prime(rng.randint(20, 32))
        rows.append(('f', p * q)); rows.append(('p', p * q))
        p = prime(rng.randint(2, 32)); k = rng.randint(1, 64)
        while p ** k > top: k -= 1
        rows.append(('d', p ** k))
        n, cnt = 1, {}
        while True:
            x = rng.choice((2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71))
            if n * x > top: break
            n *= x; cnt[x] = cnt.get(x, 0) + 1
        d = 1
        for e in cnt.values(): d *= e + 1
        rows.append(('d' if d <= 4096 else 'f', n))
        a, b = rng.getrandbits(16) | 1, prime(rng.randint(2, 40))
        rows.append(('d', a * b if a * b <= top else b))
    payload = ''.join(f'{op} {n}\n' for op, n in rows)
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, n), line in zip(rows, lines):
        got = list(map(int, line.split()))
        if op == 'p':
            want = is_prime(n)
            proper = lambda d: d == 0 if (want or n < 4) else 1 < d < n and n % d == 0
            ok = len(got) == 6 and got[:2] == got[3:5] == [want, want] and proper(got[2]) and got[5] == got[2]
        elif op == 't': ok = got == [is_prime(n)]
        else:
            kc = got[0]
            fc = [(got[1 + 2 * i], got[2 + 2 * i]) for i in range(kc)]
            got = got[1 + 2 * kc:]
            k = got[0]
            f = [(got[1 + 2 * i], got[2 + 2 * i]) for i in range(k)]
            rest = got[1 + 2 * k:]
            prod = 1
            for q, e in f: prod *= q ** e
            ok = fc == f and prod == n and all(is_prime(q) and e > 0 for q, e in f) and [q for q, _ in f] == sorted({q for q, _ in f})
            if ok:
                cnt, sig, rad = 1, 1, 1
                for q, e in f: cnt *= e + 1; sig *= (q ** (e + 1) - 1) // (q - 1); rad *= q
                pf = [q for q, e in f for _ in range(e)]
                want = [cnt, sig, f[0][0] if k == 1 else 0, int(all(e == 1 for _, e in f)), rad, len(pf)] + pf
                if op == 'd':
                    ds = [1]
                    for q, e in f: ds = [d * q ** j for d in ds for j in range(e + 1)]
                    want += [len(ds)] + sorted(ds)
                ok = rest == want
        if not ok: raise RuntimeError(f'smallest known reproducer={op} {n}, actual={got}')
    print(f'PASS exact Python oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('07-primality_factorization', ['factorize-zero', 'compact-zero', 'trial-zero', 'primefactors-zero',
        'divisors-zero', 'count-zero', 'sum-zero', 'squarefree-zero', 'radical-zero', 'base-zero', 'hcn-zero'], oracle))
