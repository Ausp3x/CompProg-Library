#!/usr/bin/env python3
"""Compare adjacency-list and CSR traversal; record setup separately, without timing gates."""
import argparse
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20260927)
    parser.add_argument('--output', type=Path, default=HERE / '00-foundations_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG', '-mno-avx2']
    with tempfile.TemporaryDirectory(prefix='cp-graph-benchmark-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '00-foundations_benchmark.cpp'),
                        '-o', str(binary)], check=True, timeout=120)
        result = subprocess.run([str(binary), str(args.seed)], check=True,
                                text=True, capture_output=True, timeout=180)
    cases = {}
    for line in result.stdout.splitlines():
        n, m, shape, run, repeats, adj, csr, setup, checksum = line.split()
        key = (int(n), shape)
        item = cases.setdefault(key, dict(n=int(n), m=int(m), shape=shape,
                    repetitions_per_sample=int(repeats), checksum=int(checksum), samples=[]))
        item['samples'].append(dict(adjacency_ms=float(adj), csr_ms=float(csr), setup_ms=float(setup)))
    for item in cases.values():
        item['medians'] = {name: statistics.median(s[name] for s in item['samples'])
                           for name in ('adjacency_ms', 'csr_ms', 'setup_ms')}
    cpuinfo = Path('/proc/cpuinfo').read_text()
    cpu = next((line.split(':', 1)[1].strip() for line in cpuinfo.splitlines()
                if line.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  flags=flags, warmup='one checked traversal per representation', samples=5,
                  setup='CSR copying/conversion timed separately; graph input construction excluded',
                  memory='Both O(n+m); CSR uses n+1 offsets and 2m adjacency ints for undirected graphs; owning edge/arc arrays included. Peak conversion also retains input Graph.',
                  interpretation='Shared-machine observations; no automatic dispatch or portable timing guarantee.',
                  cases=list(cases.values()))
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for item in cases.values():
        print(item['n'], item['shape'], item['medians'])
    print('PASS: all representation checksums agree; record:', args.output)


if __name__ == '__main__':
    main()
