#!/usr/bin/env python3
"""Checks for 96-Local Testing/04-mutation.py, the mewt campaign driver.

No test touches the network, ~/.local or the real git configuration: git runs with GIT_CONFIG_GLOBAL and
GIT_CONFIG_SYSTEM at /dev/null in temporary repositories (initial commit on master), install reads file:// tarballs
built here, and fake executables (`#!/bin/sh` scripts or `#!<this python>` scripts) sit in a per-test bin directory
that is the whole PATH (plus a `git` symlink), or comes first on it where a test needs systemd-run. Every test sets
CP_MUTATION_LOCK to its own temporary path, so concurrent seeds and a real campaign holding the machine lock never
block it. Tests needing another repository root patch the loaded module's ROOT; subprocess tests run DRIVER.
quick: every unit and fake case (blob, current, evidence, record/recorded, stale, library_path, changed, changed_lines,
scope, install, mewt_version, stage and outcomes with a fake mewt, interrupt, unchanged, memory helpers, stop, the
check/run/install CLI with fake mewts, the workdir and machine locks, and whole campaigns against FAKE_CAMPAIGN, a
stateful fake mewt with hand-set mutants and outcomes, some with a patched baseline measurement and stage()'s poll
shortened) and one seeded random round of the random tests; no real mewt. full: quick plus the real-mewt end-to-end
campaigns (SKIP unless `mewt --version` is exactly MEWT_VERSION and g++ exists) and 2 random rounds. stress: full
with --rounds random rounds (default 6). Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
Oracles (written here, independent of 04-mutation.py): `git hash-object` (with and without line-ending attributes),
`git rev-parse HEAD:<path>` and `git cat-file -e` for blob ids and the object store; hand-written expected outputs of
each fixture (records, changed-header sets, tar layouts, stage logs and lines, per-mutant timeouts, caps and shard
counts computed by hand per case); independent properties of changed_lines (identity, appended and edited lines,
deleted-line neighbours, a greedy subsequence check of the unmarked lines, range); the fake mewts' own logs of argv,
cwd, ids files, process group, start/end times and SIGINT; a datetime/Decimal parse of outcome times for the
latest-outcome merge; sample properties (size, subset, sorted, repeatable per seed, spread over the candidates: more
than half covered and a mean id near the middle over several seeds); the real _00_memory_cap supervisor (PREFLIGHT
and MEMORY lines) with a negative control that undoes CP_TEST_PREFLIGHT=0; for the end-to-end campaigns a hand oracle
of which tester stage catches each toy mutant, mewt's own `results` (merged by latest time) and `print mutants`
listings in the shards, a flock contention detector in the timeout toy's tester, and /proc for processes left
behind. Failures print seed, mode, fixture and inputs.

Feature map:
blob (git hash-object on fixed and random bytes) -> test_blob_*;
current (the blob git stores: a CRLF header under `* text=auto eol=lf` is current under its LF blob, which check and
run accept, while blob(raw bytes) differs and stale names both) -> test_current_line_endings;
evidence (path mapping) -> test_evidence_path;
recorded (missing doc, section or token; heading with trailing blanks; section ended by the next `## ` or EOF;
`###` inside; token outside the section; malformed tokens; first section wins) -> test_recorded_*;
record ((blob, untested) from the first token line of the section; count on another line or after the section
ignored; no count) -> test_record_cases;
stale and check CLI (missing, mismatched and incomplete reasons, --existing, current record, absolute paths, sorted
dedup, summary line, exit code, non-header and missing arguments, random fixtures with and without --existing) ->
test_stale_*, test_check_*;
library_path (absolute, ROOT-relative, cwd-relative, outside ROOT) -> test_library_path;
changed and check --changed (merge-base committed, staged, unstaged, untracked, ignored, deleted, aggregates,
non-library folders and nested paths, changes on base only, another base, bad base) -> test_changed_*;
changed_lines (identical, append, edit, deletion neighbours, empty old/new, random subsequence and range) ->
test_lines_*;
scope (no record, --whole, record in the repo with its changed lines, current complete record of an uncommitted
blob, record blob not in the repo, current record with untested mutants or no count: NOTE and whole file; exact
output) -> test_scope_cases, test_fake_incomplete_record (run prints no PASS and runs a whole-file campaign);
install (flattened regular members, modes, path traversal, symlink and directory members skipped, link replaced,
relative prefix resolved, wrong digest, no mewt member, CLI prefix, the PATH WARNING when `mewt` on PATH is missing
or another binary and none when it resolves to the install, also through another symlink) -> test_install_*;
mewt_version (missing, non-executable, version, other text, silent, stderr only) -> test_version_*;
stage (ids files, argv, cwd, own session, logs, empty shards, every shard concurrently, deadline SIGINT, failure
with log tail), outcomes (latest outcome over shards by time, fractions of varying length, unparsable time) and
interrupt (SIGKILL after the grace period) -> test_stage_*, test_outcomes_latest, test_interrupt_kill;
unchanged (identical shards, a mutated shard named) -> test_unchanged_*;
tester_cap/capped/available_mb (ensure(mb=...) or the default, the per-test scope command, MemAvailable) ->
test_memory_helpers;
stop (a real SIGTERM or SIGHUP raises KeyboardInterrupt once, then both are ignored) -> test_stop_handler; campaign
installs it for both unless a signal was left ignored -> test_fake_signal_handlers;
run CLI before and around the campaign (non-header, missing tester, SKIP without mewt or another version before any
workdir, record-current PASS also for an unstaged edit, workdir refusals for another header, blob, severity, seed or
sample (with .lock) and non-empty without campaign.json (before the lock), a workdir inside the checkout refused
before anything is created, a held lock, a workdir holding only .lock, ROOT-relative header from another cwd,
argparse errors for --severity, --budget, --jobs, --memory-mb, --sample -1) -> test_run_*;
machine lock (CP_MUTATION_LOCK, else $XDG_RUNTIME_DIR/cp-mutation.lock, else the temporary directory's: WAIT naming
it, blocked before any shard, starts when freed; a lock in a missing directory is a FAIL line, no traceback) ->
test_fake_machine_lock; the header edited during the wait (exit 1, `changed while this run waited`, no
MUTATION or Record line, no campaign.json, no mewt call after --version) -> test_fake_header_changed_during_wait;
measured() interrupted (SIGTERM or SIGHUP inside a cap, SIGTERM to the top-level supervisor, during a slow stage
baseline: exit 130, one INTERRUPTED line, no stage started, no process left in the workdir, the capped tester, its
supervisor and reporter included; fails against the former subprocess.run probe) -> test_fake_baseline_signal;
CP_TEST_PREFLIGHT=0 (CP_TEST_MODE=full under the real supervisor: one MUTATION line, one workdir, no PREFLIGHT;
the negative control shows two and a PREFLIGHT line) -> test_fake_no_preflight;
stage parameters (timeout max(60, ceil(3 x baseline)) and --timeout, cap min(own, max(1024, 3 x peak)) with the
floor, 3 x peak, the tester's own ensure(mb=...) cap and an own cap below the floor, fewer shards than --jobs by
--memory-mb // cap and at least one, baselines under the own cap; from the stage lines and the fake's argv) ->
test_fake_stage_parameters;
--sample (whole file: a seeded uniform sample of N of more than N candidates, after --severity, with
candidates/selected in campaign.json and `uniform sample of <candidates>` in the Record; 0 or at least the
candidates: all and no sample wording; No record names stage quick on a stop before it) -> test_fake_sample; diff
scope never samples -> test_fake_resume_recorded;
timeouts retested alone (only s0, width 1, same timeout; a repeat timeout stays caught, a pass alone survives that
stage and escalates, a fail alone is caught; in quick and in full; unconfirmed emptied) -> test_fake_timeouts_alone;
unconfirmed ({id: stage} kept across a budget stop of a stage's alone pass, No record with 0 untested naming it;
on resume retested alone only in that stage, for quick and for full, with no quick-alone even when quick runs
again for a survivor, a stage with nothing pending still running its baseline and alone pass; an entry no longer
timing out dropped without a retest) -> test_fake_unconfirmed_resume;
resume after recording (a pasted Record makes check pass; the same --workdir still resumes, keeping a diff-scoped
spec's base, and retests the survivor; another workdir or none prints PASS and creates nothing) ->
test_fake_resume_recorded, test_e2e_campaign_and_resume;
campaign end to end with real mewt (three-stage escalation per mutant against the hand oracle, latest statuses over
all shards, survivors table, report.json, Record blob stored in the object store, shards equal to the checkout,
resume refusal for another severity, seed or sample; the Record pasted, check passing, PASS without the workdir,
then resume after strengthening the tester to 0 survived; diff scope against a committed record and against the
uncommitted blob a complete run stored, record-current PASS and --whole; two-stage testers; Core full default
severity and its mini counterpart; --sample 3 of a Core full header's high and medium mutants; a runaway mutant
killed alone in its own 1024 MB scope; a slow-under-contention survivor that times out among concurrent shards,
passes alone and survives, beside a genuine infinite loop that times out alone too, through per-mutant scopes; a
deadline before the first stage (no baseline), a budget stop with exit 3 and No record naming the stage, and its
resume to completion, on a --mutations sample; a campaign-scope kill and a failing stage baseline; SIGTERM and
SIGHUP inside a cap, SIGTERM to the top-level supervisor alone and to its group: exit 130, one INTERRUPTED line, no
process left in the workdir, the headers restored; no file left in the caller's working directory) -> test_e2e_*.
"""
import argparse
import contextlib
from datetime import datetime, timezone
from decimal import Decimal
import fcntl
import hashlib
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
import tarfile
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()
_00_memory_cap.available()  # cached with the real PATH before tests narrow it to their fake bin directory

ROOT = Path(__file__).resolve().parents[2]
MUTATION_PY = ROOT / '96-Local Testing/04-mutation.py'
MODE, SEED, ROUNDS = 'full', 42, 6
FULL = {'test_e2e_campaign_and_resume', 'test_e2e_diff_scope', 'test_e2e_core_severity', 'test_e2e_sample', 'test_e2e_budget_stop',
        'test_e2e_runaway_alone', 'test_e2e_timeout_alone', 'test_e2e_stage_failures', 'test_e2e_signals'}
STAGES3 = ['quick', 'full-optimized', 'full-all']
EMPTY_BLOB = 'e69de29bb2d1d6434b8b29ae775ad8c2e48c5391'


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


