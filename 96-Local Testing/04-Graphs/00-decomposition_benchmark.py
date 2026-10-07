#!/usr/bin/env python3
"""Compare explicit LCA and SCC implementations; no portable timing gate."""
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
    parser.add_argument('--output', type=Path, default=HERE / '00-decomposition_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG', '-mno-avx2']
    with tempfile.TemporaryDirectory(prefix='cp-decomposition-bench-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '00-decomposition_benchmark.cpp'),
                        '-o', str(binary)], check=True, timeout=180)
        output = subprocess.check_output([str(binary), str(args.seed)], text=True, timeout=180)
    cases = {}
    names = ['a_setup_ms', 'b_setup_ms', 'a_query_ms', 'b_query_ms']
    for line in output.splitlines():
        family, n, m, q, shape, run, *values = line.split()
        item = cases.setdefault((family, n, shape), dict(family=family, n=int(n), m=int(m),
                    queries=int(q), shape=shape, checksum=int(values[4]), samples=[]))
        item['samples'].append(dict(zip(names, map(float, values[:4]))))
    for item in cases.values():
        item['medians'] = {name: statistics.median(s[name] for s in item['samples']) for name in names}
    cpu = next((s.split(':', 1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                if s.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  flags=flags, warmup='one construction/query run of both alternatives', samples=5,
                  alternatives=dict(lca=['LCA binary lifting', 'EulerLCA +/-1 RMQ'],
                                    scc=['tarjanScc', 'kosarajuScc']),
                  setup='Graph and random query generation excluded; algorithm allocation included. All outputs compared outside timing.',
                  workloads='LCA: chains, stars, random recursive trees, disconnected chains of <=32 vertices; uniform endpoint queries. SCC: chains, cycles, chain plus 4n random arcs, SCC blocks of <=32 vertices.',
                  memory='LCA binary O(n log(n)), Euler O(n); SCC both O(n+m), with reverse adjacency only in Kosaraju. Input graph and result verification storage excluded from these algorithm bounds.',
                  interpretation='Shared-machine observations; explicit alternatives without automatic thresholds.',
                  cases=list(cases.values()))
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for item in cases.values():
        print(item['family'], item['n'], item['shape'], item['medians'])
    print('PASS: all results checked; record:', args.output)


if __name__ == '__main__':
    main()
