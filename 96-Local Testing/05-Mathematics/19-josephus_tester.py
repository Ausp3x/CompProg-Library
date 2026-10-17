"""Josephus survivor, k-th removal, removal step and order against simulation and Python integers.

C++ (optimized, checked and, outside quick, ASan/UBSan builds): direct simulation for
every n <= 60 (quick 30) with k <= n + 10 and k in {2^40, 2^63 - 2, 2^63 - 1}, checking
every operation, every step t and every person p; seeded simulations up to n = 3000
(quick 500, stress 60 cases); josephusOrder for n up to 200000 against josephusKth and
josephusTime on sampled steps; for n up to 2^63 - 1 the inverse pair
josephusTime(josephusKth(t)) = t, josephusKth(josephusTime(p)) = p, the survivor's removal
at step n - 1 and josephusK2 = josephus(n, 2).
Python oracle (quick 120, full 600, stress 4000 rows): list simulation for n <= 2000 with
any k; for n up to 2^63 - 1 and k <= 300 the Concrete Mathematics D-sequence survivor,
the naive forward recurrence for t < 3000 and the unaccelerated count-label walk for the
removal step.
"""
import random
import shlex
import subprocess
from _00_runner import main

HI = (1 << 63) - 1


def simulate(n, k):
    people, out, cur = list(range(n)), [], 0
    while people:
        cur = (cur + k - 1) % len(people)
        out.append(people.pop(cur))
    return out


def survivor(n, k):
    """Concrete Mathematics (3.20): J = k n + 1 - D, D the first term of D := ceil(k D / (k - 1)) above (k - 1) n."""
    if k == 1:
        return n - 1
    d = 1
    while d <= (k - 1) * n:
        d = -(-k * d // (k - 1))
    return k * n - d


def kth(n, k, t):
    r = (k - 1) % (n - t)
    for s in range(n - t, n):
        r = (r + k) % (s + 1)
    return r


def step(n, k, p):
    c = p + 1
    while c % k:
        c += n - c // k
    return c // k - 1


def oracle(binary, args, env):
    for n in range(1, 13):
        for k in range(1, 15):
            order = simulate(n, k)
            assert survivor(n, k) == order[-1] and all(kth(n, k, t) == order[t] and step(n, k, order[t]) == t for t in range(n)), (n, k)
    rng = random.Random(args.seed)
    count = {'quick': 120, 'full': 600, 'stress': 4000}[args.mode]
    rows, want = [], []
    for i in range(count):
        if i % 2 == 0:
            n = rng.randint(1, 2000)
            k = rng.choice([1, 2, rng.randint(1, 50), rng.randint(1, n + 5), rng.randint(1, HI), HI])
            t, p = rng.randrange(n), rng.randrange(n)
            order = simulate(n, k)
            want.append((order[-1], order[t], order.index(p), order[-1] if k == 2 else '-'))
        else:
            n = rng.choice([rng.randint(1, HI), HI, rng.randint(1, 1 << rng.randint(1, 62))])
            k = rng.choice([1, 2, 3, rng.randint(1, 300)])
            t, p = rng.randrange(min(n, 3000)), rng.randrange(n)
            s = survivor(n, k)
            want.append((s, kth(n, k, t), step(n, k, p), s if k == 2 else '-'))
        rows.append((n, k, t, p))
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input=''.join(' '.join(map(str, row)) + '\n' for row in rows),
                            text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'Python oracle returncode={result.returncode} command={shlex.join(command)}\n{result.stderr}')
    actual = result.stdout.splitlines()
    if len(actual) != len(rows):
        raise RuntimeError(f'Python oracle output count expected={len(rows)} actual={len(actual)} command={shlex.join(command)}')
    for i, (row, expect, line) in enumerate(zip(rows, want, actual)):
        if tuple(line.split()) != tuple(map(str, expect)):
            raise RuntimeError(f'Python oracle seed={args.seed} failing row #{i}={row}\nexpected={expect}\nactual={line}\n'
                               f'command={shlex.join(command)}')
    print(f'PASS Python simulation and exact-integer oracle (validated by simulation) cases={len(rows)} seed={args.seed}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('19-josephus', ['josephus-n', 'josephus-k', 'k2-n', 'kth-t', 'time-p', 'order-k', 'order-n'], oracle))
