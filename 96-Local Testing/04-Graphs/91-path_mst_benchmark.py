#!/usr/bin/env python3
"""Compare dense/sparse shortest paths and Prim, including separate dense-view setup."""
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
    parser.add_argument('--output', type=Path, default=HERE / '91-path_mst_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG', '-mno-avx2']
    with tempfile.TemporaryDirectory(prefix='cp-path-mst-benchmark-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '91-path_mst_benchmark.cpp'),
                        '-o', str(binary)], check=True, timeout=180)
        output = subprocess.check_output([str(binary), str(args.seed)], text=True, timeout=180)
    names = ['dijkstra_sparse_ms', 'dijkstra_dense_ms', 'prim_sparse_ms', 'prim_dense_ms', 'dense_setup_ms']
    cases = {}
    for line in output.splitlines():
        n, m, shape, run, repeats, *values = line.split()
        item = cases.setdefault((int(n), shape), dict(n=int(n), m=int(m), shape=shape,
                    repetitions_per_sample=int(repeats), path_checksum=int(values[5]),
                    forest_checksum=int(values[6]), samples=[]))
        item['samples'].append(dict(zip(names, map(float, values[:5]))))
    for item in cases.values():
        item['medians'] = {name: statistics.median(s[name] for s in item['samples']) for name in names}
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  flags=flags, warmup='one checked run of each algorithm', samples=5,
                  workload='Undirected lng weights uniform [0,10^6]; chains, sparse random multigraphs, complete graphs and disconnected blocks of at most eight vertices.',
                  setup='Input graph construction excluded; DenseGraph owning conversion timed separately. Algorithm allocation, global domain checks and result comparison included.',
                  memory='Input Graph O(n+m); additional owning DenseGraph O(n^2+m); Dijkstra sparse O(n+m) and dense O(n) auxiliary, both Prim variants O(n).',
                  interpretation='Shared-machine medians; explicit variants, no automatic threshold or portable timing gate.',
                  cases=list(cases.values()))
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for item in cases.values():
        print(item['n'], item['shape'], item['medians'])
    print('PASS: every distance/result checked; record:', args.output)


if __name__ == '__main__':
    main()
