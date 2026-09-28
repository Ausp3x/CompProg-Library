"""Reproducible static-hash setup/LCP benchmark; every query is scan-verified.

No dependencies beyond Python and GNU C++20. Outputs a JSON record beside this
script by default; compile products are temporary and working directory is free.
The recorded timings are observations, never correctness gates.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import shlex
import statistics
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
SOURCE = HERE / '03-stringhash_benchmark.cpp'
HEADER = HERE.parents[1] / '07-Strings' / '03-stringhash.hpp'


def run(command):
    result = subprocess.run(command, text=True, capture_output=True, timeout=180)
    if result.returncode:
        raise RuntimeError(f'returncode={result.returncode} command={shlex.join(command)}\n'
                           f'{result.stdout}\n{result.stderr}')
    return result.stdout


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20260928)
    parser.add_argument('--warmup', type=int, default=1)
    parser.add_argument('--reps', type=int, default=5)
    parser.add_argument('--output', type=Path, default=HERE / '03-stringhash_benchmark.json')
    args = parser.parse_args()
    if not 0 <= args.seed < 2 ** 64 or args.warmup < 0 or args.reps < 1 or args.reps % 2 == 0:
        parser.error('seed must fit uint64; warmup>=0; repetitions must be positive and odd')
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    print(f'BUILD stringhash benchmark seed={args.seed} flags={shlex.join(flags)}', flush=True)
    with tempfile.TemporaryDirectory(prefix='cp-stringhash-benchmark-') as directory:
        binary = str(Path(directory) / 'benchmark')
        run([compiler, *flags, str(SOURCE), '-o', binary])
        print(f'RUN warmup={args.warmup} repetitions={args.reps}; verify every query', flush=True)
        record = json.loads(run([binary, '--seed', str(args.seed), '--warmup', str(args.warmup),
                                 '--reps', str(args.reps)]))
    cpu = 'unknown'
    cpuinfo = Path('/proc/cpuinfo')
    if cpuinfo.exists():
        cpu = next((line.split(':', 1)[1].strip() for line in cpuinfo.read_text().splitlines()
                    if line.startswith('model name')), cpu)
    record['metadata'] = {
        'utc': datetime.now(timezone.utc).isoformat(), 'cpu': cpu,
        'platform': platform.platform(), 'python': platform.python_version(),
        'compiler': run([compiler, '--version']).splitlines()[0], 'flags': flags,
        'seed': args.seed, 'warmup_per_workload_method': args.warmup,
        'measured_repetitions': args.reps, 'clock': 'std::chrono::steady_clock',
        'method_order': 'rotate by repetition; identical queries and input per method',
        'bases': {'double-prime': [911382323, 972663749], 'wrap64': 11400714819323198485},
        'input_distribution': 'uniform 256-byte random; period-11 abracadabra; unary a; empty',
        'query_distribution': 'uniform suffix starts; 1/8 identical starts; remaining even periodic '
                              'queries align phases; odd queries independently cap both ends',
        'timing_scope': 'setup includes hash object construction and all three prefix/power arrays; '
                        'queries fill preallocated output; pipeline is setup+queries per repetition; '
                        'input/query generation, direct oracle, output verification and object '
                        'destruction excluded; direct scanning has zero setup',
        'memory_scope': 'algorithm auxiliary bytes are sizeof(hash object)+actual vector capacity '
                        'payloads; direct scan is zero; shared input and harness arrays reported '
                        'separately; allocator metadata/stack/RSS are not measured',
        'verification': 'all answers in warmup and measured repetitions equal direct byte scans',
        'source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        'header_sha256': hashlib.sha256(HEADER.read_bytes()).hexdigest(),
        'reproduce': "python3 '96-Local Testing/07-Strings/03-stringhash_benchmark.py' "
                     f'--seed {args.seed} --warmup {args.warmup} --reps {args.reps}',
        'caveat': 'shared-machine observations; fixed-base agreement is finite evidence and '
                  'does not remove hash collision risk; no speed or timing gates',
    }
    for workload in record['workloads']:
        for result in workload['results']:
            for sample in result['raw_ns']:
                if sample['pipeline'] != sample['setup'] + sample['query']:
                    raise RuntimeError('pipeline timing does not equal setup+query')
            expected = {key: statistics.median(sample[key] for sample in result['raw_ns'])
                        for key in ('setup', 'query', 'pipeline')}
            if expected != result['median_ns']:
                raise RuntimeError('incorrect median aggregation')
        summary = ', '.join(f"{result['method']}={result['median_ns']['pipeline']/1e6:.3f}ms"
                            for result in workload['results'])
        print(f"PASS shape={workload['shape']} n={workload['n']} q={workload['queries']} {summary}", flush=True)
    args.output.write_text(json.dumps(record, indent=2) + '\n')
    print(f'PASS all outputs; evidence={args.output}', flush=True)


if __name__ == '__main__':
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired, json.JSONDecodeError) as error:
        raise SystemExit(f'FAIL stringhash benchmark: {error}') from error
