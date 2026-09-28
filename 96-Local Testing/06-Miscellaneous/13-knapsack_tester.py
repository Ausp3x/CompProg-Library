"""Knapsack variants verified with independent recursive multiplicity vectors.

All modes cover capacity/value optima, feasibility, exact/at-most counts,
unreachable/unbounded/finite statuses, 0/1/unlimited/bounded/mixed items, zero
weights and values, negative/full-width values, exact arbitrary and modular counts,
trace multiplicities, copies, defaults and 37 asserted-precondition probes.
Quick/full/stress exhaust alphabet sequences through 2/3/4 items and axes 4/6/8,
plus every singleton weight 0..4, value -3..3, count -1/0/1/2/5, axis 0..7.
They add 100/2000/10000 seeded cases, item/axis dimensions 1000/100000/500000,
and exact counts 2^100/2^1000/2^5000. Full/stress add ASan/UBSan with leak checks
to the optimized NDEBUG and checked configurations provided by the shared driver.
Python arbitrary integers independently verify decimal count outputs, including
the test-only big integer implementation used when Boost is unavailable.
"""
import random
import shlex
import subprocess
from _00_runner import main


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    fixtures = [([], 0), ([(0, 2147483647)] * 3, 3),
                ([(0, 1)] * (100 if args.mode == 'quick' else 1000), 0),
                ([(1, 1)] * (70 if args.mode == 'quick' else 200), 35),
                ([(1, 5), (1, 5)], 10), ([(0, -1), (2, 1)], 4),
                ([(1, -1), (2, -1), (3, -1), (5, -1)], 100)]
    for _ in range(30 if args.mode == 'quick' else 200 if args.mode == 'full' else 1000):
        fixtures.append(([(rng.randrange(8), rng.choice((-1, 0, 1, 2, 5, 2147483647)))
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
            expected.append((items, cap, w, [state, 0 if state else count,
                                           2 if infinite else 0, 0 if infinite else prefix]))
    text = ''.join(f'{len(items)} {cap}\n' + ''.join(f'{w} {c}\n' for w, c in items)
                   for items, cap in fixtures)
    command = [str(binary), '--oracle-counts']
    run = subprocess.run(command, input=text, capture_output=True, text=True, timeout=180, env=env)
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
        'counts-atmost-negative', 'counts-atmost-large'], oracle))
