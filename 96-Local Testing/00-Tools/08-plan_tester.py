#!/usr/bin/env python3
"""Checks for the contributor roles and pull-request claims of 00-Guidelines/13-Plan/plan.py and the role line of
.claude/skills/finish/gaps.py.

No test touches the network or the real git configuration. A fake `gh` (a shell wrapper around a Python script) sits
alone with a `git` symlink on PATH and answers `api user` and `pr list` from fixture files, logs its arguments, and can
fail with a given stderr or sleep; PATH without it models a missing gh, and a gh with `#!/nonexistent/interp` or a
malformed ELF one that cannot be executed. The role comes from GIT_CONFIG_COUNT/KEY_0/
VALUE_0 (the real repository's local config sets compprog.role=maintainer, so every real-repository run overrides it),
with GIT_CONFIG_GLOBAL and GIT_CONFIG_SYSTEM at /dev/null; the unset case uses a temporary `git init` repository.
Synthetic plans are dicts (claims, gate, ready) or a temporary repository with its own plan JSON, finish log, an
untracked evidence file (a live package), linked worktrees where a test adds them, and a copy of plan.py (show,
ready_text, status, the CLI, live, gaps.py). The real
repository is only read (show, ready_text, status, next, show next, gaps.py with role and gh overridden).
quick: every fixed case. full: quick plus seeded random rounds: 200 claim_branch cases, 40 PR lists for claims and
gate, 40 synthetic plans for ready(skip), 40 login strings, 4 random PR lists through the real `plan.py next`/`status`,
and 3 through the real ready_text. stress: full with --rounds random rounds (default 300) for claims, gate, ready and
login, 10 x --rounds claim_branch cases, and --rounds // 20 real-repository rounds. Modes/seeds also come from
CP_TEST_MODE/CP_TEST_SEED.
Oracles (written here, independent of plan.py): a character scanner for claim branches and for the login slug, a
lowest-number-per-package claim table, the block table and claims line rebuilt from the role, login and claims, a
first-ready scan over prerequisites rebuilt from batches.json, the landed set and first open batch from the 'verified'
lines of finish-log.tsv, the held text and a greedy folder-disjoint set in which live and claimed packages reserve.
Failing random rounds print the seed, mode, round and a greedily shrunk PR list.

Feature map:
role (unset, contributor, maintainer, invalid -> SystemExit, read in root) -> test_role_*;
gh (missing, non-zero exit first stderr line or 'gh <cmd> failed', success, 10 s timeout, not executable ->
'gh <cmd> failed: <strerror>' for ENOENT and ENOEXEC) -> test_gh_*, test_syn_gh_not_executable;
config and the claims repository (REPO default, compprog.repo override) -> test_config_and_claims_repo;
login (gh api user only when ask, git user.name fallback also for empty or whitespace-only gh output, slug, 'user') ->
test_login, test_login_ask_false, test_login_fallbacks, test_login_random;
claims (arguments, CLAIM fullmatch, batch owner, case of the batch, lowest number, value, SKIP reason, non-JSON output ->
'gh pr list printed no JSON') -> test_claims_fixture, test_claims_random, test_syn_no_json_skip;
gate (role, login only when needed and without gh after a SKIP, block texts, case-insensitive self, maintainer-only,
claims line) -> test_gate_*, test_gate_random;
claim_branch (own claim, current claim branch of a batch of the package, '<id>-<batch>-<me>') -> test_claim_branch,
test_claim_branch_random;
landed ('verified' lines only; 'partial' and 'branch' do not count) and batch_done of support batches ->
test_syn_show_unclaimed, test_gaps_role_line, test_real_show_unclaimed, test_real_gaps_role_line;
live (linked worktree directory name and branch only, whole tokens only (agent-a1c07e2f names nothing), a claim name
reduced to Pxxx-<batch>, untracked evidence) -> test_syn_live_worktrees, test_syn_ready_text_reservations;
ready(skip) -> test_ready_skip, test_ready_random; next/status/show next use the block set -> test_syn_next_*,
test_syn_status_text, test_real_next_status_show_next, test_real_next_random, test_real_all_claimed;
show (gate line; STOP; 'Claimed by you ... on <b>' ending '.' on branch b, else with the fetch-and-switch hint; for a
contributor on any other package, verified included, 'Unclaimed: claim' or after a SKIP 'Claims unknown: ...' with
claim_branch of the brief batch and --repo, ' it on this branch: `git commit ...' when the current branch is that claim
branch, else from an up-to-date master with `git switch -c`; the /package hint's batch; no crash when gh cannot run) ->
test_syn_show_*,
test_syn_no_json_skip, test_syn_gh_not_executable, test_real_show_*;
ready_text (gate line, held texts including 'maintainer-only (Core full); claimed by you in pull request #n', disjoint set,
live and claimed packages reserve, maintainer-only alone does not) -> test_syn_ready_text_*, test_syn_no_json_skip,
test_real_ready_text*;
gaps.py role line (claim_branch with the gate's login and claims and the batch key or else the first batch without a
'verified' log line, the branch itself without a package; '; repository <compprog.repo or REPO>; ', 'pull request #n; '
for the user's own claim on the package, then 'last open batch: mark the pull request ready if verified' when every
other batch is logged 'verified' or without a package, else 'more batches follow: the pull request stays a draft'; no
gh login after a SKIP; no crash when gh cannot run) -> test_gaps_role_line, test_syn_gh_not_executable,
test_real_gaps_role_line;
gaps.py target with no argument (a claim branch, also a reused landed one, takes the package's first batch without a
'verified' log line, P250-SUP17-alice -> key SUP18; every batch logged -> the batch it names; a non-claim branch keeps
its batch) -> test_gaps_role_line.
"""
import argparse
import errno
import importlib.util
import json
import os
from pathlib import Path
import random
import shlex
import shutil
import string
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
PLAN_PY = ROOT / '00-Guidelines/13-Plan/plan.py'
GAPS_PY = ROOT / '.claude/skills/finish/gaps.py'
MODE, SEED, ROUNDS = 'full', 42, 300
REPO = 'Ausp3x/CompProg-Library'
PR_LIST = ['pr', 'list', '--repo', REPO, '--state', 'open', '--limit', '1000', '--json', 'number,headRefName,isDraft,author']
RANDOM = {'test_claim_branch_random', 'test_claims_random', 'test_gate_random', 'test_ready_random', 'test_login_random', 'test_real_next_random',
          'test_real_ready_text_random'}
LETTERS, DIGITS = set(string.ascii_letters), set(string.digits)
SLUG = LETTERS | DIGITS | {'-'}
FAKE_GH = r'''import json, os, sys, time
d, args = os.environ['FAKE_GH_DIR'], sys.argv[1:]
with open(os.path.join(d, 'calls.jsonl'), 'a') as f:
    f.write(json.dumps(args) + '\n')
key = {('api', 'user'): 'user', ('pr', 'list'): 'prs'}.get(tuple(args[:2]), 'other')
p = lambda ext: os.path.join(d, key + ext)
if os.path.exists(p('.sleep')):
    time.sleep(float(open(p('.sleep')).read()))
if os.path.exists(p('.fail')):
    sys.stderr.write(open(p('.fail'), encoding='utf-8').read())
    sys.exit(1)
if not os.path.exists(p('.out')):
    sys.stderr.write('fake gh: no fixture for ' + key + '\n')
    sys.exit(4)
sys.stdout.buffer.write(open(p('.out'), 'rb').read())
'''


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PLAN = load(PLAN_PY, 'plan_under_test')
REAL_SUBPROCESS_RUN = subprocess.run


# Independent oracles

def o_slug(s):
    """Runs of [A-Za-z0-9-] joined by '-', stripped of '-', 'user' if empty (character scan)."""
    runs, cur = [], ''
    for c in s:
        if c in SLUG:
            cur += c
        elif cur:
            runs.append(cur)
            cur = ''
    runs.append(cur)
    out = '-'.join(r for r in runs if r)
    while out.startswith('-') or out.endswith('-'):
        out = out.strip('-')
    return out or 'user'


def o_parse(branch):
    """(package id, batch) of a claim branch P<3 digits>-<letters><digits>-<at least one character>, else None."""
    if len(branch) < 5 or branch[0] != 'P' or not all(c in DIGITS for c in branch[1:4]) or branch[4] != '-':
        return None
    j = 5
    while j < len(branch) and branch[j] in LETTERS:
        j += 1
    k = j
    while k < len(branch) and branch[k] in DIGITS:
        k += 1
    if j == 5 or k == j or k + 1 >= len(branch) or branch[k] != '-' or '\n' in branch[k + 1:]:
        return None
    return branch[:4], branch[5:k]


def o_claims(prs, packages):
    owner = {}
    for p in packages['packages']:
        for b in p['batches']:
            owner[b] = p['id']
    best = {}
    for pr in prs:
        r = o_parse(pr['headRefName'])
        if r and owner.get(r[1].upper()) == r[0] and (r[0] not in best or pr['number'] < best[r[0]][2]):
            best[r[0]] = (pr['author']['login'], pr['headRefName'], pr['number'], pr['isDraft'])
    return best


def o_block(role, me, claims, packages):
    """Claims by others; for a contributor every fable package not claimed by someone else is maintainer-only (a
    contributor's own claim of a fable package is still maintainer-only, as plan.gate does)."""
    out = {}
    for pid, (a, b, n, d) in claims.items():
        if a.lower() != me.lower():
            out[pid] = 'claimed by @' + a + ' in ' + ('draft ' if d else '') + 'pull request #' + str(n) + ' (' + b + ')'
    if role == 'contributor':
        for p in packages['packages']:
            if p['model'] == 'fable' and p['id'] not in out:
                out[p['id']] = 'maintainer-only (Core full)'
    return out


