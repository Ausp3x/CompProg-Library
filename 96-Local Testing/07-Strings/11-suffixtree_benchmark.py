"""Suffix-tree lca and weighted-ancestor benchmark.

Workloads: 1,000,000-symbol random texts over 2 and 26 letters, the Fibonacci
prefix and a unary text. Per workload: tree build from a SuffixArray copy moved
in (SA construction untimed); the O(n) lca (split map, 32-boundary windows,
sparse table over block minima) against an O(n * log(n)) sparse table over the
same split map (its build timed separately) and against the composition of the
LCE through the SuffixArray RMQ with the jump-pointer weightedAncestor, on
1,000,000 random node pairs and 1,000,000 leaf pairs of rank gap below 64; the
offline weightedAncestors batch against a loop of online weightedAncestor on
1,000,000 random (node, length) queries; memory of both lca structures. One
warmup and --reps repetitions per method, checksums must agree. Medians are
printed and written to 11-suffixtree_benchmark.json beside this script
(git-ignored). Timings are observations, never correctness gates.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261019)
    parser.add_argument('--reps', type=int, default=5)
    args = parser.parse_args()
    if args.reps < 1 or args.reps % 2 == 0:
        parser.error('repetitions must be positive and odd')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-suffixtree-benchmark-') as directory:
        binary = str(Path(directory) / 'benchmark')
        command = [compiler, *flags, str(HERE / '11-suffixtree_benchmark.cpp'), '-o', binary]
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise SystemExit(f'FAIL build command={shlex.join(command)}\n{result.stderr}')
        result = subprocess.run([binary, str(args.seed), str(args.reps)], capture_output=True, text=True, timeout=1800)
        if result.returncode:
            raise SystemExit(f'FAIL suffixtree benchmark seed={args.seed}\n{result.stderr}')
    records = json.loads(result.stdout)
    for r in records:
        print(f"PASS {r['workload']:12} {r['method']:42} {r['value']:.2f}", flush=True)
    out = {'seed': args.seed, 'reps': args.reps, 'flags': flags,
           'compiler': subprocess.run([compiler, '--version'], capture_output=True, text=True).stdout.splitlines()[0],
           'records': records}
    (HERE / '11-suffixtree_benchmark.json').write_text(json.dumps(out, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
