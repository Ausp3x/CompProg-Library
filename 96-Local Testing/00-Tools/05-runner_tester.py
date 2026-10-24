#!/usr/bin/env python3
"""Checks for the parallel suite runner 01-run.py and the build pool parallel() of 02-integration.py.

Fake suites are tiny Python scripts that sleep fractions of a second and record in a shared fixture directory (RT_DIR)
their start and end instants (time.monotonic, system-wide), pid, mode, seed and environment, holding a 'busy' marker
while they live; a flaky suite fails iff it sees another marker. 01-run.py runs from a copy in the fixture, so it
discovers only the fake suites and keeps its .run-stats.json there.
quick: output identity at -j 1 and -j 4, first-failure status, rerun alone, the jobs bound, parallel() budget and rerun.
full: every test, including signals, real capped scopes, integration main() on a small fake repository and 2
seeded random runs.
stress: full with --rounds seeded random runs (default 6) of random suite counts, sleeps, failures, jobs and budgets.
Oracles: the expected log rebuilt from the suite parameters (RUN lines in rounds x sorted-path order, each suite's known
text, NOTE before a task whose failed run beside others was rerun, nothing after the first always-failing task, the PASS line); overlaps from a
sweep over the recorded intervals, never wall-clock speedups. Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
Failing tests print the seed, mode and retained fixture directory.

Feature map: 96-Local Testing/Docs/00-parallel.md (Feature-to-test map).
"""
import argparse
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import random
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
import _00_memory_cap

_00_memory_cap.ensure()

LT = Path(__file__).resolve().parents[1]
MODE, SEED, ROUNDS = 'full', 42, 6
QUICK = {'test_order_identity', 'test_first_failure_status', 'test_retry_alone', 'test_jobs_bound',
         'test_parallel_budget', 'test_parallel_rerun_alone'}
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
                       capped='CP_MEMORY_CAPPED' in env), f)
    others = len(os.listdir(f"{d}/busy")) > 1
    while P.get('wait') and not os.path.exists(f"{d}/{P['wait']}.done") and time.monotonic() < t0 + 30: time.sleep(0.02)
    time.sleep(P.get('sleep', 0))
    others |= len(os.listdir(f"{d}/busy")) > 1
    print(P['name'], 'mode', env.get('CP_TEST_MODE'), 'seed', env.get('CP_TEST_SEED'), flush=True)
    print(P['name'], 'stderr line', file=sys.stderr, flush=True)
    if P.get('mem'): print(f"MEMORY peak={P['mem']}MB", flush=True)
    if P.get('flaky'): print('alone', not others, flush=True)
    code = P.get('code', 0) or 3 * (P.get('flaky', False) and others)
finally:
    time.sleep(P.get('cleanup', 0))
    t1 = time.monotonic(); os.remove(busy)
    with open(f"{d}/{me}.end", 'w') as f: json.dump(dict(t1=t1, code=code), f)
open(f"{d}/{P['name']}.done", 'w').close()
sys.exit(code)
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
        shutil.copy(LT / '01-run.py', root / '01-run.py')

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
            f'PASS: {len(order)} available suites x {len(seeds)} round(s); see 00-index.md for coverage gaps\n')


class Base(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp(prefix='runner-test-'))
        self.f = Fixture(self.root / 'f')
        self.addCleanup(self.clean)

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
        shutil.copy(LT / '_00_memory_cap.py', f.root)
        a, _ = f.add(dict(scope=True, sleep=0.2), dict(scope=True, sleep=0.2))
        code, out, err = f.run('-j', '2')
        self.assertEqual(code, 0, out + err)
        self.assertEqual(sorted(r['mode'] for r in f.recs(a)), ['full', 'quick'], 'the pre-flight ran under the runner')
        stats = json.loads((f.root / '.run-stats.json').read_text())
        self.assertTrue(all(v[0] > 0 for v in stats.values()), f'peaks learned from real MEMORY lines: {stats}')
        g = Fixture(self.root / 'g')
        shutil.copy(LT / '_00_memory_cap.py', g.root)
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
        for budget, most in (3011, 1), (3012, 2):  # each reserves 1000 * 5 // 4 + 256 = 1506 MB
            for s in f.root.glob('*.start'): s.unlink()
            self.check(f.run('-j', '8', '--memory-mb', str(budget)), 0, f.expect(), f'budget {budget}')
            self.assertEqual(f.span(), most, f'budget {budget}')

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
                    '09-Other/01-y_tester.py', '02-Data Structures/02-z_compile_tester.py'):
            f.suite(rel)
        (f.root / '02-Data Structures/03-helper.py').write_text('raise SystemExit(9)\n')
        compiled, full = f.suites.pop('02-Data Structures/02-z_compile_tester.py'), dict(f.suites)
        for mode, extra, keep in ('full', (), ''), ('quick', (), ('01-Core/01-', '02-')), ('full', ('--filter', '01-Core/'), '01-C'):
            f.suites = {k: v for k, v in full.items() if k.startswith(keep)}
            self.check(f.run('-j', '2', '--mode', mode, *extra), 0, f.expect(mode), f'{mode} {extra}')
        self.assertEqual(f.recs(compiled['name']), [])

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
            order = [(f.suites[rel], str(s)) for s in seeds for rel in sorted(f.suites, key=lambda rel: Path(rel).parts)]
            upto = next((i for i, (P, _) in enumerate(order) if P['code']), None)
            ctx = f'round {r}: -j {j} cap {cap} budget {budget} mode {mode} seeds {seeds} suites {f.suites}'
            got = f.run('--mode', mode, '--rounds', str(rounds), '--seed', str(seed), '-j', str(j), '--memory-mb',
                        str(budget), CP_TEST_MEMORY_MB=str(cap))
            runs = [[x for x in f.recs(P['name']) if x['seed'] == s and x['code'] is not None] for P, s in order]  # stopped runs restart
            notes = {i for i, x in enumerate(runs) if len(x) == 2}
            self.check(got, 0 if upto is None else order[upto][0]['code'], f.expect(mode, seeds, notes, upto), ctx)
            self.assertLessEqual(f.span(), max(1, min(j, budget // cap)), ctx)
            self.assertTrue(all(len(x) <= 1 + (j > 1) for x in runs), ctx)
            self.assertTrue(upto is None or upto not in notes or j > 1, f'{ctx}: only a failure beside others is rerun')
            self.assertTrue(all(f.alone(x[1]) for x in runs if len(x) == 2), ctx)


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
                          '01-Core/99-all.hpp': '#pragma once\n#include "01-a.hpp"\n',
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
                 patch.object(INTEGRATION.shutil, 'which', lambda name: str(stub) if ccache else None):
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


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=6, help='seeded random runs in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Runner, Integration):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n in QUICK)
    print(f"runner checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
