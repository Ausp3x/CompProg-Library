#!/usr/bin/env python3
"""Memory-safe execution of library tests, benchmarks and ad-hoc commands.

Every test entry point calls ``ensure()`` (the shared runners do so on import). The first,
uncapped invocation becomes a small supervisor:

1. For ``full`` or ``stress`` mode (``--mode`` or ``CP_TEST_MODE``) it first reruns the same
   command in quick mode under the pre-flight cap and skips the long run when quick fails or
   is killed.
2. It reruns the command inside a transient systemd user scope with ``MemoryMax`` set to the
   cap, swap disabled and ``OOMPolicy=continue``. A runaway process is killed alone (exit -9),
   the rest of the desktop is untouched, and the runner reports the failure.
3. Inside the scope, the process prints ``MEMORY peak=... cap=... oom_kills=...`` at exit,
   with a FAIL line when the cap killed anything.

Ad-hoc probes and benchmarks: ``python3 _00_memory_cap.py [--mb N] -- <command> [args...]``.

Environment: CP_TEST_MEMORY_MB (main cap, default 4096), CP_TEST_PREFLIGHT_MB (default 2048),
CP_TEST_PREFLIGHT=0 skips the pre-flight, CP_MEMORY_CAPPED marks a process already inside a cap.
Without a systemd user manager the run continues uncapped and says so.
"""
import atexit
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

MAIN_MB = int(os.environ.get('CP_TEST_MEMORY_MB', '4096'))
PREFLIGHT_MB = int(os.environ.get('CP_TEST_PREFLIGHT_MB', '2048'))
ENTRY_SUFFIXES = ('_tester.py', '_benchmark.py')
ENTRY_NAMES = ('02-integration.py',)
_available = None


def available():
    """True when transient memory-capped user scopes work on this machine."""
    global _available
    if _available is None:
        _available = bool(shutil.which('systemd-run')) and subprocess.run(
            ['systemd-run', '--user', '--scope', '--quiet', '-p', 'MemoryMax=64M', 'true'],
            capture_output=True).returncode == 0
    return _available


def capped(command, mb):
    """The command prefixed with a memory-capped scope (unchanged when scopes are unavailable)."""
    if not available():
        return list(command)
    return ['systemd-run', '--user', '--scope', '--quiet', '--expand-environment=no',
            '-p', f'MemoryMax={mb}M', '-p', 'MemorySwapMax=0', '-p', 'OOMPolicy=continue', '--', *command]


def cgroup_stats():
    """(peak bytes, oom kills) of the current cgroup, or None outside cgroup v2."""
    try:
        path = Path('/sys/fs/cgroup') / Path(open('/proc/self/cgroup').read().split('::', 1)[1].strip()).relative_to('/')
        peak = int((path / 'memory.peak').read_text())
        events = dict(line.split() for line in (path / 'memory.events').read_text().splitlines())
        return peak, int(events.get('oom_kill', 0))
    except (OSError, ValueError, IndexError):
        return None


def report():
    stats = cgroup_stats()
    cap = os.environ.get('CP_MEMORY_CAPPED', 'none')
    if stats is None or cap == 'none':
        return
    peak, kills = stats
    try:
        print(f'MEMORY peak={peak >> 20}MB cap={cap}MB oom_kills={kills}', flush=True)
    except OSError:
        os.dup2(os.open(os.devnull, os.O_WRONLY), sys.stdout.fileno())
    if kills:
        print(f'FAIL memory cap: {kills} process(es) killed at the {cap} MB cap; '
              'look for unbounded allocation or reduce the mode size', file=sys.stderr, flush=True)


def exit_code(code):
    """Shell-style exit status: a signal -k becomes 128 + k."""
    return code if code >= 0 else 128 - code


def is_mode_flag(arg):
    """'--mode' or an argparse abbreviation of it ('--mo', '--mod'), optionally with '=value'."""
    flag = arg.split('=', 1)[0]
    return len(flag) >= 4 and '--mode'.startswith(flag)


def mode_of(argv, honours_env):
    """(mode, source): an explicit --mode, else CP_TEST_MODE, else the runners' default 'full' when the entry reads it."""
    for i, a in enumerate(argv):
        if is_mode_flag(a):
            if '=' in a:
                return a.split('=', 1)[1], 'argv'
            if i + 1 < len(argv):
                return argv[i + 1], 'argv'
    if 'CP_TEST_MODE' in os.environ:
        return os.environ['CP_TEST_MODE'], 'env'
    return ('full', 'env') if honours_env else (None, 'env')


