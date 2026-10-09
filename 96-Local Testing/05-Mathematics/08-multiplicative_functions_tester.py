"""Multiplicative functions against independent C++ brute force and Python exact integers.

C++: every n through 3000 (quick), 30000 (full) or 150000 (stress) for phi, mobius, omega,
bigOmega, liouville against trial factorization; phi by gcd counting and carmichaelLambda by
brute multiplicative orders through 400, carmichaelLambda exponent and minimality over all
units through 3000; sigmaK (k = 0..3, exact and mint with a huge exponent) by divisor sums and
jordanTotient by the inversion sum(J_k(d)) = n^k through 2000 plus pair counting through 60.
All six tables at sizes 0, 1, 2, 3 and N against trial-factor values and multiple sieves;
multiplicativeTable with a salted hash and a completely multiplicative mint power. Dirichlet
convolution against the O(n^2) divisor sum (300 / 3000 / 12000), inverses checked by
convolution to the identity (wrapping ulng with a[1] = 1 and mint), and the inverse of 1 is the
Mobius table. DivisorArray on fixed and 40 / 400 random n up to 2^64 (103680 divisors for
897612484786617600): build overloads, index/get, zeta and multiple zeta against brute sums
when d(n) <= 3000, inverse round trips, setMultiplicative (phi with zeta gives identity;
Mobius against trial factors). Python oracle: n built from known primes, checked against
closed forms. 10 assertion probes run in the checked build.
"""
import random
import subprocess
from math import lcm
from _00_runner import main

MOD = 998244353


def is_prime(n):
    if n < 2: return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41):
        if n % p == 0: return n == p
    d, s = n - 1, 0
    while d % 2 == 0: d //= 2; s += 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41):
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
            x = rng.getrandbits(bits) | (1 << (bits - 1))
            if bits == 1: x = 2
            if is_prime(x): return x
    cases = []
    for _ in range(count):
        f, n = {}, 1
        for _ in range(rng.randint(1, 8)):
            p = prime(rng.randint(1, 40))
            e = rng.randint(1, 5)
            if n * p ** e > top: continue
            n *= p ** e; f[p] = f.get(p, 0) + e
        cases.append((n, f))
    cases.append((1, {}))
    payload = ''.join(f'{n}\n' for n, _ in cases)
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(cases): raise RuntimeError(f'expected {len(cases)} lines, got {len(lines)}')
    w = 1 << 128
    for (n, f), line in zip(cases, lines):
        got = list(map(int, line.split()))
        phi, mu, lam = n, 1, 1
        for q, e in f.items():
            phi = phi // q * (q - 1)
            mu = 0 if e > 1 else -mu
            v = (q - 1) * q ** (e - 1)
            lam = lcm(lam, v // 2 if q == 2 and e >= 3 else v)
        big = sum(f.values())
        def sigma(k, m=None):
            r = 1
            for q, e in f.items(): r *= sum(q ** (k * j) if m is None else pow(q, k * j, m) for j in range(e + 1))
            return r if m is None else r % m
        def jordan(k, m=None):
            r = 1
            for q, e in f.items():
                if m is None: r *= q ** (k * (e - 1)) * (q ** k - 1)
                else: r = r * pow(q, k * (e - 1), m) * (pow(q, k, m) - 1) % m
            return r
        want = [phi, mu, len(f), big, -1 if big % 2 else 1, lam, sigma(0), sigma(1) % w, sigma(3) % w, jordan(2) % w,
                sigma(10 ** 18, MOD), jordan(123456789, MOD)]
        if got != want: raise RuntimeError(f'smallest known reproducer n={n} factors={f}, expected={want}, actual={got}')
    print(f'PASS exact Python oracle: {len(cases)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('08-multiplicative_functions', ['phi-zero', 'lambda-zero', 'sigma-zero', 'table-negative',
        'table-max', 'conv-size', 'conv-empty', 'inverse-small', 'index-zero', 'index-nondivisor'], oracle))
