#!/usr/bin/env python3
"""Checks for .claude/skills/reap/reap.py (/reap: leftovers of ended Claude Code sessions and stale test temporaries).

quick: classify() on a synthetic process table holding every ownership case, temporaries() on a fixture directory with
every kind of item, scratchpads() on a fixture tree. full: quick plus end-to-end runs of reap.py on real processes owned
by a fake claude (a Python process renamed `claude` with prctl), each confined with --only to its own fixture directory:
an ended session's orphans (plain and in a real systemd scope), a live other session's processes, this session's own
leftovers with and without --own, and --tmp deletion with --dry-run. stress: as full. Modes also come from CP_TEST_MODE.
The real temp directory is never cleaned: end-to-end runs pass --only, and temporaries live in a fixture --tmp.
Oracle: the expected verdict of each synthetic process and item, written by hand from the rules in reap.py's docstring;
liveness read from /proc.

Feature map:
an ended session's related process and its children, a CLAUDE_PID reused by a non-claude process or by a claude started
later -> test_classify, test_e2e_ended, test_e2e_ended_scope (whole-scope cgroup.kill);
another live session: attached processes untouched, reparented ones and tool commands over an hour reported, short
ones silent -> test_classify, test_e2e_live_other;
this session: the reaper's own chain and claude never touched, a non-shell child of claude (hook) never ended,
background shells and reparented processes ended only with --own, otherwise the reparented ones reported ->
test_classify, test_e2e_own;
orphans of other projects' ended sessions and reparented related processes without CLAUDE_PID reported, cwd alone
not enough without CLAUDE_PID; CP_REAP_KEEP never ended, even with an ended session or --own, and reported only when
reparented and over an hour old -> test_classify;
temporaries: grace, references by argument, live workers that may own cp-*, gcc and workdir items but not preambles,
name patterns, the campaign lock, resumable and finished mutation workdirs -> test_temporaries;
a sibling checkout or scratch root whose path only starts with this project's is another project -> test_classify;
--tmp deletion, --dry-run deleting nothing -> test_e2e_tmp;
scratchpads of ended sessions: session-id directories only, main and worktree roots, sibling and other projects
excluded; live by registry (a dead or unreadable entry not), by a live claude's processes (not when the claude is newer
than the process); used by a process; named by a /pause handoff or /finish ledger; idle from the latest change in the
tree or the transcript -> test_scratchpads; reported without --scratchpads, --dry-run, the 24 h default, HOURS 0, a live
claude's registered session kept until it ends, a gone worktree's empty root removed, the main root kept even empty ->
test_e2e_scratchpads; root pruning (main, live worktree's, non-empty and other projects' roots stay) -> test_prune_roots.
"""
import argparse
import importlib.util
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import time
from types import SimpleNamespace
import unittest
import uuid
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
REAP = ROOT / '.claude/skills/reap/reap.py'
spec = importlib.util.spec_from_file_location('reap_under_test', REAP)
reap = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reap)
MODE = 'full'
SNAP = '/h/.claude/shell-snapshots/snapshot-bash-1-x.sh'
# A fake claude: renamed with prctl, it starts an attached Bash-tool-like shell and a reparented sleep (optionally in its
# own scope), records their pids, optionally runs reap.py as its own child, then stays alive until killed.
FAKE = r'''import ctypes, json, os, subprocess, sys, time
spec = json.loads(sys.argv[1])
ctypes.CDLL(None).prctl(15, b'claude')
env = dict(os.environ, CLAUDECODE='1', CLAUDE_PID=str(os.getpid()), CLAUDE_CODE_SESSION_ID=spec['sid'])
env.pop('CP_MEMORY_SUPERVISOR', None)
shell = subprocess.Popen(['bash', '-c', f": {spec['snap']}; sleep 60; true"], cwd=spec['dir'], env=env)
pre = ['systemd-run', '--user', '--scope', '--quiet', '--'] if spec['scope'] else []
launch = ('import subprocess, sys; p = subprocess.Popen(sys.argv[1:], start_new_session=True, stdin=subprocess.DEVNULL, '
          'stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL); print(p.pid, flush=True)')
out = subprocess.run([sys.executable, '-c', launch] + pre + ['sleep', '60'], cwd=spec['dir'], env=env, capture_output=True, text=True).stdout
pids = dict(claude=os.getpid(), shell=shell.pid, detached=int(out.split()[0]))
time.sleep(0.3)
pids['shell_kids'] = [int(x) for x in open(f'/proc/{shell.pid}/task/{shell.pid}/children').read().split()]
open(spec['pids'], 'w').write(json.dumps(pids))
if spec['reap'] is not None:
    r = subprocess.run([sys.executable, spec['reap_py']] + [a.replace('{dir}', spec['dir']) for a in spec['reap']], env=env, capture_output=True, text=True)
    open(spec['out'], 'w').write(r.stdout + r.stderr)
open(spec['pids'] + '.done', 'w').close()
time.sleep(120)
'''


