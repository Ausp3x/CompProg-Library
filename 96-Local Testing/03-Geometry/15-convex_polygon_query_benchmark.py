#!/usr/bin/env python3
"""Inscribed k-gons (plain, SMAWK, linear triangle and quadrilateral, large-k memory), batched location and exact queries; no timing gates."""
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
    parser.add_argument('--output', type=Path, default=HERE / '15-convex_polygon_query_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(cmd):
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=1800)
        if result.returncode:
            raise RuntimeError(f'command={shlex.join(cmd)} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result

    with tempfile.TemporaryDirectory(prefix='cp-p037-kgon-bench-') as tmp:
        binary = Path(tmp) / 'benchmark'
        run([compiler, *flags, str(HERE / '15-convex_polygon_query_benchmark.cpp'), '-o', str(binary)])
        result = run([str(binary), str(args.seed)])
        rows = [json.loads(line) for line in result.stdout.splitlines()]
        checksums = [result.stderr.strip()]
        # Large k, one process per case so that each peak resident set is its own.
        for k in (1000, 10000):
            for impl in ('plain', 'smawk'):
                r = run([str(binary), str(args.seed), '--memory', str(k), impl])
                rows += [json.loads(line) for line in r.stdout.splitlines()]
                checksums.append(r.stderr.strip())
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    paths = [ROOT / '03-Geometry/15-convex_polygon_query.hpp', HERE / '15-convex_polygon_query_benchmark.cpp']
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=run([compiler, '--version']).stdout.splitlines()[0],
                  flags=flags, python=platform.python_version(), warmups=1, repetitions=5,
                  units='microseconds', statistic='median (large k: one run, peak resident set in KB)',
                  setup='input generation and hull construction excluded',
                  validation='plain and Smawk k-gons agree (perimeters within 1e-15 relative); linear triangle and quadrilateral equal the general k = 3, 4; batches equal per-query contains; checksum consumed',
                  checksum=checksums,
                  hashes={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  rows=rows)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for row in record['rows']:
        value = row.get('median_us', row.get('single_run_us'))
        extra = f" peak={row['peak_rss_kb']} KB" if 'peak_rss_kb' in row else ''
        print(f'{row["case"]:>26} {row["size"]:>22} {row["implementation"]:>38} {value:>12} us{extra}')
    print(f'PASS {len(record["rows"])} convex polygon measurements; seed={args.seed}; {args.output}')


if __name__ == '__main__':
    main()
