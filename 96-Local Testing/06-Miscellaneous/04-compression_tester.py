"""Exact coordinate/encounter IDs and endpoint closure against independent Python scans."""
import bisect
import os
from pathlib import Path
import random
import subprocess
import tempfile
from _00_runner import main


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    lo, hi = -(1 << 63), (1 << 63) - 1
    arrays = [[], [0], [lo, hi, lo], [5, 5, 1, 3, 1], list(range(-5, 6))]
    count = {'quick': 100, 'full': 1500, 'stress': 15000}[args.mode]
    for i in range(count):
        arrays.append([rng.randint(lo, hi) if i % 2 else rng.randint(-7, 7)
                       for _ in range(rng.randrange(101))])
    payload, rows, expected = [], [], []
    for a in arrays:
        coords = sorted(set(a))
        queries = [(lo, hi, lc, rc) for lc in [0, 1] for rc in [0, 1]]
        for x in [lo, -1, 0, 1, hi]:
            queries += [(x, x, lc, rc) for lc in [0, 1] for rc in [0, 1]]
        for _ in range(10):
            l, r = sorted([rng.randint(lo, hi), rng.randint(lo, hi)])
            queries.append((l, r, rng.randrange(2), rng.randrange(2)))
        payload.append(f'{len(a)} {len(queries)}\n' + ' '.join(map(str, a)) + '\n')
        rows += [('values', a), ('encode', a), ('encounter', a)]
        expected.append([len(coords), *coords])
        expected.append([coords.index(x) for x in a])
        seen = []
        encounter = []
        for x in a:
            if x not in seen: seen.append(x)
            encounter.append(seen.index(x))
        expected.append(encounter)
        order = sorted(range(len(a)), key=lambda i: (a[i], i))
        stable = [0] * len(a)
        for rank, i in enumerate(order):
            stable[i] = rank
        rows.append(('stableRanks', a)); expected.append(stable)
        for l, r, lc, rc in queries:
            payload.append(f'{l} {r} {lc} {rc}\n')
            lb, ub = bisect.bisect_left(coords, l), bisect.bisect_right(coords, l)
            start = sum(x < l if lc else x <= l for x in coords)
            end = sum(x <= r if rc else x < r for x in coords)
            want = [coords.index(l) if l in coords else -1, lb, ub, start, max(start, end)]
            rows.append(('pointRange', a, l, r, lc, rc)); expected.append(want)
        rows.append(('ranks', a, [q[0] for q in queries]))
        expected.append([bisect.bisect_left(coords, q[0]) for q in queries])
    command = [str(binary), '--oracle']
    result = subprocess.run(command, input=''.join(payload), text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'command={command} returncode={result.returncode}\n{result.stderr}')
    actual = [list(map(int, line.split())) for line in result.stdout.splitlines()]
    if len(actual) != len(expected):
        raise RuntimeError(f'expected lines={len(expected)} actual={len(actual)}')
    for row, want, got in zip(rows, expected, actual):
        if want != got:
            raise RuntimeError(f'smallest known reproducer={row} expected={want} actual={got}')
    print(f'PASS Python coordinate/encounter/interval oracle arrays={len(arrays)} checks={len(rows)}', flush=True)
    if binary.name == 'optimized':
        root = Path(__file__).resolve().parents[2]
        headers = [root / '02-Data Structures/07-ordered_set.hpp', root / '06-Miscellaneous/04-compression.hpp']
        body = '''
int main() {
    CoordinateCompression<int> existing({3, 1, 3});
    Compression<int> extended({3, 1, 3});
    Compression<int> empty;
    if (!empty.empty() || existing.index(3) != 1 || extended.id(3) != 1) { return 1; }
    if (existing.v != extended.v || extended.rank(3) != extended.lowerBound(3)) { return 2; }
    if (extended.upperRank(3) != extended.upperBound(3) || extended.count(3) != 1) { return 3; }
    if (*extended.findByOrder(0) != 1 || extended.findByOrder(2) != extended.end()) { return 4; }
    if (extended.encode({0, 1, 3}) != vector<int>{-1, 0, 1}) { return 5; }
    existing.rebuild({7, 2, 7}); extended.assign({7, 2, 7});
    if (existing.v != extended.v || extended.pointRange(2, 7) != pair{0, 1}) { return 6; }
    extended.rebuild({5, 5});
    if (extended.size() != 1 || extended.value(0) != 5) { return 7; }
    return 0;}
'''
        with tempfile.TemporaryDirectory(prefix='cp-compression-includes-') as name:
            directory = Path(name)
            for order in [headers, headers[::-1]]:
                source = directory / 'test.cpp'
                source.write_text(''.join('#include "' + str(header) + '"\n' for header in order) + body)
                commands = [[os.environ.get('CXX', 'g++'), '-std=gnu++20', '-O2', '-DNDEBUG',
                             str(source), '-o', str(directory / 'test')], [str(directory / 'test')]]
                for cmd in commands:
                    p = subprocess.run(cmd, capture_output=True, text=True, env=env, timeout=180)
                    if p.returncode:
                        raise RuntimeError(f'command={cmd} returncode={p.returncode}\n{p.stdout}\n{p.stderr}')
        print('PASS existing CoordinateCompression/new Compression: both header orders and inherited APIs', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('04-compression', ['snapshot-negative', 'snapshot-end',
        'encounter-negative', 'encounter-end', 'range-order', 'endpoint-order'], oracle))
