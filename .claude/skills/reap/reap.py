#!/usr/bin/env python3
"""End what ended Claude Code sessions of this project left running, and delete stale test temporaries (/reap).

Processes (this user's only). Claude Code puts CLAUDE_PID, its session's claude pid, into the environment of every process
a session starts, so ownership survives reparenting. A process is related to this project when its cwd, executable or an
argument lies under the main checkout (worktrees included), a scratch root of this project (the main checkout's or a
worktree's), or the temp cp-* tree; a sibling checkout whose path merely starts with this one's is not.
- ended: a related process whose CLAUDE_PID names no live claude process started before it (its session ended). A capped
  scope (run-*.scope) holding only such processes ends at once through cgroup.kill; the rest get SIGTERM, then SIGKILL
  after 5 s. With --own also this session's leftovers: what runs under its other Bash tool shells (background tasks) and
  its processes reparented away from it. Run --own only when nothing of this session should keep running.
- reported, never ended: another live session's related processes reparented away from it or running a tool command
  for over an hour, orphans of other projects' ended sessions, related reparented processes without CLAUDE_PID, and
  processes detached by design (CP_REAP_KEEP=1 in their environment, such as the harness audit scan) once reparented
  and running for over an hour.
Temporaries directly in the temp directory (this user's, unchanged for --grace minutes, named by no process's cwd,
executable, open file, mapping or argument) are deleted: clangd's preamble-*.pch; cp-* test files and directories, gcc's
cc??????.* files and cp-mutation-<stem>-* workdirs when no live worker (a compiler, a process of a Claude session, or one
naming 96/97 testing) started before their last change, so none can own them, a workdir only when no checkout can resume
it (none holds the header at the campaign's blob without a complete record of it).
Scratchpads: the session directories under this project's scratch roots (temp/claude-<uid>/<slug>, and <slug>--claude-
worktrees-<name> for worktree sessions) of sessions that ended are reported; with --scratchpads [HOURS] (default 24;
/package and /finish pass it) those idle for HOURS (last change in the directory or the session's transcript) are deleted, and a worktree's
scratch root left empty once its worktree is gone. A session counts as live when ~/.claude/sessions/<pid>.json names it
for a live claude pid, a claude process holds a file in its directory, or a process of a live claude carries its
CLAUDE_CODE_SESSION_ID; a directory any process uses, or that a /pause handoff or /finish ledger of a checkout names, stays
too. claude --resume works without its scratchpad, which it recreates empty. Loose files beside the scratch roots are
never touched.

Usage: reap.py [--own] [--scratchpads [HOURS]] [--dry-run] [--grace MIN] [--tmp DIR] [--home DIR] [--only TEXT]; --tmp,
--home (for ~/.claude) and --only (act only on processes whose arguments or cwd contain TEXT; without --tmp it also skips
the temp directory) confine a run to a test fixture. Exit 1 when something could not be ended or deleted.
"""
import argparse
from dataclasses import dataclass, field
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import signal
import subprocess
import sys
import tempfile
import time

HZ = os.sysconf('SC_CLK_TCK')
COMPILERS = {'g++', 'g++-14', 'gcc', 'c++', 'cc1', 'cc1plus', 'as', 'ld', 'collect2', 'lto1', 'lto-wrapper', 'ccache', 'mewt'}
SHELL = '/shell-snapshots/snapshot-'  # every Bash tool shell sources one
WORKDIR = re.compile(r'cp-mutation-.+-[a-z0-9_]{8}')
GCC = re.compile(r'cc[A-Za-z0-9]{6}\.\w+')
PCH = re.compile(r'preamble-[0-9a-f]+\.pch')
UUID = r'[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}'
SESSION = re.compile(rf'/claude-\d+/[^/]+/({UUID})(?:/|$)')


@dataclass
class Proc:
    pid: int
    ppid: int
    comm: str
    start: float  # epoch seconds
    cpu: float = 0.0
    state: str = 'S'
    args: list = field(default_factory=list)
    cwd: str = ''
    exe: str = ''
    env: dict = field(default_factory=dict)  # the CLAUDE* variables and CP_REAP_KEEP
    cgroup: str = ''

    @property
    def owner(self):
        return int(self.env.get('CLAUDE_PID') or 0)