def o_line(role, claims, why):
    tail = 'SKIP (' + why + ')' if why else (str(len(claims)) + ' open' if claims else 'none open')
    return 'Role: ' + role + '; claims: ' + tail


def o_deps(batches, packages):
    owner = {b: p['id'] for p in packages['packages'] for b in p['batches']}
    bmap = {b['id']: b for b in batches['batches']}
    out = {}
    for p in packages['packages']:
        ds = set()
        for b in p['batches']:
            for d in bmap.get(b, {'prerequisites': []})['prerequisites']:
                if d in owner and owner[d] != p['id']:
                    ds.add(owner[d])
        out[p['id']] = ds
    return out


def o_claim_branch(pid, pbatches, batch, branch, claims, me):
    """The user's own claim's branch, else the current branch if it is a claim branch of a batch of pid, else
    '<pid>-<batch>-<me>'."""
    if pid in claims and claims[pid][0].lower() == me.lower():
        return claims[pid][1]
    m = o_parse(branch or '')
    if m and m[0] == pid and m[1].upper() in pbatches:
        return branch
    return pid + '-' + batch + '-' + me


def o_claim_line(skip, pid, batch, b, repo, here=False):
    """show's claim line; `here` when the current branch is already the claim branch b."""
    head = 'Claims unknown: check the open pull requests, then claim' if skip else 'Unclaimed: claim'
    where = ' it on this branch: `' if here else ' it in the checkout the session will work in, from an up-to-date master: `git switch -c ' + b + ' && '
    return (head + where + 'git commit --allow-empty -m \'' + pid + ': claim ' + batch + '\' && git push -u origin ' + b
            + ' && gh pr create --draft --fill --repo ' + repo + '`.')


def o_gaps_line(b, last, repo, n=None):
    """gaps.py's contributor role line; n is the number of the user's own open claim on the package."""
    return ('role contributor: step 7 commits on branch ' + b + ' and stops before landing; repository ' + repo + '; '
            + ('pull request #' + str(n) + '; ' if n else '')
            + ('last open batch: mark the pull request ready if verified' if last else 'more batches follow: the pull request stays a draft'))


def o_ready(batches, packages, skip):
    status = {p['id']: p['status'] for p in packages['packages']}
    deps = o_deps(batches, packages)
    for p in packages['packages']:
        if p['status'] != 'verified' and p['id'] not in skip and all(status[d] == 'verified' for d in deps[p['id']]):
            return p['id']
    return None


def o_ready_rows(batches, packages):
    """[(id, folders)] of the open packages whose prerequisites are verified, in order."""
    bmap = {b['id']: b for b in batches['batches']}
    status, deps = {p['id']: p['status'] for p in packages['packages']}, o_deps(batches, packages)
    rows = []
    for p in packages['packages']:
        if p['status'] == 'verified' or any(status[d] != 'verified' for d in deps[p['id']]):
            continue
        fs = {bmap[b]['folder'] for b in p['batches']} | {t.split('/')[0] for b in p['batches'] for t in bmap[b]['targets']}
        rows.append((p['id'], (fs - {'support'}) or {'support'}))
    return rows


def o_landed(root):
    lines = (root / '00-Guidelines/13-Plan/finish-log.tsv').read_text().splitlines()[1:]
    return {c[1] for c in (l.split('\t') for l in lines) if len(c) >= 8 and c[7] == 'verified'}


def o_held(pid, block, claims):
    """The gate's block text; a package the user claimed adds 'claimed by you in pull request #n' unless the block
    text is already a claim by someone else."""
    held = block.get(pid, '')
    if pid in claims and not held.startswith('claimed'):
        mine = 'claimed by you in pull request #' + str(claims[pid][2])
        held = held + '; ' + mine if held else mine
    return held


def o_disjoint(rows, held, busy, claims, limit):
    """Live packages and packages claimed by anyone reserve their folders; held ones are never chosen."""
    used = set()
    for pid, fs in rows:
        if pid in busy or pid in claims:
            used |= fs
    chosen = []
    for pid, fs in rows:
        if len(chosen) < limit and pid not in busy and not held[pid] and not fs & used:
            chosen.append(pid)
            used |= fs
    return chosen


def parse_ready_text(text):
    lines = text.split('\n')
    i = lines.index('Ready packages (prerequisites satisfied), schedule order:')
    listing = []
    for l in lines[i + 1:]:
        if not l:
            break
        listing.append(l)
    j = next(k for k, l in enumerate(lines) if l.startswith('Disjoint set for '))
    chosen, busy = [], set()
    for l in lines[j + 1:]:
        if l.startswith('- '):
            chosen.append(l[2:].split(' ')[0])
        elif l.startswith('Live here (worktree or uncommitted files), folders reserved: '):
            busy = set(l.split(': ', 1)[1].rstrip('.').split(', '))
        else:
            break
    return lines[0], listing, chosen, busy


def shrink(prs, fails):
    """Greedily drop pull requests while the failure persists."""
    prs, i = list(prs), 0
    while i < len(prs):
        trial = prs[:i] + prs[i + 1:]
        if fails(trial):
            prs = trial
        else:
            i += 1
    return prs


def pr(n, branch, author, draft=False):
    return {'number': n, 'headRefName': branch, 'isDraft': draft, 'author': {'login': author}}


def rand_prs(rng, packages, authors, k):
    pl = packages['packages']
    batches = [b for p in pl for b in p['batches']]
    prs, numbers = [], rng.sample(range(1, 5000), k)
    for n in numbers:
        p, kind = rng.choice(pl), rng.random()
        b = rng.choice(p['batches'])
        b = b.lower() if rng.random() < 0.2 else b
        user = rng.choice(['x', 'bob', 'a-b', 'feat/1', 'ü'])
        if kind < 0.6:
            branch = f'{p["id"]}-{b}-{user}'
        elif kind < 0.7:
            branch = f'{p["id"]}-{rng.choice(batches)}-{user}'
        else:
            branch = rng.choice([f'{p["id"]}-{b}-', f'{p["id"]}-{b}', f'p{p["id"][1:]}-{b}-{user}', f'feature/{p["id"]}-{b}-x',
                                 f'{p["id"]}0-{b}-x', f'{p["id"]}-{b}x-y', f'{p["id"]}-_{b}-y', f'{p["id"]}--{b}-y', 'main'])
        prs.append(pr(n, branch, rng.choice(authors), rng.random() < 0.5))
    return prs


def syn_plan(rng, n):
    """Random batches/packages: 1-3 batches each, prerequisites on any batch (some unknown), random status and model."""
    pkgs, batches, ids = [], [], []
    for i in range(n):
        own = [f'B{chr(65 + i)}{j}' for j in range(rng.randint(1, 3))]
        ids += own
        pkgs.append({'id': f'P{100 + i}', 'batches': own, 'status': rng.choice(['planned', 'in-progress', 'verified']),
                     'model': rng.choice(['opus', 'opus', 'fable'])})
    for b in ids:
        pre = rng.sample(ids, rng.randint(0, min(2, len(ids)))) + (['ZZ99'] if rng.random() < 0.1 else [])
        batches.append({'id': b, 'folder': rng.choice(['01-Core', '04-Graphs', 'support']), 'targets': [], 'prerequisites': pre})
    rng.shuffle(pkgs)
    return {'batches': batches}, {'packages': pkgs}


def git(cwd, *args, env=None):
    r = REAL_SUBPROCESS_RUN(['git', *args], cwd=cwd, capture_output=True, text=True, env=env or ISOLATED)
    if r.returncode:
        raise RuntimeError(f'git {" ".join(args)} failed in {cwd}: {r.stderr}')
    return r.stdout


