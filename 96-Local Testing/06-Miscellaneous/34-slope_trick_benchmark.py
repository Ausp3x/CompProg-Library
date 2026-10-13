#!/usr/bin/env python3
"""Slope trick workloads: unit heaps vs treap, weighted treap vs pair heaps, dual heaps vs treap."""
import datetime
import hashlib
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
ROOT = HERE.parents[1]


def main():
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-slope-trick-benchmark-') as directory:
        binary = Path(directory) / 'slope_trick'
        subprocess.run([compiler, *flags, str(HERE / '34-slope_trick_benchmark.cpp'), '-o', str(binary)], check=True, timeout=300)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=1800)
        if result.returncode:
            raise RuntimeError(f'command={binary} returncode={result.returncode}\n{result.stderr}')
    rows, answers = {}, {}
    for line in result.stdout.splitlines():
        workload, impl, n, rep, ms, checksum = line.split()
        row = rows.setdefault((workload, int(n), impl), {'workload': workload, 'n': int(n), 'impl': impl, 'samples_ms': []})
        row['samples_ms'].append(float(ms))
        expected = answers.setdefault((workload, int(n), int(rep)), checksum)
        if expected != checksum:
            raise RuntimeError(f'implementations disagree on {workload} n={n} rep={rep}: {expected} vs {checksum}')
    for row in rows.values():
        if len(row['samples_ms']) != 5:
            raise RuntimeError(f'expected 5 repetitions for {row}')
        row['median_ms'] = statistics.median(row['samples_ms'])
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    report = {
        'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'cpu': cpu, 'platform': platform.platform(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'flags': flags, 'python': platform.python_version(), 'seed': 20261013,
        'warmups': 1, 'repetitions': 5, 'method_order': 'rotated per repetition',
        'workloads': {
            'monotone-fit': 'n times prefixMin then addAbs(a), a uniform in [0, 1e9]; answer min',
            'bounded-step': 'n times slide(-1000, 1000) then addAbs(a); answer min',
            'weighted-fit': 'monotone fit with weights in [1, 8]: one weighted addAbs vs w repeated unit addAbs',
            'ping-pong': 'n unit addAbs at distinct points, each followed by addAbs(+-1e6, n) alternating sides; pair-heaps is the benchmark-only weighted-heap design',
            'tree-merge': 'n nodes, parent uniform among the previous 64; per node prefixMin, addAbs(a), merge into parent; answer root min',
            'choose-half': 'n items, per item addLinear(-d), slide(0, 1), addLinear(d), addConst(p); answer f(n/2)',
        },
        'timed_scope': 'Construction of the structures, every operation and the final query; input generation outside timing.',
        'verification': 'Every repetition of every implementation of a workload returns the same answer (checked here).',
        'sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in (ROOT / '06-Miscellaneous/34-slope_trick.hpp',
                                HERE / '34-slope_trick_benchmark.cpp', HERE / '34-slope_trick_benchmark.py')},
        'results': sorted(rows.values(), key=lambda r: (r['workload'], r['n'], r['impl'])),
    }
    output = HERE / '34-slope_trick_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} workload/implementation rows; answers agree across implementations; record={output}')
    for row in report['results']:
        print(f"{row['workload']:13} n={row['n']:<8} {row['impl']:14} median {row['median_ms']:10.3f} ms")


if __name__ == '__main__':
    main()
