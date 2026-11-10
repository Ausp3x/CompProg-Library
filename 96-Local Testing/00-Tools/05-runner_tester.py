#!/usr/bin/env python3
"""Checks for the parallel suite runner 01-run.py, the build pool parallel() of 02-integration.py and the signal
forwarding of _00_memory_cap.supervise().

Fake suites are tiny Python scripts that sleep fractions of a second and record in a shared fixture directory (RT_DIR)
their start and end instants (time.monotonic, system-wide), pid, mode, seed and environment, holding a 'busy' marker
while they live; a flaky suite fails iff it sees another marker. 01-run.py runs from a copy in the fixture, so it
discovers only the fake suites and keeps its .run-stats.json there.
quick: output identity at -j 1 and -j 4, first-failure status, rerun alone, the jobs bound, parallel() budget and rerun,
forwarding() handler restore.
full: every test, including signals, real capped scopes, SIGTERM/SIGHUP sent to a supervisor pid alone (main run and
pre-flight, and to the ad-hoc CLI's supervisor, each in a real scope; explicit SKIP without one), integration main() on a small fake repository and 2
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

_00_memory_cap.ensure()

LT = Path(__file__).resolve().parents[1]
MODE, SEED, ROUNDS = 'full', 42, 6
QUICK = {'test_order_identity', 'test_first_failure_status', 'test_retry_alone', 'test_jobs_bound',
         'test_parallel_budget', 'test_parallel_rerun_alone', 'test_forwarding_restores_handlers'}
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
SIG = r'''import json, os, signal, sys, time
sys.path.insert(0, os.environ['RT_LT'])
import _00_memory_cap
_00_memory_cap.ensure()
d, mode = os.environ['RT_DIR'], sys.argv[sys.argv.index('--mode') + 1]
def stop(sig, _):
    open(f"{d}/{mode}.sig", 'w').write(str(sig))
    sys.exit(40 + sig)
for s in (signal.SIGTERM, signal.SIGHUP): signal.signal(s, stop)  # also probes SIGHUP inherited as ignored
with open(f"{d}/tmp", 'w') as f: json.dump(dict(pid=os.getpid(), ppid=os.getppid()), f)
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
            (f'NOTE {len(notes)} problem(s) to fix, each within a passing run (05-testing.md, Layout and modes):\n'
             + ''.join(f"  {mode} {tasks[i][0]}: exited {tasks[i][1].get('code') or 3} beside other suites, passed alone\n" for i in sorted(notes)) if notes else '')
            + f'PASS: {len(order)} available suites x {len(seeds)} round(s); see 00-index.md for coverage gaps\n')


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
        for budget, most in (2327, 1), (2328, 2):  # each reserves 1000 * 11 // 10 + 64 = 1164 MB
            for s in f.root.glob('*.start'): s.unlink()
            self.check(f.run('-j', '8', '--memory-mb', str(budget)), 0, f.expect(), f'budget {budget}')
            self.assertEqual(f.span(), most, f'budget {budget}')

    def test_smaller_suite_passes_one_that_does_not_fit(self):
        f = self.f
        f.add(dict(sleep=1.2, mem=1000), dict(sleep=0.6, mem=1000), dict(sleep=0.1, mem=100))
        self.check(f.run('-j', '8'), 0, f.expect())
        for s in f.root.glob('*.start'): s.unlink()
        self.check(f.run('-j', '8', '--memory-mb', str(1164 + 174)), 0, f.expect())  # 1000 MB reserves 1164, 100 MB 174
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

    def test_quick_bound_note(self):
        f = self.f
        (f.root / '02-Data Structures').mkdir(exist_ok=True)
        (f.root / '02-Data Structures/01-a_tester.py').write_text("print('TIMING a optimized mode=quick compile=1.00s run=12.50s longest=12.50s limit=180s')\n"
                                                                  "print('TIMING a checked mode=quick compile=99.00s run=10.00s')\n")
        (f.root / '02-Data Structures/02-b_tester.py').write_text("print('TIMING b optimized mode=quick compile=0.10s run=30.25s')\n")
        (f.root / '02-Data Structures/03-c_tester.py').write_text("print('TIMING c optimized mode=full compile=0.10s run=99.00s')\n")
        code, out, err = f.run('--mode', 'quick', '-j', '2')
        self.assertEqual(code, 0, err)
        self.assertIn('NOTE 2 quick build(s) over 10 s, timed beside other suites (05-testing.md, Layout and modes):\n'
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
            self.assertIn('NOTE 1 quick build(s) over 10 s, timed beside other suites (05-testing.md, Layout and modes):\n  11.0 s d optimized\n', out,
                          f'{mode}: from the pre-flight, once though every stress round runs one')
            self.assertIn('NOTE 1 problem(s) to fix, each within a passing run (05-testing.md, Layout and modes):\n'
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
        self.assertIn('NOTE 2 problem(s) to fix, each within a passing run (05-testing.md, Layout and modes):\n', out)
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
        self.assertIn('NOTE 1 problem(s) to fix, each within a passing run (05-testing.md, Layout and modes):\n  x.o: 100 s of its 180 s limit\n', r.stdout)

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
        for name in ('_00_memory_cap.py', '_00_cpp_test_runner.py', '_00_contest_runner.py'):
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

    def start(self, mode, sleep=60, hup=signal.SIG_DFL, cli=False):
        """The supervisor of sig_tester.py: its own ensure(), or with cli the ad-hoc CLI, whose scope runs a reporter."""
        if not _00_memory_cap.available(): self.skipTest('SKIP: no systemd user scope')
        (self.root / 'sig_tester.py').write_text(SIG)
        for x in self.root.glob('quick.*'): x.unlink()
        env = {k: v for k, v in os.environ.items() if not k.startswith('CP_TEST_') and k != 'CP_MEMORY_CAPPED'} | dict(
            RT_DIR=str(self.root), RT_LT=str(LT), RT_SLEEP=str(sleep), CP_TEST_MEMORY_MB='256', CP_TEST_PREFLIGHT_MB='256')
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


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=6, help='seeded random runs in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Runner, Integration, Build, Forward):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n in QUICK)
    print(f"runner checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
