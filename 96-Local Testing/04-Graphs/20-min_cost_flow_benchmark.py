#!/usr/bin/env python3
"""Time MinCostFlow against an ACL-style reference, the reference without its zero reduced-cost stack and an O(n^2)
dense-scan reference; convex edges against parallel arcs; Kahn-order potentials on a reverse-labelled negative DAG."""
import argparse
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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261013)
    parser.add_argument('--output', type=Path, default=HERE / '20-min_cost_flow_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-min-cost-flow-benchmark-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '20-min_cost_flow_benchmark.cpp'), '-o', str(binary)], check=True, timeout=180)
        output = subprocess.check_output([str(binary), str(args.seed)], text=True, timeout=1800)
    columns = {'engines': ['library_ms', 'acl_style_ms', 'no_zero_stack_ms', 'dense_scan_ms'],
               'convex': ['convex_ms', 'parallel_arcs_ms'], 'negative': ['in_order_ms', 'reverse_labelled_ms']}
    cases = {}
    for line in output.splitlines():
        kind, shape, n, m, run, *values = line.split()
        names = columns[kind]
        item = cases.setdefault(kind + ' ' + shape, dict(kind=kind, shape=shape, n=int(n), m=int(m), result=values[len(names):], samples=[]))
        item['samples'].append(dict(zip(names, map(float, values[:len(names)]))))
    for item in cases.values():
        names = columns[item['kind']]
        item['medians'] = {name: statistics.median(s[name] for s in item['samples']) for name in names}
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  flags=flags, warmup='one checked run of every implementation', samples=5,
                  workload='small n=50 m=300; dense n=300 p=1/2; bipartite k=300 (k^2 unit arcs); 100x100 grid both directions; '
                           'sparse n=5000 m=50000; caps [1,100] or [1,10], costs [0,1000] or [0,1e6]; convex: n=200, 1000 edges of 50 unit pieces '
                           'costing 1,3,5,... (x^2); negative DAG: chain n=200000 cap 10 plus n unit intervals of cost [-1e6,-1]',
                  setup='Network construction included in every timing; input generation excluded. A dense_scan_ms of -1 means not run (O(n^2) per phase).',
                  memory='O(n + m) arcs; the heap holds at most m entries.',
                  interpretation='Shared-machine medians; every implementation returns the same (flow, cost).',
                  cases=list(cases.values()))
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for item in cases.values():
        print(item['kind'], item['shape'], item['n'], item['m'], item['medians'])
    print('PASS: every result checked; record:', args.output)


if __name__ == '__main__':
    main()
