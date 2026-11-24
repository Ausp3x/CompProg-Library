#!/usr/bin/env python3
"""Memory admission gate (SUP33, SUP34): a capped run waits until the machine's free memory covers its expected peak.

`_00_memory_cap.supervise()` holds `held(need, label)` around every capped run (testers, benchmarks, `02-integration.py`,
`06-compare.py`, the mutation campaign's baselines and mutants and ad-hoc `_00_memory_cap.py -- <command>` runs), so
runs of several sessions queue instead of meeting the OOM killer (2026-11-17, 2026-11-18). The need is the run's recorded
peak (`.run-stats.json`, 11/10 of it plus 64 MB, as `01-run.py` schedules) or its cap when unrecorded.

One JSON file of reservations, rewritten under an exclusive lock, is shared by the user's processes: each entry names a
supervisor (pid and start time, so a dead or reused pid drops out), its need, whether it runs or waits, since when, its
last poll and its recorded seconds. Times are CLOCK_MONOTONIC (time.monotonic()), which is shared by every process and
stops while the machine sleeps, so after a resume the waiters' polls are as far apart as before and none finds another
stale; an entry of pre-SUP34 code, on the wall clock, is converted to it when deciding. A waiter is admitted when it is the oldest live waiter (one that polled within STALE seconds) and MemAvailable,
less what running reservations may still grow into (need minus the current memory of the capped scopes below them, reclaimable
page cache excluded, each once, and the PSS of their other descendants; a descendant holding its own running reservation counts
under that one, since a recorded peak covers only its own scope), less its own need, leaves RESERVE_MB. A younger waiter
starts past the head (backfill) when it fits now and its recorded seconds end before the head is expected to fit, by the
running reservations' recorded seconds. A run inside a running reservation of its ancestor does not queue, since the
waiters before it may wait for that very reservation, but needs its whole need beside it. It is admitted with a WARNING when only its own ancestors run, since
they wait for it, when nothing else runs, since waiting would not free the memory, and, the oldest such run, when every
running reservation waits for a queued run below it (two sessions' nested runs). Polls every second; a lock held for
LOCK_WAIT seconds (a stopped holder) makes the run warn and go on without the gate.

`remember()` merges peaks into `.run-stats.json` under its own lock, each record the maxima of its last RECENT runs:
`supervise()` records the runs it starts and `01-run.py` its own.

Environment: CP_MEMORY_GATE (the reservations file, default $XDG_RUNTIME_DIR/cp-memory-gate.json; `0` turns the gate
off), CP_MEMORY_GATE_RESERVE_MB (default 2048), CP_MEMORY_GATE_STATE (a file the first held() of a process writes `wait`
to while queued and `run` after, so a caller's timeout can leave out the queue: 05-extraction.py testers), CP_RUN_STATS
(the recorded peaks, default `.run-stats.json` here; tests point it at a temporary file).
"""
import contextlib
import errno
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
STALE = 30  # seconds after which a waiter that stopped polling (stopped, hung) no longer holds its place
WALL = 1e9  # seconds: wall-clock times since 2001; a monotonic reading reaches it after 31 years of uptime
LOCK_WAIT = 10.0  # seconds a lock may stay held before the run goes on without it: far above a locked section's time
LOCK_POLL = 0.005
HERE = Path(__file__).resolve().parent  # 96-Local Testing: keys name entries relative to it
STATS = Path(os.environ.get('CP_RUN_STATS') or HERE / '.run-stats.json')
RECENT = 3  # runs a record keeps


def store():
    """The reservations file, or None when the gate is off."""
    where = os.environ.get('CP_MEMORY_GATE', '')
    if where == '0':
        return None
    return Path(where) if where else Path(os.environ.get('XDG_RUNTIME_DIR') or tempfile.gettempdir()) / 'cp-memory-gate.json'


