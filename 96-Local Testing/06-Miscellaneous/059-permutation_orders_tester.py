"""Trotter-Johnson (SJT) rank, unrank and next via an independent recursive SJT list, Knuth's Algorithm P and Python exact ranks.

C++ (every build): rank, unrank and next follow the recursive SJT list (n - 1 inserted right to left under
even-indexed parents, left to right under odd ones) exactly, the list is validated (n! distinct, identity first,
consecutive entries one adjacent transposition apart), next wraps after n! steps, and unrank(n, r >= n!) is false
with out unchanged; through n=6 in quick, 8 in full, 9 in stress. Random n in [9, 20] (quick 20, full 300, stress
1000 cases): unrank/rank round trips against Algorithm P decoded from the rank digits, absent ranks up to 2^64 - 1,
and 1000-step next walks checked against rank + 1 mod n!, one adjacent transposition and Algorithm P. Long next walks
against Algorithm P (Knuth TAOCP 7.2.1.2, Even's directions with O(1) amortized steps): from the identity (quick
n=200 for 1000 steps, full n=1000 for 4000, stress n=3000 for 9000) and from cascade states whose middle levels sit
at sweep ends, including the last permutation (quick 20 at n=25 for 1000 steps and 2 at n=3000 for 40; full
100 at n=25 for 3000, 3 at n=1e5 for 40 and 2 at n=1e6 for 6; stress 400 for 3000, 6 for 40 and 3 for 8).
forEachTrotterJohnson (every build): through the same n it visits the recursive list in order (n = 0 and 1 one
call each), the identity with i = -1 first, then 0 <= i < n - 1 with p the previous permutation swapped at i, i + 1,
and returns true after n! calls; a visitor returning false at call t ends it after t + 1 calls at list[t] and makes it
return false, for every t through n=7 and at 0, 1, n! - 2, n! - 1 and 64 random t above. Prefixes against Algorithm P's
swap index, nextTrotterJohnson and rank = call index, with the loop work counted from the moved value at most
2 calls + n: quick n=10 and 25 for 40000 calls (all checked) and n=1e5 for 300000; full n=10 whole (first 1e6
checked against next and rank), n=11 and 12 for 1e6, n=25 for 3e6 (1e6 against next) and n=1e6 for 4e6; stress
the same with n=11 and 12 for 3e6, n=25 for 1e7 and n=1e6 for 1e7 (still 1e6 against next and rank, keeping
ASan-UBSan under half its limit); stops at n! - 2 and n! - 1 for
n=10 (full, stress), at 20 random calls at n=20 against unrank (below 20000 in quick, 1e6 otherwise) and at calls 0
and 1 at n=1e5. Probe foreach-negative asserts n >= 0.
Python (every build): exact ranks through n=20 by the textbook insertion formula, itself validated against a
recursive SJT list through n=6 and Even's largest-mobile-element algorithm through n=7; unrank including absent
ranks n!, n! + 1 and 2^64 - 1; next by exact rank + 1 up to n=200 (one sample in six above 20), at random and cascade ranks (quick 300
sampled cases, full 3000, stress 12000); forEach's swap indices and stop behaviour through n=7 against Even's
algorithm, with stop calls 2^64 - 1 and n! (never reached), n! - 1, 0 and one random call.
"""
import math
import random
import sys
import threading
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run

MAX = (1 << 64) - 1


def sjt_rank(p):
    """Value m is inserted among the values below it at slot k (smaller values to its left); offset m - k under an even parent rank."""
    rank = 0
    for m in range(1, len(p)):
        k = sum(1 for x in p[:p.index(m)] if x < m)
        rank = rank * (m + 1) + (k if rank % 2 else m - k)
    return rank


def sjt_unrank(n, rank):
    digits = []
    for m in range(n, 0, -1):
        digits.append(rank % m)
        rank //= m
    if rank:
        return None
    p, parent = [], 0
    for v, d in enumerate(reversed(digits)):
        p.insert(d if parent % 2 else v - d, v)
        parent = parent * (v + 1) + d
    return p


def recursive_list(n):
    out = [[]]
    for m in range(n):
        out = [q[:at] + [m] + q[at:] for i, q in enumerate(out) for at in (range(m + 1) if i % 2 else range(m, -1, -1))]
    return out


def even_list(n):
    """Even's algorithm: move the largest mobile element, then reverse the directions of all larger ones."""
    p, d, out = list(range(n)), [-1] * n, []
    while True:
        out.append(p[:])
        best = -1
        for i, x in enumerate(p):
            j = i + d[x]
            if 0 <= j < n and p[j] < x and x > best:
                best, at = x, i
        if best < 0:
            return out
        j = at + d[best]
        p[at], p[j] = p[j], p[at]
        for x in range(best + 1, n):
            d[x] = -d[x]


