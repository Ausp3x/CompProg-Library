#!/usr/bin/env python3
"""negativeSsspScaling versus spfa and bellmanFord from source 0; --no-large skips the n=10^5, W=2^61 case (~26 s per scaling run); no timing gate."""
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
ENGINES = ['negativeSsspScaling', 'spfa', 'bellmanFord']
# (kind, n, W, large): W bounds |h| for potential and the chain-arc magnitude for antififo; chain ignores it.
CASES = [('potential', 10**4, 2**61, False), ('potential', 10**5, 50, False), ('potential', 10**5, 2**61, True),
         ('chain', 3 * 10**4, 0, False), ('antififo', 10**4, 2**30, False), ('antififo', 3 * 10**4, 2**30, False)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261103)
    parser.add_argument('--samples', type=int, default=5)
    parser.add_argument('--large-samples', type=int, default=3)
    parser.add_argument('--large', action=argparse.BooleanOptionalAction, default=True)
    parser.add_argument('--output', type=Path, default=HERE / '13-shortest_path_advanced_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG', '-Wall', '-Wextra', '-Wconversion', '-Werror']
    cases = []
    with tempfile.TemporaryDirectory(prefix='cp-sp-advanced-bench-') as folder:
        binary = Path(folder) / 'benchmark'
        subprocess.run([compiler, *flags, str(HERE / '13-shortest_path_advanced_benchmark.cpp'), '-o', str(binary)],
                       check=True, timeout=180)
        for index, (kind, n, w, large) in enumerate(CASES):
            if large and not args.large:
                continue
            seed, samples = args.seed + index, args.large_samples if large else args.samples
            output = subprocess.check_output([str(binary), kind, str(n), str(w), str(seed), str(samples)],
                                             text=True, timeout=1800)
            item = dict(kind=kind, n=n, w=w, seed=seed, samples=samples, warmup={}, runs={e: [] for e in ENGINES})
            for line in output.splitlines():
                tag, *rest = line.split()
                if tag == 'graph':
                    item['m'] = int(rest[1])
                elif tag == 'run':
                    if int(rest[1]) == -1:
                        item['warmup'][rest[0]] = float(rest[2])
                    else:
                        item['runs'][rest[0]].append(float(rest[2]))
                elif tag == 'checksum':
                    item['checksum'], item['reached'] = int(rest[0]), int(rest[1])
                elif tag == 'memory':
                    item['peak_rss_kb'] = int(rest[0])
            item['medians_s'] = {e: statistics.median(item['runs'][e]) for e in ENGINES}
            cases.append(item)
            print(kind, n, w, {e: round(s, 4) for e, s in item['medians_s'].items()}, f"peak={item['peak_rss_kb']}kB", flush=True)
    cpu = next((s.split(':', 1)[1].strip() for s in Path('/proc/cpuinfo').read_text().splitlines()
                if s.startswith('model name')), platform.processor())
    record = dict(seed=args.seed, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0], flags=flags,
                  warmup='one run of each engine per input; scaling warmup certified (feasible arcs, tight parents), every run equal to it on reachable and dist',
                  repetitions=f'{args.samples} per input, {args.large_samples} for n=10^5 W=2^61; engines interleaved per repetition; median',
                  workloads='potential: h[v] uniform [-W, W], 3n arcs u->v uniform with w = h[u] - h[v] + uniform[0, 2]; '
                            'chain: -1 chain 0->...->n-1 in reverse arc order, then 2n forward arcs u<v with w = -(v-u) + uniform[0, 2]; '
                            'antififo: star 0->v weight 0 added for v = n-1 down to 1, chain v-1->v weight -uniform[1, W] in reverse arc order, '
                            'n filler arcs among 1..n-1 with weight n*W + uniform[0, W) that never relax; uniform via mt19937_64 modulo; source 0.',
                  memory='peak_rss_kb is getrusage(RUSAGE_SELF).ru_maxrss of the per-input process (generation plus all engines).',
                  cases=cases)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    print('PASS: every run agreed with the certified scaling result; record:', args.output)


if __name__ == '__main__':
    main()