M = load(MUTATION_PY, 'mutation_under_test')
ISOLATED = {k: v for k, v in os.environ.items() if not k.startswith('GIT_')} | {
    'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_SYSTEM': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'}
GIT = shutil.which('git') or '/usr/bin/git'


def git(cwd, *args, data=None):
    r = subprocess.run(['git', *args], cwd=cwd, capture_output=True, input=data, env=ISOLATED)
    if r.returncode:
        raise RuntimeError(f'git {" ".join(args)} failed in {cwd}: {r.stderr.decode(errors="replace")}')
    return r.stdout.decode()


def o_blob(data):
    return git(None, 'hash-object', '--stdin', data=data).strip()


def init_repo(path):
    path.mkdir(parents=True, exist_ok=True)
    git(path, 'init', '-q', '-b', 'master')
    git(path, 'config', 'user.name', 'Repo User')
    git(path, 'config', 'user.email', 'repo@example.invalid')
    return path


def put(root, files):
    for rel, text in files.items():
        p = root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_bytes(text if isinstance(text, bytes) else text.encode())


def commit(root, msg='c'):
    git(root, 'add', '-A')
    git(root, 'commit', '-qm', msg)


def executable(path, text):
    path.write_text(text)
    path.chmod(0o755)
    return path


def o_subsequence(xs, ys):
    """Greedy check that xs is a subsequence of ys."""
    j = 0
    for x in xs:
        while j < len(ys) and ys[j] != x:
            j += 1
        if j == len(ys):
            return False
        j += 1
    return True


def o_overlap(intervals):
    """Largest number of intervals open at one time."""
    events = sorted([(s, 1) for s, _ in intervals] + [(e, -1) for _, e in intervals], key=lambda t: (t[0], t[1]))
    best = now = 0
    for _, d in events:
        now += d
        best = max(best, now)
    return best


def record_doc(hexblob, before='', after='', untested=0):
    count = '' if untested is None else f', {untested} untested'
    return f'# Evidence\n\n{before}## Mutation testing\n\nRecord: mewt 4.0.0 on blob `{hexblob}` (test): 3 mutants{count}.\n\n{after}'


def o_time(text):
    """(whole seconds, exact fraction) of a UTC ISO 8601 time such as 2026-11-03T10:12:15.157582821Z."""
    whole, _, frac = text.rstrip('Z').partition('.')
    return datetime.fromisoformat(whole + '+00:00'), Decimal('0.' + (frac or '0'))


def o_latest(results_by_shard):
    """{id: (status, mutant, outcome)}: each mutant's outcome with the latest time over all shards."""
    res = {}
    for results in results_by_shard:
        for r in results:
            o = r.get('outcome')
            if o and (r['mutant']['id'] not in res or o_time(o['time']) > o_time(res[r['mutant']['id']][2]['time'])):
                res[r['mutant']['id']] = o['status'], r['mutant'], o
    return res


FAKE_MEWT = r'''import json, os, signal, sys, time
rec = dict(argv=sys.argv[1:], cwd=os.getcwd(), pid=os.getpid(), pgid=os.getpgid(0), start=time.time())
def done(**kw):
    rec.update(kw, end=time.time())
    with open(os.path.join(@LOG@, '%d.json' % os.getpid()), 'w') as f:
        json.dump(rec, f)
def on_int(*_):
    done(sigint=True)
    sys.exit(130)
signal.signal(signal.SIGINT, on_int)
cfg = json.load(open('fake.json')) if os.path.exists('fake.json') else {}
print('fake mewt ran')
print(cfg.get('say', ''), flush=True)
time.sleep(cfg.get('sleep', 0))
done()
sys.exit(cfg.get('code', 0))
'''

# The end-to-end toy: hand oracle in o_stage.
TOY = ('#pragma once\ninline int add(int a, int b) { return a + b; }\ninline int clampHi(int x, int hi) {\n'
       '    if (x > hi) { return hi; }\n    return x;\n}\n')
TOY_TESTER = r'''import argparse, pathlib, subprocess, sys, tempfile
p = argparse.ArgumentParser()
p.add_argument('--mode', default='full')
p.add_argument('--seed', type=int, default=1)
p.add_argument('--configuration', choices=('optimized', 'all'), default='all')
a = p.parse_args()
header = pathlib.Path(__file__).resolve().parents[2] / '@HEADER@'
driver = ('#include "' + str(header) + '"\n'
          'volatile int A = 2, Z = 0, B = 3, X = 5, H = 3, Y = 1;\n'
          'int main() { int r[4] = {add(A, Z), add(A, B), clampHi(X, H), clampHi(Y, H)}; '
          '__builtin_printf("%d %d %d %d\\n", r[0], r[1], r[2], r[3]); }\n')
with tempfile.TemporaryDirectory() as d:
    src, exe = pathlib.Path(d, 'main.cpp'), pathlib.Path(d, 'main')
    src.write_text(driver)
    if subprocess.run(['@CXX@', '-std=gnu++20', '-O0', '-Werror=return-type', '-o', str(exe), str(src)], capture_output=True).returncode:
        sys.exit('compile failed')
    r = subprocess.run([str(exe)], capture_output=True, text=True)
    if r.returncode:
        sys.exit('run failed')
    v = [int(x) for x in r.stdout.split()]
checks = [v[0] == 2]
if a.mode == 'full':
    checks.append(v[1] == 5)
    if a.configuration == 'all':
        checks += [v[2] == 3, v[3] == 1]
@EXTRA@
sys.exit(0 if all(checks) else 'check failed')
'''
# runaway toy: bytes(2^29) is 64 MiB; the SOS mutant (<< 3) asks for 4 GiB and dies at its own 1024 MB scope
RUNAWAY = '#pragma once\ninline long long bytes(int n) { return 1LL * n >> 3; }\n'
RUNAWAY_TESTER = r'''import argparse, pathlib, subprocess, sys, tempfile
p = argparse.ArgumentParser()
p.add_argument('--mode', default='full')
p.add_argument('--seed', type=int, default=1)
a = p.parse_args()
header = pathlib.Path(__file__).resolve().parents[2] / '@HEADER@'
driver = ('#include <vector>\n#include "' + str(header) + '"\nvolatile int N = 1 << 29;\n'
          'int main() { std::vector<char> v(bytes(N), 1); __builtin_printf("%zu\\n", v.size()); }\n')
with tempfile.TemporaryDirectory() as d:
    src, exe = pathlib.Path(d, 'main.cpp'), pathlib.Path(d, 'main')
    src.write_text(driver)
    if subprocess.run(['@CXX@', '-std=gnu++20', '-O0', '-Werror=return-type', '-o', str(exe), str(src)], capture_output=True).returncode:
        sys.exit('compile failed')
    r = subprocess.run([str(exe)], capture_output=True, text=True)
sys.exit(0 if r.returncode == 0 and r.stdout.split() == [str(1 << 26)] else 'check failed')
'''
# timeout toy (--mutations NR,BL selects exactly two mutants): NR turns the root walk into an infinite loop at a root;
# BL flips spare(), which no test calls, so it survives, but its test is slow while another mutant's test runs: every
# mutated test holds LOCK shared while it compiles and runs, and a passing one then probes LOCK exclusively for PROBE s
LOOP = ('#pragma once\ninline int root(const int *p, int x) {\n    while (!(p[x] == x)) { x = p[x]; }\n    return x;\n}\n'
        'inline bool spare() { return true; }\n')
LOOP_TESTER = r'''import argparse, fcntl, pathlib, subprocess, sys, tempfile, time
p = argparse.ArgumentParser()
p.add_argument('--mode', default='full')
p.add_argument('--seed', type=int, default=1)
p.parse_args()
here = pathlib.Path(__file__).resolve()
header = here.parents[2] / '@HEADER@'
if header.read_bytes() == here.with_name(here.stem + '.golden').read_bytes():
    sys.exit(0)  # the baselines: the unmutated header needs no compile and no probe
lock = open(@LOCK@, 'a')
fcntl.flock(lock, fcntl.LOCK_SH)
driver = ('#include "' + str(header) + '"\nvolatile int P0 = 0, P1 = 0;\n'
          'int main() { int p[2] = {P0, P1}; __builtin_printf("%d %d\\n", root(p, 0), root(p, 1)); }\n')
with tempfile.TemporaryDirectory(dir=@TMP@) as d:
    src, exe = pathlib.Path(d, 'main.cpp'), pathlib.Path(d, 'main')
    src.write_text(driver)
    if subprocess.run(['@CXX@', '-std=gnu++20', '-O0', '-o', str(exe), str(src)], capture_output=True).returncode:
        sys.exit('compile failed')
    r = subprocess.run([str(exe)], capture_output=True, text=True)
if r.returncode or r.stdout.split() != ['0', '0']:
    sys.exit('check failed')
fcntl.flock(lock, fcntl.LOCK_UN)
end = time.monotonic() + @PROBE@
while time.monotonic() < end:
    try:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        time.sleep(3600)  # another test is running: this survivor is slow under contention
    fcntl.flock(lock, fcntl.LOCK_UN)
    time.sleep(0.05)
'''
# runs 04-mutation.py's main in a subprocess with ROOT patched to a temporary repository; DRIVER_KEEP_PREFLIGHT is the
# negative control of the pre-flight test (it undoes the campaign's CP_TEST_PREFLIGHT=0 before ensure)
DRIVER = '''import importlib.util, os, sys
from pathlib import Path
spec = importlib.util.spec_from_file_location('mutation_driven', sys.argv[1])
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)
m.ROOT = Path(sys.argv[2])
if os.environ.get('DRIVER_KEEP_PREFLIGHT'):
    ensure = m._00_memory_cap.ensure
    def keep(*a, **k):
        os.environ.pop('CP_TEST_PREFLIGHT', None)
        return ensure(*a, **k)
    m._00_memory_cap.ensure = keep
sys.exit(m.main(sys.argv[3:]))
'''
# stateful fake mewt for whole campaigns: mutants [id, slug, line, severity] and per-id outcomes {stage or '*': status}
# (default TestFail) from @CFG@, one test result file per shard, every call appended to @LOG@; sleep {stage: s} per id
FAKE_CAMPAIGN = r'''import datetime, json, os, sys, time
cfg = json.load(open(@CFG@))
argv = sys.argv[1:]
argv = argv[:-2] if argv[-2:] == ['--log.color', 'off'] else argv
with open(@LOG@, 'a') as f:
    f.write(json.dumps(dict(argv=argv, cwd=os.getcwd())) + '\n')
mutants = cfg['mutants']
def load():
    return json.load(open('fake-results.json')) if os.path.exists('fake-results.json') else {}
def stamp():
    s, ns = divmod(time.time_ns(), 10 ** 9)
    return datetime.datetime.fromtimestamp(s, datetime.timezone.utc).strftime('%Y-%m-%dT%H:%M:%S') + '.%09dZ' % ns
if argv == ['--version']:
    print('mewt 4.0.0')
elif argv == ['mutate']:
    pass
elif argv == ['print', 'mutations']:
    print(' Available mutations for cpp:')
    for slug, sev in sorted({(m[1], m[3]) for m in mutants}):
        print('   [%s] Name: text (Severity: %s)' % (slug, sev.capitalize()))
    print(' Available mutations for go:\n   [ZZ] Other: text (Severity: High)')
elif argv == ['print', 'mutants', '--format', 'ids']:
    print('\n'.join(str(m[0]) for m in mutants))
elif argv[:2] == ['print', 'mutants']:
    o = dict(zip(argv[2::2], argv[3::2]))
    sev, types = o.get('--severity'), o.get('--mutation-types')
    print(' Target: x (cpp)')
    for m in mutants:
        if (not sev or m[3] in sev.split(',')) and (not types or m[1] in types.split(',')):
            print("   [%s %d] Line %d: 'old' -> 'new'" % (m[1], m[0], m[2]))
elif argv[0] == 'test':
    o = dict(zip(argv[1::2], argv[2::2]))
    stage, done = o['--ids-file'][:-4], load()
    for i in open(o['--ids-file']).read().split():
        time.sleep(cfg['sleep'].get(stage, 0))
        plan = cfg['outcomes'].get(i, {})
        done[i] = dict(status=plan.get(stage, plan.get('*', 'TestFail')), time=stamp())
        json.dump(done, open('fake-results.tmp', 'w'))
        os.replace('fake-results.tmp', 'fake-results.json')
elif argv == ['results', '--all', '--format', 'json']:
    done = load()
    print(json.dumps(dict(results=[dict(mutant=dict(id=m[0], mutation_slug=m[1], line_offset=m[2] - 1, old_text='old %d' % m[0],
                                                    new_text='new %d' % m[0]), **({'outcome': done[str(m[0])]} if str(m[0]) in done else {}))
                                   for m in mutants])))
else:
    sys.exit('fake mewt: unexpected call %r' % argv)
'''
# quick checks nothing (after sleeping); full compares the header with the .golden copy beside the tester (every mutant escalates once)
GOLDEN_TESTER = r'''import argparse, pathlib, sys, time
p = argparse.ArgumentParser()
p.add_argument('--mode', default='full')
p.add_argument('--seed', type=int, default=1)
a = p.parse_args()
here = pathlib.Path(__file__).resolve()
time.sleep(@SLEEP@ if a.mode == 'quick' else 0)
same = (here.parents[2] / '@HEADER@').read_bytes() == here.with_name(here.stem + '.golden').read_bytes()
sys.exit(0 if a.mode != 'full' or same else 'golden mismatch')
'''


def o_stage(line, slug, new):
    """Stage of the toy tester that catches the mutant (None: the equivalent survivor), derived by hand.
    ER: undeclared std::runtime_error, compile error. Line 2 (add): CR misses the return (-Werror=return-type),
    RDV and * give add(2,0)=0, / and % divide by zero (SIGFPE); only - keeps add(2,0)=2 and gives add(2,3)=-1.
    Line 4 (if): CR/IF give clampHi(5,3)=5, IT and != < <= give clampHi(1,3)=3, == gives clampHi(5,3)=5, RDV gives 0;
    x >= hi returns hi == x on equality, so it is equivalent. Line 5: CR misses the return, RDV gives clampHi(1,3)=0."""
    if slug == 'ER':
        return 'quick'
    if line == 2:
        return 'full-optimized' if (slug, new) == ('AOS', '-') else 'quick'
    if line == 4:
        return None if (slug, new) == ('COS', '>=') else 'full-all'
    if line == 5:
        return 'quick' if slug == 'CR' else 'full-all'
    raise AssertionError(f'unexpected toy mutant on line {line}: {slug} {new!r}')


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='mutation-tester-')).resolve()
        self.addCleanup(lambda: shutil.rmtree(self.tmp, ignore_errors=True) if self._outcome.success else None)
        env = patch.dict(os.environ, ISOLATED, clear=True)
        env.start()
        self.addCleanup(env.stop)
        os.environ['CP_MUTATION_LOCK'] = str(self.tmp / 'machine.lock')  # never the default: a real campaign may hold it for hours
        self.bin = self.tmp / 'bin'
        self.bin.mkdir()
        (self.bin / 'git').symlink_to(GIT)
        self.rng = random.Random(f'{SEED}:{self.id()}')

    def info(self, **kw):
        return f'seed={SEED} mode={MODE} fixture={self.tmp} ' + ' '.join(f'{k}={v!r}' for k, v in kw.items())

    def eq(self, actual, expected, **kw):
        self.assertEqual(actual, expected, self.info(**kw))

    def true(self, cond, **kw):
        self.assertTrue(cond, self.info(**kw))

    def path_only(self):
        return patch.dict(os.environ, {'PATH': str(self.bin)})

    def rounds(self):
        return ROUNDS if MODE == 'stress' else 2 if MODE == 'full' else 1

    def main(self, *argv, root=None):
        out, err = io.StringIO(), io.StringIO()
        handlers = {sig: signal.getsignal(sig) for sig in (signal.SIGTERM, signal.SIGHUP)}  # campaign() replaces them
        self.addCleanup(lambda: [signal.signal(sig, h) for sig, h in handlers.items()])
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err), patch.object(M, 'ROOT', root or M.ROOT):
            try:
                code = M.main(list(argv))
            except SystemExit as e:
                code = e.code
        return code, out.getvalue(), err.getvalue()

    def fake_version(self, line):
        executable(self.bin / 'mewt', f"#!/bin/sh\nprintf '%s\\n' '{line}'\n")

    def procs(self, work):
        """(pid, comm) of every process whose working directory lies in work, now."""
        out = []
        for d in Path('/proc').glob('[0-9]*'):
            with contextlib.suppress(OSError):
                if os.readlink(d / 'cwd').startswith(str(work)):
                    out.append((d.name, (d / 'comm').read_text().strip()))
        return out

    def left(self, work):
        """procs(work) after up to 10 s for them to end."""
        until, left = time.monotonic() + 10, self.procs(work)
        while left and time.monotonic() < until:
            time.sleep(0.2)
            left = self.procs(work)
        return left

    def wait_for(self, path, text, p, seconds=120):
        """Poll the log at path until it contains text or the process p ends; True when it does."""
        until = time.monotonic() + seconds
        while text not in path.read_text() and p.poll() is None and time.monotonic() < until:
            time.sleep(0.1)
        return text in path.read_text()


