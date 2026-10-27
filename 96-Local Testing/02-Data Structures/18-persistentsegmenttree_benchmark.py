#!/usr/bin/env python3
"""Persistent segment tree family: COMMUTATIVE on/off, the f == id skip, PersistentValueTree against WaveletMatrix, PersistentDistinctCount; timings are not correctness gates."""
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
SOURCE = HERE / '18-persistentsegmenttree_benchmark.cpp'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261027)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '18-persistentsegmenttree_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=1200)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    rows = []
    with tempfile.TemporaryDirectory(prefix='cp-p030-benchmark-') as name:
        binary = str(Path(name) / 'bench')
        run([compiler, *flags, str(SOURCE), '-o', binary])
        run([binary, str(args.seed)])  # warmup, discarded
        for _ in range(args.runs):
            rows += [json.loads(line) for line in run([binary, str(args.seed)]).splitlines()]
    groups = {}
    for row in rows:
        groups.setdefault((row['workload'], row['variant']), []).append(row)
    summary = []
    for (workload, variant), cases in groups.items():
        if len({(r['checksum'], r['nodes']) for r in cases}) != 1:
            raise RuntimeError(f'nondeterministic result {workload} {variant}')
        summary.append(dict(workload=workload, variant=variant, n=cases[0]['n'], q=cases[0]['q'], checksum=cases[0]['checksum'],
                            nodes=cases[0]['nodes'], median_ms=round(1000 * statistics.median(r['seconds'] for r in cases), 2)))
    for workload in {r['workload'] for r in summary}:
        if workload != 'q4-build' and len({r['checksum'] for r in summary if r['workload'] == workload}) != 1:
            raise RuntimeError(f'variants disagree on {workload}')
    if len({r['checksum'] for r in summary if r['workload'] == 'q4-build'}) != 1:
        raise RuntimeError('builds disagree on the median element')
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, warmup=1, compiler=run([compiler, '--version']).splitlines()[0], flags=flags,
                  platform=platform.platform(), cpu=cpu, results=summary,
                  workload='q1: RangeAddRangeSum<lng>, n = q = 2e5, values [0, 1e9), adds [0, 1e4], ops on a random or the latest version, '
                           'mixed = 50% apply 50% prod, apply = 100% apply; q2: same on PersistentDualSegmentTree<MonoidAction<Sum<lng>>> '
                           'with point get; q3: affine mod 998244353, 2e4 applies chained on the latest version, then 2e5 ops on random '
                           'versions, 50% set 50% prod, F with and without operator==; q4: n = q = 2e5 values [0, 1e9) for '
                           'PersistentValueTree and WaveletMatrix, n = q = 5e5 values [0, n) for PersistentDistinctCount; '
                           'nodes = nodes allocated by the workload (q3 per phase)')
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    print(f'cpu={cpu} compiler={record["compiler"]} flags={" ".join(flags)} seed={args.seed} runs={args.runs} warmup=1')
    for r in summary:
        print(f"{r['workload']:20} {r['variant']:12} n={r['n']:>7} q={r['q']:>7} median={r['median_ms']:9.2f}ms nodes={r['nodes']}")


if __name__ == '__main__':
    main()
