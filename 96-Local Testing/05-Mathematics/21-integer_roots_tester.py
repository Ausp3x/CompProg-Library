"""Integer square, cube and k-th roots, rounding and perfect powers against brute force and Python integers.

C++ (optimized and checked builds in every mode, ASan/UBSan outside quick): explicit values
(2^64 - 1 for k = 1..5, 40, 63, 64, 109, 110, 10^9, 2^31 - 1; 2^63 and 2^63 - 1 at k = 63;
isqrt128 at 2^64 and (2^64 - 1)^2 neighbours and 2^128 - 1; icbrt at INT64_MIN, -9, -8,
INT64_MAX; isPerfectPower on 2^63, 3^40, 6^24, 7^22, prime squares). Exhaustive n < 2^16
(quick 2^12, stress 2^18) for every k <= 20 and n < 2^12 (quick 2^8, stress 2^14) for
k = 21..70, 100, 109, 110, 127..129, 200, 10^9, 2^31 - 1 against roots grown by
incrementing and big-integer rounding thresholds; isqrt, isqrtRemainder, isPerfectSquare,
isqrt128 and isPerfectPower exhaustively on n < 2^22 (quick 2^16, stress 2^23) and icbrt on
|n| <= the same bound. r^k - 1, r^k, r^k + 1 for every r >= 2 with r^k < 2^64 and
k = 3..63 (quick: first and last 4096 roots), k = 2 for the first and last 2^18 roots
(quick 2^10, stress 2^20) plus random roots, with icbrt at +-(r^3 + {-1, 0, 1}); rounding
thresholds floor((r + 1/2)^k) and + 1 for k <= 200 (k <= 3: first and last 2^16 roots plus
random, quick 2^8); isqrt128 at s^2 - 1, s^2, s^2 + 1, s^2 + 2s around 2^32, below 2^64 and
for random s; isPerfectPower on b^e (expected c^(f e) for b = c^f) and its neighbours
against a binary-search exponent search. Seeded random rounds (quick 5000, full 200000,
stress 600000) of every bit width and near powers: k-th root certificates
r^k <= n < (r + 1)^k, the big-integer rounding oracle, sqrt, 128-bit and signed cube
certificates. Assertion probes: k = 0 and k = -1 for ikthRoot, ceilKthRoot, kthRootRound.
Python oracle (quick 300, full 3000, stress 20000 rows): exact integer roots by binary
search, rounding by 2^k n > (2r + 1)^k, signed cube root by binary search, perfect powers by
exponent search, math.isqrt for 128-bit inputs; validated first against linear scans,
an enumerated perfect-power table and 60-digit Decimal roots (once per run).
"""
from decimal import Decimal, ROUND_FLOOR, getcontext
import math
import random
import shlex
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run

TOP = (1 << 64) - 1
LO, HI = -(1 << 63), (1 << 63) - 1
BIG_K = (65, 100, 109, 110, 127, 128, 129, 200, 10**9, (1 << 31) - 1)
# For k >= KCAP every n < 2^64 has root r <= 1 and 1.5^k >= 1.5^KCAP > 2^64 > n, so rounding equals that at KCAP.
KCAP = 256
VALIDATED = []
assert 3 ** KCAP > (1 << 64) << KCAP