class Pure(Base):
    def test_blob_fixed(self):
        for data in (b'', b'\0', b'hello\n', b'x' * 10000):
            self.eq(M.blob(data), o_blob(data), data=data[:20])
        self.eq(M.blob(b''), EMPTY_BLOB)

    def test_blob_random(self):
        for r in range(self.rounds()):
            for _ in range(15):
                data = self.rng.randbytes(self.rng.choice([0, 1, 7, 64, 1000, 5000]))
                self.eq(M.blob(data), o_blob(data), round=r, data=data[:20])

    def test_evidence_path(self):
        for h, d in (('02-Data Structures/01-dsu.hpp', '02-Data Structures/Docs/01-dsu.md'),
                     ('01-Core/07-infint.hpp', '01-Core/Docs/07-infint.md'),
                     ('07-Strings/12-suffix.array.hpp', '07-Strings/Docs/12-suffix.array.md')):
            self.eq(M.evidence(h), d, header=h)

    def test_memory_helpers(self):
        for text, want in (('ensure()\n', M._00_memory_cap.MAIN_MB), ('_00_memory_cap.ensure(mb=6144)\n', 6144),
                           ('ensure(force=True, mb=12288)\n', 12288), ('', M._00_memory_cap.MAIN_MB)):
            f = self.tmp / 'x_tester.py'
            f.write_text(text)
            self.eq(M.tester_cap(f), want, text=text)
        self.eq(M.capped(['py', 'a b'], 1500), [sys.executable, str(M.HERE / '_00_memory_cap.py'), '--mb', '1500', '--', 'py', 'a b'])
        line = next(l for l in Path('/proc/meminfo').read_text().splitlines() if l.startswith('MemAvailable:'))
        self.true(abs(M.available_mb() - (int(line.split()[1]) >> 10)) < 512, line=line, got=M.available_mb())

    def test_stop_handler(self):
        both = (signal.SIGTERM, signal.SIGHUP)
        old = {s: signal.getsignal(s) for s in both}
        self.addCleanup(lambda: [signal.signal(s, h) for s, h in old.items()])
        for first in both:  # a real delivery of either raises KeyboardInterrupt once; then both are ignored
            for s in both:
                signal.signal(s, M.stop)
            with self.assertRaises(KeyboardInterrupt, msg=self.info(signal=first)):
                os.kill(os.getpid(), first)
                time.sleep(5)
            self.eq([signal.getsignal(s) for s in both], [signal.SIG_IGN] * 2, signal=first)
            for s in both:
                os.kill(os.getpid(), s)  # ignored: this process lives on

    def recorded(self, text):
        h = '04-Graphs/03-x.hpp'
        if text is not None:
            put(self.tmp, {'04-Graphs/Docs/03-x.md': text})
        return M.recorded(self.tmp, h)

    def test_recorded_cases(self):
        a, b = 'a' * 40, '0123456789abcdef0123456789abcdef01234567'
        cases = [
            (None, None),
            ('# x\n', None),
            ('# x\n## Mutation testing\n\nno token\n', None),
            (record_doc(a), a),
            (f'# x\n## Mutation testing \t\nRecord: blob `{b}`.\n', b),
            (f'# x\n## Mutation testing\n\n### Sub\n\nblob `{a}`\n', a),
            (f'# x\n## Mutation testing\n\ntext\n## Next\n\nblob `{a}`\n', None),
            (f'# x\n\nblob `{a}`\n\n## Mutation testing\n\nnone\n', None),
            (f'# x\n## Mutation testing results\nblob `{a}`\n', None),
            (f'# x\n## Mutation testing\nblob `{a.upper()}` blob `{a[:39]}` blob {a}\n', None),
            (f'# x\n## Mutation testing\nblob `{a}` and blob `{b}`\n## Mutation testing\nblob `{b}`\n', a),
            (f'## Mutation testing\nblob `{b}`', b),
        ]
        for text, want in cases:
            self.eq(self.recorded(text), want, doc=text)
            (self.tmp / '04-Graphs/Docs/03-x.md').unlink(missing_ok=True)

    def test_record_cases(self):
        a, b = 'a' * 40, 'b' * 40
        cases = [
            (None, None),
            ('# x\n## Mutation testing\n\n0 untested\n', None),
            (record_doc(a), (a, 0)),
            (record_doc(a, untested=12), (a, 12)),
            (record_doc(a, untested=None), (a, None)),
            (f'## Mutation testing\n\n7 untested before\nRecord: blob `{a}`: 9 mutants.\n0 untested after\n', (a, None)),
            (f'## Mutation testing\nStage times.\nRecord: blob `{a}` (x): 1 mutant, 3 untested.\nRecord: blob `{b}`: 0 untested.\n', (a, 3)),
            (f'## Mutation testing\nRecord: blob `{a}`: 10 caught, 0 untested.\n## Next\nblob `{b}` 5 untested\n', (a, 0)),
            (f'## Mutation testing\nRecord: blob `{a}`: untested 4.\n', (a, None)),
        ]
        for text, want in cases:
            (self.tmp / '04-Graphs/Docs/03-x.md').unlink(missing_ok=True)
            if text is not None:
                put(self.tmp, {'04-Graphs/Docs/03-x.md': text})
            self.eq((M.record(self.tmp, '04-Graphs/03-x.hpp'), M.recorded(self.tmp, '04-Graphs/03-x.hpp')), (want, want and want[0]), doc=text)

    def test_library_path(self):
        root = self.tmp / 'root'
        put(root, {'02-Data Structures/01-a.hpp': 'a', 'x.hpp': 'x'})
        cwd = os.getcwd()
        self.addCleanup(os.chdir, cwd)
        os.chdir(root / '02-Data Structures')
        with patch.object(M, 'ROOT', root):
            for arg, want in ((str(root / '02-Data Structures/01-a.hpp'), '02-Data Structures/01-a.hpp'),
                              ('02-Data Structures/01-a.hpp', '02-Data Structures/01-a.hpp'), ('01-a.hpp', '02-Data Structures/01-a.hpp'),
                              ('x.hpp', 'x.hpp'), ('../x.hpp', 'x.hpp'), ('02-new.hpp', '02-Data Structures/02-new.hpp'),
                              (str(self.tmp / 'elsewhere.hpp'), '../elsewhere.hpp')):
                self.eq(M.library_path(arg), want, arg=arg)

    def test_stale_reasons(self):
        h1, h2, h3 = '02-Data Structures/01-a.hpp', '02-Data Structures/02-b.hpp', '02-Data Structures/03-c.hpp'
        h4, h5 = '02-Data Structures/04-d.hpp', '02-Data Structures/05-e.hpp'
        put(self.tmp, {h1: b'one\n', h2: b'two\n', h3: b'three\n', h4: b'four\n', h5: b'five\n',
                       '02-Data Structures/Docs/02-b.md': record_doc('f' * 40), '02-Data Structures/Docs/03-c.md': record_doc(o_blob(b'three\n')),
                       '02-Data Structures/Docs/04-d.md': record_doc(o_blob(b'four\n'), untested=5),
                       '02-Data Structures/Docs/05-e.md': record_doc(o_blob(b'five\n'), untested=None)})
        cur2 = o_blob(b'two\n')
        rest = [(h2, f'02-Data Structures/Docs/02-b.md records blob {"f" * 12}, the header is {cur2[:12]}'),
                (h4, 'the record in 02-Data Structures/Docs/04-d.md is incomplete (5 untested count)'),
                (h5, 'the record in 02-Data Structures/Docs/05-e.md is incomplete (no untested count)')]
        self.eq(M.stale(self.tmp, [h1, h2, h3, h4, h5]), [(h1, 'no `## Mutation testing` record with a blob in 02-Data Structures/Docs/01-a.md')] + rest)
        self.eq(M.stale(self.tmp, [h1, h2, h3, h4, h5], existing=True), rest)

    def test_check_cli(self):
        h1, h2 = '02-Data Structures/01-a.hpp', '04-Graphs/02-b.hpp'
        put(self.tmp, {h1: b'one\n', h2: b'two\n', '04-Graphs/Docs/02-b.md': record_doc(o_blob(b'two\n')), 'README.md': 'x'})
        code, out, err = self.main('check', h2, h1, h2, root=self.tmp)
        self.eq((code, out.splitlines()), (1, [f'FAIL {h1}: no `## Mutation testing` record with a blob in 02-Data Structures/Docs/01-a.md',
                                               'FAIL: 1 of 2 mutation records current']), err=err)
        self.eq(self.main('check', h2, root=self.tmp)[:2], (0, 'PASS: 1 of 1 mutation records current\n'))
        self.eq(self.main('check', root=self.tmp)[:2], (0, 'PASS: 0 of 0 mutation records current\n'))
        self.eq(self.main('check', str(self.tmp / h2), root=self.tmp)[:2], (0, 'PASS: 1 of 1 mutation records current\n'), what='absolute path')
        self.eq(self.main('check', '--existing', h1, h2, root=self.tmp)[:2], (0, 'PASS: 2 of 2 mutation records current\n'))
        put(self.tmp, {'04-Graphs/Docs/02-b.md': record_doc(o_blob(b'two\n'), untested=2)})
        self.eq(self.main('check', '--existing', h1, h2, root=self.tmp)[:2], (1, f'FAIL {h2}: the record in 04-Graphs/Docs/02-b.md is incomplete '
                                                                                 '(2 untested count)\nFAIL: 1 of 2 mutation records current\n'))
        cwd = os.getcwd()
        self.addCleanup(os.chdir, cwd)
        os.chdir(self.tmp)  # an argument missing under ROOT is resolved against the cwd
        for bad in ('README.md', '02-Data Structures/09-missing.hpp', '02-Data Structures/98-basic.hpp', 'x/' + h1):
            code, out, err = self.main('check', h1, bad, root=self.tmp)
            self.eq((code, out, err.strip()), (1, '', f'FAIL check: not library headers: {bad}'), arg=bad)

    def test_check_random(self):
        kinds = ('nodoc', 'nosection', 'notoken', 'after', 'current', 'other', 'incomplete', 'nocount')
        for r in range(self.rounds()):
            root, want, headers, existing = self.tmp / f'r{r}', {}, [], self.rng.random() < 0.3
            for k in range(self.rng.randint(1, 7)):
                folder = self.rng.choice(['01-Core', '02-Data Structures', '05-Mathematics', '07-Strings'])
                h, data = f'{folder}/{k:02d}-h{k}.hpp', self.rng.randbytes(self.rng.randint(0, 80))
                put(root, {h: data})
                cur, other, kind, untested = o_blob(data), self.rng.randbytes(20).hex(), self.rng.choice(kinds), self.rng.randint(1, 99)
                noise = self.rng.choice(['', f'## Notes\n\nblob `{other}`\n\n', '## Tests\n\n### Sub\n\n'])
                heading = '## Mutation testing' + self.rng.choice(['', ' ', '\t', ' \t '])
                doc = {'nosection': f'# E\n\n{noise}## Other\n\nblob `{cur}`\n',
                       'notoken': f'# E\n\n{noise}{heading}\n\nblob `{cur[:39]}` and blob {cur}\n',
                       'after': f'# E\n\n{heading}\n\nnone\n\n## Later\n\nblob `{cur}`\n',
                       'current': f'# E\n\n{noise}{heading}\n\n### Run\n\nRecord: blob `{cur}`: 0 untested.\n' + self.rng.choice(['', '\n## Next\n']),
                       'other': f'# E\n\n{heading}\n\nRecord: blob `{other}`: 0 untested.\n\n{noise}',
                       'incomplete': f'# E\n\n{heading}\n\nRecord: blob `{cur}`: {untested} untested.\n\n{noise}',
                       'nocount': f'# E\n\n{heading}\n\n0 untested\nRecord: blob `{cur}`.\n\n{noise}'}.get(kind)
                if doc is not None:
                    put(root, {M.evidence(h): doc})
                d = M.evidence(h)
                missing = None if existing else f'no `## Mutation testing` record with a blob in {d}'
                want[h] = {'current': None, 'other': f'{d} records blob {other[:12]}, the header is {cur[:12]}',
                           'incomplete': f'the record in {d} is incomplete ({untested} untested count)',
                           'nocount': f'the record in {d} is incomplete (no untested count)'}.get(kind, missing)
                headers.append(h)
            argv = headers + self.rng.sample(headers, self.rng.randint(0, len(headers)))
            self.eq(M.stale(root, sorted(want), existing), [(h, w) for h, w in sorted(want.items()) if w], round=r, want=want)
            code, out, _ = self.main('check', *argv, *(['--existing'] if existing else []), root=root)
            bad = [f'FAIL {h}: {w}' for h, w in sorted(want.items()) if w]
            self.eq((code, out.splitlines()), (int(bool(bad)), bad + [f'{"FAIL" if bad else "PASS"}: {len(want) - len(bad)} of {len(want)} '
                                                                      'mutation records current']), round=r, argv=argv)


class Lines(Base):
    def lines(self, xs, trailing=True):
        return ('\n'.join(xs) + ('\n' if trailing and xs else '')).encode()

    def test_lines_fixed(self):
        base = [f'line {i}' for i in range(10)]
        old = self.lines(base)
        self.eq(M.changed_lines(old, old), set())
        self.eq(M.changed_lines(old, self.lines(base, trailing=False)), set())
        for k in (1, 3):
            self.eq(M.changed_lines(old, self.lines(base + [f'new {i}' for i in range(k)])), set(range(11, 11 + k)), k=k)
        for i in (0, 4, 9):
            edited = base[:i] + ['edited'] + base[i + 1:]
            self.eq(M.changed_lines(old, self.lines(edited)), {i + 1}, edit=i)
        self.eq(M.changed_lines(old, self.lines(base[:4] + base[5:])), {4, 5})
        self.eq(M.changed_lines(old, self.lines(base[1:])), {1})
        self.eq(M.changed_lines(old, self.lines(base[:-1])), {9})
        self.eq(M.changed_lines(old, b''), set())
        self.eq(M.changed_lines(b'', old), set(range(1, 11)))
        self.eq(M.changed_lines(b'', b''), set())

    def test_lines_random(self):
        for r in range(self.rounds()):
            for _ in range(400):
                a = [self.rng.choice('xyz') for _ in range(self.rng.randint(0, 12))]
                b = [self.rng.choice('xyz') for _ in range(self.rng.randint(0, 12))]
                got = M.changed_lines(self.lines(a, self.rng.random() < 0.5), self.lines(b, self.rng.random() < 0.5))
                kept = [line for j, line in enumerate(b, 1) if j not in got]
                self.true(all(1 <= j <= len(b) for j in got) and o_subsequence(kept, a) and (a != b or not got),
                          round=r, old=a, new=b, marked=sorted(got))


