"""Permutation checks via independent recursive enumeration and Python exact arithmetic.

Quick: distinct permutations through n=6; three-symbol counts 0..2; 100 random
algebra cases and n=1000 long inputs. Full: n=8, counts 0..3, 3000 cases and
n=100000. Stress: n=9, counts 0..4, 30000 cases and n=500000. Full/stress include
optimized, checked and ASan/UBSan builds; Python checks exact ranks/counts and
unranking independently of Fenwick, including totals on both sides of 2^64.
Every mode: scalar rank/unrank on random, identity and reversed n<=20 against
an O(n^2) counting oracle; multiset ranks mod 998244353 and mod 13 below 16
random targets per three-symbol count against std::lexicographical_compare
enumeration, each with ModInt and ModIntMini. Python rank-mod cases (exact
rank mod 2^64 and 998244353, mint and mintmini; exact
multiset rank below targets, validated against enumeration for n<=6): quick
30 small and 4 large up to n=300, full 150 and 25 up to n=3000, stress 600 and
60 up to n=4000.
"""
from bisect import bisect_left
from collections import Counter
from itertools import permutations
import math
import random
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run

MAX = (1 << 64) - 1
P = 998244353
LABELS = [-2147483648, -9, -1, 0, 3, 42, 2147483647]


def lex_rank(a):
    """Exact lexicographic rank of distinct values by Horner over factorial digits."""
    rest, rank = sorted(a), 0
    for i, x in enumerate(a):
        j = bisect_left(rest, x)
        rest.pop(j)
        rank = rank * (len(a) - i) + j
    return rank


