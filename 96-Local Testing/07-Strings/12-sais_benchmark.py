"""SA-IS naive-threshold sweep (NAIVE = 1, 8, 16, 32, 64) and prefix-doubling SuffixArray.

Workloads: 2,000,000 symbols split into independent texts of 8/16/24/32/64/256
symbols over 4 letters and unary texts of 16/32/48/64 symbols (worst case for
the naive comparison sort); 1,000,000-symbol random texts over 2, 4 and 26 letters;
the 1,000,000-symbol Fibonacci prefix and a unary text. One warmup and --reps
rotating repetitions per method; checksums of every suffix array must agree.
Medians are printed and written to 12-sais_benchmark.json beside this script
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
    parser.add_argument('--seed', type=int, default=20261009)
    parser.add_argument('--reps', type=int, default=5)
    args = parser.parse_args()
    if args.reps < 1 or args.reps % 2 == 0:
        parser.error('repetitions must be positive and odd')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-sais-benchmark-') as directory:
        binary = str(Path(directory) / 'benchmark')
        command = [compiler, *flags, str(HERE / '12-sais_benchmark.cpp'), '-o', binary]
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise SystemExit(f'FAIL build command={shlex.join(command)}\n{result.stderr}')
        result = subprocess.run([binary, str(args.seed), str(args.reps)], capture_output=True, text=True, timeout=900)
        if result.returncode:
            raise SystemExit(f'FAIL sais benchmark seed={args.seed}\n{result.stderr}')
    records = json.loads(result.stdout)
    for r in records:
        print(f"PASS {r['workload']:28} {r['method']:9} median={r['median_ms']:.2f} ms", flush=True)
    out = {'seed': args.seed, 'reps': args.reps, 'flags': flags,
           'compiler': subprocess.run([compiler, '--version'], capture_output=True, text=True).stdout.splitlines()[0],
           'records': records}
    (HERE / '12-sais_benchmark.json').write_text(json.dumps(out, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
