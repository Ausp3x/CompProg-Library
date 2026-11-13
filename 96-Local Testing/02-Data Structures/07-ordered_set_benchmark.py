#!/usr/bin/env python3
"""OrderedMultiSet split/join cost on GCC 16 and GCC 14 (PBDS split differs between them); timings are not correctness gates."""
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
    parser.add_argument('--seed', type=int, default=20261112)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--compilers', nargs='+', default=[os.environ.get('CXX', 'g++'), 'g++-14'])
    parser.add_argument('--output', type=Path, default=HERE / '07-ordered_set_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=600)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result.stdout

    summary, versions = [], {}
    with tempfile.TemporaryDirectory(prefix='cp-ordered-set-benchmark-') as name:
        for i, compiler in enumerate(args.compilers):
            binary = Path(name) / f'b{i}'
            run([compiler, *flags, str(HERE / '07-ordered_set_benchmark.cpp'), '-o', str(binary)])
            versions[compiler] = run([compiler, '--version']).splitlines()[0]
            rows = [json.loads(line) for _ in range(args.runs) for line in run([str(binary), str(args.seed)]).splitlines()]
            groups = {}
            for row in rows:
                groups.setdefault((row['order'], row['n'], row['fraction']), []).append(row)
            for (order, n, fraction), cases in sorted(groups.items()):
                summary.append(dict(compiler=versions[compiler], order=order, n=n, fraction=fraction, moved=cases[0]['moved'],
                                    split_median_us=round(statistics.median(r['split_seconds'] for r in cases) * 1e6, 1),
                                    join_median_us=round(statistics.median(r['join_seconds'] for r in cases) * 1e6, 2)))
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compilers=versions, flags=flags, platform=platform.platform(), cpu=cpu,
                  workload='OrderedMultiSet<int> of keys 0..n-1 inserted sorted or shuffled, one split at fraction * n, then join back; fresh tree per run',
                  results=summary)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    for r in summary:
        print(f"{r['compiler'][:22]:22} {r['order']:8} n={r['n']:>8} fraction={r['fraction']:<5} moved={r['moved']:>8} "
              f"split={r['split_median_us']:>10.1f}us join={r['join_median_us']:>6.2f}us")


if __name__ == '__main__':
    main()
