#!/usr/bin/env python3
"""Time the sparse marker loop against the bitset kernel of countTriangles to place DENSE_RATIO and DENSE_WORDS."""
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
DENSE_WORDS = 1 << 22
VARIANTS = {'O2': ['-std=gnu++20', '-O2', '-DNDEBUG'], 'popcnt': ['-std=gnu++20', '-O2', '-DNDEBUG', '-mpopcnt'],
            'native': ['-std=gnu++20', '-O2', '-DNDEBUG', '-march=native']}
FIELDS = ['build_ms', 'build_full_ms', 'dense_start_ms', 'sparse_ms', 'bitset_ms', 'count_ms']
RATIOS = [0.5, 1, 1.5, 2, 3, 4, 6, 8, 12, 16, 24, 32]


def run(compiler, flags, seed, quick, folder):
    binary = Path(folder) / 'benchmark'
    subprocess.run([compiler, *flags, str(HERE / '35-subgraph_enumeration_benchmark.cpp'), '-o', str(binary)], check=True, timeout=300)
    output = subprocess.run([str(binary), str(seed), *(['quick'] if quick else [])], check=True, capture_output=True, text=True, timeout=7200).stdout
    cases = {}
    for line in output.splitlines():
        name, n, m, probes, scan, r0, rep, k, *values, triangles = line.split()
        item = cases.setdefault((name, n), dict(name=name, n=int(n), m=int(m), probes=int(probes), scan=int(scan),
                                                 r0=int(r0), inner_repeats=int(k), triangles=int(triangles), samples=[]))
        item['samples'].append(dict(zip(FIELDS, map(float, values))))
    for item in cases.values():
        med = {f: statistics.median(s[f] for s in item['samples']) for f in FIELDS}
        words = (item['n'] + 63) // 64
        item.update(medians=med, ratio=item['probes'] / item['scan'], eligible=item['n'] * words <= DENSE_WORDS,
                    sparse_ns_per_probe=med['sparse_ms'] * 1e6 / max(item['probes'], 1))
        if med['bitset_ms'] >= 0:
            item['bitset_ns_per_word'] = med['bitset_ms'] * 1e6 / item['scan']
            item['breakeven_ratio'] = item['bitset_ns_per_word'] / item['sparse_ns_per_probe'] if item['probes'] else None
            item['winner'] = 'bitset' if med['bitset_ms'] < med['sparse_ms'] else 'sparse'
        else:
            item['winner'] = 'sparse (bitset not run)'
        item['end_to_end_speedup'] = (med['build_ms'] + med['sparse_ms']) / med['count_ms']
        item['build_full_over_build'] = med['build_full_ms'] / med['build_ms']
    return list(cases.values())


