#!/usr/bin/env python3
"""Revolving-door dispatch: public forEachGrayCombination against forced Algorithm R and loopless BER, ns per output."""
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
REPETITIONS = 7


def main():
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-enumeration-benchmark-') as directory:
        binary = Path(directory) / 'enumeration'
        subprocess.run([compiler, *flags, str(HERE / '12-enumeration_benchmark.cpp'), '-o', str(binary)], check=True, timeout=300)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=1800)
        if result.returncode:
            raise RuntimeError(f'command={binary} returncode={result.returncode}\n{result.stderr}')
    rows = {}
    for line in result.stdout.splitlines():
        n, k, impl, rep, count, checksum, ns = line.split()
        row = rows.setdefault((int(n), int(k)), {'n': int(n), 'k': int(k), 'count': int(count), 'checksum': int(checksum), 'samples_ns': {}})
        # Every implementation and repetition must emit the same sequence: same count and order-sensitive checksum.
        if (row['count'], row['checksum']) != (int(count), int(checksum)):
            raise RuntimeError(f'output mismatch at n={n} k={k} impl={impl} rep={rep}')
        row['samples_ns'].setdefault(impl, []).append(float(ns))
    for row in rows.values():
        if sorted(row['samples_ns']) != ['R', 'loopless', 'public'] or any(len(v) != REPETITIONS for v in row['samples_ns'].values()):
            raise RuntimeError(f'missing samples for n={row["n"]} k={row["k"]}')
        row['median_ns'] = {impl: statistics.median(v) for impl, v in row['samples_ns'].items()}
        row['ratio'] = row['k'] / (row['n'] - row['k'] + 1)
        row['dispatch'] = 'loopless' if row['k'] > 3 * (row['n'] - row['k'] + 1) else 'R'
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    report = {
        'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'cpu': cpu, 'platform': platform.platform(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'flags': flags, 'python': platform.python_version(), 'warmups': 1, 'repetitions': REPETITIONS,
        'method_order': 'rotated per repetition',
        'workload': 'Full revolving-door output of each (n, k); the visitor folds a[0], a[k-1], removed and added into an order-sensitive checksum.',
        'sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in (ROOT / '06-Miscellaneous/12-enumeration.hpp',
                                HERE / '12-enumeration_benchmark.cpp', HERE / '12-enumeration_benchmark.py')},
        'results': list(rows.values()),
    }
    output = HERE / '12-enumeration_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} rows; every implementation emitted the same sequence; record={output}')
    for row in rows.values():
        m = row['median_ns']
        print(f"n={row['n']} k={row['k']} k/(n-k+1)={row['ratio']:.2f} dispatch={row['dispatch']}",
              f"public {m['public']:.2f} R {m['R']:.2f} loopless {m['loopless']:.2f} ns/output")


if __name__ == '__main__':
    main()
