"""Modular inverses against brute search and Python's exact pow(a, -1, m).

C++: inverseXgcd for every a in [-3m, 3m] and m <= 120 by brute search, inverseFermat and
inversePierce on the prime moduli in that range (zero residues give -1), signed extremes and
the largest 63-bit prime. inverseTable through 20000 (quick), 1000000 (full) or 5000000
(stress) for mint, plus moduli 13, 2, 2^61 - 1 and dynamic 1e9+7; inverseBatch and
inverseList for sizes 0..N/10 with 0, 1 or 3 zeros, a composite dynamic modulus with units
and with a nonunit (input unchanged); powerTable for six exponents including 0 and 2^64 - 1,
prime and composite moduli and modulus 1, against direct powers. Python oracle: random
63-bit operands and moduli, and random primes up to 2^63. 8 assertion probes.
"""
import random
import subprocess
from _00_runner import main


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
    count = {'quick': 300, 'full': 5000, 'stress': 30000}[args.mode]
    lo, hi = -(1 << 63), (1 << 63) - 1
    rows = []
    for _ in range(count):
        rows.append(('x', rng.randint(lo, hi), rng.randint(1, hi)))
        rows.append(('x', rng.randint(lo, hi), rng.randint(1, 1 << rng.randint(1, 62))))
        while True:
            p = rng.randint(2, 1 << rng.randint(2, 63))
            if p <= hi and is_prime(p): break
        rows.append(('f', rng.randint(lo, hi), p))
        rows.append(('f', p * rng.randint(-3, 3) if abs(p * 3) <= hi else 0, p))
    payload = ''.join(f'{op} {a} {m}\n' for op, a, m in rows)
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, a, m), line in zip(rows, lines):
        got = list(map(int, line.split()))
        try: want = pow(a, -1, m) if m > 1 else 0
        except ValueError: want = -1
        if op == 'f': want = [want, want]
        else: want = [want]
        if got != want: raise RuntimeError(f'smallest known reproducer={op} {a} {m}, expected={want}, actual={got}')
    print(f'PASS exact Python oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('09-modinverse', ['xgcd-zero', 'fermat-one', 'pierce-zero', 'table-negative',
        'table-modulus', 'table-composite', 'list-composite', 'power-negative'], oracle))
