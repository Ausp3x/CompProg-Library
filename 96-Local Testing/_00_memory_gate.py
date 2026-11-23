#!/usr/bin/env python3
"""Memory admission gate (SUP33): a capped run waits until the machine's free memory covers its expected peak.

`_00_memory_cap.supervise()` holds `held(need, label)` around every capped run (testers, benchmarks, `02-integration.py`,
`06-compare.py`, the mutation campaign's baselines and mutants and ad-hoc `_00_memory_cap.py -- <command>` runs), so
runs of several sessions queue instead of meeting the OOM killer (2026-11-17, 2026-11-18). The need is the run's recorded
peak (`01-run.py`'s `.run-stats.json`, 11/10 of it plus 64 MB, as `01-run.py` schedules) or its cap when unrecorded.

One JSON file of reservations, rewritten under an exclusive lock, is shared by the user's processes: each entry names a
supervisor (pid and start time, so a dead or reused pid drops out), its need, whether it runs or waits, and since when.
A waiter is admitted when it is the oldest live waiter (one that polled within STALE seconds; a run inside a running
reservation of its ancestor does not queue) and MemAvailable, less what running reservations may still grow
into (need minus the current memory of the capped scopes below them, each once, and the RSS of their other
descendants), less its own need, leaves RESERVE_MB. It is admitted with a WARNING when only its own ancestors run, since
they wait for it, and when nothing else runs, since waiting would not free the memory. Polls every second.

Environment: CP_MEMORY_GATE (the reservations file, default $XDG_RUNTIME_DIR/cp-memory-gate.json; `0` turns the gate
off), CP_MEMORY_GATE_RESERVE_MB (default 2048).
"""
import contextlib
import fcntl
import json
import os
from pathlib import Path
import re
import sys
import tempfile
import time

RESERVE_MB = int(os.environ.get('CP_MEMORY_GATE_RESERVE_MB', '2048'))
MEMINFO = Path('/proc/meminfo')
PROC = Path('/proc')
CGROUP = Path('/sys/fs/cgroup')
POLL = 1.0
STALE = 30  # seconds after which a waiter that stopped polling (suspended, hung) no longer holds its place
STATS = Path(__file__).resolve().parent / '.run-stats.json'


def store():
    """The reservations file, or None when the gate is off."""
    where = os.environ.get('CP_MEMORY_GATE', '')
    if where == '0':
        return None
    return Path(where) if where else Path(os.environ.get('XDG_RUNTIME_DIR') or tempfile.gettempdir()) / 'cp-memory-gate.json'


def need_of(peak, cap):
    """The memory a run reserves: 11/10 of its recorded peak plus 64 MB (01-run.py, run), else its cap."""
    return peak * 11 // 10 + 64 if peak else cap


def recorded(mode, main, cap):
    """need_of the peak 01-run.py recorded for main (an entry under 96-Local Testing) in mode, else cap."""
    try:
        rel = Path(main).resolve().relative_to(STATS.parent).as_posix()
        peak = json.loads(STATS.read_text()).get(f'{mode} {rel}', [0])[0]
        return need_of(peak if isinstance(peak, int) and peak > 0 else 0, cap)
    except (OSError, ValueError, TypeError, AttributeError, IndexError):
        return cap


def available():
    """MemAvailable in MB, or None without /proc/meminfo."""
    try:
        return int(re.search(r'^MemAvailable:\s+(\d+)', MEMINFO.read_text(), re.M)[1]) >> 10
    except (OSError, TypeError):
        return None


def stat(pid):
    """(parent pid, start time) of pid, or None when it is gone."""
    try:
        fields = (PROC / str(pid) / 'stat').read_text().rsplit(')', 1)[1].split()
        return int(fields[1]), int(fields[19])
    except (OSError, IndexError, ValueError):
        return None


def ancestors(pid):
    out, seen = set(), set()
    while pid > 1 and pid not in seen and (s := stat(pid)):
        seen.add(pid)
        pid = s[0]
        out.add(pid)
    return out


def children():
    """{parent pid: [child pids]} of every process."""
    out = {}
    for d in PROC.iterdir():
        if d.name.isdigit() and (s := stat(int(d.name))):
            out.setdefault(s[0], []).append(int(d.name))
    return out


def scope(pid):
    """(path, MB) of the run-*.scope cgroup pid runs in, MB None without memory.current; None outside one."""
    try:
        path = CGROUP / (PROC / str(pid) / 'cgroup').read_text().split('::', 1)[1].strip().lstrip('/')
    except (OSError, IndexError):
        return None
    if not re.fullmatch(r'run-.+\.scope', path.name):
        return None
    try:
        return path, int((path / 'memory.current').read_text()) >> 20
    except (OSError, ValueError):
        return path, None


def rss_mb(pid):
    try:
        return int((PROC / str(pid) / 'statm').read_text().split()[1]) * os.sysconf('SC_PAGE_SIZE') >> 20
    except (OSError, IndexError, ValueError):
        return 0


