#!/usr/bin/env python3
"""Counting builds (compressCounting, stableRanksCounting) against their sorting forms: perf stat cycles and instructions per call; timings are not correctness gates."""
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
ALGOS = {1: 'CoordinateCompression encode', 2: 'compressCounting', 3: 'stableRanks', 4: 'stableRanksCounting'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=1)
    parser.add_argument('--runs', type=int, default=5)
    parser.add_argument('--compiler', default=os.environ.get('CXX', 'g++'))
    parser.add_argument('--output', type=Path, default=HERE / '004-compression_benchmark.json')
    args = parser.parse_args()
    if args.runs < 3:
        parser.error('--runs must be at least 3')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG', '-falign-functions=64', '-falign-loops=32']

    def run(command):
        result = subprocess.run(command, capture_output=True, text=True, timeout=600)
        if result.returncode:
            raise RuntimeError(f'command={command!r} returncode={result.returncode}\n{result.stdout}\n{result.stderr}')
        return result

    def counters(binary, algo, n, span, reps):
        out = run(['perf', 'stat', '-x,', '-e', 'cycles:u,instructions:u', str(binary), str(algo), str(n), str(span), str(args.seed), str(reps)])
        values = {}
        for line in out.stderr.splitlines():
            fields = line.split(',')
            if len(fields) > 2 and fields[0].isdigit():
                values[fields[2].split(':')[0]] = int(fields[0])
        return values['cycles'], values['instructions'], out.stdout.strip()

    summary = []
    with tempfile.TemporaryDirectory(prefix='cp-compression-benchmark-') as name:
        binary = Path(name) / 'bench'
        run([args.compiler, *flags, '-I', str(HERE.parents[1]), str(HERE / '004-compression_benchmark.cpp'), '-o', str(binary)])
        version = run([args.compiler, '--version']).stdout.splitlines()[0]
        for n in (1000, 100000, 1000000):
            for ratio in (1 / 16, 1, 16, 256):
                span = max(1, min(int(n * ratio), 1 << 25))
                reps = max(3, 20_000_000 // (n + span))
                base = [counters(binary, 0, n, span, reps) for _ in range(args.runs)]
                base_c, base_i = statistics.median(b[0] for b in base), statistics.median(b[1] for b in base)
                row = dict(n=n, span=span, reps=reps)
                for algo, label in ALGOS.items():
                    runs = [counters(binary, algo, n, span, reps) for _ in range(args.runs)]
                    row[label] = dict(cycles_per_call=round((statistics.median(r[0] for r in runs) - base_c) / reps),
                                      instructions_per_call=round((statistics.median(r[1] for r in runs) - base_i) / reps))
                summary.append(row)
                print(f"n={n:>8} span={span:>9} " + ' '.join(f"{label}={row[label]['cycles_per_call']:>12}" for label in ALGOS.values()), flush=True)
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    record = dict(seed=args.seed, runs=args.runs, compiler=version, flags=flags, platform=platform.platform(), cpu=cpu,
                  workload='n lng values uniform in [-span/2, span/2) (mt19937_64 seed), one element flipped per call; median user-mode cycles and instructions over runs minus the input-only baseline, divided by calls',
                  results=summary)
    args.output.write_text(json.dumps(record, indent=2) + '\n')


if __name__ == '__main__':
    main()
