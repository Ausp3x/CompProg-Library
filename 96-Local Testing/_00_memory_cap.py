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
4. The scope's first process gets SIGUSR2 when the supervisor dies, however it dies (a SIGKILL from a caller's timeout
   included), and lead() turns that into the end of the whole scope, so nothing in it outlives its supervisor.

5. Before any of it, the supervisor waits until ``_00_memory_gate.py`` admits the run's expected peak (the peak
   recorded for the entry and mode, else the cap; ``--need`` or ``ensure(need=...)`` override), so concurrent
   runs of several sessions queue instead of being killed.
6. After a passing run of a tester or benchmark that reserved its recorded peak, the supervisor records its peak (the largest of
   the pre-flight's and the main scope's, which the scope's first process writes to CP_MEMORY_PEAK) and seconds in
   ``.run-stats.json``, unless 01-run.py started it (CP_RUN_STATS_OWNER), which records its own runs.

Ad-hoc probes and benchmarks: ``python3 _00_memory_cap.py [--mb N] [--need N] -- <command> [args...]``.

Environment: CP_TEST_MEMORY_MB (main cap, default 4096), CP_TEST_PREFLIGHT_MB (default 2048),
CP_TEST_PREFLIGHT=0 skips the pre-flight, CP_MEMORY_CAPPED marks a process already inside a cap,
CP_MEMORY_SUPERVISOR (the supervisor's pid) marks the scope's first process until lead() takes it, CP_MEMORY_PEAK the
file its report() writes the peak to until ensure() takes it.
Without a systemd user manager the run continues uncapped and says so.
"""
import atexit
import contextlib
import ctypes
import os
from pathlib import Path
import re
import resource
import shutil
import signal
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
import _00_memory_gate  # noqa: E402

MAIN_MB = int(os.environ.get('CP_TEST_MEMORY_MB', '4096'))
PREFLIGHT_MB = int(os.environ.get('CP_TEST_PREFLIGHT_MB', '2048'))
ENTRY_SUFFIXES = ('_tester.py', '_benchmark.py')
ENTRY_NAMES = ('02-integration.py', '06-compare.py')  # entry points without a tester or benchmark suffix
_available = None
PEAK_FILE = None  # where report() also writes the scope's peak for its supervisor (CP_MEMORY_PEAK)


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


def cgroup_dir():
    """This process's cgroup v2 directory, or None."""
    try:
        return Path('/sys/fs/cgroup') / Path(open('/proc/self/cgroup').read().split('::', 1)[1].strip()).relative_to('/')
    except (OSError, ValueError, IndexError):
        return None


def cgroup_stats():
    """(peak bytes, oom kills) of the current cgroup, or None outside cgroup v2."""
    try:
        path = cgroup_dir()
        peak = int((path / 'memory.peak').read_text())
        events = dict(line.split() for line in (path / 'memory.events').read_text().splitlines())
        return peak, int(events.get('oom_kill', 0))
    except (OSError, ValueError, TypeError):
        return None


def end_scope(*_):
    """SIGKILL every process of this capped scope, this one included: its supervisor is gone, so nothing else would."""
    path = cgroup_dir()
    if path is not None and re.fullmatch(r'run-.+\.scope', path.name):
        with contextlib.suppress(OSError):
            (path / 'cgroup.kill').write_text('1')  # Linux 5.14+
        for _ in range(10):  # older kernels: one by one, again for what forked meanwhile
            with contextlib.suppress(OSError, ValueError):
                others = [int(x) for x in (path / 'cgroup.procs').read_text().split() if int(x) != os.getpid()]
                if not others:
                    break
                for pid in others:
                    with contextlib.suppress(ProcessLookupError):
                        os.kill(pid, signal.SIGKILL)
    os._exit(128 + signal.SIGKILL)


def lead():
    """In a capped scope's first process: its supervisor's death (SIGUSR2, the parent-death signal supervise() sets)
    ends the whole scope. Children do not inherit the role."""
    supervisor = os.environ.pop('CP_MEMORY_SUPERVISOR', None)
    if supervisor is None:
        return
    signal.signal(signal.SIGUSR2, end_scope)
    if os.getppid() != int(supervisor):  # it died before the handler existed
        end_scope()


def parent_death(sig):
    """A Popen preexec_fn giving the child sig when this process dies (prctl PR_SET_PDEATHSIG); None without prctl."""
    try:
        prctl = ctypes.CDLL(None, use_errno=True).prctl  # resolved before the fork
    except (OSError, AttributeError):
        return None
    return lambda: prctl(1, int(sig))


def report():
    stats = cgroup_stats()
    cap = os.environ.get('CP_MEMORY_CAPPED', 'none')
    if stats is None or cap == 'none':
        return
    peak, kills = stats
    if PEAK_FILE:
        with contextlib.suppress(OSError):
            Path(PEAK_FILE).write_text(str(peak >> 20))
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
    """(mode, source): an explicit --mode, else, when the entry reads it, CP_TEST_MODE or the runners' default 'full'; an
    entry that ignores CP_TEST_MODE has no mode from it, so an exported one starts no pre-flight (06-compare.py)."""
    for i, a in enumerate(argv):
        if is_mode_flag(a):
            if '=' in a:
                return a.split('=', 1)[1], 'argv'
            if i + 1 < len(argv):
                return argv[i + 1], 'argv'
    if honours_env and 'CP_TEST_MODE' in os.environ:
        return os.environ['CP_TEST_MODE'], 'env'
    return ('full', 'env') if honours_env else (None, 'env')


def honours_mode_env(main):
    """True when the entry point or a shared runner it imports reads CP_TEST_MODE."""
    text = main.read_text()
    runners = re.findall(r'^from (_\w*runner\w*) import', text, re.M)
    return 'CP_TEST_MODE' in text or any('CP_TEST_MODE' in (d / (r + '.py')).read_text()
                                          for r in runners for d in (main.parent, main.parent.parent) if (d / (r + '.py')).is_file())


def sigkill_note(code, peak='MEMORY unknown'):
    if re.search(r'oom_kills=[1-9]', peak):
        return 'killed at the memory cap'
    return 'killed by SIGKILL (the memory cap if a MEMORY line above shows oom_kills > 0, otherwise an external kill)' if code in (-9, 137) else 'failed'


def forwarding(command, **kw):
    """subprocess.run that passes SIGTERM and SIGHUP on to the child: sent to this supervisor alone, they would orphan it."""
    with subprocess.Popen(command, **kw) as p:
        old = {s: signal.getsignal(s) for s in (signal.SIGTERM, signal.SIGHUP)}
        for s, h in old.items():
            if h is not signal.SIG_IGN:  # nohup keeps SIGHUP ignored
                signal.signal(s, lambda sig, _: p.send_signal(sig))
        try:
            out, err = p.communicate()
        finally:
            for s, h in old.items():
                signal.signal(s, h)
        return subprocess.CompletedProcess(command, p.returncode, out, err)


def supervise(command, mode=None, source='env', label=None, need=None, key=None):
    """Run command under the pre-flight (when mode is full/stress) and main caps, once the memory gate admits need MB
    (default: the main cap; _00_memory_gate.py); return its exit status. With key (its .run-stats.json key) a passing run
    adds its peak and seconds to its record (_00_memory_gate.remember()), unless 01-run.py started it (CP_RUN_STATS_OWNER), which
    records its own runs."""
    label = label or Path(command[1] if len(command) > 1 else command[0]).name
    secs = _00_memory_gate.stats(key)[1]  # read before 01-run.py's mark drops the key, so its suites backfill too
    key = None if os.environ.pop('CP_RUN_STATS_OWNER', None) else key  # popped: entries this run starts record themselves
    peaks = [] if key else None
    with _00_memory_gate.held(MAIN_MB if need is None else need, label, secs=secs):
        start = time.monotonic()
        code = capped_run(command, mode, source, label, peaks)
        seconds = round(time.monotonic() - start, 1)
    if peaks and not code:
        try:
            _00_memory_gate.remember({key: [max(peaks), seconds]})
        except OSError as e:
            print(f'WARNING {label}: its peak was not recorded ({e.strerror})', file=sys.stderr, flush=True)
    return code


def capped_run(command, mode, source, label, peaks=None):
    """The capped run of supervise(); peaks, when a list, gets the pre-flight's and the main scope's peaks in MB."""
    signal.signal(signal.SIGINT, lambda *_: None)  # the child's group gets the signal too: wait for its own cleanup
    base, scope = dict(os.environ), available()
    if not scope:
        print(f'WARNING {label}: no systemd user scope; running WITHOUT a memory cap', file=sys.stderr, flush=True)
    mark = lambda mb: str(mb) if scope else 'none'
    if scope:  # the scope's first process ends the scope when this supervisor dies (lead())
        base['CP_MEMORY_SUPERVISOR'] = str(os.getpid())
    bound = dict(preexec_fn=parent_death(signal.SIGUSR2)) if scope else {}
    if mode in ('full', 'stress') and os.environ.get('CP_TEST_PREFLIGHT', '1') != '0':
        quick, env = list(command), dict(base, CP_MEMORY_CAPPED=mark(PREFLIGHT_MB), CP_TEST_MODE='quick')
        if source == 'argv':
            quick = ['quick' if i and is_mode_flag(command[i - 1]) and '=' not in command[i - 1]
                     else command[i].split('=', 1)[0] + '=quick' if is_mode_flag(a) and '=' in a else a
                     for i, a in enumerate(command)]
        result = forwarding(capped(quick, PREFLIGHT_MB), env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, **bound)
        peak = next((l for l in result.stdout.splitlines() if l.startswith('MEMORY ')), 'MEMORY unknown')
        if result.returncode == -signal.SIGINT:  # the quick run itself died of the interrupt
            print(f'INTERRUPTED {label}: pre-flight quick run stopped by SIGINT; {mode} run skipped', file=sys.stderr, flush=True)
            return 128 + signal.SIGINT
        if result.returncode:
            tail = '\n'.join((result.stdout + result.stderr).splitlines()[-40:])
            print(f'{tail}\nFAIL {label}: pre-flight quick run {sigkill_note(result.returncode, peak)} ({peak}); {mode} run skipped',
                  file=sys.stderr, flush=True)
            return exit_code(result.returncode) or 1
        if peaks is not None:
            peaks += [int(x) for x in re.findall(r'^MEMORY peak=(\d+)MB', result.stdout, re.M)]
        timings = [l if l.startswith('TIMING ') else 'OVERHALF pre-flight quick ' + l.removeprefix('OVERHALF ')  # for 01-run.py's NOTEs
                   for l in result.stdout.splitlines() if l.startswith(('TIMING ', 'OVERHALF '))]
        print('\n'.join([f'PREFLIGHT {label}: quick passed, {peak}', *timings]), flush=True)
    env = dict(base, CP_MEMORY_CAPPED=mark(MAIN_MB))
    if scope and peaks is not None:
        fd, env['CP_MEMORY_PEAK'] = tempfile.mkstemp(prefix='cp-peak-')
        os.close(fd)
    try:
        code = forwarding(capped(command, MAIN_MB), env=env, **bound).returncode
        if 'CP_MEMORY_PEAK' in env:
            with contextlib.suppress(OSError, ValueError):
                peaks.append(int(Path(env['CP_MEMORY_PEAK']).read_text()))
    finally:
        if 'CP_MEMORY_PEAK' in env:
            os.unlink(env['CP_MEMORY_PEAK'])
    if code in (-9, 137):
        print(f'FAIL {label}: {sigkill_note(code)} (cap {MAIN_MB} MB, exit {exit_code(code)})', file=sys.stderr, flush=True)
    return exit_code(code)


def ensure(force=False, mb=None, need=None, variant=None):
    """Make the running entry point memory-capped; returns only inside the cap (or the warned uncapped fallback).

    ``mb`` raises or lowers the main cap for one suite; CP_TEST_MEMORY_MB still wins when set. ``need`` is the memory the
    gate waits for (0: none, for a wrapper whose heavy children gate themselves); by default the peak recorded for the
    entry in its mode, else the cap, and only then does a tester or benchmark record its own peak. ``variant``, a callable
    returning the options an entry's peak depends on (02-integration.py, 06-compare.py run), makes it record and reserve
    under its key followed by them too.
    """
    global MAIN_MB, PEAK_FILE
    if mb and 'CP_TEST_MEMORY_MB' not in os.environ:
        MAIN_MB = mb
    if os.environ.get('CP_MEMORY_CAPPED'):
        hard = resource.getrlimit(resource.RLIMIT_CORE)[1]
        if hard == resource.RLIM_INFINITY or hard >= 1:  # at 1 the kernel skips a piped core_pattern: 190 ms per death probe under systemd-coredump
            resource.setrlimit(resource.RLIMIT_CORE, (1, hard))
        lead()
        PEAK_FILE = os.environ.pop('CP_MEMORY_PEAK', None) or PEAK_FILE  # the scope's first process alone reports it
        if os.environ['CP_MEMORY_CAPPED'] != 'none':
            atexit.unregister(report)  # a second ensure() (an imported entry point) reports once
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
    v = variant() if variant and need is None else None  # parsed before the cap: a bad option exits uncapped
    key = _00_memory_gate.key(mode, main, v) if need is None and (main.name.endswith(ENTRY_SUFFIXES) or v) else None
    need = need if need is not None else _00_memory_gate.recorded(mode, main, MAIN_MB, v) if key else MAIN_MB  # a record only under a key it writes
    raise SystemExit(supervise([sys.executable, str(main), *sys.argv[1:]], mode, source, main.name, need, key))


if __name__ == '__main__':
    args = sys.argv[1:]
    usage = 'usage: _00_memory_cap.py [--mb N] [--need N] -- <command> [args...]'
    need = None
    while args and args[0].split('=', 1)[0] in ('--mb', '--need'):
        flag, (value, args) = args[0].split('=', 1)[0], (args[0].split('=', 1)[1], args[1:]) if '=' in args[0] else ((args[1:2] or [''])[0], args[2:])
        if not value.isdigit() or (flag == '--mb' and int(value) < 16):
            raise SystemExit(usage + '\n--mb needs an integer of at least 16, --need a nonnegative integer')
        if flag == '--mb':
            MAIN_MB = int(value)
        else:
            need = int(value)
    if args[:1] == ['--']:
        args = args[1:]
    if not args:
        raise SystemExit(usage)
    os.environ.pop('CP_MEMORY_CAPPED', None)
    reporter = [sys.executable, '-c', 'import signal, subprocess, sys; signal.signal(signal.SIGINT, lambda *_: None); sys.path.insert(0, sys.argv[1]); '
                'import _00_memory_cap as m; m.lead(); code = m.forwarding(sys.argv[2:]).returncode; m.report(); sys.exit(m.exit_code(code))',
                str(Path(__file__).resolve().parent), *args]
    raise SystemExit(supervise(reporter, None, 'env', Path(args[0]).name, need))
