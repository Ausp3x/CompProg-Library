#!/usr/bin/env python3
"""XorBasis benchmark: the old mask-free basis against XorBasis<true> and <false>, plus Graphs row 28 end to end, one binary per implementation; timings are not correctness gates."""
import argparse
import json
import os
from pathlib import Path
import platform
import resource
import statistics
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '62-xorbasis_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=1800)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    names = ['old', 'XorBasis<true>', 'XorBasis<false>', 'header']
    rows = []
    with tempfile.TemporaryDirectory(prefix='cp-p032-benchmark-') as name:
        binaries = [Path(name) / f'xorbasis_benchmark_{k}' for k in range(len(names))]
        for k, binary in enumerate(binaries):
            run([compiler, *flags, f'-DIMPL={k}', str(HERE / '62-xorbasis_benchmark.cpp'), '-o', str(binary)])
        # Rotate the order each repetition so no implementation always runs first.
        for rep in range(args.runs):
            for k in range(len(names)):
                binary = binaries[(k + rep) % len(names)]
                rows += [json.loads(line) for line in run([str(binary), str(args.seed)]).splitlines()]
    for row in rows:
        row['impl'] = names[row['impl']]
    peak_mb = resource.getrusage(resource.RUSAGE_CHILDREN).ru_maxrss // 1024
    groups = {}
    for row in rows:
        groups.setdefault((row['workload'], row['impl']), []).append(row)
    summary = []
    for (workload, impl), cases in groups.items():
        summary.append(dict(workload=workload, impl=impl, checksums=sorted({r['checksum'] for r in cases}),
                            median_seconds=round(statistics.median(r['seconds'] for r in cases), 4)))
    for workload in {r['workload'] for r in summary}:
        sums = {tuple(r['checksums']) for r in summary if r['workload'] == workload}
        if len(sums) != 1:
            raise RuntimeError(f'checksum mismatch {workload}: {sums}')
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=run([compiler, '--version']).splitlines()[0], flags=flags,
                  platform=platform.platform(), cpu=cpu, peak_child_rss_mb=peak_mb,
                  warmup='one untimed pass per workload in each process', order='rotated per repetition',
                  workload='inserts: 5e6 random 64-bit, fresh basis per 1000; cycles: 1e6 random 63-bit into one basis; '
                           'cuts: 2e5 fresh bases of 40 random words; XorWalk build: n=2e5, m=1e6 random 63-bit weights; '
                           'disconnects: 2e5 sets of 40 distinct edges',
                  results=summary)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['workload']:14} {r['impl']:16} median={r['median_seconds']:.4f}s")
    print(f'peak child RSS {peak_mb} MB')


if __name__ == '__main__':
    main()
