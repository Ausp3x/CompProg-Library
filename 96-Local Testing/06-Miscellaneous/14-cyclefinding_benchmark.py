#!/usr/bin/env python3
"""Complete Floyd/Brent orbit recovery with counted cheap/expensive successors."""
import datetime
import hashlib
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def main():
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    with tempfile.TemporaryDirectory(prefix='cp-cyclefinding-benchmark-') as directory:
        binary = Path(directory) / 'cyclefinding'
        command = [compiler, *flags, str(HERE / '14-cyclefinding_benchmark.cpp'), '-o', str(binary)]
        subprocess.run(command, check=True, timeout=180)
        result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=180)
        if result.returncode:
            raise RuntimeError(f'command={binary} returncode={result.returncode}\n{result.stderr}')
    groups = {}
    for line in result.stdout.splitlines():
        tail, period, successor, method, rep, batch, calls, checksum, ms = line.split()
        key = (int(tail), int(period), successor, method)
        row = groups.setdefault(key, {
            'tail': int(tail), 'period': int(period), 'entry': int(tail),
            'successor': successor, 'method': method, 'batch': int(batch),
            'successor_evaluations': int(calls), 'callback_checksum': int(checksum),
            'samples_ms': [], 'repetitions': [],
        })
        if row['callback_checksum'] != int(checksum) or row['successor_evaluations'] != int(calls):
            raise RuntimeError(f'inconsistent callback output for {key}')
        row['samples_ms'].append(float(ms))
        row['repetitions'].append(int(rep))
    rows = list(groups.values())
    if len(rows) != 80:
        raise RuntimeError(f'expected 80 workload/method rows, got {len(rows)}')
    for row in rows:
        if sorted(row.pop('repetitions')) != list(range(5)):
            raise RuntimeError(f'missing repetitions for {row}')
        row['median_ms'] = statistics.median(row['samples_ms'])
    floyd = {(row['tail'], row['period'], row['successor']): row for row in rows if row['method'] == 'floyd'}
    for row in rows:
        reference = floyd[row['tail'], row['period'], row['successor']]
        row['median_fraction_of_floyd'] = row['median_ms'] / reference['median_ms']
        row['evaluation_fraction_of_floyd'] = row['successor_evaluations'] / reference['successor_evaluations']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    report = {
        'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
        'cpu': cpu, 'platform': platform.platform(),
        'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
        'flags': flags, 'python': platform.python_version(), 'seed': 20260928,
        'warmups': 1, 'repetitions': 5, 'method_order': 'rotated per repetition',
        'workload': 'Twenty explicit lollipop orbits: 0->1->...->mu+lambda-1->mu. Includes self-loops, pure cycles, short/long tails, and periods/tails around powers of two; largest orbit 65,539 states. Successors use contiguous index tables; mix16 additionally performs 16 rounds of two unsigned 64-bit multiplications/xors per invocation.',
        'timed_scope': 'Complete detection, cycle entry recovery, and period determination with ULLONG_MAX callback budget. Includes local callback/result setup and counter/checksum operations. Input tables, expensive-digest reference precomputation, mathematical reference construction, output-buffer allocation, and validation are outside timing. Tiny workloads average 128 traversals, medium 8, and large 1 per sample.',
        'instrumentation': 'Both methods increment an observed volatile counter at every successor invocation, retaining all traversals and measuring actual calls; this common overhead is material for the cheap table workload. mix16 also accumulates a retained per-invocation checksum and calls a noinline arithmetic function, retaining its expensive work. Comparisons use the same instrumentation.',
        'memory': 'Both algorithms retain O(1) integer states. Caller-owned tables use (mu+lambda) ints; expensive checksum validation adds one uint64 per state outside timing. Retained result buffers hold at most 128 constant-size outputs.',
        'verification': 'Every warmup and timed result checks the independently known entry=tail=mu and period=lambda. Reported evaluations equal observed callback calls and independent complete-pipeline closed forms: Floyd 3*max(lambda,ceil(mu/lambda)*lambda)+2*mu+lambda; Brent p-1+2*lambda+2*mu with p=nextPow2(max(mu+1,lambda)). Heavy checksums equal untimed runs using precomputed digests. Zero, exactly-required and one-short budgets are checked outside timing. All checks survive NDEBUG.',
        'comparison_limits': 'Single-threaded shared-host warmed-cache observations on integer states and contiguous orbit tables. Counts refer to the present full recovery implementations, not detection alone. Volatile counting overhead can affect cheap-callback timing; expensive cost is synthetic fixed integer arithmetic. Does not predict arbitrary equality/state-copy costs, random table layouts, cold caches or application successors; neither method is declared universally faster.',
        'sha256': {str(path.relative_to(ROOT)): hashlib.sha256(path.read_bytes()).hexdigest()
                   for path in (ROOT / '06-Miscellaneous/14-cyclefinding.hpp',
                                HERE / '14-cyclefinding_benchmark.cpp', HERE / '14-cyclefinding_benchmark.py')},
        'results': rows,
    }
    output = HERE / '14-cyclefinding_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS {len(rows)} workload/method comparisons; complete results/calls/checksums verified; record={output}')
    for row in rows:
        if row['method'] == 'brent':
            print(f"mu={row['tail']} lambda={row['period']} {row['successor']}",
                  f"{row['median_ms']:.6f} ms", f"{row['median_fraction_of_floyd']:.4f} x Floyd time",
                  f"{row['successor_evaluations']} calls", f"{row['evaluation_fraction_of_floyd']:.4f} x Floyd calls")


if __name__ == '__main__':
    main()
