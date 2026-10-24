#!/usr/bin/env python3
"""Run the library suites as parallel memory-bounded jobs; printed output and exit status match a sequential run."""
import argparse
import contextlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time

HERE = Path(__file__).resolve().parent
QUICK = {'01-template_tester.py', '02-debug_tester.py', '03-barrett_tester.py', '04-montgomery_tester.py', '05-modint_tester.py', '06-modintmini_tester.py', '07-infint_tester.py', '08-infintmini_tester.py', '09-rational_tester.py', '16-poly_tester.py', '17-polymini_tester.py', '18-bitset_tester.py'}
STATS = HERE / '.run-stats.json'  # git-ignored: last peak MB and seconds per mode and suite, for scheduling only
CAP_MB = int(os.environ.get('CP_TEST_MEMORY_MB', '4096'))
SIGNALS = (signal.SIGINT, signal.SIGTERM, signal.SIGHUP)  # each stops every job as Ctrl-C does


def available_mb():
    """MemAvailable in MB; unbounded without /proc/meminfo."""
    try:
        return int(re.search(r'^MemAvailable:\s+(\d+)', Path('/proc/meminfo').read_text(), re.M)[1]) >> 10
    except (OSError, TypeError):
        return 1 << 40


def signal_group(proc, sig):
    try:
        os.killpg(proc.pid, sig)
    except ProcessLookupError:
        pass


@contextlib.contextmanager
def held():
    """Hold SIGINT, SIGTERM and SIGHUP (those not ignored) until the block ends, then raise KeyboardInterrupt if one came."""
    hit, old = [], {sig: signal.getsignal(sig) for sig in SIGNALS}
    for sig, handler in old.items():
        if handler is not signal.SIG_IGN:
            signal.signal(sig, lambda *_: hit.append(1))
    try:
        yield
    finally:
        for sig, handler in old.items():
            signal.signal(sig, handler)
    if hit:
        raise KeyboardInterrupt


def stop(procs):
    """Interrupt the jobs' process groups so their temporary directories are removed; kill survivors after 10 s."""
    with held():
        for proc in procs:
            signal_group(proc, signal.SIGINT)
        end = time.monotonic() + 10
        for proc in procs:
            try:
                proc.wait(max(0, end - time.monotonic()))
            except subprocess.TimeoutExpired:
                signal_group(proc, signal.SIGKILL)
                proc.wait()


def call(command):
    """Exit status of command, run as its own process group that a signal to the runner stops."""
    proc = subprocess.Popen(command, start_new_session=True)
    try:
        return proc.wait()
    except BaseException:
        stop([proc])
        raise


