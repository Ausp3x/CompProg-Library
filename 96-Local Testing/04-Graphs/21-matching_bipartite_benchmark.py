#!/usr/bin/env python3
"""Compare Hopcroft-Karp, Kuhn and bitset dense matching; no portable timing gate."""
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
    parser.add_argument('--output', type=Path, default=HERE / '21-matching_bipartite_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-matching-bench-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '21-matching_bipartite_benchmark.cpp'),
                        '-o', str(binary)], check=True, timeout=180)
        output = subprocess.check_output([str(binary), str(args.seed)], text=True, timeout=900)
    cases = {}
    names = ['hopcroft_karp_ms', 'kuhn_ms', 'dense_ms']
    for line in output.splitlines():
        shape, n, m, run, *values = line.split()
        item = cases.setdefault((shape, n), dict(shape=shape, n=int(n), m=int(m), size=int(values[3]), samples=[]))
        item['samples'].append(dict(zip(names, map(float, values[:3]))))
    for item in cases.values():
        item['medians'] = {name: statistics.median(s[name] for s in item['samples']) for name in names}
    cpu = next((s.split(':', 1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                if s.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  flags=flags, warmup='first of six runs discarded', samples=5,
                  alternatives=['BipartiteMatching::hopcroftKarp', 'BipartiteMatching::kuhn', 'bipartiteMatchingDense'],
                  setup='Edge generation excluded; construction, adjacency/bitset allocation and matching included. Sizes compared outside timing; dense_ms -1 means not run (bitset memory n^2/64 words).',
                  workloads='nl = nr = n; uniform random endpoints with multiedges; reversed path forces long augmenting paths.',
                  interpretation='Shared-machine observations; explicit alternatives without automatic thresholds.',
                  cases=list(cases.values()))
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for item in cases.values():
        print(item['shape'], item['n'], item['m'], item['medians'])
    print('PASS: all results checked; record:', args.output)


if __name__ == '__main__':
    main()