ISOLATED = {k: v for k, v in os.environ.items() if not k.startswith(('GIT_', 'CLAUDE_CODE_AGENT', 'CLAUDE_EFFORT'))} | {
    'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_SYSTEM': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'}

SYN_BATCHES = {'batches': [
    {'id': 'ST01', 'folder': '07-Strings', 'targets': ['07-Strings/01-s.hpp'], 'prerequisites': [], 'tiers': ['Basic'],
     'size': 'S', 'focus': 'f'},
    *({'id': b, 'folder': 'support', 'targets': [], 'prerequisites': pre, 'tiers': [], 'size': 'S', 'focus': 'f'}
      for b, pre in (('SUP17', []), ('SUP18', ['SUP17']), ('SUP19', ['SUP17', 'SUP18']), ('SUP20', []), ('SUP21', ['SUP20']))),
    *({'id': b, 'folder': f, 'targets': [f'{f}/{t}'], 'prerequisites': pre, 'tiers': ['Basic'], 'size': 'M', 'focus': 'f'}
      for b, f, t, pre in (('C07', '01-Core', '10-matrix.hpp', []), ('C08', '01-Core', '11-matrixmini.hpp', []),
                           ('DS01', '02-Data Structures', '01-dsu.hpp', []), ('DS02', '02-Data Structures', '02-x.hpp', []),
                           ('GR01', '04-Graphs', '01-g.hpp', ['DS01']), ('MA01', '05-Mathematics', '01-m.hpp', [])))]}
SYN_PACKAGES = {'statuses': ['planned', 'in-progress', 'verified'], 'models': {'fable': 'claude-fable-5-1', 'opus': 'claude-opus-5-5'},
                'packages': [{'id': i, 'label': 'L' + i, 'batches': bs, 'phase': 'Contest core', 'size': 'M', 'model': m,
                              'status': s, 'evidence': ev, 'note': ''}
                             for i, bs, m, s, ev in (('P900', ['ST01'], 'opus', 'verified', []),
                                                     ('P250', ['SUP17', 'SUP18', 'SUP19'], 'opus', 'planned', ['notes/p250.md']),
                                                     ('P020', ['C07'], 'fable', 'planned', []), ('P021', ['C08'], 'opus', 'planned', []),
                                                     ('P030', ['DS01'], 'opus', 'in-progress', []), ('P031', ['DS02'], 'opus', 'planned', []),
                                                     ('P040', ['GR01'], 'opus', 'planned', []), ('P050', ['MA01'], 'opus', 'planned', []),
                                                     ('P260', ['SUP20', 'SUP21'], 'opus', 'in-progress', []))]}
SYN_PRS = [pr(5, 'P050-ma01-bob', 'bob', True), pr(3, 'P030-DS01-alice', 'Alice'), pr(8, 'P040-GR01-carol', 'carol'),
           pr(2, 'P021-C07-bob', 'bob'), pr(11, 'P050-MA01-alice', 'alice')]
FINISH_LOG = 'date\tkey\tmodel\teffort\treview_findings\tdefects_fixed\tdecisions\tresult\n' \
             '2026-10-01\tSUP17\tm\th\t0\t0\t0\tverified\n2026-10-02\tSUP18\tm\th\t0\t0\t0\tbranch\n' \
             '2026-10-03\tSUP19\tm\th\t0\t0\t0\tpartial\n2026-10-04\tSUP20\tm\th\t0\t0\t0\tverified\n' \
             '2026-10-05\tSUP21\tm\th\t0\t0\t0\tpartial\n2026-10-06\tST01\tm\th\t0\t0\t0\tverified\n'
# only 'verified' lands: SUP17, SUP20 and ST01


def make_root(path, live=True):
    """A synthetic repository: plan JSON, finish log, a copy of plan.py and worktree.py, one commit on master, and an
    untracked evidence file of P250 (a live package) when `live`."""
    plan = path / '00-Guidelines/13-Plan'
    plan.mkdir(parents=True)
    (plan / 'batches.json').write_text(json.dumps(SYN_BATCHES))
    (plan / 'packages.json').write_text(json.dumps(SYN_PACKAGES))
    (plan / 'finish-log.tsv').write_text(FINISH_LOG)
    shutil.copy(PLAN_PY, plan / 'plan.py')
    git(path, 'init', '-q', '-b', 'master')
    git(path, 'config', 'user.name', 'Repo User')
    git(path, 'config', 'user.email', 'repo@example.invalid')
    git(path, 'add', '-A')
    git(path, 'commit', '-q', '-m', 'init')
    if live:
        (path / 'notes').mkdir()
        (path / 'notes/p250.md').write_text('live\n')
    return path


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='plan-tester-'))
        # a failed test keeps its fixture directory, which its failure message names
        self.addCleanup(lambda: shutil.rmtree(self.tmp, ignore_errors=True) if self._outcome.success else None)
        self.fake, self.bin, self.nogh = self.tmp / 'gh-fixtures', self.tmp / 'bin', self.tmp / 'nogh'
        self.bad = {'interp': self.tmp / 'bad-interp', 'format': self.tmp / 'bad-format'}  # gh that cannot be executed
        for d in (self.fake, self.bin, self.nogh, *self.bad.values()):
            d.mkdir()
        (self.bad['interp'] / 'gh').write_text('#!/nonexistent/interp\n')
        (self.bad['format'] / 'gh').write_bytes(b'\x7fELF\x00garbage\n')
        for d in self.bad.values():
            (d / 'gh').chmod(0o755)
        (self.tmp / 'fake_gh.py').write_text(FAKE_GH)
        wrapper = self.bin / 'gh'
        wrapper.write_text(f'#!/bin/sh\nexec {shlex.quote(sys.executable)} {shlex.quote(str(self.tmp / "fake_gh.py"))} "$@"\n')
        wrapper.chmod(0o755)
        for d in (self.bin, self.nogh, *self.bad.values()):
            (d / 'git').symlink_to(shutil.which('git', path=ISOLATED.get('PATH')) or '/usr/bin/git')
        self.repo = self.tmp / 'repo'
        self.repo.mkdir()
        git(self.repo, 'init', '-q', '-b', 'master')
        git(self.repo, 'config', 'user.name', 'Repo User')

    def fail_info(self, **kw):
        return f'seed={SEED} mode={MODE} fixture={self.tmp} ' + ' '.join(f'{k}={v}' for k, v in kw.items())

    def env(self, role=None, gh=True, cfg=None):
        """Isolated environment: fake gh (or none) on PATH, role and other `cfg` keys through GIT_CONFIG_COUNT."""
        path = self.bad[gh] if isinstance(gh, str) else self.bin if gh else self.nogh  # gh: True, False, 'interp', 'format'
        e = dict(ISOLATED, PATH=str(path), FAKE_GH_DIR=str(self.fake))
        items = ([('compprog.role', role)] if role is not None else []) + list((cfg or {}).items())
        if items:
            e['GIT_CONFIG_COUNT'] = str(len(items))
            for i, (k, v) in enumerate(items):
                e[f'GIT_CONFIG_KEY_{i}'], e[f'GIT_CONFIG_VALUE_{i}'] = k, v
        return e

    def environ(self, role=None, gh=True, cfg=None):
        return patch.dict(os.environ, self.env(role, gh, cfg), clear=True)

    def gh_set(self, key, out=None, fail=None, sleep=None):
        for ext in ('.out', '.fail', '.sleep'):
            (self.fake / (key + ext)).unlink(missing_ok=True)
        for ext, v in (('.out', out), ('.fail', fail), ('.sleep', sleep)):
            if v is not None:
                (self.fake / (key + ext)).write_bytes(v.encode() if isinstance(v, str) else v)

    def prs(self, prs):
        self.gh_set('prs', json.dumps(prs))

    def calls(self):
        f = self.fake / 'calls.jsonl'
        calls = [json.loads(l) for l in f.read_text().splitlines()] if f.exists() else []
        f.unlink(missing_ok=True)
        return calls

    def check_ready_text(self, root, role, limit, batches, packages, prs, me='alice', live=None):
        with self.environ(role):
            text = PLAN.ready_text(root, limit)
        first, listing, chosen, busy = parse_ready_text(text)
        cl = o_claims(prs, packages)
        block = o_block(role, me, cl, packages)
        rows = o_ready_rows(batches, packages)
        held = {pid: o_held(pid, block, cl) for pid, _ in rows}
        info = self.fail_info(role=role, limit=limit, prs=json.dumps(prs))
        self.assertEqual(first, o_line(role, cl, ''), info)
        self.assertEqual([l[2:].split(' ')[0] for l in listing], [pid for pid, _ in rows], info)
        self.assertTrue(busy <= {pid for pid, _ in rows}, info)
        if live is not None:
            self.assertEqual(busy, live, info)
        for l, (pid, _) in zip(listing, rows):
            if held[pid]:
                self.assertTrue(l.endswith(' — ' + held[pid]), f'{info} line={l!r}')
            else:
                self.assertFalse(' — claimed by ' in l or 'maintainer-only' in l, f'{info} line={l!r}')
        self.assertEqual(chosen, o_disjoint(rows, held, busy, cl, limit), info)
        return chosen

    def run_py(self, script, *args, cwd, role=None, gh=True, cfg=None):
        return REAL_SUBPROCESS_RUN([sys.executable, str(script), *args], cwd=cwd, capture_output=True, text=True,
                                   env=self.env(role, gh, cfg), timeout=300)

    def api_calls(self):
        return [c for c in self.calls() if c[:2] == ['api', 'user']]


