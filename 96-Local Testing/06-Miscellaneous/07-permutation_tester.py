"""Permutation checks via independent recursive enumeration and Python exact arithmetic.

Quick: distinct permutations through n=6; three-symbol counts 0..2; 100 random
algebra cases and n=1000 long inputs. Full: n=8, counts 0..3, 3000 cases and
n=100000. Stress: n=9, counts 0..4, 30000 cases and n=500000. Full/stress include
optimized, checked and ASan/UBSan builds; Python checks exact ranks/counts and
unranking independently of Fenwick, including totals on both sides of 2^64.
"""
from collections import Counter
import math
import random
import subprocess
from _00_runner import main

MAX = (1 << 64) - 1


def count(freq):
    ways = math.factorial(sum(freq.values()))
    for c in freq.values():
        ways //= math.factorial(c)
    return ways


def rank_oracle(a):
    freq = Counter(a)
    rank = 0
    for x in a:
        for y in sorted(freq):
            if y >= x:
                break
            if freq[y]:
                freq[y] -= 1
                rank += count(freq)
                freq[y] += 1
        freq[x] -= 1
    return rank


def unrank_oracle(a, rank):
    freq = Counter(a)
    out = []
    for _ in a:
        for x in sorted(freq):
            if not freq[x]:
                continue
            freq[x] -= 1
            ways = count(freq)
            if rank < ways:
                out.append(x)
                break
            rank -= ways
            freq[x] += 1
        else:
            raise RuntimeError('invalid oracle unrank request')
    return out


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    cases = []
    for n in range(21):
        a = list(range(n))
        rng.shuffle(a)
        total = math.factorial(n)
        for request in [0, total - 1, total, MAX]:
            cases.append(('D', a[:], request))
    fixtures = [[], [7] * 100, list(range(21)), [0] * 34 + [1] * 33,
                [0] * 34 + [1] * 34, [0, 0, 1, 1] + list(range(2, 19))]
    for a in fixtures:
        rng.shuffle(a)
        total = count(Counter(a))
        for request in {0, min(total - 1, MAX), min(total, MAX), MAX}:
            cases.append(('M', a[:], request))
    for _ in range(50 if args.mode == 'quick' else 500 if args.mode == 'full' else 3000):
        a = [rng.choice([-2147483648, -9, 0, 42, 2147483647]) for _ in range(rng.randrange(45))]
        total = count(Counter(a))
        cases.append(('M', a, rng.randrange(min(total, MAX) + 1)))
    extra = []
    for i in range(30 if args.mode == 'quick' else 300 if args.mode == 'full' else 3000):
        n = rng.choice([0, 1, 2, 5, 20, 21, 22, 40, rng.randrange(60)])
        a = list(range(n))
        rng.shuffle(a)
        if i % 4 == 0:
            a.sort(reverse=rng.random() < 0.5)
        k = rng.choice([rng.randrange(-1000, 1001), rng.randrange(-(1 << 63), 1 << 63), -(1 << 63), (1 << 63) - 1])
        extra.append(('K', a, k))
        lengths = rng.sample([2, 3, 4, 5, 7, 8, 9, 11, 13, 16, 17, 19, 23, 25, 27, 29, 31, 37, 41, 43, 47, 49, 53, 59, 61, 64, 67],
                             rng.randrange(1, 20))
        b, start = [], 0
        for c in lengths + [1] * rng.randrange(3):
            b += [start + (j + 1) % c for j in range(c)]
            start += c
        perm = list(range(len(b)))
        rng.shuffle(perm)
        conj = [0] * len(b)
        for i2, x in enumerate(b):
            conj[perm[i2]] = perm[x]
        extra.append(('O', conj, 0))
    text = ''.join(f'{mode} {len(a)} ' + ' '.join(map(str, a)) + f' {request % (1 << 64)}\n'
                   for mode, a, request in cases + extra)
    run = subprocess.run([str(binary), '--oracle'], input=text, capture_output=True, text=True,
                         timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={binary} --oracle returncode={run.returncode}\n{run.stderr}')
    lines = run.stdout.splitlines()
    if len(lines) != len(cases) + len(extra):
        raise RuntimeError(f'Python oracle expected {len(cases) + len(extra)} output rows, actual={len(lines)}')
    for (mode, a, k), line in zip(extra, lines[len(cases):]):
        n = len(a)
        if mode == 'K':
            rank, rest = 0, sorted(a)
            for i, x in enumerate(a):
                rank += rest.index(x) * math.factorial(n - 1 - i)
                rest.remove(x)
            target = rank + k
            ok = 0 <= target < math.factorial(n)
            target %= math.factorial(n)
            rest, out = list(range(n)), []
            for i in range(n):
                f = math.factorial(n - 1 - i)
                out.append(rest.pop(target // f))
                target %= f
            expected = [int(ok), *out]
        else:
            seen, cycles, lengths = [False] * n, 0, set()
            for i in range(n):
                if not seen[i]:
                    cycles += 1
                    m, v = 0, i
                    while not seen[v]:
                        seen[v] = True
                        v = a[v]
                        m += 1
                    lengths.add(m)
            order = math.lcm(*lengths) if lengths else 1
            fits = order < 1 << 64
            expected = [int(fits), order if fits else 0, order % 998244353, -1 if (n - cycles) % 2 else 1, cycles]
        actual = list(map(int, line.split()))
        if actual != expected:
            raise RuntimeError(f'Python kth/order oracle input={(mode, a, k)!r} expected={expected} actual={actual}')
    lines = lines[:len(cases)]
    for i, ((mode, a, request), line) in enumerate(zip(cases, lines)):
        total = count(Counter(a))
        fits = total <= MAX
        present = fits and request < total
        out = unrank_oracle(a, request) if present else [-42]
        expected = [int(fits), total if fits else 42, rank_oracle(a) if fits else 42,
                    int(present), len(out), *out]
        actual = list(map(int, line.split()))
        if actual != expected:
            raise RuntimeError(f'Python oracle case={i} input={(mode, a, request)!r} '
                               f'expected={expected} actual={actual}')
    print(f'PASS Python exact multinomial/rank/unrank/kth/order oracle: {len(cases) + len(extra)} cases', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('07-permutation', [
        'inverse-duplicate', 'inverse-negative', 'inverse-range', 'compose-size',
        'compose-invalid', 'power-invalid', 'lehmer-invalid', 'digits-negative',
        'digits-range', 'rank-size', 'rank-invalid', 'unrank-negative', 'unrank-size',
        'count-negative', 'count-size', 'kth-invalid', 'cycles-invalid', 'sign-invalid',
        'order-invalid', 'order-mod-invalid'], oracle))