class Repo(Base):
    def test_changed_sets(self):
        A, B, C, D = '02-Data Structures/01-a.hpp', '02-Data Structures/02-b.hpp', '04-Graphs/03-c.hpp', '05-Mathematics/04-d.hpp'
        Mh, N, U, I = '02-Data Structures/06-m.hpp', '07-Strings/01-new.hpp', '06-Miscellaneous/02-u.hpp', '04-Graphs/09-ignored.hpp'
        root = init_repo(self.tmp / 'repo')
        put(root, {A: 'a', B: 'b', C: 'c', D: 'd', Mh: 'm', '02-Data Structures/98-basic.hpp': 'agg', '.gitignore': I + '\n',
                   '02-Data Structures/Docs/01-a.md': '# a\n'})
        commit(root)
        git(root, 'checkout', '-q', '-b', 'feat')
        put(root, {A: 'a2'})
        commit(root)
        (root / D).unlink()
        put(root, {N: 'n', '02-Data Structures/99-all.hpp': 'all', '08-Python/01-py.hpp': 'p', '02-Data Structures/sub/01-deep.hpp': 'x',
                   'README.md': 'r', '02-Data Structures/01-a.cpp': 'c'})
        commit(root)
        git(root, 'checkout', '-q', 'master')
        put(root, {Mh: 'm2'})
        commit(root)
        git(root, 'checkout', '-q', 'feat')
        put(root, {B: 'b2', C: 'c2', U: 'u', I: 'i', '03-Geometry/02-u.txt': 'u'})
        git(root, 'add', C)
        self.eq(M.changed(root), sorted([A, B, C, N, U]))
        self.eq(M.changed(root, 'feat'), sorted([B, C, U]))
        with self.assertRaises(M.Failure):
            M.changed(root, 'no-such-base')
        put(root, {M.evidence(A): record_doc(o_blob(b'a2'))})
        code, out, err = self.main('check', '--changed', root=root)
        stale = [h for h in sorted([A, B, C, N, U]) if h != A]
        self.eq((code, [l.split(':')[0] for l in out.splitlines()]), (1, [f'FAIL {h}' for h in stale] + ['FAIL']), out=out, err=err)
        self.true(out.endswith('FAIL: 1 of 5 mutation records current\n'), out=out)
        code, out, err = self.main('check', '--changed', 'feat', A, root=root)
        self.true(code == 1 and out.endswith('FAIL: 1 of 4 mutation records current\n'), out=out)
        code, out, err = self.main('check', '--changed', 'no-such-base', root=root)
        self.true(code == 1 and out == '' and err.startswith('FAIL check: git diff'), err=err)

    def scope_repo(self):
        h = '02-Data Structures/01-toy.hpp'
        root = init_repo(self.tmp / 'repo')
        put(root, {h: TOY, '02-Data Structures/Docs/01-toy.md': '# toy\n'})
        commit(root)
        return root, h

    def test_scope_cases(self):
        root, h = self.scope_repo()
        old, new = TOY.encode(), TOY.replace('return x;', 'return x + 0;').encode()
        bo, bn = o_blob(old), o_blob(new)
        notes = []
        with patch.object(M, 'ROOT', root):
            for doc, data, now, whole, want, note in (
                    (None, new, bn, False, (None, None), None),
                    (record_doc(bo), new, bn, False, (bo, [5]), None),
                    (record_doc(bo), old, bo, False, (bo, []), None),
                    (record_doc(bn), new, bn, False, (bn, []), None),  # current, blob uncommitted
                    (record_doc(bo), new, bn, True, (None, None), None),
                    (record_doc('b' * 40), new, bn, False, (None, None), f'NOTE blob {"b" * 12} of the record is not in this repository: '
                                                                         'whole-file campaign'),
                    (record_doc(bn, untested=4), new, bn, False, (None, None), f'NOTE the record of blob {bn[:12]} leaves mutants untested: '
                                                                               'whole-file campaign'),
                    (record_doc(bn, untested=None), new, bn, False, (None, None), f'NOTE the record of blob {bn[:12]} leaves mutants untested: '
                                                                                  'whole-file campaign'),
                    (record_doc(bn, untested=4), new, bn, True, (None, None), None)):
                (root / M.evidence(h)).write_text(doc or '# toy\n')
                with contextlib.redirect_stdout(io.StringIO()) as out:
                    self.eq(M.scope(h, data, now, whole), want, doc=doc, whole=whole)
                self.eq(out.getvalue(), f'{note}\n' if note else '', doc=doc, whole=whole)

    def test_current_line_endings(self):
        # git stores a CRLF header under `* text=auto eol=lf` as its LF form: the record names that blob, not the raw bytes'
        h, raw = '02-Data Structures/01-crlf.hpp', TOY.replace('\n', '\r\n').encode()
        root = init_repo(self.tmp / 'repo')
        put(root, {'.gitattributes': '* text=auto eol=lf\n', h: raw})
        commit(root)
        stored = o_blob(TOY.encode())
        self.true((root / h).read_bytes() == raw and M.blob(raw) != stored and git(root, 'rev-parse', f'HEAD:{h}').strip() == stored,
                  what='fixture: the checkout keeps CRLF, the object store holds LF')
        self.eq(M.current(root, h), stored)
        put(root, {M.evidence(h): record_doc(stored)})
        self.eq((M.stale(root, [h]), self.main('check', h, root=root)[:2]), ([], (0, 'PASS: 1 of 1 mutation records current\n')))
        put(root, {M.evidence(h): record_doc(M.blob(raw))})
        self.eq(M.stale(root, [h]), [(h, f'{M.evidence(h)} records blob {M.blob(raw)[:12]}, the header is {stored[:12]}')])
        put(root, {M.evidence(h): record_doc(stored), f'96-Local Testing/02-Data Structures/01-crlf_tester.py': 'print(1)\n'})
        self.fake_version('mewt 4.0.0')
        with self.path_only():
            code, out, err = self.main('run', h, '--workdir', str(self.tmp / 'work'), root=root)
        self.eq((code, out), (0, f'PASS {h}: the record already names blob {stored[:12]}; --whole reruns the whole file\n'), err=err)


class Install(Base):
    def tarball(self, members):
        path = self.tmp / f'rel-{self.rng.randrange(10 ** 9)}.tar.xz'
        with tarfile.open(path, 'w:xz') as t:
            for name, data, mode in members:
                info = tarfile.TarInfo(name)
                info.mode = mode
                if data == 'dir':
                    info.type = tarfile.DIRTYPE
                    t.addfile(info)
                elif data == 'sym':
                    info.type, info.linkname = tarfile.SYMTYPE, '/etc/passwd'
                    t.addfile(info)
                else:
                    info.size = len(data)
                    t.addfile(info, io.BytesIO(data))
        with tarfile.open(path) as t:
            self.eq(t.getnames(), [m[0] for m in members], what='fixture names round-trip')
        return path.as_uri(), hashlib.sha256(path.read_bytes()).hexdigest()

    def install(self, prefix, url, digest):
        with contextlib.redirect_stdout(io.StringIO()):
            M.install(prefix, url, digest)

    def test_install_layout(self):
        url, digest = self.tarball([('mewt-x86_64', 'dir', 0o755), ('mewt-x86_64/mewt', b'#!/bin/sh\necho mewt\n', 0o755),
                                    ('mewt-x86_64/README.md', b'readme', 0o644), ('../evil', b'e1', 0o644), ('/abs/evil2', b'e2', 0o600),
                                    ('deep/a/b/lowexec', b'x', 0o100), ('mewt-x86_64/link', 'sym', 0o777)])
        prefix = self.tmp / 'prefix'
        dest, link = prefix / 'opt' / f'mewt-{M.MEWT_VERSION}', prefix / 'bin' / 'mewt'
        link.parent.mkdir(parents=True)
        link.write_text('old file')
        for attempt in range(2):
            self.install(prefix, url, digest)
            files = {p.name: (p.read_bytes(), p.stat().st_mode & 0o777) for p in dest.iterdir()}
            self.eq(files, {'mewt': (b'#!/bin/sh\necho mewt\n', 0o755), 'README.md': (b'readme', 0o644), 'evil': (b'e1', 0o644),
                            'evil2': (b'e2', 0o644), 'lowexec': (b'x', 0o755)}, attempt=attempt)
            self.true(link.is_symlink() and os.readlink(link) == str(dest / 'mewt'), attempt=attempt)
        everything = sorted(str(p.relative_to(self.tmp)) for p in self.tmp.rglob('*') if not p.name.startswith('rel-'))
        self.eq([p for p in everything if 'evil' in p], [f'prefix/opt/mewt-{M.MEWT_VERSION}/evil', f'prefix/opt/mewt-{M.MEWT_VERSION}/evil2'])
        self.true(not Path('/abs/evil2').exists())
        link.unlink()
        link.symlink_to(self.tmp / 'nowhere')
        self.install(prefix, url, digest)
        self.true(os.readlink(link) == str(dest / 'mewt'), what='dangling link replaced')
        cwd = os.getcwd()
        self.addCleanup(os.chdir, cwd)
        os.chdir(self.tmp)
        self.install(Path('rel/p'), url, digest)  # a relative prefix is resolved: the link target is absolute and valid
        rel = self.tmp / 'rel/p/bin/mewt'
        self.true(os.readlink(rel) == str(self.tmp / f'rel/p/opt/mewt-{M.MEWT_VERSION}/mewt') and rel.resolve().is_file(), link=os.readlink(rel))

    def test_install_failures(self):
        url, digest = self.tarball([('d/mewt', b'm', 0o755)])
        bad = ('0' if digest[0] != '0' else '1') + digest[1:]
        for u, d, words in ((url, bad, 'has sha256'), *((self.tarball(m) + ('holds no mewt',)) for m in (
                [('d/README', b'r', 0o644)], [('mewt', 'dir', 0o755), ('x/mewt', 'sym', 0o777)]))):
            prefix = self.tmp / f'p-{words[:4]}-{self.rng.randrange(10 ** 6)}'
            with self.assertRaises(M.Failure) as e:
                self.install(prefix, u, d)
            self.true(words in str(e.exception) and not (prefix / 'opt').exists() and not (prefix / 'bin').exists(), url=u, error=str(e.exception))

    def test_install_path_warning(self):
        url, digest = self.tarball([('d/mewt', b'#!/bin/sh\necho mewt\n', 0o755)])
        prefix = self.tmp / 'prefix'
        dest, link = prefix / 'opt' / f'mewt-{M.MEWT_VERSION}', prefix / 'bin' / 'mewt'
        other, alias = self.tmp / 'other', self.tmp / 'alias'
        other.mkdir()
        alias.mkdir()
        executable(other / 'mewt', '#!/bin/sh\n')
        (alias / 'mewt').symlink_to(dest / 'mewt')  # another directory whose mewt resolves to the installed binary
        warn = f'WARNING {prefix / "bin"} is not first on PATH for mewt: add it before campaigns run, or they SKIP'
        for path, warned in ((str(self.bin), True), (f'{prefix / "bin"}:{other}', False), (f'{other}:{prefix / "bin"}', True),
                             (f'{alias}:{other}', False), (f'{self.bin}:{prefix / "bin"}', False)):
            with patch.dict(os.environ, {'PATH': path}), contextlib.redirect_stdout(io.StringIO()) as out:
                M.install(prefix, url, digest)
            self.eq(out.getvalue().splitlines(), [f'installed mewt {M.MEWT_VERSION} in {dest}, linked as {link}'] + ([warn] if warned else []),
                    path=path)

    def test_install_cli_prefix(self):
        with patch.object(M, 'install') as fake:
            self.eq(self.main('install', '--prefix', str(self.tmp / 'p'))[0], 0)
        fake.assert_called_once_with(self.tmp / 'p')


class Version(Base):
    def test_version_cases(self):
        with self.path_only():
            self.eq(M.mewt_version(), None, case='missing')
            (self.bin / 'mewt').write_text('#!/bin/sh\necho mewt 4.0.0\n')
            self.eq(M.mewt_version(), None, case='not executable')
            for line, want in (('mewt 4.0.0', '4.0.0'), ('mewt version 12.34.56-beta (x 1.2.3)', '12.34.56'), ('mewt 4.0', ''), ('', '')):
                self.fake_version(line)
                self.eq(M.mewt_version(), want, output=line)
            executable(self.bin / 'mewt', '#!/bin/sh\necho mewt 4.0.0 >&2\n')
            self.eq(M.mewt_version(), '', case='stderr only')


class Stage(Base):
    def setUp(self):
        super().setUp()
        self.log = self.tmp / 'log'
        self.log.mkdir()
        executable(self.bin / 'mewt', f'#!{sys.executable}\n' + FAKE_MEWT.replace('@LOG@', repr(str(self.log))))

    def shards(self, cfgs):
        out = []
        for k, cfg in enumerate(cfgs):
            s = self.tmp / 'work' / f's{k}'
            s.mkdir(parents=True)
            (s / 'fake.json').write_text(json.dumps(cfg))
            out.append(s)
        return out

    def records(self):
        return [json.loads(p.read_text()) for p in self.log.iterdir()]

    def test_stage_runs_concurrently(self):
        shards = self.shards([{'sleep': 0.6}] * 6)
        todo = {s: ([k, k + 6, k + 12] if k != 3 else []) for k, s in enumerate(shards)}
        cmd = "python3 'x y/t.py' --mode quick"
        with self.path_only():
            start = time.monotonic()
            self.true(M.stage('quick', todo, cmd, 77, time.monotonic() + 60), what='complete')
        recs = {Path(r['cwd']): r for r in self.records()}
        self.eq(sorted(recs), [s for s in shards if todo[s]])
        for s in shards:
            if not todo[s]:
                self.true(not (s / 'quick.ids').exists() and not (s / 'quick.log').exists(), shard=s.name)
                continue
            r = recs[s]
            self.eq((s / 'quick.ids').read_text(), ''.join(f'{i}\n' for i in todo[s]), shard=s.name)
            self.eq(r['argv'], ['test', '--ids-file', 'quick.ids', '--test.cmd', cmd, '--test.timeout', '77', '--log.color', 'off'])
            self.true(r['pgid'] == r['pid'] and 'sigint' not in r and 'fake mewt ran' in (s / 'quick.log').read_text(), rec=r)
        self.eq(o_overlap([(r['start'], r['end']) for r in recs.values()]), 5, recs=recs, what='every nonempty shard at once')
        self.true(time.monotonic() - start < 10, what='one wave of 0.6 s')

    def test_stage_deadline(self):
        shards = self.shards([{'sleep': 30}] * 3)
        todo = {s: [k] for k, s in enumerate(shards)}
        with self.path_only():
            start = time.monotonic()
            self.eq(M.stage('full', todo, 'true', 60, time.monotonic() + 1), False)
            took = time.monotonic() - start
        recs = self.records()
        self.true(took < 10 and len(recs) == 3 and all(r.get('sigint') for r in recs), took=took, recs=recs)
        for r in recs:
            with self.assertRaises(ProcessLookupError):
                os.kill(r['pid'], 0)
        for s in shards:
            (s / 'full.ids').unlink()
        with self.path_only():
            self.eq(M.stage('full', todo, 'true', 60, time.monotonic()), False, what='deadline already passed')
        self.eq(len(self.records()), 3, what='a passed deadline launches no mewt')
        self.true(not any((s / 'full.ids').exists() for s in shards), what='nor writes ids')

    def test_stage_failure(self):
        shards = self.shards([{'sleep': 0.3, 'code': 3, 'say': 'boom-line'}, {'sleep': 30}])
        with self.path_only():
            start = time.monotonic()
            with self.assertRaises(M.Failure) as e:
                M.stage('quick', {s: [k] for k, s in enumerate(shards)}, 'true', 60, time.monotonic() + 600)
            took = time.monotonic() - start
        msg = str(e.exception)
        self.true(msg.startswith('quick: mewt test exited 3 in shard s0') and 'boom-line' in msg and took < 10, msg=msg, took=took)
        self.true([r.get('sigint', False) for r in sorted(self.records(), key=lambda r: r['cwd'])] == [False, True], recs=self.records())

    def test_outcomes_latest(self):
        executable(self.bin / 'mewt', f'#!{sys.executable}\nimport sys\nassert sys.argv[1:] == "results --all --format json --log.color off".split()\n'
                                      'sys.stdout.write(open("results.json").read())\n')
        mut = lambda i: dict(id=i, mutation_slug='COS', line_offset=i, old_text='>', new_text=f'v{i}')
        res = lambda i, status, when: dict(mutant=mut(i), outcome=dict(status=status, time=when))
        day = '2026-11-03T10:00:'
        per_shard = [[res(1, 'TestFail', day + '00.27Z'), res(2, 'Uncaught', day + '01.5Z'), res(3, 'Uncaught', day + '02Z'),
                      dict(mutant=mut(4)), res(5, 'Timeout', day + '09.123456789Z')],
                     [res(1, 'Uncaught', day + '00.2700001Z'), res(2, 'TestFail', day + '01.49999Z'), res(3, 'TestFail', day + '01.999999999Z'),
                      res(6, 'Skipped', '2026-11-04T00:00:00Z')]]
        shards = []
        for k, results in enumerate(per_shard):
            shards.append(self.tmp / f's{k}')
            put(shards[-1], {'results.json': json.dumps(dict(results=results))})
        want = {i: (status, m) for i, (status, m, _) in o_latest(per_shard).items()}
        self.eq({i: v[0] for i, v in want.items()}, {1: 'Uncaught', 2: 'Uncaught', 3: 'Uncaught', 5: 'Timeout', 6: 'Skipped'}, what='oracle sanity')
        with self.path_only():
            self.eq(M.outcomes(shards), want)
            for bad in (day + '00.27+00:00', day + '00.2.7Z', day + '00.27', 'yesterday', '2026-11-03 10:00:00Z'):
                put(shards[1], {'results.json': json.dumps(dict(results=[res(1, 'TestFail', bad)]))})
                with self.assertRaises(M.Failure) as e:
                    M.outcomes(shards)
                self.true('is not UTC ISO 8601' in str(e.exception), time=bad, error=str(e.exception))

    def test_interrupt_kill(self):
        p = subprocess.Popen([sys.executable, '-c', 'import signal, time; signal.signal(signal.SIGINT, signal.SIG_IGN); print(1, flush=True); time.sleep(60)'],
                             stdout=subprocess.PIPE, start_new_session=True)
        p.stdout.readline()
        clock = iter(range(0, 10 ** 6, 1000))
        start = time.monotonic()
        with patch.object(M, 'time', type('T', (), {'monotonic': staticmethod(lambda: next(clock))})):
            M.interrupt([p])
        p.stdout.close()
        self.true(p.returncode == -signal.SIGKILL and time.monotonic() - start < 10, returncode=p.returncode)


