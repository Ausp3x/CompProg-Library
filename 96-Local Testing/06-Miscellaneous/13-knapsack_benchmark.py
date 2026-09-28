#!/usr/bin/env python3
"""Complete DP/query/witness pipelines against direct multiplicity recurrence."""
import datetime
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
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-knapsack-benchmark-') as directory:
        binary = Path(directory) / 'knapsack'
        subprocess.run([compiler, *flags, str(HERE / '13-knapsack_benchmark.cpp'), '-o', str(binary)],
                       check=True, timeout=180)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise RuntimeError(f'command={binary} returncode={result.returncode}\n{result.stderr}')
    groups = {}
    for line in result.stdout.splitlines():
        capacity, n, distribution, trace, method, rep, batch, ms, checksum = line.split()
        key = (int(capacity), distribution, bool(int(trace)), method)
        row = groups.setdefault(key, {'capacity': int(capacity), 'items': int(n),
                                    'distribution': distribution, 'trace': bool(int(trace)),
                                    'method': method, 'batch': int(batch), 'checksum': int(checksum),
                                    'samples_ms': [], 'repetitions': []})
        if row['checksum'] != int(checksum):
            raise RuntimeError(f'inconsistent checksum for {key}')
        row['samples_ms'].append(float(ms))
        row['repetitions'].append(int(rep))
    rows = list(groups.values())
    if len(rows) != 48:
        raise RuntimeError(f'expected 48 workload/method rows, got {len(rows)}')
    for row in rows:
        if sorted(row.pop('repetitions')) != list(range(5)):
            raise RuntimeError(f'missing repetitions for {row}')
        row['median_ms'] = statistics.median(row['samples_ms'])
    baseline = {(row['capacity'], row['distribution'], row['trace']): row for row in rows
                if row['method'] == 'direct-quantities'}
    for row in rows:
        ref = baseline[row['capacity'], row['distribution'], row['trace']]
        if row['checksum'] != ref['checksum']:
            raise RuntimeError(f'method checksum differs from baseline: {row}')
        row['median_fraction_of_direct'] = row['median_ms'] / ref['median_ms']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    report = {
        'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'cpu': cpu, 'platform': platform.platform(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'flags': flags, 'python': platform.python_version(), 'seed': 20260928,
        'warmups': 1, 'repetitions': 5, 'method_order': 'rotated per repetition',
        'workload': 'capacities 16/512/4096, 12/48/48 item types; weights uniform 1..23 (1..3 for small-weight unbounded), signed values uniform -50..150; quantities one, uniform 1..32, or unlimited; complete exact table and one full-capacity at-most result, with/without trace',
        'timed_scope': 'construct DP including setup/allocation, materialize every exact answer, find at-most optimum, optionally restore its multiplicity vector, destroy internal DP; output storage retained for verification outside timing. Inputs and independent references are generated outside timing.',
        'memory': 'Library no-trace DP uses O(capacity + items) storage, optional trace O(items * capacity) ints; direct-quantity reference keeps two value/reachability rows and optional per-item take rows. Both retain O(capacity) output tables and O(items) optional witness. Small-case batching retains 64 output tables outside algorithm workspace.',
        'verification': 'Every exact reachable flag and reachable score compared with direct multiplicity recurrence for every warmup/repetition; at-most optimality and complete witness weight/value/count bounds checked. Checks survive NDEBUG.',
        'comparison_limits': 'Single-threaded warmed-cache shared-host observations. Direct multiplicity recurrence is an independent simple baseline, not a fastest known bounded-knapsack claim. Does not compare MI18 monotone queues, bitsets, value-specialized or modern fine-grained algorithms. Tiny-case overhead and regressions are retained; no timing gate or implicit dispatch cutoff.',
        'sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in (ROOT / '06-Miscellaneous/13-knapsack.hpp',
                             HERE / '13-knapsack_benchmark.cpp', HERE / '13-knapsack_benchmark.py')},
        'results': rows,
    }
    output = HERE / '13-knapsack_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} workload/method comparisons; complete outputs/witnesses verified; record={output}')
    for row in rows:
        if row['method'] == 'library':
            print(row['capacity'], row['distribution'], f"trace={row['trace']}",
                  f"{row['median_ms']:.6f} ms", f"{row['median_fraction_of_direct']:.4f} x direct")


if __name__ == '__main__':
    main()