def below(values, target):
    """Exact count of distinct arrangements of values lexicographically below target."""
    freq, left, rank = Counter(values), len(values), 0
    ways = count(freq)
    for x in target:
        if not left:
            return rank + 1
        rank += sum(ways * c // left for y, c in freq.items() if y < x)
        if not freq[x]:
            return rank
        ways = ways * freq[x] // left
        freq[x] -= 1
        left -= 1
    return rank


def below_brute(values, target):
    return sum(list(q) < target for q in set(permutations(values)))


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
    for n in [1, 2, 13, 19, 20]:
        for a in (list(range(n)), list(range(n - 1, -1, -1))):
            cases += [('D', a, request) for request in (0, math.factorial(n) - 1, math.factorial(n), MAX)]
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
    mods = mod_cases(args)
    more = construction_cases(args)
    text += ''.join(line for line, _ in mods + more)
    run = timed_run(f'007-permutation oracle {Path(binary).name}', [str(binary), '--oracle'], input=text, capture_output=True, text=True,
                                             timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={binary} --oracle returncode={run.returncode}\n{run.stderr}')
    lines = run.stdout.splitlines()
    if len(lines) != len(cases) + len(extra) + len(mods) + len(more):
        raise RuntimeError(f'Python oracle expected {len(cases) + len(extra) + len(mods) + len(more)} output rows, actual={len(lines)}')
    for (line, expected), got in zip(mods + more, lines[len(cases) + len(extra):]):
        if list(map(int, got.split())) != expected:
            raise RuntimeError(f'Python rank-mod/count oracle input={line.strip()!r} expected={expected} actual={got}')
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
        rank = rank_oracle(a)
        expected = [int(fits), total if fits else 42, rank if fits else 42,
                    int(present), rank % P, len(out), *out]
        actual = list(map(int, line.split()))
        if actual != expected:
            raise RuntimeError(f'Python oracle case={i} input={(mode, a, request)!r} '
                               f'expected={expected} actual={actual}')
    print(f'PASS Python exact multinomial/rank/unrank/kth/order oracle: {len(cases) + len(extra)} cases', flush=True)
    print(f'PASS Python exact rank mod 2^64/998244353 and multiset rank below targets: {len(mods)} cases', flush=True)
    print(f'PASS Python exact Myrvold-Ruskey rank/unrank and cycle type counts: {len(more)} cases', flush=True)


MOD_CASES = {}


def mod_cases(args):
    """Input lines with expected outputs for modes R (permutationRankMod) and P (multisetPermutationRankMod, distinct count).

    Small P cases take the expected value from set(permutations) enumeration and validate below() against it."""
    key = (args.mode, args.seed)
    if key in MOD_CASES:
        return MOD_CASES[key]
    rng = random.Random(args.seed ^ 0x5eed)
    small, large, top = {'quick': (30, 4, 300), 'full': (150, 25, 3000), 'stress': (600, 60, 4000)}[args.mode]
    res = []
    for n in [0, 1, 2, 20, 21, 64, top] * 2 + [rng.randrange(top + 1) for _ in range(large)]:
        a = list(range(n))
        rng.shuffle(a)
        for p in (a, sorted(a), sorted(a, reverse=True)):
            rank = lex_rank(p)
            res.append((f'R {n} ' + ' '.join(map(str, p)) + ' 0\n', [rank % (1 << 64), rank % P, rank % P, rank % P]))

    def targets(values):
        t = values[:]
        rng.shuffle(t)
        pool = LABELS + [5]
        out = [t, sorted(values), sorted(values, reverse=True), [], t[:rng.randrange(len(t) + 1)], t + [rng.choice(pool)],
               [rng.choice(pool) for _ in range(rng.randrange(len(t) + 3))]]
        if t:
            u = t[:]
            u[rng.randrange(len(u))] = rng.choice(pool)
            out.append(u)
        return out

    for i in range(small + large):
        n = rng.randrange(7) if i < small else rng.randrange(top + 1)
        labels = rng.sample(LABELS, rng.randrange(1, 6))
        values = [rng.choice(labels) for _ in range(n)]
        total = count(Counter(values))
        arrangements = set(permutations(values)) if i < small else None
        for t in targets(values):
            want = below(values, t)
            if arrangements is not None and want != sum(list(q) < t for q in arrangements):
                raise RuntimeError(f'Python below() disagrees with enumeration values={values} target={t}')
            line = f'P {n} ' + ' '.join(map(str, values)) + f' {len(t)} ' + ' '.join(map(str, t)) + '\n'
            res.append((line, [want % P, want % P, int(total <= MAX), total if total <= MAX else 42]))
    MOD_CASES[key] = res
    return res


def mr_rank(p):
    """Exact Myrvold-Ruskey rank1: s = p[k-1], two swaps, rank = s + k * rank1(k - 1), iterated from k = n."""
    pi, inv, steps = p[:], [0] * len(p), []
    for i, x in enumerate(p):
        inv[x] = i
    for k in range(len(p), 1, -1):
        s, j = pi[k - 1], inv[k - 1]
        pi[k - 1], pi[j] = pi[j], pi[k - 1]
        inv[s], inv[k - 1] = inv[k - 1], inv[s]
        steps.append((s, k))
    rank = 0
    for s, k in reversed(steps):
        rank = s + k * rank
    return rank


def mr_unrank(n, rank):
    pi = list(range(n))
    for k in range(n, 0, -1):
        j = rank % k
        pi[k - 1], pi[j] = pi[j], pi[k - 1]
        rank //= k
    return pi


def type_count(a):
    """n! / prod k^a_k a_k!, the permutations with a_k cycles of length k."""
    total = math.factorial(sum(k * c for k, c in enumerate(a)))
    for k, c in enumerate(a):
        if k:
            total //= k ** c * math.factorial(c)
    return total


def cycle_type(p):
    a, seen = [0] * (len(p) + 1), [False] * len(p)
    for i in range(len(p)):
        m, v = 0, i
        while not seen[v]:
            seen[v] = True
            v = p[v]
            m += 1
        a[m] += m > 0
    return a


CONSTRUCTION_CASES = {}


def construction_cases(args):
    """Input lines with expected outputs for modes L (permutationRankLinear/UnrankLinear) and C (cycleTypeCount, Mod).

    mr_rank/mr_unrank are validated as a bijection onto [0, n!) and type_count against enumerated cycle types for n <= 6."""
    key = (args.mode, args.seed)
    if key in CONSTRUCTION_CASES:
        return CONSTRUCTION_CASES[key]
    for n in range(7):
        ranks, types = set(), Counter()
        for p in permutations(range(n)):
            r = mr_rank(list(p))
            if not 0 <= r < math.factorial(n) or mr_unrank(n, r) != list(p):
                raise RuntimeError(f'Python Myrvold-Ruskey oracle is not a bijection at p={p} rank={r}')
            ranks.add(r)
            types[tuple(cycle_type(list(p)))] += 1
        if len(ranks) != math.factorial(n) or any(type_count(list(a)) != c for a, c in types.items()):
            raise RuntimeError(f'Python construction oracles fail validation at n={n}')
    rng = random.Random(args.seed ^ 0xc7c1e)
    linear, counts, top = {'quick': (4, 40, 300), 'full': (25, 300, 3000), 'stress': (60, 1000, 4000)}[args.mode]
    res = []
    for n in [0, 1, 2, 3, 5, 20, 21, 22, 64, top] + [rng.randrange(top + 1) for _ in range(linear)]:
        a = list(range(n))
        rng.shuffle(a)
        total = math.factorial(n)
        for p in (a, sorted(a), sorted(a, reverse=True)):
            rank = mr_rank(p)
            requests = [rank if rank <= MAX else rng.randrange(MAX + 1), MAX]
            if n <= 22:
                requests += [min(total - 1, MAX), min(total, MAX), rng.randrange(min(total, MAX + 1))]
            for request in requests:
                out = mr_unrank(n, request) if request < total else [-42]
                res.append((f'L {n} ' + ' '.join(map(str, p)) + f' {request}\n',
                             [rank % (1 << 64), rank % P, rank % P, int(request < total), len(out), *out]))
    types = [[], [0], [0, 0, 0], [0, 7], [0, 0, 0, 0, 1], [0, 3, 0, 0, 0, 0]]
    types += [[0] * n + [1] for n in range(1, 31)]
    types += [[0, f, m] + [0] * z for m in range(1, 30) for f, z in ((0, 0), (rng.randrange(6), rng.randrange(3)))]
    types += [[0, 0, 0, m] for m in range(1, 16)] + [[0] + [1] * m for m in range(1, 12)]
    for i in range(counts):
        n = rng.randrange(1, 45) if i % 2 else rng.randrange(1, top + 1)
        a, rest = [0] * (n + 1), n
        while rest:
            k = rng.randint(1, min(rest, rng.choice([2, 3, 7, rest])))
            a[k] += 1
            rest -= k
        while rng.random() < 0.5 and len(a) > 1 and a[-1] == 0:
            a.pop()
        types.append(a)
    for a in types:
        total = type_count(a)
        fits = total <= MAX
        res.append((f'C {len(a)} ' + ' '.join(map(str, a)) + ' 0\n', [int(fits), total if fits else 42, total % P, total % P]))
    CONSTRUCTION_CASES[key] = res
    return res


if __name__ == '__main__':
    raise SystemExit(main('007-permutation', [
        'inverse-duplicate', 'inverse-negative', 'inverse-range', 'compose-size',
        'compose-invalid', 'power-invalid', 'lehmer-invalid', 'digits-negative',
        'digits-range', 'rank-size', 'rank-invalid', 'unrank-negative', 'unrank-size',
        'kth-invalid', 'cycles-invalid', 'sign-invalid',
        'order-invalid', 'order-mod-invalid', 'rank-mod-invalid', 'rank-mod-naive-invalid', 'lehmer-naive-invalid', 'digits-naive-negative', 'digits-naive-range', 'digits-naive-late', 'digits-naive-over', 'multiset-rank-mod-size',
        'multiset-rank-mod-size-mini', 'cycle-type-invalid', 'from-cycles-range', 'from-cycles-negative-label',
        'from-cycles-duplicate', 'from-cycles-missing', 'from-cycles-negative-n', 'from-cycles-huge', 'from-cycles-repeat',
        'from-cycles-repeat-zero', 'permute-size', 'permute-size-smaller', 'permute-invalid',
        'inverse-permute-size', 'inverse-permute-size-larger', 'inverse-permute-invalid', 'swap-rounds-invalid', 'collecting-invalid',
        'rank-linear-invalid',
        'unrank-linear-negative', 'cycle-count-zero', 'cycle-count-negative-zero', 'cycle-count-negative', 'cycle-count-size',
        'cycle-count-mod-zero', 'cycle-count-mod-negative', 'cycle-count-mod-size'], oracle))
