#!/usr/bin/env python3
"""Compare GE01's two O(n log n) hulls; timing is separate from correctness."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20260927)
    parser.add_argument('--output', type=Path, default=HERE / '00-foundations_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(cmd):
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise RuntimeError(f'command={shlex.join(cmd)} returncode={result.returncode}\n'
                               f'{result.stdout}\n{result.stderr}')
        return result

    with tempfile.TemporaryDirectory(prefix='cp-p007-bench-') as tmp:
        binary = Path(tmp) / 'benchmark'
        run([compiler, *flags, str(HERE / '00-foundations_benchmark.cpp'), '-o', str(binary)])
        result = run([str(binary), str(args.seed)])
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    paths = [ROOT / '03-Geometry/01-point.hpp', ROOT / '03-Geometry/04-convexhull.hpp',
             HERE / '00-foundations_benchmark.cpp']
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=run([compiler, '--version']).stdout.splitlines()[0],
                  flags=flags, python=platform.python_version(), warmups=1, repetitions=5,
                  units='microseconds', statistic='median', setup='input copy and output allocation included; input generation excluded',
                  memory='Both use O(n) auxiliary/returned point storage; no measured peak RSS claim.',
                  validation='Exact canonical hull equality before and after each timed call; checksum consumed.',
                  checksum=result.stderr.strip(),
                  hashes={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  rows=[json.loads(line) for line in result.stdout.splitlines()])
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    print(f'PASS {len(record["rows"])} hull measurements; seed={args.seed}; {args.output}')


if __name__ == '__main__':
    main()