class Units(Base):
    """role, gh, login, claims, gate and ready on a temporary repository and synthetic plans."""
    PKGS = {'packages': [{'id': 'P250', 'batches': ['SUP17', 'SUP18'], 'model': 'opus', 'status': 'planned'},
                         {'id': 'P020', 'batches': ['C07'], 'model': 'fable', 'status': 'planned'},
                         {'id': 'P018', 'batches': ['C04'], 'model': 'fable', 'status': 'in-progress'},
                         {'id': 'P031', 'batches': ['DS02', 'DS27'], 'model': 'opus', 'status': 'planned'}]}

    def test_role_unset_and_values(self):
        with self.environ():
            self.assertEqual(PLAN.role(self.repo), 'contributor')
        for r in ('contributor', 'maintainer'):
            with self.environ(r):
                self.assertEqual(PLAN.role(self.repo), r)
        git(self.repo, 'config', 'compprog.role', 'maintainer')
        with self.environ():
            self.assertEqual(PLAN.role(self.repo), 'maintainer', 'local config of root')
        with self.environ('contributor'):
            self.assertEqual(PLAN.role(ROOT), 'contributor', 'env override of the real repository')

    def test_role_reads_root(self):
        other = self.tmp / 'other'
        other.mkdir()
        git(other, 'init', '-q')
        git(other, 'config', 'compprog.role', 'maintainer')
        with self.environ():
            self.assertEqual((PLAN.role(self.repo), PLAN.role(other)), ('contributor', 'maintainer'))

    def test_role_invalid(self):
        for bad in ('admin', 'Maintainer', 'contributors', 'x y'):
            with self.environ(bad), self.assertRaises(SystemExit) as c:
                PLAN.role(self.repo)
            self.assertIn(repr(bad), str(c.exception.code))
        with self.environ('owner'), self.assertRaises(SystemExit) as c:
            PLAN.gate(self.repo, self.PKGS)
        self.assertIn("'owner'", str(c.exception.code))

    def test_gh_success_and_failure(self):
        self.gh_set('user', out='octo\nsecond\n')
        with self.environ():
            self.assertEqual(PLAN.gh(self.repo, 'api', 'user'), ('octo\nsecond\n', ''))
            self.gh_set('user', fail='first error line\nsecond line\n')
            self.assertEqual(PLAN.gh(self.repo, 'api', 'user'), (None, 'first error line'))
            self.gh_set('prs', fail='')
            self.assertEqual(PLAN.gh(self.repo, 'pr', 'list'), (None, 'gh pr failed'))
            self.gh_set('other', out='')
            self.assertEqual(PLAN.gh(self.repo, 'repo', 'view'), ('', ''))
        self.assertEqual(self.calls(), [['api', 'user'], ['api', 'user'], ['pr', 'list'], ['repo', 'view']])

    def test_gh_not_executable(self):
        for kind, err in (('interp', errno.ENOENT), ('format', errno.ENOEXEC)):
            why = 'gh {} failed: ' + os.strerror(err)
            with self.environ('contributor', gh=kind):
                self.assertEqual(PLAN.gh(self.repo, 'api', 'user'), (None, why.format('api')), kind)
                self.assertEqual(PLAN.claims(self.repo, self.PKGS), ({}, why.format('pr')), kind)
                self.assertEqual(PLAN.login(self.repo), 'Repo-User', kind)
                self.assertEqual(PLAN.gate(self.repo, self.PKGS)[1:4],
                                 ('Repo-User', {}, f'Role: contributor; claims: SKIP ({why.format("pr")})'), kind)

    def test_gh_missing(self):
        self.gh_set('user', out='octo')
        self.prs([pr(1, 'P250-SUP17-x', 'x')])
        with self.environ(gh=False):
            self.assertEqual(PLAN.gh(self.repo, 'api', 'user'), (None, 'gh not installed'))
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), ({}, 'gh not installed'))
            self.assertEqual(PLAN.login(self.repo), 'Repo-User')
        self.assertEqual(self.calls(), [])

    def test_gh_timeout(self):
        seen = []

        def short(cmd, **kw):
            seen.append((cmd, kw.get('timeout'), kw.get('cwd')))
            return REAL_SUBPROCESS_RUN(cmd, **dict(kw, timeout=0.5))
        self.gh_set('user', out='octo', sleep='5')
        self.gh_set('prs', out='[]', sleep='5')
        with self.environ(), patch.object(PLAN.subprocess, 'run', short):
            self.assertEqual(PLAN.gh(self.repo, 'api', 'user', '--jq', '.login'), (None, 'gh api timed out'))
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), ({}, 'gh pr timed out'))
        self.assertEqual([(cmd[0], t, cwd) for cmd, t, cwd in seen if cmd[0] == 'gh'], [('gh', 10, self.repo)] * 2,
                         'gh runs in root with a 10 s timeout')

    def test_login(self):
        self.gh_set('user', out='Octo-Cat\n')
        with self.environ():
            self.assertEqual(PLAN.login(self.repo), 'Octo-Cat')
        self.assertEqual(self.calls(), [['api', 'user', '--jq', '.login']])
        cases = [('José  Pérez', 'Jos-P-rez'), ('--a__b--', 'a-b'), ('***', 'user'), (' a.b\tc ', 'a-b-c'), ('a--b', 'a--b'),
                 ('-', 'user'), ('x/y\n', 'x-y')]
        for raw, want in cases:
            self.assertEqual(o_slug(raw), want, f'oracle self-check {raw!r}')
            self.gh_set('user', out=raw)
            with self.environ():
                self.assertEqual(PLAN.login(self.repo), want, f'gh login {raw!r}')
            self.gh_set('user', fail='HTTP 401\n')
            git(self.repo, 'config', 'user.name', raw)
            with self.environ():
                self.assertEqual(PLAN.login(self.repo), want, f'git user.name {raw!r}')

    def test_login_ask_false(self):
        self.gh_set('user', out='octo\n')
        with self.environ():
            self.assertEqual((PLAN.login(self.repo, False), PLAN.login(self.repo, ask=True)), ('Repo-User', 'octo'))
        self.assertEqual(self.calls(), [['api', 'user', '--jq', '.login']], 'ask=False never calls gh')
        git(self.repo, 'config', '--unset', 'user.name')
        with self.environ():
            self.assertEqual(PLAN.login(self.repo, False), 'user')

    def test_login_fallbacks(self):
        self.gh_set('user', out='')
        with self.environ():
            self.assertEqual(PLAN.login(self.repo), 'Repo-User', 'empty gh output falls back')
        for blank in (' ', '\n', ' \t\n  \n'):
            self.gh_set('user', out=blank)
            with self.environ():
                self.assertEqual(PLAN.login(self.repo), 'Repo-User', f'whitespace-only gh output {blank!r} falls back')
        self.gh_set('user', fail='not logged in\n')
        with self.environ():
            self.assertEqual(PLAN.login(self.repo), 'Repo-User')
        git(self.repo, 'config', '--unset', 'user.name')
        with self.environ():
            self.assertEqual(PLAN.login(self.repo), 'user')
        with self.environ(gh=False):
            self.assertEqual(PLAN.login(self.repo), 'user')

    def test_login_random(self):
        rng = random.Random(SEED)
        alphabet = string.ascii_letters + string.digits + '-_. /\t@éü!' + '١'
        for r in range(40 if MODE != 'stress' else ROUNDS):
            raw = ''.join(rng.choice(alphabet) for _ in range(rng.randint(0, 12)))
            raw = rng.choice(['', ' ', ' \t ', '\n\n']) if rng.random() < 0.2 else raw
            self.gh_set('user', out=raw + rng.choice(['', '\n']))
            with self.environ():
                got = PLAN.login(self.repo)
            # empty or whitespace-only gh output falls back to git user.name 'Repo User'
            self.assertEqual(got, o_slug(raw) if raw.strip() else 'Repo-User', self.fail_info(round=r, raw=repr(raw)))

    def test_config_and_claims_repo(self):
        self.assertEqual(PLAN.REPO, REPO)
        with self.environ():
            self.assertEqual(PLAN.config(self.repo, 'compprog.repo'), '')
        with self.environ(cfg={'compprog.repo': 'fork/CompProg-Library'}):
            self.assertEqual(PLAN.config(self.repo, 'compprog.repo'), 'fork/CompProg-Library')
        self.prs([pr(1, 'P250-SUP17-x', 'x')])
        want = ({'P250': ('x', 'P250-SUP17-x', 1, False)}, '')
        with self.environ():
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), want)
        self.assertEqual(self.calls(), [PR_LIST], 'default repository')
        with self.environ('contributor', cfg={'compprog.repo': 'someone/fork'}):
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), want)
        self.assertEqual(self.calls(), [PR_LIST[:3] + ['someone/fork'] + PR_LIST[4:]], 'compprog.repo override')

    def test_claim_branch(self):
        p = {'id': 'P250', 'batches': ['SUP17', 'SUP18']}
        mine, other = {'P250': ('Alice', 'P250-sup17-old', 3, True)}, {'P250': ('bob', 'P250-SUP17-bob', 3, False)}
        for batch, branch, cl, me, want in (('SUP18', 'master', {}, 'alice', 'P250-SUP18-alice'),
                                            ('SUP18', 'P250-SUP18-x', mine, 'alice', 'P250-sup17-old'),
                                            ('SUP18', '', mine, 'ALICE', 'P250-sup17-old'),
                                            ('SUP18', 'P250-SUP17-x', other, 'alice', 'P250-SUP17-x'),
                                            ('SUP18', 'master', other, 'alice', 'P250-SUP18-alice'),
                                            ('SUP18', 'P250-sup17-a/b', {}, 'alice', 'P250-sup17-a/b'),
                                            ('SUP17', 'P250-C07-x', {}, 'alice', 'P250-SUP17-alice'),
                                            ('SUP17', 'P2500-SUP17-x', {}, 'alice', 'P250-SUP17-alice'),
                                            ('SUP17', 'P250-SUP17-', {}, 'alice', 'P250-SUP17-alice'),
                                            ('SUP17', None, {}, 'alice', 'P250-SUP17-alice'),
                                            ('SUP17', 'x/P250-SUP17-x', {}, 'alice', 'P250-SUP17-alice')):
            want_o = o_claim_branch(p['id'], p['batches'], batch, branch, cl, me)
            self.assertEqual(want_o, want, f'oracle self-check {branch!r}')
            self.assertEqual(PLAN.claim_branch(p, batch, branch, cl, me), want, f'{batch} {branch!r} {cl} {me}')

    def test_claim_branch_random(self):
        rng = random.Random(SEED + 6)
        for r in range(200 if MODE != 'stress' else 10 * ROUNDS):
            _, packages = syn_plan(rng, rng.randint(1, 4))
            p = rng.choice(packages['packages'])
            prs = rand_prs(rng, packages, ['bob', 'alice', 'ALICE'], rng.randint(0, 6))
            cl, me = o_claims(prs, packages), rng.choice(['alice', 'Alice', 'bob'])
            branch = rng.choice([x['headRefName'] for x in prs] + ['master', '', None])
            batch = rng.choice(p['batches'])
            got = PLAN.claim_branch(p, batch, branch, cl, me)
            self.assertEqual(got, o_claim_branch(p['id'], p['batches'], batch, branch, cl, me),
                             self.fail_info(round=r, p=p, batch=batch, branch=repr(branch), claims=cl, me=me))

    def test_claims_fixture(self):
        prs = [pr(9, 'P250-SUP18-bob', 'bob'), pr(4, 'P250-sup17-carol', 'carol', True), pr(2, 'P250-C07-x', 'x'),
               pr(1, 'P020-SUP17-x', 'x'), pr(3, 'feature/P031-DS02-x', 'x'), pr(5, 'P031-DS02-', 'x'),
               pr(6, 'P0310-DS02-x', 'x'), pr(7, 'p031-DS02-x', 'x'), pr(8, 'P031-DS02', 'x'), pr(10, 'P031-ds27-a-b/c', 'Dave'),
               pr(12, 'P031-DS02-eve', 'eve'), pr(13, 'P018-C04x-y', 'y'), pr(14, 'P018-C-y', 'y'), pr(15, 'P018-4-y', 'y'),
               pr(16, 'P999-C04-y', 'y')]
        self.prs(prs)
        want = {'P250': ('carol', 'P250-sup17-carol', 4, True), 'P031': ('Dave', 'P031-ds27-a-b/c', 10, False)}
        self.assertEqual(o_claims(prs, self.PKGS), want, 'oracle self-check')
        with self.environ():
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), (want, ''))
        self.assertEqual(self.calls(), [PR_LIST])
        self.prs([])
        with self.environ():
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), ({}, ''))
        self.gh_set('prs', fail='no git remotes found\nmore\n')
        with self.environ():
            self.assertEqual(PLAN.claims(self.repo, self.PKGS), ({}, 'no git remotes found'))
        for junk in ('', '\n', 'Notice: update available\n', '[{"number": 1,', '<html>'):
            self.gh_set('prs', out=junk)
            with self.environ():
                self.assertEqual(PLAN.claims(self.repo, self.PKGS), ({}, 'gh pr list printed no JSON'), repr(junk))

    def test_claims_random(self):
        rng = random.Random(SEED + 1)
        for r in range(40 if MODE != 'stress' else ROUNDS):
            batches, packages = syn_plan(rng, rng.randint(1, 8))
            prs = rand_prs(rng, packages, ['bob', 'Bob', 'alice'], rng.randint(0, 12))

            def fails(ps):
                self.prs(ps)
                with self.environ():
                    return PLAN.claims(self.repo, packages) != (o_claims(ps, packages), '')
            if fails(prs):
                self.fail(self.fail_info(round=r, packages=json.dumps(packages), prs=json.dumps(shrink(prs, fails))))

    def test_gate_maintainer(self):
        self.gh_set('user', out='alice')
        self.prs([])
        with self.environ('maintainer'):
            self.assertEqual(PLAN.gate(self.repo, self.PKGS), ('maintainer', '', {}, 'Role: maintainer; claims: none open', {}))
        self.assertEqual([c[:2] for c in self.calls()], [['pr', 'list']], 'no login without claims for a maintainer')
        self.prs([pr(4, 'P250-SUP17-bob', 'bob', True), pr(6, 'P031-DS02-alice', 'ALICE'), pr(7, 'P020-C07-carol', 'carol'),
                  pr(8, 'P250-SUP18-alice', 'alice')])
        cl = {'P250': ('bob', 'P250-SUP17-bob', 4, True), 'P031': ('ALICE', 'P031-DS02-alice', 6, False),
              'P020': ('carol', 'P020-C07-carol', 7, False)}
        block = {'P250': 'claimed by @bob in draft pull request #4 (P250-SUP17-bob)',
                 'P020': 'claimed by @carol in pull request #7 (P020-C07-carol)'}
        with self.environ('maintainer'):
            self.assertEqual(PLAN.gate(self.repo, self.PKGS), ('maintainer', 'alice', cl, 'Role: maintainer; claims: 3 open', block))
        self.gh_set('prs', fail='HTTP 502\n')
        with self.environ('maintainer'):
            self.assertEqual(PLAN.gate(self.repo, self.PKGS), ('maintainer', '', {}, 'Role: maintainer; claims: SKIP (HTTP 502)', {}))

    def test_gate_contributor(self):
        self.gh_set('user', out='alice')
        self.prs([])
        mo = 'maintainer-only (Core full)'
        with self.environ('contributor'):
            self.assertEqual(PLAN.gate(self.repo, self.PKGS),
                             ('contributor', 'alice', {}, 'Role: contributor; claims: none open', {'P020': mo, 'P018': mo}))
        self.assertEqual([c[:2] for c in self.calls()], [['pr', 'list'], ['api', 'user']], 'a contributor always resolves its login')
        self.prs([pr(3, 'P020-C07-bob', 'bob'), pr(5, 'P018-C04-alice', 'Alice'), pr(6, 'P031-DS27-alice', 'alice', True)])
        with self.environ('contributor'):
            r, me, cl, line, block = PLAN.gate(self.repo, self.PKGS)
        self.assertEqual((r, me, line), ('contributor', 'alice', 'Role: contributor; claims: 3 open'))
        self.assertEqual(block, {'P020': 'claimed by @bob in pull request #3 (P020-C07-bob)', 'P018': mo})
        self.calls()
        self.gh_set('prs', fail='')
        with self.environ():
            self.assertEqual(PLAN.gate(self.repo, self.PKGS),
                             ('contributor', 'Repo-User', {}, 'Role: contributor; claims: SKIP (gh pr failed)', {'P020': mo, 'P018': mo}))
        self.assertEqual(self.calls(), [PR_LIST], 'after a SKIP the login does not call gh')
        self.gh_set('prs', out='not json')
        with self.environ('contributor'):
            self.assertEqual(PLAN.gate(self.repo, self.PKGS)[1:4], ('Repo-User', {}, 'Role: contributor; claims: SKIP (gh pr list printed no JSON)'))
        self.assertEqual(self.calls(), [PR_LIST], 'after a no-JSON SKIP the login does not call gh')
        with self.environ('contributor', gh=False):
            self.assertEqual(PLAN.gate(self.repo, self.PKGS)[1:4], ('Repo-User', {}, 'Role: contributor; claims: SKIP (gh not installed)'))

    def test_gate_random(self):
        rng = random.Random(SEED + 2)
        for r in range(40 if MODE != 'stress' else ROUNDS):
            batches, packages = syn_plan(rng, rng.randint(1, 8))
            prs = rand_prs(rng, packages, ['bob', 'alice', 'ALICE', 'Repo-User', 'repo-user'], rng.randint(0, 10))
            role, user, why = rng.choice(['contributor', 'maintainer']), rng.choice(['alice', 'Al.ice', None, ' \n']), rng.choice(['', '', 'HTTP 500'])
            self.gh_set('user', out=user) if user else self.gh_set('user', fail='no auth\n')
            self.calls()

            def fails(ps):
                self.prs(ps) if not why else self.gh_set('prs', fail=why + '\n')
                with self.environ(role):
                    got = PLAN.gate(self.repo, packages)
                cl = o_claims(ps, packages) if not why else {}
                me = o_slug(user if user and user.strip() and not why else 'Repo User') if cl or role == 'contributor' else ''
                asked = self.api_calls()
                if asked != ([['api', 'user', '--jq', '.login']] if (cl or role == 'contributor') and not why else []):
                    return True
                return got != (role, me, cl, o_line(role, cl, why), o_block(role, me, cl, packages))
            if fails(prs):
                self.fail(self.fail_info(round=r, role=role, user=user, why=why, packages=json.dumps(packages),
                                         prs=json.dumps(shrink(prs, fails))))

    def test_ready_skip(self):
        b = {'batches': [{'id': x, 'prerequisites': pre} for x, pre in (('A1', []), ('B1', ['A1', 'B2']), ('B2', []), ('C1', ['B1']),
                                                                       ('D1', ['ZZ9']))]}
        p = {'packages': [{'id': i, 'batches': bs, 'status': s} for i, bs, s in
                          (('PA', ['A1'], 'verified'), ('PB', ['B1', 'B2'], 'planned'), ('PC', ['C1'], 'planned'), ('PD', ['D1'], 'in-progress'))]}
        self.assertEqual([(PLAN.ready(b, p, s) or {}).get('id') for s in ((), {'PB'}, {'PB', 'PD'}, ['PD'])], ['PB', 'PD', None, 'PB'])
        self.assertEqual([o_ready(b, p, s) for s in ((), {'PB'}, {'PB', 'PD'}, ['PD'])], ['PB', 'PD', None, 'PB'])
        p['packages'][1]['status'] = 'verified'
        self.assertEqual((PLAN.ready(b, p, {'PC'}) or {}).get('id'), 'PD', 'a skipped package is not verified for its dependents')

    def test_ready_random(self):
        rng = random.Random(SEED + 3)
        for r in range(40 if MODE != 'stress' else ROUNDS):
            batches, packages = syn_plan(rng, rng.randint(1, 10))
            skip = {p['id'] for p in packages['packages'] if rng.random() < 0.3}
            got = (PLAN.ready(batches, packages, skip) or {}).get('id')
            self.assertEqual(got, o_ready(batches, packages, skip),
                             self.fail_info(round=r, skip=sorted(skip), batches=json.dumps(batches), packages=json.dumps(packages)))


