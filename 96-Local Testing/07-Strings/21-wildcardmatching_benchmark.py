"""Measure the wildcard naive threshold and block spread, and headline matcher costs; emit JSONL medians."""
import argparse
import json
import os
from pathlib import Path
import platform
import shlex
import subprocess
import tempfile
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

HERE = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--seed', type=int, default=20261016)
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    flags = ['-std=gnu++20', '-O2', '-DNDEBUG']
    cpu = next((line.split(':', 1)[1].strip() for line in Path('/proc/cpuinfo').read_text().splitlines()
                if line.startswith('model name')), 'unknown')
    log = HERE / '21-wildcardmatching_benchmark.jsonl'
    header = {'compiler': subprocess.check_output([compiler, '--version'], text=True).splitlines()[0],
              'flags': flags, 'cpu': cpu, 'python': platform.python_version(), 'seed': args.seed,
              'warmup': 1, 'repetitions': 5, 'statistic': 'median'}
    print(json.dumps(header), flush=True)
    with tempfile.TemporaryDirectory(prefix='cp-wildcard-bench-') as tmp, open(log, 'w') as out:
        out.write(json.dumps(header) + '\n')
        binary = Path(tmp) / 'bench'
        build = [compiler, *flags, str(HERE / '21-wildcardmatching_benchmark.cpp'), '-o', str(binary)]
        try:
            subprocess.run(build, check=True, timeout=900)
            result = subprocess.run([str(binary), str(args.seed)], check=True, timeout=3000, capture_output=True, text=True)
        except (OSError, subprocess.CalledProcessError, subprocess.TimeoutExpired) as error:
            raise SystemExit(f'FAIL seed={args.seed} command={shlex.join(build)} error={error}')
        print(result.stdout, end='', flush=True)
        out.write(result.stdout)


if __name__ == '__main__':
    main()
