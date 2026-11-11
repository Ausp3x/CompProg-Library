"""Shared driver of the independently checked C++ feature suites of 02-07: each suite's tester, in its folder, calls main()."""
import argparse
import functools
import os
from pathlib import Path
import re
import shlex
import signal
import subprocess
import sys
import tempfile
import time
sys.path.insert(0, str(Path(__file__).resolve().parent))
import _00_memory_cap
from _00_cpp_test_runner import Builder, compiler_command, scaled, timed_run, timing

_00_memory_cap.ensure()

WARNINGS = ['-Wall', '-Wextra', '-Wconversion', '-Werror']


def main(stem, invalid, oracle=None, extra=(), timeout=720, checked='-D_GLIBCXX_DEBUG', memory=1200):
    """Build <stem>_tester.cpp beside the calling tester in each configuration and run it.

    oracle(binary, args, env) runs after each build's self-test; invalid names the checked build's --invalid death probes;
    extra holds further compiler flags; timeout is the limit of each compile and run in seconds; checked is the checked
    build's standard-library mode, -D_GLIBCXX_ASSERTIONS where debug iterators' own checks would change a suite's cost;
    memory is the MB one build needs in full mode, compile or run (quick: 800 unless the suite needs more), by which the
    builds run at once within the cap; stress runs them one at a time, its peak being unmeasured.
    """
    here = Path(sys.modules['__main__'].__file__).resolve().parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'),
                        default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '20260927')))
    parser.add_argument('--configuration', default='all', type=lambda text: text.split(','),
                        help='all, or a comma-separated subset of optimized, checked and ASan-UBSan')
    args = parser.parse_args()
    names = ('optimized', 'checked', 'ASan-UBSan')
    if args.configuration != ['all'] and not set(args.configuration) <= set(names):
        parser.error(f'--configuration takes all or a comma-separated subset of {", ".join(names)}')
    compiler = compiler_command()
    limit = scaled(timeout)
    variants = [('optimized', ['-O2', '-DNDEBUG']), ('checked', ['-O0' if args.mode == 'quick' else '-O1', '-g', checked])]
    if args.mode != 'quick' or 'ASan-UBSan' in args.configuration:
        variants.append(('ASan-UBSan', ['-O1', '-g', '-D_GLIBCXX_ASSERTIONS',
                         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                         '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']))
    if args.configuration != ['all']:
        variants = [(label, flags) for label, flags in variants if label in args.configuration]
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    color = sys.stdout.isatty() and 'NO_COLOR' not in os.environ
    need = 1 << 40 if args.mode == 'stress' else 800 if args.mode == 'quick' and memory == 1200 else memory

    def run(longest, command, *, death=False):
        start = time.monotonic()
        try:
            result = timed_run(f'{stem} {Path(command[0]).name}', command, capture_output=True, text=True, env=env, timeout=limit)
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(f'timeout={limit}s command={shlex.join(command)}\n{error.stdout}\n{error.stderr}') from error
        longest[0] = max(longest[0], time.monotonic() - start)
        if death:
            # glibc's assert prints "Assertion `"; libstdc++'s bounds checks print "Assertion '" and do not count.
            ok = result.returncode == -signal.SIGABRT and 'Assertion `' in result.stderr
        else:
            ok = result.returncode == 0
        if not ok:
            raise RuntimeError(f'returncode={result.returncode} expected={"assertion SIGABRT" if death else "0"}'
                               f' command={shlex.join(command)}\n{result.stdout}\n{result.stderr}')
        if result.stdout:
            print(result.stdout.rstrip(), flush=True)

    try:
        with tempfile.TemporaryDirectory(prefix='cp-contest-') as name:
            builder = Builder(compiler, name, env=env, failure=RuntimeError)

            def one(label, flags):
                binary = Path(name) / label
                print(f'RUN {stem} configuration={label} mode={args.mode} seed={args.seed}', flush=True)
                seconds = builder.build(f'compile {stem} {label}', ['-std=gnu++20', *WARNINGS, *extra, *flags],
                                        [here / (stem + '_tester.cpp')], binary, limit)
                start, longest = time.monotonic(), [0.0]
                run(longest, [str(binary), '--mode', args.mode, '--seed', str(args.seed)])
                checked_at = time.monotonic()
                if oracle is not None:
                    oracle(binary, args, env)
                seconds_oracle = time.monotonic() - checked_at
                if label == 'checked':
                    for probe in invalid:
                        run(longest, [str(binary), '--invalid', probe], death=True)
                    print(f'PASS {stem} {len(invalid)} assertion probes', flush=True)
                timing(f'{stem} {label}', args.mode, seconds, time.monotonic() - start,
                       f' longest={longest[0]:.2f}s limit={limit}s oracle={seconds_oracle:.2f}s')
            builder.concurrently([(need, functools.partial(one, label, flags)) for label, flags in variants])
        peak, cap = (_00_memory_cap.cgroup_stats() or (0, 0))[0] >> 20, os.environ.get('CP_MEMORY_CAPPED', '')
        if len(variants) > 1 and cap.isdigit() and peak >= int(cap) * 9 // 10:
            print(f'NOTE {stem}: builds at once peaked at {peak} of {cap} MB; declare one build\'s peak with memory= (05-testing.md, Memory safety)', flush=True)
        print(('\033[32m' if color else '') + f'PASS {stem}: {len(variants)} configurations'
              + ('\033[0m' if color else ''), flush=True)
        return 0
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        if re.search(r'(returncode|exit)=-9\b', str(error)) and len(variants) > 1:
            error = f'{error}\nkilled by SIGKILL: if the MEMORY line shows oom_kills > 0, declare one build\'s peak with memory= so the builds stop sharing the cap'
        print(('\033[31m' if color else '') + f'FAIL {stem} mode={args.mode} seed={args.seed}: {error}'
              + ('\033[0m' if color else ''), file=sys.stderr, flush=True)
        return 1
