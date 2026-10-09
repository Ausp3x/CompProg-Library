"""Convolutions and transforms against independent brute force and Python big integers.

C++: primitiveRootNtt against an order-counting brute for every n <= 2000 (quick) or 6000
and on composites and Carmichael numbers; root tables, ntt, intt, transposedNtt (brute
transpose and duality) and nttDoubling against brute bit-reversed DFTs over six NTT primes
(13, 12289, 7340033, 754974721, 998244353 and the 64-bit 29 * 2^57 + 1); fft/ifft against a
long-double DFT; convolutionFft rounding near the stated error bound; naive and Karatsuba
over wrapping 64-bit words and 1e9+7 around the thresholds; convolutionNtt at the naive
threshold, squaring and the transform-length limit; convolutionLong and convolutionU128
against __int128 brute force with signed extremes and the P1 * P2 * P3 boundary, plus a
2^24-length sampled run in stress; convolutionArbitraryMod over static, dynamic 32-bit and
64-bit moduli; convolutionLarge block paths with transform lengths 2, 4 and 4096 against brute
force and a 2^25 mint product in stress; and every wrapper (dispatcher, truncated, cross-
correlation, middle product, cyclic, negacyclic, tensor, 2-D) over NTT, non-NTT static,
dynamic and 64-bit moduli. Python oracle: random signed and unsigned products and arbitrary
moduli up to 2^62 against exact big-integer convolution. 18 assertion probes.
"""
import random
import subprocess
from _00_runner import main

P = 754974721 * 167772161 * 469762049


def conv(a, b):
    if not a or not b: return []
    c = [0] * (len(a) + len(b) - 1)
    for i, x in enumerate(a):
        for j, y in enumerate(b): c[i + j] += x * y
    return c


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    count = {'quick': 60, 'full': 600, 'stress': 3000}[args.mode]
    rows = []
    for _ in range(count):
        n, m = rng.randint(0, 120), rng.randint(0, 120)
        k = max(1, min(n, m))
        bits = rng.randint(1, 62)
        a = [rng.randint(-(1 << bits), 1 << bits) for _ in range(n)]
        lim = ((1 << 63) - 1) // (k * (1 << bits))
        b = [rng.randint(-lim, lim) for _ in range(m)]
        rows.append(('L', None, a, b, conv(a, b)))
        bits = rng.randint(1, 63)
        a = [rng.randint(0, (1 << bits) - 1) for _ in range(n)]
        lim = min((1 << 64) - 1, (P - 1) // (k * (1 << bits)))
        b = [rng.randint(0, lim) for _ in range(m)]
        rows.append(('U', None, a, b, conv(a, b)))
        mod = rng.choice([1, 2, rng.randint(1, 1 << 31), rng.randint(1, 1 << 40), rng.randint(1, 1 << 62)])
        if min(n, m) > 60 and min(n, m) * (mod - 1) ** 2 >= P: n = m = min(n, 60)
        a = [rng.randrange(mod) for _ in range(n)]
        b = [rng.randrange(mod) for _ in range(m)]
        rows.append(('A', mod, a, b, [x % mod for x in conv(a, b)]))
    payload = ''.join((f'{op} {mod} ' if mod else f'{op} ') + f'{len(a)} {len(b)} ' + ' '.join(map(str, a + b)) + '\n'
                      for op, mod, a, b, _ in rows)
    command = [str(binary), '--oracle']
    p = subprocess.run(command, input=payload, capture_output=True, text=True, env=env, timeout=180)
    if p.returncode: raise RuntimeError(f'command={command} returncode={p.returncode}\n{p.stderr}')
    lines = p.stdout.splitlines()
    if len(lines) != len(rows): raise RuntimeError(f'expected {len(rows)} lines, got {len(lines)}')
    for (op, mod, a, b, want), line in zip(rows, lines):
        got = list(map(int, line.split()))
        if got != want: raise RuntimeError(f'op={op} mod={mod} a={a} b={b}, expected={want}, actual={got}')
    print(f'PASS exact Python oracle: {len(rows)} cases seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('13-convolution', ['root-one', 'table-rank', 'ntt-size', 'intt-size', 'transposed-size',
        'ntt-rank', 'doubling-rank', 'fft-size', 'ifft-size', 'arbitrary-bound', 'middle-zero', 'cyclic-size',
        'negacyclic-size', 'truncated-negative', 'tensor-rank', 'tensor-size', 'tensor-negative', '2d-ragged'], oracle))
