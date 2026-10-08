"""Map Trie versus TrieDense<26,'a'> insert/query/erase pipelines; checksums must agree.

Workloads: 1000 and 200000 keys of length 1..8 over 26 letters, 200000 keys of
length 1..20 over 4 letters, 20000 keys of length 1..200 over 2 letters; queries
mix stored keys and stored prefixes. One warmup and --reps rotating repetitions
per method; medians of build/query/erase/total are printed and written to
04-trie_benchmark.json beside this script (git-ignored). Timings are
observations, never correctness gates.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261008)
    parser.add_argument('--reps', type=int, default=5)
    args = parser.parse_args()
    if args.reps < 1 or args.reps % 2 == 0:
        parser.error('repetitions must be positive and odd')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-trie-benchmark-') as directory:
        binary = str(Path(directory) / 'benchmark')
        command = [compiler, *flags, str(HERE / '04-trie_benchmark.cpp'), '-o', binary]
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise SystemExit(f'FAIL build command={shlex.join(command)}\n{result.stderr}')
        result = subprocess.run([binary, str(args.seed), str(args.reps)], capture_output=True, text=True, timeout=600)
        if result.returncode:
            raise SystemExit(f'FAIL trie benchmark seed={args.seed}\n{result.stderr}')
    records = json.loads(result.stdout)
    for r in records:
        build, query, erase, total = r['median_ms']
        print(f"PASS {r['method']:8} n={r['n']} L<={r['max_length']} S={r['sigma']} "
              f"build={build:.3f} query={query:.3f} erase={erase:.3f} total={total:.3f} ms", flush=True)
    out = {'seed': args.seed, 'reps': args.reps, 'flags': flags,
           'compiler': subprocess.run([compiler, '--version'], capture_output=True, text=True).stdout.splitlines()[0],
           'records': records}
    (HERE / '04-trie_benchmark.json').write_text(json.dumps(out, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
