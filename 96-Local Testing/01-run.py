#!/usr/bin/env python3
"""Run the migrated library suites; mode names describe actual available coverage."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
QUICK = {'01-template_tester.py', '02-debug_tester.py', '03-barrett_tester.py', '04-montgomery_tester.py', '05-modint_tester.py', '06-modintmini_tester.py', '18-bitset_tester.py'}

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['quick', 'full', 'stress'], default='full')
    p.add_argument('--filter', default='', help='substring of the relative suite path')
    p.add_argument('--seed', type=int, default=42)
    p.add_argument('--rounds', type=int, default=3, help='stress seed rounds')
    p.add_argument('--no-integration', action='store_true', help='skip integration and repository consistency checks')
    a = p.parse_args()
    if a.rounds < 1: p.error('--rounds must be positive')
    suites = sorted(x for x in HERE.glob('[0-9][0-9]-*/*_tester.py')
                    if '_compile_' not in x.name and a.filter in x.relative_to(HERE).as_posix())
    if a.mode == 'quick':
        suites = [x for x in suites if x.name in QUICK or x.parent.name in ('00-Tools', '02-Data Structures', '03-Geometry', '04-Graphs', '05-Mathematics', '06-Miscellaneous', '07-Strings')]
    if not suites: p.error('no suites match')
    runs = a.rounds if a.mode == 'stress' else 1
    for i in range(runs):
        env = dict(os.environ, CP_TEST_MODE=a.mode, CP_TEST_SEED=str(a.seed + i))
        for suite in suites:
            print(f'RUN {suite.relative_to(HERE)} mode={a.mode} seed={a.seed + i}', flush=True)
            result = subprocess.run([sys.executable, str(suite)], env=env)
            if result.returncode: return result.returncode
    if not a.no_integration:
        command = [sys.executable, str(HERE / '02-integration.py')]
        if a.mode != 'quick': command.append('--sanitizers')
        result = subprocess.run(command)
        if result.returncode: return result.returncode
        result = subprocess.run([sys.executable, str(HERE / '03-consistency.py')])
        if result.returncode: return result.returncode
    print(f'PASS: {len(suites)} available suites x {runs} round(s); see 00-index.md for coverage gaps')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
