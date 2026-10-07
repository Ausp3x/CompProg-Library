#!/usr/bin/env python3
"""End-to-end warmed regular-file integer pipelines; repeated medians, no gates."""
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
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    with tempfile.TemporaryDirectory(prefix='cp-fastio-benchmark-') as directory:
        binary = Path(directory) / 'fastio'
        subprocess.run([compiler, *flags, str(HERE / '90-fastio_benchmark.cpp'), '-o', str(binary)],
                       check=True, timeout=180)
        run = subprocess.run([str(binary)], capture_output=True, text=True, check=True, timeout=180)
    groups = {}
    for line in run.stdout.splitlines():
        n, distribution, method, rep, ms, input_bytes, output_bytes, checksum = line.split()
        key = (int(n), distribution, method)
        row = groups.setdefault(key, {'n': int(n), 'distribution': distribution, 'method': method,
                                    'input_bytes': int(input_bytes), 'output_bytes': int(output_bytes),
                                    'checksum': int(checksum), 'samples_ms': []})
        if int(checksum) != row['checksum']:
            raise RuntimeError('unstable checksum')
        row['samples_ms'].append(float(ms))
    results = list(groups.values())
    for row in results:
        if len(row['samples_ms']) != 5:
            raise RuntimeError('missing timing repetitions')
        row['median_ms'] = statistics.median(row['samples_ms'])
    report = {'date_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'cpu': cpu,
              'platform': platform.platform(), 'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              'flags': flags, 'python': platform.python_version(), 'seed': 20260927,
              'warmups': 1, 'repetitions': 5, 'method_order': 'rotated per repetition',
              'workload': 'integers: read signed 64-bit decimal tokens, xor unsigned bits with fixed constant, write unsigned decimal lines; uniform [-1000,1000] or random full 64-bit patterns with min/max every 101 values and mixed whitespace. doubles-fixed9: read %.17g tokens of k/1000 * 2^e (|k| <= 10^6, e in [-20,20]), write %.9f lines; 32/20000/500000 values',
              'timed_scope': 'construct buffers, parse complete regular-file input, transform, format complete output, explicit FILE flush and writer destruction; file open/close, fixture generation, rewind, independent to_chars output comparison excluded; warm file page cache, no fsync',
              'memory': 'FastInput/Output<N>: two N-byte arrays plus constant fields; libc FILE buffers additional and implementation dependent. fscanf/fprintf uses libc buffers. O(1) conversion workspace for each. Fixture strings excluded from timed methods.',
              'comparison_limits': 'Shared-host observations for valid ASCII decimal data on warmed regular files; not interactive, cold disk, malformed-input or network throughput; fscanf/fprintf has broader locale/format support; no universal timing gates.',
              'sha256': {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                         for p in (ROOT / '06-Miscellaneous/03-fastio.hpp', HERE / '90-fastio_benchmark.cpp')},
              'results': results}
    output = HERE / '90-fastio_benchmark.json'
    output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS full independent output comparisons; benchmark record:', output)
    for row in results:
        print(row['n'], row['distribution'], row['method'], f"{row['median_ms']:.6f} ms")


if __name__ == '__main__':
    main()
