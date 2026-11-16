"""Exact first-encounter ids (ordered and hashed), stable ranks, argsort, counting builds, point and endpoint compression against independent Python scans."""
import os
from pathlib import Path
import random
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from _00_contest_runner import main
from _00_cpp_test_runner import timed_run
from _00_cpp_test_runner import Builder, compiler_command


def oracle(binary, args, env):
    rng = random.Random(args.seed)
    lo, hi = -(1 << 63), (1 << 63) - 1
    arrays = [[], [0], [lo, hi, lo], [5, 5, 1, 3, 1], list(range(-5, 6)), [lo + 1, lo, lo + 1, lo + 4],
              [hi, hi - 3, hi, hi - 1], [-(1 << 19), 0, (1 << 19) - 1, 0]]
    count = {'quick': 100, 'full': 1500, 'stress': 15000}[args.mode]
    families = [lambda: rng.randint(-7, 7), lambda: rng.randint(lo, hi),
                lambda: lo + rng.randrange(20), lambda: hi - rng.randrange(20)]
    for i in range(count):
        arrays.append([families[i % 4]() for _ in range(rng.randrange(101))])
    payload, rows, expected = [], [], []
    for a in arrays:
        intervals = [(lo, hi)] + [(x, x) for x in [lo, -1, 0, 1, hi]]
        intervals += [tuple(sorted([rng.randint(lo, hi), rng.randint(lo, hi)])) for _ in range(10)]
        intervals += [tuple(sorted(a[i:i + 2])) for i in range(0, len(a) - 1, 2)]
        counting = not a or max(a) - min(a) < 1 << 20
        payload.append(f'{len(a)} {len(intervals)} {int(counting)}\n' + ' '.join(map(str, a)) + '\n'
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
        rows.append(('argsort', a)); expected.append(order)
        rows.append(('HashedEncounterCompression', a)); expected.append(encounter)
        axis = sorted(set(a))
        dense = {x: k for k, x in enumerate(axis)}
        rows.append(('compressPoints', list(zip(a, a[::-1]))))
        expected.append([len(axis), len(axis)] + [dense[x] for pair in zip(a, a[::-1]) for x in pair])
        if counting:
            rows.append(('compressCounting ids', a)); expected.append([dense[x] for x in a])
            rows.append(('compressCounting values', a)); expected.append(axis)
            rows.append(('stableRanksCounting', a)); expected.append(stable)
    command = [str(binary), '--oracle']
    result = timed_run(f'04-compression oracle {Path(command[0]).name}', command, input=''.join(payload), text=True, capture_output=True, env=env, timeout=180)
    if result.returncode:
        raise RuntimeError(f'command={command} returncode={result.returncode}\n{result.stderr}')
    actual = [list(map(int, line.split())) for line in result.stdout.splitlines()]
    if len(actual) != len(expected):
        raise RuntimeError(f'expected lines={len(expected)} actual={len(actual)}')
    for row, want, got in zip(rows, expected, actual):
        if want != got:
            raise RuntimeError(f'smallest known reproducer={row} expected={want} actual={got}')
    print(f'PASS Python encounter/stable-rank/endpoint/argsort/counting/points/hashed oracle arrays={len(arrays)} checks={len(rows)}', flush=True)
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
    if (argsort(vector<int>{3, 1, 3}) != vector<int>{1, 0, 2} || stableRanksCounting({3, 1, 3}) != vector<int>{1, 0, 2}) { return 4; }
    if (compressCounting({5, 1, 3, 3}).second != vector<lng>{1, 3, 5}) { return 5; }
    auto points = compressPoints(vector<pair<int, int>>{{5, 1}, {1, 3}});
    if (points.xs.v != vector<int>{1, 5} || points.ids != vector<pair<int, int>>{{1, 0}, {0, 1}}) { return 6; }
    HashedEncounterCompression<int> h;
    if (h.add(7) != 0 || h.add(2) != 1 || h.id(7) != 0 || h.id(3) != -1) { return 7; }
    return 0;}
'''
        with tempfile.TemporaryDirectory(prefix='cp-compression-includes-') as name:
            directory = Path(name)
            builder = Builder(compiler_command(), directory, env=env, failure=RuntimeError)
            for order in [headers, headers[::-1]]:
                source = directory / 'test.cpp'
                source.write_text(''.join('#include "' + str(header) + '"\n' for header in order) + body)
                builder.build('both header orders', ['-std=gnu++20', '-O2', '-DNDEBUG'], [source], directory / 'test', 180)
                p = timed_run('04-compression header-order run', [str(directory / 'test')], capture_output=True, text=True, env=env, timeout=180)
                if p.returncode:
                    raise RuntimeError(f'command={directory / "test"} returncode={p.returncode}\n{p.stdout}\n{p.stderr}')
        print('PASS compressEndpoints returns CoordinateCompression: both header orders', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('04-compression', ['encounter-negative', 'encounter-end', 'endpoint-order', 'counting-span', 'counting-full-span',
                                             'counting-ranks-span', 'counting-ranks-full-span', 'hashed-negative', 'hashed-end', 'encounter-past-end', 'hashed-past-end',
                                             'argsort-max-size'], oracle))
