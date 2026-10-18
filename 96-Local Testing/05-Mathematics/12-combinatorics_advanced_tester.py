"""Advanced combinatorics against Pascal, DP tables, enumeration, congruences and exact Python integers.

C++ (every mode, optimized, checked and, outside quick, ASan/UBSan builds): factorialValuation against
summed valuations and kummerCarries against Legendre and digit sums (128-bit digit sums at p = 2^64 - 1);
binomialLucas, BinomialSmallPrimeTable and BinomialPrimePower (e = 1 and prime powers up to 2^20),
BinomialCompositeMod (m up to 2^31 - 1) and binomialRowCompositeMod (m up to 2^63 - 1) against Pascal's
triangle modulo m; binomialPrefixSum for weights 1, 2, -1, 0 and random, Mo's queries and the block table
against Pascal sums; factorial powers against C(x, n) n! and 64-bit wraparound; Stirling rows and
columns against the recurrences modulo 998244353, 1000003 and the composite 10^6 (rows), StirlingSmallPrime
against DP for n <= 400 and the explicit sum for 64-bit n; bellTable against the Bell triangle,
bellNumberLarge against the triangle modulo small primes, B_(p^m) = m + 1 and the period (p^p - 1) / (p - 1);
Eulerian rows and numbers against the recurrence and the closed forms for k <= 2 at 64-bit n; Entringer
numbers against alternating permutations, Lah numbers, surjections and bounded tuples against recurrences
and polynomial powers; partitions against coin-change DP (also modulo 2^64), enumeratePartitions against
a recursive generator in order; QCombinatorics against q-Pascal for every q modulo primes <= 17 and 101;
the four transforms against direct sums and round trips; inclusionExclusion pruned and unpruned against a
scan; subsetSumCount against knapsack; pisanoPeriod against simulation for m <= 20000; Zeckendorf validity;
indecomposable permutations against the first-block recurrence and enumeration. quick shrinks every
bound; stress repeats the randomized sections three times.
Python oracle (quick 60, full 400, stress 2000 rows): math.comb for indices up to 10^18 (Lucas, prime
powers, composites, rows, weighted prefix sums), exact v_p(C(n, k)), Eulerian and surjection DP tables,
and Pisano periods certified by F_pi = 0, F_(pi + 1) = 1 and non-periodicity of every pi / q.
"""
from math import comb
import random
import shlex
import subprocess
from _00_runner import main

P = 998244353
SMALL = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101, 103, 107, 109, 113]


def sieve(n):
    flags = bytearray([1]) * (n + 1)
    flags[0:2] = b'\x00\x00'
    for i in range(2, int(n ** 0.5) + 1):
        if flags[i]:
            flags[i * i::i] = bytearray(len(flags[i * i::i]))
    return [i for i in range(n + 1) if flags[i]]


PRIMES = sieve(1 << 20)


def fib(n, m):
    a, b = 0, 1 % m
    for bit in bin(n)[2:] if n else '':
        c, d = a * (2 * b - a) % m, (a * a + b * b) % m
        a, b = (d, (c + d) % m) if bit == '1' else (c, d)
    return a, b


def factor_small(n):
    res = []
    for p in PRIMES:
        if p * p > n:
            break
        if n % p == 0:
            e = 0
            while n % p == 0:
                n //= p
                e += 1
            res.append(p)
    if n > 1:
        res.append(n)
    return res


def vp(x, p):
    v = 0
    while x and x % p == 0:
        x //= p
        v += 1
    return v


def eulerian_table(n, k):
    a = [[0] * (k + 1) for _ in range(n + 1)]
    a[0][0] = 1
    for i in range(1, n + 1):
        for j in range(min(i - 1, k) + 1):
            a[i][j] = ((j + 1) * a[i - 1][j] + ((i - j) * a[i - 1][j - 1] if j else 0)) % P
    return a


def surj_table(n, k):
    s = [[0] * (k + 1) for _ in range(n + 1)]
    s[0][0] = 1
    for i in range(1, n + 1):
        for j in range(1, min(i, k) + 1):
            s[i][j] = j * (s[i - 1][j] + s[i - 1][j - 1]) % P
    return s


