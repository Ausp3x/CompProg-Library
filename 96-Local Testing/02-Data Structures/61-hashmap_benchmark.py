#!/usr/bin/env python3
"""HashMap against unordered_map and gp_hash_table with the same CustomHash; timings are not correctness gates."""
import argparse
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261009)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '61-hashmap_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=600)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    rows = []
    with tempfile.TemporaryDirectory(prefix='cp-p227-benchmark-') as name:
        binary = Path(name) / 'benchmark'
        run([compiler, *flags, str(HERE / '61-hashmap_benchmark.cpp'), '-o', str(binary)])
        for _ in range(args.runs):
            rows += [json.loads(line) for line in run([str(binary), str(args.seed)]).splitlines()]
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
                  flags=flags, platform=platform.platform(), cpu=cpu, workload='q = 4e6 ops over a pool of n keys (random 63-bit or i << 20): 40% operator[] +=, 40% find, 20% erase; '
                           'construction and destruction timed; default (process-random) CustomHash seed',
                  results=summary)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['workload']:24} n={r['n']:>8} {r['variant']:26} median={r['median_seconds']:.4f}s")


if __name__ == '__main__':
    main()
