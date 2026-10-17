"""The shipped josephus (run skipping) against the plain O(n) survivor loop it replaced, for small to huge k.

Builds the C++ workload with -O2 (no -march), runs one warmup plus N timed repetitions per
workload, checks identical survivors, and writes medians with CPU and compiler conditions
to 19-josephus_benchmark.json beside this driver.
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
        subprocess.run([cxx, *flags, str(HERE / '19-josephus_benchmark.cpp'), '-o', str(exe)], check=True)
        out = subprocess.run([str(exe), str(reps)], check=True, capture_output=True, text=True, timeout=1800).stdout
    rows = json.loads(out)
    cpu = next((l.split(':', 1)[1].strip() for l in open('/proc/cpuinfo') if l.startswith('model name')), platform.processor())
    version = subprocess.run([cxx, '--version'], capture_output=True, text=True).stdout.splitlines()[0]
    record = {'cpu': cpu, 'compiler': version, 'flags': flags, 'reps': reps, 'warmup': 1, 'rows': rows}
    (HERE / '19-josephus_benchmark.json').write_text(json.dumps(record, indent=2) + '\n')
    print(f'{cpu} | {version} | {" ".join(flags)} | median of {reps}')
    for r in rows:
        print(f"{r['workload']:<45} plain {r['plain_ms']:9.2f} ms  josephus {r['josephus_ms']:9.2f} ms")


if __name__ == '__main__':
    main()