def validate_oracle():
    for m in range(1, 300):
        a, b, n = 0, 1 % m, 0
        while True:
            a, b, n = b, (a + b) % m, n + 1
            if a == 0 and b == 1 % m:
                break
        assert fib(n, m) == (0, 1 % m) and all(fib(n // q, m) != (0, 1 % m) for q in factor_small(n)), m
    assert factor_small(2 * 3 * 3 * 1000003) == [2, 3, 1000003]


def oracle(binary, args, env):
    validate_oracle()
    rng = random.Random(args.seed)
    count = {'quick': 60, 'full': 400, 'stress': 2000}[args.mode]
    rows, want = [], []
    et, st = eulerian_table(1500, 30), surj_table(1500, 40)
    for i in range(count):
        c = i % 7
        if c == 0:
            p = rng.choice(SMALL + [1009, 10007])
            e = 1
            while p ** (e + 1) <= 200000 and rng.random() < 0.6:
                e += 1
            m = rng.randint(1, 200000)
            n = rng.choice([rng.randint(0, 3000), rng.getrandbits(rng.randint(1, 63))])
            k = rng.choice([rng.randint(0, 30), max(0, n - rng.randint(0, 30)), rng.randint(0, n)] if n <= 3000 else [rng.randint(0, 30), n - rng.randint(0, min(n, 30))])
            b = comb(n, k)
            rows.append(f'binom {n} {k} {p} {e} {m}')
            want.append((b % p, b % p ** e, b % m))
        elif c == 1:
            n = rng.randint(0, 2500)
            m = rng.choice([rng.randint(1, 1000), rng.getrandbits(63) or 1, (1 << 63) - 1, 2 ** 20 * 3 ** 10])
            h = 0
            for k in range(n + 1):
                h = (h * 1000003 + comb(n, k) % m) % (1 << 64)
            rows.append(f'row {n} {m}')
            want.append((h,))
        elif c == 2:
            n = rng.choice([rng.randint(0, 200), rng.getrandbits(rng.randint(1, 64))])
            k = rng.randint(-1, 200) if n > 200 else rng.randint(-1, n + 3)
            r = rng.choice([1, 2, P - 1, 0, rng.randrange(P)])
            s = sum(pow(r, j, P) * (comb(n, j) % P) for j in range(0, min(k, n) + 1)) % P if k >= 0 else 0
            rows.append(f'prefix {n} {k} {r}')
            want.append((s,))
        elif c == 3:
            p = rng.choice(SMALL[:10] + [rng.randint(2, 1000)])
            n = rng.randint(0, 5000)
            k = rng.randint(0, n)
            if all(p % q for q in SMALL if q < p):
                v = vp(comb(n, k), p)
                leg = sum(n // p ** j for j in range(1, 64) if p ** j <= n)
                rows.append(f'val {n} {k} {p}')
                want.append((leg, v))
            else:
                rows.append(f'val {n} {k} 2')
                want.append((sum(n // 2 ** j for j in range(1, 64) if 2 ** j <= n), vp(comb(n, k), 2)))
        elif c == 4:
            ps = rng.sample(PRIMES[1:20000] if rng.random() < 0.5 else PRIMES[:30], rng.randint(1, 4))
            m = 1
            for q in ps:
                for _ in range(rng.randint(1, 3)):
                    if m * q <= 3 * 10 ** 18:
                        m *= q
            pi = int(subprocess.run([str(binary), '--oracle'], input=f'pisano {m}\n', text=True, capture_output=True, env=env, timeout=60).stdout.split()[0])
            ok = 1 <= pi <= 6 * m and fib(pi, m) == (0, 1 % m) and all(fib(pi // q, m) != (0, 1 % m) for q in factor_small(pi))
            if not ok:
                raise RuntimeError(f'Pisano certificate failed m={m} pi={pi} seed={args.seed}')
            rows.append(f'pisano {m}')
            want.append((pi,))
        elif c == 5:
            n, k = rng.randint(0, 1500), rng.randint(-1, 30)
            rows.append(f'eulerian {n} {k}')
            want.append((et[n][k] if 0 <= k and (k < n or n == k == 0) else 0,))
        else:
            n, k = rng.randint(0, 1500), rng.randint(0, 40)
            rows.append(f'surj {n} {k}')
            want.append((st[n][k],))
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
    print(f'PASS Python exact-integer oracle and Pisano certificates (validated by simulation) cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('12-combinatorics_advanced', [
        'valuation-p', 'carries-k', 'lucas-digit', 'small-table-n', 'prime-power-q', 'prime-power-composite', 'composite-zero', 'row-m', 'prefix-k',
        'queries-n', 'table-n', 'rising-n', 'stirling-small-prime', 'bell-composite', 'q-table', 'partitions-negative',
        'enumerate-negative', 'subset-negative', 'pisano-zero', 'surjection-k'], oracle))
