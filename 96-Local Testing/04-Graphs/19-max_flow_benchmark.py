#!/usr/bin/env python3
"""Compare Dinic, capacity-scaling Dinic, highest-label push-relabel and an ACL-style recursive Dinic."""
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
    parser.add_argument('--seed', type=int, default=20260927)
    parser.add_argument('--output', type=Path, default=HERE / '19-max_flow_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-max-flow-benchmark-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '19-max_flow_benchmark.cpp'), '-o', str(binary)], check=True, timeout=180)
        output = subprocess.check_output([str(binary), str(args.seed)], text=True, timeout=1800)
    names = ['dinic_ms', 'scaling_ms', 'push_relabel_ms', 'acl_style_ms']
    cases = {}
    for line in output.splitlines():
        shape, n, m, run, repeats, *values = line.split()
        item = cases.setdefault(shape, dict(shape=shape, n=int(n), m=int(m), repetitions_per_sample=int(repeats),
                                            max_flow=int(values[4]), samples=[]))
        item['samples'].append(dict(zip(names, map(float, values[:4]))))
    for item in cases.values():
        item['medians'] = {name: statistics.median(s[name] for s in item['samples']) for name in names}
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  flags=flags, warmup='one checked run of each engine', samples=5,
                  workload='small n=50 m=300 caps [1,100] x2000; sparse n=20000 m=100000 caps [1,1e9]; unit bipartite k=50000 m=300000; '
                           'dense n=400 p=1/2 caps [1,1e6]; undirected 300x300 grid caps [1,1e6]; chain n=20000 plus random side arcs every third vertex (adversarial: about n Dinic phases)',
                  setup='Network construction (addEdge) included in every timing; input generation excluded.',
                  memory='O(n + m) arcs for every engine; push-relabel adds O(n) labels, lists and buckets.',
                  interpretation='Shared-machine medians; every engine result equals the certified Dinic value.',
                  cases=list(cases.values()))
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for item in cases.values():
        print(item['shape'], item['n'], item['m'], item['medians'])
    print('PASS: every result checked; record:', args.output)


if __name__ == '__main__':
    main()