def iroot(n, k):
    """floor(n^(1/k)) for n >= 0, k >= 1 by binary search on exact powers."""
    if k == 1:
        return n
    if n.bit_length() <= k:
        return int(n > 0)
    lo, hi = 0, 1 << (n.bit_length() // k + 1)
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if mid ** k <= n:
            lo = mid
        else:
            hi = mid - 1
    return lo


def kth_round(n, k):
    r = iroot(n, k)
    k = min(k, KCAP)
    return r if r == 0 else r + ((2 * r + 1) ** k < n << k)


def icbrt(s):
    lo, hi = -(1 << 22), 1 << 22
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if mid ** 3 <= s:
            lo = mid
        else:
            hi = mid - 1
    return lo


def perfect_power(n):
    for e in range(63, 1, -1):
        b = iroot(n, e)
        if b > 1 and b ** e == n:
            return b, e
    return n, 1


def expected(row):
    n, k, m, s = row
    q = math.isqrt(n)
    r = iroot(n, k)
    b, e = perfect_power(n)
    return tuple(map(str, (q, q, n - q * q, int(q * q == n), r, r + (r ** k != n), kth_round(n, k), b, e, math.isqrt(m), icbrt(s))))


def decimal_round(n, k):
    x = Decimal(n) ** (Decimal(1) / Decimal(k))
    return int((x + Decimal('0.5')).to_integral_value(rounding=ROUND_FLOOR))


def validate_oracle(rng):
    getcontext().prec = 60
    table = {}
    for b in range(2, 72):
        p, e = b * b, 2
        while p < 5000:
            if e > table.get(p, (0, 1))[1]:
                table[p] = (b, e)
            p, e = p * b, e + 1
    for n in range(5000):
        assert perfect_power(n) == table.get(n, (n, 1)), n
    for k in (*range(1, 13), 65, 100, 300):
        r = 0
        for n in range(5000):
            while (r + 1) ** k <= n:
                r += 1
            assert iroot(n, k) == r, (n, k)
            assert n >= 1000 or kth_round(n, k) == decimal_round(n, k), (n, k)
    for _ in range(300):
        n, k = rng.getrandbits(rng.randint(1, 64)), rng.randint(2, 64)
        assert kth_round(n, k) == decimal_round(n, k), (n, k)
    c = -20
    for s in range(-5000, 5001):
        while (c + 1) ** 3 <= s:
            c += 1
        assert icbrt(s) == c, s


def clamp(x, hi):
    return min(max(x, 0), hi)


def make_row(rng):
    t = rng.randrange(8)
    k = rng.randint(1, 64) if t < 6 else rng.randint(65, 200) if t == 6 else rng.choice(BIG_K)
    t, kk = rng.randrange(6), rng.randint(2, 63)
    rmax = iroot(TOP, kk)
    r = clamp(rng.getrandbits(rng.randint(1, rmax.bit_length())), rmax) or 1
    if t == 0:
        n = rng.choice([0, 1, 2, 3, TOP, TOP - 1, 1 << 63, (1 << 63) - 1, 1 << 32, (1 << 32) - 1])
    elif t <= 2:
        n = rng.getrandbits(rng.randint(1, 64))
    elif t == 3:
        n = clamp(r ** kk + rng.randint(-2, 2), TOP)
    elif t == 4:
        j = min(k, 200) if k > 1 else 2
        base = 1 if j > 63 else clamp(r, iroot(TOP, j))
        n = clamp(((2 * base + 1) ** j >> j) + rng.randint(-1, 2), TOP)
    else:
        n = clamp(r ** kk + rng.choice([-1, 0, 0, 1]), TOP)
    t = rng.randrange(4)
    if t == 0:
        m = rng.choice([0, 1, TOP, 1 << 64, (1 << 128) - 1, TOP * TOP, TOP * TOP - 1, (1 << 126) - 1])
    elif t == 1:
        m = rng.getrandbits(rng.randint(1, 128))
    else:
        q = rng.getrandbits(rng.randint(1, 64))
        m = clamp(q * q + rng.choice([-1, 0, 1, 2 * q]), (1 << 128) - 1)
    t = rng.randrange(4)
    if t == 0:
        s = rng.choice([LO, LO + 1, HI, HI - 1, -9, -8, -1, 0, 1, 8])
    elif t == 1:
        s = rng.getrandbits(rng.randint(1, 64)) - (1 << 63)
        s = s >> rng.randint(0, 63)
    else:
        c = rng.randint(0, 2097151)
        s = rng.choice([1, -1]) * c ** 3 + rng.randint(-1, 1)
    return n, k, m, s


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    if not VALIDATED:
        validate_oracle(random.Random(args.seed))
        VALIDATED.append(True)
    count = {'quick': 300, 'full': 3000, 'stress': 20000}[args.mode]
    rows = [make_row(rng) for _ in range(count)]
    command = [str(binary), '--oracle']
    result = timed_run(f'21-integer_roots oracle {Path(command[0]).name}', command, input=''.join(' '.join(map(str, row)) + '\n' for row in rows),
                                                  text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    for i, (row, line) in enumerate(zip(rows, actual)):
        want, got = expected(row), tuple(line.split())
        if got != want:
            raise RuntimeError(f'Python integer oracle seed={args.seed} failing row #{i} (n, k, n128, signed n)={row}\n'
                               f'fields=isqrt root rem isPerfectSquare ikthRoot ceilKthRoot kthRootRound base exp isqrt128 icbrt\n'
                               f'expected={want}\nactual={got}\ncommand={shlex.join(command)}')
    print(f'PASS Python exact-integer oracle (validated by brute force) cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('21-integer_roots', ['ikthroot-zero', 'ikthroot-negative', 'ceilkthroot-zero', 'ceilkthroot-negative',
                                               'kthrootround-zero', 'kthrootround-negative'], oracle))
