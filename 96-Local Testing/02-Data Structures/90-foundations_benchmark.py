#!/usr/bin/env python3
"""Reproducible P006 construction comparison; timings are not correctness gates."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20260927)
    parser.add_argument('--runs', type=int, default=7)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    with tempfile.TemporaryDirectory(prefix='cp-p006-benchmark-') as name:
        binary = Path(name) / 'benchmark'
        run([compiler, *flags, str(HERE / '90-foundations_benchmark.cpp'), '-o', str(binary)])
        rows = [json.loads(line) for _ in range(args.runs)
                for line in run([str(binary), str(args.seed)]).splitlines()]
    groups = {}
    for row in rows:
        key = (row['workload'], row['variant'], row['n'])
        groups.setdefault(key, []).append(row)
    summary = []
    for (workload, variant, n), cases in groups.items():
        assert len({(r['checksum'], r['repeats']) for r in cases}) == 1
        summary.append(dict(workload=workload, variant=variant, n=n,
                            repeats=cases[0]['repeats'], checksum=cases[0]['checksum'],
                            median_seconds=statistics.median(r['seconds'] for r in cases),
                            samples_seconds=[r['seconds'] for r in cases]))
    paths = [ROOT / '02-Data Structures/02-fenwick.hpp',
             HERE / '90-foundations_benchmark.cpp', Path(__file__)]
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=run([compiler, '--version']).splitlines()[0],
                  flags=flags, platform=platform.platform(), cpu=cpu,
                  workload='Uniform integer values [0,100]; both timed paths include allocation/build/destruction; one untimed linear warmup per row; checksum query included; O(n) storage.',
                  hashes={str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  results=summary)
    text = json.dumps(record, indent=2) + '\n'
    if args.output:
        args.output.write_text(text)
    print(text, end='')


if __name__ == '__main__':
    main()
