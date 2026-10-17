#!/usr/bin/env python3
"""PersistentArray arity sweep (B = 1..5) over array and PersistentDSU workloads; timings are not correctness gates."""
import argparse
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
HEADERS = ('35-persistentarray.hpp', '10-persistentdsu.hpp')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261017)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '35-persistentarray_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    source = (ROOT / '02-Data Structures' / HEADERS[0]).read_text()
    current = re.search(r'template<typename T, int B = (\d+)>', source)

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=1200)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    rows = []
    with tempfile.TemporaryDirectory(prefix='cp-p034-benchmark-') as name:
        base = Path(name)
        os.symlink(ROOT / '01-Core', base / '01-Core')
        (base / '02-Data Structures').mkdir()
        (base / '96-Local Testing/02-Data Structures').mkdir(parents=True)
        (base / '96-Local Testing/02-Data Structures/35-persistentarray_benchmark.cpp').write_text((HERE / '35-persistentarray_benchmark.cpp').read_text())
        (base / '02-Data Structures' / HEADERS[1]).write_text((ROOT / '02-Data Structures' / HEADERS[1]).read_text())
        binaries = []
        for b in range(1, 6):
            (base / '02-Data Structures' / HEADERS[0]).write_text(source.replace(current[0], f'template<typename T, int B = {b}>'))
            binary = base / f'b{b}'
            run([compiler, *flags, f'-DVARIANT="B={b}"', str(base / '96-Local Testing/02-Data Structures/35-persistentarray_benchmark.cpp'), '-o', str(binary)])
            binaries.append([str(binary), str(args.seed)])
        for _ in range(args.runs):
            for command in binaries:
                rows += [json.loads(line) for line in run(command).splitlines()]
    groups = {}
    for row in rows:
        groups.setdefault((row['workload'], row['n'], row['variant']), []).append(row)
    summary = []
    for (workload, n, variant), cases in sorted(groups.items()):
        summary.append(dict(workload=workload, n=n, q=cases[0]['q'], variant=variant, checksums=sorted({r['checksum'] for r in cases}),
                            pool_mb=round(cases[0]['pool_bytes'] / 2**20, 1), median_seconds=round(statistics.median(r['seconds'] for r in cases), 4)))
    for key in {(r['workload'], r['n']) for r in summary}:
        sums = {tuple(r['checksums']) for r in summary if (r['workload'], r['n']) == key}
        if len(sums) != 1:
            raise RuntimeError(f'checksum mismatch {key}: {sums}')
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=run([compiler, '--version']).splitlines()[0], flags=flags,
                  platform=platform.platform(), cpu=cpu, library=f'B={current[1]}', results=summary,
                  workload='array-mixed: q ops half set, half get on random versions; array-chain: q sets on the latest version '
                           'then q gets on random versions; dsu-checker: query i on a random earlier version, half uniteSets, '
                           'half isSameSet; dsu-chain: q unions on the latest version then q isSameSet + getSize on random versions')
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['workload']:12} n={r['n']:>8} q={r['q']:>8} {r['variant']:4} median={r['median_seconds']:.4f}s pool={r['pool_mb']}MB")


if __name__ == '__main__':
    main()
