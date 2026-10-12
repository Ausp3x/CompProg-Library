"""Exact first-encounter ids, stable ranks and endpoint collection against independent Python scans."""
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
        intervals = [(lo, hi)] + [(x, x) for x in [lo, -1, 0, 1, hi]]
        intervals += [tuple(sorted([rng.randint(lo, hi), rng.randint(lo, hi)])) for _ in range(10)]
        intervals += [tuple(sorted(a[i:i + 2])) for i in range(0, len(a) - 1, 2)]
        payload.append(f'{len(a)} {len(intervals)}\n' + ' '.join(map(str, a)) + '\n'
                       + ''.join(f'{l} {r}\n' for l, r in intervals))
        seen = []
        encounter = []
        for x in a:
            if x not in seen: seen.append(x)
            encounter.append(seen.index(x))
        rows.append(('encounter', a)); expected.append(encounter)
        order = sorted(range(len(a)), key=lambda i: (a[i], i))
        stable = [0] * len(a)
        for rank, i in enumerate(order):
            stable[i] = rank
        rows.append(('stableRanks', a)); expected.append(stable)
        ends = sorted({x for interval in intervals for x in interval})
        rows.append(('compressEndpoints', intervals)); expected.append([len(ends), *ends])
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
    print(f'PASS Python encounter/stable-rank/endpoint oracle arrays={len(arrays)} checks={len(rows)}', flush=True)
    if binary.name == 'optimized':
        root = Path(__file__).resolve().parents[2]
        headers = [root / '02-Data Structures/07-ordered_set.hpp', root / '06-Miscellaneous/04-compression.hpp']
        body = '''
int main() {
    CoordinateCompression<int> direct({5, 1, 3, 3});
    auto endpoints = compressEndpoints(vector<pair<int, int>>{{3, 5}, {1, 3}});
    if (endpoints.v != direct.v || endpoints.pointRange(1, 3) != pair{0, 1}) { return 1; }
    if (stableRanks(vector<int>{3, 1, 3}) != vector<int>{1, 0, 2}) { return 2; }
    EncounterCompression<int> e;
    if (e.add(7) != 0 || e.add(2) != 1 || e.id(7) != 0) { return 3; }
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
        print('PASS compressEndpoints returns CoordinateCompression: both header orders', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('04-compression', ['encounter-negative', 'encounter-end', 'endpoint-order'], oracle))
