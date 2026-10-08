"""Shared driver for independently checked string C++ feature suites."""
import argparse
import os
from pathlib import Path
import shlex
import signal
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent


def main(stem, invalid, oracle=None, strict=False):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'),
                        default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '20260927')))
    parser.add_argument('--configuration', choices=('all', 'optimized', 'checked', 'ASan-UBSan'),
                        default='all', help='select a build when retrying an environment failure')
    args = parser.parse_args()
    compiler = os.environ.get('CXX', 'g++')
    warnings = ['-Wall', '-Wextra', '-Wshadow', '-Wconversion', '-Werror'] if strict else []
    checked = '-D_GLIBCXX_DEBUG'
    variants = [('optimized', ['-O2', '-DNDEBUG']), ('checked', ['-O0', '-g', checked])]
    if args.mode != 'quick':
        variants.append(('ASan-UBSan', ['-O1', '-g', '-D_GLIBCXX_ASSERTIONS',
                         '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
                         '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']))
    if args.configuration != 'all':
        variants = [(label, flags) for label, flags in variants if label == args.configuration]
        if not variants:
            parser.error('quick mode excludes ASan-UBSan; use full or stress')
    env = dict(os.environ, ASAN_OPTIONS='detect_leaks=1:halt_on_error=1',
               UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')
    color = sys.stdout.isatty() and 'NO_COLOR' not in os.environ

    def progress(message):
        if sys.stdout.isatty():
            print('\r' + message.ljust(110), end='', flush=True)
        else:
            print(message, flush=True)

    def run(command, *, death=False):
        try:
            result = subprocess.run(command, capture_output=True, text=True, env=env, timeout=180)
        except subprocess.TimeoutExpired as error:
            raise RuntimeError(f'timeout=180s command={shlex.join(command)}\n{error.stdout}\n{error.stderr}') from error
        if death:
            ok = result.returncode == -signal.SIGABRT and 'assert' in result.stderr.lower()
        else:
            ok = result.returncode == 0
        if not ok:
            raise RuntimeError(f'returncode={result.returncode} expected={"assertion SIGABRT" if death else "0"}'
                               f' command={shlex.join(command)}\n{result.stdout}\n{result.stderr}')
        if result.stdout:
            print(result.stdout.rstrip(), flush=True)

    try:
        with tempfile.TemporaryDirectory(prefix='cp-strings-') as name:
            for label, flags in variants:
                binary = Path(name) / label
                progress(f'BUILD {stem} configuration={label} mode={args.mode} seed={args.seed}')
                run([compiler, '-std=gnu++20', *warnings, *flags, str(HERE / (stem + '_tester.cpp')), '-o', str(binary)])
                progress(f'RUN {stem} configuration={label} mode={args.mode} seed={args.seed}')
                if sys.stdout.isatty():
                    print(flush=True)
                run([str(binary), '--mode', args.mode, '--seed', str(args.seed)])
                if oracle is not None:
                    oracle(binary, args, env)
                if label == 'checked':
                    for probe in invalid:
                        run([str(binary), '--invalid', probe], death=True)
                    print(f'PASS {stem} {len(invalid)} assertion probes', flush=True)
        print(('\033[32m' if color else '') + f'PASS {stem}: {len(variants)} configurations'
              + ('\033[0m' if color else ''), flush=True)
        return 0
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        if sys.stdout.isatty():
            print(flush=True)
        print(('\033[31m' if color else '') + f'FAIL {stem} mode={args.mode} seed={args.seed}: {error}'
              + ('\033[0m' if color else ''), file=sys.stderr, flush=True)
        return 1