def run(tasks, mode, jobs, budget, stats):
    """Run (key, seed, suite) tasks, longest first, each reserving 5/4 of its last peak plus 256 MB (the cap when unmeasured).

    A task that fails after running beside others reruns alone, and later running tasks restart after it, so only a
    failure alone counts. Logs print in task order (in task order and live when jobs is 1); the status is that of the
    first failing task, as in a sequential run.
    """
    def need(i):
        peak = stats.get(tasks[i][0], (0, 0))[0]
        return peak * 5 // 4 + 256 if peak else CAP_MB
    running, codes, alone, crowded, head, failed, reader = {}, {}, {}, set(), 0, len(tasks), None
    rank = lambda i: (i not in alone, (i in alone) * i, -stats.get(tasks[i][0], (0, float('inf')))[1])  # reruns in task order
    queue = sorted(range(len(tasks)), key=rank) if jobs > 1 else list(range(len(tasks)))
    fits = lambda i: not running or (i not in alone and not alone.keys() & running.keys() and len(running) < jobs
                                     and sum(map(need, running)) + need(i) <= budget and need(i) <= available_mb())
    with tempfile.TemporaryDirectory(prefix='cp-run-') as logs:
        log = lambda i, kind='': Path(logs) / f'{i}{kind}.log'
        try:
            while head < failed or head in codes:
                queue = [i for i in queue if i < failed]
                while queue and fits(queue[0]):
                    i = queue.pop(0)
                    with open(log(i), 'wb') as out, held():  # recorded in running before an interrupt can land
                        running[i] = subprocess.Popen([sys.executable, str(tasks[i][2])], stdin=subprocess.DEVNULL, stdout=out,
                                                      stderr=subprocess.STDOUT, start_new_session=True,
                                                      env=dict(os.environ, CP_TEST_MODE=mode, CP_TEST_SEED=str(tasks[i][1]))), time.monotonic()
                if len(running) > 1:
                    crowded |= running.keys()
                for i in list(running):
                    if i not in running or running[i][0].poll() is None:
                        continue
                    proc, start = running.pop(i)
                    if proc.returncode and i in crowded and i not in alone:
                        alone[i] = proc.returncode
                        log(i).rename(log(i, '.crowded'))
                        later = [j for j in running if j > i and running[j][0].poll() is None]  # restart after the rerun
                        with held():
                            stop([running.pop(j)[0] for j in later])
                        crowded -= set(later)
                        queue = sorted([*queue, i, *later], key=rank)
                        continue
                    codes[i] = proc.returncode
                    if not proc.returncode:
                        peaks = re.findall(rb'MEMORY peak=(\d+)MB', log(i).read_bytes())
                        stats[tasks[i][0]] = [max(map(int, peaks), default=0), round(time.monotonic() - start, 1)]
                    else:
                        failed = i
                if reader is None and (head in codes or jobs == 1 and head in running):
                    rel = tasks[head][2].relative_to(HERE)
                    if head in alone:
                        print(f'NOTE {rel} exited {alone[head]} beside other suites; the last lines of that run, then its rerun alone:', flush=True)
                        sys.stdout.buffer.write(b''.join(b'| ' + line + b'\n' for line in log(head, '.crowded').read_bytes().splitlines()[-20:]))
                    print(f'RUN {rel} mode={mode} seed={tasks[head][1]}', flush=True)
                    reader = open(log(head), 'rb')
                if reader:
                    sys.stdout.buffer.write(reader.read())
                    sys.stdout.flush()
                if head in codes:
                    reader.close()
                    reader = None
                    if codes[head]:
                        return codes[head]
                    head += 1
                else:
                    time.sleep(0.1)
            return 0
        finally:
            if reader:
                reader.close()
            stop([proc for proc, _ in running.values()])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--mode', choices=['quick', 'full', 'stress'], default='full')
    p.add_argument('--filter', default='', help='substring of the relative suite path')
    p.add_argument('--seed', type=int, default=42)
    p.add_argument('--rounds', type=int, default=3, help='stress seed rounds')
    p.add_argument('-j', '--jobs', type=int, default=os.cpu_count() or 1, help='parallel suites (default: logical CPUs; 1 is sequential)')
    p.add_argument('--memory-mb', type=int, default=0, help='memory budget of the running suites (default: MemAvailable at start minus 2048)')
    p.add_argument('--no-integration', action='store_true', help='skip integration and repository consistency checks')
    a = p.parse_args()
    for sig in SIGNALS:
        if signal.getsignal(sig) is not signal.SIG_IGN:  # nohup keeps SIGHUP ignored
            signal.signal(sig, signal.default_int_handler)
    os.environ.pop('CP_MEMORY_CAPPED', None)  # every suite and the integration build cap themselves in their own scopes
    if a.rounds < 1 or a.jobs < 1 or a.memory_mb < 0: p.error('--rounds and --jobs must be positive, --memory-mb nonnegative')
    suites = sorted(x for x in HERE.glob('[0-9][0-9]-*/*_tester.py')
                    if '_compile_' not in x.name and a.filter in x.relative_to(HERE).as_posix())
    if a.mode == 'quick':
        suites = [x for x in suites if x.name in QUICK or x.parent.name in ('00-Tools', '02-Data Structures', '03-Geometry', '04-Graphs', '05-Mathematics', '06-Miscellaneous', '07-Strings', '08-Python')]
    if not suites: p.error('no suites match')
    runs = a.rounds if a.mode == 'stress' else 1
    try:
        stats = {key: [int(v[0]), float(v[1])] for key, v in json.loads(STATS.read_text()).items()}
    except (OSError, ValueError, TypeError, AttributeError, IndexError, KeyError):
        stats = {}
    tasks = [(f'{a.mode} {s.relative_to(HERE).as_posix()}', a.seed + i, s) for i in range(runs) for s in suites]
    try:
        code = run(tasks, a.mode, a.jobs, a.memory_mb or available_mb() - 2048, stats)
    finally:
        temporary = STATS.with_name(f'{STATS.name}.{os.getpid()}')
        temporary.write_text(json.dumps(stats, indent=0, sort_keys=True) + '\n')
        temporary.replace(STATS)
    if code: return code
    if not a.no_integration:
        command = [sys.executable, str(HERE / '02-integration.py'), '--jobs', str(a.jobs)]
        if a.mode != 'quick': command.append('--sanitizers')
        code = call(command) or call([sys.executable, str(HERE / '03-consistency.py')])
        if code: return code
    print(f'PASS: {len(suites)} available suites x {runs} round(s); see 00-index.md for coverage gaps')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