class Unchanged(Base):
    def test_unchanged_cases(self):
        h = '02-Data Structures/01-toy.hpp'
        put(self.tmp / 'root', {h: TOY})
        shards = [self.tmp / f's{k}' for k in range(3)]
        for s in shards:
            put(s, {h: TOY})
        with patch.object(M, 'ROOT', self.tmp / 'root'):
            self.eq(M.unchanged(h, shards), None)
            put(shards[1], {h: TOY.replace('>', '>=')})
            with self.assertRaises(M.Failure) as e:
                M.unchanged(h, shards)
        self.true('shard(s) s1;' in str(e.exception), error=str(e.exception))


class Run(Base):
    H = '02-Data Structures/01-toy.hpp'
    T = '96-Local Testing/02-Data Structures/01-toy_tester.py'

    def repo(self, tester=True):
        root = init_repo(self.tmp / 'repo')
        put(root, {self.H: TOY, '02-Data Structures/Docs/01-toy.md': '# toy\n', 'README.md': 'r', '02-Data Structures/98-basic.hpp': 'a',
                   **({self.T: 'print(1)\n'} if tester else {})})
        commit(root)
        return root

    def run_main(self, root, *rest):
        with self.path_only():
            return self.main('run', *rest, root=root)

    def test_run_not_a_header(self):
        root = self.repo()
        for arg in (root / 'README.md', root / '02-Data Structures/98-basic.hpp', root / '02-Data Structures/07-missing.hpp', self.tmp / 'x.hpp'):
            code, out, err = self.run_main(root, str(arg))
            self.true(code == 1 and err.startswith(f'FAIL run: {arg}: not a library header'), arg=arg, err=err)

    def test_run_missing_tester(self):
        root = self.repo(tester=False)
        self.fake_version('mewt 4.0.0')
        code, out, err = self.run_main(root, str(root / self.H))
        self.true(code == 1 and err.startswith(f'FAIL run: {self.T} does not exist'), err=err)

    def test_run_skip(self):
        root = self.repo()
        for line, found in ((None, 'mewt not found'), ('mewt 3.9.9', 'mewt 3.9.9 found'), ('mewt (dev build)', 'mewt of unknown version found')):
            if line is not None:
                self.fake_version(line)
            work = self.tmp / 'work'
            code, out, err = self.run_main(root, str(root / self.H), '--workdir', str(work))
            self.eq((code, out.splitlines()[0]), (0, f"SKIP mutation {self.H}: {found}, needs {M.MEWT_VERSION}; "
                                                      "python3 '96-Local Testing/04-mutation.py' install"), err=err)
            self.true(not work.exists(), line=line)

    def test_run_record_current(self):
        root = self.repo()
        cur = o_blob(TOY.encode())
        put(root, {M.evidence(self.H): record_doc(cur)})
        self.fake_version('mewt 4.0.0')
        work = self.tmp / 'work'
        code, out, err = self.run_main(root, str(root / self.H), '--workdir', str(work))
        self.eq((code, out), (0, f'PASS {self.H}: the record already names blob {cur[:12]}; --whole reruns the whole file\n'), err=err)
        self.true(not work.exists())
        edited = TOY + '// uncommitted\n'  # a current record of an unstaged edit: its blob is in no object store
        put(root, {self.H: edited, M.evidence(self.H): record_doc(o_blob(edited.encode()))})
        code, out, err = self.run_main(root, str(root / self.H), '--workdir', str(work))
        self.eq((code, out), (0, f'PASS {self.H}: the record already names blob {o_blob(edited.encode())[:12]}; --whole reruns the whole file\n'),
                err=err, case='uncommitted current record')
        self.true(not work.exists())

    def test_run_workdir_refusals(self):
        root = self.repo()
        self.fake_version('mewt 4.0.0')
        cur, spec = o_blob(TOY.encode()), dict(severity=None, mutations=None, base=None, seed=1, sample=500)  # the defaults' spec
        for k, meta in enumerate([dict(header='02-Data Structures/02-other.hpp', blob=cur, spec=spec),
                                  dict(header=self.H, blob='c' * 40, spec=spec),
                                  dict(header=self.H, blob=cur, spec=dict(spec, severity='low')),
                                  dict(header=self.H, blob=cur, spec=dict(spec, seed=2)),
                                  dict(header=self.H, blob=cur, spec=dict(spec, sample=0)), None]):
            work = self.tmp / f'work{k}'
            put(work, {'campaign.json': json.dumps(meta)} if meta else {'stray.txt': 'x'})
            code, out, err = self.run_main(root, str(root / self.H), '--workdir', str(work))
            words = 'start a new --workdir' if meta else 'is not empty and holds no campaign.json'
            left = ['.lock', 'campaign.json'] if meta else ['stray.txt']  # the stray refusal comes before the lock
            self.true(code == 1 and err.startswith('FAIL run: ') and words in err and sorted(p.name for p in work.iterdir()) == left,
                      meta=meta, err=err, left=sorted(p.name for p in work.iterdir()))
        for work in (root / 'scratch/w', root, root / '02-Data Structures', self.tmp / 'repo/../repo/x'):  # every shard would copy it
            code, out, err = self.run_main(root, str(root / self.H), '--workdir', str(work))
            self.eq((code, err.strip()), (1, f'FAIL run: {work.resolve()} lies in the checkout, which every shard copies: choose a workdir '
                                             'outside it'), work=work)
            self.true(not (root / 'scratch').exists() and not (root / '.lock').exists() and git(root, 'status', '--porcelain') == '', work=work)

    def test_run_lock(self):
        root = self.repo()
        executable(self.bin / 'mewt', f'#!{sys.executable}\nimport sys\na = sys.argv[1:]\nif a == ["--version"]: print("mewt 4.0.0")\n'
                                      'elif a[:2] == ["results", "--all"]: print(\'{"results": []}\')\n')
        work = self.tmp / 'work'
        put(work, {'.lock': ''})
        with open(work / '.lock') as held:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
            code, out, err = self.run_main(root, self.H, '--workdir', str(work))
            self.true(code == 1 and err.strip() == f'FAIL run: another run uses {work}' and sorted(os.listdir(work)) == ['.lock'], err=err)
        cwd = os.getcwd()
        self.addCleanup(os.chdir, cwd)
        os.chdir(self.tmp)  # a ROOT-relative header from another directory; a workdir holding only .lock counts as empty
        code, out, err = self.run_main(root, self.H, '--workdir', str(work))
        self.true(code == 0 and 'COMPLETE: 0 of 0 sampled mutants tested' in out and f'Record: mewt {M.MEWT_VERSION} on blob `{o_blob(TOY.encode())}`' in out and (work / 'campaign.json').is_file(), out=out, err=err)

    def test_run_argparse_errors(self):
        for rest in (['--severity', 'bogus'], ['--severity', 'high,'], ['--severity', 'HIGH'], ['--budget', '0'], ['--budget', '-1'],
                     ['--jobs', '0'], ['--jobs', '-2'], ['--budget', 'x'], ['--sample', '-1'], ['--sample', '2.5'], ['--memory-mb', '-1']):
            code, out, err = self.main('run', 'README.md', *rest)
            self.true(code == 2 and 'error:' in err, argv=rest, err=err)
        code, out, err = self.main('run', 'README.md', '--severity', 'high,low', '--budget', '0.01', '--jobs', '1', '--sample', '0')
        self.true(code == 1 and 'not a library header' in err, err=err)


