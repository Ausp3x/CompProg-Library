"""Chinese remaindering against brute enumeration and Python exact integers.

C++: crt2, CrtIncremental and crtMod for every pair of moduli through 12 (quick), 24 (full)
or 28 (stress) and residues across three periods, against enumeration of [0, lcm); 2000 /
30000 / 150000 random systems of 0..4 congruences with moduli <= 12 (half of them
consistent by construction) for crt, crtMod, crtScaled with signed coefficients,
superChiRemThm, garner and garnerDigits (status on shared factors, untouched output on
failure, digit ranges and mixed-radix reconstruction); empty systems, lcm overflow, an
inconsistency detected before overflow, extreme residues, sticky failure of CrtIncremental,
and signedRepresentative for m <= 50 and |x| <= 200 against enumeration plus 128-bit
extremes. Python oracle: systems with moduli up to 2^62 built from shared random primes,
consistent or perturbed, checked against exact big-integer CRT including the overflow flag,
the target-modulus answers and the digit expansion. 9 assertion probes.
"""
import math
import random
import subprocess
from _00_runner import main

HI = (1 << 63) - 1


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


def merge(x, l, a, m):
    """Exact CRT of x mod l with a mod m; None when inconsistent."""
    g = math.gcd(l, m)
    if (a - x) % g: return None
    t = (a - x) // g * pow(l // g, -1, m // g) % (m // g)
    return (x + l * t) % (l * m // g), l * m // g


def fold(pairs):
    x, l = 0, 1
    for a, m in pairs:
        r = merge(x, l, a, m)
        if r is None: return [0, 0, 0, 0]
        if r[1] > HI: return [0, 0, 0, 1]
        x, l = r
    return [x, l, 1, 0]


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 300, 'full': 4000, 'stress': 20000}[args.mode]
    def prime(bits):
        while True:
            x = rng.getrandbits(bits) | (1 << (bits - 1))
            if bits == 1: x = 2
            if is_prime(x): return x
    rows = []
    for it in range(count):
        pool = [prime(rng.randint(1, 31)) for _ in range(rng.randint(1, 6))]
        k = rng.randint(1, 6)
        ms = []
        for _ in range(k):
            m = 1
            for p in rng.sample(pool, rng.randint(1, len(pool))):
                e = rng.randint(1, 3)
                if m * p ** e <= (1 << 62): m *= p ** e
            ms.append(m)
        if it % 3 == 0: ms = [prime(rng.randint(2, 62)) for _ in range(k)]
        x = rng.getrandbits(126)
        cong = [(x % m if rng.random() < 0.9 else rng.randint(-HI - 1, HI), m) for m in ms]
        if it % 4 == 0: cong = [(a - m * rng.randint(-3, 3), m) if abs(a) + 3 * m <= HI else (a, m) for a, m in cong]
        rows.append(('c', rng.randint(1, HI), cong))
        eqs = [(rng.randint(-HI - 1, HI), rng.randint(-HI - 1, HI), m) for m in ms[:3]]
        if it % 2: eqs = [(a, a * x % m, m) for a, _, m in eqs]
        rows.append(('s', 0, eqs))
    payload = ''.join(f'{op} {t} {len(c)} ' + ' '.join(' '.join(map(str, e)) for e in c) + '\n' for op, t, c in rows)
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, target, c), line in zip(rows, lines):
        got = list(map(int, line.split()))
        if op == 's':
            pairs = []
            for a, b, m in c:
                g = math.gcd(a, m)
                if b % g: pairs = None; break
                mg = m // g
                pairs.append((b // g * pow(a // g, -1, mg) % mg if mg > 1 else 0, mg))
            want = [0, 0, 0, 0] if pairs is None else fold(pairs)
            want += [want[0], want[1]] if want[2] else [-1, -1]
            ok = got == want
        else:
            res = fold(c)
            whole = (0, 1)
            for a, m in c:
                whole = merge(*whole, a, m) if whole else None
            coprime = all(math.gcd(c[i][1], c[j][1]) == 1 for i in range(len(c)) for j in range(i))
            want = res + res + [whole[0] % target if whole else -1, whole[0] % target if coprime else -1, int(coprime)]
            ok = got[:11] == want
            if ok and coprime:
                d = got[11:]
                val, pre = 0, 1
                for di, (a, m) in zip(d, c):
                    ok &= 0 <= di < m
                    val += di * pre; pre *= m
                ok &= len(d) == len(c) and val == whole[0]
        if not ok: raise RuntimeError(f'smallest known reproducer={op} target={target} system={c}, actual={got}')
    print(f'PASS exact Python oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('10-crt', ['crt2-zero', 'crt-zero', 'scaled-zero', 'garner-mod', 'digits-zero',
        'crtmod-mod', 'crtmod-zero', 'signed-zero', 'incremental-zero'], oracle))
