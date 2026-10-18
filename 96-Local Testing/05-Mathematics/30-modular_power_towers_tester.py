"""Power towers, iterated powers and modulus chains against brute force and an independent Python oracle.

C++ (every mode, optimized, checked and, outside quick, ASan/UBSan builds): eulerChain, carmichaelChain
and towerThreshold for every m <= 400 (quick 120) against brute phi, lambda as the lcm of unit orders and
the least t with a^t = a^(t + lambda) for all a; every tower of length <= 3 over bases < 6 (quick 4) for
m <= 90 (quick 40), then seeded towers of length up to 40 with zeros, ones, small and 64-bit bases for
m <= 3000, against a cycle oracle that simulates each base's powers mod m (preperiod and period) without
Euler's or Carmichael's theorem, through both chains and the iterated and tetration forms; exact values
of short towers against 128-bit saturating evaluation (powerTowerCapped for caps 0 to 2^64 - 1, powerTowerMod
for any 64-bit modulus); counts up to 2^64 - 1 against the same parity at depth 150 and across both chains;
edge cases (m = 1, empty tower, 0^0 = 1, cap 0, a 100000-long zero run). Rounds: quick 300, full 4000,
stress 30000.
Python oracle (quick 100, full 800, stress 4000 rows): Chinese remaindering over the prime powers of m,
Euler's theorem for coprime bases and exact valuations for the others, with Pollard rho factorization and
cycle detection for counts up to 2^64 - 1; validated first against exact big integers and brute cycles.
"""
from functools import lru_cache
import math
import random
import shlex
import subprocess
from _00_runner import main

U64 = (1 << 64) - 1


def is_prime(n):
    if n < 2:
        return False
    small = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)
    for p in small:
        if n % p == 0:
            return n == p
    d, s = n - 1, 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for a in small:
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


def rho(n, rng):
    if n % 2 == 0:
        return 2
    while True:
        c, y, g, r, q = rng.randrange(1, n), rng.randrange(n), 1, 1, 1
        while g == 1:
            x = y
            for _ in range(r):
                y = (y * y + c) % n
            k = 0
            while k < r and g == 1:
                ys = y
                for _ in range(min(128, r - k)):
                    y = (y * y + c) % n
                    q = q * abs(x - y) % n
                g = math.gcd(q, n)
                k += 128
            r *= 2
        if g == n:
            g = 1
            while g == 1:
                ys = (ys * ys + c) % n
                g = math.gcd(abs(x - ys), n)
        if g != n:
            return g


