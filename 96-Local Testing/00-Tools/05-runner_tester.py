#!/usr/bin/env python3
"""Checks for the parallel suite runner 01-run.py, the build pool parallel() of 02-integration.py, the signal
forwarding of _00_memory_cap.supervise() and the memory admission gate _00_memory_gate.py (SUP33).

Fake suites are tiny Python scripts that sleep fractions of a second and record in a shared fixture directory (RT_DIR)
their start and end instants (time.monotonic, system-wide), pid, mode, seed and environment, holding a 'busy' marker
while they live; a flaky suite fails iff it sees another marker. 01-run.py runs from a copy in the fixture, so it
discovers only the fake suites and keeps its .run-stats.json there.
Every test queues in its own gate file (CP_MEMORY_GATE in its fixture, reserve 0), never the machine's.
quick: output identity at -j 1 and -j 4, first-failure status, rerun alone, the jobs bound, parallel() budget and rerun,
forwarding() handler restore, gate decide(), need_of()/recorded() and held().
full: every test, including a later task stopped after its result restarting without a NOTE, signals, real capped scopes, SIGTERM/SIGHUP sent to a supervisor pid alone (main run and
pre-flight, and to the ad-hoc CLI's supervisor, each in a real scope; explicit SKIP without one), a supervisor's SIGKILL
ending its whole scope (entry point or CLI reporter, and a grandchild), integration main() on a small fake repository, 2
seeded random runs (a task stopped as it finishes may restart without a NOTE), and the gate: decide() (FIFO by since then pid, running entries' remaining growth, the exact
RESERVE boundary, free None, ancestors only and nothing else running admitted with their WARNING text, others running
and short waiting, no queue inside a running ancestor's reservation, a waiter silent over STALE seconds holding no
place, its beat rewritten at every poll), need_of, recorded (hit, resolved path, zero/negative/malformed peak, miss, malformed or missing
stats, a path outside the folder), store() defaults, load() (dead pid, reused pid by start time, malformed entries,
non-list and unreadable files) with stat()/lineage() on a live child, children()/usage() on a fake /proc and cgroup
tree (each run-*.scope below once, never the holder's own, else a descendant's RSS), held() (run entry during the block, released
after a normal exit, an exception and a KeyboardInterrupt while queued (WAIT printed), need <= 0 and the gate off never
waiting or creating the file, the two WARNINGs, an unusable file warning and running, an OSError after the entry was saved removing it before the block, a deadline passed in the queue: STOP, no entry, False;
admitted or need 0 past it: True), every tracked script calling ensure() without force
named as an entry point ensure() caps, mode_of() (--mode first, CP_TEST_MODE and the 'full' default only for an entry
that reads it), the standalone 02-online, 03-notebook and 02-debug_compile testers reading it as the others do (ensure()
on the real file, their own --mode rejecting a bad value, --help showing the exported default, a full run without pre-flight), two real processes on one gate
(B prints WAIT, starts after A's END, then ADMIT), supervise()/ensure() passing need (given, main cap, recorded, 0) and
the CLI's --need/--mb parsing in either order and with `=`, its validation messages and the need it reserves.
SUP34 (full; the decide() parts also quick): a nested run's whole need beside its ancestors' reservations, the
deadlock of two sessions' nests broken for the oldest queued nested run (decide() and two real nests), backfill by recorded
seconds against shadow(), usage() less a scope's reclaimable page cache (tmpfs kept) and by PSS (RSS without smaps_rollup), a nested running
reservation's subtree counted under it alone, a pre-SUP34 wall-clock entry decided on the monotonic clock, lineage(), key() and
stats(), remember() (merge under the lock, 01-run.py's format, malformed files, six concurrent writers), a lock held by a
stopped process (warning, no second wait), the CP_MEMORY_GATE_STATE file, monotonic since and beat, supervise() recording
a passing run's peak as one of its record's last three runs (none when failing, keyless or under CP_RUN_STATS_OWNER, which still reserves
with its recorded seconds; report()'s peak file; end to end in a real
scope, SKIP without one), and 01-run.py merging its records beside another writer's and marking its suites.
Exclusive runs (full and quick; user decision 2026-11-26): decide() for an exclusive entry (it waits for running runs
other than its ancestors and those idle below a waiting exclusive run, passes older ordinary waiters, never older
exclusive ones; a stale one holds no place; its WARNING when memory is short) and for others beside it (a running one
blocks all but its descendants, a waiting one every younger top-level run, backfill included, never a run inside a
running reservation or an older run); held(0, exclusive) entering the gate with `excl`, its WAIT texts and the entry
kept whole; supervise(alone), ensure() running a *_benchmark.py alone, the CLI's --alone; main_checkout() and the
default .run-stats.json of a linked worktree (the main checkout's) unless CP_RUN_STATS names one.
stress: full with --rounds seeded random runs (default 6) of random suite counts, sleeps, failures, jobs and budgets.
Oracles: the expected log rebuilt from the suite parameters (RUN lines in rounds x sorted-path order, each suite's known
text, NOTE before a task whose failed run beside others was rerun, nothing after the first always-failing task, the PASS line); overlaps from a
sweep over the recorded intervals, never wall-clock speedups. Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
Failing tests print the seed, mode and retained fixture directory.

Feature map: 96-Local Testing/Docs/00-parallel.md (Feature-to-Test Map).
"""
import argparse
import contextlib
import errno
import importlib.util
import io
import json
import os
from pathlib import Path
import random
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_cpp_test_runner as CPP
import _00_memory_cap
import _00_memory_gate as GATE

_00_memory_cap.ensure()

LT = Path(__file__).resolve().parents[1]
MODE, SEED, ROUNDS = 'full', 42, 6
QUICK = {'test_order_identity', 'test_first_failure_status', 'test_retry_alone', 'test_jobs_bound',
         'test_parallel_budget', 'test_parallel_rerun_alone', 'test_forwarding_restores_handlers', 'test_gate_decide',
         'test_gate_need_and_recorded', 'test_gate_held', 'test_gate_nested_decide', 'test_gate_backfill', 'test_gate_stats_keys', 'test_gate_exclusive'}
SUITE = r'''import json, os, signal, sys, time
d, env = os.environ['RT_DIR'], os.environ
if P.get('deaf'): signal.signal(signal.SIGINT, signal.SIG_IGN)
if P.get('scope'):
    sys.path.insert(0, d)
    import _00_memory_cap
    _00_memory_cap.ensure()
me = f"{P['name']}.{os.getpid()}"; busy = f"{d}/busy/{me}"
open(busy, 'w').close()
t0, code = time.monotonic(), None
try:
    with open(f"{d}/{me}.start", 'w') as f:
        json.dump(dict(t0=t0, pid=os.getpid(), mode=env.get('CP_TEST_MODE'), seed=env.get('CP_TEST_SEED'),
                       capped='CP_MEMORY_CAPPED' in env, owner=env.get('CP_RUN_STATS_OWNER')), f)
    others = len(os.listdir(f"{d}/busy")) > 1
    while P.get('wait') and not os.path.exists(f"{d}/{P['wait']}.done") and time.monotonic() < t0 + 30: time.sleep(0.02)
    time.sleep(P.get('sleep', 0))
    others |= len(os.listdir(f"{d}/busy")) > 1
    print(P['name'], 'mode', env.get('CP_TEST_MODE'), 'seed', env.get('CP_TEST_SEED'), flush=True)
    print(P['name'], 'stderr line', file=sys.stderr, flush=True)
    if P.get('mem'): print(f"MEMORY peak={P['mem']}MB", flush=True)
    if P.get('queued'):  # its own wait, and a nested run's (another label) that waited inside its reservation
        time.sleep(P['queued']); print(f"ADMIT {os.path.basename(sys.argv[0])}: after {P['queued']} s in the memory queue", flush=True)
        print('ADMIT inner: after 9 s in the memory queue', flush=True)
    if P.get('flaky'): print('alone', not others, flush=True)
    code = P.get('code', 0) or 3 * (P.get('flaky', False) and others)
finally:
    time.sleep(P.get('cleanup', 0))
    t1 = time.monotonic(); os.remove(busy)
    with open(f"{d}/{me}.end", 'w') as f: json.dump(dict(t1=t1, code=code), f)
open(f"{d}/{P['name']}.done", 'w').close()
time.sleep(P.get('linger', 0))  # still running after its result is written, where a stop races its finish
sys.exit(code)
'''
SIG = r'''import json, os, signal, subprocess, sys, time
sys.path.insert(0, os.environ['RT_LT'])
import _00_memory_cap
_00_memory_cap.ensure()
d, mode = os.environ['RT_DIR'], sys.argv[sys.argv.index('--mode') + 1]
def stop(sig, _):
    open(f"{d}/{mode}.sig", 'w').write(str(sig))
    sys.exit(40 + sig)
for s in (signal.SIGTERM, signal.SIGHUP): signal.signal(s, stop)  # also probes SIGHUP inherited as ignored
kid = subprocess.Popen(['sleep', '60']).pid if os.environ.get('RT_KID') else None
with open(f"{d}/tmp", 'w') as f: json.dump(dict(pid=os.getpid(), ppid=os.getppid(), kid=kid), f)
os.replace(f"{d}/tmp", f"{d}/{mode}.start")
time.sleep(float(os.environ['RT_SLEEP']))
open(f"{d}/{mode}.end", 'w').close()
'''
spec = importlib.util.spec_from_file_location('integration_under_test', LT / '02-integration.py')
INTEGRATION = importlib.util.module_from_spec(spec)
spec.loader.exec_module(INTEGRATION)


def overlap(intervals):
    """Most intervals alive at once (ends sort before starts at equal instants)."""
    best = now = 0
    for _, step in sorted([(a, 1) for a, _ in intervals] + [(b, -1) for _, b in intervals]):
        now += step
        best = max(best, now)
    return best


class Fixture:
    def __init__(self, root):
        self.root, self.suites = root, {}
        (root / 'busy').mkdir(parents=True)
        for name in ('01-run.py', '_00_memory_gate.py'):  # 01-run.py merges its records through the gate's remember()
            shutil.copy(LT / name, root / name)

    def suite(self, rel, **P):
        P['name'] = Path(rel).stem.removesuffix('_tester')
        path = self.root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(f'P = {P!r}\n' + SUITE)
        self.suites[rel] = P
        return path

    def add(self, *specs):
        """Suites 01-A/00-s ... in order; returns their names."""
        return [self.suite(f'01-A/{i:02}-s_tester.py', **P).stem.removesuffix('_tester') for i, P in enumerate(specs)]

    def env(self, **extra):
        return {k: v for k, v in os.environ.items() if not k.startswith('CP_TEST_')} | dict(
            RT_DIR=str(self.root), CP_MEMORY_CAPPED='4096', CP_TEST_MEMORY_MB='64') | extra

    def run(self, *args, **env):
        r = subprocess.run([sys.executable, self.root / '01-run.py', '--mode', 'full', '--no-integration', '--memory-mb',
                            '100000', *args], capture_output=True, env=self.env(**env), timeout=120)
        return r.returncode, r.stdout.decode(), r.stderr.decode()

    def recs(self, name='*'):
        out = []
        for s in self.root.glob(f'{name}.*.start'):
            end = s.with_suffix('.end')
            out.append(json.loads(s.read_text()) | dict(name=s.name.split('.')[0], t1=None, code=None)
                       | (json.loads(end.read_text()) if end.exists() else {}))
        return sorted(out, key=lambda r: r['t0'])

    def span(self):
        return overlap([(r['t0'], r['t1']) for r in self.recs()])

    def alone(self, rec):
        return all(r == rec or r['t1'] < rec['t0'] or rec['t1'] < r['t0'] for r in self.recs())

    def expect(self, mode='full', seeds=(42,), notes=(), upto=None):
        """The sequential log; a task in notes first failed beside others (a flaky one saw company) and was rerun alone."""
        order = sorted(self.suites, key=lambda rel: Path(rel).parts)
        tasks = [(rel, self.suites[rel], s) for s in seeds for rel in order][:None if upto is None else upto + 1]
        body = lambda P, s, alone: [f"{P['name']} mode {mode} seed {s}", f"{P['name']} stderr line"] + (
            [f"MEMORY peak={P['mem']}MB"] if P.get('mem') else []) + ([f'alone {alone}'] if P.get('flaky') else [])
        return ''.join((f"NOTE {rel} exited {P.get('code') or 3} beside other suites; the last lines of that run, then its "
                        'rerun alone:\n' + ''.join(f'| {line}\n' for line in body(P, s, False)) if i in notes else '')
                       + f'RUN {rel} mode={mode} seed={s}\n' + ''.join(f'{line}\n' for line in body(P, s, True))
                       for i, (rel, P, s) in enumerate(tasks)) + ('' if upto is not None else
            (f'NOTE {len(notes)} problem(s) to fix, each within a passing run (05-testing.md, Layout and Modes):\n'
             + ''.join(f"  {mode} {tasks[i][0]}: exited {tasks[i][1].get('code') or 3} beside other suites, passed alone\n" for i in sorted(notes)) if notes else '')
            + f'PASS: {len(order)} available suites x {len(seeds)} round(s); see 00-index.md for coverage gaps\n')


class Base(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp(prefix='runner-test-'))
        self.f = Fixture(self.root / 'f')
        self.addCleanup(self.clean)
        # every supervise()/ensure() of a test, in process or below it, queues in the fixture's own memory gate, never the
        # machine's, and admits whatever memory is free (reserve 0); the Gate tests set their own reserve
        gate = patch.dict(os.environ, CP_MEMORY_GATE=str(self.root / 'gate.json'), CP_MEMORY_GATE_RESERVE_MB='0')
        gate.start()
        self.addCleanup(gate.stop)

    def clean(self):
        result = self._outcome.result
        if any(t is self or getattr(t, 'test_case', None) is self for t, _ in result.failures + result.errors):
            print(f'\nREPRO seed={SEED} mode={MODE} rounds={ROUNDS} test={self.id()} fixture={self.root}', file=sys.stderr)
        else:
            shutil.rmtree(self.root)

    def check(self, got, code, out, ctx=''):
        self.assertEqual(got[:2], (code, out), f'{ctx} stderr: {got[2]}')


