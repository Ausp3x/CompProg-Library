#!/usr/bin/env python3
"""Naive-square versus convolutionLong crossover for treeDistanceFrequency, and centroid timings; no timing gate."""
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20260927)
    parser.add_argument('--samples', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '16-centroiddecomposition_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    squares, trees = {}, {}
    with tempfile.TemporaryDirectory(prefix='cp-centroid-bench-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '16-centroiddecomposition_benchmark.cpp'), '-o', str(binary)],
                       check=True, timeout=180)
        subprocess.run([str(binary), str(args.seed)], check=True, capture_output=True, timeout=600)
        for sample in range(args.samples):
            output = subprocess.check_output([str(binary), str(args.seed + sample)], text=True, timeout=600)
            for line in output.splitlines():
                kind, *rest = line.split()
                if kind == 'square':
                    squares.setdefault(int(rest[0]), []).append((float(rest[1]), float(rest[2])))
                else:
                    trees.setdefault((rest[0], int(rest[1]), rest[2]), []).append(float(rest[3]))
    cpu = next((s.split(':', 1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                if s.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, samples=args.samples, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0], flags=flags,
                  warmup='one full run of the binary before the samples',
                  squares={t: dict(naive_us=statistics.median(a for a, _ in v), ntt_us=statistics.median(b for _, b in v))
                           for t, v in sorted(squares.items())},
                  trees=[dict(shape=k[0], n=k[1], op=k[2], ms=statistics.median(v)) for k, v in trees.items()],
                  workloads='squares: uniform counts below 1000, length t; trees: n=200000 random recursive, path, star, '
                            'caterpillar, complete-binary shapes; 200000 contour queries [l, l + 4), l < 8, unit values.')
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for t, v in record['squares'].items():
        print('square', t, v)
    for item in record['trees']:
        print(item)
    print('PASS: all results checked; record:', args.output)


if __name__ == '__main__':
    main()
