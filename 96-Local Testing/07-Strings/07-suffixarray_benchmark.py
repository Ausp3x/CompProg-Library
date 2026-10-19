"""SuffixArray construction: SA-IS (vector through alphabetCodes, and bytes) against SuffixArray<int>::doubling.

Workloads: 2,000,000 symbols split into independent texts of 8/16/32/64/128/256/1024
symbols over 4 and 26 letters and unary texts of 16/64/256 symbols; 1,000,000-symbol
random texts over 2 and 26 letters, sparse values below 10^9 (no byte method), the
Fibonacci prefix and a unary text. Methods vector, bytes and doubling build sa, rank
and lcp without RMQ. Code-range threshold workloads (2,000,000 symbols of random values
in [0, range), n = 16/64 with range 64/128/256 and n = 10^4/10^6 with range n/2..16n)
compare SA-IS over shifted codes (shift) with SA-IS through compressAlphabet (compress).
One warmup and --reps rotating repetitions per method; sa/lcp checksums must agree.
Medians are printed and written to 07-suffixarray_benchmark.json beside this script
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
    with tempfile.TemporaryDirectory(prefix='cp-suffixarray-benchmark-') as directory:
        binary = str(Path(directory) / 'benchmark')
        command = [compiler, *flags, str(HERE / '07-suffixarray_benchmark.cpp'), '-o', binary]
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise SystemExit(f'FAIL build command={shlex.join(command)}\n{result.stderr}')
        result = subprocess.run([binary, str(args.seed), str(args.reps)], capture_output=True, text=True, timeout=900)
        if result.returncode:
            raise SystemExit(f'FAIL suffixarray benchmark seed={args.seed}\n{result.stderr}')
    records = json.loads(result.stdout)
    for r in records:
        print(f"PASS {r['workload']:24} {r['method']:9} median={r['median_ms']:.2f} ms", flush=True)
    out = {'seed': args.seed, 'reps': args.reps, 'flags': flags,
           'compiler': subprocess.run([compiler, '--version'], capture_output=True, text=True).stdout.splitlines()[0],
           'records': records}
    (HERE / '07-suffixarray_benchmark.json').write_text(json.dumps(out, indent=2) + '\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