def main_checkout(here):
    """The main checkout of the repository holding here (a worktree's included)."""
    r = subprocess.run(['git', '-C', str(here), 'rev-parse', '--path-format=absolute', '--git-common-dir'], capture_output=True, text=True)
    return Path(r.stdout.strip()).parent if r.returncode == 0 else Path(here).resolve().parents[2]


def boot_time():
    return next(int(line.split()[1]) for line in open('/proc/stat') if line.startswith('btime '))


def link(path):
    try:
        return os.readlink(path).removesuffix(' (deleted)')
    except OSError:
        return ''


def read(pid, boot):
    """This user's process pid, or None."""
    base = f'/proc/{pid}'
    try:
        if os.stat(base).st_uid != os.getuid():
            return None
        stat = Path(base, 'stat').read_bytes().decode(errors='replace')
        f = stat[stat.rindex(')') + 2:].split()
        env = Path(base, 'environ').read_bytes().decode(errors='replace').split('\0')
        cgroup = Path(base, 'cgroup').read_text()
        args = Path(base, 'cmdline').read_bytes().decode(errors='replace').split('\0')[:-1]
    except (OSError, ValueError):
        return None
    return Proc(pid, int(f[1]), stat[stat.index('(') + 1:stat.rindex(')')], boot + int(f[19]) / HZ, (int(f[11]) + int(f[12])) / HZ,
                f[0], args, link(base + '/cwd'), link(base + '/exe'), dict(v.split('=', 1) for v in env if v.startswith(('CLAUDE', 'CP_REAP_KEEP=')) and '=' in v),
                cgroup.split('::', 1)[1].strip() if '::' in cgroup else '')


def processes():
    boot = boot_time()
    return {p.pid: p for p in (read(int(d), boot) for d in os.listdir('/proc') if d.isdigit()) if p and p.state != 'Z'}


def chain(table, pid):
    """pid's ancestors in table, nearest first."""
    out, seen = [], {pid}
    while pid in table and (pid := table[pid].ppid) in table and pid not in seen:
        out.append(pid)
        seen.add(pid)
    return out


def related(p, markers, cwd=True):
    """markers = (directories, name prefixes): a path equals or lies under a directory, or contains a prefix."""
    dirs, prefixes = markers
    return any(s == d or d + '/' in s for d in dirs for s in (p.args + [p.exe] + [p.cwd] * cwd)) or any(
        m in s for m in prefixes for s in (p.args + [p.exe] + [p.cwd] * cwd))


def classify(table, me, markers, now, own=False):
    """(end, report): end maps pid to its reason; report lists (pid, reason) for what stays."""
    mine = chain(table, me) + [me]
    session = table[me].owner if me in table else 0
    my_shell = next((q for q in mine if table.get(q) and table[q].ppid == session), None)
    end, report = {}, []
    for p in table.values():
        if p.pid in mine or p.comm == 'claude':
            continue
        up, owner = chain(table, p.pid), p.owner
        parent = table.get(p.ppid)
        reparented = p.ppid == 1 or (parent is not None and parent.comm == 'systemd')
        if p.env.get('CP_REAP_KEEP'):
            if reparented and now - p.start >= 3600:
                report.append((p.pid, f'detached by design (CP_REAP_KEEP), running for {span(now - p.start)}'))
            continue
        if not owner:
            if reparented and related(p, markers, cwd=False):
                report.append((p.pid, 'reparented, started outside Claude Code'))
            continue
        host = table.get(owner)
        if host is None or host.comm != 'claude' or host.start > p.start:
            if related(p, markers):
                end[p.pid] = f"its session {p.env.get('CLAUDE_CODE_SESSION_ID', '?')[:8]} ended"
            elif reparented:
                report.append((p.pid, 'orphan of an ended session of another project'))
            continue
        shell = next((q for q in [p.pid] + up if table[q].ppid == owner), None)
        background = shell is not None and shell != my_shell and SHELL in ' '.join(table[shell].args)
        if owner == session:
            if own and (owner not in up or background):
                end[p.pid] = 'left by this session' + (' (reparented)' if owner not in up else ' in a background shell')
            elif owner not in up and related(p, markers):
                report.append((p.pid, 'reparented away from this session (--own ends it)'))
        elif related(p, markers):
            sid = p.env.get('CLAUDE_CODE_SESSION_ID', '?')[:8]
            if owner not in up:
                report.append((p.pid, f'reparented away from live session {sid}'))
            elif parent is not None and SHELL in ' '.join(parent.args) and now - p.start >= 3600:
                report.append((p.pid, f'tool command of live session {sid} running for {span(now - p.start)}'))
    return end, report