class Synthetic(Base):
    """show, status_text, ready_text, the CLI and gaps.py on a synthetic repository with a copy of plan.py."""
    def setUp(self):
        super().setUp()
        self.root = make_root(self.tmp / 'synroot')
        self.cli = self.root / '00-Guidelines/13-Plan/plan.py'
        self.gh_set('user', out='alice\n')
        self.prs(SYN_PRS)

    def show(self, ident, role='contributor'):
        with self.environ(role):
            return PLAN.show(self.root, ident).split('\n')

    def test_syn_show_gate_line_and_stop(self):
        line = 'Role: contributor; claims: 3 open'
        for ident, third in (('P020', 'STOP: P020 is maintainer-only (Core full); pick another package (`plan.py ready`).'),
                             ('C07', 'STOP: P020 is maintainer-only (Core full); pick another package (`plan.py ready`).'),
                             ('P050', 'STOP: P050 is claimed by @bob in draft pull request #5 (P050-ma01-bob); pick another package '
                                      '(`plan.py ready`).'),
                             ('P040', 'STOP: P040 is claimed by @carol in pull request #8 (P040-GR01-carol); pick another package '
                                      '(`plan.py ready`).'),
                             ('P030', 'Claimed by you: pull request #3 on P030-DS01-alice; work on it after '
                                      '`git fetch origin P030-DS01-alice && git switch P030-DS01-alice`.')):
            out = self.show(ident)
            self.assertEqual(out[0].split(' — ')[0], ident if ident.startswith('P') else 'P020')
            self.assertEqual(out[1:3], [line, third], ident)
        out = self.show('P030', 'maintainer')
        self.assertEqual(out[1:3], ['Role: maintainer; claims: 3 open', 'Claimed by you: pull request #3 on P030-DS01-alice; work on '
                                    'it after `git fetch origin P030-DS01-alice && git switch P030-DS01-alice`.'])
        git(self.root, 'checkout', '-q', '-b', 'P030-DS01-alice')
        for role in ('contributor', 'maintainer'):
            self.assertEqual(self.show('DS01', role)[2], 'Claimed by you: pull request #3 on P030-DS01-alice.', 'on the claim branch')
        git(self.root, 'checkout', '-q', '-b', 'P030-ds01-alice')
        self.assertEqual(self.show('P030')[2], 'Claimed by you: pull request #3 on P030-DS01-alice; work on it after '
                                               '`git fetch origin P030-DS01-alice && git switch P030-DS01-alice`.', 'branch names are exact')
        git(self.root, 'checkout', '-q', 'master')
        self.assertEqual(self.show('P020', 'maintainer')[1:3], ['Role: maintainer; claims: 3 open', ''])
        self.assertEqual(self.show('P050', 'maintainer')[2][:15], 'STOP: P050 is c')

    def test_syn_show_unclaimed(self):
        # Only SUP17, SUP20 and ST01 have a 'verified' finish-log line (SUP18's is 'branch', SUP19's and SUP21's
        # 'partial'), so the first open batch of P250 is SUP18 and that of P260 is SUP21.
        self.assertEqual(sorted(o_landed(self.root)), ['ST01', 'SUP17', 'SUP20'])
        self.assertEqual(sorted(PLAN.landed(self.root)), ['ST01', 'SUP17', 'SUP20'])
        self.assertEqual(self.show('P260')[2], o_claim_line(False, 'P260', 'SUP21', 'P260-SUP21-alice', REPO))
        for ident, p, b in (('P250', 'P250', 'SUP18'), ('SUP17', 'P250', 'SUP17'), ('SUP19', 'P250', 'SUP19'), ('P031', 'P031', 'DS02'),
                            ('P021', 'P021', 'C08')):
            self.assertEqual(self.show(ident)[2], o_claim_line(False, p, b, f'{p}-{b}-alice', REPO), ident)
        hint = '. One batch per session, under its batch id: run `/package SUP18`, then `/finish SUP18`.'
        self.assertEqual([l for l in self.show('P250') if l.startswith('Batches: ')],
                         ['Batches: SUP17 (done), SUP18 (open), SUP19 (open)' + hint], 'the hint reuses the brief batch')
        with self.environ('contributor', cfg={'compprog.repo': 'fork/Lib'}):
            self.assertEqual(PLAN.show(self.root, 'P031').split('\n')[2], o_claim_line(False, 'P031', 'DS02', 'P031-DS02-alice', 'fork/Lib'))
        # the current branch of root is kept when it is a claim branch of a batch of the package, else the default
        # (on that branch the claim commands drop `git switch -c`)
        for branch, ident, b in (('P250-sup19-zed', 'P250', 'P250-sup19-zed'), ('P250-sup19-zed', 'SUP17', 'P250-sup19-zed'),
                                 ('P250-C07-zed', 'P250', 'P250-SUP18-alice'), ('P031-DS02', 'P031', 'P031-DS02-alice'),
                                 ('P031-DS02-alice', 'P031', 'P031-DS02-alice'), ('P250-SUP18-alice', 'P250', 'P250-SUP18-alice')):
            git(self.root, 'checkout', '-q', '-B', branch, 'master')
            batch = 'SUP17' if ident == 'SUP17' else 'SUP18' if ident == 'P250' else 'DS02'
            self.assertEqual(self.show(ident)[2], o_claim_line(False, ident[:4] if ident != 'SUP17' else 'P250', batch, b, REPO,
                                                               here=branch == b), f'{ident} on {branch}')
        self.gh_set('prs', fail='HTTP 503\n')
        self.assertEqual(self.show('P250')[2], o_claim_line(True, 'P250', 'SUP18', 'P250-SUP18-alice', REPO, here=True))
        git(self.root, 'checkout', '-q', '-B', 'P250-SUP18-Repo-User', 'master')
        self.assertEqual(self.show('P250')[2], o_claim_line(True, 'P250', 'SUP18', 'P250-SUP18-Repo-User', REPO, here=True))
        self.prs(SYN_PRS)
        git(self.root, 'checkout', '-q', 'master')
        self.gh_set('prs', fail='HTTP 503\n')
        self.calls()
        out = self.show('P031')
        self.assertEqual(out[1:3], ['Role: contributor; claims: SKIP (HTTP 503)',
                                    o_claim_line(True, 'P031', 'DS02', 'P031-DS02-Repo-User', REPO)])
        self.assertEqual(self.api_calls(), [], 'after a SKIP show does not ask gh for the login')
        self.prs(SYN_PRS)
        self.assertEqual(self.show('P900')[2], o_claim_line(False, 'P900', 'ST01', 'P900-ST01-alice', REPO),
                         'a verified package gets the claim line too')
        self.assertEqual(self.show('P900', 'maintainer')[2], '')
        self.assertEqual(self.show('P031', 'maintainer')[2], '', 'a maintainer gets no claim line')
        with self.environ('contributor'), self.assertRaises(SystemExit):
            PLAN.show(self.root, 'P777')

    def test_syn_show_next_and_status_text(self):
        for role, prs, want in (('contributor', SYN_PRS, 'P250'), ('maintainer', SYN_PRS, 'P250'),
                                ('contributor', SYN_PRS + [pr(1, 'P250-SUP19-bob', 'bob')], 'P021'),
                                ('maintainer', SYN_PRS + [pr(1, 'P250-SUP19-bob', 'bob')], 'P020')):
            self.prs(prs)
            cl = o_claims(prs, SYN_PACKAGES)
            self.assertEqual(o_ready(SYN_BATCHES, SYN_PACKAGES, o_block(role, 'alice', cl, SYN_PACKAGES)), want, 'oracle self-check')
            self.assertEqual(self.show('next', role)[0].split(' — ')[0], want, role)
            with self.environ(role):
                tail = PLAN.status_text(self.root).split('\n')[-2:]
            self.assertEqual(tail, [o_line(role, cl, ''), f'Next: {want} — L{want}'])

    def test_syn_next_cli(self):
        for role, prs, out, code in (('contributor', SYN_PRS + [pr(1, 'P250-SUP19-bob', 'bob')], 'P021\n', 0),
                                     ('maintainer', SYN_PRS + [pr(1, 'P250-SUP19-bob', 'bob')], 'P020\n', 0),
                                     ('contributor', [pr(i + 1, f'{p["id"]}-{p["batches"][-1]}-bob', 'bob')
                                                      for i, p in enumerate(SYN_PACKAGES['packages'])], '', 1)):
            self.prs(prs)
            r = self.run_py(self.cli, 'next', cwd=self.tmp, role=role)
            self.assertEqual((r.returncode, r.stdout), (code, out), r.stderr)
            if code:
                r = self.run_py(self.cli, 'show', 'next', cwd=self.tmp, role=role)
                self.assertEqual((r.returncode, r.stdout, r.stderr.strip()), (1, '', 'No package is ready'))
                r = self.run_py(self.cli, 'status', cwd=self.tmp, role=role)
                self.assertEqual(r.stdout.rstrip('\n').split('\n')[-1], 'Next: none ready (all verified, blocked or claimed).')
        self.gh_set('prs', fail='boom\n')
        r = self.run_py(self.cli, 'status', cwd=self.tmp, role='maintainer')
        self.assertEqual(r.stdout.rstrip('\n').split('\n')[-2:], ['Role: maintainer; claims: SKIP (boom)', 'Next: P250 — LP250'])

    def test_syn_ready_text_reservations(self):
        args = (self.root, 'contributor', 10, SYN_BATCHES, SYN_PACKAGES, SYN_PRS)
        # P250 live (untracked evidence), P020 maintainer-only (01-Core not reserved), P030 claimed by you and P050 by
        # bob (their folders reserved): only P021 is free.
        self.assertEqual(self.check_ready_text(*args, live={'P250'}), ['P021'])
        self.assertEqual(self.check_ready_text(self.root, 'maintainer', 10, *args[3:], live={'P250'}), ['P020'])
        self.assertEqual(self.check_ready_text(self.root, 'maintainer', 0, *args[3:]), [])
        self.prs([])
        self.assertEqual(self.check_ready_text(self.root, 'maintainer', 10, *args[3:5], [], live={'P250'}), ['P020', 'P030', 'P050'])
        self.assertEqual(self.check_ready_text(self.root, 'maintainer', 2, *args[3:5], []), ['P020', 'P030'])
        # A contributor's own claim of the fable P020 stays maintainer-only, adds the claim and reserves 01-Core, so P021
        # is no longer free; bob's claim of P031 reserves 02-Data Structures, so P030 is not free either.
        own = [pr(4, 'P020-C07-alice', 'ALICE', True), pr(6, 'P031-DS02-bob', 'bob')]
        self.prs(own)
        with self.environ('contributor'):
            listing = parse_ready_text(PLAN.ready_text(self.root, 10))[1]
        self.assertIn('- P020 [', listing[1])
        self.assertTrue(listing[1].endswith(' — maintainer-only (Core full); claimed by you in pull request #4'), listing[1])
        self.assertEqual(self.check_ready_text(self.root, 'contributor', 10, *args[3:5], own, live={'P250'}), ['P050'])
        self.assertEqual(self.check_ready_text(self.root, 'maintainer', 10, *args[3:5], own, live={'P250'}), ['P050'])

    def test_syn_no_json_skip(self):
        self.gh_set('prs', out='Notice: update available\n')
        line = 'Role: {}; claims: SKIP (gh pr list printed no JSON)'
        for role, nxt in (('contributor', 'P250'), ('maintainer', 'P250')):
            out = self.show('P031', role)
            self.assertEqual(out[1:3], [line.format(role), o_claim_line(True, 'P031', 'DS02', 'P031-DS02-Repo-User', REPO)
                                        if role == 'contributor' else ''])
            with self.environ(role):
                self.assertEqual(PLAN.status_text(self.root).split('\n')[-2:], [line.format(role), f'Next: {nxt} — L{nxt}'])
                self.assertEqual(PLAN.ready_text(self.root, 3).split('\n')[0], line.format(role))
            r = self.run_py(self.cli, 'next', cwd=self.tmp, role=role)
            self.assertEqual((r.returncode, r.stdout, r.stderr), (0, nxt + '\n', ''))
            for args in (('status',), ('ready',), ('show', 'P021')):
                r = self.run_py(self.cli, *args, cwd=self.tmp, role=role)
                self.assertEqual((r.returncode, r.stderr), (0, ''), args)
                self.assertIn(line.format(role), r.stdout.split('\n'), args)

    def test_syn_gh_not_executable(self):
        line = 'Role: {}; claims: SKIP (gh pr failed: ' + os.strerror(errno.ENOENT) + ')'
        for role in ('contributor', 'maintainer'):
            for args in (('show', 'P031'), ('show', 'next'), ('status',), ('ready',), ('next',)):
                r = self.run_py(self.cli, *args, cwd=self.tmp, role=role, gh='interp')
                self.assertEqual((r.returncode, r.stderr), (0, ''), f'{role} {args}')
                if args != ('next',):
                    self.assertIn(line.format(role), r.stdout.split('\n'), f'{role} {args}')
                else:
                    self.assertEqual(r.stdout, 'P250\n')
            r = self.run_py(GAPS_PY, 'P031', cwd=self.root, role=role, gh='interp')
            self.assertEqual((r.returncode, r.stderr), (0, ''), f'gaps.py {role}')
            self.assertIn(line.format(role), r.stdout.split('\n'), 'the brief in gaps.py')
            if role == 'contributor':
                self.assertIn(o_gaps_line('P031-DS02-Repo-User', True, REPO), r.stdout.split('\n'))

    def test_syn_live_worktrees(self):
        batches, packages = SYN_BATCHES, SYN_PACKAGES
        with self.environ():
            self.assertEqual(PLAN.live(self.root, batches, packages), {'P250'}, 'untracked evidence only')
        # The directory's parent names nothing, a claim branch's user part names nothing ('ds01' would be P030's DS01).
        for path, branch in (('gr01-home/wt', 'P030-DS01-alice'), ('wt2', 'P031-DS02-ds01'), ('P040-GR01-ma01', None),
                             ('ma01', 'topic'), ('detached-c08', ''), ('agent-a1c07e2f', ''), ('xds02y', '')):
            args = ['worktree', 'add', '-q'] + (['-b', branch] if branch else ['--detach'] if branch == '' else [])
            git(self.root, *args, str(self.tmp / 'wts' / path), *([] if branch is None else ['master']))
        # P040-GR01-ma01 (directory, a claim name) -> P040 via GR01, its own branch P040-GR01-ma01 likewise;
        # ma01 -> P050 (MA01); detached-c08 -> P021 (C08); only whole tokens count, so agent-a1c07e2f (c07 inside a1c07e2f)
        # and xds02y name nothing.
        with self.environ():
            self.assertEqual(PLAN.live(self.root, batches, packages), {'P250', 'P030', 'P031', 'P040', 'P050', 'P021'})
        for path in ('ma01', 'detached-c08', 'P040-GR01-ma01'):
            git(self.root, 'worktree', 'remove', '--force', str(self.tmp / 'wts' / path))
        with self.environ():
            self.assertEqual(PLAN.live(self.root, batches, packages), {'P250', 'P030', 'P031'})
            text = PLAN.ready_text(self.root, 10)
        self.assertEqual(parse_ready_text(text)[3], {'P250', 'P030', 'P031'})
        git(self.root, 'worktree', 'add', '-q', '-b', 'P030-DS01-x', str(self.tmp / 'wts' / 'gr01-ds02-home' / 'P050-MA01-ds02'), 'master')
        with self.environ():
            self.assertEqual(PLAN.live(self.root, batches, packages), {'P250', 'P030', 'P031', 'P050'},
                             'directory P050-MA01-ds02 names P050 only; its parent and user part name nothing')

    def gaps(self, *arg, role='contributor', gh=True, branch=None):
        if branch == '':
            git(self.root, 'checkout', '-q', '--detach')
        elif branch:
            git(self.root, 'checkout', '-q', '-B', branch, 'master')
        r = self.run_py(GAPS_PY, *arg, cwd=self.root, role=role, gh=gh)
        lines = r.stdout.split('\n')
        i = lines.index('### Target') + 1
        self.assertTrue(lines[i].startswith('key '), r.stdout[:500])
        self.key = lines[i][4:].split(' | ')[0]
        i += lines[i + 1].startswith('WARNING ')
        return r, lines[i + 1]

    def test_gaps_role_line(self):
        self.gh_set('user', out='Dana.K\n')
        stops = lambda b, last=True, repo=REPO, n=None: o_gaps_line(b, last, repo, n)
        r, line = self.gaps('P250', role='maintainer', branch='master')
        self.assertEqual((r.returncode, line), (0, 'role maintainer: step 7 lands on master'), r.stderr)
        # P250's first open batch is SUP18 (SUP17 landed); P021 is claimed by this user, P050 by bob.
        self.prs(SYN_PRS + [pr(1, 'P021-c08-dana-k', 'DANA-K', True)])
        # last: every other batch of the package is done (P250: SUP17 only; P260: SUP20 only); single-batch packages are last.
        # Without a package the branch is the whole scope, so the line reads 'last open batch'.
        # A no-argument run on a claim branch takes the package's first batch without a 'verified' log line (SUP18, not the
        # SUP19 the branch names); a branch that is not a claim keeps its batch.
        for arg, branch, want, last in ((('P250',), 'master', 'P250-SUP18-Dana-K', False), (('SUP18',), 'master', 'P250-SUP18-Dana-K', False),
                                        (('SUP17',), 'master', 'P250-SUP17-Dana-K', False), (('P260',), 'master', 'P260-SUP21-Dana-K', True),
                                        (('SUP21',), 'master', 'P260-SUP21-Dana-K', True), (('SUP20',), 'master', 'P260-SUP20-Dana-K', False),
                                        ((), 'P250-SUP19-erin', 'P250-SUP19-erin', False), ((), 'P250-sup19-erin', 'P250-sup19-erin', False),
                                        ((), 'P250-SUP19', 'P250-SUP19-Dana-K', False), (('P250',), 'P031-DS02-x', 'P250-SUP18-Dana-K', False),
                                        (('P031',), 'P031-DS02-y/z', 'P031-DS02-y/z', True), (('P250',), 'P250-C07-x', 'P250-SUP18-Dana-K', False),
                                        (('SUP19',), 'P250-C07-x', 'P250-SUP19-Dana-K', False), (('P250',), 'P250-sup18-erin', 'P250-sup18-erin', False),
                                        (('SUP19',), 'P250-SUP18-erin', 'P250-SUP18-erin', False), (('P250',), 'P250-ZZ99-x', 'P250-SUP18-Dana-K', False),
                                        (('P031',), 'P0311-DS02-x', 'P031-DS02-Dana-K', True), (('P021',), 'master', 'P021-c08-dana-k', True),
                                        (('P021',), 'P021-C08-other', 'P021-c08-dana-k', True), (('P050',), 'master', 'P050-MA01-Dana-K', True),
                                        (('P050',), 'P050-MA01-mine', 'P050-MA01-mine', True), ((), 'feature-x', 'feature-x', True),
                                        ((), '', '(detached)', True)):
            r, line = self.gaps(*arg, branch=branch)
            self.assertEqual((r.returncode, line), (0, stops(want, last, n=1 if want == 'P021-c08-dana-k' else None)),
                             f'{arg} on {branch!r}: {r.stderr}')
        # keys of no-argument runs (target): a claim branch, also a reused landed one, moves to the first batch not logged
        # 'verified'; with every batch logged it keeps the batch it names; a non-claim branch keeps its batch.
        for branch, key, want, last in (('P250-SUP17-alice', 'SUP18', 'P250-SUP17-alice', False),
                                        ('P250-SUP19-erin', 'SUP18', 'P250-SUP19-erin', False),
                                        ('P260-SUP20-alice', 'SUP21', 'P260-SUP20-alice', True),
                                        ('P260-sup21-alice', 'SUP21', 'P260-sup21-alice', True),
                                        ('P900-ST01-zed', 'ST01', 'P900-ST01-zed', True),
                                        ('P250-SUP19', 'SUP19', 'P250-SUP19-Dana-K', False),
                                        ('P250-SUP17', 'SUP17', 'P250-SUP17-Dana-K', False)):
            r, line = self.gaps(branch=branch)
            self.assertEqual((self.key, line), (key, stops(want, last)), f'no argument on {branch}: {r.stderr}')
        r, line = self.gaps('P021', branch='master', role='maintainer')
        self.assertEqual(line, 'role maintainer: step 7 lands on master')
        r = self.run_py(GAPS_PY, 'P260', cwd=self.root, role='contributor', cfg={'compprog.repo': 'fork/Lib'})
        self.assertIn(stops('P260-SUP21-Dana-K', True, 'fork/Lib'), r.stdout.split('\n'), 'compprog.repo override')
        r, line = self.gaps('P031', branch='master', gh=False)
        self.assertEqual(line, stops('P031-DS02-Repo-User'), 'gh missing: git user.name')
        self.gh_set('prs', fail='HTTP 401\n')
        self.calls()
        r, line = self.gaps('P031', branch='master')
        self.assertEqual(line, stops('P031-DS02-Repo-User'), 'claims SKIP: git user.name')
        self.assertEqual(self.api_calls(), [], 'after a SKIP neither gaps.py nor its brief asks gh for the login')
        self.prs(SYN_PRS)
        r, line = self.gaps('P031', role=None, branch='master')
        self.assertEqual(line, stops('P031-DS02-Dana-K'), 'unset role is contributor')
        r = self.run_py(GAPS_PY, 'P031', cwd=self.root, role='boss')
        self.assertEqual(r.returncode, 1)
        self.assertIn("'boss'", r.stderr)


