"""permutationWithInversions against an exhaustive enumeration, inversion counters and the greedy Lehmer code.

C++ (every build): for every n through 6 in quick, 8 in full and 9 in stress and every k in [0, n(n-1)/2], equality with
the lexicographically largest permutation with k inversions of a recursive enumeration whose counts per k match the
Mahonian numbers, and with the greedy Lehmer code d_i = min(rest, n-1-i) decoded in O(n^2); random n < 70 (quick 100,
full 3000, stress 30000 cases, a quarter at the ends of the descending prefix and their neighbours): a permutation of
[0, n) with exactly k inversions by brute O(n^2) and merge-sort counts, equal to the decoded greedy code; random n in
[70, 3000] (quick 5, full 100, stress 300), n=65537 at k = 2^31 - 2, 2^31 - 1, 2^31, and n = 1000 (quick), 100000
(full), 500000 (stress), each with k in {0, 1, max/2, max-1, max, random} and three prefix ends: a permutation with
the merge-sort count k whose Lehmer code (Fenwick) equals the greedy code.
Python (every build): exact outputs from the greedy Lehmer code decoded in Python, itself validated against
itertools enumeration through n=7, on random n up to 300 with random, extreme and prefix-end k (quick 300 cases,
full 3000, stress 12000).
"""
from itertools import permutations
import random
import sys
import threading
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run


def largest(n, k):
    """Lexicographically largest code of sum k under d_i <= n-1-i, decoded by taking the d_i-th smallest unused value."""
    rest, p = list(range(n)), []
    for i in range(n):
        d = min(k, n - 1 - i)
        k -= d
        p.append(rest.pop(d))
    return p


def validate():
    for n in range(8):
        best = {}
        for p in permutations(range(n)):
            k = sum(p[j] < p[i] for i in range(n) for j in range(i + 1, n))
            best[k] = max(best.get(k, p), p)
        if sorted(best) != list(range(n * (n - 1) // 2 + 1)) or any(largest(n, k) != list(p) for k, p in best.items()):
            raise RuntimeError(f'Python greedy Lehmer oracle disagrees with the enumeration at n={n}')


CASES, LOCK = {}, threading.Lock()


def make_cases(args):
    """Inputs with expected outputs, built once per mode and seed and shared by the builds."""
    with LOCK:
        key = (args.mode, args.seed)
        if key not in CASES:
            validate()
            rng = random.Random(args.seed ^ 0x2214)
            count = {'quick': 300, 'full': 3000, 'stress': 12000}[args.mode]
            cases = []
            for i in range(count):
                n = rng.randrange(301)
                top = n * (n - 1) // 2
                if i % 4 == 1:
                    t = rng.randrange(n + 1)
                    k = min(max(t * (2 * n - 1 - t) // 2 + rng.randrange(-1, 2), 0), top)
                elif i % 8 == 2:
                    k = rng.choice([0, 1, top - 1, top])
                    k = min(max(k, 0), top)
                else:
                    k = rng.randrange(top + 1)
                cases.append((n, k, largest(n, k)))
            CASES[key] = cases
        return CASES[key]


def oracle(binary, args, env):
    cases = make_cases(args)
    text = ''.join(f'{n} {k}\n' for n, k, _ in cases)
    run = timed_run(f'060-permutation_problems oracle {Path(binary).name}', [str(binary), '--oracle'], input=text,
                    capture_output=True, text=True, timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={binary} --oracle returncode={run.returncode}\n{run.stderr}')
    lines = run.stdout.split('\n')[:-1]
    if len(lines) != len(cases):
        raise RuntimeError(f'Python oracle expected {len(cases)} output rows, actual={len(lines)}')
    for (n, k, expected), line in zip(cases, lines):
        actual = list(map(int, line.split()))
        if actual != expected:
            raise RuntimeError(f'Python oracle seed={args.seed} operation=permutationWithInversions n={n} k={k} expected={expected} actual={actual}')
    print(f'PASS Python greedy Lehmer oracle: {len(cases)} cases', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('060-permutation_problems', ['inversions-large', 'inversions-negative-k', 'inversions-negative-n', 'inversions-large-wide'], oracle))