def span(s):
    s = int(s)
    return f'{s // 86400}d{s % 86400 // 3600}h' if s >= 86400 else f'{s // 3600}h{s % 3600 // 60:02d}m' if s >= 3600 else f'{s // 60}m{s % 60:02d}s'


def gone(pid, start, boot):
    try:
        f = Path(f'/proc/{pid}/stat').read_text()
        f = f[f.rindex(')') + 2:].split()
        return f[0] == 'Z' or boot + int(f[19]) / HZ != start
    except (OSError, ValueError):
        return True


def stop(table, pids, dry):
    """End pids: each wholly orphaned capped scope through cgroup.kill, the rest SIGTERM then SIGKILL. Returns the survivors."""
    if dry or not pids:
        return set()
    boot, left = boot_time(), set(pids)
    for cg in {table[p].cgroup for p in pids}:
        path = Path('/sys/fs/cgroup', cg.lstrip('/'))
        if re.fullmatch(r'run-.+\.scope', path.name):
            try:
                members = {int(x) for x in (path / 'cgroup.procs').read_text().split()}
                if members and members <= set(pids):
                    (path / 'cgroup.kill').write_text('1')
            except (OSError, ValueError):
                pass
    for sig, wait in (signal.SIGTERM, 5), (signal.SIGKILL, 2):
        for p in list(left):
            if gone(p, table[p].start, boot):
                left.discard(p)
            else:
                try:
                    os.kill(p, sig)
                except ProcessLookupError:
                    left.discard(p)
                except PermissionError:
                    pass
        deadline = time.monotonic() + wait
        while left and time.monotonic() < deadline:
            left = {p for p in left if not gone(p, table[p].start, boot)}
            time.sleep(0.05)
    return left


def paths(p):
    """The paths process p uses: cwd, executable, arguments, open files and mappings."""
    out = [p.cwd, p.exe] + p.args
    try:
        out += [link(f'/proc/{p.pid}/fd/{fd}') for fd in os.listdir(f'/proc/{p.pid}/fd')]
        out += [x[5].strip() for line in open(f'/proc/{p.pid}/maps') if len(x := line.split(None, 5)) == 6]
    except OSError:
        pass
    return out


def references(table, tmp):
    """Names directly in tmp that some process uses."""
    pre, used = tmp.rstrip('/') + '/', set()
    for p in table.values():
        for s in paths(p):
            for m in re.finditer(re.escape(pre) + r'([^/\s=:,]+)', s):
                used.add(m[1].removesuffix(' (deleted)'))
    return used


def newest(path):
    """The latest change of path and its direct entries."""
    t = os.lstat(path).st_mtime
    if os.path.isdir(path) and not os.path.islink(path):
        with os.scandir(path) as it:
            for e in it:
                try:
                    t = max(t, e.stat(follow_symlinks=False).st_mtime)
                except OSError:
                    pass
    return t


def tree(path):
    """(bytes, latest change) of path and everything under it."""
    st = os.lstat(path)
    total, last = st.st_size * (not os.path.isdir(path)), st.st_mtime
    if os.path.isdir(path) and not os.path.islink(path):
        for r, dirs, files in os.walk(path):
            for f in dirs + files:
                try:
                    st = os.lstat(os.path.join(r, f))
                except OSError:
                    continue
                total, last = total + st.st_size * (f in files), max(last, st.st_mtime)
    return total, last


def size(path):
    return tree(path)[0]


def resumable(work, checkouts, mutation):
    """True when some checkout holds the campaign's header at its blob without a complete record of that blob."""
    try:
        meta = json.loads((Path(work) / 'campaign.json').read_text())
    except (OSError, ValueError):
        return False
    for root in checkouts:
        if (root / meta['header']).is_file() and mutation.current(root, meta['header']) == meta['blob']:
            rec = mutation.record(root, meta['header'])
            if not (rec and rec[0] == meta['blob'] and rec[1] == 0):
                return True
    return False


