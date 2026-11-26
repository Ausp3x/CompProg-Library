"""Knapsack variants verified with independent recursive multiplicity vectors.

All modes cover capacity/value optima, feasibility, exact/at-most counts,
unreachable/unbounded/finite statuses, 0/1/unlimited/bounded/mixed items, zero
weights and values, negative/full-width values, exact arbitrary and modular counts
(full ModInt and mini ModIntMini, mod 998244353 and 2, and wrapping ulng),
trace multiplicities, copies, defaults, fewest coins (minimumCoins: multiset
enumeration and BFS), ordered compositions (compositionCountTable: sequence
enumeration and part-count layers, exact, mod 998244353, mod 2 and mod 2^64),
equal-sum splits and sign vectors (equalPartitionCount, signedSumCount: brute
force over all 2^n assignments, binomials for ones, 2^k for zeros, and a Python
signed-sum walk over 1..60/150/250; every reordering of small lists) and 48
asserted-precondition probes.
Quick/full/stress exhaust alphabet sequences through 2/3/4 items and axes 4/6/8,
plus every singleton weight 0..4, value -3..3, count -1/0/1/2/5, axis 0..7.
They add 100/2000/10000 seeded cases, item/axis dimensions 1000/100000/500000,
and exact counts 2^100/2^1000/2^5000. Bounded count layers take counts
1/2/3/5/bound-1/bound/bound+1 at every step 1..cap+1 for caps up to 10/16/24,
alone and among 0/1/bounded/unlimited items, plus 100/1000/5000 mixed seeded
cases. Coin and part lists exhaust an alphabet
through 2/3/4 values and totals up to 6/8/10, with 100/2000/10000 seeded cases,
large totals 1000/100000/500000 and Python tables to 300/2000/5000 ({1,2}) and
200/1000/3000 ({1,1,1}). Split/sign lists exhaust {0,1,2,3,7} through 4/5/6
values at every target, with 100/2000/10000 seeded cases, ones 1000/3000/6000
and zeros 1000/100000/500000. Full/stress add ASan/UBSan with leak checks
to the optimized NDEBUG and checked configurations provided by the shared driver.
Python arbitrary integers independently verify decimal count outputs, including
the test-only big integer implementation used when Boost is unavailable.
"""
import math
import random
import shlex
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    fixtures = [([], 0), ([(0, 2147483647)] * 3, 3),
                ([(0, 1)] * (100 if args.mode == 'quick' else 1000), 0),
                ([(1, 1)] * (70 if args.mode == 'quick' else 200), 35),
                ([(1, 5), (1, 5)], 10), ([(0, -1), (2, 1)], 4),
                ([(1, -1), (2, -1), (3, -1), (5, -1)], 100), ([(1, 3)] * 40, 120),
                ([(2, 7), (3, 4), (1, 1), (5, -1), (0, 2)] * 8, 150)]
    for _ in range(30 if args.mode == 'quick' else 200 if args.mode == 'full' else 1000):
        fixtures.append(([(rng.randrange(8), rng.choice((-1, 0, 1, 2, 3, 5, 2147483647)))
                          for _ in range(rng.randrange(12))], rng.randrange(20)))
    expected = []
    for items, cap in fixtures:
        ways = [1] + [0] * cap
        infinite = any(w == 0 and count == -1 for w, count in items)
        # Direct convolution sums each allowed multiplicity, without sliding
        # windows, binary grouping, or the implementation's in-place direction.
        for weight, count in items:
            if weight == 0:
                if count >= 0:
                    ways = [x * (count + 1) for x in ways]
                continue
            out = [0] * (cap + 1)
            for w in range(cap + 1):
                bound = w // weight
                if count >= 0:
                    bound = min(bound, count)
                out[w] = sum(ways[w - used * weight] for used in range(bound + 1))
            ways = out
        prefix = 0
        for w, count in enumerate(ways):
            prefix += count
            state = 1 if count == 0 else 2 if infinite else 0
            exact = 0 if state else count
            expected.append((items, cap, w, [state, exact, 2 if infinite else 0, 0 if infinite else prefix,
                                           exact % 998244353, exact % 998244353, exact % (1 << 64)]))
    text = ''.join(f'{len(items)} {cap}\n' + ''.join(f'{w} {c}\n' for w, c in items)
                   for items, cap in fixtures)
    command = [str(binary), '--oracle-counts']
    run = timed_run(f'13-knapsack oracle {Path(command[0]).name}', command, input=text, capture_output=True, text=True, timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={shlex.join(command)} returncode={run.returncode}\n{run.stdout}\n{run.stderr}')
    rows = run.stdout.splitlines()
    if len(rows) != len(expected):
        raise RuntimeError(f'count oracle expected rows={len(expected)} actual={len(rows)}')
    for (items, cap, w, want), row in zip(expected, rows):
        got = list(map(int, row.split()))
        if got != want:
            raise RuntimeError(f'Python count oracle seed={args.seed} items={items} capacity={cap} target={w}'
                               f' expected={want} actual={got}')
    print(f'PASS Python arbitrary-integer count oracle fixtures={len(fixtures)} rows={len(rows)}', flush=True)
    compositions(binary, args, env, rng)
    splits(binary, args, env, rng)


def multinomial_table(parts, total):
    """Ordered sequences summed over multiplicity vectors a: (sum a)! / prod(a_i!)."""
    out = [0] * (total + 1)

    def visit(i, weight, used, denominator):
        if i == len(parts):
            out[weight] += math.factorial(used) // denominator
            return
        for k in range((total - weight) // parts[i] + 1):
            visit(i + 1, weight + k * parts[i], used + k, denominator * math.factorial(k))
    visit(0, 0, 0, 1)
    return out


def fibonacci_pair(m):
    """(F(m), F(m+1)) by fast doubling."""
    if m == 0:
        return 0, 1
    a, b = fibonacci_pair(m // 2)
    c, d = a * (2 * b - a), a * a + b * b
    return (d, c + d) if m % 2 else (c, d)


def compositions(binary, args, env, rng):
    big = {'quick': (300, 200), 'full': (2000, 1000)}.get(args.mode, (5000, 3000))
    fixtures = [([], 0), ([], 6), ([1, 1], 2), ([5, 2, 3], 9), ([7], 3), ([2147483647, 1], 5), ([3, 1, 3, 12], 40)]
    for _ in range(30 if args.mode == 'quick' else 200 if args.mode == 'full' else 1000):
        fixtures.append(([rng.randrange(1, 13) for _ in range(rng.randrange(5))], rng.randrange(31)))
    expected = [multinomial_table(parts, total) for parts, total in fixtures]
    # Parts {1,2} give F(w+1): the binomial sum C(w-k,k) on a prefix and fast doubling on every row.
    fixtures.append(([2, 1, big[0] + 1], big[0]))
    expected.append([fibonacci_pair(w + 1)[0] for w in range(big[0] + 1)])
    if any(sum(math.comb(w - k, k) for k in range(w // 2 + 1)) != expected[-1][w] for w in range(301)):
        raise RuntimeError('binomial and fast-doubling Fibonacci oracles disagree')
    fixtures.append(([1, 1, 1], big[1]))
    expected.append([3 ** w for w in range(big[1] + 1)])
    fixtures.append(([1, 1], 150))
    expected.append([2 ** w for w in range(151)])
    text = ''.join(f'{len(parts)} {total}\n' + ''.join(f'{x}\n' for x in parts) for parts, total in fixtures)
    command = [str(binary), '--oracle-compositions']
    run = timed_run(f'13-knapsack compositions {Path(command[0]).name}', command, input=text, capture_output=True, text=True, timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={shlex.join(command)} returncode={run.returncode}\n{run.stdout}\n{run.stderr}')
    rows = run.stdout.splitlines()
    if len(rows) != sum(len(table) for table in expected):
        raise RuntimeError(f'composition oracle expected rows={sum(len(t) for t in expected)} actual={len(rows)}')
    at = 0
    for (parts, total), table in zip(fixtures, expected):
        for w, count in enumerate(table):
            want = [count, count % 998244353, count % (1 << 64), count % 998244353]
            got = list(map(int, rows[at].split()))
            at += 1
            if got != want:
                raise RuntimeError(f'Python composition oracle seed={args.seed} parts={parts} total={total}'
                                   f' at={w} expected={want} actual={got}')
    print(f'PASS Python arbitrary-integer composition oracle fixtures={len(fixtures)} rows={len(rows)}'
          f' fibonacci_total={big[0]} power3_total={big[1]}', flush=True)


def signed_walk(values):
    """Sign-vector counts by signed sum, one +-value step per item; index s + sum(values)."""
    total = sum(values)
    ways = [0] * (2 * total + 1)
    ways[total] = 1
    for x in values:
        ways = [(ways[s - x] if s >= x else 0) + (ways[s + x] if s + x <= 2 * total else 0)
                for s in range(2 * total + 1)]
    return total, ways


def splits(binary, args, env, rng):
    n = {'quick': 60, 'full': 150}.get(args.mode, 250)
    m = {'quick': 200, 'full': 600}.get(args.mode, 1000)
    fixtures, expected = [], []
    for values, target in [(list(range(1, n + 1)), 0), (list(range(1, n + 1)), n), (list(range(1, n + 1)), n * (n + 1) // 2 - 2),
                           ([], 0), ([0], 0), ([1, 2, 3, 4, 5, 6, 7], 0), ([0, 0, 5, 5], 10)] + [
            ([rng.randrange(16) for _ in range(k)], 0) for k in [rng.randrange(13) for _ in range(30 if args.mode == 'quick' else 200 if args.mode == 'full' else 1000)]]:
        if values and target == 0 and len(values) <= 12 and rng.randrange(2):
            target = rng.randrange(-sum(values) - 2, sum(values) + 3)
        total, ways = signed_walk(values)
        signed = ways[target + total] if -total <= target <= total else 0
        fixtures.append((values, target))
        expected.append((ways[total] // 2 if values else 1, signed))
    # Ones: C(m, (m + t) / 2) sign vectors and C(m, m/2) / 2 splits.
    for target in (0, 2, m - 2, m, 1, m + 2):
        signed = math.comb(m, (m + target) // 2) if (m + target) % 2 == 0 and abs(target) <= m else 0
        fixtures.append(([1] * m, target))
        expected.append((math.comb(m, m // 2) // 2, signed))
    text = ''.join(f'{len(values)} {target}\n' + ''.join(f'{x}\n' for x in values) for values, target in fixtures)
    command = [str(binary), '--oracle-splits']
    run = timed_run(f'13-knapsack splits {Path(command[0]).name}', command, input=text, capture_output=True, text=True, timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={shlex.join(command)} returncode={run.returncode}\n{run.stdout}\n{run.stderr}')
    rows = run.stdout.splitlines()
    if len(rows) != len(fixtures):
        raise RuntimeError(f'split oracle expected rows={len(fixtures)} actual={len(rows)}')
    for (values, target), (split, signed), row in zip(fixtures, expected, rows):
        want = [split, signed, split % 998244353, signed % 998244353, split % 998244353, signed % 998244353]
        got = list(map(int, row.split()))
        if got != want:
            raise RuntimeError(f'Python split oracle seed={args.seed} values={values} target={target}'
                               f' expected={want} actual={got}')
    print(f'PASS Python arbitrary-integer split/sign oracle fixtures={len(fixtures)} range_n={n} ones_m={m}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('13-knapsack', [
        'capacity-negative', 'capacity-intmax', 'capacity-weight', 'capacity-count',
        'capacity-exact-negative', 'capacity-exact-large', 'capacity-atmost-negative',
        'capacity-atmost-large', 'capacity-restore-disabled',
        'capacity-restore-unreachable', 'capacity-restore-unbounded',
        'capacity-restore-negative', 'capacity-restore-large',
        'value-negative', 'value-intmax', 'value-weight', 'value-count',
        'value-negative-item', 'value-query-negative', 'value-query-large',
        'value-budget-negative', 'value-restore-disabled', 'value-restore-unreachable',
        'value-restore-negative', 'value-restore-large',
        'feasible-negative', 'feasible-intmax', 'feasible-weight', 'feasible-count',
        'counts-negative', 'counts-intmax', 'counts-weight', 'counts-count',
        'counts-exact-negative', 'counts-exact-large',
        'counts-atmost-negative', 'counts-atmost-large',
        'coins-negative', 'coins-intmax', 'coins-coin',
        'compositions-negative', 'compositions-intmax', 'compositions-zero', 'compositions-part',
        'partition-negative', 'partition-wide', 'signed-negative', 'signed-wide'], oracle))
