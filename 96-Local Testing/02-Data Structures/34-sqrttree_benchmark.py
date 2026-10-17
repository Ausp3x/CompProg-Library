#!/usr/bin/env python3
"""SqrtTree layering-threshold sweep (stop at lay <= 1, 2, 3, 4); timings are not correctness gates."""
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
HEADER = ROOT / '02-Data Structures/34-sqrttree.hpp'
FIELDS = ('build', 'random', 'short', 'sets')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261017)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '34-sqrttree_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    source = HEADER.read_text()
    current = re.search(r'for \(int t = lg; t > (\d+); t = \(t \+ 1\) / 2\)', source)

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
        (base / '96-Local Testing/02-Data Structures/34-sqrttree_benchmark.cpp').write_text((HERE / '34-sqrttree_benchmark.cpp').read_text())
        binaries = []
        for stop in (1, 2, 3, 4):
            (base / '02-Data Structures/34-sqrttree.hpp').write_text(source.replace(current[0], current[0].replace(f't > {current[1]}', f't > {stop}')))
            binary = base / f'stop{stop}'
            run([compiler, *flags, f'-DVARIANT="lay>{stop}"', str(base / '96-Local Testing/02-Data Structures/34-sqrttree_benchmark.cpp'), '-o', str(binary)])
            binaries.append([str(binary), str(args.seed)])
        for _ in range(args.runs):
            for command in binaries:
                rows += [json.loads(line) for line in run(command).splitlines()]
    groups = {}
    for row in rows:
        groups.setdefault((row['workload'], row['n'], row['variant']), []).append(row)
    summary = []
    for (workload, n, variant), cases in sorted(groups.items()):
        summary.append(dict(workload=workload, n=n, q=cases[0]['q'], variant=variant, layers=cases[0]['layers'],
                            cells_per_n=round(cases[0]['cells_per_n'], 2), checksums=sorted({r['checksum'] for r in cases}),
                            **{f'median_{k}': round(statistics.median(r[k] for r in cases), 4) for k in FIELDS}))
    for key in {(r['workload'], r['n']) for r in summary}:
        sums = {tuple(r['checksums']) for r in summary if (r['workload'], r['n']) == key}
        if len(sums) != 1:
            raise RuntimeError(f'checksum mismatch {key}: {sums}')
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=run([compiler, '--version']).splitlines()[0], flags=flags,
                  platform=platform.platform(), cpu=cpu, library=f'lay>{current[1]}', results=summary,
                  workload='build; q uniform random ranges; q ranges of length 1..16; q/200 random point sets')
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['workload']:6} n={r['n']:>8} {r['variant']:6} layers={r['layers']} cells/n={r['cells_per_n']:5} "
              + ' '.join(f"{k}={r['median_' + k]:.4f}s" for k in FIELDS))


if __name__ == '__main__':
    main()