@lru_cache(maxsize=None)
def factor(n):
    """Prime factorization of n >= 1 as a sorted tuple of (p, e)."""
    res, stack, rng = {}, [n], random.Random(n)
    while stack:
        m = stack.pop()
        if m == 1:
            continue
        for p in (2, 3, 5, 7, 11, 13):
            while m % p == 0:
                res[p] = res.get(p, 0) + 1
                m //= p
        if m == 1:
            continue
        if is_prime(m):
            res[m] = res.get(m, 0) + 1
            continue
        d = rho(m, rng)
        stack += [d, m // d]
    return tuple(sorted(res.items()))


def phi(n):
    for p, _ in factor(n):
        n = n // p * (p - 1)
    return n


def lam(n):
    res = 1
    for p, e in factor(n):
        v = p ** (e - 1) * (p - 1)
        res = math.lcm(res, v // 2 if p == 2 and e >= 3 else v)
    return res


def chain(m, f):
    res = [m]
    while res[-1] > 1:
        res.append(f(res[-1]))
    return res


def capped_pow(b, e, cap):
    """min(b^e, cap) for cap >= 1, 0^0 = 1."""
    if e == 0:
        return 1
    if b <= 1:
        return min(b, cap)
    if e >= 70:
        return cap
    return min(b ** e, cap)


def capped_iter(a, cnt, top, cap):
    """min of a^a^...^a^top (cnt copies) and cap, iterating v -> min(a^v, cap) with cycle detection."""
    v, seen, step = min(top, cap), {}, 0
    while step < cnt:
        if v in seen:
            for _ in range((cnt - step) % (step - seen[v])):
                v = capped_pow(a, v, cap)
            return v
        seen[v] = step
        v = capped_pow(a, v, cap)
        step += 1
    return v


def crt(parts):
    x, m = 0, 1
    for r, q in parts:
        t = (r - x) * pow(m, -1, q) % q
        x, m = x + m * t, m * q
    return x


def tower(base, length, top, m):
    """base(i)^base(i + 1)^...^top modulo m for i = 0, with length levels below top."""
    @lru_cache(maxsize=None)
    def capped(i, cap):
        return capped_iter(base(i), length - i, top, cap) if base.constant else capped_iter_list(i, cap)

    def capped_iter_list(i, cap):
        v = min(top, cap)
        for j in range(length - 1, i - 1, -1):
            v = capped_pow(base(j), v, cap)
        return v

    @lru_cache(maxsize=None)
    def go(i, m):
        if m == 1:
            return 0
        if i == length:
            return top % m
        b, parts = base(i), []
        for p, e in factor(m):
            q = p ** e
            if b % p:
                r = pow(b, go(i + 1, q // p * (p - 1)), q)
            elif b == 0:
                r = (1 if capped(i + 1, 1) == 0 else 0) % q
            else:
                v, x = 0, b
                while x % p == 0:
                    x //= p
                    v += 1
                need = -(-e // v)
                big = capped(i + 1, need)
                r = 0 if big >= need else pow(b, big, q)
            parts.append((r, q))
        return crt(parts)
    return go(0, m)


class ListBase:
    constant = False

    def __init__(self, a):
        self.a = a

    def __call__(self, i):
        return self.a[i]


class ConstBase:
    constant = True

    def __init__(self, a):
        self.a = a

    def __call__(self, i):
        return self.a


def exact(a, top=1):
    v = top
    for b in reversed(a):
        v = b ** v
    return v


def validate_oracle():
    rng = random.Random(5)
    for _ in range(3000):
        a = [rng.choice([0, 1, 2, 3, 4, 5, 6, 7]) for _ in range(rng.randint(0, 3))]
        try:
            v = exact(a) if all(x <= 3 for x in a) or len(a) <= 2 else None
        except (OverflowError, MemoryError):
            v = None
        if v is not None and v.bit_length() > 100000:
            v = None
        m = rng.randint(1, 500)
        got = tower(ListBase(a), len(a), 1, m)
        if v is not None:
            assert got == v % m, (a, m)
        # brute cycles of the bottom base when the exponent tower is exact
        if len(a) >= 2 and all(x <= 3 for x in a[1:]):
            e = exact(a[1:])
            if e.bit_length() < 64:
                assert got == pow(a[0], e, m), (a, m)
        if a and all(x == a[0] for x in a):
            assert tower(ConstBase(a[0]), len(a), 1, m) == got, (a, m)
    for n in range(1, 400):
        assert phi(n) == sum(math.gcd(i, n) == 1 for i in range(n)) or n == 1
        assert factor(n) and math.prod(p ** e for p, e in factor(n)) == n or n == 1


def oracle(binary, args, env):
    validate_oracle()
    rng = random.Random(args.seed)
    count = {'quick': 100, 'full': 800, 'stress': 4000}[args.mode]

    def modulus():
        c = rng.randrange(6)
        if c == 0:
            return rng.randint(1, 3000)
        if c == 1:
            while True:
                p = rng.getrandbits(rng.randint(20, 62)) | 1
                if is_prime(p):
                    return p
        if c == 2:
            m = 1
            for _ in range(rng.randint(1, 4)):
                q = rng.choice([2, 3, 5, 7, 11, 13]) ** rng.randint(1, 6)
                m = m * q if m * q <= U64 else m
            return m
        if c == 3:
            return U64 - rng.randrange(100)
        return max(1, rng.getrandbits(64) >> rng.randrange(64))

    def base():
        c = rng.randrange(8)
        return rng.randrange(3) if c < 2 else rng.randrange(30) if c < 5 else U64 - rng.randrange(3) if c == 5 else rng.getrandbits(64)

    rows, want = [], []
    for i in range(count):
        m = modulus()
        if i % 3 == 0:
            a = [base() for _ in range(rng.randint(0, 10))]
            rows.append(f'L {m} {len(a)} ' + ' '.join(map(str, a)))
            v = tower(ListBase(a), len(a), 1, m)
            want.append((v, v))
        elif i % 3 == 1:
            a, x0 = base(), rng.choice([0, 1, 2, rng.getrandbits(64)])
            k = rng.choice([0, 1, 2, 3, rng.randint(4, 100), U64, U64 - 1, rng.getrandbits(64)])
            rows.append(f'I {m} {a} {k} {x0}')
            v = tower(ConstBase(a), k, x0, m)
            want.append((v, v))
        else:
            rows.append(f'C {m}')
            e, c = chain(m, phi), chain(m, lam)
            t = max((x for _, x in factor(m)), default=0)
            want.append(tuple(map(str, e)) + ('|',) + tuple(map(str, c)) + ('|', t))
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
    print(f'PASS Python prime-power CRT oracle (validated by exact values and brute cycles) cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('30-modular_power_towers', ['euler-zero', 'carmichael-zero', 'threshold-zero', 'chain-end',
                                                      'chain-empty', 'chain-order'], oracle))
