"""Seed replay, exact integer mapping and exhaustive reduced-word uniformity proof checks."""
import itertools
import random
import subprocess
from _00_runner import main


def oracle(binary, args, env):
    modulus = 1 << 64
    rng = random.Random(args.seed)
    seeds = [0, 1, 5489, modulus - 1, args.seed]
    seeds += [rng.getrandbits(64) for _ in range({'quick': 2, 'full': 20, 'stress': 100}[args.mode])]

    def run(rows):
        cmd = [str(binary), '--oracle']
        p = subprocess.run(cmd, input=''.join(' '.join(map(str, row)) + '\n' for row in rows),
                           capture_output=True, text=True, env=env, timeout=180)
        if p.returncode:
            raise RuntimeError(f'command={cmd} returncode={p.returncode}\n{p.stderr}')
        lines = p.stdout.splitlines()
        if len(lines) != len(rows):
            raise RuntimeError(f'command={cmd} expected {len(rows)} lines actual={len(lines)}')
        return [list(map(int, line.split())) for line in lines]

    raw = dict(zip(seeds, run([('raw', seed, 8192) for seed in seeds])))
    cases, expected = [], []
    rejected = 0

    def add(row):
        nonlocal rejected
        words = iter(raw[row[1]])

        def bounded(width):
            nonlocal rejected
            if width == modulus:
                return next(words)
            threshold = modulus % width
            while True:
                word = next(words)
                quotient, remainder = divmod(word * width, modulus)
                if remainder >= threshold:
                    return quotient
                rejected += 1

        if row[0] == 'h':
            result = list(range(row[2]))
            for i in range(len(result) - 1, 0, -1):
                j = bounded(i + 1)
                result[i], result[j] = result[j], result[i]
        else:
            lo, width = (0, row[2]) if row[0] == 'b' else (row[2], row[3] - row[2] + 1)
            result = [lo + bounded(width) for _ in range(row[-1])]
        cases.append(row)
        expected.append(result)

    for seed in seeds:
        for width in [1, 2, 3, 7, 255, 256, 257, (1 << 32) + 1, (1 << 63) + 1, modulus - 1]:
            add(('b', seed, width, 300))
        for lo, hi in [(0, modulus - 1), (0, 0), (modulus - 1, modulus - 1), (17, (1 << 63) + 29)]:
            add(('u', seed, lo, hi, 300))
        for lo, hi in [(-(1 << 63), (1 << 63) - 1), (-(1 << 63), 0), (-7, 9), (-1, -1),
                       (-(1 << 63), -(1 << 63)), ((1 << 63) - 1, (1 << 63) - 1)]:
            add(('s', seed, lo, hi, 300))
        for lo, hi in [(-(1 << 31), (1 << 31) - 1), (-7, 9), (0, 0)]:
            add(('i', seed, lo, hi, 300))
        for n in [0, 1, 2, 3, 10, 31, 256]:
            add(('h', seed, n))
    for row, want, got in zip(cases, expected, run(cases)):
        if want != got:
            at = next(i for i, (a, b) in enumerate(zip(want, got)) if a != b) if len(want) == len(got) else -1
            raise RuntimeError(f'smallest known reproducer={row} index={at} expected={want} actual={got}')
    if not rejected:
        raise RuntimeError('expected rejection-path coverage, actual=0')
    print(f'PASS exact Python bounded/shuffle oracle: {len(cases)} cases, rejected words={rejected}', flush=True)
    for bits in range(1, 9 if args.mode == 'quick' else 11):
        m = 1 << bits
        for width in range(1, m):
            counts = [0] * width
            for x in range(m):
                q, r = divmod(x * width, m)
                if r >= m % width:
                    counts[q] += 1
            if any(c != m // width for c in counts):
                raise RuntimeError(f'uniformity model bits={bits} width={width} counts={counts}')
    for n in range(1, 8):
        outcomes = set()
        for choices in itertools.product(*(range(i + 1) for i in range(n - 1, 0, -1))):
            a = list(range(n))
            for i, j in zip(range(n - 1, 0, -1), choices):
                a[i], a[j] = a[j], a[i]
            outcomes.add(tuple(a))
        import math
        if len(outcomes) != math.factorial(n):
            raise RuntimeError(f'Fisher-Yates choices n={n} actual outcomes={len(outcomes)}')
    print('PASS exhaustive reduced-word balanced preimages and Fisher-Yates choices through n=7', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('01-random', ['below-zero', 'int-bounds', 'lng-bounds',
                                     'ulng-bounds', 'shuffle-bounds'], oracle))