def validate():
    for n in range(7):
        lst = recursive_list(n)
        if lst != even_list(n) or [sjt_rank(q) for q in lst] != list(range(len(lst))) \
                or [sjt_unrank(n, r) for r in range(len(lst))] != lst or sjt_unrank(n, len(lst)) is not None:
            raise RuntimeError(f'Python SJT oracles disagree at n={n}')
    lst = even_list(7)
    if len(lst) != 5040 or len(set(map(tuple, lst))) != 5040 or any(sjt_rank(q) != r for r, q in enumerate(lst)):
        raise RuntimeError('Python rank formula disagrees with Even\'s algorithm at n=7')


CASES, LOCK = {}, threading.Lock()


def make_cases(args):
    """Inputs with expected outputs, built once per mode and seed and shared by the builds."""
    with LOCK:
        key = (args.mode, args.seed)
        if key not in CASES:
            CASES[key] = build_cases(args)
        return CASES[key]


def build_cases(args):
    validate()
    rng = random.Random(args.seed ^ 0x5717)
    count = {'quick': 300, 'full': 3000, 'stress': 12000}[args.mode]
    cases = []
    for n in range(21):
        total = math.factorial(n)
        for r in (0, 1, total - 1, total, total + 1, MAX, rng.randrange(total), rng.randrange(total, MAX + 1)):
            if r <= MAX:
                cases.append(('U', n, r))
        for p in (list(range(n)), list(range(n - 1, -1, -1)), sjt_unrank(n, total - 1)):
            cases += [('R', n, p), ('N', n, p)]
    for i in range(count):
        n = rng.randrange(21) if i % 6 else rng.randrange(21, 201)
        total = math.factorial(n)
        if i % 4 == 0:
            k = rng.randrange(min(n, 9) + 1)
            block = total // math.factorial(k)
            r = rng.randrange(math.factorial(k)) * block + block - 1
        else:
            r = rng.randrange(total)
        p = sjt_unrank(n, r)
        if rng.random() < 0.3:
            p = list(range(n))
            rng.shuffle(p)
        if n <= 20:
            cases += [('R', n, p), ('U', n, r)]
        cases.append(('N', n, p))
    moves = {}
    for n in range(8):
        lst = even_list(n)
        moves[n] = [-1] + [next(k for k in range(n) if a[k] != b[k]) for a, b in zip(lst, lst[1:])]
        cases += [('F', n, t) for t in (MAX, len(lst), len(lst) - 1, 0, rng.randrange(len(lst)))]
    res = []
    for op, n, x in cases:
        if op == 'F':
            calls = min(x + 1, len(moves[n]))
            expected = [int(x >= len(moves[n])), calls, *moves[n][:calls]]
        elif op == 'R':
            expected = [sjt_rank(x)]
        elif op == 'U':
            p = sjt_unrank(n, x)
            expected = [1, *p] if p is not None else [0, -42]
        else:
            r = sjt_rank(x) + 1
            expected = [1, *sjt_unrank(n, r)] if r < math.factorial(n) else [0, *range(n)]
        res.append((op, n, x, expected))
    return res


def oracle(binary, args, env):
    cases = make_cases(args)
    text = ''.join(f'{op} {n} {x}\n' if op in 'UF' else f'{op} {n} ' + ' '.join(map(str, x)) + '\n' for op, n, x, _ in cases)
    run = timed_run(f'059-permutation_orders oracle {Path(binary).name}', [str(binary), '--oracle'], input=text,
                    capture_output=True, text=True, timeout=180, env=env)
    if run.returncode:
        raise RuntimeError(f'command={binary} --oracle returncode={run.returncode}\n{run.stderr}')
    lines = run.stdout.splitlines()
    if len(lines) != len(cases):
        raise RuntimeError(f'Python oracle expected {len(cases)} output rows, actual={len(lines)}')
    for (op, n, x, expected), line in zip(cases, lines):
        actual = list(map(int, line.split()))
        if actual != expected:
            raise RuntimeError(f'Python oracle seed={args.seed} operation={op} n={n} input={x!r} expected={expected} actual={actual}')
    print(f'PASS Python exact SJT rank/unrank/next oracle: {len(cases)} cases', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('059-permutation_orders', ['rank-size', 'rank-invalid', 'unrank-negative', 'unrank-size', 'next-invalid',
                                                    'foreach-negative'], oracle))
