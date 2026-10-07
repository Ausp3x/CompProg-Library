#!/usr/bin/env python3
"""Copy/allocate/sort pipelines against standard algorithms; no timing gates."""
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
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    with tempfile.TemporaryDirectory(prefix='cp-sorting-benchmark-') as directory:
        binary = Path(directory) / 'sorting'
        subprocess.run([compiler, *flags, str(HERE / '11-sorting_selection_benchmark.cpp'), '-o', str(binary)],
                       check=True, timeout=180)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise RuntimeError(f'command={binary} returncode={result.returncode}\n{result.stderr}')
    groups, selection = {}, {}
    for line in result.stdout.splitlines():
        if line.startswith('select '):
            _, n, distribution, method, rep, batch, ms, value = line.split()
            row = selection.setdefault((int(n), distribution, method), {'n': int(n), 'distribution': distribution, 'method': method, 'rank': int(n) // 2,
                                                                        'batch': int(batch), 'value': int(value), 'samples_ms': []})
            row['samples_ms'].append(float(ms))
            continue
        n, data_type, distribution, method, rep, batch, ms, checksum = line.split()
        key = (int(n), data_type, distribution, method)
        row = groups.setdefault(key, {'n': int(n), 'type': data_type, 'distribution': distribution,
                                    'method': method, 'batch': int(batch), 'checksum': int(checksum),
                                    'samples_ms': [], 'repetitions': []})
        if row['checksum'] != int(checksum):
            raise RuntimeError(f'inconsistent checksum for {key}')
        row['samples_ms'].append(float(ms))
        row['repetitions'].append(int(rep))
    rows = list(groups.values())
    selection_rows = list(selection.values())
    if len(selection_rows) != 30 or any(len(row['samples_ms']) != 5 for row in selection_rows):
        raise RuntimeError(f'expected 30 selection rows with 5 samples, got {len(selection_rows)}')
    for row in selection_rows:
        row['median_ms'] = statistics.median(row['samples_ms'])
    for row in selection_rows:
        ref = selection[row['n'], row['distribution'], 'std-nth-element']
        row['median_fraction_of_standard'] = row['median_ms'] / ref['median_ms']
    if len(rows) != 132:
        raise RuntimeError(f'expected 132 workload/method rows, got {len(rows)}')
    for row in rows:
        if sorted(row.pop('repetitions')) != list(range(5)):
            raise RuntimeError(f'missing repetitions for {row}')
        row['median_ms'] = statistics.median(row['samples_ms'])
    baseline = {(row['n'], row['type'], row['distribution']): row for row in rows
                if row['method'].startswith('std-')}
    for row in rows:
        ref = baseline[row['n'], row['type'], row['distribution']]
        if row['checksum'] != ref['checksum']:
            raise RuntimeError(f'method checksum differs from baseline: {row}')
        row['median_fraction_of_standard'] = row['median_ms'] / ref['median_ms']
    report = {
        'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'cpu': cpu, 'platform': platform.platform(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'flags': flags, 'python': platform.python_version(), 'seed': 20260928,
        'warmups': 1, 'repetitions': 5, 'method_order': 'rotated per repetition',
        'batches': {'32': 256, '4096': 8, '200000': 1},
        'workload': 'signed64 values and 32-byte records with signed64 keys: random dense [0,255], sorted/reverse dense, random full-width bit patterns, sorted/reverse full-width, equal key 17; signed128 scalars: random/sorted/reverse full-width and equal',
        'timed_scope': 'allocate and copy fresh output vector from fixed input, sort including algorithm setup/workspace allocation and internal destruction; per-sort time averages a batch. Input generation, reference sorting, vector-of-vector metadata, complete equality/stability verification, checksum, and retained output destruction are outside timing.',
        'memory': 'Each timed pipeline includes n*sizeof(T) output allocation/copy. Counting adds 256 int counters; stable counting adds n int cached keys, n records of reorder storage and 256 int counters; radix adds n records of reorder storage and 256 stack int counters, except all-equal keys return after the scan without reorder allocation. std::sort has O(log(n)) stack; std::stable_sort may allocate O(n*sizeof(T)) workspace. Batch outputs coexist for verification; input/reference fixtures are outside algorithm workspace.',
        'verification': 'Every warmup and timed output is compared element-by-element with a standard sorted reference; records include original IDs and two payload words, so exact equality verifies stability and record preservation. A position-dependent checksum is also retained.',
        'comparison_limits': 'Shared-host warmed-cache observations, single-threaded GNU C++20; batch repetition reuses the input and may improve cache/predictor state. Includes common inputs where adaptive standard sort or stable_sort can win. No arbitrary-key comparator, cold-cache, move-only payload, or universal dispatch-threshold claim. Header correctness uses its separate independent suite.',
        'sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                   for p in (ROOT / '06-Miscellaneous/11-sorting_selection.hpp',
                             HERE / '11-sorting_selection_benchmark.cpp', HERE / '11-sorting_selection_benchmark.py')},
        'results': rows,
        'selection_workload': 'signed64 rank n/2 selection over random dense [0,255], random/sorted/reverse full-width and equal keys; timed scope copies the input then selects; every output is checked for the exact value and the partition property',
        'selection_results': selection_rows,
    }
    output = HERE / '11-sorting_selection_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    for row in selection_rows:
        if row['method'] != 'std-nth-element':
            print('select', row['n'], row['distribution'], f"{row['median_ms']:.6f} ms", f"{row['median_fraction_of_standard']:.3f} x nth_element")
    print(f'PASS {len(rows)} workload/method comparisons; all complete outputs verified; record={output}')
    for row in rows:
        if row['method'].startswith('std-'):
            continue
        print(row['n'], row['type'], row['distribution'], row['method'],
              f"{row['median_ms']:.6f} ms", f"{row['median_fraction_of_standard']:.3f} x standard")


if __name__ == '__main__':
    main()