def temporaries(tmp, table, now, grace, checkouts, mutation):
    """(kind, path, bytes) of every stale temporary directly in tmp."""
    used = references(table, tmp)
    workers = [p.start for p in table.values() if p.comm != 'claude' and (
        p.comm in COMPILERS or p.owner or any(t in ' '.join(p.args) for t in ('96-Local Testing', '97-Online Testing')))]
    first = min(workers, default=float('inf'))
    out = []
    for e in os.scandir(tmp):
        try:
            if e.stat(follow_symlinks=False).st_uid != os.getuid() or e.name in used:
                continue
            kind = ('workdir' if e.is_dir(follow_symlinks=False) and WORKDIR.fullmatch(e.name) else
                    'cp' if e.name.startswith('cp-') and e.name != 'cp-mutation.lock' else
                    'gcc' if GCC.fullmatch(e.name) else 'pch' if PCH.fullmatch(e.name) else None)
            if kind is None or now - (changed := newest(e.path)) < grace * 60:
                continue
            if kind != 'pch' and first <= changed or kind == 'workdir' and resumable(e.path, checkouts, mutation):
                continue
            out.append((kind, e.path, size(e.path)))
        except OSError:
            continue
    return out


def live_sessions(table, home):
    """Ids of the sessions alive now: registered for a live claude pid, holding a file open, or carried by a live
    claude's processes."""
    live = set()
    for p in table.values():
        if p.comm == 'claude':
            live |= {m[1] for s in paths(p) if (m := SESSION.search(s))}
            try:
                live.add(json.loads(Path(home, '.claude/sessions', f'{p.pid}.json').read_text())['sessionId'])
            except (OSError, ValueError, KeyError, TypeError):
                pass
        host = table.get(p.owner)
        if p.env.get('CLAUDE_CODE_SESSION_ID') and host is not None and host.comm == 'claude' and host.start <= p.start:
            live.add(p.env['CLAUDE_CODE_SESSION_ID'])
    return live


def scratch_roots(tmp, slug):
    """This project's scratch roots: the main checkout's and each worktree's."""
    base = Path(tmp, f'claude-{os.getuid()}')
    return sorted(b for b in (base.iterdir() if base.is_dir() else [])
                  if b.is_dir() and not b.is_symlink() and (b.name == slug or b.name.startswith(slug + '--claude-worktrees-')))


def prune_roots(tmp, slug, root):
    """Remove each empty scratch root of a worktree that no longer exists; the main checkout's stays."""
    for b in scratch_roots(tmp, slug):
        if b.name != slug and not (Path(root) / '.claude/worktrees' / b.name.removeprefix(slug + '--claude-worktrees-')).exists():
            try:
                b.rmdir()
            except OSError:
                pass


def scratchpads(tmp, slug, table, home, checkouts, now):
    """(path, bytes, idle seconds) of each scratchpad of an ended session of this project that nothing protects."""
    live, used = live_sessions(table, home), set()
    for p in table.values():
        used |= {m[1] for s in paths(p) if (m := SESSION.search(s))}
    named = ''
    for root in checkouts:
        for f in [*Path(root, '.claude/pause').glob('*'), *Path(root, '.claude/finish').glob('*')]:
            try:
                named += f.read_text(errors='replace') if f.is_file() else ''
            except OSError:
                pass
    out = []
    for b in scratch_roots(tmp, slug):
        for d in sorted(b.iterdir()):
            if not re.fullmatch(UUID, d.name) or not d.is_dir() or d.is_symlink() or d.name in live | used or d.name in named:
                continue
            n, last = tree(d)
            for t in Path(home, '.claude/projects').glob(f'*/{d.name}.jsonl'):
                last = max(last, t.stat().st_mtime)
            out.append((d, n, now - last))
    return out