class Fake(Base):
    """Whole campaigns against FAKE_CAMPAIGN (hand-set mutants and outcomes), optionally with a patched baseline measurement."""
    H, T = Run.H, Run.T
    KINDS = (('ER', 'high'), ('COS', 'low'), ('CR', 'medium'))

    def setUp(self):
        super().setUp()
        self.cfg, self.log = self.bin / 'fake-campaign.json', self.bin / 'mewt.log'
        executable(self.bin / 'mewt', f'#!{sys.executable}\n' + FAKE_CAMPAIGN.replace('@CFG@', repr(str(self.cfg))).replace('@LOG@', repr(str(self.log))))
        self.configure([])
        self.path = f'{self.bin}:{ISOLATED.get("PATH", os.defpath)}'

    def configure(self, mutants, outcomes=None, sleep=None):
        self.cfg.write_text(json.dumps(dict(mutants=mutants, outcomes={str(k): v for k, v in (outcomes or {}).items()}, sleep=sleep or {})))

    def mutants(self, n, lines=(2, 3, 4, 5)):
        """[id, slug, line, severity] for ids 1..n: severities cycle high, low, medium; lines cycle."""
        return [[i, *self.KINDS[i % 3][:1], lines[i % len(lines)], self.KINDS[i % 3][1]] for i in range(1, n + 1)]

    def repo(self, name='repo', tester='print(1)\n'):
        root = init_repo(self.tmp / name)
        put(root, {self.H: TOY, M.evidence(self.H): '# toy\n', self.T: tester})
        commit(root)
        return root

    def go(self, root, *argv, seconds=None, peak=50):
        """main('run', ...) with the fake first on PATH and stage()'s poll shortened to 0.05 s (deadlines unchanged); seconds
        (with peak) replaces the baseline measurement, whose commands go to self.baselines. The mewt call log restarts."""
        self.baselines = []
        self.log.unlink(missing_ok=True)

        def measured(cmd, cwd):
            self.baselines.append(cmd)
            return seconds, 0, '', peak
        fast = type('FastTime', (), {'monotonic': staticmethod(time.monotonic), 'sleep': staticmethod(lambda s: time.sleep(min(s, 0.05)))})
        with patch.dict(os.environ, {'PATH': self.path}), patch.object(M, 'time', fast), (
                patch.object(M, 'measured', measured) if seconds is not None else contextlib.nullcontext()):
            return self.main('run', str(root / self.H), '--jobs', '2', '--memory-mb', '4096', *argv, root=root)

    def calls(self, name):
        """[(shard name, ids, timeout, argv of --test.cmd)] of the fake's `test` calls of the named stage."""
        out = []
        for c in map(json.loads, self.log.read_text().splitlines() if self.log.exists() else []):
            o = dict(zip(c['argv'][1::2], c['argv'][2::2]))
            if c['argv'][0] == 'test' and o['--ids-file'] == f'{name}.ids':
                out.append((Path(c['cwd']).name, [int(x) for x in (Path(c['cwd']) / o['--ids-file']).read_text().split()],
                            int(o['--test.timeout']), shlex.split(o['--test.cmd'])))
        return out

    def stage_line(self, out, name):
        """(mutants, width, shards, timeout, cap) of the stage's printed line, or None."""
        m = re.search(rf'^{re.escape(name)}: (\d+) mutants on (\d+) of (\d+) shards; baseline [\d.]+ s, peak \d+ MB; per mutant timeout (\d+) s, '
                      r'cap (\d+) MB; about \d+ min, budget left -?\d+ min$', out, re.M)
        return m and tuple(map(int, m.groups()))

    def record_line(self, out):
        m = re.search(r'^Record: .*$', out, re.M)
        return m and m[0]

    def meta(self, work):
        return json.loads((work / 'campaign.json').read_text())

    def test_fake_stage_parameters(self):
        # per-mutant timeout max(60, ceil(3 x baseline)) or --timeout, cap min(own, max(1024, 3 x peak)), width
        # max(1, min(jobs, budget // cap)): hand-computed for each case (None: the real baseline of `print(1)`)
        cases = [(3, 4096, 4096, None, None, [], (60, 1024, 3)),
                 (3, 4200, 4096, 30.2, 700, [], (91, 2100, 2)),
                 (3, 5000, 6144, 20.01, 3000, [], (61, 6144, 1)),
                 (3, 3072, 1536, 100.0, 341, ['--timeout', '7'], (7, 1024, 3)),
                 (2, 1000, 800, 19.9, 10, [], (60, 800, 1))]
        self.configure(self.mutants(6))
        for k, (jobs, memory, own, seconds, peak, extra, (timeout, cap, width)) in enumerate(cases):
            root = self.repo(f'repo{k}', f'# _00_memory_cap.ensure(mb={own})\nprint(1)\n')
            work = self.tmp / f'w{k}'
            code, out, err = self.go(root, '--workdir', str(work), '--jobs', str(jobs), '--memory-mb', str(memory), *extra, seconds=seconds, peak=peak)
            self.eq(code, 0, out=out, err=err, case=k)
            self.true(f'(cap {own} MB) workdir {work}; memory budget {memory} MB\n' in out, out=out, case=k)
            self.eq(self.stage_line(out, 'quick'), (6, width, jobs, timeout, cap), out=out, case=k)
            runs = self.calls('quick')
            self.eq((len(runs), sorted(i for _, ids, _, _ in runs for i in ids)), (width, list(range(1, 7))), runs=runs, case=k)
            for _, _, t, cmd in runs:
                self.eq((t, cmd[cmd.index('--mb') + 1], cmd[-4:]), (timeout, str(cap), ['--mode', 'quick', '--seed', '1']), cmd=cmd, case=k)
            if seconds is not None:
                self.true(all(c[c.index('--mb') + 1] == str(own) and c[-4:] in (['--mode', 'quick', '--seed', '1'], ['--mode', 'full', '--seed', '1'])
                              for c in self.baselines) and self.baselines, baselines=self.baselines, case=k, what='baselines under the own cap')

    def test_fake_sample(self):
        root, every = self.repo(), list(range(1, 21))
        self.configure(self.mutants(20))
        cur = o_blob(TOY.encode())
        work = self.tmp / 'w-sampled'
        code, out, err = self.go(root, '--workdir', str(work), '--sample', '5', seconds=0.1)
        meta = self.meta(work)
        sel = meta['selected']
        self.true(code == 0 and (meta['candidates'], len(sel), meta['spec']['sample']) == (20, 5, 5) and sel == sorted(set(sel)) and set(sel) <= set(every),
                  meta=meta, out=out, err=err)
        self.eq(sorted(i for _, ids, _, _ in self.calls('quick') for i in ids), sel, what='only the sample is tested')
        self.true(re.fullmatch(rf'Record: mewt 4\.0\.0 on blob `{cur}` \(\d{{4}}-\d\d-\d\d, seed 1, whole file, uniform sample of 20\): 5 mutants, '
                               r'5 caught \(0 by timeout\), 0 survived, 0 untested\.', self.record_line(out) or ''), out=out)
        code, out, err = self.go(root, '--workdir', str(self.tmp / 'w-all'), '--sample', '0', seconds=0.1)
        meta = self.meta(self.tmp / 'w-all')
        self.true(code == 0 and meta['selected'] == every and meta['candidates'] == 20 and 'whole file): 20 mutants, 20 caught' in out
                  and 'uniform sample' not in out, meta=meta, out=out)
        for k, n in enumerate(('20', '25')):  # at least as many as the candidates: all, no sample
            code, out, err = self.go(root, '--workdir', str(self.tmp / f'w-n{k}'), '--sample', n, '--budget', '1e-9')
            self.true(code == 3 and self.meta(self.tmp / f'w-n{k}')['selected'] == every, n=n, out=out)
        code, out, err = self.go(root, '--workdir', str(self.tmp / 'w-sev'), '--severity', 'high,medium', '--sample', '3', '--budget', '1e-9')
        meta, highmed = self.meta(self.tmp / 'w-sev'), [m[0] for m in self.mutants(20) if m[3] != 'low']
        self.true(code == 3 and meta['candidates'] == len(highmed) == 13 and len(meta['selected']) == 3 and set(meta['selected']) <= set(highmed),
                  meta=meta, out=out)
        self.true(out.endswith('\nNo record: 3 sampled mutants are untested and stage quick did not finish; rerun with the same --workdir to finish '
                               f'the campaign.\nWorkdir {self.tmp / "w-sev"} (rerun with --workdir to continue or to retest survivors; delete it '
                               'once recorded)\n'), out=out)
        picks = {}  # seeded: a seed repeats its sample; over seeds the sample spreads over the candidates (prefix or suffix bias fails)
        for seed in range(1, 1 + 6 * self.rounds()):
            w = self.tmp / f'w-seed{seed}'
            self.eq(self.go(root, '--workdir', str(w), '--sample', '5', '--seed', str(seed), '--budget', '1e-9')[0], 3, seed=seed)
            picks[seed] = self.meta(w)['selected']
        flat = [i for p in picks.values() for i in p]
        self.true(picks[1] == sel and len(set(flat)) > 10 and abs(sum(flat) / len(flat) - 10.5) < 5, picks=picks)

    def test_fake_timeouts_alone(self):
        # a: times out among concurrent tests, passes alone -> survivor of quick, escalates, survives; b: times out alone too ->
        # caught by timeout; c: alone it fails -> caught; d, e: time out in full only, d again alone, e passes alone -> survivor
        a, b, c, d, e = 1, 2, 3, 4, 5
        self.configure(self.mutants(8), {a: {'quick': 'Timeout', 'quick-alone': 'Uncaught', 'full': 'Uncaught'}, b: {'*': 'Timeout'},
                                         c: {'quick': 'Timeout', 'quick-alone': 'TestFail'}, d: {'quick': 'Uncaught', 'full': 'Timeout', 'full-alone': 'Timeout'},
                                         e: {'quick': 'Uncaught', 'full': 'Timeout', 'full-alone': 'Uncaught'}})
        root, work = self.repo(), self.tmp / 'work'
        code, out, err = self.go(root, '--workdir', str(work), '--timeout', '5', seconds=0.1)
        self.eq(code, 0, out=out, err=err)
        got = {n: [(s, sorted(ids), t) for s, ids, t, _ in self.calls(n)] for n in ('quick-alone', 'full', 'full-alone')}
        self.eq(got['quick-alone'], [('s0', [a, b, c], 5)], out=out)
        self.eq(sorted(i for _, ids, _ in got['full'] for i in ids), [a, d, e])
        self.eq(got['full-alone'], [('s0', [d, e], 5)])
        self.true(self.stage_line(out, 'quick-alone') == (3, 1, 2, 5, 1024) and self.stage_line(out, 'full-alone') == (2, 1, 2, 5, 1024), out=out)
        self.true(self.record_line(out).endswith(': 8 mutants, 6 caught (2 by timeout), 2 survived, 0 untested.') and self.meta(work)['unconfirmed'] == {}
                  and f'| `COS {a}` (low) | 3 | `old {a}` → `new {a}` |' in out and f'| `CR {e}` (medium) | 3 |' in out, out=out)

    def test_fake_unconfirmed_resume(self):
        # unconfirmed maps each timeout to the stage it timed out in; a budget stop of that stage's alone pass keeps it in
        # campaign.json, and the rerun retests it alone in that stage only (a stage with nothing pending still runs its
        # baseline and alone pass); an entry whose latest outcome is no longer Timeout is dropped at its alone pass
        root = self.repo()
        for k, (stage, a, plan) in enumerate((('quick', 3, {'quick': 'Timeout', 'quick-alone': 'Timeout'}),
                                              ('full', 2, {'quick': 'Uncaught', 'full': 'Timeout', 'full-alone': 'Timeout'}))):
            work, other = self.tmp / f'w-{stage}', [s for s in ('quick', 'full') if s != stage][0]
            self.configure(self.mutants(4), {a: plan}, {f'{stage}-alone': 60})
            code, out, err = self.go(root, '--workdir', str(work), '--budget', '0.04', seconds=0.1)
            self.true(code == 3 and self.meta(work)['unconfirmed'] == {str(a): stage} and 'STOPPED by the budget: 4 of 4 sampled mutants tested' in out
                      and f'\nNo record: 0 sampled mutants are untested and stage {stage}-alone did not finish; rerun with the same --workdir to '
                      'finish the campaign.\n' in out and not self.record_line(out), out=out, err=err, stage=stage)
            self.configure(self.mutants(4), {a: plan})
            code, out, err = self.go(root, '--workdir', str(work), seconds=0.1)
            self.true(code == 0 and self.stage_line(out, stage) is None and self.stage_line(out, f'{stage}-alone')[:2] == (1, 1)
                      and [(s, ids) for s, ids, _, _ in self.calls(f'{stage}-alone')] == [('s0', [a])] and self.meta(work)['unconfirmed'] == {}
                      and not self.calls(f'{other}-alone') and not self.calls(other) and not self.calls(stage)
                      and [c[c.index('--mode') + 1] for c in self.baselines] == [stage]
                      and self.record_line(out).endswith(': 4 mutants, 4 caught (1 by timeout), 0 survived, 0 untested.'), out=out, err=err,
                      stage=stage, baselines=self.baselines, what='resume retests the timeout alone in its own stage only')
        # a survivor makes quick run again on resume while a full timeout is unconfirmed: quick-alone must leave it to full-alone
        work, a, s = self.tmp / 'w-mixed', 2, 4
        plan = {a: {'quick': 'Uncaught', 'full': 'Timeout', 'full-alone': 'Timeout'}, s: {'*': 'Uncaught'}}
        self.configure(self.mutants(4), plan, {'full-alone': 60})
        code, out, err = self.go(root, '--workdir', str(work), '--budget', '0.04', seconds=0.1)
        self.true(code == 3 and self.meta(work)['unconfirmed'] == {str(a): 'full'}, out=out, err=err)
        self.configure(self.mutants(4), plan)
        code, out, err = self.go(root, '--workdir', str(work), seconds=0.1)
        self.true(code == 0 and [ids for _, ids, _, _ in self.calls('quick')] == [[s]] and not self.calls('quick-alone')
                  and [(sh, ids) for sh, ids, _, _ in self.calls('full-alone')] == [('s0', [a])] and self.meta(work)['unconfirmed'] == {}
                  and self.record_line(out).endswith(': 4 mutants, 3 caught (1 by timeout), 1 survived, 0 untested.'), out=out, err=err)
        work, a = self.tmp / 'w-drop', 3
        self.configure(self.mutants(4), {a: {'quick': 'Timeout', 'quick-alone': 'Timeout'}}, {'quick-alone': 60})
        self.eq(self.go(root, '--workdir', str(work), '--budget', '0.04', seconds=0.1)[0], 3)
        results = json.loads((work / 's0/fake-results.json').read_text())  # a later outcome of a in s0 (any shard may test any mutant)
        results[str(a)] = dict(status='TestFail', time=datetime.now(timezone.utc).strftime('%Y-%m-%dT%H:%M:%S.%fZ'))
        (work / 's0/fake-results.json').write_text(json.dumps(results))
        code, out, err = self.go(root, '--workdir', str(work), seconds=0.1)
        self.true(code == 0 and not self.calls('quick-alone') and not self.calls('quick') and self.meta(work)['unconfirmed'] == {}
                  and self.stage_line(out, 'quick-alone') is None and [c[c.index('--mode') + 1] for c in self.baselines] == ['quick']
                  and self.record_line(out).endswith(': 4 mutants, 4 caught (0 by timeout), 0 survived, 0 untested.'), out=out, err=err,
                  what='an entry no longer timing out is dropped without a retest')

    def test_fake_resume_recorded(self):
        # a diff-scoped campaign never samples; once its Record is pasted, the same --workdir still resumes it (keeping its base)
        # and retests the survivor, while any other workdir, or none, passes at once
        root = self.repo()
        old = o_blob(TOY.encode())
        put(root, {M.evidence(self.H): record_doc(old)})
        commit(root)
        put(root, {self.H: TOY.replace('return x;', 'return x | 0;')})
        new = o_blob((root / self.H).read_bytes())
        on5 = [m[0] for m in self.mutants(16) if m[2] == 5]
        self.configure(self.mutants(16), {on5[0]: {'*': 'Uncaught'}})
        work = self.tmp / 'work'
        code, out, err = self.go(root, '--workdir', str(work), '--sample', '2', seconds=0.1)
        meta = self.meta(work)
        self.true(code == 0 and meta['spec']['base'] == old and meta['spec']['sample'] == 0 and meta['selected'] == on5 == sorted(on5)
                  and len(on5) > 2, meta=meta, out=out, err=err)
        record = self.record_line(out)
        self.true(f'on blob `{new}`' in record and f'1 lines changed since blob `{old[:12]}`): {len(on5)} mutants' in record and 'sample' not in record
                  and record.endswith(f'{len(on5) - 1} caught (0 by timeout), 1 survived, 0 untested.'), record=record)
        put(root, {M.evidence(self.H): f'# toy\n\n## Mutation testing\n\n{record}\n'})
        self.eq(self.main('check', self.H, root=root)[:2], (0, 'PASS: 1 of 1 mutation records current\n'))
        for other in (self.tmp / 'w-other', None):
            tmpdir = self.tmp / 'tmpdir'
            tmpdir.mkdir(exist_ok=True)
            with patch.object(tempfile, 'tempdir', str(tmpdir)):
                code, out, err = self.go(root, *(['--workdir', str(other)] if other else []), '--sample', '2')
            self.eq((code, out), (0, f'PASS {self.H}: the record already names blob {new[:12]}; --whole reruns the whole file\n'), err=err, other=other)
            self.true((other is None or not other.exists()) and not list(tmpdir.iterdir()), other=other)
        self.configure(self.mutants(16))  # the strengthened tester
        code, out, err = self.go(root, '--workdir', str(work), '--sample', '2', seconds=0.1)
        record = self.record_line(out)
        self.true(code == 0 and 'PASS' not in out and f'lines changed since blob `{old[:12]}`' in record and self.meta(work)['spec']['base'] == old
                  and [ids for _, ids, _, _ in self.calls('quick')] == [[on5[0]]]
                  and record.endswith(f'{len(on5)} caught (0 by timeout), 0 survived, 0 untested.'), out=out, err=err)

    def test_fake_incomplete_record(self):
        # a current record that leaves mutants untested (or has no count) is no PASS: a whole-file campaign runs
        root = self.repo()
        cur = o_blob(TOY.encode())
        for k, untested in enumerate((5, None)):
            put(root, {M.evidence(self.H): record_doc(cur, untested=untested)})
            work = self.tmp / f'w{k}'
            code, out, err = self.go(root, '--workdir', str(work))
            self.true(code == 0 and out.startswith(f'NOTE the record of blob {cur[:12]} leaves mutants untested: whole-file campaign\nMUTATION ')
                      and 'PASS' not in out and 'COMPLETE: 0 of 0' in out and self.meta(work)['spec']['base'] is None, out=out, err=err, untested=untested)

    def test_fake_signal_handlers(self):
        # campaign() installs stop() for SIGTERM and SIGHUP, except where a caller (nohup) left the signal ignored
        root = self.repo()
        for k, ignored in enumerate((None, signal.SIGHUP, signal.SIGTERM)):
            for s in (signal.SIGTERM, signal.SIGHUP):
                signal.signal(s, signal.SIG_IGN if s == ignored else signal.SIG_DFL)
            code, out, err = self.go(root, '--workdir', str(self.tmp / f'w{k}'))
            self.eq([code] + [signal.getsignal(s) for s in (signal.SIGTERM, signal.SIGHUP)],
                    [0] + [signal.SIG_IGN if s == ignored else M.stop for s in (signal.SIGTERM, signal.SIGHUP)], ignored=ignored, err=err)

    def driver(self, root, *argv, env, out):
        d = self.tmp / 'driver.py'
        d.write_text(DRIVER)
        return subprocess.Popen([sys.executable, str(d), str(MUTATION_PY), str(root), 'run', str(root / self.H), '--jobs', '2', *argv],
                                env=env, stdout=out, stderr=subprocess.STDOUT, start_new_session=True)

    def test_fake_machine_lock(self):
        root = self.repo()
        for variant in ('CP_MUTATION_LOCK', 'XDG_RUNTIME_DIR', 'TMPDIR'):
            d = self.tmp / f'lock-{variant}'
            d.mkdir()
            env = {k: v for k, v in os.environ.items() if k not in ('CP_MUTATION_LOCK', 'XDG_RUNTIME_DIR', 'TMPDIR')} | {'PATH': self.path, variant: str(d)}
            want = d / 'cp-mutation.lock'
            if variant == 'CP_MUTATION_LOCK':
                env[variant] = str(want := d / 'custom.lock')
            log, work = self.tmp / f'{variant}.log', self.tmp / f'w-{variant}'
            with open(want, 'w') as held, open(log, 'w') as f:
                fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
                p = self.driver(root, '--workdir', str(work), env=env, out=f)
                self.addCleanup(lambda p=p: p.poll() is None and os.killpg(p.pid, signal.SIGKILL))
                wait = f'WAIT another campaign holds {want}; this one starts when it ends\n'
                self.true(self.wait_for(log, wait, p, 60), log=log.read_text(), variant=variant)
                time.sleep(0.5)
                self.true(p.poll() is None and '\nMUTATION ' not in log.read_text() and sorted(os.listdir(work)) == ['.lock'], log=log.read_text(),
                          variant=variant, what='blocked until the lock is free', code=p.poll(), work=sorted(os.listdir(work)))
            code = p.wait(60)
            text = log.read_text()
            self.true(code == 0 and text.index(wait) < text.index('\nMUTATION ') < text.index('COMPLETE: 0 of 0'), log=text, variant=variant)
        missing = self.tmp / 'no-such-dir' / 'x.lock'
        env = dict(os.environ, PATH=self.path, CP_MUTATION_LOCK=str(missing))
        log = self.tmp / 'missing.log'
        with open(log, 'w') as f:
            code = self.driver(root, '--workdir', str(self.tmp / 'w-missing'), env=env, out=f).wait(60)
        text = log.read_text()
        self.true(code == 1 and f'FAIL run: cannot open the campaign lock {missing}' in text and 'Traceback' not in text, log=text, code=code)

    def test_fake_header_changed_during_wait(self):
        # the blob read before the wait must still be the header's once the lock is taken, else the run stops
        root = self.repo()
        self.configure(self.mutants(3))
        log, work, lock = self.tmp / 'run.log', self.tmp / 'work', Path(os.environ['CP_MUTATION_LOCK'])
        env = dict(os.environ, PATH=self.path)
        with open(lock, 'w') as held, open(log, 'w') as f:
            fcntl.flock(held, fcntl.LOCK_EX | fcntl.LOCK_NB)
            p = self.driver(root, '--workdir', str(work), env=env, out=f)
            self.addCleanup(lambda: p.poll() is None and os.killpg(p.pid, signal.SIGKILL))
            self.true(self.wait_for(log, f'WAIT another campaign holds {lock}', p, 60), log=log.read_text())
            put(root, {self.H: TOY + '// edited while waiting\n'})
        code, text = p.wait(60), log.read_text()
        self.true(code == 1 and f'FAIL run: {self.H} changed while this run waited for the campaign lock: run it again\n' in text
                  and '\nMUTATION ' not in text and 'Record:' not in text and not (work / 'campaign.json').exists()
                  and [c['argv'] for c in map(json.loads, self.log.read_text().splitlines())] == [['--version']],
                  log=text, code=code, what='stopped before any shard or mewt call')

    def test_fake_baseline_signal(self):
        # SIGTERM or SIGHUP while a stage baseline runs (inside a cap, or to the top-level supervisor): measured() takes the
        # probe's whole group down (capped tester, its _00_memory_cap supervisor and reporter): exit 130, nothing left
        mark = self.tmp / 'baseline.started'
        tester = (f'import pathlib, sys, time\nif sys.argv[1:3] == ["--mode", "quick"]:\n'
                  f'    pathlib.Path({str(mark)!r}).write_text("started")\n    time.sleep(120)\n')
        root = self.repo(tester=tester)
        self.configure(self.mutants(3))
        for name, sig, supervised in (('sigterm', signal.SIGTERM, False), ('sighup', signal.SIGHUP, False), ('supervisor', signal.SIGTERM, True)):
            mark.unlink(missing_ok=True)
            work, log = self.tmp / f'w-{name}', self.tmp / f'{name}.log'
            env = {k: v for k, v in os.environ.items() if not (supervised and k == 'CP_MEMORY_CAPPED')} | {'PATH': self.path}
            with open(log, 'w') as f:
                p = self.driver(root, '--workdir', str(work), env=env, out=f)
            self.addCleanup(lambda p=p: p.poll() is None and os.killpg(p.pid, signal.SIGKILL))
            until = time.monotonic() + 60
            while not mark.exists() and p.poll() is None and time.monotonic() < until:
                time.sleep(0.1)
            self.true(mark.exists() and self.procs(work), log=log.read_text(), variant=name, what='the baseline runs in s0')
            start = time.monotonic()
            p.send_signal(sig)
            code = p.wait(60)
            took, text = time.monotonic() - start, log.read_text()
            self.true(code == 130 and text.count('INTERRUPTED run') == 1 and took < 30 and 'quick:' not in text and not self.calls('quick'),
                      code=code, took=took, log=text, variant=name)
            self.eq(self.left(work), [], what='processes left in the workdir', variant=name)
            self.eq((work / 's0' / self.H).read_bytes(), (root / self.H).read_bytes(), variant=name, what='header unchanged')

    def test_fake_no_preflight(self):
        # under the real supervisor with CP_TEST_MODE=full a campaign runs once; the negative control (the campaign's
        # CP_TEST_PREFLIGHT=0 undone) shows what this oracle sees otherwise: a pre-flight line and a second workdir
        root = self.repo()
        for control in (False, True):
            tmpdir = self.tmp / f'tmp-{control}'
            tmpdir.mkdir()
            env = {k: v for k, v in os.environ.items() if k not in ('CP_MEMORY_CAPPED', 'CP_TEST_PREFLIGHT')} | {
                'PATH': self.path, 'CP_TEST_MODE': 'full', 'TMPDIR': str(tmpdir), **({'DRIVER_KEEP_PREFLIGHT': '1'} if control else {})}
            log = self.tmp / f'{control}.log'
            with open(log, 'w') as f:
                p = self.driver(root, env=env, out=f)
            code, text = p.wait(300), log.read_text()
            works = sorted(tmpdir.glob('cp-mutation-01-toy-*'))
            self.true(code == 0 and len(re.findall(r'^MUTATION ', text, re.M)) == 1 and text.count('COMPLETE: 0 of 0') == 1
                      and len(works) == 1 + control and ('PREFLIGHT driver.py: quick passed' in text) == control
                      and (not _00_memory_cap.available() or 'MEMORY peak=' in text), log=text, works=works, control=control)


