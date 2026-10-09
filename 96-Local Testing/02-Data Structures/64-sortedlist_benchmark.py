#!/usr/bin/env python3
"""SortedList bucket-constant sweep against a PBDS order-statistics tree; timings are not correctness gates."""
import argparse
import json
import os
from pathlib import Path
import platform
import re
import shutil
import statistics
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
HEADER = ROOT / '02-Data Structures/64-sortedlist.hpp'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261009)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '64-sortedlist_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    source = HEADER.read_text()
    current = re.search(r'BMIN = (\d+), RATIO = (\d+);', source)
    variants = sorted({(int(current[1]), int(current[2]))} | {(32, r) for r in (1, 2, 4, 8, 16, 32, 64)} | {(16, 8), (64, 8), (128, 8)})

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=600)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    rows = []
    with tempfile.TemporaryDirectory(prefix='cp-p227-benchmark-') as name:
        base = Path(name)
        os.symlink(ROOT / '01-Core', base / '01-Core')
        (base / '02-Data Structures').mkdir()
        (base / '96-Local Testing/02-Data Structures').mkdir(parents=True)
        shutil.copy(HERE / '64-sortedlist_benchmark.cpp', base / '96-Local Testing/02-Data Structures')
        binaries = []
        for bmin, ratio in variants:
            (base / '02-Data Structures/64-sortedlist.hpp').write_text(
                source.replace(current[0], f'BMIN = {bmin}, RATIO = {ratio};'))
            binary = base / f'b{bmin}r{ratio}'
            run([compiler, *flags, f'-DVARIANT="BMIN={bmin} RATIO={ratio}"',
                 str(base / '96-Local Testing/02-Data Structures/64-sortedlist_benchmark.cpp'), '-o', str(binary)])
            binaries.append([str(binary), str(args.seed)])
        binaries.append([binaries[0][0], str(args.seed), 'tree'])
        for _ in range(args.runs):
            for command in binaries:
                rows += [json.loads(line) for line in run(command).splitlines()]
    groups = {}
    for row in rows:
        groups.setdefault((row['workload'], row['n'], row['variant']), []).append(row)
    summary = []
    for (workload, n, variant), cases in sorted(groups.items()):
        summary.append(dict(workload=workload, n=n, q=cases[0]['q'], variant=variant,
                            checksums=sorted({r['checksum'] for r in cases}),
                            median_seconds=round(statistics.median(r['seconds'] for r in cases), 4)))
    for workload in {r['workload'] for r in summary}:
        for n in {r['n'] for r in summary if r['workload'] == workload}:
            sums = {tuple(r['checksums']) for r in summary if r['workload'] == workload and r['n'] == n}
            if len(sums) != 1:
                raise RuntimeError(f'checksum mismatch workload={workload} n={n}: {sums}')
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=run([compiler, '--version']).splitlines()[0],
                  flags=flags, platform=platform.platform(), cpu=cpu, library=f'BMIN={current[1]} RATIO={current[2]}',
                  workload='mixed: n random inserts of values in [0, 4n), then q ops 25% insert, 25% eraseOne, 20% rank, '
                           '20% kth, 10% lowerBound; ascending-then-popFront: n ascending inserts then n popFront',
                  results=summary)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['workload']:24} n={r['n']:>8} {r['variant']:22} median={r['median_seconds']:.4f}s")


if __name__ == '__main__':
    main()