def need_of(peak, cap):
    """The memory a run reserves: 11/10 of its recorded peak plus 64 MB, at most its cap, else its cap (01-run.py schedules by it too)."""
    return min(peak * 11 // 10 + 64, cap) if peak else cap


def key(mode, main, variant=None):
    """The .run-stats.json key of main (an entry under 96-Local Testing) in mode, `run` without one (a benchmark), then
    variant (the options its peak depends on) when given; None outside the folder."""
    try:
        return f'{mode or "run"} {Path(main).resolve().relative_to(HERE).as_posix()}' + (f' {variant}' if variant else '')
    except ValueError:
        return None


def stats(k):
    """(peak MB, seconds) recorded under key k, each 0 when missing or malformed."""
    try:
        v = json.loads(STATS.read_text()).get(k) if k else None
        number = lambda x: isinstance(x, (int, float)) and not isinstance(x, bool) and x > 0
        return (v[0] if isinstance(v[0], int) and v[0] > 0 else 0), (v[1] if len(v) > 1 and number(v[1]) else 0)
    except (OSError, ValueError, TypeError, AttributeError, IndexError, KeyError):
        return 0, 0


def recorded(mode, main, cap, variant=None):
    """need_of the peak recorded for main (an entry under 96-Local Testing) in mode and variant, else cap."""
    return need_of(stats(key(mode, main, variant))[0], cap)


def remember(updates):
    """Merge {key: [peak MB, seconds]} into .run-stats.json under its lock, so no concurrent writer's update is lost; the
    file is replaced atomically, and written even without updates, as 01-run.py always leaves one. A record keeps the
    last RECENT runs' pairs after its peak and seconds, which are their maxima (user decision 2026-11-24): a lighter
    suite lowers its record after RECENT runs, while one light, partial or warm-cache run cannot. A two-element record,
    written before this rule or by code from before it, counts as one run. A lock held past LOCK_WAIT merges without it."""
    def pair(x):
        return isinstance(x, list) and len(x) >= 2 and all(isinstance(y, (int, float)) and not isinstance(y, bool) for y in x[:2])

    def merge():
        try:
            data = json.loads(STATS.read_text())
        except (OSError, ValueError):
            data = {}
        data = data if isinstance(data, dict) else {}
        for k, v in updates.items():
            old = data.get(k)
            runs = [r for r in old[2] if pair(r)] if pair(old) and len(old) > 2 and isinstance(old[2], list) else [old[:2]] if pair(old) else []
            runs = [r[:2] for r in runs][-(RECENT - 1):] + [list(v)]
            data[k] = [max(r[0] for r in runs), max(r[1] for r in runs), runs]
        save(STATS, json.dumps(data, indent=0, sort_keys=True) + '\n')
    try:
        with locked(STATS):
            merge()
    except TimeoutError:
        merge()


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


def lineage(pid, parent):
    """The ancestors of pid by parent ({child pid: parent pid})."""
    out = set()
    while (pid := parent.get(pid, 0)) > 0 and pid not in out:
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
    """(path, MB) of the run-*.scope cgroup pid runs in: memory.current less memory.stat's `file` without its `shmem`
    (tmpfs files such as /tmp's, which stay), the page cache the kernel can reclaim, at least 0; MB None without
    memory.current; None outside such a scope."""
    try:
        path = CGROUP / (PROC / str(pid) / 'cgroup').read_text().split('::', 1)[1].strip().lstrip('/')
    except (OSError, IndexError):
        return None
    if not re.fullmatch(r'run-.+\.scope', path.name):
        return None
    try:
        current = int((path / 'memory.current').read_text())
    except (OSError, ValueError):
        return path, None
    try:
        text = (path / 'memory.stat').read_text()
        field = lambda name: int(m[1]) if (m := re.search(rf'^{name} (\d+)$', text, re.M)) else 0
        cache = max(0, field('file') - field('shmem'))
    except (OSError, ValueError):
        cache = 0
    return path, max(0, current - cache) >> 20


def rss_mb(pid):
    try:
        return int((PROC / str(pid) / 'statm').read_text().split()[1]) * os.sysconf('SC_PAGE_SIZE') >> 20
    except (OSError, IndexError, ValueError):
        return 0


def pss_mb(pid):
    """pid's proportional set size (each shared page split among the processes mapping it), else its RSS."""
    try:
        return int(re.search(r'^Pss:\s+(\d+) kB', (PROC / str(pid) / 'smaps_rollup').read_text(), re.M)[1]) >> 10
    except (OSError, TypeError, ValueError):
        return rss_mb(pid)


def usage(pid, tree, runs=()):
    """MB a reservation's run holds now: each run-*.scope its descendants run in, once, except the one pid runs in itself
    (a campaign reserving in its own capped process), plus the PSS of the descendants outside such scopes; a descendant
    in runs (the pids of the other running reservations) and its own descendants count under that reservation instead."""
    own = (scope(pid) or (None,))[0]
    seen, total, todo = set(), 0, [c for c in tree.get(pid, []) if c not in runs]
    while todo:
        p = todo.pop()
        todo += [c for c in tree.get(p, []) if c not in runs]
        s = scope(p)
        if s and s[0] != own and s[1] is not None:
            total += s[1] * (s[0] not in seen)
            seen.add(s[0])
        else:
            total += pss_mb(p)
    return total


def shadow(spare, head, running, grows, used, now):
    """When the head waiter is expected to fit: each running reservation ends at its admission plus its recorded seconds
    (now when unrecorded, as it may), freeing what it holds and may still grow into; the last end when even all of them
    leave it short (it then starts with nothing else running)."""
    short, end = head['need'] - spare, now
    if short <= 0:
        return now
    for end, gain in sorted((e['admit'] + e['secs'] if e.get('secs', 0) > 0 and 'admit' in e else now, grows(e) + used(e)) for e in running):
        short -= gain
        if short <= 0:
            break
    return end


def decide(free, me, entries, grows, mine, above=lambda e: (), used=lambda e: 0, now=None):
    """(admit, warning) for entry me among the live entries: free is MemAvailable in MB, grows(entry) the MB a running
    entry may still use and used(entry) the MB its run holds now, mine the pids of me's ancestors, above(entry) those of
    another entry, now the poll's clock (default me's beat).

    A waiter whose last poll is STALE seconds older than me's holds no place. A run inside a running reservation (its
    ancestor's) does not queue, since the waiters before it may wait for that very reservation, but must fit beside it,
    as a recorded peak covers only its own scope. Behind an older waiter, me starts only by backfill: it fits
    now and its recorded seconds end before the oldest waiter is expected to fit (shadow())."""
    now = me.get('beat', me['since']) if now is None else now
    beat = lambda e: e.get('beat', e['since'])
    order = lambda e: (e['since'], e['pid'])
    running = [e for e in entries if e is not me and e['state'] == 'run']
    waiting = [e for e in entries if e is not me and e['state'] == 'wait' and beat(e) >= beat(me) - STALE]
    inside = [e for e in running if e['pid'] in mine]
    others = [e for e in running if e['pid'] not in mine]
    spare = None if free is None else free - sum(map(grows, running)) - RESERVE_MB
    fits = spare is None or spare >= me['need']
    ahead = [e for e in waiting if order(e) < order(me)]
    if not inside and ahead and not (fits and spare is not None and me.get('secs', 0) > 0
                                     and now + me['secs'] <= shadow(spare, min(ahead, key=order), running, grows, used, now)):
        return False, ''
    if fits:
        return True, ''
    short = f'{free} MB available, {me["need"]} MB needed and {RESERVE_MB} MB kept free'
    if not others:
        held = f'{len(running)} ancestor run(s) hold reservations' if running else 'no other gated run is running'
        return True, f'{short}; {held}, so it runs now'
    queued = [*waiting, me]
    lineage_of = lambda e: mine if e is me else above(e)
    nested = [e for e in queued if any(r['pid'] in lineage_of(e) for r in running)]
    if inside and all(any(r['pid'] in lineage_of(w) for w in queued) for r in running) and min(nested, key=order) is me:
        return True, f'{short}; every running gated run waits for a queued run below it, so the oldest of those runs now'
    return False, ''


@contextlib.contextmanager
def locked(path, wait=None):
    """Hold path's lock file exclusively; TimeoutError once another process held it for wait seconds (LOCK_WAIT)."""
    wait = LOCK_WAIT if wait is None else wait
    with open(path.with_name(path.name + '.lock'), 'a') as lock:
        deadline = None
        while True:
            try:
                fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
                break
            except BlockingIOError:
                deadline = time.monotonic() + wait if deadline is None else deadline
                if time.monotonic() >= deadline:
                    raise TimeoutError(errno.ETIMEDOUT, f'its lock stayed held for {wait:g} s') from None
                time.sleep(LOCK_POLL)
        try:
            yield
        finally:
            fcntl.flock(lock, fcntl.LOCK_UN)


def monotonic(e, offset):
    """e with its times on CLOCK_MONOTONIC: an entry of pre-SUP34 code (a worktree on older code) holds wall-clock times,
    past WALL, which no monotonic reading (seconds since boot) reaches, so offset (time.time() - time.monotonic()) is
    subtracted from them. A suspend since such an entry was written grows offset, moving it earlier by the suspend's length."""
    if e['since'] < WALL:
        return e
    return {**e, **{k: e[k] - offset for k in ('since', 'beat', 'admit') if k in e}}


def load(path):
    """The live entries of the reservations file."""
    try:
        entries = json.loads(path.read_text())
    except (OSError, ValueError):
        return []
    ok = lambda e: (isinstance(e, dict) and all(isinstance(e.get(k), int) for k in ('pid', 'start', 'need'))
                    and all(isinstance(e.get(k, 0.0), (int, float)) for k in ('since', 'beat', 'secs', 'admit')) and 'since' in e
                    and e.get('state') in ('wait', 'run'))
    return [e for e in entries if ok(e) and (stat(e['pid']) or (0, None))[1] == e['start']] if isinstance(entries, list) else []


def save(path, text):
    tmp = path.with_name(f'{path.name}.{os.getpid()}')
    try:
        tmp.write_text(text)
        tmp.replace(path)
    finally:
        tmp.unlink(missing_ok=True)


def mark(state, word):
    """Write word to the CP_MEMORY_GATE_STATE file, if any."""
    if state:
        with contextlib.suppress(OSError):
            Path(state).write_text(word)


def wait(path, me, label, out, until=None, state=None):
    """Block until me is admitted (its entry then runs) and return True; print WAIT once, then ADMIT, and any WARNING.
    Return False, still queued, once the time.monotonic() deadline until has passed."""
    waited = None
    while True:
        with locked(path):
            me['beat'] = time.monotonic()
            entries = [e for e in load(path) if e['pid'] != me['pid']] + [me]
            offset = time.time() - me['beat']  # this poll's monotonic reading
            view = [monotonic(e, offset) if e is not me else me for e in entries]  # decided on one clock, saved as written
            tree = children()
            parent = {c: p for p, cs in tree.items() for c in cs}
            mine = lineage(me['pid'], parent)
            runs = {e['pid'] for e in entries if e['state'] == 'run'}
            used = {pid: usage(pid, tree, runs - {pid}) for pid in runs}
            lines = {e['pid']: lineage(e['pid'], parent) for e in entries if e['state'] == 'wait'}
            admit, warning = decide(available(), me, view, lambda e: max(0, e['need'] - used[e['pid']]), mine,
                                    lambda e: lines[e['pid']], lambda e: used[e['pid']])
            me['state'] = 'run' if admit else 'wait'
            if admit:
                me['admit'] = me['beat']
            save(path, json.dumps(entries))
        if admit:
            break
        if until is not None and time.monotonic() >= until:
            print(f'STOP {label}: its deadline passed in the memory queue', file=out, flush=True)
            return False
        if waited is None:
            waited = time.monotonic()
            mark(state, 'wait')
            print(f'WAIT {label}: needs {me["need"]} MB beside the running gated runs (_00_memory_gate.py); queued', file=out, flush=True)
        time.sleep(POLL)
    if warning:
        print(f'WARNING {label}: memory gate: {warning}', file=out, flush=True)
    if waited is not None:
        print(f'ADMIT {label}: after {time.monotonic() - waited:.0f} s in the memory queue', file=out, flush=True)
    return True


@contextlib.contextmanager
def held(need, label, out=sys.stderr, until=None, secs=0):
    """Wait for admission of a run needing need MB and expected to take secs seconds (0: unknown), hold its reservation
    while the block runs, then release it; a need of 0 or less, or the gate turned off, passes at once. Yields whether it
    was admitted: False only when the time.monotonic() deadline until passed in the queue, and the caller then skips its run."""
    path, state = store(), os.environ.pop('CP_MEMORY_GATE_STATE', None)
    if path is None or need <= 0:
        yield True
        return
    me = {'pid': os.getpid(), 'start': (stat(os.getpid()) or (0, 0))[1], 'need': int(need), 'label': label, 'state': 'wait',
          'since': time.monotonic(), 'secs': secs}
    patience = None
    try:
        try:
            path.parent.mkdir(parents=True, exist_ok=True)
            admitted = wait(path, me, label, out, until, state)
        except OSError as e:  # an unusable reservations file or a lock held too long never stops a run
            print(f'WARNING {label}: memory gate unavailable ({e.strerror}); running without it', file=out, flush=True)
            admitted, patience = None, 0 if isinstance(e, TimeoutError) else None
        mark(state, 'run')
        if not admitted:  # past the deadline, or without the gate: no entry of it stays while the block runs
            release(path, me, patience)
        yield admitted is not False
    finally:  # also after an interrupt while queued
        release(path, me, patience)


def release(path, me, wait=None):
    """Remove me's entry, if saved: one left from a failed or interrupted wait would block every later waiter. After a
    lock timeout it tries the lock once: a dead or stale entry holds no place anyway."""
    with contextlib.suppress(OSError), locked(path, wait):
        entries = load(path)
        if any(e['pid'] == me['pid'] for e in entries):
            save(path, json.dumps([e for e in entries if e['pid'] != me['pid']]))
