#!/usr/bin/env python3
"""maxPointsInDisk with the lll comparator versus the 384-bit comparator, and GridHash versus SortedGrid; no timing gates."""
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
    parser.add_argument('--output', type=Path, default=HERE / '54-fixed_radius_queries_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(cmd):
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=1200)
        if result.returncode:
            raise RuntimeError(f'command={shlex.join(cmd)} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result

    with tempfile.TemporaryDirectory(prefix='cp-p037-disk-bench-') as tmp:
        binary = Path(tmp) / 'benchmark'
        run([compiler, *flags, str(HERE / '54-fixed_radius_queries_benchmark.cpp'), '-o', str(binary)])
        result = run([str(binary), str(args.seed)])
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    paths = [ROOT / '03-Geometry/54-fixed_radius_queries.hpp', HERE / '54-fixed_radius_queries_benchmark.cpp']
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=run([compiler, '--version']).stdout.splitlines()[0],
                  flags=flags, python=platform.python_version(), warmups=1, repetitions=5,
                  units='microseconds', statistic='median',
                  setup='scaled: r2 = 10^8 with uniform points in a square sized for the stated candidates per pivot (side at most 10^6), the wide run uses the same points times 1000 and r2 times 10^6; threshold: the same points at r2 = 2^30 (lll) and 2^30 + 1 (wide); grids: r = 1000, uniform points with about 8 neighbours or clusters of 10 in a 300 x 300 square, GridHash and SortedGrid on the same points',
                  validation='both scales report the same count; the larger threshold radius covers at least as many points; both grids return identical pair lists and cover counts; checksum consumed',
                  checksum=result.stderr.strip(),
                  hashes={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  rows=[json.loads(line) for line in result.stdout.splitlines()])
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for row in record['rows']:
        print(' '.join(f'{k}={v}' for k, v in row.items()))
    print(f'PASS {len(record["rows"])} maxPointsInDisk and grid measurements; seed={args.seed}; {args.output}')


if __name__ == '__main__':
    main()