class E2E(Base):
    def setUp(self):
        super().setUp()
        version = M.mewt_version()
        if version != M.MEWT_VERSION:
            self.skipTest(f'SKIP: mewt {version} found, the end-to-end campaigns need mewt {M.MEWT_VERSION}')
        self.cxx = shutil.which('g++')
        if not self.cxx:
            self.skipTest('SKIP: g++ is not installed')
        cwd, here = os.getcwd(), self.tmp / 'cwd'
        here.mkdir()
        os.chdir(here)  # a campaign runs mewt only inside its shards: the caller's directory stays empty
        self.addCleanup(lambda: self.eq(sorted(os.listdir(here)), [], what='files left in the working directory'))
        self.addCleanup(os.chdir, cwd)

    def repo(self, files):
        root = init_repo(self.tmp / 'repo')
        put(root, files)
        commit(root, 'init')
        return root

    def golden(self, header, text=TOY, sleep=0):
        folder, stem = header.split('/')[0], Path(header).stem
        return {header: text, f'96-Local Testing/{folder}/{stem}_tester.py': GOLDEN_TESTER.replace('@HEADER@', header).replace('@SLEEP@', str(sleep)),
                f'96-Local Testing/{folder}/{stem}_tester.golden': text}

    def campaign(self, root, header, work, *extra):
        return self.main('run', str(root / header), '--workdir', str(work), '--jobs', '2', '--seed', '1', '--memory-mb', '4096', *extra, root=root)

    def mewt(self, shard, *args):
        r = subprocess.run(['mewt', *args, '--log.color', 'off'], cwd=shard, capture_output=True, text=True)
        self.eq(r.returncode, 0, mewt=args, out=r.stdout + r.stderr)
        return r.stdout

    def listing(self, shard, *flags):
        text = self.mewt(shard, 'print', 'mutants', *flags)
        return {int(m[2]): (m[1], int(m[3])) for m in re.finditer(r'\[(\w+) (\d+)\] Lines? (\d+)', text)}

    def ids(self, shard, *flags):
        return sorted(int(x) for x in self.mewt(shard, 'print', 'mutants', *flags, '--format', 'ids').split())

    def statuses(self, work, jobs=2):
        """{id: (status, mutant, outcome)}: the latest outcome of each mutant over every shard (any shard may test any)."""
        return o_latest([json.loads(self.mewt(work / f's{k}', 'results', '--all', '--format', 'json'))['results'] for k in range(jobs)])

    def stage_ids(self, shard, name):
        p = shard / f'{name}.ids'
        return [int(x) for x in p.read_text().split()] if p.is_file() else []

    def common(self, root, header, work, out, jobs=2, complete=True):
        """A complete run prints the Record line with the checkout's git blob and stores that blob in the object store; a
        stopped one prints No record with its untested count and the stage (or alone pass) that did not finish. Every
        shard's header equals the checkout."""
        meta, report = json.loads((work / 'campaign.json').read_text()), json.loads((work / 'report.json').read_text())
        m = re.search(r'^Record: mewt 4\.0\.0 on blob `([0-9a-f]{40})` \(', out, re.M)
        if complete:
            self.true(m and m[1] == git(root, 'hash-object', header).strip() and report['complete'], out=out)
            git(root, 'cat-file', '-e', m[1])
        else:
            stopped = re.search(rf'\nNo record: {report["untested"]} sampled mutants are untested and stage ([\w-]+) did not finish; rerun with '
                                'the same --workdir to finish the campaign.\n', out)
            self.true(not m and not report['complete'] and stopped and stopped[1].removesuffix('-alone') in STAGES3 + ['full'], out=out)
        for k in range(jobs):
            self.eq((work / f's{k}' / header).read_bytes(), (root / header).read_bytes(), shard=k)
        return meta, report

    def test_e2e_campaign_and_resume(self):
        header, tpath = '02-Data Structures/01-toy.hpp', '96-Local Testing/02-Data Structures/01-toy_tester.py'
        tester = TOY_TESTER.replace('@HEADER@', header).replace('@CXX@', self.cxx)
        root = self.repo({header: TOY, '02-Data Structures/Docs/01-toy.md': '# toy\n', tpath: tester.replace('@EXTRA@', '')})
        work = self.tmp / 'work'
        code, out, err = self.campaign(root, header, work)
        self.eq(code, 0, out=out, err=err)
        meta, report = self.common(root, header, work, out)
        every = self.ids(work / 's0')
        seen = self.statuses(work)
        self.eq((meta['selected'], sorted(seen), meta['spec'], meta['jobs'], sorted(meta['stages'])),
                (every, every, dict(severity=None, mutations=None, base=None, seed=1, sample=500), 2, sorted(STAGES3)))
        survivors = []
        for i, (status, m, _) in seen.items():
            want = o_stage(m['line_offset'] + 1, m['mutation_slug'], m['new_text'])
            ran = [n for n in STAGES3 if any(i in self.stage_ids(work / f's{k}', n) for k in range(2))]
            self.eq((status, ran), ('Uncaught' if want is None else 'TestFail', STAGES3[:STAGES3.index(want) + 1] if want else STAGES3), mutant=m)
            survivors += [i] if want is None else []
        self.eq((len(survivors), report['survivors'], report['counts'], report['untested']),
                (1, survivors, {'TestFail': len(every) - 1, 'Uncaught': 1}, 0), report=report)
        self.true(f'COMPLETE: {len(every)} of {len(every)} sampled mutants tested' in out and
                  f'| `COS {survivors[0]}` (low) | 4 | `>` → `>=` |' in out, out=out)
        for other in (['--severity', 'high'], ['--seed', '2'], ['--sample', '3']):
            code, out2, err2 = self.campaign(root, header, work, *other)
            self.true(code == 1 and 'start a new --workdir' in err2, err=err2, what='resume with another sample', argv=other)
        # the reviewer's scenario: the Record (1 survivor, 0 untested) goes into the evidence document, so the record is current
        record = re.search(r'^Record: .*$', out, re.M)[0]
        self.true(record.endswith(f': {len(every)} mutants, {len(every) - 1} caught (0 by timeout), 1 survived, 0 untested.'), record=record)
        put(root, {'02-Data Structures/Docs/01-toy.md': f'# toy\n\n## Mutation testing\n\n{record}\n'})
        self.eq(self.main('check', header, root=root)[:2], (0, 'PASS: 1 of 1 mutation records current\n'))
        tmpdir = self.tmp / 'tmpdir'
        tmpdir.mkdir()
        for other in (self.tmp / 'w-other', None):  # any other workdir, or none, has nothing to do
            with patch.object(tempfile, 'tempdir', str(tmpdir)):
                code, out2, err2 = self.main('run', header, *(['--workdir', str(other)] if other else []), root=root)
            self.eq((code, out2), (0, f'PASS {header}: the record already names blob {meta["blob"][:12]}; --whole reruns the whole file\n'),
                    err=err2, other=other)
            self.true((other is None or not other.exists()) and not list(tmpdir.iterdir()), other=other)
        put(root, {tpath: tester.replace('@EXTRA@', "checks.append('if (x > hi)' in header.read_text())")})
        code, out3, err3 = self.campaign(root, header, work)
        self.eq(code, 0, out=out3, err=err3, what='resume after recording and strengthening the tester')
        _, report = self.common(root, header, work, out3)
        self.eq((report['survivors'], report['counts']), ([], {'TestFail': len(every)}), report=report)
        record = re.search(r'^Record: .*$', out3, re.M)[0]
        self.true('PASS' not in out3 and record.endswith(f': {len(every)} mutants, {len(every)} caught (0 by timeout), 0 survived, 0 untested.'),
                  out=out3)
        put(root, {'02-Data Structures/Docs/01-toy.md': f'# toy\n\n## Mutation testing\n\n{record}\n'})
        self.eq(self.main('check', header, root=root)[:2], (0, 'PASS: 1 of 1 mutation records current\n'))
        self.eq(self.stage_ids(work / 's0', 'quick'), survivors, what='survivor retested from the first stage')
        self.eq({s for s, _, _ in self.statuses(work).values()}, {'TestFail'})
        for k in range(2):
            self.eq((work / f's{k}' / tpath).read_text(), (root / tpath).read_text(), what='tester re-synced', shard=k)

    def test_e2e_diff_scope(self):
        header, doc = '02-Data Structures/01-toy.hpp', '02-Data Structures/Docs/01-toy.md'
        root = self.repo({**self.golden(header), doc: '# toy\n'})
        old = git(root, 'hash-object', header).strip()
        put(root, {doc: record_doc(old, after='## Other\n')})
        commit(root, 'record')
        self.eq(self.main('check', header, root=root)[:2], (0, 'PASS: 1 of 1 mutation records current\n'))
        work = self.tmp / 'w-current'
        code, out, err = self.campaign(root, header, work)
        self.true(code == 0 and f'PASS {header}: the record already names blob {old[:12]}' in out and not work.exists(), out=out, err=err)
        for k, (before, after, line) in enumerate((('return a + b;', 'return b + a;', 2), ('return x;', 'return x | 0;', 5))):
            # k = 0: diff against the committed record; k = 1: against the uncommitted blob the complete k = 0 run stored
            base = git(root, 'hash-object', header).strip()
            if k:
                put(root, {doc: record_doc(base, after='## Other\n')})
            put(root, self.golden(header, (root / header).read_text().replace(before, after)))
            new = git(root, 'hash-object', header).strip()
            with self.assertRaises(RuntimeError, msg=self.info(what='blob not yet in the object store', step=k)):
                git(root, 'cat-file', '-e', new)
            self.eq(self.main('check', header, root=root)[0], 1)
            work = self.tmp / f'w-diff{k}'
            code, out, err = self.campaign(root, header, work)
            self.eq(code, 0, out=out, err=err, step=k)
            meta, report = self.common(root, header, work, out)
            listing = self.listing(work / 's0')
            want = sorted(i for i, (_, at) in listing.items() if at == line)
            self.true(meta['spec']['base'] == base and meta['lines'] == 1 and meta['selected'] == want and 0 < len(want) < len(listing),
                      meta=meta, listing=listing, step=k)
            self.eq((sorted(meta['stages']), report['counts']), (['full', 'quick'], {'TestFail': len(want)}), report=report, step=k)
            ran = {n: sorted(i for s in range(2) for i in self.stage_ids(work / f's{s}', n)) for n in ('quick', 'full')}
            self.eq(ran, {'quick': want, 'full': want}, step=k, what='every mutant escalates once')
        work = self.tmp / 'w-whole'
        code, out, err = self.campaign(root, header, work, '--whole')
        meta, _ = self.common(root, header, work, out)
        self.true(code == 0 and meta['spec']['base'] is None and meta['selected'] == sorted(self.listing(work / 's0')), meta=meta, err=err)

    def test_e2e_core_severity(self):
        full, mini = '01-Core/05-toy.hpp', '01-Core/06-toymini.hpp'
        root = self.repo({**self.golden(full), **self.golden(mini)})
        for header, severity in ((full, 'high,medium'), (mini, None)):
            work = self.tmp / f'w-{Path(header).stem}'
            code, out, err = self.campaign(root, header, work)
            self.eq(code, 0, out=out, err=err, header=header)
            meta, report = self.common(root, header, work, out)
            every = self.ids(work / 's0')
            want = self.ids(work / 's0', '--severity', severity) if severity else every
            self.true(meta['spec']['severity'] == severity and meta['selected'] == want and report['counts'] == {'TestFail': len(want)}
                      and (severity is None or len(want) < len(every)), header=header, meta=meta, every=every)

    def test_e2e_sample(self):
        # a Core full whole-file campaign with --sample 3 tests 3 of its high and medium mutants and says so in the Record
        header = '01-Core/05-toy.hpp'
        root = self.repo(self.golden(header))
        work = self.tmp / 'work'
        code, out, err = self.campaign(root, header, work, '--sample', '3')
        self.eq(code, 0, out=out, err=err)
        meta, report = self.common(root, header, work, out)
        want = self.ids(work / 's0', '--severity', 'high,medium')
        tested = sorted(i for k in range(2) for i in self.stage_ids(work / f's{k}', 'quick'))
        self.true(meta['candidates'] == len(want) > 3 and len(meta['selected']) == 3 and set(meta['selected']) <= set(want) and meta['spec']['sample'] == 3
                  and tested == meta['selected'] and report['counts'] == {'TestFail': 3}, meta=meta, want=want, tested=tested)
        self.true(f', seed 1, whole file, severity high,medium, uniform sample of {len(want)}): 3 mutants, 3 caught (0 by timeout), 0 survived, '
                  '0 untested.\n' in out, out=out)

    def test_e2e_runaway_alone(self):
        # each test runs in its own scope capped at max(1024, 3 x baseline peak): the runaway mutant is caught alone,
        # the campaign scope sees no kill, and the concurrent shard's equivalent survivor (1LL + n >> 3) still survives
        header, tpath = '02-Data Structures/01-run.hpp', '96-Local Testing/02-Data Structures/01-run_tester.py'
        root = self.repo({header: RUNAWAY, tpath: RUNAWAY_TESTER.replace('@HEADER@', header).replace('@CXX@', self.cxx)})
        work = self.tmp / 'work'
        code, out, err = self.campaign(root, header, work, '--memory-mb', '2048')
        self.eq(code, 0, out=out, err=err)
        self.true(re.search(r'^quick: \d+ mutants on 2 of 2 shards; .* cap 1024 MB;', out, re.M), out=out)
        self.common(root, header, work, out)
        res = {(m['mutation_slug'], m['new_text']): o for _, m, o in self.statuses(work).values()}
        runaway = res[('SOS', '<<')]
        self.true(runaway['status'] == 'TestFail' and 'memory cap' in runaway['output'], outcome=runaway)
        survivors = sorted(key for key, o in res.items() if o['status'] == 'Uncaught')
        self.eq(survivors, [('AOS', '+')], res={k: o['status'] for k, o in res.items()})

    def test_e2e_timeout_alone(self):
        # LOOP on 2 shards, one mutant each: both time out in the concurrent quick pass (NR loops, BL sees NR's lock); alone,
        # NR times out again (caught by timeout through its per-mutant scope) and BL passes, escalates and survives
        header, tpath = '02-Data Structures/01-loop.hpp', '96-Local Testing/02-Data Structures/01-loop_tester.py'
        scratch = self.tmp / 'scratch'
        scratch.mkdir()
        tester = (LOOP_TESTER.replace('@HEADER@', header).replace('@CXX@', self.cxx).replace('@LOCK@', repr(str(self.tmp / 'contention.lock')))
                  .replace('@TMP@', repr(str(scratch))).replace('@PROBE@', '3'))
        root = self.repo({header: LOOP, tpath: tester, tpath.replace('.py', '.golden'): LOOP})
        work = self.tmp / 'work'
        code, out, err = self.campaign(root, header, work, '--mutations', 'NR,BL', '--timeout', '8')
        self.eq(code, 0, out=out, err=err)
        meta, report = self.common(root, header, work, out)
        res = {m['mutation_slug']: (i, o['status']) for i, (_, m, o) in self.statuses(work).items()}
        self.eq({k: v[1] for k, v in res.items()}, {'NR': 'Timeout', 'BL': 'Uncaught'}, out=out)
        nr, bl = res['NR'][0], res['BL'][0]
        ran = {n: [sorted(self.stage_ids(work / f's{k}', n)) for k in range(2)] for n in ('quick', 'quick-alone', 'full', 'full-alone')}
        ran['quick'].sort()
        self.eq(ran, {'quick': sorted([[nr], [bl]]), 'quick-alone': [sorted([nr, bl]), []], 'full': [[bl], []], 'full-alone': [[], []]}, out=out)
        self.true(re.search(r'^quick: 2 mutants on 2 of 2 shards; .* timeout 8 s, cap 1024 MB;', out, re.M)
                  and re.search(r'^quick-alone: 2 mutants on 1 of 2 shards; .* timeout 8 s, cap 1024 MB;', out, re.M)
                  and f': 2 mutants, 1 caught (1 by timeout), 1 survived, 0 untested.\n' in out and f'| `BL {bl}` (low) | 6 |' in out
                  and meta['unconfirmed'] == {} and report['survivors'] == [bl], out=out, meta=meta)
        self.eq(self.left(work), [], what='processes left in the workdir')

    def test_e2e_budget_stop(self):
        header = '02-Data Structures/01-toy.hpp'
        root = self.repo(self.golden(header, sleep=1))  # 1 s per quick-mode test, full mode immediate
        work, sample = self.tmp / 'work', ['--mutations', 'COS,RDV']
        code, out, err = self.campaign(root, header, work, *sample, '--budget', '1e-9')
        self.eq(code, 3, out=out, err=err, what='deadline before the first stage')
        meta, report = self.common(root, header, work, out, complete=False)
        want = self.ids(work / 's0', '--mutation-types', 'COS,RDV')
        self.true(meta['selected'] == want and len(want) < len(self.ids(work / 's0')) and report['untested'] == len(want)
                  and not re.search(r'^quick:', out, re.M) and not list(work.glob('s*/*.ids')), meta=meta, out=out, what='no baseline run')
        start = time.monotonic()
        code, out, err = self.campaign(root, header, work, *sample, '--budget', '0.1')
        took = time.monotonic() - start
        self.eq(code, 3, out=out, err=err)
        meta, report = self.common(root, header, work, out, complete=False)
        self.true('STOPPED by the budget: ' in out and 0 < report['untested'] < len(want) + 1 and took < 90, report=report, took=took, out=out)
        code, out, err = self.campaign(root, header, work, *sample)
        self.eq(code, 0, out=out, err=err, what='resume after the budget stop')
        _, report = self.common(root, header, work, out)
        self.eq((report['untested'], report['counts']), (0, {'TestFail': len(want)}), report=report)

    def test_e2e_stage_failures(self):
        header, tpath = '02-Data Structures/01-toy.hpp', '96-Local Testing/02-Data Structures/01-toy_tester.py'
        root = self.repo(self.golden(header))
        with patch.object(M._00_memory_cap, 'cgroup_stats', return_value=(0, 1)), patch.dict(os.environ, {'CP_MEMORY_CAPPED': '4096'}):
            code, out, err = self.campaign(root, header, self.tmp / 'w-kill', '--mutations', 'IF')
        self.true(code == 1 and 'FAIL run: quick: the campaign cap killed 1 process(es)' in err, out=out, err=err)
        put(root, {tpath: 'import sys\nprint("usage")\nsys.exit(1)\n'})
        code, out, err = self.campaign(root, header, self.tmp / 'w-base', '--mutations', 'IF')
        self.true(code == 1 and 'FAIL run: quick baseline failed (exit 1)' in err, out=out, err=err)

    def test_e2e_signals(self):
        # SIGTERM or SIGHUP to a campaign inside a cap, and SIGTERM to the top-level _00_memory_cap supervisor (alone, which
        # forwards it, or to its whole group, so the campaign gets it twice): exit 130, one INTERRUPTED line, no process
        # left in the workdir, every shard's header restored
        header = '02-Data Structures/01-toy.hpp'
        root = self.repo(self.golden(header, sleep=2))
        driver = self.tmp / 'driver.py'
        driver.write_text(DRIVER)
        for name, sig, supervised, group in (('sigterm', signal.SIGTERM, False, False), ('sighup', signal.SIGHUP, False, False),
                                             ('supervisor', signal.SIGTERM, True, False), ('group', signal.SIGTERM, True, True)):
            work, log = self.tmp / f'w-{name}', self.tmp / f'{name}.log'
            env = {k: v for k, v in os.environ.items() if not (supervised and k == 'CP_MEMORY_CAPPED')}
            with open(log, 'w') as f:
                p = subprocess.Popen([sys.executable, str(driver), str(MUTATION_PY), str(root), 'run', str(root / header), '--workdir', str(work),
                                      '--jobs', '2', '--memory-mb', '4096'], env=env, stdout=f, stderr=subprocess.STDOUT, start_new_session=True)
            self.addCleanup(lambda p=p: p.poll() is None and os.killpg(p.pid, signal.SIGKILL))
            until = time.monotonic() + 120
            while not (work / 's0/quick.ids').exists() and p.poll() is None and time.monotonic() < until:
                time.sleep(0.2)
            self.true((work / 's0/quick.ids').exists(), log=log.read_text(), variant=name)
            time.sleep(3)  # past mewt's 2 s baseline: a mutant is applied in the shards
            start = time.monotonic()
            os.killpg(p.pid, sig) if group else p.send_signal(sig)
            code = p.wait(150)
            took, text = time.monotonic() - start, log.read_text()
            self.true(code == 130 and text.count('INTERRUPTED run') == 1 and took < 150, code=code, took=took, log=text, variant=name)
            self.true(not supervised or not _00_memory_cap.available() or re.search(r'^MEMORY peak=\d+MB cap=\d+MB', text, re.M), log=text, variant=name,
                      what='the campaign ran inside the supervisor\'s cap')
            self.eq(self.left(work), [], what='processes left in the workdir', variant=name)
            for k in range(2):
                self.eq((work / f's{k}' / header).read_bytes(), (root / header).read_bytes(), shard=k, what='header restored', variant=name)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=6, help='random rounds per random test in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Pure, Lines, Repo, Install, Version, Stage, Unchanged, Run, Fake, E2E):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n not in FULL)
    print(f"mutation checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else 2 if MODE == 'full' else 1} "
          f"tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