def describe(p, now):
    cmd = ' '.join(p.args) or p.comm
    return f'pid {p.pid} up {span(now - p.start)} cpu {span(p.cpu)}: {cmd[:160]}'


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--own', action='store_true', help="also end this session's background shells and reparented processes")
    ap.add_argument('--dry-run', action='store_true', help='list without ending or deleting')
    ap.add_argument('--grace', type=float, default=60, help='minutes a temporary stays after its last change (default 60)')
    ap.add_argument('--tmp', help='temp directory (default: the system one)')
    ap.add_argument('--only', help='consider only processes whose arguments or cwd contain this text')
    ap.add_argument('--scratchpads', type=float, nargs='?', const=24.0, metavar='HOURS',
                    help="delete ended sessions' scratchpads idle for HOURS (default 24)")
    ap.add_argument('--home', default=str(Path.home()), help='home directory holding .claude (default: yours)')
    a = ap.parse_args(argv)
    root = main_checkout(Path(__file__).resolve().parent)
    slug = re.sub(r'[^A-Za-z0-9]', '-', str(root))
    tmp = str(Path(a.tmp or tempfile.gettempdir()).resolve())
    scratch = os.path.join(tmp, f'claude-{os.getuid()}', slug)
    markers = ([str(root), scratch], [os.path.join(tmp, 'cp-'), scratch + '--claude-worktrees-'])
    table, now = processes(), time.time()
    end, report = classify(table, os.getpid(), markers, now, a.own)
    if a.only:
        keep = {p for p in end if a.only in ' '.join(table[p].args) + table[p].cwd}
        end, report = {p: r for p, r in end.items() if p in keep}, [x for x in report if a.only in ' '.join(table[x[0]].args) + table[x[0]].cwd]
    verb, failed = ('WOULD END', 'WOULD DELETE') if a.dry_run else ('ENDED', 'DELETED'), 0
    for pid in sorted(end):
        if table[pid].ppid not in end:  # one line per tree
            kids = sum(1 for q in end if q != pid and pid in chain(table, q))
            print(f'{verb[0]} {describe(table[pid], now)} ({end[pid]}{f"; +{kids} below it" if kids else ""})')
    for pid in sorted(survivors := stop(table, list(end), a.dry_run)):
        print(f'FAIL could not end {describe(table[pid], now)}')
    failed += len(survivors)
    shown = {pid for pid, _ in report}
    for pid, why in report:
        if table[pid].ppid not in shown:
            print(f'LEFT {describe(table[pid], now)} ({why})')
    checkouts = [root] + sorted(p for p in (root / '.claude/worktrees').glob('*') if (p / '.git').exists())
    spec = importlib.util.spec_from_file_location('reap_mutation', root / '96-Local Testing/04-mutation.py')
    mutation = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mutation)
    stale = temporaries(tmp, processes(), time.time(), a.grace, checkouts, mutation) if a.only is None or a.tmp else []
    freed = {}
    for kind, path, n in stale:
        if not a.dry_run:
            try:
                shutil.rmtree(path) if os.path.isdir(path) and not os.path.islink(path) else os.unlink(path)
            except OSError as e:
                print(f'FAIL could not delete {path}: {e.strerror}')
                failed += 1
                continue
        c = freed.setdefault(kind, [0, 0])
        c[0], c[1] = c[0] + 1, c[1] + n
    names = dict(cp='cp-* test temporaries', gcc='gcc intermediates', pch='clangd preambles', workdir='finished mutation workdirs')
    for kind, (count, n) in sorted(freed.items(), key=lambda x: -x[1][1]):
        print(f'{verb[1]} {count} {names[kind]}, {n >> 20} MB')
    pads = scratchpads(tmp, slug, processes(), a.home, checkouts, time.time()) if a.only is None or a.tmp else []
    hours = a.scratchpads
    gone_pads = [(d, n) for d, n, idle in pads if hours is not None and idle >= hours * 3600]
    for d, n in list(gone_pads):
        if not a.dry_run:
            try:
                shutil.rmtree(d)
            except OSError as e:
                print(f'FAIL could not delete {d}: {e.strerror}')
                failed += 1
                gone_pads.remove((d, n))
    if gone_pads:
        print(f'{verb[1]} {len(gone_pads)} scratchpads of ended sessions, {sum(n for _, n in gone_pads) >> 20} MB')
    if not a.dry_run and hours is not None:
        prune_roots(tmp, slug, root)
    if kept := [(d, n) for d, n, idle in pads if (d, n) not in gone_pads]:
        print(f'LEFT {len(kept)} scratchpads of ended sessions, {sum(n for _, n in kept) >> 20} MB, under '
              f'{Path(tmp, f"claude-{os.getuid()}")} ({"idle under " + format(hours, "g") + " h" if hours is not None else "--scratchpads deletes them"})')
    print(f"{'FAIL' if failed else 'PASS'} reap: {len(end)} processes {'to end' if a.dry_run else 'ended'}, "
          f"{sum(c for c, _ in freed.values())} temporaries, {sum(n for _, n in freed.values()) >> 20} MB, "
          f"{len(gone_pads)} scratchpads {sum(n for _, n in gone_pads) >> 20} MB")
    return 1 if failed else 0


if __name__ == '__main__':
    sys.exit(main())
