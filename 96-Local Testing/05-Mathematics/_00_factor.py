"""The mathematics Python oracles' exact primality and factorization."""
from functools import lru_cache
import math
import random


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
