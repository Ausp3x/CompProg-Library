#!/usr/bin/env python3
"""Exactly-k fractional selection: Eppstein-Hirschberg against the replaced Dinkelbach engine."""
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
    with tempfile.TemporaryDirectory(prefix='cp-fractional-benchmark-') as directory:
        binary = Path(directory) / 'fractional'
        command = [compiler, *flags, str(HERE / '55-fractional_programming_benchmark.cpp'), '-o', str(binary)]
        subprocess.run(command, check=True, timeout=180)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=1200)
        if result.returncode:
            raise RuntimeError(f'command={binary} returncode={result.returncode}\n{result.stderr}')
    groups = {}
    for line in result.stdout.splitlines():
        family, n, k, method, rep, calls, ms = line.split()
        row = groups.setdefault((family, int(n), int(k), method), {
            'family': family, 'n': int(n), 'k': int(k), 'method': method, 'oracle_calls': [], 'samples_ms': []})
        row['oracle_calls'].append(int(calls))
        row['samples_ms'].append(float(ms))
    rows = list(groups.values())
    if len(rows) != 90 or any(len(row['samples_ms']) != 5 for row in rows):
        raise RuntimeError(f'expected 90 workload/method rows of 5 samples, got {len(rows)}')
    for row in rows:
        row['median_ms'] = statistics.median(row['samples_ms'])
    dinkelbach = {(row['family'], row['n'], row['k']): row for row in rows if row['method'] == 'dinkelbach'}
    for row in rows:
        row['fraction_of_dinkelbach'] = row['median_ms'] / dinkelbach[row['family'], row['n'], row['k']]['median_ms']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    report = {
        'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'cpu': cpu, 'platform': platform.platform(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'flags': flags, 'seed': 20261024, 'warmups': 1, 'repetitions': 5, 'method_order': 'alternated per repetition',
        'workload': 'n in {1e3, 1e5, 1e6}, k in {1, n/2, n-1}; random: a < 1e6, b in [1, 1000]; wide: |a| < 2^40, b in [1, 2^20]; '
                    'duplicates: a in [0, 4], b in [1, 3]; concurrent: b in [1, 1000], a = 7b - 5 (every pair of lines crosses at t = 7); '
                    'tangent: 28 tangents b = 2^(j+2), a = b * (2^30 - 2^j) to (2^30 - t)^2 after n - 28 fillers (0, 2^30), where Dinkelbach halves the gap per call for k = 1.',
        'timed_scope': 'One call of each engine on caller-owned vectors, including its allocations; Eppstein-Hirschberg seeded SEED + rep + 1.',
        'verification': 'Every warmup and timed result: k sorted distinct items whose sums equal num/den, and the same ratio for both engines and every seed; '
                        'the first result per workload also passes the independent zero-optimum certificate. All checks survive NDEBUG.',
        'sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in (ROOT / '06-Miscellaneous/55-fractional_programming.hpp',
                                HERE / '55-fractional_programming_benchmark.cpp', HERE / '55-fractional_programming_benchmark.py')},
        'results': rows,
    }
    output = HERE / '55-fractional_programming_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} workload/method rows; witnesses, equal ratios and certificates verified; record={output}')
    for row in rows:
        if row['method'] == 'eppstein':
            ref = dinkelbach[row['family'], row['n'], row['k']]
            print(f"{row['family']} n={row['n']} k={row['k']}", f"EH {row['median_ms']:.3f} ms",
                  f"Dinkelbach {ref['median_ms']:.3f} ms ({max(ref['oracle_calls'])} calls)", f"{row['fraction_of_dinkelbach']:.3f} x")


if __name__ == '__main__':
    main()