def usage(pid, tree):
    """MB a reservation's run holds now: each run-*.scope its descendants run in, once, except the one pid runs in itself
    (a campaign reserving in its own capped process), plus the RSS of the descendants outside such scopes."""
    own = (scope(pid) or (None,))[0]
    seen, total, todo = set(), 0, list(tree.get(pid, []))
    while todo:
        p = todo.pop()
        todo += tree.get(p, [])
        s = scope(p)
        if s and s[0] != own and s[1] is not None:
            total += s[1] * (s[0] not in seen)
            seen.add(s[0])
        else:
            total += rss_mb(p)
    return total


def decide(free, me, entries, grows, mine):
    """(admit, warning) for entry me among the live entries: free is MemAvailable in MB, grows(entry) the MB a running
    entry may still use, mine the pids of me's ancestors. A run inside a running reservation (its ancestor's) does not
    queue, since the waiters before it may wait for that very reservation; a waiter whose last poll is STALE seconds old
    holds no place."""
    running = [e for e in entries if e is not me and e['state'] == 'run']
    beat = lambda e: e.get('beat', e['since'])
    if not any(e['pid'] in mine for e in running) and any(
            e['state'] == 'wait' and (e['since'], e['pid']) < (me['since'], me['pid']) and beat(e) >= beat(me) - STALE
            for e in entries if e is not me):
        return False, ''
    others = [e for e in running if e['pid'] not in mine]
    if free is None or free - sum(map(grows, running)) - me['need'] >= RESERVE_MB:
        return True, ''
    if not others:
        held = f'{len(running)} ancestor run(s) hold reservations' if running else 'no other gated run is running'
        return True, f'{free} MB available, {me["need"]} MB needed and {RESERVE_MB} MB kept free; {held}, so it runs now'
    return False, ''


@contextlib.contextmanager
def locked(path):
    with open(path.with_name(path.name + '.lock'), 'a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(lock, fcntl.LOCK_UN)


def load(path):
    """The live entries of the reservations file."""
    try:
        entries = json.loads(path.read_text())
    except (OSError, ValueError):
        return []
    ok = lambda e: (isinstance(e, dict) and all(isinstance(e.get(k), int) for k in ('pid', 'start', 'need'))
                    and all(isinstance(e.get(k, 0.0), (int, float)) for k in ('since', 'beat')) and 'since' in e
                    and e.get('state') in ('wait', 'run'))
    return [e for e in entries if ok(e) and (stat(e['pid']) or (0, None))[1] == e['start']] if isinstance(entries, list) else []


def save(path, entries):
    tmp = path.with_name(f'{path.name}.{os.getpid()}')
    try:
        tmp.write_text(json.dumps(entries))
        tmp.replace(path)
    finally:
        tmp.unlink(missing_ok=True)


def wait(path, me, label, out, until=None):
    """Block until me is admitted (its entry then runs) and return True; print WAIT once, then ADMIT, and any WARNING.
    Return False, still queued, once the time.monotonic() deadline until has passed."""
    mine, waited = ancestors(me['pid']), None
    while True:
        with locked(path):
            me['beat'] = time.time()
            entries = [e for e in load(path) if e['pid'] != me['pid']] + [me]
            tree = children()
            admit, warning = decide(available(), me, entries, lambda e: max(0, e['need'] - usage(e['pid'], tree)), mine)
            me['state'] = 'run' if admit else 'wait'
            save(path, entries)
        if admit:
            break
        if until is not None and time.monotonic() >= until:
            print(f'STOP {label}: its deadline passed in the memory queue', file=out, flush=True)
            return False
        if waited is None:
            waited = time.monotonic()
            print(f'WAIT {label}: needs {me["need"]} MB beside the running gated runs (_00_memory_gate.py); queued', file=out, flush=True)
        time.sleep(POLL)
    if warning:
        print(f'WARNING {label}: memory gate: {warning}', file=out, flush=True)
    if waited is not None:
        print(f'ADMIT {label}: after {time.monotonic() - waited:.0f} s in the memory queue', file=out, flush=True)
    return True


@contextlib.contextmanager
def held(need, label, out=sys.stderr, until=None):
    """Wait for admission of a run needing need MB, hold its reservation while the block runs, then release it; a need of
    0 or less, or the gate turned off, passes at once. Yields whether it was admitted: False only when the
    time.monotonic() deadline until passed in the queue, and the caller then skips its run."""
    path = store()
    if path is None or need <= 0:
        yield True
        return
    me = {'pid': os.getpid(), 'start': (stat(os.getpid()) or (0, 0))[1], 'need': int(need), 'label': label, 'state': 'wait',
          'since': time.time()}
    try:
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            admitted = wait(path, me, label, out, until)
        except OSError as e:  # an unusable reservations file never stops a run
            print(f'WARNING {label}: memory gate unavailable ({e.strerror}); running without it', file=out, flush=True)
            admitted = None
        if not admitted:  # past the deadline, or without the gate: no entry of it stays while the block runs
            release(path, me)
        yield admitted is not False
    finally:  # also after an interrupt while queued
        release(path, me)


def release(path, me):
    """Remove me's entry, if saved: one left from a failed or interrupted wait would block every later waiter."""
    with contextlib.suppress(OSError), locked(path):
        entries = load(path)
        if any(e['pid'] == me['pid'] for e in entries):
            save(path, [e for e in entries if e['pid'] != me['pid']])
