#!/usr/bin/env python3
"""Standalone headers, aggregates, multiple-TU linkage, and optional sanitizer self-tests, built as parallel jobs."""
import argparse
import contextlib
from concurrent.futures import FIRST_COMPLETED, ThreadPoolExecutor, wait
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import threading
sys.path.insert(0, str(Path(__file__).resolve().parents[0]))
import _00_memory_cap

_00_memory_cap.ensure(mb=8192)

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
LIGHT, HEAVY = 512, 1664  # MB reserved by a syntax check and by a linked build (measured peaks: Docs/00-parallel.md)


def run(command, timeout=180):
    result = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
    if result.returncode:
        raise RuntimeError('Command failed: '+repr(command)+'\n'+result.stdout+'\n'+result.stderr)


def parallel(tasks, jobs, budget):
    """Run (MB, [(command, timeout), ...][, cost]) tasks, at most `jobs` at once within `budget` MB, heaviest and costliest first.

    A task that failed beside others reruns alone afterwards, in task order; a failure alone is final, stops later
    tasks from starting, and the first one in task order is raised. With one job every task runs in order.
    """
    stopping = threading.Event()
    def task(commands):
        try:
            for command in commands:
                if stopping.is_set(): return None  # interrupted: start no further command
                run(*command)
        except (OSError, subprocess.TimeoutExpired, RuntimeError) as error:
            return error
    pending = sorted(range(len(tasks)), key=lambda i: (-tasks[i][0], -sum(tasks[i][2:]))) if jobs > 1 else list(range(len(tasks)))
    running, errors, crowded = {}, {}, set()
    with ThreadPoolExecutor(jobs) as pool:
        try:
            while pending or running:
                for i in list(pending):
                    if not running or len(running) < jobs and sum(tasks[k][0] for k in running.values()) + tasks[i][0] <= budget:
                        pending.remove(i)
                        running[pool.submit(task, tasks[i][1])] = i
                if len(running) > 1:
                    crowded |= set(running.values())
                for future in wait(running, return_when=FIRST_COMPLETED).done:
                    i = running.pop(future)
                    errors[i] = future.result()
                    if errors[i] and i not in crowded:
                        pending = [k for k in pending if k < i]
        except BaseException:
            stopping.set()  # the running commands got the signal too; their tasks start nothing more
            raise
    for i, error in sorted(errors.items()):
        if error and i not in crowded: raise error
        if error:
            tail = ''.join(f'| {line}\n' for line in str(error).strip().splitlines()[-20:])
            print(f'NOTE {type(error).__name__} beside other builds; the last lines, then its rerun alone:\n{tail}', end='', flush=True)
            for command in tasks[i][1]: run(*command)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('-j', '--jobs', type=int, default=os.cpu_count() or 1, help='parallel builds (default: logical CPUs; 1 is sequential)')
    args = parser.parse_args()
    if args.jobs < 1: parser.error('--jobs must be positive')
    workspace = ROOT/'99-Workspace/template.cpp'
    if not workspace.is_file():
        raise RuntimeError('Required workspace snapshot missing: '+str(workspace)+'; run 97-Online Testing/03-workspace.py explicitly')
    cxx = os.environ.get('CXX', 'g++')
    compiler = [*(['ccache'] if shutil.which('ccache') else []), cxx]
    os.environ.setdefault('CCACHE_NOHASHDIR', '1')  # the build directory differs per run; -g would hash it
    headers = sorted(p for d in ROOT.iterdir() if d.is_dir() and d.name[:2] in ('01','02','03','04','05','06','07') for p in d.rglob('*.hpp'))
    flags = ['-std=gnu++20']
    with tempfile.TemporaryDirectory(prefix='cp-integration-') as name, contextlib.chdir(name):
        build = Path(name)  # generated sources keep relative names, so their preprocessed text is the same every run
        Path('probe.cpp').write_text('int main() {}\n')
        probe = subprocess.run([*compiler, *flags, '-fsyntax-only', 'probe.cpp'], capture_output=True, text=True)
        if compiler[0] == 'ccache' and probe.returncode:
            print('SKIP ccache: ' + ((probe.stderr.strip().splitlines() or ['probe failed'])[0]))
            compiler = [cxx]
        def linked(output, units, options, *runs):
            """Compile each unit to an object (ccache caches -c and -fsyntax-only, never a compile-and-link), link, then run.

            Source bytes stand in for the build time when ordering the heavy tasks.
            """
            objects = [f'{output}-{k}.o' for k in range(len(units))]
            return (HEAVY, [([*compiler, *options, '-c', str(unit), '-o', o],) for unit, o in zip(units, objects)]
                    + [([cxx, *options, *objects, '-o', str(output)],), *runs], sum(u.stat().st_size for u in units))
        tasks = []
        for header in headers:
            unit = Path(header.relative_to(ROOT).with_suffix('.cpp').as_posix().replace('/', '__').replace(' ', '_'))
            unit.write_text(f'#include "{header}"\nint main() {{}}\n')
            tasks.append((LIGHT, [([*compiler, *flags, '-fsyntax-only', str(unit)],)]))
        includes = ''.join(f'#include "{p}"\n' for p in headers if p.name == '99-all.hpp')
        a,b = Path('a.cpp'),Path('b.cpp')
        a.write_text(includes+'int other(); int main() { return other(); }\n')
        b.write_text(includes+'int other() { return 0; }\n')
        variants = [('scalar', ['-mno-avx2'])]
        cpu = Path('/proc/cpuinfo')
        if cpu.exists() and 'avx2' in cpu.read_text().split(): variants.append(('avx2', ['-mavx2']))
        else: print('SKIP AVX2 integration: hardware support not detected')
        for label,target in variants:
            output = build/f'all-{label}'
            tasks.append(linked(output, [a, b], [*flags, *target], ([str(output)],)))
        for local in ([],['-DLOCAL']): tasks.append((LIGHT, [([*compiler,*flags,*local,'-fsyntax-only',str(workspace)],)]))
        if args.sanitizers:
            core = HERE/'01-Core'
            for source in sorted([*core.glob('*_tester.cpp'), *core.glob('05-*_cases.cpp')]):
                output=build/source.stem
                second=source.with_name(source.stem+'_tu.cpp')
                units=[source]+([second] if second.is_file() else [])
                tasks.append(linked(output, units, [*flags,'-O1','-g','-fno-omit-frame-pointer','-fno-pie','-no-pie',
                                                    '-fsanitize=address,undefined','-fno-sanitize-recover=all'],
                                    ([str(output),'--mode','quick'],300)))
        cap = os.environ.get('CP_MEMORY_CAPPED', '')  # the cap in force, also under an outer cap or the pre-flight
        parallel(tasks, args.jobs, (int(cap) if cap.isdigit() else _00_memory_cap.MAIN_MB) - 512)
        print(f'PASS: {len(headers)} standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace'+(', sanitizer self-tests' if args.sanitizers else ''))
    return 0

if __name__=='__main__':
    try: raise SystemExit(main())
    except (OSError,subprocess.TimeoutExpired,RuntimeError) as error:
        print('FAIL:',error,file=sys.stderr);raise SystemExit(1)
