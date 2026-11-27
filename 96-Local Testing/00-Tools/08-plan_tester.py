#!/usr/bin/env python3
"""Checks for the contributor roles and pull-request claims, the follow-ups and the limits gate of
00-Guidelines/13-Plan/plan.py and the role line, ledger rows and facts output of .claude/skills/finish/gaps.py.

No test touches the network or the real git configuration. A fake `gh` (a shell wrapper around a Python script) sits
alone with a `git` symlink on PATH and answers `api user` and `pr list` from fixture files, logs its arguments, and can
fail with a given stderr or sleep; PATH without it models a missing gh, and a gh with `#!/nonexistent/interp` or a
malformed ELF one that cannot be executed. The role comes from GIT_CONFIG_COUNT/KEY_0/
VALUE_0 (the real repository's local config sets compprog.role=maintainer, so every real-repository run overrides it),
with GIT_CONFIG_GLOBAL and GIT_CONFIG_SYSTEM at /dev/null; the unset case uses a temporary `git init` repository.
Synthetic plans are dicts (claims, gate, ready) or a temporary repository with its own plan JSON, finish log, an
untracked evidence file (a live package), linked worktrees where a test adds them, and a copy of plan.py (show,
ready_text, status, the CLI, live, gaps.py). gaps.py's row, ledger_rows, section, emit and limits and plan.py's room arithmetic are also called
on the imported module, emit with SECTIONS set and its __file__ patched beside a SKILL.md of chosen length. The real
repository is only read (show, ready_text, status, next, show next, gaps.py with role and gh overridden), except that
gaps.py writes its facts file there: each such run holds a lock against concurrent testers and then removes the
`.claude/finish/*.facts.txt` files it created and restores those it changed.
quick: every fixed case, 200 seeded random line lists for plan.fit, the real items.py on 3 sections and the real
rooms of the first 12 open packages. full: quick plus the real rooms of every open
package, every real items.py section, one real validator run, and seeded random rounds: 200 claim_branch cases, 40 PR lists for claims and
gate, 40 synthetic plans for ready(skip), 40 login strings, 40 random ledgers of up to 30 lines, 4 random PR lists
through the real `plan.py next`/`status`, and 3 through the real ready_text. stress: full with --rounds random rounds
(default 300) for claims, gate, ready, login and ledgers, 10 x --rounds fit lists, 10 x --rounds claim_branch cases, and --rounds // 20
real-repository rounds. Modes/seeds also come from
CP_TEST_MODE/CP_TEST_SEED.
Oracles (written here, independent of plan.py): a character scanner for claim branches and for the login slug, a
lowest-number-per-package claim table, the block table and claims line rebuilt from the role, login and claims, a
first-ready scan over prerequisites rebuilt from batches.json, the landed set and first open batch from the 'verified'
lines of finish-log.tsv, the held text and a greedy folder-disjoint set in which live and claimed packages reserve;
for gaps.py (SUP33) hand-written rows and outputs, random ledger lines whose class (open, closed, stray, blank) is
known by construction, the order of need written here, and the printed length against o_room: 5,000 tokens less the
skill's text at 2.5 characters a token, at 2.0 characters a token of output, shared by its '!`' lines (SUP34).
Failing random rounds print the seed, mode, round and a greedily shrunk PR list.

Feature map:
role (unset, contributor, maintainer, invalid -> SystemExit, read in root) -> test_role_*;
gh (missing, non-zero exit first stderr line or 'gh <cmd> failed', success, 10 s timeout, not executable ->
'gh <cmd> failed: <strerror>' for ENOENT and ENOEXEC) -> test_gh_*, test_syn_gh_not_executable;
config and the claims repository (REPO default, compprog.repo override) -> test_config_and_claims_repo;
login (gh api user only when ask, git user.name fallback also for empty or whitespace-only gh output, slug, 'user') ->
test_login, test_login_ask_false, test_login_fallbacks, test_login_random;
claims (arguments, CLAIM fullmatch, batch owner, case of the batch, a follow-up claim `FU<n>` of any package, lowest
number, value, SKIP reason, non-JSON output -> 'gh pr list printed no JSON') -> test_claims_fixture, test_claims_random,
test_syn_no_json_skip;
gate (role, login only when needed and without gh after a SKIP, block texts, case-insensitive self, maintainer-only,
claims line) -> test_gate_*, test_gate_random;
claim_branch (own claim, current claim branch of a batch or of the follow-ups of the package, '<id>-<batch>-<me>') ->
test_claim_branch, test_claim_branch_random;
landed ('verified' lines only; 'partial' and 'branch' do not count) and batch_done of support batches ->
test_syn_show_unclaimed, test_gaps_role_line, test_real_show_unclaimed, test_real_gaps_role_line;
live (linked worktree directory name and branch only, whole tokens only (agent-a1c07e2f names nothing), a claim name
reduced to Pxxx-<batch>, untracked evidence) -> test_syn_live_worktrees, test_syn_ready_text_reservations;
support_targets (in a batch's brief, marked support, and in live's file set) -> test_syn_support_targets;
waiting (push: commits ahead of the upstream, a claim branch without one; pull requests from one gh list call with
commits: maintainer `/merge-pr N`, drafts with batch progress from `Pxxx: <batch> ... — verified` subjects (any
verified subject for a one-batch package) and requested changes under others; contributor: own pull requests only,
case-insensitive, `gh pr ready N` once every batch is verified, `/package <next>`, changes requested, a ready one under
others; a full page noted; gh SKIP; /finish `ask` rows; /merge-pr records with an existing worktree; nothing; the CLI)
-> test_syn_waiting;
sweeps (verified Core full headers without a candidates table are pending; minis, unverified rows, the template and debug are not; a table's
shape, verdict words, empty evidence and undecided wording are faults; the Operation column's names; the maintainer's
waiting line) -> test_syn_sweeps;
gaps.py Core full sections (undecided wording and a missing candidates table; the newest Comparison line of 06-compare.py
against the current rig, header blobs and the deciding compiler's version, the first listed; neither section for a contest batch) -> test_gaps_core_full;
gaps.py review record (--reviewed stores the blobs of targets, tests and evidence in git's object database with the
report, --wrong the full-audit flag; Facts: none, covers with the report, changed, added and removed files with their
diff since the review in <key>.since-review.diff, a recorded wrong answer; no id fails) -> test_gaps_review_record; a batch without targets
covering the package's evidence and the branch's and uncommitted changes, not plan.py's four files or .claude/finish, an
edit and a new file reopening it (user decision 2026-11-23), its facts reading the package's evidence headings ->
test_gaps_review_record_support;
ready(skip) -> test_ready_skip, test_ready_random; next/status/show next use the block set -> test_syn_next_*,
test_syn_status_text, test_real_next_status_show_next, test_real_next_random, test_real_all_claimed;
show (gate line; STOP; 'Claimed by you ... on <b>' ending '.' on branch b, else with the fetch-and-switch hint; for a
contributor on any other package, verified included, 'Unclaimed: claim' or after a SKIP 'Claims unknown: ...' with
claim_branch of the brief batch and --repo, ' it on this branch: `git commit ...' when the current branch is that claim
branch, else from an up-to-date master with `git switch -c`; the /package hint's batch; no crash when gh cannot run) ->
test_syn_show_*,
test_syn_no_json_skip, test_syn_gh_not_executable, test_real_show_*;
show's commit-hook WARNING as its last line (none without setup_check.py in the checkout; not enabled, relative path,
enabled) -> test_syn_show_hook_warning;
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
its batch) -> test_gaps_role_line;
gaps.py row and ledger_rows (exactly six non-empty fields; 5, 7 and table-piped lines, an empty field or state, kind not
exactly defect or decision (Defect, bug), an id not matching [A-Za-z0-9][\\w.-]*; state by its leading letters in any case: open,
ask, `open:` and `ask,later` open, fixed, answered, `fixed.` and `answered 2026-11-22 (was: open ...)` closed, done,
closed, dropped, opened, fixes and `: open` stray; blank lines neither; 1-based stray numbers) -> test_gaps_row, test_gaps_ledger_random;
gaps.py ledger section (none: start at step 1; no open rows; the stray WARNING with the first 10 line numbers then
' ...' and the <key>.lab.tsv path, 'N fixed or answered rows not shown', then open rows) and the head's brief lines (its
first line and every WARNING or STOP line, printed before the facts pointer; a contributor's STOP line on a
maintainer-only package) -> test_gaps_facts, test_gaps_head_brief;
section (empty -> (none), the 400-line cap and its own cap) -> test_gaps_section;
emit (the facts file .claude/finish/<key>.facts.txt in full in the order sections were added, never .facts.md; the
'Full facts:' line with its length; printed sections by ORDER, ledger first, brief last, unknown titles after it; at
most LINES=12 lines per other section then '... k more in the facts file'; the ledger's open rows as many as fit;
sections that do not fit named under '### Not printed (in the facts file)' in order; printed length within
plan.room_of(SKILL.md), floored at 1500, 10,000 without SKILL.md; 'not written (<strerror>)' when the facts folder is a
file or the facts file a folder) -> test_gaps_emit, test_gaps_emit_room, test_gaps_emit_unwritable, test_gaps_facts,
test_real_gaps_role_line (the real repository's facts file name);
plan.py's room arithmetic (SUP34): its constants in one place, gaps.py holding none; room() shared by the '!`' lines and
room_of() with its floor and without a SKILL.md; reattach(), 03-consistency.py's skill_reattach rule (a skill whose '!`'
lines get under 2,500 characters each, any other over 11,250, an unpiped `plan.py show` line without --skill and the
skill's own name; both edges of each; the real skills flag none) -> test_gaps_facts_room; the validator printing
reattach(ROOT) (full mode) -> test_validator_reattach; fit() and spans() (everything when it fits, the first and kept
lines first, whole lines in order, the numbered pointer; seeded random lists against o_fit) -> test_fit; spill() (the
session's scratchpad, else claude-<uid>, None when unwritable) -> test_fit_spill; `plan.py show --skill` (fitted to that
skill's SKILL.md with its first, WARNING and STOP lines, whole when it fits, an unknown skill) -> test_syn_show_skill;
prune's items.py (whole, fitted with its headings and the pointer to duplicates.md, every real section within the real
room) -> test_items_room; gaps.py's Recorded limits (each limit heading's section in full, nested headings, the facts
file past 400 lines and 160 characters, the printed cap) -> test_gaps_limits, test_gaps_limits_facts; gaps.py --start
(once per key, an unknown id, a batch without targets leaving out unchanged files uncommitted at the start, covering a
new file and one changed since, other work changed later reopening it, an unreadable record) -> test_gaps_review_start;
every open package's gaps.py facts and fitted briefs within their rooms on the real plan (quick: the first 12) ->
test_real_rooms; gaps.py --reviewed after the change -> test_gaps_review_record;
follow-ups (user decision 2026-11-26): the checklist markers `[ ]`, `[-]`, `[x]`, `[!]` and the ' Follow-ups: (1) a; (2) b.'
suffix of any package -> test_render_markers; `plan.py follow-up` (one-line text, refusals, --done by number, the field
dropped with the last, saved and re-rendered) -> test_syn_follow_up_cli; a verified package with follow-ups as ready
work first in ready/next, its show block with the follow-up claim batch, waiting's follow-up claims ->
test_syn_follow_up_ready_and_show, test_syn_waiting; check's format rule, doctor's partial row and check's undecided
limits in a verified package, both allowed while follow-ups are open -> test_syn_follow_up_check_and_doctor;
undecided limits: plan.undecided (items under level-2 and level-3 Limits or Handoff headings without `user decision
<date>` or a `follow-up on Pxxx` whose packages all have open follow-ups; paragraphs, list items with continuations and
nested items, table body rows; none skipped), limit_line's `[no open follow-up on Pxxx]` marks, open_ids -> test_undecided;
`plan.py set Pxxx verified` refusing a package whose evidence (with the new --evidence paths) holds one, unless open
follow-ups of a verified package cover it, and `--done` keeping its last follow-up meanwhile -> test_syn_set_verified_gate;
a hand-off whose package has no open follow-up (user decision 2026-11-27) in check's message and set's refusal,
`--done` listing the limits citing the package it closes and keeping its last follow-up while one lacks a user decision
-> test_syn_hand_off; gaps.py's Undecided limits (a closed hand-off marked) and Follow-ups sections after the review
record -> test_gaps_undecided_and_follow_ups;
gaps.py's Sources section, the checkout's 00-sources.py check for the key and package, after the follow-ups and absent
without the script (user decision 2026-11-27) -> test_gaps_sources.
"""
import argparse
import contextlib
import datetime
import errno
import fcntl
import importlib.util
import io
import json
import os
from pathlib import Path
import random
import re
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
RANDOM = {'test_gaps_ledger_random', 'test_claim_branch_random', 'test_claims_random', 'test_gate_random', 'test_ready_random', 'test_login_random', 'test_real_next_random',
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
GAPS = load(GAPS_PY, 'gaps_under_test')
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


def o_follow(batch):
    """Whether a claim's batch part names follow-ups: FU then digits, in any case (user decision 2026-11-26)."""
    return len(batch) > 2 and batch[:2].upper() == 'FU' and all(c in DIGITS for c in batch[2:])


def o_claims(prs, packages):
    owner, ids = {}, set()
    for p in packages['packages']:
        ids.add(p['id'])
        for b in p['batches']:
            owner[b] = p['id']
    best = {}
    for pr in prs:
        r = o_parse(pr['headRefName'])
        if r and (owner.get(r[1].upper()) == r[0] or (r[0] in ids and o_follow(r[1]))) and (r[0] not in best or pr['number'] < best[r[0]][2]):
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
    """The user's own claim's branch, else the current branch if it is a claim branch of a batch or of the follow-ups
    of pid, else '<pid>-<batch>-<me>'."""
    if pid in claims and claims[pid][0].lower() == me.lower():
        return claims[pid][1]
    m = o_parse(branch or '')
    if m and m[0] == pid and (m[1].upper() in pbatches or o_follow(m[1])):
        return branch
    return pid + '-' + batch + '-' + me


def o_claim_line(skip, pid, batch, b, repo, here=False):
    """show's claim line; `here` when the current branch is already the claim branch b."""
    head = 'Claims unknown: check the open pull requests, then claim' if skip else 'Unclaimed: claim'
    where = ' it on this branch: `' if here else ' it in the checkout the session will work in, from an up-to-date master: `git switch -c ' + b + ' && '
    return (head + where + 'git commit --allow-empty -m \'' + pid + ': claim ' + batch + '\' && CP_USER=1 git push -u origin ' + b
            + ' && gh pr create --draft --fill --repo ' + repo + '`.')


def o_gaps_line(b, last, repo, n=None):
    """gaps.py's contributor role line; n is the number of the user's own open claim on the package."""
    return ('role contributor: step 7 commits on branch ' + b + ' and stops before landing; repository ' + repo + '; '
            + ('pull request #' + str(n) + '; ' if n else '')
            + ('last open batch: mark the pull request ready if verified' if last else 'more batches follow: the pull request stays a draft'))


FACTS_ORDER = ('Ledger', 'Review record', 'Undecided limits', 'Follow-ups', 'Undecided optimization', 'Mutation records', 'Agent-made',
               'Open sweep', 'Recorded limits', 'Comparison record', 'Packages filed', 'TODO markers', 'Branch and', 'Other worktrees',
               'Brief')  # most needed first (SUP33; limits and follow-ups 2026-11-26)
MIN_ROOM, FLOOR, CAP_LINES = 2500, 1500, 12  # validator minimum; printed floor; lines per section (SUP33)
NO_SKILL = 10000  # o_room(''): 5,000 tokens x 2.0 characters of output
ROW_POOLS = {'id': (['A1', 'x', '9z', 'D-3', 'a.b_c', 'P250-SUP17', 'AÄ'], ['', '-a', '_a', '.a', 'a b', 'a/b', 'Ä1', '#1']),
             'kind': (['defect', 'decision'], ['Defect', 'DECISION', 'bug', 'decisions', '']),
             'text': (['f.hpp:3', 'gap text', 'review', 'x', '01-Core/10-x.hpp:12'], ['', '  ']),
             'open': ['open', 'OPEN', 'Open since step 3', 'ask', 'ASK the user', 'ask\tnow', 'open:', 'ask,later'],
             'closed': ['fixed', 'Fixed in abc1234', 'answered', 'answered 2026-11-22 (was: open, asked)', 'FIXED', 'fixed.', 'answered:yes'],
             'bad': ['', ' ', 'done', 'closed', 'dropped', 'opened', 'asked', 'fixes', 'reopen', '(open)', ': open']}


def o_rank(title):
    return next((i for i, p in enumerate(FACTS_ORDER) if title.startswith(p)), len(FACTS_ORDER))


def o_room(text):
    """Characters each '!`' line of a skill may print (user decision 2026-11-24): 5,000 tokens less the text at 2.5
    characters a token, at 2.0 characters a token of output, shared equally by its '!`' lines (SUP34)."""
    bangs = sum(line.startswith('!`') for line in text.split('\n'))
    return int(((5000 - len(text) / 2.5) * 2.0) // max(1, bangs))


def o_skill(room, bangs=1):
    """A skill text of '!`' lines and padding whose o_room is exactly `room` (room * bangs even)."""
    n = round(2.5 * (5000 - room * bangs / 2.0))
    head = ''.join(f'!`x{i}`\n' for i in range(bangs))
    return head + 's' * (n - len(head))


def o_reattach(text, name='x'):
    """03-consistency.py's skill_reattach rule (plan.reattach): a skill with a line starting '!`' gives each such line
    at least 2,500 characters; any other fits in nine tenths of 12,500 (5,000 tokens at 2.5); an unpiped '!`' line
    running `plan.py show` passes --skill and the skill's own name."""
    lines = [line for line in text.split('\n') if line.startswith('!`')]
    flagged = o_room(text) < MIN_ROOM if lines else len(text) > 11250
    return flagged or any(re.search(r'plan\.py"? show\b', l) and '|' not in l and f'--skill {name} ' not in l + ' ' for l in lines)


def o_spans(numbers):
    """'a-b' runs of consecutive numbers joined by ', ', the first six runs, then ' ...' (grouped by number minus index)."""
    groups = {}
    for k, n in enumerate(numbers):
        groups.setdefault(n - k, []).append(n)
    runs = [f'{g[0]}-{g[-1]}' if len(g) > 1 else str(g[0]) for g in groups.values()]
    return ', '.join(runs[:6]) + (' ...' if len(runs) > 6 else '')


def o_fit(lines, size, rest, keep):
    """plan.fit's contract: everything when it fits; else the first line, then the kept ones, then the others, each taken
    whole while it fits in size less len(rest) less 140, a line counting its characters and newline, at least 8; then, in the same order, each first or kept line not taken
    while at least 160 characters are left, cut to that room less one (its last two characters ' …'); printed in order,
    then '... k of n lines cut or not printed (lines <spans>): rest'."""
    c = lambda x: max(len(x) + 1, 8)  # a very short line still takes its tokens
    if sum(map(c, lines)) <= size:
        return list(lines)
    left, take = size - len(rest) - 140, {}
    first = [0] * bool(lines) + [i for i in range(1, len(lines)) if keep(lines[i])]
    for i in first + [i for i in range(1, len(lines)) if not keep(lines[i])]:
        if c(lines[i]) <= left:
            take[i] = lines[i]
            left -= c(lines[i])
    for i in first:
        if i not in take and left >= 160:
            take[i] = lines[i][:left - 3] + ' …'
            left = 0  # the cut line fills the room
    cut = [i + 1 for i in range(len(lines)) if take.get(i) != lines[i]]
    return [take[i] for i in sorted(take)] + [f'... {len(cut)} of {len(lines)} lines cut or not printed (lines {o_spans(cut)}): {rest}']


def rand_ledger_line(rng):
    """(line, class) of a random ledger line, its class 'open', 'closed', 'stray' or 'blank' known by construction:
    a row is exactly six non-empty fields, a valid id, kind defect or decision, a state whose leading letters are a state."""
    r = rng.random()
    if r < 0.1:
        return rng.choice(['', '  ', '\t']), 'blank'
    if r < 0.2:
        return rng.choice(['note: tried X, 3% slower', 'see 01-Core/x.hpp', '- [ ] todo', 'open']), 'stray'
    good, fields = True, []
    for pool in ('id', 'kind', 'text', 'text', 'text'):
        ok = rng.random() < (0.85 if pool != 'text' else 0.92)
        fields.append(rng.choice(ROW_POOLS[pool][0 if ok else 1]))
        good &= ok
    cls = rng.choices(['open', 'closed', 'bad'], [4, 4, 2])[0]
    fields.append(rng.choice(ROW_POOLS[cls]))
    n = rng.choices([6, 5, 7], [8, 1, 1])[0]
    if n == 5:
        del fields[rng.randrange(6)]
    elif n == 7:
        fields.insert(rng.randrange(7), 'extra')
    line = rng.choice(['', ' ', '  ']) + rng.choice(['|', ' | ', ' |', '|  ']).join(fields) + rng.choice(['', ' ', '\t'])
    return line, cls if good and n == 6 and cls != 'bad' else 'stray'


@contextlib.contextmanager
def real_facts():
    """Around a gaps.py run on the real repository: one such run at a time across concurrent testers (stress mode), and
    afterwards the facts files it wrote there removed, those that existed before restored (SUP33)."""
    finish = ROOT / '.claude/finish'
    with open(Path(tempfile.gettempdir()) / 'plan-tester-real-gaps.lock', 'w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        had = finish.is_dir()
        before = {f: f.read_bytes() for f in finish.glob('*.facts.txt')}
        try:
            yield
        finally:
            for f in finish.glob('*.facts.txt'):
                if f not in before:
                    f.unlink(missing_ok=True)
            for f, data in before.items():
                if not f.is_file() or f.read_bytes() != data:
                    f.write_bytes(data)
            if not had and finish.is_dir() and not any(finish.iterdir()):
                finish.rmdir()


def sections_of(text):
    """[(title, lines)] of the '### ' sections after the head of gaps.py's printed output or facts file."""
    return [(c.split('\n')[0], c.rstrip('\n').split('\n')[1:]) for c in text.split('\n\n### ')[1:]]


def o_ready(batches, packages, skip):
    """The first verified package with follow-ups whose prerequisites are verified, else the first such open package."""
    status = {p['id']: p['status'] for p in packages['packages']}
    deps = o_deps(batches, packages)
    for verified in (True, False):
        for p in packages['packages']:
            if ((p['status'] == 'verified') == verified and (not verified or p.get('followups')) and p['id'] not in skip
                    and all(status[d] == 'verified' for d in deps[p['id']])):
                return p['id']
    return None


def o_ready_rows(batches, packages):
    """[(id, folders)] of the verified packages with follow-ups, then the open packages, whose prerequisites are verified, in order."""
    bmap = {b['id']: b for b in batches['batches']}
    status, deps = {p['id']: p['status'] for p in packages['packages']}, o_deps(batches, packages)
    rows = []
    for p in [q for q in packages['packages'] if q['status'] == 'verified'] + [q for q in packages['packages'] if q['status'] != 'verified']:
        if (p['status'] == 'verified' and not p.get('followups')) or any(status[d] != 'verified' for d in deps[p['id']]):
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
        if kind < 0.08:  # a follow-up claim (user decision 2026-11-26), well formed or not
            branch = f'{p["id"]}-{rng.choice(["FU", "fu", "Fu", "FUX", "F"])}{rng.choice(["261126", "1", ""])}-{user}'
        elif kind < 0.6:
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
                     'model': rng.choice(['opus', 'opus', 'fable'])} | ({'followups': ['f']} if rng.random() < 0.3 else {}))
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
                'packages': [{'id': i, 'label': 'L' + i, 'batches': bs, 'phase': 'Contest Core', 'size': 'M', 'model': m,
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
                                            ('SUP17', 'x/P250-SUP17-x', {}, 'alice', 'P250-SUP17-alice'),
                                            ('FU261126', 'P250-fu261125-x', {}, 'alice', 'P250-fu261125-x'),  # follow-up claims (2026-11-26)
                                            ('FU261126', 'P250-FUX1-x', {}, 'alice', 'P250-FU261126-alice'),
                                            ('FU261126', 'P250-FU-x', {}, 'alice', 'P250-FU261126-alice'),
                                            ('SUP17', 'P250-FU1-x', {}, 'alice', 'P250-FU1-x')):
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
               pr(16, 'P999-C04-y', 'y'), pr(17, 'P018-fu261126-zed', 'zed'), pr(18, 'P999-FU1-y', 'y'), pr(19, 'P020-FUX2-y', 'y'),
               pr(20, 'P020-FU-y', 'y')]
        self.prs(prs)
        want = {'P250': ('carol', 'P250-sup17-carol', 4, True), 'P031': ('Dave', 'P031-ds27-a-b/c', 10, False),
                'P018': ('zed', 'P018-fu261126-zed', 17, False)}
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


    def test_render_markers(self):
        """The checklist boxes (user decision 2026-11-26): `[ ]` planned, `[-]` in progress, `[x]` verified, `[!]` verified with
        open follow-ups, which every package lists after its other text."""
        pk = lambda i, s, fu=None, note='': ({'id': i, 'batches': ['B1'], 'model': 'opus', 'size': 'S', 'label': 'L', 'phase': 'Contest Core',
                                              'status': s, 'note': note, 'evidence': []} | ({'followups': fu} if fu else {}))
        text = PLAN.render_text({'packages': [pk('P001', 'planned'), pk('P002', 'in-progress', ['x'], 'n'), pk('P003', 'verified'),
                                              pk('P004', 'verified', ['a', 'b; c'])]})
        self.assertEqual([l for l in text.split('\n') if l.startswith('- ')], [
            '- [ ] **P001 — B1** (opus, S): L. Status: planned.', '- [-] **P002 — B1** (opus, S): L. Status: in-progress. Note: n Follow-ups: (1) x.',
            '- [x] **P003 — B1** (opus, S): L.', '- [!] **P004 — B1** (opus, S): L. Follow-ups: (1) a; (2) b; c.'])
        self.assertIn('`[ ]` planned, `[-]` in progress, `[x]` verified, `[!]` verified with open follow-ups', text)

    def test_undecided(self):
        """plan.undecided (user decision 2026-11-26): the items of the level-2 and level-3 headings naming Limits or a
        Handoff, each a list item or a paragraph with its continuation lines, or a table body row, that name no
        `user decision <date>`; `none` items and table headers are skipped; a section ends at the next heading of its level
        or above, while a deeper heading only ends the item before it; an indented item continues its parent, and an item naming
        the follow-ups it filed (`follow-up on Pxxx`) is decided too (review of 2026-11-26) while each of those packages has an
        open follow-up (user decision 2026-11-27), limit_line marking each that has none; a missing file gives none."""
        doc = self.tmp / 'Docs/01-x.md'
        doc.parent.mkdir()
        text = ('# Doc\n## Contracts\n- c\n## Limits and Handoffs\n- open one\n  continued here\n- decided (user decision 2026-11-26)\n'
                '- also decided, user decisions 2026-11-17 and 2026-11-18\n* star item\n1. numbered\n   more\n\nA paragraph\nsecond line of it\n\n'
                '### Sub detail\n- nested\n| A | B |\n|---|---|\n| row | x |\n| row2 user decision 2026-01-02 | y |\n- None.\n'
                '- user decision (no date)\n## History\n- h\n### Dependent-Owner Handoff\n- handoff item\n#### deep\n- deep item\n'
                '## Limits\n- none\n2) n\n- handed on (follow-up on P031)\n- parent (user decision 2026-11-26):\n  - child a;\n  - child b.\n'
                '- open two\n    - its child\n- two hands (follow-up on P031, follow-up on P040)\n'
                '- decided anyway (follow-up on P040; user decision 2026-11-27)\n- a follow-up on P1234 is no hand-off\n'
                '## Open questions\n- not a limits heading\n# Top Limits\n- level 1 is not\n')
        doc.write_text(text)
        lines = text.split('\n')
        at = lambda t: lines.index(t) + 1
        base = [(at('- open one'), '- open one continued here'), (at('* star item'), '* star item'), (at('1. numbered'), '1. numbered more'),
                (at('A paragraph'), 'A paragraph second line of it'), (at('- nested'), '- nested'), (at('| row | x |'), '| row | x |'),
                (at('- user decision (no date)'), '- user decision (no date)'), (at('- handoff item'), '- handoff item'),
                (at('- deep item'), '- deep item'), (at('2) n'), '2) n'), (at('- open two'), '- open two - its child')]
        nohand = (at('- a follow-up on P1234 is no hand-off'), '- a follow-up on P1234 is no hand-off')
        one, two = [(at(t), t) for t in ('- handed on (follow-up on P031)', '- two hands (follow-up on P031, follow-up on P040)')]
        key = lambda item: item[0]
        for handed, want in (({'P031', 'P040'}, base + [nohand]), ({'P031'}, base + [two, nohand]), ({'P040', 'P900'}, base + [one, two, nohand]),
                             (set(), base + [one, two, nohand])):
            self.assertEqual(PLAN.undecided(doc, handed), sorted(want, key=key), handed)
        self.assertEqual(PLAN.undecided(self.tmp / 'none.md', set()), [])
        self.assertEqual(PLAN.limit_line('d.md', 7, two[1], {'P040'}), f'd.md:7: [no open follow-up on P031] {two[1]}')
        self.assertEqual(PLAN.limit_line('d.md', 7, two[1], set()), f'd.md:7: [no open follow-up on P031] [no open follow-up on P040] {two[1]}')
        self.assertEqual(PLAN.limit_line('d.md', 7, 'x' * 200, set()), 'd.md:7: ' + 'x' * 140)
        self.assertEqual(PLAN.open_ids([{'id': 'P1'}, {'id': 'P2', 'followups': ['a']}, {'id': 'P3', 'followups': []}]), {'P2'})


class Facts(Base):
    """gaps.py's ledger rows, section, emit and limits as functions of the imported module (SUP33), and plan.py's room
    arithmetic, fit and the validator's skill_reattach rule (SUP34); expected values written here."""
    def test_gaps_row(self):
        for line, want in (('A1 | defect | f.hpp:3 | gap | review | open', 'open'), ('A1|decision|f:1|g|s|ASK the user', 'ask'),
                           ('  x.y-z_2 | defect | a:1 | g | s | Fixed in abc1234  ', 'fixed'), ('9 | defect | a:1 | g | s | OPEN', 'open'),
                           ('D3 | decision | a:1 | g | s | answered 2026-11-22 (was: open, asked)', 'answered'),
                           ('AÄ | defect | a:1 | g | s | open\tsince step 3', 'open'), ('a-.b | decision | a | g | s | Answered', 'answered'),
                           ('A1 | defect | f:3 | gap | open', None), ('A1 | defect | f:3 | gap | s | open | x', None),
                           ('| A1 | defect | f:3 | gap | s | open |', None), ('A1 | defect |  | gap | s | open', None),
                           ('A1 | defect | f | g | s |   ', None), (' | defect | f | g | s | open', None), ('A1 | defect | f | g |  | open', None),
                           ('A1 | Defect | f | g | s | open', None), ('A1 | bug | f | g | s | open', None), ('A1 | decisions | f | g | s | open', None),
                           ('-A1 | defect | f | g | s | open', None), ('_A1 | defect | f | g | s | open', None), ('.a | defect | f | g | s | open', None),
                           ('A 1 | defect | f | g | s | open', None), ('A/1 | defect | f | g | s | open', None), ('Ä1 | defect | f | g | s | open', None),
                           ('A1 | defect | f | g | s | done', None), ('A1 | defect | f | g | s | closed', None), ('A1 | defect | f | g | s | dropped', None),
                           ('A1 | defect | f | g | s | opened', None), ('A1 | defect | f | g | s | open: now', 'open'), ('A1 | defect | f | g | s | fixed.', 'fixed'),
                           ('A1 | defect | f | g | s | ask,later', 'ask'), ('A1 | defect | f | g | s | fixes', None), ('A1 | defect | f | g | s | : open', None), ('A1 | defect | f | g | s | reopen', None),
                           ('A1 | defect | f | g | s | (open)', None), ('', None), ('   ', None), ('plain note', None), ('open', None)):
            self.assertEqual(GAPS.row(line), want, repr(line))
        text = ('A1 | defect | f:1 | g | s | open\n\nnote: tried X, 3% slower\n   \n'
                '  A2 | decision | f:2 | g | s | answered 2026-11-22 (was: open)  \nA3 | defect | f:3 | g | s | done\n'
                'A4 | decision | f:4 | g | s | Ask the user\r\nA5 | defect | f:5 | g | s | fixed\n')
        self.assertEqual(GAPS.ledger_rows(text), (['A1 | defect | f:1 | g | s | open', 'A4 | decision | f:4 | g | s | Ask the user'],
                                                  ['A2 | decision | f:2 | g | s | answered 2026-11-22 (was: open)', 'A5 | defect | f:5 | g | s | fixed'],
                                                  [3, 6]), 'blank lines are neither rows nor stray; a done row is stray')
        self.assertEqual(GAPS.ledger_rows(''), ([], [], []))
        self.assertEqual(GAPS.ledger_rows('\n \n\t\n'), ([], [], []))

    def test_gaps_ledger_random(self):
        rng = random.Random(SEED + 7)
        for r in range(40 if MODE != 'stress' else ROUNDS):
            lines = [rand_ledger_line(rng) for _ in range(rng.randint(0, 30))]
            want = ([l.strip() for l, c in lines if c == 'open'], [l.strip() for l, c in lines if c == 'closed'],
                    [i for i, (_, c) in enumerate(lines, 1) if c == 'stray'])
            self.assertEqual(GAPS.ledger_rows('\n'.join(l for l, _ in lines)), want, self.fail_info(round=r, lines=json.dumps(lines)))

    def test_gaps_facts_room(self):
        """plan.py's room arithmetic and rule (SUP34): the constants, room() and room_of() against o_room at their edges,
        reattach() on synthetic skills at both edges of both branches and of the shared room, the brief's --skill check,
        and the real skills flagging none."""
        self.assertEqual((PLAN.REATTACH_TOKENS, PLAN.SKILL_CHARS_PER_TOKEN, PLAN.OUTPUT_CHARS_PER_TOKEN, PLAN.MIN_FACTS, PLAN.FLOOR, GAPS.LINES),
                         (5000, 2.5, 2.0, MIN_ROOM, FLOOR, CAP_LINES))
        self.assertFalse(any(hasattr(GAPS, n) for n in ('REATTACH_CHARS', 'CHARS_PER_TOKEN', 'MIN_FACTS', 'facts_room')), 'one place: plan.py')
        for n in (0, 1, 9283, 9375, 9376, 12500, 20000):
            for bangs in (0, 1, 2, 3):
                text = '!`x`\n' * bangs + 'x' * n
                self.assertEqual(PLAN.room(text), o_room(text), (n, bangs))
        self.assertEqual((o_room(''), o_room('x' * 9375), o_room('x' * 9376), o_room('!`a`\n!`b`\n' + 'x' * 6240)), (NO_SKILL, 2500, 2499, 2500))
        md = self.tmp / 'SKILL.md'
        self.assertEqual(PLAN.room_of(md), NO_SKILL, 'no SKILL.md: an empty one')
        for text in ('x' * 7500, 'x' * 10625, 'x' * 10626, 'x' * 30000):
            md.write_text(text)
            self.assertEqual(PLAN.room_of(md), max(o_room(text), FLOOR), len(text))
        skills = self.tmp / 'r/.claude/skills'
        pipe = '!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" show $ARGUMENTS | head -1`\n'
        own = lambda name: f'!`python3 "${{CLAUDE_PROJECT_DIR}}/00-Guidelines/13-Plan/plan.py" show --skill {name} $ARGUMENTS`\n'
        cases = {'one-ok': o_skill(2500), 'one-low': o_skill(2499), 'two-ok': o_skill(2500, 2), 'two-low': o_skill(2499, 2),
                 'plain-ok': 'a' * 11250, 'plain-big': 'a' * 11251, 'inline': 'see !`x`\n' + 'a' * 11242,
                 'late': 'x\n' + o_skill(2499)[:-2], 'brief': own('brief') + 'x', 'brief-bare': own('brief').replace('--skill brief ', '') + 'x',
                 'brief-other': own('package') + 'x', 'brief-prefix': own('brief-x').replace('brief-x', 'brief-xyz') + 'x', 'brief-pipe': pipe}
        for name, text in cases.items():
            (skills / name).mkdir(parents=True, exist_ok=True)
            (skills / name / 'SKILL.md').write_text(text)
        got = PLAN.reattach(self.tmp / 'r')
        want = sorted(n for n, t in cases.items() if o_reattach(t, n))
        self.assertEqual(sorted({w.split('/')[2] for w in got}), want, got)
        self.assertEqual(want, ['brief-bare', 'brief-other', 'brief-prefix', 'inline', 'late', 'one-low', 'plain-big', 'two-low'], 'oracle self-check')
        self.assertIn('.claude/skills/two-low/SKILL.md: leaves 2499 characters for the output of each of its 2 `!` line(s) (at least 2500)', got)
        self.assertIn('.claude/skills/plain-big/SKILL.md: 11251 characters, near the 12500 re-attached after a compaction', got)
        self.assertIn('.claude/skills/brief-bare/SKILL.md: `plan.py show` not fitted to this skill: add --skill brief-bare', got)
        flagged = [str(f.relative_to(ROOT)) for f in sorted((ROOT / '.claude/skills').glob('*/SKILL.md')) if o_reattach(f.read_text(), f.parent.name)]
        self.assertEqual((flagged, PLAN.reattach(ROOT)), ([], []), "the real skills leave 03-consistency.py's skill_reattach empty")

    def test_validator_reattach(self):
        """03-consistency.py reports plan.reattach(ROOT) as skill_reattach (run on the real repository; full mode): as it is,
        and with the plan module it loads answering a probe list, so an empty list is no accident."""
        if MODE == 'quick':
            self.skipTest('full mode only')
        probe = ('import importlib.machinery as m, runpy, sys\norig = m.SourceFileLoader.exec_module\n'
                 'def run(self, mod):\n    orig(self, mod)\n    if mod.__name__ == "library_plan": mod.reattach = lambda root: ["probe " + root.name]\n'
                 'm.SourceFileLoader.exec_module = run\nsys.argv = sys.argv[1:]\nrunpy.run_path(sys.argv[0], run_name="__main__")\n')
        validator = str(ROOT / '96-Local Testing/03-consistency.py')
        for cmd, want in (([validator], PLAN.reattach(ROOT)), (['-c', probe, validator], ['probe ' + ROOT.name])):
            r = REAL_SUBPROCESS_RUN([sys.executable, *cmd], cwd=ROOT, capture_output=True, text=True, timeout=600)
            out = json.JSONDecoder().raw_decode(r.stdout[r.stdout.index('{'):])[0]  # failures follow the JSON
            self.assertEqual(out['skill_reattach'], want, r.stderr[-2000:])
        with patch.object(PLAN, 'BANG', re.compile('^#', re.M)):  # premise: reattach() can flag the real skills
            self.assertNotEqual(PLAN.reattach(ROOT), [])

    def test_fit(self):
        """plan.fit and spans against o_fit and o_spans: fixed cases and seeded random line lists (SUP34)."""
        self.assertEqual(PLAN.fit([], 0, 'r'), [])
        self.assertEqual(PLAN.fit(['ab' * 4, 'c'], 17, 'r'), ['ab' * 4, 'c'], 'exactly the room: 9 + 8, a short line counting 8')
        self.assertEqual(PLAN.fit(['ab' * 4, 'c'], 16, 'r')[-1], '... 2 of 2 lines cut or not printed (lines 1-2): r', 'one short of it')
        lines = ['head'] + [f'line {i} ' + 'x' * 60 for i in range(40)] + ['WARNING keep me' + 'k' * 60, 'tail']
        out = PLAN.fit(lines, 1500, 'see X')
        self.assertEqual(out, o_fit(lines, 1500, 'see X', lambda l: False))
        self.assertEqual(out[-2:], ['tail', '... 22 of 43 lines cut or not printed (lines 21-42): see X'])
        out = PLAN.fit(lines, 1500, 'see X', lambda l: l.startswith('WARNING'))
        self.assertEqual(out[-3:], [lines[-2], 'tail', '... 22 of 43 lines cut or not printed (lines 20-41): see X'], 'the kept line before others')
        self.assertEqual(PLAN.fit(['h' * 2000, 'a', 'b'], 1500, 'r'), ['h' * 1340 + ' …', 'a', 'b', '... 1 of 3 lines cut or not printed (lines 1): r'],
                         'a long first line is cut to the room the others leave')
        self.assertEqual(PLAN.fit(['h' * 2000, 'K' * 900, 'a'], 1219, 'r', lambda l: l[0] == 'K'), ['h' * 166 + ' …', 'K' * 900, 'a',
                         '... 1 of 3 lines cut or not printed (lines 1): r'], '1219 - 1 - 140 - 901 - 8 = 169 left: cut to 168 with the newline')
        self.assertEqual(PLAN.fit(['h' * 2000, 'K' * 900, 'a'], 1200, 'r', lambda l: l[0] == 'K'), ['K' * 900, 'a',
                         '... 1 of 3 lines cut or not printed (lines 1): r'], 'under 160 left: not printed')
        self.assertEqual(PLAN.spans([1, 2, 3, 5, 7, 8]), '1-3, 5, 7-8')
        self.assertEqual(PLAN.spans([1, 3, 5, 7, 9, 11, 13]), '1, 3, 5, 7, 9, 11 ...')
        rng = random.Random(SEED + 11)
        for r in range(200 if MODE != 'stress' else 10 * ROUNDS):
            lines = [rng.choice(['WARNING ', 'STOP ', '', '- ']) + 'y' * rng.choice([0, 1, 5, 40, 157, 158, 200, 900, 3000]) for _ in range(rng.randint(0, 60))]
            lines += [''] * rng.choice([0, 0, 300])  # many blank lines: each counts 8
            size, rest = rng.choice([1500, 1501, 2000, 3187, 9229, 10 ** 5, sum(max(len(x) + 1, 8) for x in lines)]), 'z' * rng.randint(0, 150)  # the last: exactly all
            keep = lambda l: l.startswith(('WARNING', 'STOP'))
            got = PLAN.fit(lines, size, rest, keep)
            self.assertEqual(got, o_fit(lines, size, rest, keep), self.fail_info(round=r, size=size, lines=json.dumps(lines)))
            self.assertLessEqual(sum(max(len(x) + 1, 8) for x in got), size, self.fail_info(round=r, size=size))

    def test_fit_spill(self):
        """plan.spill: <name>.txt in the session's scratchpad under the temp directory's claude-<uid>, else in claude-<uid>
        itself; None when unwritable."""
        base = self.tmp / f'claude-{os.getuid()}'
        pad = base / '-proj' / 'S-1'
        pad.mkdir(parents=True)
        with patch.object(tempfile, 'tempdir', str(self.tmp)):
            for sid, where in (('S-1', pad), ('', base), ('other', base), ('../x', base)):
                with patch.dict(os.environ, {'CLAUDE_CODE_SESSION_ID': sid}):
                    self.assertEqual(PLAN.spill('n', 'text\n'), where / 'n.txt', sid)
                    self.assertEqual((where / 'n.txt').read_text(), 'text\n')
            (pad / 'n.txt').unlink()
            (pad / 'n.txt').mkdir()
            with patch.dict(os.environ, {'CLAUDE_CODE_SESSION_ID': 'S-1'}):
                self.assertIsNone(PLAN.spill('n', 'x'))

    def test_gaps_limits(self):
        """gaps.limits: each heading LIMIT_HEADING matches with its path and line, then every non-blank line of its section
        in full up to the next heading of its level or above; deeper headings stay inside; a missing file gives none."""
        doc = self.tmp / 'Docs/01-x.md'
        doc.parent.mkdir()
        long = '- first ' + 'L' * 500
        doc.write_text(f'# Doc\n## Contracts\n- c\n## Limits\n{long}\n  continued\n- second\n\n### Sub detail\n- nested\n## History\n'
                       f'- h\n## Known gaps\n- g1\n### Open questions\n- q\n# Top\n- t\n## {"Limit " * 40}\n')
        self.assertEqual(GAPS.limits(doc, self.tmp), ['Docs/01-x.md:4: ## Limits', long, '  continued', '- second', '### Sub detail', '- nested',
                                                      'Docs/01-x.md:13: ## Known gaps', '- g1', '### Open questions', '- q',
                                                      'Docs/01-x.md:19: ' + ('## ' + 'Limit ' * 40).strip()[:160]])
        self.assertEqual(GAPS.limits(self.tmp / 'none.md', self.tmp), [])

    def emit(self, sections, skill=None, key='K', root=None):
        """gaps.emit's printed text for the given sections, with SKILL.md of the given text beside a stand-in gaps.py
        (None: no SKILL.md); the facts file goes under root (default tmp/root)."""
        d = self.tmp / 'skill'
        d.mkdir(exist_ok=True)
        (d / 'SKILL.md').unlink(missing_ok=True)
        if skill is not None:
            (d / 'SKILL.md').write_text(skill)
        GAPS.SECTIONS[:] = sections
        self.addCleanup(GAPS.SECTIONS.clear)
        buf = io.StringIO()
        with patch.object(GAPS, '__file__', str(d / 'gaps.py')), contextlib.redirect_stdout(buf):
            GAPS.emit(['### Target\nkey K'], key, root or self.tmp / 'root')
        return buf.getvalue()

    def test_gaps_section(self):
        GAPS.SECTIONS.clear()
        self.addCleanup(GAPS.SECTIONS.clear)
        GAPS.section('Empty', [])
        GAPS.section('Many', [str(i) for i in range(401)])
        GAPS.section('Capped', [str(i) for i in range(5)], 3)
        self.assertEqual(GAPS.SECTIONS, [('Empty', ['(none)']), ('Many', [str(i) for i in range(400)] + ['... 1 more']),
                                         ('Capped', ['0', '1', '2', '... 2 more'])])

    def test_gaps_emit(self):
        led = 'Ledger .claude/finish/K.txt (open rows)'
        out = self.emit([('Brief', ['b1', 'b2']), (led, ['r1', '0 fixed or answered rows not shown'])])
        facts = self.tmp / 'root/.claude/finish/K.facts.txt'
        full = f'### Target\nkey K\n\n### Brief\nb1\nb2\n\n### {led}\nr1\n0 fixed or answered rows not shown\n'
        self.assertEqual(facts.read_text(), full, 'the facts file in the order the sections were added')
        self.assertFalse((self.tmp / 'root/.claude/finish/K.facts.md').exists())
        self.assertEqual(out, f'### Target\nkey K\nFull facts: .claude/finish/K.facts.txt ({len(full)} characters); read it at step 1 when '
                              f'sections below are cut or not printed.\n\n### {led}\nr1\n0 fixed or answered rows not shown\n\n### Brief\nb1\nb2\n',
                         'printed: the ledger first, the brief last')
        # LINES per section, the ledger whole while it fits
        lim = 'Recorded limits (evidence headings)'
        out = self.emit([(lim, [f'l{i}' for i in range(20)]), ('Open sweep ledger rows', [f's{i}' for i in range(12)]),
                         ('TODO markers in changed files', [f't{i}' for i in range(13)]), (led, [f'r{i}' for i in range(30)])])
        got = dict(sections_of(out))
        self.assertEqual(got[led], [f'r{i}' for i in range(30)])
        self.assertEqual(got[lim], [f'l{i}' for i in range(12)] + ['... 8 more in the facts file'])
        self.assertEqual(got['Open sweep ledger rows'], [f's{i}' for i in range(12)])
        self.assertEqual(got['TODO markers in changed files'], [f't{i}' for i in range(12)] + ['... 1 more in the facts file'])
        self.assertEqual(dict(sections_of(facts.read_text()))[lim], [f'l{i}' for i in range(20)], 'the facts file keeps every line')
        # ORDER: titles by prefix, unknown ones after the brief, ties in the order added
        titles = ['Zeta unknown', *(p + ' t' for p in reversed(FACTS_ORDER)), 'Alpha unknown']
        out = self.emit([(t, ['x']) for t in titles])
        self.assertEqual([t for t, _ in sections_of(out)], [p + ' t' for p in FACTS_ORDER] + ['Zeta unknown', 'Alpha unknown'])
        self.assertEqual([t for t, _ in sections_of(facts.read_text())], titles)

    def test_gaps_emit_room(self):
        """Printed text within plan.room_of(SKILL.md) (floor 1500; a line counted at least 8, a section of short lines included), the ledger's open rows as many as fit, sections that do
        not fit named under 'Not printed'; the facts file whole."""
        led = 'Ledger .claude/finish/K.txt (open rows)'
        rows = [f'D{i:03} | defect | 01-Core/10-x.hpp:{i:03} | gap {"g" * 20} | review | open' for i in range(100)]
        others = ['Brief', 'Recorded limits (evidence headings)', 'Agent-made omissions and deferrals (notes lines without a user decision)',
                  'Undecided limits (no `user decision <date>`)', 'Follow-ups of P123',
                  'Review record (/package step 10)', 'Undecided optimization candidates (Core full evidence)',
                  'Comparison record (05-testing.md, Stale evidence)', 'Packages filed by P123', 'Open sweep ledger rows',
                  'Branch and uncommitted changes', 'Mutation records (04-mutation.py check on changed headers; /package step 7)',
                  'TODO markers in changed files', 'Other worktrees touching the same files (committed work; worktree.py --finish adds their uncommitted edits)']
        sections = [(led, rows + ['3 fixed or answered rows not shown'])] + [(t, ['t'] * 20 if t.startswith('TODO') else ['x' * 50]) for t in others]
        facts = self.tmp / 'root/.claude/finish/K.facts.txt'
        for skill, room in ((None, NO_SKILL), (o_skill(4000), 4000), (o_skill(2000), 2000), (o_skill(FLOOR), FLOOR), ('s' * 20000, FLOOR)):
            out = self.emit(sections, skill)
            info = f'SKILL.md {len(skill or "")} characters, room {room}'
            self.assertLessEqual(sum(max(len(x) + 1, 8) for x in out.split('\n')), room, info + ' (a short line counts 8)')
            self.assertEqual([t for t, _ in sections_of(facts.read_text())], [led, *others], info)
            self.assertEqual(dict(sections_of(facts.read_text()))[led], rows + ['3 fixed or answered rows not shown'], info)
            got = sections_of(out)
            self.assertEqual(got[0][0], led, info)
            shown = got[0][1]
            k = len(shown) - 1 if shown[-1].endswith(' more in the facts file') else len(shown)
            self.assertGreater(k, 0, info)
            self.assertEqual(shown[:k], (rows + ['3 fixed or answered rows not shown'])[:k], info)
            if k < 101:
                self.assertEqual(shown[k:], [f'... {101 - k} more in the facts file'], info)
            printed = [t for t, _ in got[1:] if t != 'Not printed (in the facts file)']
            skipped = [t for t in sorted(others, key=o_rank) if t not in printed]
            self.assertEqual(printed, [t for t in sorted(others, key=o_rank) if t in printed], info)
            if skipped:
                self.assertEqual(got[-1], ('Not printed (in the facts file)', skipped), info)
            else:
                self.assertNotIn('### Not printed', out, info)
            if room == NO_SKILL:
                self.assertEqual((k, skipped), (101, []), 'without SKILL.md the whole 10,000 characters: everything fits')
            if room == 4000:
                self.assertLess(k, 101, 'a room of 4,000 cuts the ledger')
        short = [(led, rows[:3])] + [(t, ['y' * 16] * 11) for t in others]  # many blank separators: each line pays the floor
        for skill, room in ((o_skill(2600), 2600), (o_skill(2000), 2000), (o_skill(FLOOR), FLOOR)):
            out = self.emit(short, skill)
            self.assertLessEqual(sum(max(len(x) + 1, 8) for x in out.split('\n')), room, f'short sections, room {room}')

    def test_gaps_emit_unwritable(self):
        root = self.tmp / 'ro'
        (root / '.claude').mkdir(parents=True)
        (root / '.claude/finish').write_text('a file in the way\n')
        out = self.emit([('Brief', ['b'])], root=root)
        self.assertEqual(out.split('\n')[2], 'Full facts: not written (File exists) (30 characters); read it at step 1 when sections '
                                             'below are cut or not printed.')
        self.assertEqual(sections_of(out), [('Brief', ['b'])], 'printed anyway')
        (root / '.claude/finish').unlink()
        (root / '.claude/finish/K.facts.txt').mkdir(parents=True)
        out = self.emit([('Brief', ['b'])], root=root)
        self.assertTrue(out.split('\n')[2].startswith('Full facts: not written (Is a directory) (30 characters);'), out)


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

    def test_syn_show_hook_warning(self):
        last = lambda cfg=None: self.show_cfg('P030', cfg)[-1]
        self.assertFalse(last().startswith('WARNING: commit hook'), 'no setup_check.py in the checkout')
        script = self.root / '.claude/skills/setup-check/setup_check.py'
        script.parent.mkdir(parents=True)
        shutil.copy(ROOT / '.claude/skills/setup-check/setup_check.py', script)
        want = str(self.root / '.claude/githooks')
        tail = f'; run: git config core.hooksPath {shlex.quote(want)}). Give the user that command; this warning does not stop the run.'
        self.assertEqual(last(), 'WARNING: commit hook off (not enabled' + tail)
        self.assertEqual(last({'core.hooksPath': '.claude/githooks'}), "WARNING: commit hook off (core.hooksPath is '.claude/githooks'" + tail)
        self.assertFalse(last({'core.hooksPath': want}).startswith('WARNING'), 'enabled')

    def test_syn_show_skill(self):
        """plan.py show --skill <name> (SUP34): the brief fitted to the room of that skill's SKILL.md (o_fit, its first,
        WARNING, STOP and Focus lines first, the pointer naming `plan.py show <id>`), whole when it fits or without --skill, an
        error for an unknown skill."""
        f = self.root / '00-Guidelines/13-Plan/packages.json'
        data = json.loads(f.read_text())
        pkg = next(p for p in data['packages'] if p['id'] == 'P020')
        pkg['note'] = 'long note ' + 'n' * 3000
        f.write_text(json.dumps(data))
        bf = self.root / '00-Guidelines/13-Plan/batches.json'
        bd = json.loads(bf.read_text())
        for b in bd['batches']:
            if b['id'] in pkg['batches']:
                b['focus'] = 'long focus ' + 'f' * 2500  # too long for every room below: cut, never dropped
        bf.write_text(json.dumps(bd))
        script = self.root / '.claude/skills/setup-check/setup_check.py'  # the commit-hook WARNING comes last
        script.parent.mkdir(parents=True)
        shutil.copy(ROOT / '.claude/skills/setup-check/setup_check.py', script)
        skill = self.root / '.claude/skills/package/SKILL.md'
        skill.parent.mkdir(parents=True)
        whole = self.run_py(self.cli, 'show', 'P020', cwd=self.root, role='contributor')
        lines = whole.stdout.rstrip('\n').split('\n')
        self.assertTrue(lines[2].startswith('STOP') and lines[-1].startswith('WARNING') and len(whole.stdout) > 3000, whole.stdout[:300])
        rest = "run `python3 '00-Guidelines/13-Plan/plan.py' show P020` for the whole brief before acting on it"
        for text, size in (('x' * 20000, FLOOR), (o_skill(2000), 2000), ('short\n', NO_SKILL)):
            skill.write_text(text)
            r = self.run_py(self.cli, 'show', '--skill', 'package', 'P020', cwd=self.root, role='contributor')
            self.assertEqual((r.returncode, r.stderr), (0, ''))
            self.assertEqual(r.stdout.rstrip('\n').split('\n'), o_fit(lines, size, rest, lambda l: l.startswith(('WARNING', 'STOP', 'Focus: '))), size)
            self.assertTrue(any(l.startswith('Focus: ') for l in r.stdout.split('\n')), 'the Focus line, whole or cut')
            self.assertLessEqual(len(r.stdout), size)
            if size < len(whole.stdout):
                self.assertTrue(all(l in r.stdout for l in (lines[0], lines[2], lines[-1])), r.stdout)
            else:
                self.assertEqual(r.stdout, whole.stdout)
        r = self.run_py(self.cli, 'show', '--skill', 'nosuch', 'P020', cwd=self.root, role='contributor')
        self.assertEqual((r.returncode, r.stdout, r.stderr), (1, '', "No skill 'nosuch'\n"))

    def test_items_room(self):
        """.claude/skills/prune/items.py (SUP34) on a copy beside a synthetic duplicates.md: the package's section and the
        Recorded Decisions naming it, whole when they fit, else fitted to the room of prune's SKILL.md with the section
        heading and the decisions heading first and a pointer to duplicates.md; the real script within the real room."""
        tree = self.tmp / 'it'
        items = tree / '.claude/skills/prune/items.py'
        for src, dst in ((ROOT / '.claude/skills/prune/items.py', items), (PLAN_PY, tree / '00-Guidelines/13-Plan/plan.py')):
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(src, dst)
        rows = [f'| {i} | `f{i}` | remove | ' + 'r' * 80 + ' |' for i in range(40)]
        (tree / '00-Guidelines/23-Sweep Ledgers').mkdir(parents=True)
        (tree / '00-Guidelines/23-Sweep Ledgers/duplicates.md').write_text(
            '# Duplicates\n\n## P123 Label\n\n' + '\n'.join(rows) + '\n\n## P124 Other\n\n- x\n\n## Recorded Decisions\n\n'
            '- P123 keeps `a`\n  because b\n- P999 keeps c\n| P123 | row |\n')
        want = ['## P123 Label', '', *rows, '', 'Recorded Decisions naming P123:', '- P123 keeps `a`', '  because b', '| P123 | row |']
        run = lambda pid: REAL_SUBPROCESS_RUN([sys.executable, str(items), pid], capture_output=True, text=True, timeout=60)
        r = run('P123')
        self.assertEqual((r.returncode, r.stdout.rstrip('\n').split('\n')), (0, want), r.stderr)
        (items.parent / 'SKILL.md').write_text('x' * 20000)
        r = run('P123')
        rest = '`## P123` and `## Recorded Decisions` in 00-Guidelines/23-Sweep Ledgers/duplicates.md hold them'
        self.assertEqual(r.stdout.rstrip('\n').split('\n'), o_fit(want, FLOOR, rest, lambda l: l.startswith(('## ', 'Recorded Decisions naming'))))
        self.assertLessEqual(len(r.stdout), FLOOR)
        self.assertIn('Recorded Decisions naming P123:', r.stdout)
        self.assertEqual(run('P124').stdout, '## P124 Other\n\n- x\n\nRecorded Decisions naming P124:\nnone\n')
        ids = re.findall(r'^## (P\d{3}) ', (ROOT / '00-Guidelines/23-Sweep Ledgers/duplicates.md').read_text(), re.M)
        room = max(o_room((ROOT / '.claude/skills/prune/SKILL.md').read_text()), FLOOR)
        for pid in ids[:3] if MODE == 'quick' else ids:
            r = REAL_SUBPROCESS_RUN([sys.executable, str(ROOT / '.claude/skills/prune/items.py'), pid], capture_output=True, text=True, timeout=60)
            self.assertTrue(r.returncode == 0 and len(r.stdout) <= room, (pid, len(r.stdout), room, r.stderr))

    def show_cfg(self, ident, cfg):
        with self.environ('contributor', cfg=cfg):
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
        hint = '. One batch per session, under its batch id: run `/package SUP18`, which lands it when nothing is open, else `/clear`, then `/finish SUP18`.'
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

    def test_syn_support_targets(self):
        b = {'id': 'SUP21', 'size': 'S', 'focus': 'f', 'prerequisites': [], 'targets': [], 'support_targets': ['07-Strings/01-s.hpp']}
        text = '\n'.join(PLAN.batch_brief(self.root, b, {'07-Strings/01-s.hpp': (['op', 'verified'],)}))
        self.assertIn('- `07-Strings/01-s.hpp` [missing, support]\n  op — verified', text)
        (self.root / '07-Strings').mkdir()
        (self.root / '07-Strings/01-s.hpp').write_text('x\n')
        batches = {'batches': [b if x['id'] == 'SUP21' else x for x in SYN_BATCHES['batches']]}
        with self.environ():
            self.assertEqual(PLAN.live(self.root, SYN_BATCHES, SYN_PACKAGES), {'P250'}, 'ST01 belongs to verified P900')
            self.assertEqual(PLAN.live(self.root, batches, SYN_PACKAGES), {'P250', 'P260'}, 'a changed support target is live')

    def test_syn_follow_up_cli(self):
        """plan.py follow-up (user decision 2026-11-26): adds a one-line text (whitespace runs joined), refuses an empty or
        repeated one, an unknown package, both or neither of text and --done, and numbers outside 1..n; --done closes by
        number and drops the field with the last; each change saves packages.json and re-renders the checklist."""
        cli = lambda *a: self.run_py(self.cli, 'follow-up', *a, cwd=self.root, role='maintainer')
        pkg = lambda pid: next(p for p in json.loads((self.root / '00-Guidelines/13-Plan/packages.json').read_text())['packages'] if p['id'] == pid)
        line = lambda pid: next(l for l in (self.root / '01-prompts.md').read_text().splitlines() if f'**{pid} ' in l)
        r = cli('P900', '  limits   recheck\n of 01-s ')
        self.assertEqual((r.returncode, r.stdout), (0, 'P900: 1 open follow-up(s)\n1. limits recheck of 01-s\nRendered 01-prompts.md (9 packages)\n'), r.stderr)
        self.assertEqual((pkg('P900')['followups'], pkg('P900')['status']), (['limits recheck of 01-s'], 'verified'))
        self.assertEqual(line('P900'), '- [!] **P900 — ST01** (opus, M): LP900. Follow-ups: (1) limits recheck of 01-s.')
        for args, err in ((('P900', 'limits recheck of 01-s'), 'already has this follow-up'), (('P900', '  '), 'Empty follow-up'),
                          (('P777', 'x'), 'Unknown package P777'), (('P900',), 'not both'), (('P900', 'x', '--done', '1'), 'not both'),
                          (('P900', '--done', '2'), 'has follow-ups 1 to 1'), (('P031', '--done', '1'), 'has no open follow-up')):
            r = cli(*args)
            self.assertEqual((r.returncode != 0, err in r.stderr), (True, True), (args, r.stderr))
        cli('P900', 'second')
        cli('P900', 'third')
        self.assertEqual(cli('P900', '--done', '1', '3').stdout.split('\n')[:2], ['P900: 1 open follow-up(s)', '1. second'])
        cli('P900', '--done', '1')
        self.assertNotIn('followups', pkg('P900'))
        self.assertEqual(line('P900'), '- [x] **P900 — ST01** (opus, M): LP900.')
        cli('P030', 'on an open package')
        self.assertEqual(line('P030'), '- [-] **P030 — DS01** (opus, M): LP030. Status: in-progress. Follow-ups: (1) on an open package.')

    def test_syn_follow_up_check_and_doctor(self):
        """check's followups rule (a non-empty list of distinct one-line texts, absent when none), and doctor's partial row and
        check's undecided limit in a verified package, each allowed while it has open follow-ups (user decision 2026-11-26)."""
        for rel in ('98-Team Notebook/05-dependencies.py', '96-Local Testing/01-run.py'):  # check and doctor read them
            (self.root / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(ROOT / rel, self.root / rel)
        plan = self.root / '00-Guidelines/13-Plan/packages.json'
        data = json.loads(plan.read_text())
        p900 = next(p for p in data['packages'] if p['id'] == 'P900')
        bad = lambda: [l for l in self.run_py(self.cli, 'check', cwd=self.root, role='maintainer').stdout.splitlines() if 'Bad followups' in l]
        for value, want in ((None, False), (['a'], False), (['a', 'b'], False), ([], True), (['a', 'a'], True), (['a\nb'], True), ([' '], True),
                            ('a', True), ([1], True), ([{'a': 1}], True)):
            p900.pop('followups', None)
            if value is not None:
                p900['followups'] = value
            plan.write_text(json.dumps(data))
            self.assertEqual(bool(bad()), want, value)
        p900.pop('followups')
        plan.write_text(json.dumps(data))
        (self.root / '07-Strings/Docs').mkdir(parents=True, exist_ok=True)
        (self.root / '07-Strings/01-s.hpp').write_text('#pragma once\n')
        (self.root / '07-Strings/00-index.md').write_text('# 07 Strings — Inventory\n\n## Basic\n\n| Header | Operations | Status |\n|---|---|---|\n'
                                                          '| `01-s.hpp` | s; t | partial missing: t; [evidence](Docs/01-s.md) |\n')
        doctor = lambda: self.run_py(self.cli, 'doctor', cwd=self.root, role='maintainer')
        partial = lambda: [l for l in doctor().stdout.splitlines() if 'partial row inside' in l]
        self.assertEqual(len(partial()), 1, f'a partial row in a verified package without follow-ups: {doctor()}')
        self.assertEqual(self.run_py(self.cli, 'follow-up', 'P900', 'implement t', cwd=self.root, role='maintainer').returncode, 0)
        self.assertEqual(partial(), [], 'allowed while a follow-up is open')
        doc = self.root / '07-Strings/Docs/01-s.md'
        limits = lambda: [l for l in self.run_py(self.cli, 'check', cwd=self.root, role='maintainer').stdout.splitlines() if 'Undecided limits' in l]
        doc.write_text('# s\n\n## Contracts\n\n- inherent, no decision.\n\n## Limits and Handoffs\n\n- t not tested.\n')
        self.assertEqual(limits(), [], 'an undecided limit is allowed while a follow-up is open')
        doc.write_text('# s\n')
        self.assertEqual(self.run_py(self.cli, 'follow-up', 'P900', '--done', '1', cwd=self.root, role='maintainer').returncode, 0)
        doc.write_text('# s\n\n## Contracts\n\n- inherent, no decision.\n\n## Limits and Handoffs\n\n- t not tested.\n')  # e.g. a later edit
        self.assertEqual(limits(), ['Undecided limits in verified P900: 1, the first at 07-Strings/Docs/01-s.md:9; file a follow-up '
                                    '(plan.py follow-up P900 ...) or decide them (02-structure.md, Names)'], 'verified without follow-ups')
        doc.write_text(doc.read_text().replace('tested.', 'tested (user decision 2026-11-26).'))
        self.assertEqual(limits(), [], 'a decided limit')

    def test_syn_follow_up_ready_and_show(self):
        """A verified package with open follow-ups is ready work ahead of open packages (next, ready, status), keeps its
        status, lists its follow-ups in its brief, and a contributor claims it on `Pxxx-FU<yymmdd>-<login>`."""
        plan = json.loads((self.root / '00-Guidelines/13-Plan/packages.json').read_text())
        next(p for p in plan['packages'] if p['id'] == 'P900')['followups'] = ['recheck limits', 'second']
        (self.root / '00-Guidelines/13-Plan/packages.json').write_text(json.dumps(plan))
        r = self.run_py(self.cli, 'next', cwd=self.root, role='maintainer')
        self.assertEqual(r.stdout, 'P900\n', r.stderr)
        with self.environ('maintainer'):
            first = parse_ready_text(PLAN.ready_text(self.root, 3))[1][0]
        self.assertTrue(first.startswith('- P900 [verified, 2 follow-up(s), opus, M] 07-Strings — LP900'), first)
        out = self.show('P900', 'maintainer')
        self.assertIn('| verified with 2 open follow-up(s) |', out[0])
        i = out.index('Follow-ups: the package stays verified; /package runs only these and closes each one done with '
                      '`plan.py follow-up P900 --done <n>`:')
        self.assertEqual(out[i + 1:i + 4], ['1. recheck limits', '2. second', ''])
        today = f'FU{datetime.date.today():%y%m%d}'
        self.assertEqual(self.show('P900')[2], o_claim_line(False, 'P900', today, f'P900-{today}-alice', REPO))

    def test_syn_set_verified_gate(self):
        """plan.py set Pxxx verified (user decision 2026-11-26) refuses while an evidence document of the package (a
        target's or an --evidence path) has a limit without a `user decision <date>`, naming each, and changes nothing;
        once each is decided it sets the status. Open follow-ups of a verified package cover its limits, as in check, and its
        last follow-up stays open until they are decided."""
        doc = self.root / '02-Data Structures/Docs/01-dsu.md'
        doc.parent.mkdir(parents=True)
        doc.write_text('# DSU\n## Limits and Handoffs\n- open limit\n- decided (user decision 2026-11-26)\n')
        before = (self.root / '00-Guidelines/13-Plan/packages.json').read_text()
        r = self.run_py(self.cli, 'set', 'P030', 'verified', cwd=self.root, role='maintainer')
        self.assertEqual((r.returncode, r.stderr.split('\n')[1:]), (1, ['  02-Data Structures/Docs/01-dsu.md:3: - open limit', '']), r.stderr)
        self.assertIn('P030 not set verified: 1 limit(s) without a `user decision <date>`', r.stderr)
        self.assertEqual((self.root / '00-Guidelines/13-Plan/packages.json').read_text(), before)
        (self.root / 'notes').mkdir(exist_ok=True)
        (self.root / 'notes/extra.md').write_text('## Handoff\n- to another package\n')
        doc.write_text('# DSU\n## Limits and Handoffs\n- decided (user decision 2026-11-26)\n')
        r = self.run_py(self.cli, 'set', 'P030', 'verified', '--evidence', 'notes/extra.md', cwd=self.root, role='maintainer')
        self.assertIn('notes/extra.md:2: - to another package', r.stderr, 'an --evidence path counts too')
        r = self.run_py(self.cli, 'set', 'P030', 'in-progress', '--evidence', 'notes/extra.md', cwd=self.root, role='maintainer')
        self.assertEqual(r.returncode, 0, 'other statuses pass')
        (self.root / 'notes/extra.md').write_text('## Handoff\n- to another package (user decision 2026-11-26)\n')
        r = self.run_py(self.cli, 'set', 'P030', 'verified', cwd=self.root, role='maintainer')
        self.assertEqual(r.returncode, 0, r.stderr)
        doc.write_text('# DSU\n## Limits and Handoffs\n- open again\n')
        cli = lambda *a: self.run_py(self.cli, *a, cwd=self.root, role='maintainer')
        self.assertEqual(cli('follow-up', 'P030', 'recheck the limits').returncode, 0)
        r = cli('set', 'P030', 'verified', '--note', 'state')
        self.assertEqual(r.returncode, 0, 'a verified package with open follow-ups updates its note: they cover its limits')
        r = cli('follow-up', 'P030', '--done', '1')
        self.assertEqual((r.returncode, r.stderr), (1, 'P030 keeps its last follow-up while 1 limit(s) lack a `user decision <date>` '
                                                       '(02-structure.md, Names):\n  02-Data Structures/Docs/01-dsu.md:3: - open again\n'))
        doc.write_text('# DSU\n## Limits and Handoffs\n- open again (user decision 2026-11-26)\n')
        self.assertEqual(cli('follow-up', 'P030', '--done', '1').returncode, 0, 'closed once its limits are decided')

    def test_syn_hand_off(self):
        """A limit handed to another package (`follow-up on Pxxx`, user decision 2026-11-27) counts only while that package
        has an open follow-up: check names the first such item with `[no open follow-up on Pxxx]` and set verified refuses
        it; `follow-up --done` lists every limit citing the package it closes, and keeps its last follow-up open, changing
        nothing, while one of them lacks a `user decision <date>`."""
        for rel in ('98-Team Notebook/05-dependencies.py', '96-Local Testing/01-run.py'):  # check reads them
            (self.root / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(ROOT / rel, self.root / rel)
        cli = lambda *a: self.run_py(self.cli, *a, cwd=self.root, role='maintainer')
        plan = self.root / '00-Guidelines/13-Plan/packages.json'
        (self.root / '07-Strings/Docs').mkdir(parents=True)
        (self.root / '07-Strings/01-s.hpp').write_text('#pragma once\n')
        (self.root / '07-Strings/00-index.md').write_text('# 07 Strings — Inventory\n\n## Basic\n\n| Header | Operations | Status |\n|---|---|---|\n'
                                                          '| `01-s.hpp` | s | verified; [evidence](Docs/01-s.md) |\n')
        s = self.root / '07-Strings/Docs/01-s.md'
        s.write_text('# s\n\n## Limits and Handoffs\n\n- t belongs to P031 (follow-up on P031).\n')
        dsu = self.root / '02-Data Structures/Docs/01-dsu.md'
        dsu.parent.mkdir(parents=True)
        dsu.write_text('# DSU\n## Limits and Handoffs\n- u handed on (follow-up on P031)\n')
        (self.root / '02-Data Structures/00-index.md').write_text('# 02 Data Structures — Inventory\n\n## Basic\n\n| Header | Operations | Status |\n'
                                                                  '|---|---|---|\n| `01-dsu.hpp` | DSU | partial; [evidence](Docs/01-dsu.md) |\n')
        limits = lambda: [l for l in cli('check').stdout.splitlines() if 'Undecided limits' in l]
        self.assertEqual(limits(), ['Undecided limits in verified P900: 1, the first at 07-Strings/Docs/01-s.md:5 [no open follow-up on P031]; '
                                    'file a follow-up (plan.py follow-up P900 ...) or decide them (02-structure.md, Names)'])
        r = cli('set', 'P030', 'verified')
        self.assertEqual((r.returncode, r.stderr.split('\n')[1:]),
                         (1, ['  02-Data Structures/Docs/01-dsu.md:3: [no open follow-up on P031] - u handed on (follow-up on P031)', '']), r.stderr)
        self.assertEqual((cli('follow-up', 'P031', 'do t and u').returncode, cli('follow-up', 'P031', 'second').returncode), (0, 0))
        self.assertEqual(limits(), [], 'covered while P031 has an open follow-up')
        self.assertEqual(cli('set', 'P030', 'verified').returncode, 0)
        named = ['Limits citing `follow-up on P031`: delete or update each the closed follow-up(s) settled:',
                 '  02-Data Structures/Docs/01-dsu.md:3: - u handed on (follow-up on P031)',
                 '  07-Strings/Docs/01-s.md:5: - t belongs to P031 (follow-up on P031).']
        r = cli('follow-up', 'P031', '--done', '1')
        self.assertEqual((r.returncode, r.stdout.split('\n')[:5]), (0, ['P031: 1 open follow-up(s)', '1. second', *named]), r.stderr)
        before = plan.read_text()
        r = cli('follow-up', 'P031', '--done', '1')
        self.assertEqual((r.returncode, r.stderr.split('\n')[1:]), (1, named[1:] + ['']), r.stderr)
        self.assertIn('P031 keeps its last follow-up while 2 limit(s) hand work to it (`follow-up on P031`) without a `user decision <date>`', r.stderr)
        self.assertEqual(plan.read_text(), before, 'a refusal changes nothing')
        dsu.write_text('# DSU\n## Limits and Handoffs\n- u handed on (follow-up on P031; user decision 2026-11-27)\n')
        s.write_text('# s\n\n## Limits and Handoffs\n\n- none\n')
        r = cli('follow-up', 'P031', '--done', '1')
        self.assertEqual((r.returncode, r.stdout.split('\n')[:3]),
                         (0, ['P031: 0 open follow-up(s)', named[0], '  02-Data Structures/Docs/01-dsu.md:3: - u handed on (follow-up on P031; user decision 2026-11-27)']),
                         r.stderr)
        self.assertEqual(limits(), [], 'decided by the user once P031 has none open')

    def test_syn_waiting(self):
        def wpr(n, branch, author, draft=False, heads=None, review=''):
            return pr(n, branch, author, draft) | {'title': 'T' + str(n), 'reviewDecision': review,
                                                   **({'commits': [{'messageHeadline': h} for h in heads]} if heads is not None else {})}
        def waiting(role):
            with self.environ(role):
                return PLAN.waiting(self.root).split('\n')
        self.prs([])
        self.assertEqual(waiting('maintainer'), ['Role: maintainer', 'Waiting on you: nothing'])
        bare = self.tmp / 'remote.git'
        git(self.tmp, 'init', '-q', '--bare', str(bare))
        git(self.root, 'remote', 'add', 'origin', str(bare))
        git(self.root, 'push', '-q', '-u', 'origin', 'master')
        git(self.root, 'commit', '-q', '--allow-empty', '-m', 'local')
        (self.root / '.claude/finish').mkdir(parents=True)
        (self.root / '.claude/finish/SUP18.txt').write_text('a | d | f:1 | g | s | ask\nb | d | f:2 | g | s | fixed\nc | d | f:3 | g | s | ask\n')
        (self.root / '.claude/finish/ST01.txt').write_text('a | d | f:1 | g | s | answered\n')
        (self.root / '.claude/merge-pr').mkdir()
        (self.tmp / 'wt5').mkdir()
        (self.root / '.claude/merge-pr/5.json').write_text(json.dumps({'worktree': str(self.tmp / 'wt5')}))
        (self.root / '.claude/merge-pr/9.json').write_text(json.dumps({'worktree': str(self.tmp / 'gone')}))
        (self.root / '.claude/merge-pr/10.json').write_text('[1]')
        self.prs([wpr(4, 'P250-SUP18-matt', 'matt', True, ['P250: claim SUP18', 'P250: SUP17 a — verified', 'P250: sup18 b — verified', 'P250: SUP19 c — partial']),
                  wpr(5, 'P030-DS01-matt', 'matt', heads=[]), wpr(6, 'P040-GR01-carol', 'carol', review='CHANGES_REQUESTED'),
                  wpr(7, 'feature/x', 'dan'), wpr(3, 'P050-MA01-erin', 'erin', True)])
        self.assertEqual(waiting('maintainer'), [
            'Role: maintainer', 'Waiting on you:', '- push: master is 1 commit(s) ahead of origin/master',
            '- `/merge-pr 5`: #5 P030 (DS01) by @matt', '- `/merge-pr 7`: #7 "T7" by @dan', '- answer: `/finish SUP18` has 2 open question(s)',
            f'- `/merge-pr 5`: checked out at {self.tmp / "wt5"}; land or drop it', 'Waiting on others:', '- #3 P050 (MA01) by @erin: draft',
            '- #4 P250 (SUP18) by @matt: draft, 2 of 3 batches verified, next SUP19', '- #6 P040 (GR01) by @carol: changes requested'])
        self.calls()
        git(self.root, 'checkout', '-q', '-b', 'P250-SUP19-alice')
        shutil.rmtree(self.root / '.claude')
        self.prs([wpr(8, 'P250-SUP19-alice', 'alice', True, ['P250: SUP17 a — verified', 'P250: SUP18 b — verified', 'P250: SUP19 c — verified']),
                  wpr(9, 'P031-DS02-alice', 'alice', True, []), wpr(12, 'P031-DS02-alice', 'alice', True, ['P031: label — verified']),
                  wpr(10, 'P050-MA01-Alice', 'Alice', review='CHANGES_REQUESTED'), wpr(11, 'P021-C08-alice', 'alice'), wpr(5, 'P030-DS01-bob', 'bob'),
                  wpr(13, 'feature/y', 'alice', True), wpr(14, 'P900-FU261126-alice', 'alice', True, ['P900: claim FU261126']),
                  wpr(15, 'P040-fu1-alice', 'alice', True, ['P040: LP040 — verified'])])
        self.assertEqual(waiting('contributor'), [
            'Role: contributor', 'Waiting on you:', '- push: P250-SUP19-alice has no upstream yet', '- `gh pr ready 8`: #8 P250 (SUP19), every batch verified',
            '- `/package DS02`: #9 P031 (DS02), 0 of 1 batches verified, next DS02', '- #10 P050 (MA01): changes requested; address the review',
            '- `gh pr ready 12`: #12 P031 (DS02), every batch verified', '- #13 "T13": draft',
            '- `/package P900`: #14 P900 (FU261126), 0 of 1 follow-up runs verified, next P900', '- `gh pr ready 15`: #15 P040 (FU1), every batch verified',
            'Waiting on others:',
            "- #11 P021 (C08): waiting for the maintainer's /merge-pr"])
        self.assertEqual(self.calls(), [['pr', 'list', '--repo', REPO, '--state', 'open', '--limit', '30', '--json',
                                         'number,title,headRefName,isDraft,author,reviewDecision,commits'], ['api', 'user', '--jq', '.login']])
        self.prs([wpr(n, f'feature/{n}', 'zed') for n in range(PLAN.PR_PAGE)])
        self.assertEqual(waiting('contributor')[-2:], ['Waiting on others:', '- only the newest 30 open pull requests are listed'])
        self.gh_set('prs', out='not json')
        self.assertEqual(waiting('maintainer')[2:], ['- push: P250-SUP19-alice has no upstream yet', '- pull requests: SKIP (gh pr list printed no JSON)'])
        with self.environ('maintainer', gh=False):
            self.assertEqual(PLAN.waiting(self.root).split('\n')[3:], ['- pull requests: SKIP (gh not installed)'])
        self.prs([])
        r = self.run_py(self.cli, 'waiting', cwd=self.root, role='maintainer')
        self.assertEqual((r.returncode, r.stdout), (0, 'Role: maintainer\nWaiting on you:\n- push: P250-SUP19-alice has no upstream yet\n'), r.stderr)

    def test_syn_sweeps(self):
        core = self.root / '01-Core'
        (core / 'Docs').mkdir(parents=True)
        row = lambda n, ops, status: f'| `{n}.hpp` | yes | {ops} | {status}; [{n}.md](Docs/{n}.md) |\n'
        (core / '00-index.md').write_text('# 01 Core — inventory\n\n| Header | Basic | Operations | Status |\n|---|---|---|---|\n' + row('03-a', 'A: f, g', 'verified')
                                          + row('04-b', 'B: h', 'verified') + row('05-c', 'C: k', 'verified') + row('06-cmini', 'CMini: k', 'verified')
                                          + row('07-d', 'D: m', 'partial') + row('08-e', 'E: q', 'verified') + row('01-template', 'aliases', 'verified'))
        head = '## Benchmarks\n\n### Optimization Candidates\n\nText.\n\n| Operation | Candidate | Verdict | Evidence |\n|---|---|---|---|\n'
        (core / 'Docs/03-a.md').write_text(head + '| `f`, `g` | select | adopted | GCC 15: 0.91 |\n| `g` | simd | rejected | argument: outside the ISA set |\n| `f` | — | none found: one add | read the code |\n\n## Next\n\n| x | y |\n')
        (core / 'Docs/04-b.md').write_text('## Benchmarks\n')
        (core / 'Docs/05-c.md').write_text(head + '| `k` | table | maybe | 1.0 |\n| `k` | chain | rejected | |\n| `k` | asm | rejected: Deferred | x |\n| `k` | fold | rejected | 1.2 |\n')
        (core / 'Docs/06-cmini.md').write_text('mini\n')
        (core / 'Docs/08-e.md').write_text('### Optimization Candidates\n\nNo table.\n')
        pending, faults, named = PLAN.sweeps(self.root)
        self.assertEqual(pending, ['01-Core/04-b.hpp'], 'a mini, a partial row and the template are not pending')
        self.assertEqual(named, {'01-Core/03-a.hpp': {'f', 'g'}, '01-Core/05-c.hpp': {'k'}})
        where = 'Optimization Candidates (05-testing.md, Benchmarks): 01-Core/'
        self.assertEqual([f[len(where):].split(':')[0] + ':' + f[len(where):].split(': ')[1].split(' ')[0] for f in faults],
                         ['05-c.hpp:row', '05-c.hpp:row', '05-c.hpp:undecided', '05-c.hpp:verdict', '08-e.hpp:no'], faults)
        self.assertTrue(all(f.startswith(where) for f in faults))
        self.prs([])
        with self.environ('maintainer'):
            self.assertEqual(PLAN.waiting(self.root).split('\n'), ['Role: maintainer', 'Waiting on you:', '- optimization sweep pending (05-testing.md, Benchmarks): 01-Core/04-b.hpp'])
        with self.environ('contributor'):
            self.assertEqual(PLAN.waiting(self.root).split('\n'), ['Role: contributor', 'Waiting on you: nothing'])

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

    def test_gaps_review_record(self):
        section = lambda out: out.split('### Review record (/package step 10)\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        for f, text in (('07-Strings/01-s.hpp', 'h\n'), ('07-Strings/Docs/01-s.md', 'd\n'), ('96-Local Testing/07-Strings/01-s_tester.py', 't\n'),
                        ('96-Local Testing/07-Strings/02-other_tester.py', 'o\n')):
            (self.root / f).parent.mkdir(parents=True, exist_ok=True)
            (self.root / f).write_text(text)
        r, _ = self.gaps('ST01', role='maintainer', branch='master')
        self.assertEqual(section(r.stdout), ['none: step 2 runs the full audit'], r.stderr)
        w = REAL_SUBPROCESS_RUN([sys.executable, str(GAPS_PY), '--reviewed', 'ST01'], cwd=self.root, capture_output=True, text=True,
                                input='1. finding\n2. limit\n', env=self.env('maintainer', True, None), timeout=300)
        self.assertEqual((w.returncode, w.stdout), (0, 'review of ST01 recorded: 3 files, .claude/finish/ST01.review.json\n'), w.stderr)
        r, _ = self.gaps('ST01', role='maintainer')
        self.assertEqual(section(r.stdout), ['covers the current files: step 2 reuses this report and starts no audit', '1. finding', '2. limit'])
        (self.root / '07-Strings/01-s.hpp').write_text('h2\n')
        (self.root / '96-Local Testing/07-Strings/01-s_tester.py').unlink()
        (self.root / '96-Local Testing/07-Strings/01-s_extra.cpp').write_text('e\n')
        r, _ = self.gaps('ST01', role='maintainer')
        self.assertEqual(section(r.stdout), ['changed since the review: step 2 reviews only these changes, in .claude/finish/ST01.since-review.diff:',
                                             '07-Strings/01-s.hpp', '96-Local Testing/07-Strings/01-s_extra.cpp', '96-Local Testing/07-Strings/01-s_tester.py'])
        self.assertEqual((self.root / '.claude/finish/ST01.since-review.diff').read_text(),
                         '=== 07-Strings/01-s.hpp\n@@ -1 +1 @@\n-h\n+h2\n=== 96-Local Testing/07-Strings/01-s_extra.cpp: new since the review\n'
                         '=== 96-Local Testing/07-Strings/01-s_tester.py: deleted since the review\n', 'the diff since the review (user decision 2026-11-26)')
        blob = git(self.root, 'hash-object', '-w', '07-Strings/01-s.hpp').strip()
        self.assertEqual(GAPS.since(self.root, 'x.hpp', 'f' * 40, blob), '=== x.hpp: its reviewed blob is gone, so the whole file\n')
        w = REAL_SUBPROCESS_RUN([sys.executable, str(GAPS_PY), '--reviewed', 'ST01', '--wrong'], cwd=self.root, capture_output=True, text=True,
                                input='1. wrong answer\n', env=self.env('maintainer', True, None), timeout=300)
        self.assertEqual(w.returncode, 0, w.stderr)
        self.assertTrue(json.loads((self.root / '.claude/finish/ST01.review.json').read_text())['wrong'])
        r, _ = self.gaps('ST01', role='maintainer')
        self.assertEqual(section(r.stdout), ['the review found a wrong answer or undefined behavior: step 2 runs the full audit'],
                         'a review that found a wrong answer leaves the full audit to /finish, even over unchanged files')
        bad = REAL_SUBPROCESS_RUN([sys.executable, str(GAPS_PY), '--reviewed', 'nothing'], cwd=self.root, capture_output=True, text=True,
                                  input='', env=self.env('maintainer', True, None), timeout=300)
        self.assertNotEqual(bad.returncode, 0)

    def test_gaps_review_record_support(self):
        """A batch without targets (SUP17 of P250; user decision 2026-11-23): the record covers the package's evidence and every
        file the branch or the working tree changed, except the plan files plan.py rewrites and .claude/finish, so a later
        edit or a newly changed file reopens the review."""
        section = lambda out: out.split('### Review record (/package step 10)\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        record = self.root / '.claude/finish/SUP17.review.json'
        git(self.root, 'checkout', '-q', '-B', 'side', 'master')
        (self.root / 'tools').mkdir()
        (self.root / 'tools/b.py').write_text('b\n')
        git(self.root, 'add', 'tools/b.py')
        git(self.root, 'commit', '-q', '-m', 'b')
        (self.root / 'tools/a.py').write_text('a\n')
        (self.root / '01-prompts.md').write_text('rendered\n')
        (self.root / 'notes/p250.md').write_text('# P250\n\n## Limits\n\n- one\n')
        w = REAL_SUBPROCESS_RUN([sys.executable, str(GAPS_PY), '--reviewed', 'SUP17'], cwd=self.root, capture_output=True, text=True,
                                input='1. finding\n', env=self.env('maintainer', True, None), timeout=300)
        self.assertEqual((w.returncode, w.stdout), (0, 'review of SUP17 recorded: 3 files, .claude/finish/SUP17.review.json\n'), w.stderr)
        self.assertEqual(sorted(json.loads(record.read_text())['files']), ['notes/p250.md', 'tools/a.py', 'tools/b.py'],
                         'evidence, the uncommitted file and the committed one; not 01-prompts.md or the record itself')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual(section(r.stdout), ['covers the current files: step 2 reuses this report and starts no audit', '1. finding'])
        limits = r.stdout.split('### Recorded limits (evidence headings)\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        self.assertEqual(limits, ['notes/p250.md:3: ## Limits', '- one'], "the facts read the package's evidence too")
        (self.root / '01-prompts.md').write_text('rendered again\n')
        for name in ('packages.json', 'batches.json'):  # plan.py set rewrites them after the review
            f = self.root / '00-Guidelines/13-Plan' / name
            f.write_text(json.dumps(json.loads(f.read_text()), indent=1))
        with open(self.root / '00-Guidelines/13-Plan/finish-log.tsv', 'a') as f:
            f.write('2026-11-23\tSUP99\tm\th\t0\t0\t0\tpartial\tnone\n')
        (self.root / '.claude/finish/SUP17.txt').write_text('A1 | defect | x:1 | g | s | open\n')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual(section(r.stdout)[0], 'covers the current files: step 2 reuses this report and starts no audit', 'bookkeeping edits')
        (self.root / 'tools/b.py').write_text('b2\n')
        (self.root / 'tools/c.py').write_text('c\n')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual(section(r.stdout), ['changed since the review: step 2 reviews only these changes, in .claude/finish/SUP17.since-review.diff:', 'tools/b.py', 'tools/c.py'])

    def test_gaps_undecided_and_follow_ups(self):
        """gaps.py's Undecided limits (plan.undecided over the package's evidence, a hand-off counting while its package has an
        open follow-up, user decision 2026-11-27) and Follow-ups sections (user decision 2026-11-26), printed right after the
        review record."""
        part = lambda out, head: out.split(f'### {head}', 1)[1].split('\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        (self.root / 'notes/p250.md').write_text('# P250\n\n## Limits and Handoffs\n\n- open item\n- kept, user decision 2026-11-20\n'
                                                 '- handed (follow-up on P900)\n')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual((part(r.stdout, 'Undecided limits'), part(r.stdout, 'Follow-ups of P250')),
                         (['notes/p250.md:5: - open item', 'notes/p250.md:7: [no open follow-up on P900] - handed (follow-up on P900)'], ['(none)']),
                         r.stderr)
        for pid, text in (('P250', 'recheck'), ('P900', 'do the handed work')):
            self.assertEqual(self.run_py(self.cli, 'follow-up', pid, text, cwd=self.root, role='maintainer').returncode, 0)
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual((part(r.stdout, 'Undecided limits'), part(r.stdout, 'Follow-ups of P250')), (['notes/p250.md:5: - open item'], ['1. recheck']))
        heads = [l.split(' (')[0] for l in r.stdout.splitlines() if l.startswith('### ')]
        self.assertEqual(heads[heads.index('### Review record'):][:3], ['### Review record', '### Undecided limits', '### Follow-ups of P250'])

    def test_gaps_sources(self):
        """gaps.py's Sources section (95-Resources/00-index.md, Filing; user decision 2026-11-27): 00-sources.py check of
        the checkout, counting files placed for the key or its package; after the follow-ups; absent without the script."""
        part = lambda out, head: out.split(f'### {head}', 1)[1].split('\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertNotIn('### Sources', r.stdout, r.stderr)
        res = self.root / '95-Resources'
        (res / '00-Inbox').mkdir(parents=True)
        shutil.copy(ROOT / '95-Resources/00-sources.py', res / '00-sources.py')
        (res / '99-sources.json').write_text(json.dumps({'description': 'd', 'resources': []}))
        (res / '00-Inbox/00-requests.md').write_text('- 2026-11-27 | P250 | T | u | mine.pdf\n- 2026-11-27 | P900 | T | u | theirs.pdf\n')
        for name in ('mine.pdf', 'theirs.pdf', 'stray.pdf'):
            (res / '00-Inbox' / name).write_text(name)
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual(part(r.stdout, 'Sources'), ['00-Inbox/mine.pdf: placed for P250; file it (00-sources.py file)',
                                                     '00-Inbox/stray.pdf: no request line; file it if this work used it (00-sources.py file), else ask the user'],
                         r.stderr)
        heads = [l.split(' (')[0] for l in r.stdout.splitlines() if l.startswith('### ')]
        self.assertEqual(heads[heads.index('### Follow-ups of P250') + 1], '### Sources')

    def test_gaps_review_start(self):
        """gaps.py --start (SUP34): once per key it records the files uncommitted when /package starts, and a batch without
        targets leaves those out of its review while they keep that content; one changed since, and every file absent
        from the record, is covered. An unknown id records nothing; an unreadable record covers every file."""
        section = lambda out: out.split('### Review record (/package step 10)\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        start, record = self.root / '.claude/finish/SUP17.start.json', self.root / '.claude/finish/SUP17.review.json'
        run = lambda *a, stdin='': REAL_SUBPROCESS_RUN([sys.executable, str(GAPS_PY), *a], cwd=self.root, capture_output=True, text=True,
                                                       input=stdin, env=self.env('maintainer', True, None), timeout=300)
        (self.root / 'tools').mkdir()
        for name in ('other', 'shared'):  # another package's uncommitted work in the main checkout
            (self.root / f'tools/{name}.py').write_text(f'{name}\n')
        r = run('--start', 'nothing')
        self.assertEqual((r.returncode, r.stdout, r.stderr, start.parent.exists()), (0, '', '', False), 'no package: no record')
        r = run('--start', 'SUP17')
        self.assertEqual((r.returncode, r.stdout, r.stderr), (0, '', ''))
        blob = lambda f: git(self.root, 'hash-object', f).strip()
        want = {f: blob(f) for f in ('notes/p250.md', 'tools/other.py', 'tools/shared.py')}
        self.assertEqual(json.loads(start.read_text())['files'], want)
        first = start.read_bytes()
        (self.root / 'tools/shared.py').write_text('shared, edited by the batch\n')
        (self.root / 'tools/a.py').write_text('a\n')
        self.assertEqual(run('--start', 'SUP17').returncode, 0)
        self.assertEqual(start.read_bytes(), first, 'a resumed /package keeps the first record')
        w = run('--reviewed', 'SUP17', stdin='1. finding\n')
        self.assertEqual((w.returncode, w.stdout), (0, 'review of SUP17 recorded: 3 files, .claude/finish/SUP17.review.json\n'), w.stderr)
        self.assertEqual(sorted(json.loads(record.read_text())['files']), ['notes/p250.md', 'tools/a.py', 'tools/shared.py'],
                         'the evidence, a new file and one changed since the start; not the untouched other work')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual(section(r.stdout)[0], 'covers the current files: step 2 reuses this report and starts no audit')
        (self.root / 'tools/other.py').write_text('other, edited later\n')
        r, _ = self.gaps('SUP17', role='maintainer')
        self.assertEqual(section(r.stdout), ['changed since the review: step 2 reviews only these changes, in .claude/finish/SUP17.since-review.diff:', 'tools/other.py'],
                         'other work changed after the start is covered (a recorded limit)')
        start.write_text('{"files": 3}')
        w = run('--reviewed', 'SUP17', stdin='')
        self.assertEqual(w.stdout, 'review of SUP17 recorded: 4 files, .claude/finish/SUP17.review.json\n', 'an unreadable record leaves nothing out')

    def test_gaps_limits_facts(self):
        """Recorded limits (user decision 2026-11-24): the facts file holds every line of each Limits section in full, past
        400 lines and 160 characters; the printed copy keeps its 12-line cap within the room."""
        bullets = [f'- limit {i} ' + 'w' * 300 for i in range(450)]
        (self.root / 'notes/p250.md').write_text('# P250\n\n## Limits\n\n' + '\n'.join(bullets) + '\n\n## History\n\n- h\n')
        r, _ = self.gaps('SUP17', role='maintainer')
        facts = dict(sections_of((self.root / '.claude/finish/SUP17.facts.txt').read_text()))
        self.assertEqual(facts['Recorded limits (evidence headings)'], ['notes/p250.md:3: ## Limits', *bullets])
        self.assertLessEqual(len(r.stdout), max(o_room((GAPS_PY.parent / 'SKILL.md').read_text()), FLOOR))
        shown = dict(sections_of(r.stdout)).get('Recorded limits (evidence headings)')
        self.assertTrue(shown is None or (len(shown) <= CAP_LINES + 1 and shown[-1].endswith(' more in the facts file')), shown)

    def test_gaps_core_full(self):
        part = lambda out, title: out.split(f'### {title}\n', 1)[1].split('\n\n### ', 1)[0].split('\n')
        undecided, record = 'Undecided optimization candidates (Core full evidence)', 'Comparison record (05-testing.md, Stale evidence)'
        r, _ = self.gaps('ST01', role='maintainer', branch='master')
        self.assertNotIn(record, r.stdout, 'a contest batch has neither section')
        self.assertNotIn(undecided, r.stdout)
        core, doc = self.root / '01-Core', self.root / '01-Core/Docs/10-matrix.md'
        doc.parent.mkdir(parents=True, exist_ok=True)
        files = {'10-matrix.hpp': 'm\n', '01-template.hpp': 't\n'}
        for f, text in files.items():
            (core / f).write_text(text)
        rig = self.root / '96-Local Testing/01-Core/10-matrix_compare.cpp'
        rig.parent.mkdir(parents=True, exist_ok=True)
        rig.write_text('rig\n')
        doc.write_text('## Benchmarks\n\nThe window size was not tried.\n')
        r, _ = self.gaps('C07', role='maintainer')
        self.assertEqual(part(r.stdout, undecided), ['01-Core/Docs/10-matrix.md:3: The window size was not tried.', '01-Core/Docs/10-matrix.md: no "### Optimization Candidates" table'])
        self.assertEqual(part(r.stdout, record), ['01-Core/Docs/10-matrix.md: no Comparison line (96-Local Testing/06-compare.py)'])
        fake = self.bin / 'fakecxx'
        fake.write_text('#!/bin/sh\necho 15.3.0\n')
        fake.chmod(0o755)
        blob = lambda p: git(self.root, 'hash-object', str(p))[:12]
        line = lambda version='15.3.0', extra='': (f'Comparison: 2026-11-17, rig 96-Local Testing/01-Core/10-matrix_compare.cpp {blob(rig)}, base {"b" * 12}, headers '
                                                   f'01-template.hpp {blob(core / "01-template.hpp")} 10-matrix.hpp {blob(core / "10-matrix.hpp")}, fakecxx {version}, {extra}CPU Test, 3 passes, 4 rows, 0 losing over 3%.')
        old = 'Comparison: 2026-11-01, rig gone.cpp aaaaaaaaaaaa, base bbbbbbbbbbbb, headers 10-matrix.hpp cccccccccccc, fakecxx 1.0.0, CPU Test, 3 passes, 4 rows, 0 losing over 3%.'
        doc.write_text(f'## Benchmarks\n\n### Optimization Candidates\n\n| Operation | Candidate | Verdict | Evidence |\n\n{old}\n\n- {line()}\n')
        r, _ = self.gaps('C07', role='maintainer')
        self.assertEqual(part(r.stdout, undecided), ['(none)'])
        self.assertEqual(part(r.stdout, record), ['01-Core/Docs/10-matrix.md: comparison of 2026-11-17 is current'], 'the newest line counts')
        (core / '01-template.hpp').write_text('t2\n')
        rig.write_text('rig2\n')
        doc.write_text(doc.read_text().replace('fakecxx 15.3.0, ', 'fakecxx 15.2.0, no-such-cxx 9.1, '))
        r, _ = self.gaps('C07', role='maintainer')
        self.assertEqual(part(r.stdout, record), ['01-Core/Docs/10-matrix.md: comparison of 2026-11-17 is STALE, rerun it: the rig changed, 01-template.hpp changed, '
                                                  'fakecxx 15.2.0 is now 15.3.0'], 'only the deciding compiler, the first listed, counts (user decision 2026-11-26)')
        doc.write_text(doc.read_text().replace('fakecxx 15.2.0, no-such-cxx 9.1, ', 'no-such-cxx 9.1, fakecxx 15.2.0, '))
        r, _ = self.gaps('C07', role='maintainer')
        self.assertIn('no-such-cxx 9.1 is now not installed', part(r.stdout, record)[0])
        self.assertNotIn('fakecxx', part(r.stdout, record)[0])
        (core / '10-matrix.hpp').unlink()
        r, _ = self.gaps('C07', role='maintainer')
        self.assertIn('10-matrix.hpp changed', part(r.stdout, record)[0])

    def test_gaps_facts(self):
        """gaps.py end to end: the ledger section from the ledger file, the facts file in full, the printed copy within the
        room the real SKILL.md leaves (o_room, floor 1500), ledger first and sections in order of need."""
        ledger, facts = self.root / '.claude/finish/ST01.txt', self.root / '.claude/finish/ST01.facts.txt'
        led = 'Ledger .claude/finish/ST01.txt (open rows)'
        room = max(o_room((GAPS_PY.parent / 'SKILL.md').read_text()), FLOOR)

        def warn(n, shown):
            return (f'WARNING {n} stray line(s) in .claude/finish/ST01.txt (lines {shown}): keep only rows; move each experiment to '
                    '.claude/finish/ST01.lab.tsv as one row per attempt (SKILL.md, Ledger)')

        def run():
            r, _ = self.gaps('ST01', role='maintainer', branch='master')
            self.assertEqual((r.returncode, r.stderr), (0, ''))
            self.assertLessEqual(len(r.stdout), room, f'printed {len(r.stdout)} characters, room {room}')
            text = facts.read_text()
            lines = r.stdout.split('\n')
            self.assertIn(f'Full facts: .claude/finish/ST01.facts.txt ({len(text)} characters); read it at step 1 when sections below '
                          'are cut or not printed.', lines)
            brief = dict(sections_of(text))['Brief']
            head = lines[:lines.index(next(x for x in lines if x.startswith('Full facts: ')))]
            want = [x for k, x in enumerate(brief) if not k or re.match(r'(WARNING|STOP)\b', x)]
            self.assertEqual(head[-len(want):], want, 'the head keeps the brief\'s first line and its WARNING and STOP lines, however cut')
            self.assertFalse((self.root / '.claude/finish/ST01.facts.md').exists())
            got = sections_of(r.stdout)
            self.assertEqual(got[0][0], led, 'the ledger first')
            printed = [t for t, _ in got if t != 'Not printed (in the facts file)']
            skipped = got[-1][1] if got[-1][0] == 'Not printed (in the facts file)' else []
            titles = printed + skipped
            known = lambda ts: [t for t in ts if o_rank(t) < len(FACTS_ORDER)]  # the brief's own '### <batch>' heading is no section
            printed, skipped, titles = known(printed), known(skipped), known(titles)
            self.assertEqual(sorted(titles), sorted(known(t for t, _ in sections_of(text))), 'each section printed or named as not printed')
            # a section whose first line does not fit is named, and a later one that fits still prints
            self.assertEqual(printed, sorted(printed, key=o_rank), 'printed sections in order of need')
            self.assertEqual(skipped, sorted(skipped, key=o_rank), 'not printed sections in order of need')
            return dict(got), dict(sections_of(text))
        out, _ = run()
        self.assertEqual(out[led], ['(none: start at step 1)'])
        ledger.parent.mkdir(parents=True, exist_ok=True)
        ledger.write_text('A1 | decision | x:1 | y | user | fixed\n')
        self.assertEqual(run()[0][led], ['1 fixed or answered rows not shown', '(no open rows)'])
        o1, o2 = 'O1 | defect | 07-Strings/01-s.hpp:3 | gap | review | open', 'O2 | decision | x:2 | y | z | ask the user'
        ledger.write_text(f'{o1}\n\nA1 | decision | x:1 | y | user | answered 2026-11-22 (was: open)\ntried: 5% slower\n{o2}\n'
                          'C1 | defect | x | y | z | done\n')
        out, full = run()
        self.assertEqual(out[led], [warn(2, '4, 6'), '1 fixed or answered rows not shown', o1, o2], 'a done row is stray; the warning first')
        self.assertIn('Brief', out, 'a small ledger leaves room for the brief')
        lines, opened, stray = [], [], []
        for i in range(1, 101):
            if i % 7 == 0:
                lines.append(f'attempt {i}: 2% slower')
                stray.append(i)
            elif i % 9 == 0:
                lines.append('')
            elif i % 31 == 0:
                lines.append(f'C{i} | defect | 07-Strings/01-s.hpp:{i} | gap | review | fixed')
            else:
                lines.append(f'O{i:03} | defect | 07-Strings/01-s.hpp:{i} | a gap that takes some words to state | review | open')
                opened.append(lines[-1])
        ledger.write_text('\n'.join(lines) + '\n')
        out, full = run()
        self.assertEqual(len(stray), 14)
        whole = [warn(14, '7, 14, 21, 28, 35, 42, 49, 56, 63, 70 ...'), '3 fixed or answered rows not shown'] + opened
        self.assertEqual(full[led], whole, 'the facts file holds the whole ledger section')
        k = len(out[led]) - 1
        self.assertTrue(2 < k < len(whole), k)
        self.assertEqual(out[led], whole[:k] + [f'... {len(whole) - k} more in the facts file'], 'the warning, the count, then open rows as many as fit')
        self.assertNotIn('Brief', out)

    def test_gaps_head_brief(self):
        """The head keeps the brief's first line and every WARNING and STOP line of it (step 1 acts on them) before the facts
        pointer, whatever the room cuts: C07 is maintainer-only, so a contributor's brief holds a STOP line."""
        stop = 'STOP: P020 is maintainer-only (Core full); pick another package (`plan.py ready`).'
        for ident, role in (('C07', 'contributor'), ('C07', 'maintainer'), ('ST01', 'maintainer')):
            r, _ = self.gaps(ident, role=role, branch='master')
            lines = r.stdout.split('\n')
            head = lines[:next(i for i, x in enumerate(lines) if x.startswith('Full facts: '))]
            brief = dict(sections_of((self.root / f'.claude/finish/{ident}.facts.txt').read_text()))['Brief']
            want = [brief[0]] + [x for x in brief[1:] if x.startswith(('WARNING', 'STOP'))]
            self.assertEqual(head[-len(want):], want, (ident, role))
            self.assertEqual(stop in want, role == 'contributor', (ident, role, want))

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
        here = git(ROOT, 'branch', '--show-current').strip()
        for open_fu in (False, True):  # a verified package's claim names its follow-ups while any is open (user decision 2026-11-26)
            v = next((p for p in self.pl if p['status'] == 'verified' and p['model'] == 'opus' and bool(p.get('followups')) == open_fu), None)
            if v:
                self.prs([])
                with self.environ('contributor'):
                    out = PLAN.show(ROOT, v['batches'][0]).split('\n')
                batch = f'FU{datetime.date.today():%y%m%d}' if open_fu else v['batches'][0]
                want = o_claim_branch(v['id'], v['batches'], batch, here, {}, 'alice')
                self.assertEqual(out[2], o_claim_line(False, v['id'], batch, want, REPO, here=here == want), f'a verified package, follow-ups {open_fu}')

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

    def test_real_launch_one_effort_per_preset(self):
        """Every package's brief prints its preset's own effort, whatever its size (CLAUDE.md, Models and Delegation)."""
        want = {}
        for f in (ROOT / '.claude/agents').glob('*-session.md'):
            head = [l.split(':', 1) for l in f.read_text().split('---')[1].strip().split('\n')]
            meta = {k.strip(): v.strip() for k, v in head}
            want[meta['model']] = f'launch: claude --agent {meta["name"]} --effort {meta["effort"]}'
        sizes = set()
        with self.environ('maintainer'):
            for p in self.pl:
                if p['model'] == 'fable' or p['size'] == 'XL':
                    first = PLAN.show(ROOT, p['id']).split('\n')[0].split(' | ')
                    self.assertEqual(first[5], want[self.packages['models'][p['model']]], p['id'])
                    sizes.add((p['model'], p['size']))
        self.assertIn(('fable', 'XL'), sizes)
        self.assertGreater(len({s for m, s in sizes if m == 'fable'}), 1)

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
        fu = f'FU{datetime.date.today():%y%m%d}'  # a verified package with open follow-ups is ready work, claimed on their branch
        prs = [pr(i + 1, f'{p["id"]}-{fu if p["status"] == "verified" else p["batches"][0]}-bob', 'bob') for i, p in enumerate(self.pl)
               if p['status'] != 'verified' or p.get('followups')]
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

    def test_real_rooms(self):
        """Every open package of the real plan (SUP34; quick: the first 12): gaps.py's printed facts within the room of
        /finish's SKILL.md, and `plan.py show --skill <s>` within the room of each skill printing the brief, its first line
        kept; gh faked, eight runs at a time, the facts files restored afterwards."""
        import concurrent.futures
        import time
        ids = [p['id'] for p in self.pl if p['status'] != 'verified']
        ids = ids[:12] if MODE == 'quick' else ids
        rooms = {s: max(o_room((ROOT / '.claude/skills' / s / 'SKILL.md').read_text()), FLOOR) for s in ('finish', 'package', 'prune', 'restyle')}
        jobs = [(GAPS_PY, (i,), 'finish') for i in ids] + [(PLAN_PY, ('show', '--skill', s, i), s) for i in ids for s in ('package', 'prune', 'restyle')]
        t0 = time.monotonic()
        with real_facts(), concurrent.futures.ThreadPoolExecutor(8) as pool:
            results = list(pool.map(lambda j: self.run_py(j[0], *j[1], cwd=ROOT, role='maintainer'), jobs))
        print(f'  {len(ids)} open packages, {len(jobs)} runs in {time.monotonic() - t0:.1f} s', file=sys.stderr, flush=True)
        worst = {}
        for (script, args, skill), r in zip(jobs, results):
            info = f'{script.name} {" ".join(args)}: {len(r.stdout)} characters, room {rooms[skill]}\n{r.stderr[-1500:]}'
            self.assertEqual(r.returncode, 0, info)
            self.assertLessEqual(len(r.stdout), rooms[skill], info)
            if skill != 'finish':
                self.assertTrue(r.stdout.startswith(f'{args[-1]} — '), info)
            worst[skill] = max(worst.get(skill, 0), len(r.stdout))
        print(f'  largest printed: {worst}, rooms {rooms}', file=sys.stderr, flush=True)

    def test_real_gaps_role_line(self):
        branch = git(ROOT, 'branch', '--show-current').strip()
        p = self.first(lambda p: p['model'] == 'opus' and len(p['batches']) > 1)
        first = next((b for b in p['batches'] if b not in o_landed(ROOT)), p['batches'][0])  # log-based, not inventory rows
        for arg, role, batch in ((p['id'], 'contributor', first), (p['batches'][-1], 'contributor', p['batches'][-1]),
                                 (p['id'], 'maintainer', None)):
            want = batch and o_claim_branch(p['id'], p['batches'], batch, branch, {}, 'alice')
            last = batch and all(b == batch or b in o_landed(ROOT) for b in p['batches'])
            with real_facts():
                r = self.run_py(GAPS_PY, arg, cwd=ROOT, role=role)
                lines = r.stdout.split('\n')
                key = lines[lines.index('### Target') + 1][4:].split(' | ')[0]
                facts = ROOT / '.claude/finish' / f'{key}.facts.txt'
                self.assertTrue(facts.is_file() and any(l.startswith(f'Full facts: .claude/finish/{key}.facts.txt ({len(facts.read_text())} characters)')
                                                        for l in lines), r.stdout[:800])
                self.assertFalse((ROOT / '.claude/finish' / f'{key}.facts.md').exists())
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
    for case in (Units, Facts, Synthetic, Real):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n not in RANDOM)
    print(f"plan role/claim checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} tests={suite.countTestCases()}",
          flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