def honours_mode_env(main):
    """True when the entry point or a shared runner it imports reads CP_TEST_MODE."""
    text = main.read_text()
    runners = re.findall(r'^from (_\w*runner\w*) import', text, re.M)
    return 'CP_TEST_MODE' in text or any('CP_TEST_MODE' in (main.parent / (r + '.py')).read_text()
                                          for r in runners if (main.parent / (r + '.py')).is_file())


def sigkill_note(code, peak='MEMORY unknown'):
    if re.search(r'oom_kills=[1-9]', peak):
        return 'killed at the memory cap'
    return 'killed by SIGKILL (the memory cap if a MEMORY line above shows oom_kills > 0, otherwise an external kill)' if code in (-9, 137) else 'failed'


def supervise(command, mode=None, source='env', label=None):
    """Run command under the pre-flight (when mode is full/stress) and main caps; return its exit status."""
    label = label or Path(command[1] if len(command) > 1 else command[0]).name
    base, scope = dict(os.environ), available()
    if not scope:
        print(f'WARNING {label}: no systemd user scope; running WITHOUT a memory cap', file=sys.stderr, flush=True)
    mark = lambda mb: str(mb) if scope else 'none'
    if mode in ('full', 'stress') and os.environ.get('CP_TEST_PREFLIGHT', '1') != '0':
        quick, env = list(command), dict(base, CP_MEMORY_CAPPED=mark(PREFLIGHT_MB), CP_TEST_MODE='quick')
        if source == 'argv':
            quick = ['quick' if i and is_mode_flag(command[i - 1]) and '=' not in command[i - 1]
                     else command[i].split('=', 1)[0] + '=quick' if is_mode_flag(a) and '=' in a else a
                     for i, a in enumerate(command)]
        result = subprocess.run(capped(quick, PREFLIGHT_MB), env=env, capture_output=True, text=True)
        peak = next((l for l in result.stdout.splitlines() if l.startswith('MEMORY ')), 'MEMORY unknown')
        if result.returncode:
            tail = '\n'.join((result.stdout + result.stderr).splitlines()[-40:])
            print(f'{tail}\nFAIL {label}: pre-flight quick run {sigkill_note(result.returncode, peak)} ({peak}); {mode} run skipped',
                  file=sys.stderr, flush=True)
            return exit_code(result.returncode) or 1
        print(f'PREFLIGHT {label}: quick passed, {peak}', flush=True)
    code = subprocess.run(capped(command, MAIN_MB), env=dict(base, CP_MEMORY_CAPPED=mark(MAIN_MB))).returncode
    if code in (-9, 137):
        print(f'FAIL {label}: {sigkill_note(code)} (cap {MAIN_MB} MB, exit {exit_code(code)})', file=sys.stderr, flush=True)
    return exit_code(code)


def ensure(force=False, mb=None):
    """Make the running entry point memory-capped; returns only inside the cap (or the warned uncapped fallback).

    ``mb`` raises or lowers the main cap for one suite; CP_TEST_MEMORY_MB still wins when set.
    """
    global MAIN_MB
    if mb and 'CP_TEST_MEMORY_MB' not in os.environ:
        MAIN_MB = mb
    if os.environ.get('CP_MEMORY_CAPPED'):
        if os.environ['CP_MEMORY_CAPPED'] != 'none':
            atexit.register(report)
        return
    main = getattr(sys.modules.get('__main__'), '__file__', None)
    if not main:
        return
    main = Path(main).resolve()
    if not force and not (main.name.endswith(ENTRY_SUFFIXES) or main.name in ENTRY_NAMES):
        return
    mode, source = mode_of(sys.argv[1:], honours_mode_env(main))
    sys.stdout.flush()
    raise SystemExit(supervise([sys.executable, str(main), *sys.argv[1:]], mode, source, main.name))


if __name__ == '__main__':
    args = sys.argv[1:]
    usage = 'usage: _00_memory_cap.py [--mb N] -- <command> [args...]'
    if args and (args[0] == '--mb' or args[0].startswith('--mb=')):
        value, args = (args[0].split('=', 1)[1], args[1:]) if '=' in args[0] else ((args[1:2] or [''])[0], args[2:])
        if not value.isdigit() or int(value) < 16:
            raise SystemExit(usage + '\n--mb needs an integer of at least 16')
        MAIN_MB = int(value)
    if args[:1] == ['--']:
        args = args[1:]
    if not args:
        raise SystemExit(usage)
    os.environ.pop('CP_MEMORY_CAPPED', None)
    reporter = [sys.executable, '-c', 'import subprocess, sys; sys.path.insert(0, sys.argv[1]); '
                'import _00_memory_cap as m; code = subprocess.run(sys.argv[2:]).returncode; m.report(); sys.exit(m.exit_code(code))',
                str(Path(__file__).resolve().parent), *args]
    raise SystemExit(supervise(reporter, None, 'env', Path(args[0]).name))