class Real(Base):
    """The real repository, read only, with role and gh overridden."""
    def setUp(self):
        super().setUp()
        self.batches, self.packages = (json.loads((ROOT / '00-Guidelines/13-Plan' / n).read_text()) for n in ('batches.json', 'packages.json'))
        self.pl = self.packages['packages']
        self.gh_set('user', out='alice\n')
        self.prs([])

    def oracle_next(self, role, prs):
        cl = o_claims(prs, self.packages)
        me = 'alice' if cl or role == 'contributor' else ''
        return o_ready(self.batches, self.packages, o_block(role, me, cl, self.packages)), cl

    def first(self, cond):
        p = next((p for p in self.pl if p['status'] != 'verified' and cond(p)), None)
        if p is None:
            self.skipTest('SKIP: the real plan has no such package')
        return p

    def test_real_show_stop_and_claims(self):
        f = self.first(lambda p: p['model'] == 'fable')
        with self.environ('contributor'):
            out = PLAN.show(ROOT, f['id']).split('\n')
        self.assertEqual(out[1:3], ['Role: contributor; claims: none open',
                                    f'STOP: {f["id"]} is maintainer-only (Core full); pick another package (`plan.py ready`).'])
        o = self.first(lambda p: p['model'] == 'opus')
        b = o['batches'][-1]
        self.prs([pr(7, f'{o["id"]}-{b.lower()}-bob', 'bob', True), pr(9, f'{o["id"]}-{b}-alice', 'alice')])
        for role in ('contributor', 'maintainer'):
            with self.environ(role):
                out = PLAN.show(ROOT, o['id']).split('\n')
            self.assertEqual(out[1:3], [f'Role: {role}; claims: 1 open', f'STOP: {o["id"]} is claimed by @bob in draft pull request #7 '
                                        f'({o["id"]}-{b.lower()}-bob); pick another package (`plan.py ready`).'])
        self.prs([pr(9, f'{o["id"]}-{b}-alice', 'ALICE')])
        with self.environ('contributor'):
            out = PLAN.show(ROOT, b).split('\n')
        mine = f'{o["id"]}-{b}-alice'
        self.assertEqual(out[2], f'Claimed by you: pull request #9 on {mine}' + ('.' if git(ROOT, 'branch', '--show-current').strip() == mine
                                 else f'; work on it after `git fetch origin {mine} && git switch {mine}`.'))
        v = next((p for p in self.pl if p['status'] == 'verified' and p['model'] == 'opus'), None)
        if v:
            self.prs([])
            with self.environ('contributor'):
                out = PLAN.show(ROOT, v['batches'][0]).split('\n')
            want = o_claim_branch(v['id'], v['batches'], v['batches'][0], git(ROOT, 'branch', '--show-current').strip(), {}, 'alice')
            self.assertEqual(out[2], o_claim_line(False, v['id'], v['batches'][0], want, REPO,
                                                  here=git(ROOT, 'branch', '--show-current').strip() == want), 'a verified package')

    def test_real_show_unclaimed(self):
        bmap = {b['id']: b for b in self.batches['batches']}
        p = self.first(lambda p: p['model'] == 'opus' and len(p['batches']) > 1 and not any(bmap[b]['targets'] for b in p['batches']))
        done = o_landed(ROOT)
        first = next((b for b in p['batches'] if b not in done), p['batches'][0])
        branch = git(ROOT, 'branch', '--show-current').strip()
        for ident, b, cfg, repo in ((p['id'], first, None, REPO), (p['batches'][-1], p['batches'][-1], {'compprog.repo': 'fork/x'}, 'fork/x')):
            with self.environ('contributor', cfg=cfg):
                cb = o_claim_branch(p['id'], p['batches'], b, branch, {}, 'alice')
                self.assertEqual(PLAN.show(ROOT, ident).split('\n')[2], o_claim_line(False, p['id'], b, cb, repo, here=branch == cb), ident)
        with self.environ('maintainer'):
            self.assertEqual(PLAN.show(ROOT, p['id']).split('\n')[1:3], ['Role: maintainer; claims: none open', ''])

    def cli(self, *args, role):
        return self.run_py(PLAN_PY, *args, cwd=self.tmp, role=role)

    def check_cli(self, role, prs, info=''):
        self.prs(prs)
        want, cl = self.oracle_next(role, prs)
        r = self.cli('next', role=role)
        self.assertEqual((r.returncode, r.stdout), (0, want + '\n') if want else (1, ''), f'{info} {r.stderr}')
        r = self.cli('status', role=role)
        label = next((p['label'] for p in self.pl if p['id'] == want), '')
        self.assertEqual(r.stdout.rstrip('\n').split('\n')[-2:], [o_line(role, cl, ''), f'Next: {want} — {label}' if want else
                                                                  'Next: none ready (all verified, blocked or claimed).'], info)
        r = self.cli('show', 'next', role=role)
        self.assertEqual((r.stdout.split(' — ')[0], r.returncode) if want else (r.stderr.strip(), r.returncode),
                         (want, 0) if want else ('No package is ready', 1), info)
        return want

    def test_real_next_status_show_next(self):
        for role in ('contributor', 'maintainer'):
            want = self.check_cli(role, [])
            if want:
                b = next(p for p in self.pl if p['id'] == want)['batches'][0]
                self.check_cli(role, [pr(3, f'{want}-{b}-bob', 'bob')], f'{role}: {want} claimed by bob')
                self.check_cli(role, [pr(3, f'{want}-{b}-alice', 'Alice')], f'{role}: {want} claimed by you')

    def test_real_all_claimed(self):
        prs = [pr(i + 1, f'{p["id"]}-{p["batches"][0]}-bob', 'bob') for i, p in enumerate(self.pl) if p['status'] != 'verified']
        self.assertIsNone(self.check_cli('maintainer', prs))

    def test_real_next_random(self):
        rng = random.Random(SEED + 4)
        open_ = {'packages': [p for p in self.pl if p['status'] != 'verified'][:30]}
        for r in range(4 if MODE != 'stress' else max(4, ROUNDS // 20)):
            prs = rand_prs(rng, open_, ['bob', 'alice', 'ALICE'], rng.randint(0, 25))
            self.check_cli(rng.choice(['contributor', 'maintainer']), prs, self.fail_info(round=r, prs=json.dumps(prs)))

    def test_real_ready_text(self):
        rows = o_ready_rows(self.batches, self.packages)
        opus = [pid for pid, _ in rows if next(p for p in self.pl if p['id'] == pid)['model'] == 'opus']
        prs = [pr(4 + i, f'{pid}-{next(p for p in self.pl if p["id"] == pid)["batches"][0]}-{u}', u, i == 0)
               for i, (pid, u) in enumerate(zip(opus[:2], ('bob', 'alice')))]
        self.prs(prs)
        for role in ('contributor', 'maintainer'):
            self.check_ready_text(ROOT, role, 3, self.batches, self.packages, prs)

    def test_real_ready_text_random(self):
        rng = random.Random(SEED + 5)
        ready = {'packages': [p for p in self.pl if p['id'] in {pid for pid, _ in o_ready_rows(self.batches, self.packages)}]}
        for r in range(3 if MODE != 'stress' else max(3, ROUNDS // 20)):
            prs = rand_prs(rng, ready, ['bob', 'alice'], rng.randint(0, 15))
            self.prs(prs)
            self.check_ready_text(ROOT, rng.choice(['contributor', 'maintainer']), rng.randint(1, 6), self.batches,
                                       self.packages, prs)

    def test_real_gaps_role_line(self):
        branch = git(ROOT, 'branch', '--show-current').strip()
        p = self.first(lambda p: p['model'] == 'opus' and len(p['batches']) > 1)
        first = next((b for b in p['batches'] if b not in o_landed(ROOT)), p['batches'][0])  # log-based, not inventory rows
        for arg, role, batch in ((p['id'], 'contributor', first), (p['batches'][-1], 'contributor', p['batches'][-1]),
                                 (p['id'], 'maintainer', None)):
            want = batch and o_claim_branch(p['id'], p['batches'], batch, branch, {}, 'alice')
            last = batch and all(b == batch or b in o_landed(ROOT) for b in p['batches'])
            r = self.run_py(GAPS_PY, arg, cwd=ROOT, role=role)
            lines = r.stdout.split('\n')
            i = lines.index('### Target') + 2
            i += lines[i].startswith('WARNING ')
            self.assertEqual((r.returncode, lines[i]), (0, o_gaps_line(want, last, REPO) if want else 'role maintainer: step 7 lands on master'),
                             r.stderr)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=300, help='random rounds per random test in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Units, Synthetic, Real):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n not in RANDOM)
    print(f"plan role/claim checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
