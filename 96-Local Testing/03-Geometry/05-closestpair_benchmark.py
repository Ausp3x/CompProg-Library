#!/usr/bin/env python3
"""Reproducible closest-pair/legacy/brute timings; no timing pass/fail gates."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20260927)
    parser.add_argument('--output', type=Path, default=HERE / '05-closestpair_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(cmd):
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise RuntimeError(f'command={shlex.join(cmd)} returncode={result.returncode}\n'
                               f'{result.stdout}\n{result.stderr}')
        return result

    with tempfile.TemporaryDirectory(prefix='cp-p008-closest-bench-') as tmp:
        binary = Path(tmp) / 'benchmark'
        run([compiler, *flags, str(HERE / '05-closestpair_benchmark.cpp'), '-o', str(binary)])
        result = run([str(binary), str(args.seed)])
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    paths = [ROOT / '03-Geometry/05-closestpair.hpp', ROOT / '03-Geometry/01-point.hpp',
             ROOT / 'OLD/Team Notebook/src/geometry/getnearestpair.cpp',
             HERE / '05-closestpair_benchmark.cpp']
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=run([compiler, '--version']).stdout.splitlines()[0],
                  flags=flags, python=platform.python_version(), warmups=1, repetitions=5,
                  units='microseconds', statistic='median',
                  setup='algorithm allocations and sorting included; input generation and result checking excluded',
                  memory='closestPair: O(n) auxiliary, 2 * n int arrays generally and n int indices for duplicate/axis-collinear early returns, plus sorting/recursion stack; legacy: 2 * n int arrays plus sorting/recursion stack; brute: O(1)',
                  validation='Brute-force exact pair/distance comparison for n<=2048; every timed output checked; large cases use tested implementation and legacy distance agreement; checksum consumed',
                  limits='Legacy uses 64-bit distance, finite INF64 and unspecified pair ties; benchmarks use safe bounded coordinates and compare its distance only. Shared-host timing is descriptive.',
                  checksum=result.stderr.strip(),
                  hashes={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  rows=[json.loads(line) for line in result.stdout.splitlines()])
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    print(f'PASS {len(record["rows"])} closest-pair measurements; seed={args.seed}; {args.output}')


if __name__ == '__main__':
    main()
