#!/usr/bin/env python3
"""minimumEnclosingCircle (budgeted move-to-front) versus the plain three scans, and the floating version; no timing gates."""
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
    parser.add_argument('--output', type=Path, default=HERE / '14-minimumenclosingcircle_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(cmd):
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=1200)
        if result.returncode:
            raise RuntimeError(f'command={shlex.join(cmd)} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result

    with tempfile.TemporaryDirectory(prefix='cp-p037-mec-bench-') as tmp:
        binary = Path(tmp) / 'benchmark'
        run([compiler, *flags, str(HERE / '14-minimumenclosingcircle_benchmark.cpp'), '-o', str(binary)])
        result = run([str(binary), str(args.seed)])
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    paths = [ROOT / '03-Geometry/14-minimumenclosingcircle.hpp', HERE / '14-minimumenclosingcircle_benchmark.cpp']
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=run([compiler, '--version']).stdout.splitlines()[0],
                  flags=flags, python=platform.python_version(), warmups=1, repetitions=5,
                  units='microseconds', statistic='median', setup='input generation excluded; shuffle included',
                  validation='both exact versions return the same centre and radius2; the floating radius agrees within 1e-9 of the scale; checksum consumed',
                  checksum=result.stderr.strip(),
                  hashes={p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},
                  rows=[json.loads(line) for line in result.stdout.splitlines()])
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for row in record['rows']:
        print(f'{row["case"]:>16} {row["size"]:>8} {row["implementation"]:>30} {row["median_us"]:>10} us')
    print(f'PASS {len(record["rows"])} minimum enclosing circle measurements; seed={args.seed}; {args.output}')


if __name__ == '__main__':
    main()