def analyse(cases):
    timed = [c for c in cases if c['eligible'] and c['medians']['bitset_ms'] >= 0]
    sparse_wins = [c['ratio'] for c in timed if c['winner'] == 'sparse']
    bitset_wins = [c['ratio'] for c in timed if c['winner'] == 'bitset']
    best = sum(min(c['medians']['sparse_ms'], c['medians']['bitset_ms']) for c in timed)
    table = []
    for r in RATIOS:
        chosen = [c['medians']['bitset_ms' if r * c['scan'] < c['probes'] else 'sparse_ms'] for c in timed]
        worst = max(t / min(c['medians']['sparse_ms'], c['medians']['bitset_ms']) for t, c in zip(chosen, timed))
        table.append(dict(dense_ratio=r, total_ms=sum(chosen), oracle_ms=best, worst_case_slowdown=worst))
    return dict(largest_ratio_sparse_wins=max(sparse_wins, default=None), smallest_ratio_bitset_wins=min(bitset_wins, default=None),
                breakeven_ratios=sorted(c['breakeven_ratio'] for c in timed if c.get('breakeven_ratio')), candidates=table)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261029)
    parser.add_argument('--quick', action='store_true', help='only n <= 1000 G(n, p) and n = m = 1e5')
    parser.add_argument('--variants', nargs='+', choices=sorted(VARIANTS), default=list(VARIANTS))
    parser.add_argument('--output', type=Path, default=HERE / '35-subgraph_enumeration_benchmark.json')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    cpu = next((line.split(':', 1)[1].strip() for line in subprocess.check_output(['lscpu'], text=True).splitlines()
                if line.startswith('Model name')), platform.processor())
    record = dict(seed=args.seed, quick=args.quick, cpu=cpu, platform=platform.platform(),
                  compiler=subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
                  warmup='one checked unprinted run of every section per graph', samples=5,
                  inner_repeats='each section repeated k = clamp(4e6 / (n + m), 1, 200) times per sample',
                  workload='G(n, p) for n in {500..16000} x p in {0.005..1} with m <= 1.7e7, G(32000, {0.01, 0.02}) beyond DENSE_WORDS; '
                           '2000-clique plus a 10000- or 100000-vertex path; 3000-clique plus 13384, 13385 or 100000 isolated vertices or a 100000-vertex path; K(a, a) for a in {1000, 2000, 4000}; random n = m in {1e5, 5e5}',
                  setup='Graph construction excluded; build_ms is Ranked(g), build_full_ms Ranked(g, true), dense_start_ms denseStart (r0), '
                        'sparse_ms the whole-graph marker loop and bitset_ms the whole-graph bitsetTriangles(h) (r0 = 0) on Ranked(g), '
                        'count_ms countTriangles end to end (suffix dispatch). probes, scan, ratio, crossover and the DENSE_RATIO table '
                        'use whole-graph probes and scan with r0 = 0, not the suffix rule.',
                  check='sparse == bitset == countTriangles, and the closed form for complete graphs, cliques and K(a, a).',
                  memory='Graph about 72 B per edge, Ranked build peak about 70 B per edge, bitset rows 8 * n * ceil(n / 64) bytes '
                         '(bitset skipped above 1.6 GB of rows).',
                  variants={})
    with tempfile.TemporaryDirectory(prefix='cp-subgraph-benchmark-') as folder:
        for label in args.variants:
            cases = run(compiler, VARIANTS[label], args.seed, args.quick, folder)
            record['variants'][label] = dict(flags=VARIANTS[label], cases=cases, analysis=analyse(cases))
            print(f'== {label}: {" ".join(VARIANTS[label])}')
            print(f'{"case":24} {"n":>6} {"m":>9} {"probes":>12} {"scan":>11} {"ratio":>8} {"r0":>6} {"build":>8} {"full":>8} {"sparse":>9} {"bitset":>9} '
                  f'{"count":>9} {"even":>6} {"e2e":>6} winner')
            for c in cases:
                med = c['medians']
                print(f'{c["name"]:24} {c["n"]:6} {c["m"]:9} {c["probes"]:12} {c["scan"]:11} {c["ratio"]:8.3f} {c["r0"]:6} {med["build_ms"]:8.2f} {med["build_full_ms"]:8.2f} '
                      f'{med["sparse_ms"]:9.3f} {med["bitset_ms"]:9.3f} {med["count_ms"]:9.3f} '
                      f'{c.get("breakeven_ratio") or 0:6.2f} {c["end_to_end_speedup"]:6.2f} {c["winner"]}')
            a = record['variants'][label]['analysis']
            print('largest ratio where sparse wins:', a['largest_ratio_sparse_wins'], '| smallest ratio where bitset wins:', a['smallest_ratio_bitset_wins'])
            print('break-even ratios:', ' '.join(f'{x:.2f}' for x in a['breakeven_ratios']))
            for row in a['candidates']:
                print(f'DENSE_RATIO={row["dense_ratio"]:<5} total={row["total_ms"]:.1f} ms oracle={row["oracle_ms"]:.1f} ms worst slowdown={row["worst_case_slowdown"]:.2f}x')
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    print('PASS: every count checked; record:', args.output)


if __name__ == '__main__':
    main()
