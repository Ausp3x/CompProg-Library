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
    text = ''.join(f'{mode} {len(a)} ' + ' '.join(map(str, a)) + f' {request}\n'
                   for mode, a, request in cases)
    run = subprocess.run([str(binary), '--oracle'], input=text, capture_output=True, text=True,
                         timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={binary} --oracle returncode={run.returncode}\n{run.stderr}')
    lines = run.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f'Python oracle expected {len(cases)} output rows, actual={len(lines)}')
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
    print(f'PASS Python exact multinomial/rank/unrank oracle: {len(cases)} cases', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('07-permutation', [
        'inverse-duplicate', 'inverse-negative', 'inverse-range', 'compose-size',
        'compose-invalid', 'power-invalid', 'lehmer-invalid', 'digits-negative',
        'digits-range', 'rank-size', 'rank-invalid', 'unrank-negative', 'unrank-size',
        'count-negative', 'count-size'], oracle))