def alive(pid):
    try:
        return Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()[0] != 'Z'
    except OSError:
        return False


def P(pid, ppid, comm='python3', start=1000.0, args=(), cwd='', owner=None, sid='s', keep=False):
    env = ({'CLAUDECODE': '1', 'CLAUDE_PID': str(owner), 'CLAUDE_CODE_SESSION_ID': sid} if owner else {}) | (
        {'CP_REAP_KEEP': '1'} if keep else {})
    return reap.Proc(pid, ppid, comm, start, args=list(args), cwd=cwd, env=env)


class Reap(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='cp-reaptest-'))
        self.addCleanup(shutil.rmtree, self.tmp, True)

    def full(self):
        if MODE == 'quick':
            self.skipTest('full mode only')

    # ---- classification on a synthetic table
    def test_classify(self):
        now, R, M = 100000.0, '/r', (['/r', '/t/claude-1000/-r'], ['/t/cp-', '/t/claude-1000/-r--claude-worktrees-'])
        rows = [
            P(10, 1, 'systemd', 0), P(50, 10, 'bash', 50),
            P(100, 50, 'claude', 100), P(101, 100, 'bash', 101, ['/bin/bash', '-c', f'source {SNAP} && eval x'], R, 100),
            P(102, 101, 'python3', 102, ['python3', 'reap.py'], R, 100),
            P(103, 100, 'bash', 103, ['/bin/bash', '-c', f'source {SNAP} && eval y'], R, 100),
            P(104, 103, 'python3', 104, [f'{R}/a.py'], R, 100),
            P(105, 10, 'drv', 105, ['/t/cp-x/drv'], '/h', 100),
            P(106, 100, 'python3', 106, [f'{R}/.claude/hooks/h.py'], R, 100),
            P(200, 10, 'claude', 200), P(201, 200, 'bash', 201, ['/bin/bash', '-c', f'source {SNAP}'], R, 200, 'B'),
            P(202, 201, 'python3', now - 7200, [f'{R}/t.py'], R, 200, 'B'), P(203, 201, 'python3', now - 60, [f'{R}/t.py'], R, 200, 'B'),
            P(204, 10, 'python3', 204, ['x'], R, 200, 'B'), P(205, 10, 'python3', 205, ['x'], '/elsewhere', 200, 'B'),
            P(300, 10, 'python3', 300, ['/t/claude-1000/-r--claude-worktrees-W/s/drv2'], '/', 999, 'dead'),
            P(301, 300, 'drv2', 301, ['drv2'], f'{R}/.claude/worktrees/W', 999, 'dead'),
            P(302, 10, 'python3', 302, ['x'], R, 400), P(400, 10, 'bash', 1, ['bash']),
            P(303, 10, 'python3', 150, ['x'], R, 200),
            P(304, 10, 'node', 304, ['node'], '/other', 998), P(305, 304, 'node', 305, ['node'], '/other', 998),
            P(306, 10, 'python3', 306, [f'{R}/96-Local Testing/t.py']), P(307, 10, 'code', 307, ['code'], R),
            P(308, 50, 'python3', 308, [f'{R}/t.py']),
            P(309, 10, 'python3', now - 60, [f'{R}/scan.py'], R, 999, keep=True), P(310, 309, 'claude', now - 60),
            P(311, 10, 'python3', now - 7200, [f'{R}/scan.py'], R, 100, keep=True), P(312, 311, 'python3', now - 7000, [], R, 100, keep=True),
            P(313, 10, 'python3', 313, ['/r2/t.py', '-DX=/r2/h.hpp'], '/r2', 999),  # a sibling checkout /r2 is another project
            P(314, 10, 'python3', 314, ['/t/claude-1000/-r2/s/drv'], '/', 999), P(315, 10, 'drv', 315, ['/r'], '/', 999),
        ]
        table = {p.pid: p for p in rows}
        end, report = reap.classify(table, 102, M, now)
        self.assertEqual(sorted(end), [300, 301, 302, 303, 315])
        self.assertEqual(sorted(p for p, _ in report), [105, 202, 204, 304, 306, 311, 313, 314])
        why = dict(report)
        self.assertIn('--own', why[105])
        self.assertIn('live session B', why[202])
        self.assertIn('2h00m', why[202])
        self.assertIn('reparented away from live session B', why[204])
        self.assertIn('another project', why[304])
        self.assertIn('outside Claude', why[306])
        self.assertIn('detached by design (CP_REAP_KEEP), running for 2h00m', why[311])
        self.assertIn('dead', end[300])
        end, report = reap.classify(table, 102, M, now, own=True)
        self.assertEqual(sorted(end), [103, 104, 105, 300, 301, 302, 303, 315])
        self.assertEqual(sorted(p for p, _ in report), [202, 204, 304, 306, 311, 313, 314])
        self.assertIn('background', end[104])
        self.assertIn('reparented', end[105])

    # ---- temporaries on a fixture directory
    def item(self, name, mtime=1e9, folder=False, body=b'x' * 100):
        p = self.tmp / name
        if folder:
            p.mkdir()
            (p / 'f').write_bytes(body)
            os.utime(p / 'f', (mtime, mtime))
        else:
            p.write_bytes(body)
        os.utime(p, (mtime, mtime))
        return p

    def test_temporaries(self):
        now = time.time()
        co = self.tmp / 'checkout'
        co.mkdir()
        for h in 'h1', 'h2', 'h3':
            (co / h).write_text(h)
        blobs, recs = {'h1': 'b1', 'h2': 'b3', 'h3': 'b4'}, {'h3': ('b4', 0)}
        mut = SimpleNamespace(current=lambda root, h: blobs[h], record=lambda root, h: recs.get(h))
        def work(name, header, blob):
            p = self.item(name, folder=True)
            (p / 'campaign.json').write_text(json.dumps(dict(header=header, blob=blob)))
            os.utime(p / 'campaign.json', (1e9, 1e9))
            os.utime(p, (1e9, 1e9))
        self.item('cp-contest-aaaa1111', folder=True)
        self.item('cp-contest-fresh1234', mtime=now, folder=True)
        self.item('cp-contest-used5678', folder=True)
        self.item('ccABCDEF.s')
        self.item('ccXY.s')
        self.item('preamble-abc123.pch')
        self.item('cp-mutation.lock')
        self.item('other-file')
        work('cp-mutation-x-abcdefgh', 'h1', 'b1')      # resumable: checkout at its blob, no record
        work('cp-mutation-y-abcdefg2', 'h2', 'b2')      # the header moved on
        work('cp-mutation-z-abcdefg3', 'h3', 'b4')      # complete record of its blob
        user = P(1, 0, 'python3', now, [f'{self.tmp}/cp-contest-used5678/checked'])
        got = {Path(p).name: k for k, p, _ in reap.temporaries(str(self.tmp), {1: user}, now, 60, [co], mut)}
        self.assertEqual(got, {'cp-contest-aaaa1111': 'cp', 'ccABCDEF.s': 'gcc', 'preamble-abc123.pch': 'pch',
                               'cp-mutation-y-abcdefg2': 'workdir', 'cp-mutation-z-abcdefg3': 'workdir'})
        sizes = {Path(p).name: n for _, p, n in reap.temporaries(str(self.tmp), {1: user}, now, 60, [co], mut)}
        self.assertEqual((sizes['cp-contest-aaaa1111'], sizes['ccABCDEF.s']), (100, 100))
        worker = P(2, 0, 'cc1plus', 5e8)  # started before every item changed: it may own them
        got = {Path(p).name for _, p, _ in reap.temporaries(str(self.tmp), {1: user, 2: worker}, now, 60, [co], mut)}
        self.assertEqual(got, {'preamble-abc123.pch'})
        claude = P(3, 0, 'claude', 5e8)  # claude itself is no worker
        got = {Path(p).name for _, p, _ in reap.temporaries(str(self.tmp), {1: user, 3: claude}, now, 60, [co], mut)}
        self.assertIn('cp-contest-aaaa1111', got)
        got = {Path(p).name for _, p, _ in reap.temporaries(str(self.tmp), {1: user}, now, 1e9, [co], mut)}
        self.assertEqual(got, set(), 'a grace longer than every age keeps everything')

    def pads(self, roots, home, mtime=1e9, deep=None):
        """Session directories (root, id) with one 10-byte file two levels down, all changed at mtime except that file at
        deep when given; returns {id: path}."""
        base, out = self.tmp / f'claude-{os.getuid()}', {}
        for root, sid in roots:
            d = base / root / sid
            (d / 'scratchpad').mkdir(parents=True)
            (d / 'scratchpad/f').write_bytes(b'y' * 10)
            for x in d / 'scratchpad/f', d / 'scratchpad', d:
                t = deep if deep is not None and x.name == 'f' else mtime
                os.utime(x, (t, t))
            out[sid] = d
        return out

    def test_scratchpads(self):
        home, co, now = self.tmp / 'home', self.tmp / 'co', time.time()
        sid = {k: str(uuid.UUID(int=i)) for i, k in enumerate(
            'ended reg dead env stale_env used named ledger wt sib other recent_t recent_f'.split(), 1)}
        roots = [('-r', sid[k]) for k in ('ended', 'reg', 'dead', 'env', 'stale_env', 'used', 'named', 'ledger', 'recent_t')] + [
            ('-r--claude-worktrees-W', sid['wt']), ('-r2', sid['sib']), ('-other', sid['other'])]
        pads = self.pads(roots, home)
        pads |= self.pads([('-r', sid['recent_f'])], home, deep=now - 60)
        (self.tmp / f'claude-{os.getuid()}/-r/tasks-old').mkdir()  # not a session id: never touched
        (self.tmp / f'claude-{os.getuid()}/-r/loose.log').write_text('x')
        (home / '.claude/sessions').mkdir(parents=True)
        (home / '.claude/sessions/7.json').write_text(json.dumps(dict(pid=7, sessionId=sid['reg'])))
        (home / '.claude/sessions/9.json').write_text(json.dumps(dict(pid=9, sessionId=sid['dead'])))  # pid 9 is gone
        (home / '.claude/sessions/8.json').write_text('{broken')
        (home / '.claude/projects/-r').mkdir(parents=True)
        (home / f".claude/projects/-r/{sid['recent_t']}.jsonl").write_text('{}')  # transcript changed now
        for d, f, text in ('pause', 'k.md', f"see /tmp/claude-1/-r/{sid['named']}/scratchpad/x"), ('finish', 'k.txt', f"1 | {sid['ledger']}"):
            (co / '.claude' / d).mkdir(parents=True)
            (co / '.claude' / d / f).write_text(text)
        table = {7: P(7, 1, 'claude', 1), 8: P(8, 1, 'claude', 1),
                 10: P(10, 7, 'bash', 2, owner=7, sid=sid['env']),
                 11: P(11, 1, 'bash', 0.5, owner=8, sid=sid['stale_env']),  # its claude started after it: pid reused
                 12: P(12, 1, 'drv', 3, [f"{pads[sid['used']]}/scratchpad/drv"])}
        got = {d.name: (n, idle) for d, n, idle in reap.scratchpads(str(self.tmp), '-r', table, home, [co], now)}
        want = {sid[k] for k in ('ended', 'dead', 'stale_env', 'wt', 'recent_t', 'recent_f')}
        self.assertEqual(set(got), want, {k: v in got for k, v in sid.items()})
        self.assertEqual({got[x][0] for x in want}, {10})
        self.assertGreater(got[sid['ended']][1], now - 1e9 - 5)
        self.assertLess(got[sid['recent_t']][1], 30, 'the transcript counts as activity')
        self.assertTrue(55 <= got[sid['recent_f']][1] < 120, 'a file deep in the scratchpad counts as activity')

    # ---- end to end on real processes
    def fake(self, scope=False, run=None):
        """A live fake claude with its children; returns (process, pids, reap output or None)."""
        d = self.tmp / f'cp-reaptest-{uuid.uuid4().hex[:8]}'
        d.mkdir()
        spec = dict(sid=str(uuid.uuid4()), snap=SNAP, dir=str(d), scope=scope, pids=str(d / 'pids.json'), reap=run,
                    reap_py=str(REAP), out=str(d / 'out'))
        proc = subprocess.Popen([sys.executable, '-c', FAKE, json.dumps(spec)], start_new_session=True)
        self.addCleanup(proc.wait)  # cleanups run last first: the kills below come before this wait
        self.addCleanup(lambda: os.killpg(proc.pid, signal.SIGKILL) if proc.poll() is None else None)
        done, deadline = Path(spec['pids'] + '.done'), time.monotonic() + 60
        while not done.exists() and time.monotonic() < deadline and proc.poll() is None:
            time.sleep(0.05)
        self.assertTrue(done.exists(), f'fixture premise: the fake claude started its children; exit {proc.poll()}')
        pids = json.loads(Path(spec['pids']).read_text())
        everyone = [pids['claude'], pids['shell'], pids['detached']] + pids['shell_kids']
        self.addCleanup(lambda: [os.kill(p, signal.SIGKILL) for p in everyone if alive(p)])
        self.assertTrue(all(map(alive, everyone if run is None else [pids['claude']])), 'fixture premise: everything runs')
        return proc, pids, d.name, (Path(spec['out']).read_text() if run is not None else None)

    def run_reap(self, *args):
        r = subprocess.run([sys.executable, str(REAP), *args], capture_output=True, text=True, timeout=120)
        self.assertEqual(r.returncode, 0, r.stdout + r.stderr)
        return r.stdout

    def wait_gone(self, pids):
        deadline = time.monotonic() + 10
        while any(map(alive, pids)) and time.monotonic() < deadline:
            time.sleep(0.05)
        return [p for p in pids if alive(p)]

    def ended(self, scope):
        self.full()
        if scope and not _00_memory_cap.available():
            self.skipTest('SKIP: no systemd user scope')
        proc, pids, marker, _ = self.fake(scope=scope)
        cgroup = Path(f"/proc/{pids['detached']}/cgroup").read_text().split('::', 1)[1].strip()
        os.kill(pids['claude'], signal.SIGKILL)  # the session ends; its processes stay
        proc.wait(10)
        victims = [pids['shell'], pids['detached']] + pids['shell_kids']
        self.assertTrue(all(map(alive, victims)), 'fixture premise: the session left them running')
        dry = self.run_reap('--dry-run', '--only', marker)
        self.assertIn('WOULD END', dry)
        self.assertTrue(all(map(alive, victims)), '--dry-run ends nothing')
        out = self.run_reap('--only', marker)
        self.assertEqual(out.count('ENDED'), 2, out)  # the shell tree and the reparented sleep, one line each
        self.assertIn('ended', out)
        self.assertEqual(self.wait_gone(victims), [], out)
        if scope:
            self.assertRegex(cgroup, r'/run-[^/]+\.scope$')
            path = Path('/sys/fs/cgroup', cgroup.lstrip('/'))
            self.assertFalse(path.exists() and (path / 'cgroup.procs').read_text().split(), 'the orphaned scope is empty')

    def test_e2e_ended(self):
        self.ended(scope=False)

    def test_e2e_ended_scope(self):
        self.ended(scope=True)

    def test_e2e_live_other(self):
        self.full()
        _, pids, marker, _ = self.fake()
        for args in ('--only', marker), ('--own', '--only', marker):  # --own concerns the reaper's session, not this one
            out = self.run_reap(*args)
            self.assertNotIn('ENDED', out)
            self.assertIn(f"pid {pids['detached']} ", out)
            self.assertIn('reparented away from live session', out)
        self.assertTrue(all(map(alive, [pids['claude'], pids['shell'], pids['detached']] + pids['shell_kids'])))

    def test_e2e_own(self):
        self.full()
        _, pids, _, out = self.fake(run=['--only', '{dir}'])
        self.assertNotIn('ENDED', out)
        self.assertIn('reparented away from this session (--own ends it)', out)
        self.assertTrue(all(map(alive, [pids['shell'], pids['detached']])))
        _, pids, _, out = self.fake(run=['--own', '--only', '{dir}'])
        self.assertEqual(out.count('ENDED'), 2, out)
        self.assertIn('in a background shell', out)
        self.assertIn('(reparented)', out)
        self.assertEqual(self.wait_gone([pids['shell'], pids['detached']] + pids['shell_kids']), [], out)
        self.assertTrue(alive(pids['claude']), 'the session itself stays')

    def test_prune_roots(self):
        base, root = self.tmp / f'claude-{os.getuid()}', self.tmp / 'root'
        (root / '.claude/worktrees/W').mkdir(parents=True)
        for name in '-r', '-r--claude-worktrees-W', '-r--claude-worktrees-X', '-r--claude-worktrees-Y', '-r2':
            (base / name).mkdir(parents=True)
        (base / '-r--claude-worktrees-Y/f').write_text('x')
        reap.prune_roots(str(self.tmp), '-r', root)
        self.assertEqual(sorted(x.name for x in base.iterdir()), ['-r', '-r--claude-worktrees-W', '-r--claude-worktrees-Y', '-r2'],
                         "only the empty root of a gone worktree goes: never the main one, a live worktree's, a full one or another project's")

    def test_e2e_scratchpads(self):
        self.full()
        slug = __import__('re').sub(r'[^A-Za-z0-9]', '-', str(ROOT))
        fx, home = self.tmp / 'fx', self.tmp / 'home'
        fx.mkdir()
        self.tmp, kept = fx, self.tmp
        old, new, reg, wt = (str(uuid.uuid4()) for _ in range(4))
        gone_wt = f'{slug}--claude-worktrees-reaptest-{uuid.uuid4().hex[:8]}'  # no such worktree
        pads = self.pads([(slug, old), (slug, reg), (gone_wt, wt)], home, mtime=time.time() - 3 * 86400)
        pads |= self.pads([(slug, new)], home, mtime=time.time() - 3600)
        self.tmp = kept
        proc, pids, _, _ = self.fake()  # a live claude whose registry entry names reg
        (home / '.claude/sessions').mkdir(parents=True)
        (home / f".claude/sessions/{pids['claude']}.json").write_text(json.dumps(dict(sessionId=reg)))
        args = ('--tmp', str(fx), '--home', str(home), '--only', 'no-such-process')
        out = self.run_reap(*args)
        self.assertIn('LEFT 3 scratchpads of ended sessions', out)
        self.assertIn('--scratchpads deletes them', out)
        out = self.run_reap('--dry-run', '--scratchpads', *args)
        self.assertIn('WOULD DELETE 2 scratchpads', out)
        self.assertTrue(all(p.exists() for p in pads.values()), '--dry-run deletes nothing')
        out = self.run_reap('--scratchpads', *args)  # default: idle 24 h
        self.assertIn('DELETED 2 scratchpads', out)
        self.assertIn('LEFT 1 scratchpads of ended sessions, 0 MB', out)
        self.assertIn('idle under 24 h', out)
        self.assertEqual({k: p.exists() for k, p in pads.items()}, {old: False, reg: True, wt: False, new: True})
        self.assertFalse(pads[wt].parent.exists(), "a removed worktree's empty scratch root goes")
        self.assertTrue(pads[old].parent.exists(), "the main checkout's scratch root stays")
        out = self.run_reap('--scratchpads', '0', *args)
        self.assertIn('DELETED 1 scratchpads', out)
        self.assertEqual((pads[new].exists(), pads[reg].exists()), (False, True), 'a live session keeps its scratchpad')
        os.kill(pids['claude'], signal.SIGKILL)
        proc.wait(10)
        out = self.run_reap('--scratchpads', '0', *args)
        self.assertIn('DELETED 1 scratchpads', out)
        self.assertFalse(pads[reg].exists(), 'its session ended')
        self.assertEqual(list(pads[reg].parent.iterdir()), [], "the main checkout's scratch root stays, even empty")

    def test_e2e_tmp(self):
        self.full()
        fx = self.tmp / 'fx'
        fx.mkdir()
        self.tmp, kept = fx, self.tmp
        old = [self.item('cp-contest-old11111', folder=True, mtime=1e8), self.item('ccQWERTY.o', mtime=1e8)]
        fresh = self.item('cp-contest-new22222', folder=True, mtime=time.time())
        self.tmp = kept
        out = self.run_reap('--dry-run', '--tmp', str(fx), '--only', 'no-such-process')
        self.assertIn('WOULD DELETE 1 cp-* test temporaries', out)
        self.assertTrue(all(p.exists() for p in old))
        out = self.run_reap('--tmp', str(fx), '--only', 'no-such-process')
        self.assertIn('DELETED 1 gcc intermediates', out)
        self.assertEqual([p.exists() for p in old + [fresh]], [False, False, True], out)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')), help='unused: no randomness')
    MODE = parser.parse_args().mode
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(Reap)
    print(f'reap checks: mode={MODE} tests={suite.countTestCases()}', flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