class Runner(Base):
    def test_order_identity(self):
        for i, rel in enumerate(['01-A/01-a_tester.py', '01-A/02-b_tester.py', '01-A/10-c_tester.py', '02-B/01-d_tester.py']):
            self.f.suite(rel, sleep=0.4 - 0.1 * i, mem=10 * i)
        for j in (1, 4, 4):  # the second -j 4 run schedules longest first from the learned stats
            self.check(self.f.run('-j', str(j)), 0, self.f.expect(), f'-j {j}')

    def test_first_failure_status(self):
        f, g = self.f, Fixture(self.root / 'g')
        _, b, c, d = f.add(dict(wait='01-s', sleep=1), dict(code=3), {}, {})
        self.check(f.run('-j', '2'), 3, f.expect(notes={1}, upto=1))
        self.assertEqual((len(f.recs(b)), f.recs(c), f.recs(d)), (2, [], []), 'rerun once; later tasks never start')
        self.assertTrue(f.alone(f.recs(b)[1]), 'the rerun ran beside another suite')
        g.add(dict(sleep=0.2), dict(code=3), {})
        self.check(g.run('-j', '1'), 3, g.expect(upto=1), '-j 1')
        self.assertEqual((len(g.recs(b)), g.recs(c)), (1, []))

    def test_alone_failure_final(self):
        f = self.f
        a, b = f.add(dict(sleep=0.3), dict(code=5))
        self.check(f.run('-j', '4', '--memory-mb', '100'), 5, f.expect(upto=1))  # unmeasured: 64 MB each, one at a time
        self.assertEqual((len(f.recs(a)), len(f.recs(b))), (1, 1), 'a failure that ran alone is not rerun')

    def test_warm_sequential_order(self):
        f = self.f
        a, b = f.add(dict(sleep=0.1), dict(sleep=0.6))
        self.check(f.run('-j', '4'), 0, f.expect())
        f.suite('01-A/00-s_tester.py', sleep=0.1, code=7)
        for x in f.root.glob('*.start'): x.unlink()
        self.check(f.run('-j', '1'), 7, f.expect(upto=0), '-j 1 with stats that rank 01-s first')
        self.assertEqual((len(f.recs(a)), f.recs(b)), (1, []), '-j 1 keeps task order')

    def test_real_scope(self):
        if not _00_memory_cap.available(): self.skipTest('SKIP: no systemd user scope')
        f = self.f
        for name in ('_00_memory_cap.py', '_00_memory_gate.py'): shutil.copy(LT / name, f.root)
        a, _ = f.add(dict(scope=True, sleep=0.2), dict(scope=True, sleep=0.2))
        code, out, err = f.run('-j', '2')
        self.assertEqual(code, 0, out + err)
        self.assertEqual(sorted(r['mode'] for r in f.recs(a)), ['full', 'quick'], 'the pre-flight ran under the runner')
        stats = json.loads((f.root / '.run-stats.json').read_text())
        self.assertTrue(all(v[0] > 0 for v in stats.values()), f'peaks learned from real MEMORY lines: {stats}')
        g = Fixture(self.root / 'g')
        for name in ('_00_memory_cap.py', '_00_memory_gate.py'): shutil.copy(LT / name, g.root)
        for i in range(2): g.suite(f'02-Data Structures/0{i}-s_tester.py', scope=True, sleep=60, cleanup=1)  # quick: no pre-flight
        proc = subprocess.Popen([sys.executable, g.root / '01-run.py', '--mode', 'quick', '--no-integration', '-j', '2'],
                                env=g.env(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        deadline = time.monotonic() + 30
        while len(list(g.root.glob('*.start'))) < 2 and time.monotonic() < deadline: time.sleep(0.05)
        os.kill(proc.pid, signal.SIGTERM)
        try:
            self.assertNotEqual(proc.wait(60), 0)
        finally:
            if proc.poll() is None: os.killpg(proc.pid, signal.SIGKILL)
        self.assertEqual([r['t1'] is None for r in g.recs()], [False, False], 'SIGTERM: a 1 s cleanup finished in every capped suite')

    def test_integration_stage_stopped(self):
        f = self.f
        f.add({})
        f.suite('02-integration.py', sleep=60, cleanup=0.5)  # stands in for the integration build
        proc = subprocess.Popen([sys.executable, f.root / '01-run.py', '--mode', 'full', '-j', '2'], env=f.env(),
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        deadline = time.monotonic() + 30
        while not f.recs('02-integration') and time.monotonic() < deadline: time.sleep(0.05)
        os.kill(proc.pid, signal.SIGTERM)
        try:
            self.assertNotEqual(proc.wait(30), 0)
        finally:
            if proc.poll() is None: os.killpg(proc.pid, signal.SIGKILL)
        (rec,) = f.recs('02-integration')
        self.assertIsNotNone(rec['t1'], 'SIGTERM stopped the integration stage and its cleanup ran')
        self.assertRaises(ProcessLookupError, os.kill, rec['pid'], 0)

    def test_second_signal_still_kills(self):
        f = self.f
        f.add(dict(sleep=60, deaf=True), dict(sleep=60, deaf=True))
        proc = subprocess.Popen([sys.executable, f.root / '01-run.py', '--no-integration', '-j', '2'], env=f.env(),
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        deadline = time.monotonic() + 30
        while len(f.recs()) < 2 and time.monotonic() < deadline: time.sleep(0.05)
        self.assertEqual(len(f.recs()), 2, 'fixture premise: both suites started')
        os.kill(proc.pid, signal.SIGTERM)
        time.sleep(1)
        os.kill(proc.pid, signal.SIGHUP)  # arrives while the runner waits for suites that ignore SIGINT
        try:
            self.assertNotEqual(proc.wait(40), 0)
        finally:
            if proc.poll() is None: os.killpg(proc.pid, signal.SIGKILL)
        for r in f.recs():
            self.assertRaises(ProcessLookupError, os.kill, r['pid'], 0)

    def test_signal_held_during_restart_stop(self):
        f = self.f
        _, b, c = f.add(dict(sleep=3), dict(sleep=0.3, flaky=True), dict(sleep=60, cleanup=2))
        proc = subprocess.Popen([sys.executable, f.root / '01-run.py', '--no-integration', '-j', '3', '--memory-mb', '999'],
                                env=f.env(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True)
        deadline = time.monotonic() + 30
        while not any(r['t1'] for r in f.recs(b)) and time.monotonic() < deadline: time.sleep(0.02)
        time.sleep(0.5)  # c is now in its 2 s cleanup, stopped for b's rerun
        os.kill(proc.pid, signal.SIGINT)
        try:
            self.assertNotEqual(proc.wait(30), 0, 'the interrupt held during the stop is raised afterwards')
        finally:
            if proc.poll() is None: os.killpg(proc.pid, signal.SIGKILL)
        self.assertEqual(len(f.recs(b)), 1, 'no rerun after the interrupt')
        self.assertEqual(len(f.recs(c)), 1)

    def test_nohup_keeps_sighup_ignored(self):
        f = self.f
        (a,) = f.add(dict(sleep=60))
        proc = subprocess.Popen([sys.executable, f.root / '01-run.py', '--no-integration', '-j', '1'], env=f.env(),
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True,
                                preexec_fn=lambda: signal.signal(signal.SIGHUP, signal.SIG_IGN))
        try:
            deadline = time.monotonic() + 30
            while not f.recs(a) and time.monotonic() < deadline: time.sleep(0.05)
            os.kill(proc.pid, signal.SIGHUP)
            time.sleep(1)
            self.assertIsNone(proc.poll(), 'SIGHUP ignored as under nohup')
            self.assertIsNone(f.recs(a)[0]['t1'])
            os.kill(proc.pid, signal.SIGTERM)
            self.assertNotEqual(proc.wait(30), 0)
        finally:
            if proc.poll() is None: os.killpg(proc.pid, signal.SIGKILL)

    def test_earlier_failure_wins(self):
        a, b, _ = self.f.add(dict(sleep=1, code=4), dict(code=3), dict(sleep=0.2))
        self.check(self.f.run('-j', '3'), 4, self.f.expect(notes={0}, upto=0))
        ra, rb = self.f.recs(a), self.f.recs(b)
        self.assertLess(rb[0]['t1'], ra[0]['t1'], 'fixture premise: the later task fails first')
        self.assertEqual((len(ra), len(rb)), (2, 1), 'the later failure is never rerun')
        self.assertTrue(self.f.alone(ra[1]))

    def test_retry_alone(self):
        f = self.f
        _, b, c = f.add(dict(sleep=0.8), dict(sleep=0.3, flaky=True), dict(sleep=0.8))
        self.check(f.run('-j', '3'), 0, f.expect(notes={1}))
        (first, rerun), (stopped, restarted) = f.recs(b), f.recs(c)
        self.assertFalse(f.alone(first), 'fixture premise: the first run had company')
        self.assertTrue(f.alone(rerun))
        self.assertEqual((stopped['code'], restarted['code']), (None, 0), 'a later running task stops and restarts')
        self.assertGreater(restarted['t0'], rerun['t1'], 'the later task restarts after the rerun alone')
        self.check(f.run('-j', '1'), 0, f.expect(), '-j 1')

    def test_stopped_after_finishing(self):
        """A later task stopped for a rerun after it wrote its result restarts without a NOTE (P247 follow-up 2)."""
        f = self.f
        a, b, _ = f.add(dict(sleep=1, flaky=True), dict(linger=2), dict(sleep=1.5))
        self.check(f.run('-j', '3'), 0, f.expect(notes={0}))
        finished, restarted = f.recs(b)
        self.assertEqual((finished['code'], restarted['code']), (0, 0), 'fixture premise: stopped after its result')
        self.assertGreater(restarted['t0'], f.recs(a)[1]['t1'], 'restarted after the rerun alone')

    def test_jobs_bound(self):
        self.f.add(*[dict(sleep=0.6)] * 6)
        self.check(self.f.run('-j', '3'), 0, self.f.expect())
        self.assertEqual(self.f.span(), 3)

    def test_memory_admission(self):
        f = self.f
        f.add(*[dict(sleep=0.4, mem=1000)] * 4)
        self.check(f.run('-j', '8'), 0, f.expect())
        stats = json.loads((f.root / '.run-stats.json').read_text())
        self.assertEqual({k: v[0] for k, v in stats.items()}, {f'full {rel}': 1000 for rel in f.suites})
        for budget, most, cap in (2327, 1, 4096), (2328, 2, 4096), (2327, 2, 1000):  # each reserves 1000 * 11 // 10 + 64 = 1164 MB, at most the cap
            for s in f.root.glob('*.start'): s.unlink()
            self.check(f.run('-j', '8', '--memory-mb', str(budget), CP_TEST_MEMORY_MB=str(cap)), 0, f.expect(), f'budget {budget}')
            self.assertEqual(f.span(), most, f'budget {budget}, cap {cap}')

    def test_smaller_suite_passes_one_that_does_not_fit(self):
        f = self.f
        f.add(dict(sleep=1.2, mem=1000), dict(sleep=0.6, mem=1000), dict(sleep=0.1, mem=100))
        self.check(f.run('-j', '8'), 0, f.expect())
        for s in f.root.glob('*.start'): s.unlink()
        self.check(f.run('-j', '8', '--memory-mb', str(1164 + 174), CP_TEST_MEMORY_MB='4096'), 0, f.expect())  # 1000 MB reserves 1164, 100 MB 174
        recs = {r['name']: r for r in f.recs()}
        self.assertLess(recs['02-s']['t0'], recs['00-s']['t1'], 'the small suite starts beside the first big one')
        self.assertGreaterEqual(recs['01-s']['t0'], recs['00-s']['t1'], 'the second big one waits for memory')

    def test_unmeasured_reservation(self):
        self.f.add(*[dict(sleep=0.3)] * 3)
        self.check(self.f.run('-j', '8', '--memory-mb', '5000', CP_TEST_MEMORY_MB='4096'), 0, self.f.expect())
        self.assertEqual(self.f.span(), 1)

    def test_stats_tolerance(self):
        f, stats = self.f, self.f.root / '.run-stats.json'
        f.add(dict(mem=7))
        for content in ('{"full 01-A/00-s_tester.py": [1, 2', None):
            stats.write_text(content) if content else stats.unlink()
            self.check(f.run('-j', '2'), 0, f.expect(), repr(content))
            self.assertEqual(json.loads(stats.read_text())['full 01-A/00-s_tester.py'][0], 7)

    def test_stats_merged(self):
        """01-run.py merges what it measured into .run-stats.json: a record another run wrote while it ran stays, one it did
        not measure keeps its value, and its suites get CP_RUN_STATS_OWNER, so their own supervise() does not record them."""
        f, stats = self.f, self.f.root / '.run-stats.json'
        f.add(dict(mem=7, wait='go'), dict(mem=9))
        stats.write_text(json.dumps({'quick 01-A/00-s_tester.py': [3, 1.0], 'full 01-A/01-s_tester.py': [5, 2.0]}))

        def late():  # another session's tester records while 00-s runs
            deadline = time.monotonic() + 30
            while not list(f.root.glob('00-s.*.start')) and time.monotonic() < deadline: time.sleep(0.02)
            stats.write_text(json.dumps(dict(json.loads(stats.read_text()), **{'full 07-X/01-x_tester.py': [11, 1.5]})))
            (f.root / 'go.done').touch()
        writer = threading.Thread(target=late)
        writer.start()
        self.check(f.run('-j', '1'), 0, f.expect())
        writer.join()
        got = json.loads(stats.read_text())
        self.assertEqual({k: v[0] for k, v in got.items()}, {'quick 01-A/00-s_tester.py': 3, 'full 01-A/00-s_tester.py': 7,
                                                              'full 01-A/01-s_tester.py': 9, 'full 07-X/01-x_tester.py': 11})
        self.assertEqual((got['quick 01-A/00-s_tester.py'][1], got['full 07-X/01-x_tester.py'][1]), (1.0, 1.5))
        self.assertEqual([r['owner'] for r in f.recs()], ['1', '1'])
        self.check(f.run('-j', '1'), 0, f.expect())
        got = json.loads(stats.read_text())
        self.assertEqual({k: len(v[2]) for k, v in got.items() if len(v) > 2}, {'full 01-A/00-s_tester.py': 2, 'full 01-A/01-s_tester.py': 3},
                         'every passing suite adds a run, an equal one too; an older two-element record counts as one')

    def test_stats_queue_left_out(self):
        """01-run.py records a suite's seconds without the time the suite waited in the memory queue (its own ADMIT line;
        a nested run's wait, inside the suite's reservation, still counts), so backfill compares run times; never below 0."""
        f, stats = self.f, self.f.root / '.run-stats.json'
        f.add(dict(mem=7, queued=2, sleep=0.5))
        r = f.run('-j', '1')
        self.assertEqual(r[0], 0, r)
        secs = json.loads(stats.read_text())['full 01-A/00-s_tester.py'][1]
        self.assertTrue(0.3 <= secs < 2, secs)

    def test_stress_seeds_env(self):
        f = self.f
        f.suite('01-A/01-a_tester.py', sleep=0.1)
        f.suite('02-B/01-b_tester.py')
        self.check(f.run('--mode', 'stress', '--rounds', '3', '--seed', '7', '-j', '3'), 0, f.expect('stress', (7, 8, 9)))
        got = sorted((r['name'], r['mode'], r['seed'], r['capped']) for r in f.recs())
        self.assertEqual(got, sorted((n, 'stress', str(s), False) for n in ('01-a', '01-b') for s in (7, 8, 9)))

    def test_quick_discovery(self):
        f = self.f
        for rel in ('01-Core/01-template_tester.py', '02-Data Structures/01-q_tester.py', '01-Core/10-x_tester.py',
                    '09-Other/01-y_tester.py', '02-Data Structures/002-z_compile_tester.py'):
            f.suite(rel)
        (f.root / '02-Data Structures/03-helper.py').write_text('raise SystemExit(9)\n')
        compiled, full = f.suites.pop('02-Data Structures/002-z_compile_tester.py'), dict(f.suites)
        for mode, extra, keep in ('full', (), ''), ('quick', (), ('01-Core/01-', '02-')), ('full', ('--filter', '01-Core/'), '01-C'):
            f.suites = {k: v for k, v in full.items() if k.startswith(keep)}
            self.check(f.run('-j', '2', '--mode', mode, *extra), 0, f.expect(mode), f'{mode} {extra}')
        self.assertEqual(f.recs(compiled['name']), [])

    def test_quick_bound_note(self):
        f = self.f
        (f.root / '02-Data Structures').mkdir(exist_ok=True)
        (f.root / '02-Data Structures/01-a_tester.py').write_text("print('TIMING a optimized mode=quick compile=1.00s run=12.50s longest=12.50s limit=180s')\n"
                                                                  "print('TIMING a checked mode=quick compile=99.00s run=10.00s')\n")
        (f.root / '02-Data Structures/02-b_tester.py').write_text("print('TIMING b optimized mode=quick compile=0.10s run=30.25s')\n")
        (f.root / '02-Data Structures/03-c_tester.py').write_text("print('TIMING c optimized mode=full compile=0.10s run=99.00s')\n")
        code, out, err = f.run('--mode', 'quick', '-j', '2')
        self.assertEqual(code, 0, err)
        self.assertIn('NOTE 2 quick build(s) over 10 s, timed beside other suites (05-testing.md, Layout and Modes):\n'
                      '  30.2 s b optimized\n  12.5 s a optimized\nPASS:', out, 'sorted, exactly over the bound, never a failure')
        if not _00_memory_cap.available(): self.skipTest('SKIP: no systemd user scope for the pre-flight half')
        child = ("import os\nif os.environ['CP_TEST_MODE'] == 'quick':\n    print('TIMING d optimized mode=quick compile=0.10s run=11.00s')\n"
                 "    print('OVERHALF d run: 70 s of its 120 s limit')\n")
        for name in ('01-a', '02-b', '03-c'): (f.root / f'02-Data Structures/{name}_tester.py').unlink()
        (f.root / '02-Data Structures/04-d_tester.py').write_text(  # a real pre-flight, as ensure() runs it
            f"import os, sys\nsys.path.insert(0, {str(LT)!r})\nimport _00_memory_cap\n"
            f"sys.exit(_00_memory_cap.supervise([sys.executable, '-c', {child!r}], os.environ['CP_TEST_MODE']))\n")
        for mode in ('full', 'stress'):
            code, out, err = f.run('--mode', mode, '--rounds', '2', '-j', '2', CP_MEMORY_CAPPED='', CP_TEST_MEMORY_MB='256')
            self.assertEqual(code, 0, err)
            self.assertIn('NOTE 1 quick build(s) over 10 s, timed beside other suites (05-testing.md, Layout and Modes):\n  11.0 s d optimized\n', out,
                          f'{mode}: from the pre-flight, once though every stress round runs one')
            self.assertIn('NOTE 1 problem(s) to fix, each within a passing run (05-testing.md, Layout and Modes):\n'
                          '  pre-flight quick d run: 70 s of its 120 s limit\n', out, f'{mode}: labelled as the pre-flight, once')

    def test_problems_note(self):
        f = self.f
        (f.root / '02-Data Structures').mkdir(exist_ok=True)
        (f.root / '02-Data Structures/01-a_tester.py').write_text("print('TIMING a ASan-UBSan mode=full compile=1.00s run=400.00s longest=400.50s limit=720s oracle=0.00s')\nprint('OVERHALF a ASan-UBSan: 400 s of its 720 s limit')\n"
                                                                  "print('TIMING a checked mode=full compile=1.00s run=360.00s longest=360.00s limit=720s oracle=0.00s')\n"
                                                                  "print('TIMING a optimized mode=full compile=1.00s run=9.00s')\n"
                                                                  "import time; time.sleep(1.5)\n")
        mark = f.root / 'flaky-mark'
        (f.root / '02-Data Structures/02-f_tester.py').write_text(f"import pathlib, sys, time\nm = pathlib.Path({str(mark)!r})\n"
                                                                  "if m.exists(): sys.exit(0)\nm.touch(); time.sleep(0.5); sys.exit(3)\n")
        code, out, err = f.run('--mode', 'full', '-j', '2', '--memory-mb', '99999')
        self.assertEqual(code, 0, err)
        self.assertIn('NOTE 2 problem(s) to fix, each within a passing run (05-testing.md, Layout and Modes):\n', out)
        problems = out[out.index('NOTE 2 problem'):out.index('PASS:')].splitlines()[1:]
        self.assertEqual(sorted(problems), ['  a ASan-UBSan: 400 s of its 720 s limit',
                                            '  full 02-Data Structures/02-f_tester.py: exited 3 beside other suites, passed alone'],
                         'a rerun and a command over half its limit')
        mark.unlink()
        (f.root / '02-Data Structures/01-a_tester.py').write_text("print('TIMING a checked mode=full compile=1.00s run=360.00s longest=360.00s limit=720s oracle=0.00s')\n")
        (f.root / '02-Data Structures/02-f_tester.py').write_text('pass\n')
        code, out, err = f.run('--mode', 'full', '-j', '2', '--memory-mb', '99999')
        self.assertEqual((code, 'problem(s)' in out), (0, False), 'nothing to report')

    def test_integration_over_half(self):
        f = self.f
        (f.root / '02-Data Structures').mkdir(exist_ok=True)
        (f.root / '02-Data Structures/01-a_tester.py').write_text('pass\n')
        (f.root / '02-integration.py').write_text("print('OVERHALF x.o: 100 s of its 180 s limit')\n")
        (f.root / '03-consistency.py').write_text('pass\n')
        r = subprocess.run([sys.executable, f.root / '01-run.py', '--mode', 'quick', '-j', '1', '--memory-mb', '999'],
                           env=f.env(), capture_output=True, text=True)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('NOTE 1 problem(s) to fix, each within a passing run (05-testing.md, Layout and Modes):\n  x.o: 100 s of its 180 s limit\n', r.stdout)

    def test_over_half(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            CPP.over_half('exact', 360.0, 720)
            CPP.over_half('above', 360.5, 720)
            CPP.timed_run('fast', [sys.executable, '-c', 'pass'], timeout=60)
            CPP.timed_run('slow', [sys.executable, '-c', 'import time; time.sleep(1.6)'], timeout=3)
        self.assertEqual([l.split(':')[0] for l in out.getvalue().splitlines()], ['OVERHALF above', 'OVERHALF slow'], 'exactly half is not over')

    def test_sigint_stops(self):
        f = self.f
        f.add(*[dict(sleep=60)] * 3)
        proc = subprocess.Popen([sys.executable, f.root / '01-run.py', '--no-integration', '-j', '3', '--memory-mb', '999'],
                                env=f.env(), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, start_new_session=True,
                                preexec_fn=lambda: signal.signal(signal.SIGINT, signal.SIG_DFL))
        deadline = time.monotonic() + 30
        while len(list(f.root.glob('*.start'))) < 3 and time.monotonic() < deadline: time.sleep(0.05)
        os.kill(proc.pid, signal.SIGINT)
        try:
            self.assertNotEqual(proc.wait(60), 0)
        finally:
            if proc.poll() is None: os.killpg(proc.pid, signal.SIGKILL)
        recs = f.recs()
        self.assertEqual((len(recs), [r['name'] for r in recs if r['t1'] is None]), (3, []), 'cleanup ran in every suite')
        for r in recs:
            self.assertRaises(ProcessLookupError, os.kill, r['pid'], 0)
        self.assertEqual(list((f.root / 'busy').iterdir()), [])
        json.loads((f.root / '.run-stats.json').read_text())

    def test_random_runs(self):
        rng = random.Random(SEED)
        for r in range(ROUNDS if MODE == 'stress' else 2):
            f, j, cap, budget = Fixture(self.root / f'r{r}'), rng.randint(1, 4), rng.randint(100, 400), rng.randint(150, 1200)
            for i in range(rng.randint(2, 6)):
                kind = rng.choice('ppppfx')
                f.suite(f'0{rng.randint(1, 2)}-F/{i:02}-r{i}_tester.py', sleep=round(rng.uniform(0, 0.3), 2),
                        code=rng.randint(4, 9) * (kind == 'x'), flaky=kind == 'f')
            mode, rounds, seed = rng.choice(('full', 'stress')), rng.randint(1, 2), rng.randint(0, 99)
            seeds = list(range(seed, seed + rounds)) if mode == 'stress' else [seed]
            keys = [(rel, str(s)) for s in seeds for rel in sorted(f.suites, key=lambda rel: Path(rel).parts)]
            order = [(f.suites[rel], s) for rel, s in keys]
            upto = next((i for i, (P, _) in enumerate(order) if P['code']), None)
            ctx = f'round {r}: -j {j} cap {cap} budget {budget} mode {mode} seeds {seeds} suites {f.suites}'
            got = f.run('--mode', mode, '--rounds', str(rounds), '--seed', str(seed), '-j', str(j), '--memory-mb',
                        str(budget), CP_TEST_MEMORY_MB=str(cap))
            runs = [[x for x in f.recs(P['name']) if x['seed'] == s and x['code'] is not None] for P, s in order]  # stopped runs restart
            # A task the runner stops for an earlier task's rerun may finish its record first (its exit races the stop) and
            # restarts without a NOTE, so after an earlier failure one finished run more carries no NOTE (P247 follow-up 2).
            late = {k for k in range(len(runs)) if any(x['code'] for done in runs[:k] for x in done)}
            failed = {k for k, x in enumerate(runs) if len(x) >= 2 and x[-2]['code']}  # a failed run, then another
            notes, note = set(), None
            for line in got[1].splitlines():
                if m := re.match(r'NOTE (\S+) exited ', line):
                    note = m[1]
                elif m := re.match(r'RUN (\S+) mode=\S+ seed=(\d+)$', line):
                    notes |= {keys.index((m[1], m[2]))} if m[1] == note else set()
                    note = None
            self.assertTrue(failed - late <= notes <= failed, f'{ctx}: NOTEs {notes}, failed {failed}, late {late}')
            self.check(got, 0 if upto is None else order[upto][0]['code'], f.expect(mode, seeds, notes, upto), ctx)
            self.assertLessEqual(f.span(), max(1, min(j, budget // cap)), ctx)
            self.assertTrue(all(len(x) <= 1 + (j > 1) + (k in late) for k, x in enumerate(runs)), ctx)
            self.assertTrue(upto is None or upto not in notes or j > 1, f'{ctx}: only a failure beside others is rerun')
            self.assertTrue(all(f.alone(runs[k][-1]) for k in notes), f'{ctx}: a rerun runs alone')


class Integration(Base):
    def setUp(self):
        super().setUp()
        patcher = patch.dict(os.environ, RT_DIR=str(self.f.root))
        patcher.start()
        self.addCleanup(patcher.stop)

    def cmd(self, name, *timeout, **P):
        return ([sys.executable, str(self.f.suite(f'x/{name}.py', **P))], *timeout)

    def test_parallel_budget(self):
        tasks = [(400, [self.cmd(f'w{i}', sleep=0.4)]) for i in range(5)]
        tasks.insert(2, (1500, [self.cmd('heavy', sleep=0.3), self.cmd('second', 30, sleep=0.1)]))
        INTEGRATION.parallel(tasks, 6, 1000)
        (heavy,), (second,) = self.f.recs('heavy'), self.f.recs('second')
        self.assertEqual(self.f.span(), 2)
        self.assertTrue(self.f.alone(heavy) and self.f.alone(second) and heavy['t1'] < second['t0'])

    def test_parallel_fills_budget(self):
        heavy = [(300, [self.cmd(f'h{i}', sleep=0.6)]) for i in range(4)]
        light = [(50, [self.cmd(f'l{i}', sleep=0.2)]) for i in range(4)]
        INTEGRATION.parallel(heavy + light, 4, 700)
        first_heavy_end = min(r['t1'] for r in self.f.recs('h*'))
        self.assertLess(min(r['t0'] for r in self.f.recs('l*')), first_heavy_end, 'light tasks fill the budget heavy ones leave')
        self.assertLessEqual(self.f.span(), 4)

    def test_parallel_budget_boundary(self):
        for budget, most in (1000, 2), (999, 1):
            for x in self.f.root.glob('*.start'): x.unlink()
            INTEGRATION.parallel([(500, [self.cmd(f'b{i}', sleep=0.3)]) for i in range(2)], 4, budget)
            self.assertEqual(self.f.span(), most, f'budget {budget}')

    def test_parallel_final_failure_order(self):
        a, b = self.cmd('a', code=3), self.cmd('b', code=5)
        with self.assertRaises(RuntimeError) as caught:
            INTEGRATION.parallel([(10, [a]), (200, [b])], 2, 100)  # b is heavier: it runs first, alone, and fails finally
        self.assertIn(repr(a[0]), str(caught.exception), 'an earlier task still runs after a later final failure')
        self.assertEqual((len(self.f.recs('a')), len(self.f.recs('b'))), (1, 1))

    def test_parallel_interrupt(self):
        tasks = [(400, [self.cmd(f't{i}', sleep=1)]) for i in range(8)]
        timer = threading.Timer(0.5, os.kill, (os.getpid(), signal.SIGINT))
        timer.start()
        with self.assertRaises(KeyboardInterrupt):
            INTEGRATION.parallel(tasks, 4, 1000)
        timer.join()
        self.assertEqual(len(self.f.recs()), 2, 'nothing starts after the interrupt')

    def test_main_builds(self):
        root, here = self.root / 'repo', self.root / 'repo/96-Local Testing'
        for rel, text in {'01-Core/01-a.hpp': '#pragma once\ninline int a() { return 1; }\n',
                          '01-Core/999-all.hpp': '#pragma once\n#include "01-a.hpp"\n',
                          '02-Data Structures/01-x.hpp': '#pragma once\ninline int x() { return 2; }\n',
                          '99-Workspace/template.cpp': 'int main() {}\n',
                          '96-Local Testing/01-Core/01-t_tester.cpp': '#include "../../01-Core/01-a.hpp"\nint main() { return a() - 1; }\n'}.items():
            (root / rel).parent.mkdir(parents=True, exist_ok=True)
            (root / rel).write_text(text)
        stub, log = self.root / 'bin/ccache', self.root / 'ccache.log'
        stub.parent.mkdir()
        budgets = []
        real = INTEGRATION.parallel
        def run(*args, ccache=None, **env):
            stub.write_text(f'#!/bin/sh\necho "$PWD|$CCACHE_NOHASHDIR|$*" >> "{log}"\n' + ('exit 1\n' if ccache == 'broken' else 'exec "$@"\n'))
            stub.chmod(0o755)
            log.unlink(missing_ok=True)
            out = io.StringIO()
            with patch.object(INTEGRATION, 'ROOT', root), patch.object(INTEGRATION, 'HERE', here), \
                 patch.object(INTEGRATION, 'parallel', lambda t, j, b: (budgets.append(b), real(t, j, b))), \
                 patch.object(sys, 'argv', ['02-integration.py', *args]), contextlib.redirect_stdout(out), \
                 patch.dict(os.environ, {'PATH': f"{stub.parent}:{os.environ['PATH']}" if ccache else os.environ['PATH'], **env}), \
                 patch.object(CPP.shutil, 'which', lambda name: str(stub) if ccache else None):
                CPP._ccache_prefix.cache_clear()
                self.assertEqual(INTEGRATION.main(), 0)
            return out.getvalue(), log.read_text().splitlines() if log.exists() else []
        out, calls = run('--sanitizers', '-j', '4', ccache='ok', CP_MEMORY_CAPPED='3000')
        self.assertIn('PASS: 3 standalone/aggregate headers', out)
        self.assertEqual(budgets[-1], 2488, 'the budget follows the cap in force')
        args = [c.split('|', 2)[2] for c in calls]
        self.assertTrue(calls and all(c.split('|')[1] == '1' for c in calls), 'CCACHE_NOHASHDIR set for every compile')
        self.assertTrue(all(' -c ' in a or '-fsyntax-only' in a for a in args), 'ccache never sees a link')
        self.assertIn('-fsyntax-only 02-Data_Structures__01-x.cpp', ' '.join(args), 'syntax units named by header path, relative')
        self.assertTrue(any(' -c a.cpp ' in a for a in args) and any('01-t_tester.cpp' in a and ' -c ' in a for a in args))
        out, calls = run('-j', '2', ccache='broken')
        self.assertIn('SKIP ccache:', out)
        self.assertEqual(len(calls), 1, 'a broken ccache is probed once, then bypassed')
        out, calls = run('-j', '1')
        self.assertEqual((calls, 'PASS' in out), ([], True), 'without ccache the compiler runs directly')

    def test_parallel_rerun_alone(self):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            INTEGRATION.parallel([(1, [self.cmd('a', sleep=0.6)]), (1, [self.cmd('f', sleep=0.3, flaky=True)]),
                                  (1, [self.cmd('c', sleep=0.6)])], 3, 100)
        self.assertTrue(out.getvalue().startswith('NOTE RuntimeError beside other builds; the last lines, then its rerun alone:\n| Command failed:'), out.getvalue())
        first, rerun = self.f.recs('f')
        self.assertFalse(self.f.alone(first), 'fixture premise: the first run had company')
        self.assertTrue(self.f.alone(rerun))

    def test_parallel_first_failure(self):
        a, b = self.cmd('a', sleep=1, code=4), self.cmd('b', code=3)
        with self.assertRaises(RuntimeError) as caught:
            INTEGRATION.parallel([(1, [a]), (1, [b]), (1, [self.cmd('c', sleep=0.2)])], 3, 100)
        self.assertIn(repr(a[0]), str(caught.exception))
        ra, rb = self.f.recs('a'), self.f.recs('b')
        self.assertLess(rb[0]['t1'], ra[0]['t1'], 'fixture premise: the later task fails first')
        self.assertEqual((len(ra), len(rb)), (2, 1))
        self.assertTrue(self.f.alone(ra[1]))

    def test_parallel_sequential(self):
        b = self.cmd('b', code=3)
        with self.assertRaises(RuntimeError) as caught:
            INTEGRATION.parallel([(1, [self.cmd('a')]), (1, [b]), (1, [self.cmd('c', code=5)])], 1, 100)
        self.assertIn(repr(b[0]), str(caught.exception))
        self.assertEqual((len(self.f.recs('b')), self.f.recs('c')), (1, []), 'jobs == 1 never reruns and stops at b')
        with self.assertRaises(subprocess.TimeoutExpired):
            INTEGRATION.parallel([(1, [self.cmd('t', 0.5, sleep=20)])], 1, 100)
        with self.assertRaises(OSError):
            INTEGRATION.parallel([(1, [self.cmd('d')]), (1, [([str(self.root / 'missing')],)])], 2, 100)


class Build(Base):
    """The shared build helper: ccache only for -c and -fsyntax-only, probes once, admission, hits across copies of the tree."""

    def setUp(self):
        super().setUp()
        CPP._ccache_prefix.cache_clear()
        self.addCleanup(CPP._ccache_prefix.cache_clear)
        self.bin, self.log = self.root / 'bin', self.root / 'ccache.log'
        self.bin.mkdir()
        self.cxx = os.environ.get('CXX', 'g++')

    def stub(self, broken=False):
        """A logging ccache on PATH, broken or passing through to the compiler."""
        path = self.bin / 'ccache'
        path.write_text(f'#!/bin/sh\necho "$PWD|$CCACHE_NOHASHDIR|$CCACHE_BASEDIR|$*" >> "{self.log}"\n' + ('exit 1\n' if broken else 'exec "$@"\n'))
        path.chmod(0o755)
        return patch.dict(os.environ, {'PATH': f"{self.bin}:{os.environ['PATH']}"})

    def calls(self):
        return [line.split('|', 3) for line in self.log.read_text().splitlines()] if self.log.exists() else []

    def test_units_through_ccache_link_not(self):
        tree = self.root / 'tree'
        (tree / 'src').mkdir(parents=True)
        (tree / 'src/lib.cpp').write_text('int lib() { return 3; }\n')
        build = self.root / 'build'
        build.mkdir()
        (build / 'gen.cpp').write_text('int lib(); int main() { return lib() - 3; }\n')
        with self.stub(), patch.object(CPP, 'ROOT', tree):
            builder = CPP.Builder([self.cxx], build)
            seconds = builder.build('b', ['-std=gnu++20'], [build / 'gen.cpp', tree / 'src/lib.cpp'], build / 'out', 60)
            builder.syntax('s', ['-std=gnu++20'], build / 'gen.cpp', 60)
        self.assertGreater(seconds, 0)
        self.assertEqual(subprocess.run([build / 'out']).returncode, 0, 'the two objects link into a working binary')
        probe, gen, lib, syntax = self.calls()
        self.assertIn('-fsyntax-only probe.cpp', probe[3], 'one probe')
        self.assertEqual(gen[0], str(build), 'a generated unit compiles where it lives')
        self.assertIn(' -c gen.cpp ', gen[3], 'under its relative name')
        self.assertEqual(lib[0], str(tree), 'a tree unit compiles from the root')
        self.assertIn(f' -c {tree}/src/lib.cpp ', lib[3])
        self.assertIn('-fsyntax-only gen.cpp', syntax[3])
        for call in (gen, lib, syntax):
            self.assertEqual(call[1:3], ['1', str(tree)], 'CCACHE_NOHASHDIR and CCACHE_BASEDIR on every compile')
        self.assertEqual(len(self.calls()), 4, 'the link never goes through ccache')

    def test_fail_fast_builds_when_first_run(self):
        build, ran = self.root / 'build', []
        build.mkdir()
        for name, code in (('a', 0), ('b', 3), ('c', 0)):
            (build / f'{name}.cpp').write_text(f'int main() {{ return {code}; }}\n')
        specs = [(n, f'compile {n}', ['-std=gnu++20'], [build / f'{n}.cpp'], build / n, 60, 100) for n in 'abc']
        built = lambda: sorted(p.name for p in build.iterdir() if '.' not in p.name)
        def job(n):
            code = CPP.timed_run(n, [str(build / n)], timeout=60).returncode
            ran.append(n)
            if code:
                raise CPP.TestFailure(n)
        self.addCleanup(CPP.PENDING.clear)
        with patch.object(CPP.shutil, 'which', lambda name: None), contextlib.redirect_stdout(io.StringIO()):
            CPP._ccache_prefix.cache_clear()
            builder = CPP.Builder([self.cxx], build)
            with patch.dict(os.environ, {'CP_TEST_FAIL_FAST': '1'}):
                self.assertEqual((builder.build_all(specs), built()), ({'a': 0.0, 'b': 0.0, 'c': 0.0}, []), 'no build is made up front')
                mark = builder.stopwatch()
                with self.assertRaises(CPP.TestFailure):
                    builder.concurrently([(100, lambda n=n: job(n)) for n in 'abc'])
                self.assertGreater(builder._spent() - mark[1], 0, 'a build made when first run counts as compile time')
                self.assertEqual((ran, built()), (['a', 'b'], ['a', 'b']), 'jobs run in order and the build after the first failure is never made')
                CPP.ready(build / 'c')
                self.assertEqual((built(), CPP.PENDING), (['a', 'b', 'c'], {}), 'ready makes the pending build of a binary started another way')
                CPP.ready(build / 'c')
            for n in 'abc':
                (build / n).unlink()
            seconds = builder.build_all(specs)
            self.assertEqual((built(), all(s > 0 for s in seconds.values()), CPP.PENDING), (['a', 'b', 'c'], True, {}), 'without the switch every build is made up front')

    def test_failure_reports_command_and_output(self):
        class Custom(RuntimeError):
            pass
        build = self.root / 'build'
        build.mkdir()
        (build / 'bad.cpp').write_text('int main() { return x; }\n')
        with patch.object(CPP.shutil, 'which', lambda name: None):
            CPP._ccache_prefix.cache_clear()
            builder = CPP.Builder([self.cxx], build, failure=Custom)
            with self.assertRaises(Custom) as caught:
                builder.build('bad build', ['-std=gnu++20'], [build / 'bad.cpp'], build / 'out', 60)
        text = str(caught.exception)
        for piece in ('bad build: exit=', 'command=', 'bad.cpp', 'was not declared'):
            self.assertIn(piece, text)

    def test_timeout_is_a_failure(self):
        slow = self.bin / 'slow'
        slow.write_text('#!/bin/sh\nsleep 30\n')
        slow.chmod(0o755)
        with patch.object(CPP.shutil, 'which', lambda name: None):
            builder = CPP.Builder([str(slow)], self.root)
            with self.assertRaises(CPP.TestFailure) as caught:
                builder.syntax('slow step', [], self.root / 'x.cpp', 1)
        self.assertIn('slow step: timeout=1s', str(caught.exception))

    def test_fallback_reported_once(self):
        out = io.StringIO()
        with self.stub(broken=True), contextlib.redirect_stdout(out):
            self.assertEqual(CPP.cached([self.cxx]), [self.cxx], 'a broken ccache is bypassed')
            self.assertEqual(CPP.cached([self.cxx]), [self.cxx])
        self.assertEqual(out.getvalue().count('SKIP ccache:'), 1)
        self.assertEqual(len(self.calls()), 1, 'probed once')
        CPP._ccache_prefix.cache_clear()
        out = io.StringIO()
        with patch.object(CPP.shutil, 'which', lambda name: None), contextlib.redirect_stdout(out):
            self.assertEqual(CPP.cached([self.cxx]), [self.cxx])
            CPP.cached([self.cxx])
        self.assertEqual(out.getvalue(), 'SKIP ccache: not installed; plain builds\n')
        CPP._ccache_prefix.cache_clear()
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            self.assertEqual(CPP.cached(['/usr/bin/ccache', self.cxx]), ['/usr/bin/ccache', self.cxx], 'a CXX naming ccache is not wrapped twice')
        self.assertEqual(out.getvalue(), '')

    def test_compiler_command_splits(self):
        for value, want in (('g++-14', ['g++-14']), ('ccache "g++ x" -O1', ['ccache', 'g++ x', '-O1']), ('', ['g++']), (' ', ['g++'])):
            with patch.dict(os.environ, {'CXX': value}):
                self.assertEqual(CPP.compiler_command(), want)
        env = {k: v for k, v in os.environ.items() if k != 'CXX'}
        with patch.dict(os.environ, env, clear=True):
            self.assertEqual(CPP.compiler_command(), ['g++'])

    def fake_compiler(self, seconds, failing=()):
        """A compiler recording each -c interval; a source named in failing exits 1 after its sleep."""
        path, spans = self.bin / 'fake', self.root / 'spans'
        spans.mkdir()
        path.write_text(f"""#!{sys.executable}
import pathlib, sys, time, uuid
a = sys.argv[1:]
if '-c' in a:
    start = time.monotonic()
    time.sleep({seconds})
    pathlib.Path({str(spans)!r}, uuid.uuid4().hex).write_text(f'{{start}} {{time.monotonic()}}')
    sys.exit(1 if a[a.index('-c') + 1] in {list(failing)!r} else 0)
""")
        path.chmod(0o755)
        return [str(path)], spans

    def intervals(self, spans):
        return [tuple(map(float, f.read_text().split())) for f in spans.iterdir()]

    def builds(self, n, need=100):
        return [(f'b{k}', f'build {k}', [], [f'u{k}.cpp'], str(self.root / f'o{k}'), 60, need) for k in range(n)]

    def test_build_all_three_at_once(self):
        compiler, spans = self.fake_compiler(0.3)
        with patch.object(CPP.shutil, 'which', lambda name: None), patch.dict(os.environ, {'CP_MEMORY_CAPPED': 'none'}):
            CPP._ccache_prefix.cache_clear()
            seconds = CPP.Builder(compiler, self.root).build_all(self.builds(7))
        self.assertEqual(sorted(seconds), [f'b{k}' for k in range(7)])
        self.assertEqual(overlap(self.intervals(spans)), 3, 'exactly three of seven at once without a memory limit')

    def test_build_all_within_the_cap(self):
        compiler, spans = self.fake_compiler(0.2)
        with patch.object(CPP.shutil, 'which', lambda name: None), patch.dict(os.environ, {'CP_MEMORY_CAPPED': '1000'}):  # budget 600 MB
            CPP._ccache_prefix.cache_clear()
            CPP.Builder(compiler, self.root).build_all(self.builds(4, 300))  # 300 + 300 fits exactly
        self.assertEqual(overlap(self.intervals(spans)), 2)
        compiler, spans = self.fake_compiler_again(0.2)
        with patch.object(CPP.shutil, 'which', lambda name: None), patch.dict(os.environ, {'CP_MEMORY_CAPPED': '1000'}):
            CPP._ccache_prefix.cache_clear()
            CPP.Builder(compiler, self.root).build_all(self.builds(3, 301))
        self.assertEqual(overlap(self.intervals(spans)), 1, 'one MB more than fits runs them one at a time')
        compiler, spans = self.fake_compiler_again(0.05)
        with patch.object(CPP.shutil, 'which', lambda name: None), patch.dict(os.environ, {'CP_MEMORY_CAPPED': '1000'}):
            CPP._ccache_prefix.cache_clear()
            CPP.Builder(compiler, self.root).build_all(self.builds(2, 5000))
        self.assertEqual(len(self.intervals(spans)), 2, 'a build larger than the budget still runs, alone')

    def test_interrupt_stops_builds_and_jobs(self):
        """SIGINT during build_all or concurrently, to the process alone or as Ctrl-C twice to its group (the dying commands
        race the main thread's handler): the three running commands die, no queued build or job starts a command, and
        KeyboardInterrupt ends the process at once."""
        starts = self.root / 'starts'
        fake = self.bin / 'slow'
        fake.write_text(f"#!/bin/sh\n: > {shlex.quote(str(starts))}/$$\nsleep 30\nfor a; do last=$a; done\n: > \"$last\"\n")  # dies at once on SIGINT
        fake.chmod(0o755)
        child = f"""import functools, subprocess, sys
sys.path.insert(0, {str(Path(CPP.__file__).parent)!r})
import _00_cpp_test_runner as CPP
CPP.shutil.which = lambda name: None
b = CPP.Builder([{str(fake)!r}], {str(self.root)!r})
def job(k):
    for step in range(3):
        try:
            CPP.timed_run('job', [{str(fake)!r}, str(k)], timeout=60)
        except Exception:
            pass
if sys.argv[1] == 'builds':
    b.build_all([(f'b{{k}}', 'build', [], [f'u{{k}}.cpp'], {str(self.root)!r} + f'/o{{k}}', 60, 1) for k in range(6)])
else:
    b.concurrently([(1, functools.partial(job, k)) for k in range(5)])
"""
        for kind, group in (('builds', False), ('jobs', False), ('builds', True), ('jobs', True)):
            with self.subTest(kind=kind, group=group):
                shutil.rmtree(starts, ignore_errors=True)
                starts.mkdir()
                proc = subprocess.Popen([sys.executable, '-c', child, kind], env=dict(os.environ, CP_MEMORY_CAPPED='none'),
                                        stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True, start_new_session=True, cwd=self.root)
                self.addCleanup(self.kill_group, proc.pid)  # a failed check leaves no fake running to finish its work
                deadline = time.monotonic() + 20
                while len(list(starts.iterdir())) < 3 and time.monotonic() < deadline:
                    time.sleep(0.05)
                start = time.monotonic()
                if group:
                    os.killpg(proc.pid, signal.SIGINT)
                    time.sleep(0.15)
                    with contextlib.suppress(ProcessLookupError):
                        os.killpg(proc.pid, signal.SIGINT)
                else:
                    os.kill(proc.pid, signal.SIGINT)
                _, err = proc.communicate(timeout=20)
                elapsed = time.monotonic() - start
                time.sleep(0.3)
                self.assertIn('KeyboardInterrupt', err)
                self.assertLess(elapsed, 5, 'the process ends at once')
                self.assertEqual(len(list(starts.iterdir())), 3, 'nothing starts after the interrupt')

    @staticmethod
    def kill_group(pid):
        with contextlib.suppress(ProcessLookupError):
            os.killpg(pid, signal.SIGKILL)

    def fake_compiler_again(self, seconds):
        shutil.rmtree(self.root / 'spans')
        (self.bin / 'fake').unlink()
        return self.fake_compiler(seconds)

    def test_build_all_first_failure_in_order(self):
        compiler, spans = self.fake_compiler(0.1, ['u1.cpp', 'u3.cpp'])
        with patch.object(CPP.shutil, 'which', lambda name: None):
            CPP._ccache_prefix.cache_clear()
            with self.assertRaises(CPP.TestFailure) as caught:
                CPP.Builder(compiler, self.root).build_all(self.builds(5))
        self.assertIn('build 1:', str(caught.exception), 'the first failure in build order, not in time')
        self.assertEqual(len(self.intervals(spans)), 5, 'every build finished first')

    def test_report_counts_builds_as_compile(self):
        build = self.root / 'build'
        build.mkdir()
        (build / 'a.cpp').write_text('int main() {}\n')
        out = io.StringIO()
        with patch.object(CPP.shutil, 'which', lambda name: None), contextlib.redirect_stdout(out):
            CPP._ccache_prefix.cache_clear()
            builder = CPP.Builder([self.cxx], build)
            mark = builder.stopwatch()
            spent = builder.build('a', ['-std=gnu++20'], [build / 'a.cpp'], build / 'out', 60)
            time.sleep(0.2)
            builder.report(mark, 'name config', 'quick', 1.5)
        line = next(l for l in out.getvalue().splitlines() if l.startswith('TIMING'))
        fields = dict(x.split('=') for x in line.split()[3:])
        self.assertTrue(line.startswith('TIMING name config mode=quick '), line)
        self.assertAlmostEqual(float(fields['compile'].rstrip('s')), 1.5 + spent, delta=0.05)
        self.assertLess(float(fields['run'].rstrip('s')), 0.5, 'the nested build is not run time')
        self.assertGreater(float(fields['run'].rstrip('s')), 0.15)

    def real_cache(self):
        if not shutil.which('ccache'):
            self.skipTest('SKIP ccache not installed')
        return patch.dict(os.environ, {'CCACHE_DIR': str(self.root / 'cache'), 'CCACHE_MAXSIZE': '200M'})

    def hits(self):
        out = subprocess.run(['ccache', '--print-stats'], capture_output=True, text=True).stdout
        return {k: int(v) for k, v in (line.split('\t')[:2] for line in out.splitlines())}

    def test_real_hits_across_copies_of_the_tree(self):
        with self.real_cache():
            subprocess.run(['ccache', '-z'], capture_output=True)
            objects = {}
            for name, value in (('s0', 1), ('s1', 1), ('s2', 2)):
                tree = self.root / name
                (tree / 'dir').mkdir(parents=True)
                (tree / 'dir/h.hpp').write_text(f'inline int f() {{ return {value}; }}\n')
                (tree / 'dir/x.cpp').write_text('#include "h.hpp"\nint main() { return f(); }\n')
                build = self.root / (name + '-build')
                build.mkdir()
                time.sleep(2.1)  # past ccache's too-new window, as a header written before a run is in a later stage
                with patch.object(CPP, 'ROOT', tree):
                    builder = CPP.Builder([self.cxx], build)
                    builder.compile_unit('x', ['-std=gnu++20', '-O1', '-g'], tree / 'dir/x.cpp', build / 'x.o', 60)
                objects[name] = (build / 'x.o').read_bytes()
            self.assertEqual(objects['s0'], objects['s1'], 'the shard copy reuses the same object')
            self.assertNotEqual(objects['s0'], objects['s2'], 'a changed header misses')
            stats = self.hits()
            self.assertGreaterEqual(stats['direct_cache_hit'] + stats['preprocessed_cache_hit'], 1, stats)
            self.assertEqual(stats['cache_miss'], 3, 'probe, the first tree and the changed header each compile; the copy hits')

    def contest_tree(self, flags_check=''):
        """A tree with the real shared runners and one contest suite whose C++ reports its build flags."""
        lt = self.root / 'tree/96-Local Testing'
        suite = lt / '02-Data Structures'
        suite.mkdir(parents=True)
        for name in ('_00_memory_cap.py', '_00_memory_gate.py', '_00_cpp_test_runner.py', '_00_contest_runner.py'):
            shutil.copy2(LT / name, lt / name)
        (suite / '01-t_tester.cpp').write_text("""#include <cassert>
#include <cstdio>
#include <cstring>
int main(int argc, char **argv) {
#ifdef NDEBUG
    std::puts("NDEBUG");
#endif
#ifdef _GLIBCXX_DEBUG
    std::puts("DEBUG");
#endif
#ifdef EXTRA_FLAG
    std::puts("EXTRA");
#endif
#ifdef __OPTIMIZE__
    std::puts("OPTIMIZED");
#endif
#ifdef __SANITIZE_ADDRESS__
    std::puts("ASAN");
#endif
    if (argc > 2 && !std::strcmp(argv[1], "--invalid")) { assert(false); }
    return 0; }
""")
        return suite / '01-t_tester.py'

    def contest(self, tester, call, *args):
        tester.write_text('import sys\nfrom pathlib import Path\nsys.path.insert(0, str(Path(__file__).resolve().parents[1]))\n'
                          f'from _00_contest_runner import main\nraise SystemExit({call})\n')
        return subprocess.run([sys.executable, str(tester), *args], capture_output=True, text=True,
                              env=dict(os.environ, CP_MEMORY_CAPPED='none', CCACHE_DIR=str(self.root / 'cache')))

    def test_contest_runner_parameters(self):
        tester = self.contest_tree()
        r = self.contest(tester, "main('01-t', ['p'])", '--mode', 'quick')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('NDEBUG', r.stdout)
        self.assertIn('DEBUG', r.stdout.replace('NDEBUG', ''), 'the checked build defaults to _GLIBCXX_DEBUG')
        self.assertIn('PASS 01-t 1 assertion probes', r.stdout)
        self.assertIn('PASS 01-t: 2 configurations', r.stdout)
        self.assertEqual(sum(l.startswith('TIMING 01-t ') for l in r.stdout.splitlines()), 2, r.stdout)
        self.assertRegex(r.stdout, r'TIMING 01-t optimized mode=quick compile=[\d.]+s run=[\d.]+s longest=[\d.]+s limit=720s')
        r = self.contest(tester, "main('01-t', ['p'], extra=('-DEXTRA_FLAG',), timeout=77, checked='-D_GLIBCXX_ASSERTIONS')", '--mode', 'quick')
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertIn('EXTRA', r.stdout)
        self.assertNotIn('DEBUG', r.stdout.replace('NDEBUG', ''), 'checked replaces the default standard-library mode')
        self.assertIn('limit=77s', r.stdout)

    def test_contest_runner_checked_level(self):
        tester = self.contest_tree()
        for mode, optimized in (('quick', False), ('full', True), ('stress', True)):
            r = self.contest(tester, "main('01-t', [])", '--mode', mode, '--configuration', 'checked')
            self.assertEqual(r.returncode, 0, r.stderr)
            self.assertEqual('OPTIMIZED' in r.stdout, optimized, mode)

    def test_contest_runner_configurations(self):
        tester = self.contest_tree()
        ran = lambda r: [l.split('configuration=')[1].split()[0] for l in r.stdout.splitlines() if l.startswith('RUN ')]
        r = self.contest(tester, "main('01-t', [])", '--mode', 'full', '--configuration', 'checked,ASan-UBSan')
        self.assertEqual((r.returncode, ran(r)), (0, ['checked', 'ASan-UBSan']), r.stderr)
        self.assertIn('ASAN', r.stdout)
        r = self.contest(tester, "main('01-t', [])", '--mode', 'quick', '--configuration', 'ASan-UBSan')
        self.assertEqual((r.returncode, ran(r)), (0, ['ASan-UBSan']), r.stderr)
        r = self.contest(tester, "main('01-t', [])", '--mode', 'quick')
        self.assertEqual(ran(r), ['optimized', 'checked'], 'quick builds no sanitizer unless asked')
        r = self.contest(tester, "main('01-t', [])", '--mode', 'full')
        self.assertEqual(ran(r), ['optimized', 'checked', 'ASan-UBSan'])
        r = self.contest(tester, "main('01-t', [])", '--configuration', 'nonsense')
        self.assertEqual(r.returncode, 2, 'an unknown configuration is a usage error')
        r = self.contest(tester, "main('01-t', [])", '--configuration', 'native')
        self.assertEqual(r.returncode, 2, 'native is a usage error unless the suite asks for it')
        fma = CPP.cpu_runs({'fma'})
        r = self.contest(tester, "main('01-t', [], native=True)", '--mode', 'quick')
        self.assertEqual((r.returncode, ran(r)), (0, ['optimized', 'checked']), 'quick builds no native unless asked')
        for args, want in ((('--mode', 'full'), ['optimized', 'checked', 'ASan-UBSan']), (('--mode', 'quick', '--configuration', 'native'), [])):
            r = self.contest(tester, "main('01-t', [], native=True)", *args)
            self.assertEqual((r.returncode, ran(r)), (0, want + ['native'] if fma else want), (args, r.stderr))
            self.assertEqual('SKIP 01-t native' in r.stdout, not fma, args)

    def test_contest_runner_warnings_are_errors(self):
        tester = self.contest_tree()
        tester.with_suffix('.cpp').write_text('int main() { int unused; return 0; }\n')
        r = self.contest(tester, "main('01-t', [])", '--mode', 'quick')
        self.assertEqual(r.returncode, 1, 'every suite compiles with -Werror')
        self.assertIn('FAIL 01-t', r.stderr)
        self.assertIn('unused', r.stderr)

    def test_bypasses(self):
        self.assertEqual(CPP.bypasses('from _00_contest_runner import main\n', True), [])
        self.assertEqual(CPP.bypasses('print(1)\n', True), ['no _00_contest_runner'])
        self.assertEqual(CPP.bypasses('print(1)\n', False), [])
        for allowed in ('run([*compiler, "-fsyntax-only", bad])', 'run([*compiler, "-dumpversion"])', 'run([*compiler, "-dM", "-E"])',
                        'compiler = shlex.split(os.environ.get("CXX", "g++"))'):
            self.assertEqual(CPP.bypasses(allowed, False), [], allowed)
        for direct in ('run([*compiler, "-O2", src])', 'run(["g++", src])', "run(['clang++', src])", 'run([cxx, src])', 'cmd += ["-o", out]', "cmd = [tool, '-c', src]"):
            self.assertEqual(CPP.bypasses(direct, False), [direct], direct)

    def test_runs_in_order(self):
        self.assertTrue(CPP.runs_in_order('builder.build_all(x)\nfor name in x: run(name)\n'))
        self.assertFalse(CPP.runs_in_order('builder.build_all(x)\nbuilder.concurrently(y)\n'))
        self.assertFalse(CPP.runs_in_order('from _00_contest_runner import main\n'))

    def test_concurrently_runs_together_in_job_order(self):
        builder, barrier = CPP.Builder([self.cxx], self.root), threading.Barrier(3, timeout=30)

        def job(k, delay):
            def run():
                print(f'a{k}')
                barrier.wait()
                time.sleep(delay)
                print(f'b{k}')
            return run
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            builder.concurrently([(1, job(0, 0.2)), (1, job(1, 0)), (1, job(2, 0.05))])
        self.assertEqual(out.getvalue().split(), ['a0', 'b0', 'a1', 'b1', 'a2', 'b2'])

    def test_concurrently_holds_the_cap(self):
        builder, lock, now, most = CPP.Builder([self.cxx], self.root), threading.Lock(), [0], [0]
        builder.budget = 300

        def job():
            with lock:
                now[0] += 1
                most[0] = max(most[0], now[0])
            time.sleep(0.05)
            with lock:
                now[0] -= 1
        builder.concurrently([(200, job)] * 4 + [(10**6, job)])  # 200 MB each: one at a time; a lone oversized job still runs
        self.assertEqual(most[0], 1)

    def test_concurrently_stops_at_the_first_failure(self):
        builder, started, skipped = CPP.Builder([self.cxx], self.root), threading.Event(), []

        def long():
            process = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
            started.set()
            process.wait()

        def fail():
            self.assertTrue(started.wait(30))
            time.sleep(0.1)  # the sibling's process exists
            raise RuntimeError('first failure')
        start = time.monotonic()
        with self.assertRaisesRegex(RuntimeError, 'first failure'):
            builder.concurrently([(1, long), (1, fail), (1, lambda: time.sleep(0.4)), (1, lambda: skipped.append(1))])
        self.assertLess(time.monotonic() - start, 20, 'the running sibling was killed')
        self.assertFalse(skipped, 'a job not yet started is skipped')

    def test_capped_process_skips_core_dumps(self):
        code = (f'import resource, sys; sys.path.insert(0, {str(LT)!r}); import _00_memory_cap; _00_memory_cap.ensure(); '
                'print(resource.getrlimit(resource.RLIMIT_CORE)[0])')
        result = subprocess.run([sys.executable, '-c', code], env=dict(os.environ, CP_MEMORY_CAPPED='none'), capture_output=True, text=True, timeout=60)
        self.assertEqual(result.stdout.strip(), '1', result.stderr)  # 1 makes the kernel skip a piped core_pattern; 0 would not

    def test_memory_cap_sees_the_shared_runner(self):
        d = self.root / 'lt/02-X'
        d.mkdir(parents=True)
        main = d / '01-t_tester.py'
        main.write_text('from _00_contest_runner import main\n')
        self.assertFalse(_00_memory_cap.honours_mode_env(main), 'no runner file')
        (d.parent / '_00_contest_runner.py').write_text("os.environ.get('CP_TEST_MODE')\n")
        self.assertTrue(_00_memory_cap.honours_mode_env(main), 'the shared runner in the parent directory')


class Forward(Base):
    """supervise() and the CLI reporter pass SIGTERM and SIGHUP sent to the supervisor pid alone on to the capped command."""

    def start(self, mode, sleep=60, hup=signal.SIG_DFL, cli=False, kid=False):
        """The supervisor of sig_tester.py: its own ensure(), or with cli the ad-hoc CLI, whose scope runs a reporter."""
        if not _00_memory_cap.available(): self.skipTest('SKIP: no systemd user scope')
        (self.root / 'sig_tester.py').write_text(SIG)
        for x in self.root.glob('quick.*'): x.unlink()
        env = {k: v for k, v in os.environ.items() if not k.startswith('CP_TEST_') and k != 'CP_MEMORY_CAPPED'} | dict(
            RT_DIR=str(self.root), RT_LT=str(LT), RT_SLEEP=str(sleep), CP_TEST_MEMORY_MB='256', CP_TEST_PREFLIGHT_MB='256') | (
            dict(RT_KID='1') if kid else {})
        cmd = [sys.executable, self.root / 'sig_tester.py', '--mode', mode]
        with open(self.root / 'out', 'w') as out, open(self.root / 'err', 'w') as err:  # files: an orphan holding a pipe would block
            proc = subprocess.Popen([sys.executable, LT / '_00_memory_cap.py', '--mb', '256', '--'] * cli + cmd, env=env,
                                    stdout=out, stderr=err, start_new_session=True,
                                    preexec_fn=lambda: signal.signal(signal.SIGHUP, hup))
        self.addCleanup(self.reap, proc.pid)
        start, deadline = self.root / 'quick.start', time.monotonic() + 30
        while not start.exists() and time.monotonic() < deadline and proc.poll() is None: time.sleep(0.02)
        self.assertTrue(start.exists(), f'fixture premise: the capped child started; exit {proc.poll()}')
        rec = json.loads(start.read_text())
        scoped = int(Path(f"/proc/{rec['ppid']}/stat").read_text().rsplit(')', 1)[1].split()[1]) if cli else rec['ppid']
        self.assertEqual(scoped, proc.pid, 'systemd-run --scope execs the command, so the capped process is the supervisor\'s own child')
        return proc, rec

    def reap(self, group):
        """SIGKILL whatever is left of the supervisor's group, an orphaned child included."""
        with contextlib.suppress(ProcessLookupError):
            os.killpg(group, signal.SIGKILL)

    def stopped(self, mode, sig, cli=False):
        """Send sig to the supervisor pid alone; the child records it, exits 40 + sig, and nothing is left behind."""
        proc, rec = self.start(mode, cli=cli)
        os.kill(proc.pid, sig)
        proc.wait(30)
        err = (self.root / 'err').read_text()
        ctx = f"{mode} {sig!r}: stdout {(self.root / 'out').read_text()!r} stderr {err!r}"
        self.assertEqual(proc.returncode, 40 + sig, f"{ctx}: the child's status, not the supervisor's own death")
        self.assertEqual((self.root / 'quick.sig').read_text(), str(int(sig)), ctx)
        for pid in rec['pid'], rec['ppid']:
            self.assertRaises(ProcessLookupError, os.kill, pid, 0)
        return err

    def test_forward_sigterm(self):
        self.stopped('quick', signal.SIGTERM)

    def test_forward_sighup(self):
        self.stopped('quick', signal.SIGHUP)

    def test_forward_cli(self):
        for sig in signal.SIGTERM, signal.SIGHUP:
            with self.subTest(sig=sig):
                self.stopped('quick', sig, cli=True)

    def test_forward_preflight(self):
        for sig in signal.SIGTERM, signal.SIGHUP:
            with self.subTest(sig=sig):
                err = self.stopped('full', sig)
                self.assertIn('FAIL sig_tester.py: pre-flight quick run failed', err)
                self.assertIn('full run skipped', err)
                self.assertFalse((self.root / 'full.start').exists(), 'the full run never starts')

    def test_supervisor_death_ends_scope(self):
        """A supervisor killed outright (a caller's subprocess timeout sends SIGKILL, which cannot be forwarded) still takes
        its whole scope down: the capped entry point or the CLI reporter, its command and a grandchild."""
        def alive(pid):
            with contextlib.suppress(OSError):
                return Path(f'/proc/{pid}/stat').read_text().rsplit(')', 1)[1].split()[0] != 'Z'
            return False
        for cli in False, True:
            with self.subTest(cli=cli):
                proc, rec = self.start('quick', cli=cli, kid=True)
                pids = {rec['pid'], rec['kid']} | ({rec['ppid']} if cli else set())
                self.assertTrue(all(map(alive, pids)), 'fixture premise: the scope runs')
                os.kill(proc.pid, signal.SIGKILL)
                proc.wait(30)
                deadline = time.monotonic() + 10
                while any(map(alive, pids)) and time.monotonic() < deadline: time.sleep(0.05)
                self.assertEqual([p for p in sorted(pids) if alive(p)], [], f'cli={cli}: nothing of the scope outlives its supervisor')
                self.assertFalse((self.root / 'quick.end').exists(), 'the capped child never finished its sleep')

    def test_preflight_echoes_timing(self):
        """The PREFLIGHT line carries the quick run's TIMING and OVERHALF lines, so 01-run.py's NOTEs see them in a full run."""
        child = ('import os\nif os.environ["CP_TEST_MODE"] == "quick":\n'
                 '    print("noise\\nTIMING a optimized mode=quick compile=1.00s run=12.00s\\nOVERHALF a run: 70 s of its 120 s limit")\n'
                 'else:\n    print("FULL")\n')
        driver = f'import sys\nsys.path.insert(0, {str(LT)!r})\nimport _00_memory_cap\nsys.exit(_00_memory_cap.supervise([sys.executable, "-c", {child!r}], "full"))\n'
        env = {k: v for k, v in os.environ.items() if k not in ('CP_MEMORY_CAPPED', 'CP_TEST_PREFLIGHT')}
        r = subprocess.run([sys.executable, '-c', driver], env=dict(env, CP_TEST_MODE='full'), capture_output=True, text=True, timeout=120)
        self.assertEqual(r.returncode, 0, r.stderr)
        lines = r.stdout.splitlines()
        at = next(i for i, line in enumerate(lines) if line.startswith('PREFLIGHT '))
        self.assertEqual(lines[at + 1:at + 3], ['TIMING a optimized mode=quick compile=1.00s run=12.00s', 'OVERHALF pre-flight quick a run: 70 s of its 120 s limit'])
        self.assertNotIn('noise', r.stdout)
        self.assertIn('FULL', lines[at + 3:])

    def test_forward_nohup_keeps_sighup_ignored(self):
        proc, rec = self.start('quick', sleep=2, hup=signal.SIG_IGN)
        os.kill(proc.pid, signal.SIGHUP)
        self.assertEqual(proc.wait(30), 0, (self.root / 'err').read_text())
        self.assertEqual(((self.root / 'quick.sig').exists(), (self.root / 'quick.end').exists()), (False, True),
                         'SIGHUP not forwarded; the child finished normally')

    def test_forwarding_restores_handlers(self):
        mine = lambda *_: None
        saved = {s: signal.signal(s, h) for s, h in ((signal.SIGTERM, mine), (signal.SIGHUP, signal.SIG_IGN))}
        self.addCleanup(lambda: [signal.signal(s, h) for s, h in saved.items()])
        ready = self.root / 'ready'
        child = ('import signal, sys, time\nfor s in (signal.SIGTERM, signal.SIGHUP): signal.signal(s, lambda sig, _: sys.exit(40 + sig))\n'
                 'print("x", flush=True)\nopen(sys.argv[1], "w").close()\ntime.sleep(30)\n')
        def send():
            deadline = time.monotonic() + 20
            while not ready.exists() and time.monotonic() < deadline: time.sleep(0.02)
            os.kill(os.getpid(), signal.SIGHUP)
            time.sleep(0.3)
            os.kill(os.getpid(), signal.SIGTERM)
        sender = threading.Thread(target=send)
        sender.start()
        r = _00_memory_cap.forwarding([sys.executable, '-c', child, str(ready)], stdout=subprocess.PIPE, text=True)
        sender.join()
        self.assertEqual((r.returncode, r.stdout), (40 + signal.SIGTERM, 'x\n'), 'SIGTERM forwarded, ignored SIGHUP kept ignored')
        self.assertEqual((signal.getsignal(signal.SIGTERM), signal.getsignal(signal.SIGHUP)), (mine, signal.SIG_IGN),
                         'handlers restored')


GATE_CHILD = r"""import pathlib, sys, time
sys.path.insert(0, sys.argv[1])
import _00_memory_gate as g
g.MEMINFO, g.POLL = pathlib.Path(sys.argv[2]), 0.05
d, name = pathlib.Path(sys.argv[3]), sys.argv[4]
with g.held(int(sys.argv[5]), name, out=sys.stdout):
    print('START', repr(time.monotonic()), flush=True)
    (d / f'{name}.started').touch()
    while not (d / f'{name}.release').exists(): time.sleep(0.02)
    print('END', repr(time.monotonic()), flush=True)
"""


def start_time(pid):
    """Field 22 of /proc/<pid>/stat, read by whitespace (the commands used here have no space in their name)."""
    return int(Path(f'/proc/{pid}/stat').read_text().split()[21])


class Gate(Base):
    """_00_memory_gate.py and its use in _00_memory_cap: hand-computed admissions on fake MemAvailable, fake /proc and
    cgroup trees, reservation files written here, and two real processes queueing on one gate file."""

    def setUp(self):
        super().setUp()
        self.path, self.meminfo = Path(os.environ['CP_MEMORY_GATE']), self.root / 'meminfo'
        for name, value in (('MEMINFO', self.meminfo), ('RESERVE_MB', 1000), ('POLL', 0.01)):
            p = patch.object(GATE, name, value)
            p.start()
            self.addCleanup(p.stop)
        self.free(100000)

    def free(self, mb):
        self.meminfo.write_text(f'MemTotal:       64000000 kB\nMemFree:          1000 kB\nMemAvailable:   {mb << 10} kB\nBuffers: 1 kB\n')

    def foreign(self, need, state='run'):
        """A reservation of a live process that is no ancestor of this one (a sleep this test owns)."""
        p = subprocess.Popen([sys.executable, '-c', 'import time; time.sleep(60)'])
        self.addCleanup(lambda: (p.kill(), p.wait()))
        return dict(pid=p.pid, start=start_time(p.pid), need=need, label='other', state=state, since=0.5)

    def test_gate_decide(self):
        e = lambda pid, since, state='wait', need=0, grow=0: dict(pid=pid, since=since, state=state, need=need, grow=grow)
        d = lambda free, me, others, mine=(): GATE.decide(free, me, [*others, me], lambda x: x['grow'], set(mine))
        me = e(10, 5.0, need=500)
        self.assertEqual(d(10 ** 6, me, [e(20, 4.0)]), (False, ''), 'an older live waiter goes first')
        self.assertEqual(d(10 ** 6, me, [e(9, 5.0)]), (False, ''), 'a tie goes to the lower pid')
        self.assertEqual(d(10 ** 6, me, [e(11, 5.0)]), (True, ''), 'a higher pid at the same instant queues behind')
        self.assertEqual(d(10 ** 6, me, [e(20, 6.0)]), (True, ''), 'a younger waiter never blocks')
        self.assertEqual(d(10 ** 6, me, [e(20, 4.0, 'run')]), (True, ''), 'an older running entry is no queue')
        run = [e(30, 1.0, 'run', 4000, 1500), e(31, 2.0, 'run', 3000, 1000)]  # remaining growth 1500 and 1000
        self.assertEqual(d(5000, e(10, 5.0, need=1500), run), (True, ''), '5000 - 2500 - 1500 = 1000 = RESERVE admits')
        self.assertEqual(d(4999, e(10, 5.0, need=1500), run), (False, ''), 'one MB less free waits')
        self.assertEqual(d(5000, e(10, 5.0, need=1501), run), (False, ''), 'one MB more needed waits')
        self.assertEqual(d(4000, e(10, 5.0, need=3000), [e(30, 1.0, 'run', 4000, 0)]), (True, ''),
                         'a running entry counts its remaining growth, not its need: 4000 - 0 - 3000 = 1000')
        self.assertEqual(d(1500, me, [e(20, 6.0, 'wait', 10 ** 6, 10 ** 6)]), (True, ''), "a waiter's growth never counts")
        self.assertEqual(d(None, me, run), (True, ''), 'no /proc/meminfo admits')
        self.assertEqual(d(100, me, run, mine={30, 31, 1}), (True, '100 MB available, 500 MB needed and 1000 MB kept free; '
                                                              '2 ancestor run(s) hold reservations, so it runs now'))
        alone = (True, '100 MB available, 500 MB needed and 1000 MB kept free; no other gated run is running, so it runs now')
        self.assertEqual(d(100, me, []), alone)
        self.assertEqual(d(100, me, [e(20, 6.0, need=10 ** 6)]), alone, 'only a younger waiter')
        self.assertEqual(d(100, me, run), (False, ''), 'others run and memory is short')
        self.assertEqual(d(100, me, run, mine={30}), (False, ''), 'one running entry is no ancestor')
        self.assertEqual(d(100, me, [e(20, 4.0)], mine={20}), (False, ''), 'the queue comes before the warnings')
        nested = [e(20, 4.0), e(30, 1.0, 'run')]  # 20 may wait for the very reservation 30 holds for me's run
        self.assertEqual(d(10 ** 6, me, nested, mine={30}), (True, ''), 'inside a running ancestor reservation: no queue')
        self.assertEqual(d(10 ** 6, me, nested), (False, ''), 'beside a foreign one: the queue holds')
        self.assertEqual(d(10 ** 6, me, [e(20, 4.0)], mine={20}), (False, ''), 'a waiting ancestor does not lift the queue')
        beat = dict(me, beat=100.0)
        self.assertEqual(d(10 ** 6, beat, [dict(e(20, 4.0), beat=70.0)]), (False, ''), 'a waiter that polled STALE = 30 s ago keeps its place')
        self.assertEqual(d(10 ** 6, beat, [dict(e(20, 4.0), beat=69.9)]), (True, ''), 'one silent for longer (stopped, hung) holds none')
        self.assertEqual(d(10 ** 6, beat, [e(20, 4.0)]), (True, ''), 'without a beat, its arrival counts as its last poll')

    def test_gate_need_and_recorded(self):
        self.assertEqual([GATE.need_of(p, 4096) for p in (0, 1, 9, 10, 1000, 3665, 3667, 9000)], [4096, 65, 73, 75, 1164, 4095, 4096, 4096])
        lt = self.root.resolve() / 'lt'
        main = lt / '00-Tools/x_tester.py'
        main.parent.mkdir(parents=True)
        main.write_text('')
        stats = lt / '.run-stats.json'
        with patch.object(GATE, 'STATS', stats), patch.object(GATE, 'HERE', lt):
            self.assertEqual(GATE.recorded('full', main, 3000), 3000, 'no stats file')
            stats.write_text(json.dumps({'full 00-Tools/x_tester.py': [1000, 2.5], 'quick 00-Tools/x_tester.py': [0, 1.0],
                                         'stress 00-Tools/x_tester.py': [-4, 1.0], 'full 00-Tools/y_tester.py': [], 'quick 00-Tools/y_tester.py': 'x',
                                         'stress 00-Tools/y_tester.py': ['12', 1]}))
            self.assertEqual(GATE.recorded('full', main, 3000), 1164, 'hit: 11/10 of 1000 plus 64')
            self.assertEqual(GATE.recorded('full', lt / '00-Tools/../00-Tools/x_tester.py', 3000), 1164, 'the resolved path')
            self.assertEqual(GATE.recorded('full', main, 900), 900, 'a peak whose share exceeds the cap reserves the cap')
            for mode, m in (('quick', main), ('stress', main), (None, main), ('full', lt / 'x_tester.py'), ('full', lt / '00-Tools/y_tester.py'),
                            ('quick', lt / '00-Tools/y_tester.py'), ('stress', lt / '00-Tools/y_tester.py'), ('full', self.root / 'x_tester.py')):
                self.assertEqual(GATE.recorded(mode, m, 3000), 3000, f'{mode} {m}: zero, negative or malformed peak, miss, or outside the folder')
            for text in ('not json', '[1, 2]', '"x"', '{"full 00-Tools/x_tester.py": 5}'):
                stats.write_text(text)
                self.assertEqual(GATE.recorded('full', main, 3000), 3000, text)

    def test_gate_store(self):
        for gate, xdg, want in (('0', '/x', None), ('/a/b.json', '/x', Path('/a/b.json')), ('', '/x', Path('/x/cp-memory-gate.json')),
                                (None, '/x', Path('/x/cp-memory-gate.json')), (None, None, Path(tempfile.gettempdir()) / 'cp-memory-gate.json')):
            with patch.dict(os.environ):
                for k, v in (('CP_MEMORY_GATE', gate), ('XDG_RUNTIME_DIR', xdg)):
                    if v is None:
                        os.environ.pop(k, None)
                    else:
                        os.environ[k] = v
                self.assertEqual(GATE.store(), want, (gate, xdg))

    def test_gate_load_and_stat(self):
        me = dict(pid=os.getpid(), start=start_time(os.getpid()), need=5, label='x', state='run', since=1.5)
        waiting = dict(me, state='wait', since=2)
        dead = subprocess.Popen([sys.executable, '-c', ''])
        dead.wait()
        self.assertIsNone(GATE.stat(dead.pid), 'a reaped pid')
        kid = self.foreign(1)
        self.assertEqual(GATE.stat(kid['pid']), (os.getpid(), kid['start']))
        line = GATE.lineage(kid['pid'], {c: p for p, cs in GATE.children().items() for c in cs})
        self.assertTrue({os.getpid(), os.getppid()} <= line and kid['pid'] not in line, line)
        bad = [dict(me, pid=dead.pid), dict(me, start=me['start'] + 1), dict(me, need='5'), dict(me, state='done'), dict(me, since='1'),
               {k: v for k, v in me.items() if k != 'start'}, dict(me, pid=float(os.getpid())), 5, None, [1], dict(me, beat='x'), dict(me, beat=None)]
        self.path.write_text(json.dumps([bad[0], me, *bad[1:5], waiting, *bad[5:]]))
        self.assertEqual(GATE.load(self.path), [me, waiting], 'dead, reused (start mismatch) and malformed entries dropped')
        for text in ('{"pid": 1}', '"x"', 'not json', '', '5'):
            self.path.write_text(text)
            self.assertEqual(GATE.load(self.path), [], text)
        self.path.unlink()
        self.assertEqual(GATE.load(self.path), [], 'no file')

    def test_gate_usage(self):
        proc, cg, page = self.root / 'proc', self.root / 'cgroup', os.sysconf('SC_PAGE_SIZE')
        scope = 'user.slice/user-1000.slice/user@1000.service/app.slice'

        def task(pid, ppid, group='user.slice/session-3.scope', mb=0, pss=None):
            d = proc / str(pid)
            d.mkdir(parents=True)
            (d / 'stat').write_text(f'{pid} (a) b) S {ppid} ' + '0 ' * 17 + '77 0 0\n')  # a name holding ') ', parsed from the last ')'
            (d / 'statm').write_text(f'9999 {(mb << 20) // page} 0 0 0 0 0\n')
            (d / 'cgroup').write_text(f'0::/{group}\n')
            if pss is not None:  # the kernel's rollup: Rss first, then Pss and Pss_* lines
                (d / 'smaps_rollup').write_text(f'5555-6666 ---p 00000000 00:00 0 [rollup]\nRss: {mb << 10} kB\nPss: {pss} kB\n'
                                                f'Pss_Anon: 7 kB\n')

        def current(name, mb, cache=None, shmem=0):
            (cg / scope / name).mkdir(parents=True)
            (cg / scope / name / 'memory.current').write_text(f'{(mb << 20) + 12345}\n')
            if cache is not None:  # memory.stat: anon, file (the page cache, tmpfs included), file_mapped, then shmem (tmpfs)
                (cg / scope / name / 'memory.stat').write_text(f'anon 5\nfile {cache << 20}\nfile_mapped 9\nshmem {shmem << 20}\n')
        for pid in (100, 200, 300, 400):
            task(pid, 1)
        task(101, 100, f'{scope}/run-a.scope', 999)
        task(102, 100, f'{scope}/run-b.scope', 999)
        task(103, 100, mb=999)  # beside scopes: its RSS counts
        current('run-a.scope', 300)
        current('run-b.scope', 200)
        task(201, 200, mb=10)
        task(202, 201, mb=20)
        task(203, 202, f'{scope}/run-z.scope', 5)  # a scope below a child: the tree's RSS still counts it
        current('run-z.scope', 4000)
        task(301, 300, f'{scope}/run-c.scope', 7)  # a run scope without memory.current
        task(302, 300, f'{scope}/not-run-d.scope', 8)
        current('not-run-d.scope', 4000)
        # a reservation held inside its own scope (04-mutation.py's campaign): that scope is the holder's, never counted; each
        # test scope below it counts once, however many of its processes the walk meets
        task(600, 1)  # SUP34: a scope's page cache and a process's shared pages are not what its run holds
        task(601, 600, f'{scope}/run-cache.scope')
        current('run-cache.scope', 900, cache=650)  # 900 MB + 12345 B less 650 MB of file cache: 250
        task(602, 600, f'{scope}/run-full.scope')
        current('run-full.scope', 40, cache=4000)  # more cache than memory.current (read at another instant): 0, never negative
        task(607, 600, f'{scope}/run-tmpfs.scope')
        current('run-tmpfs.scope', 500, cache=450, shmem=300)  # tmpfs files stay: 500 MB + 12345 B less 150 MB reclaimable: 350
        task(603, 600, mb=500, pss=120 << 10)  # PSS 120 MB counts, not RSS 500
        task(604, 600, mb=30, pss=(2 << 10) + 1023)  # kB truncated to MB once per process
        task(605, 600, mb=70)  # no smaps_rollup (another user's process, a zombie): its RSS
        (proc / '605' / 'smaps_rollup').mkdir()  # unreadable as a file
        task(606, 600, mb=80, pss=None)
        (proc / '606' / 'smaps_rollup').write_text('')  # empty (a kernel thread): its RSS
        task(500, 1, f'{scope}/run-own.scope')
        current('run-own.scope', 9999)
        task(501, 500, f'{scope}/run-own.scope', 50)
        task(502, 501, f'{scope}/run-t.scope', 1)
        task(503, 502, f'{scope}/run-t.scope', 1)
        task(504, 501, f'{scope}/run-t.scope', 1)
        current('run-t.scope', 300)
        task(505, 500, f'{scope}/run-u.scope', 1)
        current('run-u.scope', 70)
        (proc / 'self').mkdir()
        (proc / 'meminfo').write_text('x')
        with patch.object(GATE, 'PROC', proc), patch.object(GATE, 'CGROUP', cg):
            tree = GATE.children()
            self.assertEqual({k: sorted(v) for k, v in tree.items()}, {1: [100, 200, 300, 400, 500, 600], 100: [101, 102, 103], 200: [201], 201: [202],
                                                                      202: [203], 300: [301, 302], 500: [501, 505], 501: [502, 504], 502: [503],
                                                                      600: [601, 602, 603, 604, 605, 606, 607]})
            self.assertEqual([GATE.usage(p, tree) for p in (100, 200, 300, 400, 999, 500)], [300 + 200 + 999, 10 + 20 + 4000, 7 + 8, 0, 0, 50 + 300 + 70],
                             "each run scope below once (not the holder's own), else a descendant's RSS (no smaps_rollup here)")
            self.assertEqual(GATE.usage(600, tree), 250 + 0 + 350 + 120 + 2 + 70 + 80,
                             'scopes less their reclaimable file cache (not tmpfs), processes by PSS, else RSS')
            self.assertEqual([GATE.usage(100, tree, {101}), GATE.usage(200, tree, {202, 7}), GATE.usage(500, tree, {501})],
                             [200 + 999, 10, 70], 'a descendant holding its own running reservation, and its subtree, count under it')
            self.assertEqual(GATE.lineage(503, {c: p for p, cs in tree.items() for c in cs}), {502, 501, 500, 1})

    def test_gate_held(self):
        out = io.StringIO()
        with GATE.held(100, 'lbl', out=out):
            inside = json.loads(self.path.read_text())
        self.assertEqual(inside, [dict(pid=os.getpid(), start=start_time(os.getpid()), need=100, label='lbl', state='run', since=inside[0]['since'],
                                       beat=inside[0]['beat'], secs=0, admit=inside[0]['beat'])])
        self.assertTrue(abs(inside[0]['since'] - time.monotonic()) < 60 and inside[0]['since'] <= inside[0]['beat'] <= time.monotonic(),
                        'arrival and polls on CLOCK_MONOTONIC (time.monotonic())')
        with GATE.held(100, 'lbl', out=out, secs=12.5):
            self.assertEqual(json.loads(self.path.read_text())[0]['secs'], 12.5)
        self.assertEqual((json.loads(self.path.read_text()), out.getvalue()), ([], ''), 'released after a normal exit, silently')
        with self.assertRaises(ZeroDivisionError), GATE.held(100, 'lbl', out=out):
            self.assertEqual(len(GATE.load(self.path)), 1)
            1 / 0
        self.assertEqual(json.loads(self.path.read_text()), [], 'released after an exception')
        for x in self.root.glob('gate.json*'): x.unlink()
        for need, where in ((0, self.path), (-3, self.path), (100, '0')):
            ran = []
            with patch.dict(os.environ, CP_MEMORY_GATE=str(where)), patch.object(GATE, 'wait', side_effect=AssertionError('waited')), \
                    GATE.held(need, 'lbl', out=out):
                ran.append(1)
            self.assertEqual(ran, [1], (need, where))
        self.assertEqual(([x.name for x in self.root.glob('gate.json*')], out.getvalue()), ([], ''), 'need <= 0 or the gate off: no file, no output')

    def test_gate_held_warnings(self):
        self.free(500)
        out = io.StringIO()
        with GATE.held(100, 'lbl', out=out):
            pass
        self.assertEqual(out.getvalue(), 'WARNING lbl: memory gate: 500 MB available, 100 MB needed and 1000 MB kept free; no other gated '
                                         'run is running, so it runs now\n')
        parent = dict(pid=os.getppid(), start=start_time(os.getppid()), need=10 ** 6, label='parent', state='run', since=0.25)
        self.path.write_text(json.dumps([parent]))
        out = io.StringIO()
        with GATE.held(100, 'lbl', out=out):
            self.assertEqual([e['state'] for e in GATE.load(self.path)], ['run', 'run'])
        self.assertEqual(out.getvalue(), 'WARNING lbl: memory gate: 500 MB available, 100 MB needed and 1000 MB kept free; 1 ancestor run(s) '
                                         'hold reservations, so it runs now\n')
        self.assertEqual(GATE.load(self.path), [parent], "only its own entry released")

    def test_gate_held_growth(self):
        """held() through the real wait(): a running reservation holds back only its need less what its run uses now
        (5000 free, 1000 kept, need 3000 beside a foreign run of need 3000): using 2500 leaves 5000 - 500 - 3000 = 1500, so it
        starts; using 1500 leaves 500, so it queues (the poll's sleep stops the test)."""
        self.free(5000)
        other = self.foreign(3000)
        self.path.write_text(json.dumps([other]))
        stop = type('Stop', (Exception,), {})
        clock = type('Clock', (), {'time': staticmethod(time.time), 'monotonic': staticmethod(time.monotonic),
                                   'sleep': staticmethod(lambda s: (_ for _ in ()).throw(stop()))})
        for used, starts in ((2500, True), (1500, False)):
            out, ran = io.StringIO(), []
            with patch.object(GATE, 'usage', lambda pid, tree, runs=(), u=used: u if pid == other['pid'] else 0), patch.object(GATE, 'time', clock):
                try:
                    with GATE.held(3000, 'lbl', out=out):
                        ran.append(1)
                except stop:
                    pass
            self.assertEqual((ran, out.getvalue()), ([1], '') if starts else
                             ([], 'WAIT lbl: needs 3000 MB beside the running gated runs (_00_memory_gate.py); queued\n'), used)
            self.assertEqual(GATE.load(self.path), [other], used)

    def test_gate_old_clock(self):
        """An entry of pre-SUP34 code holds wall-clock since and beat: monotonic() moves it onto CLOCK_MONOTONIC when deciding,
        so an old waiter that arrived earlier keeps its place ahead of a new one and is not stale, one that arrived later
        queues behind, and the file keeps the old entry's own times."""
        w = GATE.WALL
        self.assertEqual(GATE.monotonic({'pid': 1, 'since': w + 1500, 'beat': w + 1600, 'admit': w + 1700}, w + 1000),
                         {'pid': 1, 'since': 500, 'beat': 600, 'admit': 700})
        self.assertEqual(GATE.monotonic({'pid': 1, 'since': w, 'beat': w}, w + 3600), {'pid': 1, 'since': -3600, 'beat': -3600},
                         'after a suspend the offset exceeds the time since boot: still shifted, now earlier')
        new = {'pid': 1, 'since': w - 1, 'beat': w - 1}
        self.assertIs(GATE.monotonic(new, 10.0), new, 'a monotonic entry as it is')
        self.free(10 ** 6)
        stop = type('Stop', (Exception,), {})
        clock = type('Clock', (), {'time': staticmethod(time.time), 'monotonic': staticmethod(time.monotonic),
                                   'sleep': staticmethod(lambda s: (_ for _ in ()).throw(stop()))})
        base = self.foreign(100, 'wait')
        for ahead, starts in ((-5, False), (5, True)):
            old = dict(base, since=time.time() + ahead, beat=time.time())
            self.path.write_text(json.dumps([old]))
            out, ran = io.StringIO(), []
            with patch.object(GATE, 'time', clock):
                try:
                    with GATE.held(100, 'lbl', out=out):
                        ran.append(1)
                except stop:
                    pass
            self.assertEqual(ran, [1] if starts else [], (ahead, out.getvalue()))
            self.assertEqual(GATE.load(self.path), [old], 'the old entry saved with its own times')

    def test_gate_beat_refreshed(self):
        """A queued waiter rewrites its beat with the monotonic clock at every poll while its since stays, so a waiter queued
        for longer than STALE keeps its place (a stepped time.monotonic(), 31 s per reading, the wall clock left real: since,
        beat and the WAIT timer read it; the third poll's sleep stops the test)."""
        self.free(500)
        other = self.foreign(10 ** 6)
        self.path.write_text(json.dumps([other]))
        stop, now, seen = type('Stop', (Exception,), {}), [1000.0], []

        def tick():
            now[0] += 31
            return now[0]

        def sleep(s):
            seen.append(next((e['since'], e['beat'], e['state']) for e in GATE.load(self.path) if e['pid'] == os.getpid()))
            if len(seen) == 3:
                raise stop()
        clock = type('Clock', (), {'time': staticmethod(time.time), 'monotonic': staticmethod(tick), 'sleep': staticmethod(sleep)})
        with patch.object(GATE, 'time', clock), self.assertRaises(stop), GATE.held(100, 'lbl', out=io.StringIO()):
            pass
        self.assertEqual(seen, [(1031.0, 1062.0, 'wait'), (1031.0, 1124.0, 'wait'), (1031.0, 1155.0, 'wait')])
        self.assertEqual(GATE.load(self.path), [other])

    def test_gate_deadline(self):
        """held(until=...) beside a foreign run no memory covers: past the deadline it prints WAIT then STOP, removes its
        entry before the block and yields False; admitted, or with the gate off, it yields True."""
        self.free(500)
        other = self.foreign(10 ** 6)
        self.path.write_text(json.dumps([other]))
        out, polls, stop = io.StringIO(), [], type('Stop', (Exception,), {})

        def sleep(s):  # bounded: a wait that ignored its deadline would poll for ever
            polls.append(s)
            if len(polls) > 200:
                raise stop()
            time.sleep(s)
        clock = type('Clock', (), {'time': staticmethod(time.time), 'monotonic': staticmethod(time.monotonic), 'sleep': staticmethod(sleep)})
        with patch.object(GATE, 'time', clock), GATE.held(100, 'lbl', out=out, until=time.monotonic() + 0.05) as ok:
            inside = GATE.load(self.path)
        self.assertEqual((ok, inside, GATE.load(self.path)), (False, [other], [other]))
        self.assertEqual(out.getvalue(), 'WAIT lbl: needs 100 MB beside the running gated runs (_00_memory_gate.py); queued\n'
                                         'STOP lbl: its deadline passed in the memory queue\n')
        self.path.write_text('[]')
        self.free(100000)
        for need, until in ((100, time.monotonic() + 600), (100, time.monotonic() - 1), (0, time.monotonic() - 1)):
            with GATE.held(need, 'lbl', out=io.StringIO(), until=until) as ok:
                self.assertTrue(ok, (need, until))

    def test_gate_interrupt_while_queued(self):
        """Ctrl-C during the queue's poll: KeyboardInterrupt propagates and the waiting entry is gone, so a live process that
        handles it blocks no later waiter."""
        self.free(500)
        other = self.foreign(10 ** 6)
        self.path.write_text(json.dumps([other]))
        clock = type('Clock', (), {'time': staticmethod(time.time), 'monotonic': staticmethod(time.monotonic),
                                   'sleep': staticmethod(lambda s: (_ for _ in ()).throw(KeyboardInterrupt))})
        out, ran = io.StringIO(), []
        with patch.object(GATE, 'time', clock), self.assertRaises(KeyboardInterrupt), GATE.held(100, 'lbl', out=out):
            ran.append(1)
        self.assertEqual((ran, out.getvalue()), ([], 'WAIT lbl: needs 100 MB beside the running gated runs (_00_memory_gate.py); queued\n'))
        self.assertEqual(GATE.load(self.path), [other], 'the queued entry is removed; the other one stays')

    def test_gate_unusable_file(self):
        (self.root / 'file').write_text('')
        cases = [(self.root / 'file/gate.json', 'File exists')]
        if os.geteuid():
            ro = self.root / 'ro'
            ro.mkdir(mode=0o500)
            self.addCleanup(ro.chmod, 0o700)
            cases.append((ro / 'gate.json', 'Permission denied'))
        for where, why in cases:
            out, ran = io.StringIO(), []
            with patch.dict(os.environ, CP_MEMORY_GATE=str(where)), GATE.held(100, 'lbl', out=out):
                ran.append(1)
            self.assertEqual((ran, out.getvalue()), ([1], f'WARNING lbl: memory gate unavailable ({why}); running without it\n'), where)

    def test_gate_error_after_save(self):
        """An OSError after wait() saved the queued entry (a full disk, a lost lock file): the run goes ahead with a WARNING
        and its entry is removed before the block runs, so it never blocks a later waiter."""
        other = self.foreign(10)

        def wait(path, me, label, out, until=None, state=None):
            GATE.save(path, json.dumps([other, me]))
            raise OSError(errno.EIO, 'Input/output error')
        out, inside = io.StringIO(), []
        with patch.object(GATE, 'wait', wait), GATE.held(100, 'lbl', out=out):
            inside.append(GATE.load(self.path))
        self.assertEqual((inside, out.getvalue()), ([[other]], 'WARNING lbl: memory gate unavailable (Input/output error); running without it\n'))
        self.assertEqual(GATE.load(self.path), [other])

    def test_mode_of(self):
        """An explicit --mode wins; CP_TEST_MODE and the default 'full' apply only to an entry that reads CP_TEST_MODE, so an
        exported mode starts no pre-flight for one that ignores it (06-compare.py under CP_TEST_MODE=full)."""
        for env, argv, honours, want in ((None, [], True, ('full', 'env')), (None, [], False, (None, 'env')),
                                         ('quick', [], True, ('quick', 'env')), ('full', [], False, (None, 'env')),
                                         ('full', ['--mode', 'stress'], False, ('stress', 'argv')), ('full', ['--mode=quick'], True, ('quick', 'argv')),
                                         ('quick', ['run', 'x', '--mode'], True, ('quick', 'env'))):
            with patch.dict(os.environ):
                os.environ.pop('CP_TEST_MODE', None)
                if env:
                    os.environ['CP_TEST_MODE'] = env
                self.assertEqual(_00_memory_cap.mode_of(argv, honours), want, (env, argv, honours))

    def standalone_mode(self, rel):
        """A standalone tester (no shared runner) reads CP_TEST_MODE as the others do (fixed 2026-11-24): ensure() on the
        real file hands supervise() the exported mode, the 'full' default or an explicit --mode, which wins; the tester
        itself rejects a bad --mode before any work and defaults to the exported one; a real full run gets the main cap
        but no pre-flight, since every mode runs the same checks (user decision 2026-11-24), and as 01-run.py's child
        records its peak, here in a temporary .run-stats.json (CP_RUN_STATS), never the real one."""
        main, calls = LT / rel, []
        with patch.dict(os.environ), patch.object(sys.modules['__main__'], '__file__', str(main)), \
                patch.object(_00_memory_cap, 'supervise', lambda *a: calls.append(a[1:3]) or 0):
            os.environ.pop('CP_MEMORY_CAPPED', None)
            for env, argv in (('full', []), ('stress', []), (None, []), ('full', ['--mode', 'quick']), ('quick', ['--mode=stress'])):
                os.environ.pop('CP_TEST_MODE', None)
                if env:
                    os.environ['CP_TEST_MODE'] = env
                with patch.object(sys, 'argv', [str(main), *argv]), self.assertRaises(SystemExit):
                    _00_memory_cap.ensure()
        self.assertEqual(calls, [('full', 'env'), ('stress', 'env'), ('full', 'env'), ('quick', 'argv'), ('stress', 'argv')])
        env = {k: v for k, v in os.environ.items() if not k.startswith('CP_TEST_')} | dict(CP_MEMORY_CAPPED='none', CP_TEST_MODE='stress')
        bad = subprocess.run([sys.executable, str(main), '--mode', 'bogus'], env=env, capture_output=True, text=True, timeout=60)
        self.assertEqual((bad.returncode, bad.stdout), (2, ''), bad.stderr)
        self.assertIn("invalid choice: 'bogus'", bad.stderr)
        usage = subprocess.run([sys.executable, str(main), '--help'], env=env, capture_output=True, text=True, timeout=60)
        self.assertEqual(usage.returncode, 0, usage.stderr)
        self.assertIn('(default stress)', ' '.join(usage.stdout.split()), 'the tester defaults to CP_TEST_MODE')
        env.pop('CP_MEMORY_CAPPED')
        with tempfile.TemporaryDirectory() as tmp:
            stats = Path(tmp) / 'stats.json'
            run = subprocess.run([sys.executable, str(main)], env=dict(env, CP_TEST_MODE='full', CP_RUN_STATS=str(stats)), capture_output=True, text=True, timeout=300)
            self.assertEqual(run.returncode, 0, run.stdout[-2000:] + run.stderr[-2000:])
            record = json.loads(stats.read_text())[f'full {rel}']
            self.assertEqual(len(record[2]), 1, 'one run recorded, in the temporary file')
        self.assertNotIn('PREFLIGHT', run.stdout, 'every mode runs the same checks: no quick pre-flight runs them twice')
        self.assertIn('MEMORY peak=', run.stdout, 'still under the main cap')

    def test_mode_online_tester(self):
        self.standalone_mode('00-Tools/02-online_tester.py')

    def test_mode_notebook_tester(self):
        self.standalone_mode('00-Tools/03-notebook_tester.py')

    def test_mode_debug_compile_tester(self):
        self.standalone_mode('01-Core/02-debug_compile_tester.py')

    def test_gate_entry_names(self):
        """Every tracked script under 96-Local Testing that calls ensure() without force=True has a name ensure() caps (a
        tester or benchmark suffix, or ENTRY_NAMES): otherwise the call returns uncapped and ungated, as 06-compare.py's did
        until SUP33. Shared runners (a leading `_`) are imported by testers, whose names count; a call is a statement of its own
        line (03-consistency.py names the call inside a string)."""
        files = subprocess.run(['git', 'ls-files', '-z', '--', '*.py'], cwd=LT, capture_output=True, text=True, check=True).stdout.split('\0')
        callers = [LT / f for f in files if f and not Path(f).name.startswith('_') and (LT / f).is_file()
                   and any('force=True' not in c for c in re.findall(r'^[ \t]*_00_memory_cap\.ensure\(([^)]*)\)', (LT / f).read_text(errors='replace'), re.M))]
        self.assertIn(LT / '06-compare.py', callers, 'the case that motivated this test is found')
        self.assertEqual([p for p in callers if not (p.name.endswith(_00_memory_cap.ENTRY_SUFFIXES) or p.name in _00_memory_cap.ENTRY_NAMES)], [])

    def test_gate_two_processes(self):
        """A is admitted (2000 - 900 >= 1000); B (2000 - 900 - 900 < 1000 beside A) prints WAIT, starts only after A released its
        reservation, then prints ADMIT."""
        self.meminfo.write_text(f'MemAvailable:   {2000 << 10} kB\n')
        env = dict(os.environ, CP_MEMORY_GATE_RESERVE_MB='1000')
        procs = {}

        def start(name):
            with open(self.root / f'{name}.out', 'w') as out:
                procs[name] = p = subprocess.Popen([sys.executable, '-c', GATE_CHILD, str(LT), str(self.meminfo), str(self.root), name, '900'],
                                                   env=env, stdout=out, stderr=subprocess.STDOUT)
            self.addCleanup(lambda: p.poll() is None and (p.kill(), p.wait()))

        def until(cond, what):
            deadline = time.monotonic() + 30
            while not cond() and time.monotonic() < deadline: time.sleep(0.02)
            self.assertTrue(cond(), f"{what}: a {(self.root / 'a.out').read_text()!r} b {(self.root / 'b.out').read_text() if 'b' in procs else ''!r}")
        start('a')
        until((self.root / 'a.started').exists, 'A admitted')
        start('b')
        until(lambda: 'WAIT' in (self.root / 'b.out').read_text(), 'B queued')
        time.sleep(0.5)  # ten polls of B
        self.assertFalse((self.root / 'b.started').exists(), 'B runs beside A')
        self.assertEqual(sorted((e['label'], e['state'], e['need']) for e in GATE.load(self.path)), [('a', 'run', 900), ('b', 'wait', 900)])
        (self.root / 'b.release').touch()
        (self.root / 'a.release').touch()
        self.assertEqual([procs[n].wait(30) for n in 'ab'], [0, 0])
        a, b = ((self.root / f'{n}.out').read_text().splitlines() for n in 'ab')
        self.assertEqual([x.split()[0] for x in a], ['START', 'END'], a)
        self.assertEqual(b[0], 'WAIT b: needs 900 MB beside the running gated runs (_00_memory_gate.py); queued', b)
        self.assertRegex(b[1], r'^ADMIT b: after \d+ s in the memory queue$')
        self.assertEqual([x.split()[0] for x in b[2:]], ['START', 'END'], b)
        self.assertLess(float(a[1].split()[1]), float(b[2].split()[1]), 'B started after A ended')
        self.assertEqual(json.loads(self.path.read_text()), [], 'both released')

    def test_gate_supervise_and_ensure_need(self):
        seen = []

        def capped_run(command, mode, source, label, peaks):
            seen.append((label, [(e['need'], e['state'], e['label']) for e in GATE.load(self.path)]))
            return 5
        with patch.object(_00_memory_cap, 'capped_run', capped_run), patch.object(_00_memory_cap, 'MAIN_MB', 300):
            codes = [_00_memory_cap.supervise(['x', 'y'], None, 'env', 'a', need=77), _00_memory_cap.supervise(['x', 'y'], None, 'env', 'b'),
                     _00_memory_cap.supervise(['x', 'y'], None, 'env', 'c', need=0), _00_memory_cap.supervise(['x', 'y'], None, 'env', 'd', need=0, alone=True)]
        self.assertEqual((codes, seen), ([5, 5, 5, 5], [('a', [(77, 'run', 'a')]), ('b', [(300, 'run', 'b')]), ('c', []), ('d', [(0, 'run', 'd')])]),
                         'need passed through, the main cap by default, 0 ungated unless alone')
        self.assertEqual(GATE.load(self.path), [])
        lt = self.root.resolve() / 'lt'
        main = lt / '00-Tools/x_tester.py'
        main.parent.mkdir(parents=True)
        main.write_text("os.environ.get('CP_TEST_MODE')\n")
        (lt / '.run-stats.json').write_text(json.dumps({'full 00-Tools/x_tester.py': [1000, 2.0]}))
        calls = []
        with patch.dict(os.environ), patch.object(sys.modules['__main__'], '__file__', str(main)), \
                patch.object(GATE, 'STATS', lt / '.run-stats.json'), patch.object(GATE, 'HERE', lt), patch.object(_00_memory_cap, 'MAIN_MB', 3000), \
                patch.object(_00_memory_cap, 'supervise', lambda *a: calls.append(a) or 3):
            os.environ.pop('CP_MEMORY_CAPPED', None)
            for argv, need in ((['--mode', 'full'], None), (['--mode', 'quick'], None), (['--mode', 'full'], 0), (['--mode', 'full'], 55)):
                with patch.object(sys, 'argv', [str(main), *argv]), self.assertRaises(SystemExit) as stop:
                    _00_memory_cap.ensure(need=need)
                self.assertEqual(stop.exception.code, 3)
        bench = lt / '00-Tools/x_benchmark.py'  # a benchmark driver runs alone (user decision 2026-11-26)
        bench.write_text("os.environ.get('CP_TEST_MODE')\n")
        with patch.dict(os.environ), patch.object(sys.modules['__main__'], '__file__', str(bench)), \
                patch.object(GATE, 'STATS', lt / '.run-stats.json'), patch.object(GATE, 'HERE', lt), patch.object(_00_memory_cap, 'MAIN_MB', 3000), \
                patch.object(_00_memory_cap, 'supervise', lambda *a: calls.append(a) or 3):
            os.environ.pop('CP_MEMORY_CAPPED', None)
            with patch.object(sys, 'argv', [str(bench), '--mode', 'full']), self.assertRaises(SystemExit):
                _00_memory_cap.ensure()
        self.assertEqual(calls.pop(), ([sys.executable, str(bench), '--mode', 'full'], 'full', 'argv', 'x_benchmark.py', 3000,
                                       'full 00-Tools/x_benchmark.py', True), 'a benchmark: its key, alone')
        integration = lt / '02-integration.py'  # an entry point that is no tester or benchmark, without a variant: its cap, no key
        integration.write_text("os.environ.get('CP_TEST_MODE')\n")
        (lt / '.run-stats.json').write_text(json.dumps({'full 00-Tools/x_tester.py': [1000, 2.0], 'full 02-integration.py': [100, 1.0]}))
        with patch.dict(os.environ), patch.object(sys.modules['__main__'], '__file__', str(integration)), \
                patch.object(GATE, 'STATS', lt / '.run-stats.json'), patch.object(GATE, 'HERE', lt), patch.object(_00_memory_cap, 'MAIN_MB', 3000), \
                patch.object(_00_memory_cap, 'supervise', lambda *a: calls.append(a) or 3):
            os.environ.pop('CP_MEMORY_CAPPED', None)
            with patch.object(sys, 'argv', [str(integration), '--mode', 'full']), self.assertRaises(SystemExit):
                _00_memory_cap.ensure()
        self.assertEqual(calls.pop(), ([sys.executable, str(integration), '--mode', 'full'], 'full', 'argv', '02-integration.py', 3000, None, False),
                         'a record under no key it writes is never read')
        self.assertEqual(calls, [([sys.executable, str(main), '--mode', m], m, 'argv', 'x_tester.py', n, k, False)
                                 for m, n, k in (('full', 1164, 'full 00-Tools/x_tester.py'), ('quick', 3000, 'quick 00-Tools/x_tester.py'),
                                                 ('full', 0, None), ('full', 55, None))],
                         'the recorded need (11/10 of 1000 plus 64), the cap when unrecorded, or the given one; the key to record '
                         'under only with a recorded need')

    def test_gate_cli_need(self):
        usage = 'usage: _00_memory_cap.py [--mb N] [--need N] [--alone] -- <command> [args...]'
        bad = usage + '\n--mb needs an integer of at least 16, --need a nonnegative integer'
        env = {k: v for k, v in os.environ.items() if k not in ('CP_MEMORY_CAPPED', 'CP_TEST_MEMORY_MB')}
        cli = lambda *args, **extra: subprocess.run([sys.executable, LT / '_00_memory_cap.py', *args], capture_output=True, text=True,
                                                    timeout=60, env=dict(env, **extra))
        for args in (['--need'], ['--need', '-1', '--', 'true'], ['--need=x', '--', 'true'], ['--need=', '--', 'true'],
                     ['--mb', '64', '--need', '1.5', '--', 'true'], ['--need', '5', '--mb', '15', '--', 'true'], ['--mb=8', '--need=1', '--', 'true']):
            r = cli(*args)
            self.assertEqual((r.returncode, r.stderr.strip(), r.stdout), (1, bad, ''), args)
        r = cli('--need', '5', '--mb', '64')
        self.assertEqual((r.returncode, r.stderr.strip()), (1, usage), 'no command')
        r = cli('--alone')
        self.assertEqual((r.returncode, r.stderr.strip()), (1, usage), 'no command after --alone')
        probe = [sys.executable, '-c', 'import json, os; p = os.environ["CP_MEMORY_GATE"]; '
                 'print(json.dumps([[e["need"], e["state"], e["label"], e.get("excl", False)] for e in json.load(open(p))]) if os.path.exists(p) else "absent")']
        me = Path(sys.executable).name
        for k, (args, want) in enumerate(((['--need', '7', '--mb', '64', '--'], [[7, 'run', me, False]]), (['--mb=64', '--need=12', '--'], [[12, 'run', me, False]]),
                                          (['--mb', '64', '--'], [[64, 'run', me, False]]), (['--need=9'], [[9, 'run', me, False]]),
                                          (['--mb', '64', '--need', '0', '--'], 'absent'), (['--alone', '--need', '7', '--'], [[7, 'run', me, True]]),
                                          (['--mb', '64', '--alone', '--'], [[64, 'run', me, True]]), (['--need=0', '--alone', '--'], [[0, 'run', me, True]]))):
            gate = self.root / f'cli{k}.json'
            r = cli(*args, *probe, CP_MEMORY_GATE=str(gate))
            self.assertEqual((r.returncode, r.stdout.splitlines()[:1]), (0, [json.dumps(want) if want != 'absent' else want]), f'{args}: {r.stderr}')
            if want != 'absent':
                self.assertEqual(json.loads(gate.read_text()), [], 'released')

    def test_gate_nested_decide(self):
        """SUP34: a run inside a running ancestor's reservation needs its whole need beside it (a recorded peak covers only
        its own scope), however much of the ancestor's is unused; when it does not fit and every running reservation waits for a queued run below it
        (two sessions' nests), the oldest such queued run starts with a WARNING, the others wait."""
        e = lambda pid, since, state='wait', need=0, grow=0, above=(): dict(pid=pid, since=since, state=state, need=need, grow=grow, above=set(above))
        d = lambda free, me, others, mine=(): GATE.decide(free, me, [*others, me], lambda x: x['grow'], set(mine), lambda x: x['above'])
        run = [e(30, 1.0, 'run', 4000, 1000), e(31, 2.0, 'run', 4000, 1000)]  # spare at 3500 free: 3500 - 2000 - 1000 = 500
        for need, mine, want in ((800, {30}, False), (501, {30}, False), (500, {30}, True), (800, (), False), (501, (), False),
                                 (500, (), True), (501, {30, 31}, True)):  # below both, only its ancestors run: the WARNING path
            got = d(3500, e(10, 5.0, need=need), run, mine)
            self.assertEqual((got[0], bool(got[1])), (want, want and need > 500), f'need {need} below {mine}: nothing drawn from 1000 unused')
        warn = (True, '100 MB available, 500 MB needed and 1000 MB kept free; every running gated run waits for a queued run below it, '
                      'so the oldest of those runs now')
        me = e(10, 5.0, need=500)
        self.assertEqual(d(100, me, [*run, e(20, 6.0, above={31, 1})], {30, 1}), warn, 'both nests wait below: the older queued run starts')
        self.assertEqual(d(100, me, [*run, e(20, 4.0, above={31, 1})], {30, 1}), (False, ''), 'the older one of the other nest goes first')
        self.assertEqual(d(100, me, [*run, e(20, 4.0, above={1})], {30, 1}), (False, ''), 'an older top-level waiter is not nested: it does not count')
        self.assertEqual(d(100, me, [*run, e(20, 6.0, above={1})], {30, 1}), (False, ''), '31 waits for nothing queued: it will end')
        self.assertEqual(d(100, me, [*run, e(20, 6.0, above={31}), e(32, 3.0, 'run', 10, 10)], {30}), (False, ''),
                         'a running nested run (32) will end: no deadlock')
        stale = dict(e(20, 6.0, above={31}), beat=5.0 - 31)
        self.assertEqual(d(100, dict(me, beat=5.0), [*run, stale], {30}), (False, ''), 'a stale waiter blocks nothing')
        self.assertEqual(d(100, e(10, 5.0, need=500), [*run, e(20, 6.0, above={31}), e(21, 7.0, above={30})]), (False, ''),
                         'a top-level run never breaks the nests')

    def test_gate_exclusive(self):
        """An exclusive (timed) run, user decision 2026-11-26: it starts once only its ancestors and runs idle below a waiting
        exclusive run are running and no older exclusive run waits, passing older ordinary waiters; while it waits no younger
        top-level run starts, while it runs only its descendants do."""
        e = lambda pid, since, state='wait', need=0, grow=0, excl=False, above=(), **kw: dict(
            pid=pid, since=since, state=state, need=need, grow=grow, above=set(above), **({'excl': True} if excl else {}), **kw)
        d = lambda free, me, others, mine=(): GATE.decide(free, me, [*others, me], lambda x: x['grow'], set(mine), lambda x: x['above'])
        holds, ahead = (False, 'an exclusive (timed) run holds the machine'), (False, 'an exclusive (timed) run waits ahead of it')
        x_run, me = e(30, 1.0, 'run', excl=True), e(10, 5.0, need=100)
        self.assertEqual(d(10 ** 6, me, [x_run]), holds, 'a running exclusive run blocks any other run')
        self.assertEqual(d(10 ** 6, dict(me, excl=True), [x_run]), holds, 'another exclusive one too')
        self.assertEqual(d(10 ** 6, me, [x_run, e(31, 0.5, 'run')], {31}), holds, 'a run inside another reservation too')
        self.assertEqual(d(10 ** 6, me, [x_run], {30}), (True, ''), 'its own descendants run')
        xme = e(10, 5.0, excl=True)
        self.assertEqual(d(10 ** 6, xme, [e(30, 1.0, 'run', 100)]), (False, ''), 'it waits for a running ordinary run')
        self.assertEqual(d(10 ** 6, xme, [e(30, 1.0, 'run', 100)], {30}), (True, ''), 'beside its own ancestor only, it starts')
        self.assertEqual(d(10 ** 6, xme, [e(20, 4.0, need=10 ** 6)]), (True, ''), 'it passes an older ordinary waiter')
        self.assertEqual(d(10 ** 6, xme, [e(20, 4.0, excl=True)]), (False, ''), 'never an older exclusive one')
        self.assertEqual(d(10 ** 6, xme, [e(20, 6.0, excl=True)]), (True, ''), 'a younger exclusive waiter is no queue')
        stale = dict(e(20, 4.0, excl=True), beat=5.0 - 31)
        self.assertEqual(d(10 ** 6, dict(xme, beat=5.0), [stale]), (True, ''), 'a stale exclusive waiter holds no place')
        self.assertEqual(d(10 ** 6, dict(me, beat=5.0), [stale]), (True, ''), 'nor holds an ordinary run back')
        p1, p2 = e(30, 1.0, 'run', 4000, 100), e(31, 2.0, 'run', 4000, 100)  # two sessions' parents, each with an exclusive run below
        older, younger = e(10, 5.0, excl=True, above={30}), e(20, 6.0, excl=True, above={31})
        self.assertEqual(d(10 ** 6, older, [p1, p2, younger], {30}), (True, ''), "the other's parent idles below its waiting exclusive run")
        self.assertEqual(d(10 ** 6, younger, [p1, p2, older], {31}), (False, ''), 'the younger one waits for the older')
        self.assertEqual(d(10 ** 6, younger, [p1, p2, dict(older, state='run')], {31}), holds, 'then for its run')
        self.assertEqual(d(10 ** 6, older, [p1, p2, e(21, 7.0, above={31})], {30}), (False, ''), 'below an ordinary waiter it is busy')
        r_a, r_b = e(30, 1.0, 'run', 14000, 13800), e(31, 2.0, 'run', 4096, 3096)  # the reviewer's cycle: A's exclusive pass waits for B,
        x_a, c_b = e(40, 3.0, excl=True, above={30}), e(41, 4.0, need=3000, above={31})  # B's younger nested build for memory
        self.assertEqual(d(16000, x_a, [r_a, r_b, c_b], {30}), (False, ''), 'B is busy below its ordinary waiter')
        self.assertEqual(d(16000, c_b, [r_a, r_b, x_a], {31}), (True, '16000 MB available, 3000 MB needed and 1000 MB kept free; every running '
                         'gated run waits for a queued run below it, so the oldest of those runs now'), 'the nested build breaks the cycle')
        self.assertEqual(d(10 ** 6, dict(me, secs=1), [e(20, 4.0, excl=True), e(30, 1.0, 'run', 100, 0)]), ahead,
                         'an ordinary run waits behind an older exclusive waiter even when backfill fits')
        self.assertEqual(d(10 ** 6, me, [e(20, 4.0, excl=True)]), ahead, 'with nothing running too')
        self.assertEqual(d(10 ** 6, me, [e(20, 4.0, excl=True, above={31}), e(31, 1.0, 'run')], {31}), (True, ''),
                         'inside a running reservation it starts: that reservation must end before the exclusive run anyway')
        self.assertEqual(d(10 ** 6, me, [e(20, 6.0, excl=True)]), (True, ''), 'a younger exclusive waiter does not hold an older run back')
        self.assertEqual(d(1500, xme, []), (True, ''), 'need 0 fits whenever MemAvailable covers the reserve')
        self.assertEqual(d(500, xme, []), (True, '500 MB available, 0 MB needed and 1000 MB kept free; it runs alone now'))
        self.assertEqual(d(500, dict(xme, need=100), [e(30, 1.0, 'run', 100)], {30}),
                         (True, '500 MB available, 100 MB needed and 1000 MB kept free; it runs alone now'), 'inside an ancestor too')
        out = io.StringIO()
        with GATE.held(0, 'timed', out=out, exclusive=True):
            inside = json.loads(self.path.read_text())
        self.assertEqual([(x['need'], x['state'], x.get('excl')) for x in inside], [(0, 'run', True)], 'need 0 still enters the gate, marked excl')
        with GATE.held(-5, 'timed', out=out, exclusive=True):
            self.assertEqual(json.loads(self.path.read_text())[0]['need'], 0, 'a negative need counts as 0')
        with GATE.held(0, 'plain', out=out):
            self.assertEqual(json.loads(self.path.read_text()), [], 'an ordinary need 0 stays ungated')
        self.assertEqual((json.loads(self.path.read_text()), out.getvalue()), ([], ''))
        other = self.foreign(100)
        self.path.write_text(json.dumps([other]))
        with GATE.held(0, 'timed', out=out, until=time.monotonic() + 0.2, exclusive=True) as ok:
            self.assertFalse(ok)
        self.assertIn('WAIT timed: waits until no other gated run runs, to be timed alone (_00_memory_gate.py); queued', out.getvalue())
        self.path.write_text(json.dumps([dict(other, excl=True)]))
        out = io.StringIO()
        with GATE.held(100, 'plain', out=out, until=time.monotonic() + 0.2) as ok:
            self.assertFalse(ok)
        self.assertIn('WAIT plain: an exclusive (timed) run holds the machine (_00_memory_gate.py); queued', out.getvalue())
        self.assertEqual(GATE.load(self.path), [dict(other, excl=True)], 'the entry kept whole, its excl key included')

    def test_gate_main_checkout(self):
        """.run-stats.json is the main checkout's in every linked worktree (user decision 2026-11-26)."""
        main, other = self.root.resolve() / 'repo', self.root.resolve() / 'plain'
        other.mkdir()
        run = lambda *a, cwd=main: subprocess.run(['git', *a], cwd=cwd, check=True, capture_output=True)
        main.mkdir()
        run('init', '-q', '-b', 'master')
        run('-c', 'user.name=u', '-c', 'user.email=u@example.invalid', 'commit', '-q', '--allow-empty', '-m', 'init')
        run('worktree', 'add', '-q', '-b', 'w', str(main / '.claude/worktrees/W'))
        wt = main / '.claude/worktrees/W'
        self.assertEqual([GATE.main_checkout(p) for p in (main, wt, other)], [main, main, other])
        link = (wt / '.git').read_text()
        (wt / '.git').write_text('gitdir: /nonexistent\n')
        self.assertEqual(GATE.main_checkout(wt), wt, 'an unreadable git directory: the checkout itself')
        (wt / '.git').write_text(link)
        here = wt / '96-Local Testing'
        here.mkdir()
        shutil.copy(LT / '_00_memory_gate.py', here)
        code = 'import sys; sys.path.insert(0, sys.argv[1]); import _00_memory_gate as g; print(g.STATS)'
        env = {k: v for k, v in os.environ.items() if k != 'CP_RUN_STATS'}
        got = subprocess.run([sys.executable, '-c', code, str(here)], capture_output=True, text=True, env=env, timeout=60)
        self.assertEqual(got.stdout.strip(), str(main / '96-Local Testing/.run-stats.json'), got.stderr)
        got = subprocess.run([sys.executable, '-c', code, str(here)], capture_output=True, text=True, env=dict(env, CP_RUN_STATS='/x/s.json'), timeout=60)
        self.assertEqual(got.stdout.strip(), '/x/s.json', 'CP_RUN_STATS wins')

    def test_gate_backfill(self):
        """SUP34: behind an older waiter (the head), a run starts at once when it fits now and its recorded seconds end before
        the head is expected to fit: running reservations end at admit + secs (now when unrecorded) and free their held plus
        their remaining growth (5000 free, 1000 kept)."""
        e = lambda pid, since, state='wait', need=0, grow=0, use=0, **kw: dict(pid=pid, since=since, state=state, need=need, grow=grow, use=use, **kw)
        d = lambda free, me, others, now=120.0: GATE.decide(free, me, [*others, me], lambda x: x['grow'], set(), used=lambda x: x['use'], now=now)
        head = e(20, 1.0, need=3000)
        r = e(30, 0.5, 'run', 3000, 1000, 2000, admit=100.0, secs=50)  # spare 5000 - 1000 - 1000 = 3000; the head is 0 short: fits
        self.assertEqual(d(5000, e(10, 5.0, need=500, secs=1), [head, r]), (False, ''), 'the head fits now: it goes next, nobody passes')
        r = dict(r, grow=2000)  # spare 2000, the head 1000 short until r ends at 150 freeing 2000 + 2000
        for secs, need, want in ((30, 500, True), (30, 2000, True), (30.5, 500, False), (31, 500, False), (0, 500, False), (30, 2001, False)):
            self.assertEqual(d(5000, e(10, 5.0, need=need, secs=secs), [head, r]), (want, ''), f'{secs} s and {need} MB against the head at 150')
        self.assertEqual(d(5000, e(10, 5.0, need=500), [head, r]), (False, ''), 'no recorded seconds: no backfill')
        self.assertEqual(d(None, e(10, 5.0, need=500, secs=1), [head, r]), (False, ''), 'without MemAvailable the head is admitted itself')
        self.assertEqual(d(5000, e(10, 5.0, need=500, secs=1), [head, {k: v for k, v in r.items() if k != 'secs'}]), (False, ''),
                         'an unrecorded running reservation may end now')
        small = [dict(r, grow=1000, use=0, need=1000), dict(r, pid=31, grow=1000, use=0, need=1000, admit=150.0, secs=50)]
        for secs, want in ((80, True), (80.1, False)):  # spare 4000 - 2000 - 1000 = 1000, the head 2000 short: both must end (200)
            self.assertEqual(d(4000, e(10, 5.0, need=500, secs=secs), [head, *small]), (want, ''), secs)
        self.assertEqual(d(5000, e(10, 5.0, need=500, secs=1), [head, dict(head, pid=21, since=2.0)]), (False, ''),
                         'nothing runs: the head starts next (with its WARNING)')
        self.assertEqual(GATE.shadow(-500, head, [r], lambda x: x['grow'], lambda x: x['use'], 120.0), 150.0, 'r frees 4000 of the 3500 short')
        self.assertEqual(GATE.shadow(-5000, head, [r], lambda x: x['grow'], lambda x: x['use'], 120.0), 150.0, 'still short after all: the last end')

    def test_gate_stats_keys(self):
        lt = self.root.resolve() / 'lt'
        stats = lt / '.run-stats.json'
        with patch.object(GATE, 'STATS', stats), patch.object(GATE, 'HERE', lt):
            self.assertEqual([GATE.key('full', lt / '00-Tools/x_tester.py'), GATE.key(None, lt / 'x_tester.py'), GATE.key('full', self.root / 'x.py')],
                             ['full 00-Tools/x_tester.py', 'run x_tester.py', None])
            with patch.object(GATE, 'STATS', self.root / 'elsewhere.json'):  # CP_RUN_STATS: keys stay relative to the folder
                self.assertEqual(GATE.key('full', lt / '00-Tools/x_tester.py'), 'full 00-Tools/x_tester.py')
            self.assertEqual(GATE.stats('full a'), (0, 0), 'no file')
            lt.mkdir()
            stats.write_text(json.dumps({'a': [10, 2.5], 'b': [10, True], 'c': [10], 'd': [0, 3], 'e': 'x', 'f': [5, -1]}))
            self.assertEqual([GATE.stats(k) for k in ('a', 'b', 'c', 'd', 'e', 'f', 'g', None)],
                             [(10, 2.5), (10, 0), (10, 0), (0, 3), (0, 0), (5, 0), (0, 0), (0, 0)])

    def test_gate_variant_keys(self):
        """02-integration.py and 06-compare.py run reserve and record under keys naming what their peaks depend on (user
        decision 2026-11-24): --jobs, --sanitizers and the compiler; the rig, --compilers and --gate. An unrecorded variant
        reserves the cap; compare's show reserves nothing and parses no run options; a bad option exits before the cap."""
        spec = importlib.util.spec_from_file_location('compare_under_test', LT / '06-compare.py')
        compare = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(compare)
        rig = LT / '01-Core/99-x_compare.cpp'
        seen = {'run 02-integration.py --jobs 3 --sanitizers g++-14': 1000,
                'run 06-compare.py 96-Local Testing/01-Core/99-x_compare.cpp --compilers g++-15 --gate': 2000}
        cases = ((INTEGRATION, ['-j', '3', '--sanitizers'], {}, (1164, 'run 02-integration.py --jobs 3 --sanitizers g++-14')),
                 (INTEGRATION, ['--jobs=3'], {}, (8192, 'run 02-integration.py --jobs 3 g++-14')),
                 (compare, ['run', str(rig), '--compilers', 'g++-15', '--gate'], dict(need=None),
                  (2264, 'run 06-compare.py 96-Local Testing/01-Core/99-x_compare.cpp --compilers g++-15 --gate')),
                 (compare, ['show', 'log.json'], dict(need=0), (0, None)))
        for module, argv, kw, want in cases:
            main, calls = Path(module.__file__), []
            with patch.dict(os.environ, {'CXX': 'g++-14'}), patch.object(sys.modules['__main__'], '__file__', str(main)), \
                    patch.object(sys, 'argv', [str(main), *argv]), patch.object(GATE, 'stats', lambda k: (seen.get(k, 0), 0)), \
                    patch.object(_00_memory_cap, 'supervise', lambda *a: calls.append(a[4:6]) or 0), patch.object(_00_memory_cap, 'MAIN_MB', 8192):
                os.environ.pop('CP_MEMORY_CAPPED', None)
                with self.assertRaises(SystemExit):
                    _00_memory_cap.ensure(variant=module.variant, **kw)
            self.assertEqual(calls, [want], argv)
        with patch.dict(os.environ), patch.object(sys.modules['__main__'], '__file__', str(LT / '02-integration.py')), \
                patch.object(sys, 'argv', ['02-integration.py', '-j', '0']), patch.object(_00_memory_cap, 'supervise', lambda *a: self.fail('capped')), \
                contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as bad:
            os.environ.pop('CP_MEMORY_CAPPED', None)
            _00_memory_cap.ensure(variant=INTEGRATION.variant)
        self.assertEqual(bad.exception.code, 2)

    def test_stats_remember(self):
        """remember() merges into the file, re-read under its lock (another writer's record stays); a record is the maxima of
        its last three runs, which follow it (user decision 2026-11-24), a two-element record counting as one run; malformed
        records and files are replaced, malformed runs dropped; six processes recording 25 keys each at once lose none."""
        stats = self.root / 'stats.json'
        read = lambda: json.loads(stats.read_text())
        with patch.object(GATE, 'STATS', stats):
            GATE.remember({'full a': [5, 1.0]})
            stats.write_text(json.dumps(dict(read(), **{'quick b': [7, 2.0]})))
            GATE.remember({'full a': [6, 1.5], 'full c': [8, 3.0]})
            GATE.remember({})
            want = {'full a': [6, 1.5, [[5, 1.0], [6, 1.5]]], 'quick b': [7, 2.0], 'full c': [8, 3.0, [[8, 3.0]]]}
            self.assertEqual(stats.read_text(), json.dumps(want, indent=0, sort_keys=True) + '\n')
            got = []
            for run in ([3, 9.0], [2, 0.5], [1, 0.5], [1, 0.25]):
                GATE.remember({'full a': run})
                got.append(read()['full a'][:2])
            self.assertEqual(got, [[6, 9.0], [6, 9.0], [3, 9.0], [2, 0.5]], 'a lighter suite lowers its record after three runs, not one')
            self.assertEqual(read()['full a'][2], [[2, 0.5], [1, 0.5], [1, 0.25]])
            GATE.remember({'quick b': [1, 1.0]})
            self.assertEqual(read()['quick b'], [7, 2.0, [[7, 2.0], [1, 1.0]]], 'a two-element record is one run')
            stats.write_text(json.dumps({'full a': 'bad', 'full c': [True, 1.0], 'full e': [4, 1.0, 'x'], 'full f': [4, 1.0, [[9, 9], 'y', [1], [True, 2]]]}))
            GATE.remember({'full a': [2, 1.0], 'full c': [2, 1.0], 'full e': [1, 1.0], 'full f': [1, 1.0]})
            self.assertEqual(read(), {'full a': [2, 1.0, [[2, 1.0]]], 'full c': [2, 1.0, [[2, 1.0]]], 'full e': [4, 1.0, [[4, 1.0], [1, 1.0]]],
                                      'full f': [9, 9, [[9, 9], [1, 1.0]]]}, 'a malformed record is replaced, malformed runs dropped')
            for bad in ('not json', '[1]'):
                stats.write_text(bad)
                GATE.remember({'x': [1, 1.0]})
                self.assertEqual(read(), {'x': [1, 1.0, [[1, 1.0]]]}, bad)
        code = ('import pathlib, sys; sys.path.insert(0, sys.argv[1]); import _00_memory_gate as g; g.STATS = pathlib.Path(sys.argv[2])\n'
                'for i in range(25): g.remember({f"{sys.argv[3]} {i}": [i + 1, 0.5]})\n')
        procs = [subprocess.Popen([sys.executable, '-c', code, str(LT), str(stats), f'p{k}']) for k in range(6)]
        self.assertEqual([p.wait(60) for p in procs], [0] * 6)
        self.assertEqual(set(json.loads(stats.read_text())), {'x'} | {f'p{k} {i}' for k in range(6) for i in range(25)})

    def test_gate_lock_timeout(self):
        """A process stopped while it holds the gate's lock (Ctrl-Z in the locked section): after LOCK_WAIT a run warns and
        goes on without the gate, its releases try the lock once, and remember() merges without the lock."""
        stats = self.root / 'stats.json'
        holder = subprocess.Popen([sys.executable, '-c', 'import fcntl, os, signal, sys\nfs = [open(p + ".lock", "a") for p in sys.argv[1:]]\n'
                                   'for f in fs: fcntl.flock(f, fcntl.LOCK_EX)\nprint("held", flush=True)\nos.kill(os.getpid(), signal.SIGSTOP)\n',
                                   str(self.path), str(stats)], stdout=subprocess.PIPE, text=True)
        self.addCleanup(lambda: (holder.kill(), holder.wait()))
        self.assertEqual(holder.stdout.readline(), 'held\n')
        out, ran = io.StringIO(), []

        def run():
            with GATE.held(100, 'lbl', out=out):
                ran.append(1)
            GATE.remember({'k': [1, 1.0]})
        with patch.object(GATE, 'LOCK_WAIT', 0.3), patch.object(GATE, 'STATS', stats):
            start = time.monotonic()
            thread = threading.Thread(target=run, daemon=True)
            thread.start()
            thread.join(30)
            spent = time.monotonic() - start
        self.assertFalse(thread.is_alive(), 'a stopped holder blocks the run')
        self.assertEqual((ran, out.getvalue()), ([1], 'WARNING lbl: memory gate unavailable (its lock stayed held for 0.3 s); running without it\n'))
        self.assertLess(spent, 3, 'one wait for the gate and one for the records, never one per release')
        self.assertEqual(json.loads(stats.read_text()), {'k': [1, 1.0, [[1, 1.0]]]})

    def test_gate_state_file(self):
        """CP_MEMORY_GATE_STATE: held() takes it from the environment (nested runs do not inherit it) and writes `wait` while
        queued and `run` once it goes on (admitted, past its deadline or without the gate), so 05-extraction.py's timeout leaves
        out the queue."""
        state, seen = self.root / 'state', []
        self.free(500)
        other = self.foreign(10 ** 6)
        self.path.write_text(json.dumps([other]))
        clock = type('Clock', (), {'time': staticmethod(time.time), 'monotonic': staticmethod(time.monotonic),
                                   'sleep': staticmethod(lambda s: (seen.append(state.read_text()), time.sleep(s)))})
        with patch.dict(os.environ, CP_MEMORY_GATE_STATE=str(state)), patch.object(GATE, 'time', clock), \
                GATE.held(100, 'lbl', out=io.StringIO(), until=time.monotonic() + 0.05) as ok:
            self.assertNotIn('CP_MEMORY_GATE_STATE', os.environ)
            inside = state.read_text()
        self.assertEqual((ok, seen[0], inside), (False, 'wait', 'run'))
        self.free(100000)
        self.path.write_text('[]')
        for where in (self.path, self.root / 'file/gate.json'):
            (self.root / 'file').write_text('')
            state.unlink()
            with patch.dict(os.environ, CP_MEMORY_GATE_STATE=str(state), CP_MEMORY_GATE=str(where)), GATE.held(100, 'lbl', out=io.StringIO()):
                self.assertEqual(state.read_text(), 'run', where)

    def test_supervise_records(self):
        """supervise() with a key records a passing run's largest peak and its seconds in .run-stats.json and reserves with
        its recorded seconds, adding a run to its record, whose smaller run does not lower it; a failing run, no key, or a run 01-run.py
        started (CP_RUN_STATS_OWNER, popped) records nothing, the last still reserving with its recorded seconds.
        report() writes the peak to the file ensure() took from CP_MEMORY_PEAK. With systemd scopes, a tester run by hand
        records its quick and full peaks, and one under CP_RUN_STATS_OWNER none."""
        stats, seen, code = self.root / 'stats.json', [], [0]
        stats.write_text(json.dumps({'full x': [100, 7.5], 'full z': [10, 4.0]}))

        def capped_run(command, mode, source, label, peaks):
            seen.append(([e.get('secs') for e in GATE.load(self.path)], peaks is not None, 'CP_RUN_STATS_OWNER' in os.environ))
            if peaks is not None:
                peaks += [40, 90]
            return code[0]
        with patch.object(_00_memory_cap, 'capped_run', capped_run), patch.object(GATE, 'STATS', stats):
            self.assertEqual(_00_memory_cap.supervise(['x', 'y'], 'full', 'env', 'a', 50, 'full x'), 0)
            x = json.loads(stats.read_text())['full x']
            self.assertEqual((x[:2], len(x[2])), ([100, 7.5], 2), 'one smaller run does not lower a record; it is its second run')
            self.assertEqual(_00_memory_cap.supervise(['x', 'y'], 'full', 'env', 'a', 50, 'full w'), 0)
            rec = json.loads(stats.read_text())['full w']
            self.assertTrue(rec[0] == 90 and 0 <= rec[1] < 5, rec)
            code[0] = 3
            self.assertEqual(_00_memory_cap.supervise(['x', 'y'], 'full', 'env', 'a', 50, 'full y'), 3)
            code[0] = 0
            _00_memory_cap.supervise(['x', 'y'], 'full', 'env', 'a', 50)
            with patch.dict(os.environ, CP_RUN_STATS_OWNER='1'):
                _00_memory_cap.supervise(['x', 'y'], 'full', 'env', 'a', 50, 'full z')
            self.assertEqual(json.loads(stats.read_text()), {'full x': x, 'full z': [10, 4.0], 'full w': rec})
        self.assertEqual(seen, [([7.5], True, False), ([0], True, False), ([0], True, False), ([0], False, False), ([4.0], False, False)])
        peak = self.root / 'peak'
        with patch.object(_00_memory_cap, 'cgroup_stats', lambda: ((123 << 20) + 5, 0)), patch.dict(os.environ, CP_MEMORY_CAPPED='500', CP_MEMORY_PEAK=str(peak)), \
                patch.object(_00_memory_cap, 'PEAK_FILE', None), contextlib.redirect_stdout(io.StringIO()) as out:
            _00_memory_cap.ensure()
            self.assertNotIn('CP_MEMORY_PEAK', os.environ, 'taken: processes it starts do not write it')
            _00_memory_cap.report()
        self.assertEqual((peak.read_text(), out.getvalue()), ('123', 'MEMORY peak=123MB cap=500MB oom_kills=0\n'))
        if not _00_memory_cap.available():
            print('SKIP test_supervise_records end to end: no systemd user scope', flush=True)
            return
        lt = self.root / 'lt'
        (lt / '00-Tools').mkdir(parents=True)
        for name in ('_00_memory_cap.py', '_00_memory_gate.py'):
            shutil.copy(LT / name, lt / name)
        tester = lt / '00-Tools/x_tester.py'
        tester.write_text('import os, sys\nsys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))\n'
                          'import _00_memory_cap\n_00_memory_cap.ensure()\nos.environ.get("CP_TEST_MODE")\nb = bytearray(80 << 20)\nb[::4096] = b"x" * len(b[::4096])\n')
        env = {k: v for k, v in os.environ.items() if not k.startswith('CP_TEST_') and k not in ('CP_MEMORY_CAPPED', 'CP_RUN_STATS_OWNER')}
        for mode, extra in (('quick', {}), ('full', {}), ('stress', {'CP_RUN_STATS_OWNER': '1'})):
            r = subprocess.run([sys.executable, str(tester), '--mode', mode], env=env | extra, capture_output=True, text=True, timeout=120)
            self.assertEqual(r.returncode, 0, r.stderr)
        bench = lt / '00-Tools/x_benchmark.py'  # no mode: it reads no CP_TEST_MODE
        bench.write_text(tester.read_text().replace('os.environ.get("CP_TEST_MODE")\n', ''))
        r = subprocess.run([sys.executable, str(bench), '--n', '5'], env=env | {'CP_TEST_MODE': 'full'}, capture_output=True, text=True, timeout=120)
        self.assertEqual(r.returncode, 0, r.stderr)
        got = json.loads((lt / '.run-stats.json').read_text())
        self.assertEqual(set(got), {'quick 00-Tools/x_tester.py', 'full 00-Tools/x_tester.py', 'run 00-Tools/x_benchmark.py'},
                         'a run 01-run.py starts is recorded by it alone; a benchmark under `run`')
        self.assertTrue(all(80 <= v[0] < 1024 and v[1] >= 0 for v in got.values()), got)

    def test_gate_nested_two_sessions(self):
        """Two sessions each hold a running reservation (A, B: 500 MB of 2100, 1000 kept) whose child queues for 900 MB: the
        older child starts with the deadlock WARNING, the younger once A's nest ended (only its own ancestor then runs), and both
        sessions finish instead of waiting on each other."""
        self.meminfo.write_text(f'MemAvailable:   {2100 << 10} kB\n')
        nest = self.root / 'nest.py'
        nest.write_text(r"""import pathlib, subprocess, sys, time
sys.path.insert(0, sys.argv[1])
import _00_memory_gate as g
g.MEMINFO, g.POLL = pathlib.Path(sys.argv[2]), 0.05
d, name, need, kid = pathlib.Path(sys.argv[3]), sys.argv[4], int(sys.argv[5]), int(sys.argv[6])
with g.held(need, name, out=sys.stdout):
    print('START', name, flush=True)
    if kid:
        (d / f'{name}.running').touch()
        while not (d / 'go').exists(): time.sleep(0.02)
        sys.exit(subprocess.run([sys.executable, __file__, sys.argv[1], sys.argv[2], str(d), name + '-kid', str(kid), '0']).returncode)
""")
        env = dict(os.environ, CP_MEMORY_GATE_RESERVE_MB='1000')
        procs = []
        for name in 'ab':
            out = open(self.root / f'{name}.out', 'w')
            self.addCleanup(out.close)
            procs.append(subprocess.Popen([sys.executable, str(nest), str(LT), str(self.meminfo), str(self.root), name, '500', '900'],
                                          env=env, stdout=out, stderr=subprocess.STDOUT, start_new_session=True))
            self.addCleanup(lambda p=procs[-1]: p.poll() is None and (os.killpg(p.pid, signal.SIGKILL), p.wait()))
            deadline = time.monotonic() + 30
            while not (self.root / f'{name}.running').exists() and time.monotonic() < deadline: time.sleep(0.02)
        (self.root / 'go').touch()
        deadline = time.monotonic() + 30
        while any(p.poll() is None for p in procs) and time.monotonic() < deadline: time.sleep(0.05)
        logs = {n: (self.root / f'{n}.out').read_text() for n in 'ab'}
        self.assertEqual([p.poll() for p in procs], [0, 0], f'the two nests wait on each other: {logs}')
        warned = [line for text in logs.values() for line in text.splitlines() if 'every running gated run waits' in line]
        self.assertEqual(len(warned), 1, logs)
        self.assertRegex(warned[0], r'^WARNING [ab]-kid: memory gate: \d+ MB available, 900 MB needed and 1000 MB kept free; every running')
        self.assertEqual(json.loads(self.path.read_text()), [], 'all released')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=6, help='seeded random runs in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Runner, Integration, Build, Forward, Gate):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n in QUICK)
    print(f"runner checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
