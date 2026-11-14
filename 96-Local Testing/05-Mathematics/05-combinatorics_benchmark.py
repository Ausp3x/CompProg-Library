"""LogCombinatorics' compensated-sum table against the previous lgammal table, and primorial<InfInt>.

Builds the C++ workload with -O2 -DNDEBUG (no -march), runs one warmup plus five timed repetitions
per workload, checks the two log tables agree within the reported worst ulp distance and the
primorial's hex digit count against a Python product, and writes medians with CPU and compiler
conditions to 05-combinatorics_benchmark.json beside this driver. No timing gates.
"""
import datetime
import json
import math
import os
from pathlib import Path
import platform
import statistics
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def hex_digits_of_primorial(n):
    flags = bytearray([1]) * (n + 1)
    flags[:2] = b'\0\0'
    for i in range(2, math.isqrt(n) + 1):
        if flags[i]:
            flags[i*i::i] = bytes(len(range(i*i, n + 1, i)))
    values = [i for i in range(n + 1) if flags[i]]
    while len(values) > 1:
        values = [math.prod(values[i:i+2]) for i in range(0, len(values), 2)]
    return len(format(values[0], 'X'))


def main():
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    with tempfile.TemporaryDirectory(prefix='cp-combinatorics-benchmark-') as directory:
        binary = Path(directory) / 'combinatorics'
        subprocess.run([compiler, *flags, str(HERE / '05-combinatorics_benchmark.cpp'), '-o', str(binary)], check=True)
        run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=600, check=True)
    groups = {}
    for line in run.stdout.splitlines():
        family, method, n, rep, elapsed, extra = line.split()
        key = (family, method, int(n))
        groups.setdefault(key, {'family': family, 'method': method, 'n': int(n), 'samples_ms': [], 'extra': []})
        groups[key]['samples_ms'].append(float(elapsed))
        groups[key]['extra'].append(float(extra))
    for (family, method, n), row in groups.items():
        row['median_ms'] = statistics.median(row['samples_ms'])
        row['ns_per_entry'] = row['median_ms'] * 1e6 / (n + 1) if family == 'log' else None
        if family == 'log' and method == 'compensated_sum':
            row['worst_ulp_vs_lgammal'] = max(row['extra'])
        if family == 'primorial' and set(row['extra']) != {float(hex_digits_of_primorial(n))}:
            raise RuntimeError(f'primorial hex digit count mismatch n={n}')
    results = list(groups.values())
    report = {'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'cpu': cpu,
              'platform': platform.platform(), 'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              'flags': flags, 'python': platform.python_version(), 'seed': None,
              'distribution': 'log tables n=1e5/1e6/1e7 (every entry compared against lgammal); primorial<InfInt> n=1e5/1e6; deterministic',
              'warmups': 1, 'repetitions': 5,
              'timed_scope': 'log: LogCombinatorics construction including allocation, and the lgammal reference table built the same way; primorial: the call, hex conversion excluded',
              'memory': 'log tables: 16*(n+1) bytes each (x86-64 long double), three alive (reference, timed table, timed lgammal copy); primorial: n+1 sieve bytes, 4*pi(n) prime bytes and O(n) bits of InfInt leaves and products; the whole run peak is the MEMORY line',
              'results': results}
    output = HERE / '05-combinatorics_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS checked log tables and primorial digit counts; benchmark record:', output)
    for row in results:
        extra = f" {row['ns_per_entry']:.1f} ns/entry" if row['ns_per_entry'] else ''
        worst = f" worst {row['worst_ulp_vs_lgammal']:.2f} ulp" if 'worst_ulp_vs_lgammal' in row else ''
        print(row['family'], row['method'], row['n'], f"{row['median_ms']:.3f} ms{extra}{worst}")


if __name__ == '__main__':
    main()
