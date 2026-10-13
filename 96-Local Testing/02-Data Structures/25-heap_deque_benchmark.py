#!/usr/bin/env python3
"""Heap family benchmark: push/pop, meld tournaments and Dijkstra with decreaseKey; timings are not correctness gates."""
import argparse
import json
import os
from pathlib import Path
import platform
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
    parser.add_argument('--seed', type=int, default=20261013)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '25-heap_deque_benchmark.json')
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

    rows = []
    with tempfile.TemporaryDirectory(prefix='cp-p029-benchmark-') as name:
        binary = Path(name) / 'heap_benchmark'
        run([compiler, *flags, str(HERE / '25-heap_deque_benchmark.cpp'), '-o', str(binary)])
        for _ in range(args.runs):
            rows += [json.loads(line) for line in run([str(binary), str(args.seed)]).splitlines()]
    groups = {}
    for row in rows:
        groups.setdefault((row['workload'], row['n'], row['impl']), []).append(row)
    summary = []
    for (workload, n, impl), cases in groups.items():
        summary.append(dict(workload=workload, n=n, impl=impl, checksums=sorted({r['checksum'] for r in cases}),
                            median_seconds=round(statistics.median(r['seconds'] for r in cases), 4)))
    for key in {(r['workload'], r['n']) for r in summary}:
        sums = {tuple(r['checksums']) for r in summary if (r['workload'], r['n']) == key}
        if len(sums) != 1:
            raise RuntimeError(f'checksum mismatch {key}: {sums}')
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=run([compiler, '--version']).splitlines()[0], flags=flags,
                  platform=platform.platform(), cpu=cpu,
                  workload='pushpop: n uniform keys in [0, 1e12) pushed then popped; meld: n/16 heaps of 16 keys melded '
                           'pairwise in rounds then drained; dijkstra: random directed graph, weights in [1, 1e9], source 0',
                  results=summary)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['workload']:22} n={r['n']:>8} {r['impl']:28} median={r['median_seconds']:.4f}s")


if __name__ == '__main__':
    main()
