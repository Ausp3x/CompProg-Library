"""Montgomery (Core Montgomery64) versus the Compact twins (plain 128-bit remainders) for primality and factorization.

Builds the C++ workload with -O2 (no -march), runs one warmup plus N timed repetitions per
workload, checks identical outputs, and writes medians with CPU/compiler conditions to
07-primality_factorization_benchmark.json beside this driver.
"""
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def main():
    reps = int(sys.argv[1]) if len(sys.argv) > 1 else 5
    cxx = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2']
    with tempfile.TemporaryDirectory(prefix='cp-bench-') as d:
        exe = Path(d) / 'bench'
        subprocess.run([cxx, *flags, str(HERE / '07-primality_factorization_benchmark.cpp'), '-o', str(exe)], check=True)
        out = subprocess.run([str(exe), str(reps)], check=True, capture_output=True, text=True, timeout=1800).stdout
    rows = json.loads(out)
    cpu = next((l.split(':', 1)[1].strip() for l in open('/proc/cpuinfo') if l.startswith('model name')), platform.processor())
    version = subprocess.run([cxx, '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    record = {'cpu': cpu, 'compiler': version, 'flags': flags, 'reps': reps, 'warmup': 1, 'rows': rows}
    (HERE / '07-primality_factorization_benchmark.json').write_text(json.dumps(record, indent=2) + '\n')
    print(f'{cpu} | {version} | {" ".join(flags)} | median of {reps}')
    for r in rows:
        print(f"{r['workload']:<40} montgomery {r['montgomery_ms']:9.2f} ms  compact {r['compact_ms']:9.2f} ms  "
              f"saving {100 * (1 - r['montgomery_ms'] / r['compact_ms']):5.1f}%")


if __name__ == '__main__':
    main()
