#!/usr/bin/env python3
"""Reproducible observations of alternative sieve workloads; no timing gates."""
import datetime
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent

def main():
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    with tempfile.TemporaryDirectory(prefix='cp-math-benchmark-') as directory:
        binary = Path(directory) / 'sieves'
        subprocess.run([compiler, *flags, str(HERE / '90-foundations_benchmark.cpp'), '-o', str(binary)], check=True)
        run = subprocess.run([str(binary)], capture_output=True, text=True, timeout=180, check=True)
    groups = {}
    for line in run.stdout.splitlines():
        family, method, l, n, block, rep, elapsed, count, checksum = line.split()
        key = (family, method, int(l), int(n), int(block))
        if key not in groups: groups[key] = {'family': family, 'method': method, 'l': int(l), 'n': int(n), 'block': int(block), 'samples_ms': [],
                                           'count': int(count), 'checksum': int(checksum)}
        group = groups[key]
        if (int(count), int(checksum)) != (group['count'], group['checksum']): raise RuntimeError('unstable result')
        group['samples_ms'].append(float(elapsed))
    results = list(groups.values())
    for row in results:
        row['median_ms'] = statistics.median(row['samples_ms'])
    report = {'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'cpu': cpu,
              'platform': platform.platform(), 'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              'flags': flags, 'python': platform.python_version(), 'seed': None,
              'distribution': 'dense inclusive [0,n]; streaming [l,l+n), l=0 or 10^12, n=32/50000/2000000; blocks=1024/32768/262144; deterministic',
              'warmups': 1, 'repetitions': 5,
              'timed_scope': 'dense: constructor including allocation, table and prime list; setup: base-prime constructor; stream: reusable base excluded, block allocation and ordered checksum callback included; all destruction and vector equality checks excluded',
              'memory': 'packed baseline: ceil((n+1)/8) bytes + prime vector; byte Eratosthenes: n+1 bytes + prime vector; linear SPF: 4*(n+1) bytes + prime vector; optional tables not constructed; segmented stored bases: 8*pi(floor(sqrt(max_value))) bytes, construction seed+block workspace; streaming plain/wheel block bytes, odd ceil(block/2) bytes, no output list in timed calls; allocator capacity may exceed logical sizes',
              'comparison_limits': 'linear sieve also constructs SPF; segmented variants are compared against plain output plus tested independent per-header oracles; setup measured separately; no universal winners or timing gates',
              'results': results}
    output = HERE / '90-foundations_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS independently compared prime lists; benchmark record:', output)
    for row in results: print(row['family'], row['method'], row['l'], row['n'], row['block'], f"{row['median_ms']:.6f} ms")

if __name__ == '__main__':
    main()
