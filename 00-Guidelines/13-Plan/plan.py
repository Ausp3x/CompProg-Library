#!/usr/bin/env python3
"""Work-plan manifest CLI: show, status, next, set, follow-up, render, rank, check, ready, doctor and waiting for packages
and batches. show, next, status, ready and waiting also read the clone's role and the open pull requests."""
import argparse
import collections
import datetime
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PLAN = '00-Guidelines/13-Plan'
BEGIN, END = '<!-- plan:begin -->', '<!-- plan:end -->'
PHASES = ['Foundations Re-audit', 'Core Full Types', 'Contest Core',
          'Python Contest Layer', 'Specialist and Research Families', 'Final Support Integration']
LEGEND = ('Legend: `[ ]` planned, `[-]` in progress, `[x]` verified, `[!]` verified with open follow-ups; **package — batches in '
          'order** (model, size): label. Open packages add their status and note, and any package its open follow-ups; '
          '`plan.py show Pxxx` gives a verified package\'s evidence and note.')
TIER_RANK = {'Basic': 0, 'Core': 0, 'Advanced': 1, 'Esoteric': 2}
REPO = 'Ausp3x/CompProg-Library'  # canonical repository; `git config compprog.repo` overrides it
CLAIM = re.compile(r'(P\d{3})-([A-Za-z]+\d+)-(.+)')  # claim branch Pxxx-<batch>-<user>, or Pxxx-FU<yymmdd>-<user> for follow-ups
FOLLOW = re.compile(r'FU\d+')  # the batch part of a follow-up claim (user decision 2026-11-26)
LIMITS = re.compile(r'(#{2,3}) .*\b(Limits|Handoffs?)\b')  # an evidence document's limits sections (02-structure.md, Names)
DECIDED = re.compile(r'\buser decisions? \d{4}-\d\d-\d\d')  # a limit the user decided (02-structure.md, Names)
HANDED = re.compile(r'\bfollow-up on (P\d{3})\b')  # work a rule handed to another package: decided while its follow-up is open
PR_PAGE = 30  # open pull requests `waiting` lists: their commits must stay under GitHub's GraphQL node limit
TARGET = re.compile(r'`((?:\d{2,3}-[^`]+\.hpp)|(?:_\d{2,3}_[^`]+\.py))`')
# Re-attachment room (SUP33, SUP34; 00-harness-review.md): after a compaction Claude Code re-attaches each invoked skill's
# first 5,000 tokens, its `!` output included (docs, Skills). The scripts a skill's `!` lines run fit their output to what
# its text leaves, and 03-consistency.py warns (skill_reattach) below MIN_FACTS; every one of them reads these.
REATTACH_TOKENS = 5000
SKILL_CHARS_PER_TOKEN = 2.5  # skill text: 2.5 or more measured from transcript usage (user decision 2026-11-24)
OUTPUT_CHARS_PER_TOKEN = 2.0  # printed facts, any `!` output: gaps.py's median about 2.0 (user decision 2026-11-24)
MIN_FACTS = 2500  # the least room a `!` line's output may get
FLOOR = 1500  # the least room a script fits its output to
CUT = 160  # the least room in which fit() prints part of a first or kept line too long to print whole
SHORT = 8  # the characters a printed line counts at least: a very short line still takes its tokens and its newline
BANG = re.compile(r'^!`.*', re.M)


def load(root=ROOT):
    return [json.loads((root / PLAN / n).read_text()) for n in ('batches.json', 'packages.json')]


def save(root, data, name='packages.json'):
    (root / PLAN / name).write_text(json.dumps(data, indent=2, ensure_ascii=False) + '\n')


def cells(line):
    return [x.strip() for x in re.split(r'(?<!\\)\|', line.strip())[1:-1]]


def inventories(root=ROOT):
    """Map each numbered inventory target to (row cells after the name, tier)."""
    rows = {}
    for folder in sorted(p for p in root.iterdir() if p.is_dir() and re.match(r'0[1-8]-', p.name)):
        tier = 'Core' if folder.name == '01-Core' else None
        for line in (folder / '00-index.md').read_text().splitlines():
            if line in ('## Basic', '## Advanced', '## Esoteric'):
                tier = line[3:]
            match = TARGET.fullmatch(cells(line)[0]) if line.startswith('|') else None
            if match:
                rows.setdefault(folder.name + '/' + match[1], (cells(line)[1:], tier))
    return rows


def dependencies(root):
    spec = importlib.util.spec_from_file_location('library_dependencies', root / '98-Team Notebook/05-dependencies.py')
    deps = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(deps)
    return deps


def include_edges(root, batches, found=None):
    """(batch, prerequisite batch, header, include) for every include between headers that different batches own;
    `found` is a `05-dependencies.py` scan to reuse."""
    owner = {t: b['id'] for b in batches['batches'] for t in b['targets']}
    return [(owner[h], owner[d], h, d) for h, ds in dependencies(root).headers(root, found).items() if h in owner
            for d in ds if owner.get(d, owner[h]) != owner[h]]


def prereqs(batches, packages):
    """Package id -> earlier-owner package ids derived from batch prerequisites."""
    owner = {b: p['id'] for p in packages['packages'] for b in p['batches']}
    order = {p['id']: i for i, p in enumerate(packages['packages'])}
    bmap = {b['id']: b for b in batches['batches']}
    return {p['id']: sorted({owner[d] for b in p['batches'] if b in bmap for d in bmap[b]['prerequisites'] if d in owner} - {p['id']},
                            key=order.get) for p in packages['packages']}


def link(path):
    return f'[evidence](<{path}>)' if ' ' in path else f'[evidence]({path})'


def followups(p):
    """A package's open follow-ups (`followups` in packages.json, absent when none): work filed on it that leaves its status as it is."""
    return p.get('followups', [])


def marker(p):
    """The checklist box of a package: planned, in progress, verified, or verified with open follow-ups."""
    return {'planned': '[ ]', 'in-progress': '[-]'}.get(p['status'], '[!]' if followups(p) else '[x]')


def render_text(packages):
    out = [BEGIN, '', LEGEND]
    for phase in PHASES:
        out += ['', '### ' + phase, '']
        for p in packages['packages']:
            if p['phase'] == phase:
                head = f'**{p["id"]} — {", ".join(p["batches"])}** ({p["model"]}, {p["size"]}): {p["label"]}.'
                tail = ''.join([f' Note: {p["note"]}' if p['note'] else '', *(' ' + link(e) for e in p['evidence'])])
                fu = ' Follow-ups: ' + '; '.join(f'({i}) {x}' for i, x in enumerate(followups(p), 1)) + '.' if followups(p) else ''
                out.append(f'- {marker(p)} {head}{fu}' if p['status'] == 'verified' else f'- {marker(p)} {head} Status: {p["status"]}.{tail}{fu}')
    return '\n'.join(out + ['', END])


def rendered_file(root, packages):
    text = (root / '01-prompts.md').read_text() if (root / '01-prompts.md').exists() else ''
    if BEGIN in text and END in text:
        return text[:text.index(BEGIN)] + render_text(packages) + text[text.index(END) + len(END):]
    return text.rstrip('\n') + ('\n\n' if text.strip() else '') + render_text(packages) + '\n'


def rank(batches, packages):
    """Schedule order: phases in PHASES order; in each, started packages keep their order, then planned ones go by
    support pass first, then most basic tier (Basic or Core, Advanced, Esoteric), then most transitive open
    dependents, then current position, each after its prerequisites while both are open (`late`; an unsatisfiable
    one is left to `check`)."""
    bmap = {b['id']: b for b in batches['batches']}
    pl, deps = packages['packages'], prereqs(batches, packages)
    status, index = {p['id']: p['status'] for p in pl}, {p['id']: i for i, p in enumerate(pl)}
    users = collections.defaultdict(set)
    for p, ds in deps.items():
        for d in ds:
            users[d].add(p)

    def below(x, acc):
        for y in users[x] - acc:
            acc.add(y)
            below(y, acc)
        return acc

    def key(p):
        if p['status'] != 'planned':
            return (-2, 0, index[p['id']])
        tiers = [TIER_RANK.get(t, 2) for b in p['batches'] for t in bmap[b]['tiers']]
        return (min(tiers) if tiers else -1, -sum(status[y] != 'verified' for y in below(p['id'], set())), index[p['id']])

    out = []
    for phase in PHASES:
        todo = [p for p in pl if p['phase'] == phase]
        while todo:
            placed = {q['id'] for q in out}
            fit = [p for p in todo if all(d in placed or not late(status, d, p['id']) for d in deps[p['id']])] or todo[:1]
            out.append(min(fit, key=key))
            todo.remove(out[-1])
    packages['packages'] = out + [p for p in pl if p['phase'] not in PHASES]


def late(status, d, p):
    """Whether prerequisite d must precede p in the schedule: only while both are open."""
    return 'verified' not in (status[d], status[p])


def pending(p):
    """Whether a package has work left: open, or verified with open follow-ups."""
    return p['status'] != 'verified' or bool(followups(p))


def ready(batches, packages, skip=()):
    """The first package with work left whose prerequisites are verified, leaving out `skip` (ids `gate` blocks): a
    verified package with open follow-ups first, in schedule order, then the first open package."""
    status = {p['id']: p['status'] for p in packages['packages']}
    deps = prereqs(batches, packages)
    fit = [p for p in packages['packages'] if pending(p) and p['id'] not in skip and all(status[d] == 'verified' for d in deps[p['id']])]
    return next((p for p in fit if p['status'] == 'verified'), fit[0] if fit else None)


def gh(root, *args):
    """(stdout, '') of a gh command, or (None, reason) when gh is missing, fails or times out."""
    if not shutil.which('gh'):
        return None, 'gh not installed'
    try:
        r = subprocess.run(['gh', *args], cwd=root, capture_output=True, text=True, timeout=10)
    except subprocess.TimeoutExpired:
        return None, f'gh {args[0]} timed out'
    except OSError as e:
        return None, f'gh {args[0]} failed: {e.strerror}'
    return (r.stdout, '') if r.returncode == 0 else (None, (r.stderr.strip().splitlines() or [f'gh {args[0]} failed'])[0])


def config(root, key):
    return subprocess.run(['git', 'config', '--get', key], cwd=root, capture_output=True, text=True).stdout.strip()


def role(root):
    """This clone's role, `git config compprog.role`: contributor (the default) or maintainer."""
    r = config(root, 'compprog.role')
    if r not in ('', 'contributor', 'maintainer'):
        raise SystemExit(f'git config compprog.role is {r!r}; set contributor or maintainer')
    return r or 'contributor'


def login(root, ask=True):
    """The GitHub login through gh (only if `ask`), else `git config user.name`, as a branch-name slug."""
    out = (ask and gh(root, 'api', 'user', '--jq', '.login')[0] or '').strip() or config(root, 'user.name')
    return re.sub(r'[^A-Za-z0-9-]+', '-', out).strip('-') or 'user'


def claims(root, packages):
    """({package id: (author, branch, number, draft)}, SKIP reason): the open pull requests whose head branch is a claim
    branch `Pxxx-<batch>-<user>` of a batch Pxxx owns, in the canonical repository (`REPO`); the reason is empty unless
    gh could not list them."""
    out, why = gh(root, 'pr', 'list', '--repo', config(root, 'compprog.repo') or REPO, '--state', 'open', '--limit', '1000', '--json', 'number,headRefName,isDraft,author')
    try:
        prs = json.loads(out) if out is not None else None
    except ValueError:
        prs, why = None, 'gh pr list printed no JSON'
    if prs is None:
        return {}, why
    owner = {b: p['id'] for p in packages['packages'] for b in p['batches']}
    ids = {p['id'] for p in packages['packages']}
    res = {}
    for pr in sorted(prs, key=lambda pr: pr['number']):
        m = CLAIM.fullmatch(pr['headRefName'])
        if m and (owner.get(m[2].upper()) == m[1] or (m[1] in ids and FOLLOW.fullmatch(m[2].upper()))):
            res.setdefault(m[1], (pr['author']['login'], pr['headRefName'], pr['number'], pr['isDraft']))
    return res, ''


def gate(root, packages):
    """(role, login or '', claims, claims line, {package id: why this clone may not start it}): a package claimed by
    someone else's open pull request, and for a contributor a Core full package (model fable), which is maintainer-only.
    After a failed claim lookup the login skips gh, so the lookups take at most two gh timeouts."""
    r, (cl, why) = role(root), claims(root, packages)
    me = login(root, not why) if cl or r == 'contributor' else ''
    block = {pid: f'claimed by @{a} in {"draft " if d else ""}pull request #{n} ({b})'
             for pid, (a, b, n, d) in cl.items() if a.lower() != me.lower()}
    if r == 'contributor':
        block.update({p['id']: 'maintainer-only (Core full)' for p in packages['packages'] if p['model'] == 'fable' and p['id'] not in block})
    line = f'Role: {r}; claims: ' + (f'SKIP ({why})' if why else f'{len(cl)} open' if cl else 'none open')
    return r, me, cl, line, block


def current(root):
    return subprocess.run(['git', 'branch', '--show-current'], cwd=root, capture_output=True, text=True).stdout.strip()


def claim_branch(p, batch, branch, cl, me):
    """A contributor's claim branch for package p: their own open claim's branch, else the current `branch` when it is a
    claim branch of a batch or of the follow-ups of p, else `<id>-<batch>-<login>`."""
    if p['id'] in cl and cl[p['id']][0].lower() == me.lower():
        return cl[p['id']][1]
    m = CLAIM.fullmatch(branch or '')
    return branch if m and m[1] == p['id'] and (m[2].upper() in p['batches'] or FOLLOW.fullmatch(m[2].upper())) else f'{p["id"]}-{batch}-{me}'


def check(root=ROOT):
    errors = []
    require = lambda cond, msg: cond or errors.append(msg)
    batches, packages = load(root)
    bl, pl = batches['batches'], packages['packages']
    bmap = {b['id']: b for b in bl}
    require(len(bmap) == len(bl), 'Duplicate batch ids')
    for b in bl:
        for d in b['prerequisites']:
            require(d in bmap and d != b['id'], f'Unknown/self batch prerequisite: {b["id"]} -> {d}')
    state = {}

    def cycle(b, path):
        state[b] = 1
        for d in bmap[b]['prerequisites']:
            if state.get(d) == 1:
                require(False, 'Batch prerequisite cycle: ' + ' -> '.join(path[path.index(d):] + [d]))
            elif d in bmap and d not in state:
                cycle(d, path + [d])
        state[b] = 2
    for b in bmap:
        if b not in state:
            cycle(b, [b])
    ids = [p['id'] for p in pl]
    require(len(set(ids)) == len(ids) and all(re.fullmatch(r'P\d{3}', i) for i in ids), 'Package ids must be unique Pnnn; list order is the schedule')
    scheduled = collections.Counter(b for p in pl for b in p['batches'])
    require(scheduled == collections.Counter(list(bmap)), 'Batches not scheduled exactly once: '
            + str(sorted(set(scheduled.items()) ^ {(b, 1) for b in bmap})))
    order = {p: i for i, p in enumerate(ids)}
    deps, status = prereqs(batches, packages), {p['id']: p['status'] for p in pl}
    for p in pl:
        for d in deps[p['id']]:
            require(order[d] < order[p['id']] or not late(status, d, p['id']), f'Prerequisite scheduled late: {p["id"]} -> {d}')
        local = {b: i for i, b in enumerate(p['batches'])}
        for b in p['batches']:
            require(b in bmap, f'Unknown batch in {p["id"]}: {b}')
            for d in bmap.get(b, {}).get('prerequisites', []):
                require(d not in local or local[d] < local[b], f'Within-package batch order: {p["id"]} {d} before {b}')
        require(p['status'] in packages['statuses'], f'Bad status: {p["id"]} {p["status"]}')
        require(p['model'] in packages['models'], f'Bad model: {p["id"]} {p["model"]}')
        require(p['phase'] in PHASES, f'Bad phase: {p["id"]} {p["phase"]}')
        fu = p.get('followups', [])
        require(isinstance(fu, list) and fu and all(isinstance(x, str) and x.strip() and '\n' not in x for x in fu) and len(set(fu)) == len(fu)
                if 'followups' in p else True, f'Bad followups: {p["id"]} (a non-empty list of distinct one-line texts, absent when none)')
        for e in p['evidence']:
            require((root / e).exists(), f'Missing evidence: {p["id"]} -> {e}')
        if p['status'] == 'verified' and not fu and all(b in bmap for b in p['batches']):
            left = open_limits(root, p, bmap, open_ids(pl))
            first = re.match(r'(.*?:\d+): (\[[^]]*\])?', left[0]) if left else None
            require(not left, f'Undecided limits in verified {p["id"]}: {len(left)}, the first at {" ".join(filter(None, first.groups())) if first else ""}; '
                              f'file a follow-up (plan.py follow-up {p["id"]} ...) or decide them (02-structure.md, Names)')
    found = dependencies(root).scan(root)
    for b, d, h, inc in include_edges(root, batches, found):
        require(d in bmap[b]['prerequisites'], f'Undeclared include edge: {h} -> {inc}; add {d} to the prerequisites of {b} (plan.py doctor --fix)')
    for h, incs in found.items():
        for inc, cond in incs:
            require(not cond, f'Conditional include (the dependency graph treats every include as active): {h} -> {inc}')
    inv = inventories(root)
    owned = collections.Counter(t for b in bl for t in b['targets'])
    for t in inv:
        require(owned[t] == 1, f'Inventory target owned {owned[t]} times: {t}')
    for t in owned:
        require(t in inv, f'Batch target not in any inventory: {t}')
    for b in bl:
        require(set(b['tiers']) == {inv[t][1] for t in b['targets'] if t in inv}, f'Batch tier mismatch: {b["id"]}')
    require((root / '01-prompts.md').exists() and rendered_file(root, packages) == (root / '01-prompts.md').read_text(),
            '01-prompts.md checklist out of sync; run plan.py render')
    return errors


def stem(name):
    return re.sub(r'^_?\d{2,3}[-_]|\.(hpp|py)$', '', name)


def row_stem(name):
    """Prefix 00 is shared with folder-level files, so a 00 row's files must also match its stem."""
    return stem(name) if re.match(r'_?00[-_]', name) else ''


def tests_for(root, target):
    folder, name = target.split('/')
    prefix, base = re.search(r'\d+', name)[0], root / '96-Local Testing' / folder
    return sorted(str(p.relative_to(root)) for p in base.iterdir() if p.is_file()
                  and re.match(r'_?(\d+)[-_]', p.name) and re.match(r'_?(\d+)', p.name)[1] == prefix
                  and re.sub(r'^_?\d+[-_]', '', p.name).startswith(row_stem(name))) if base.is_dir() else []


def legacy_for(root, target):
    folder, name = target.split('/')
    index = root / folder / '97-Legacy/00-index.md'
    base = stem(name).lower()
    keys = {base, base.replace('_', ''), base.replace('_', ' ')}
    rows = [cells(l) for l in index.read_text().splitlines() if l.startswith('| [') and any(k in l.lower() for k in keys)] if index.exists() else []
    return [' — '.join([re.sub(r'\[[^]]*\]\(<?([^)>]+)>?\)', rf'`{folder}/97-Legacy/\1`', r[0]), *r[1:]]) for r in rows]


def docs_for(root, target):
    folder, name = target.split('/')
    prefix = re.search(r'\d+', name)[0]
    base = root / folder / 'Docs'
    return sorted(str(p.relative_to(root)) for p in base.glob(f'{prefix}-{row_stem(name)}*.md')) if base.is_dir() else []


def row_status(parts):
    """The status word of an inventory row's cells (`verified; [evidence](...)` -> `verified`)."""
    return parts[-1].split(';')[0].split(' ')[0].strip('*').lower()


def open_ids(packages):
    """The ids of the packages with open follow-ups: the ones a limit may hand work to (`follow-up on Pxxx`)."""
    return {p['id'] for p in packages if followups(p)}


def stale(text, handed):
    """The packages a limit item hands work to (HANDED) that are not in `handed`, the ids with open follow-ups."""
    return sorted(set(HANDED.findall(text)) - handed)


def undecided(doc, handed):
    """(line number, text) of each item under a limits heading (LIMITS) of an evidence document that names neither a
    `user decision <date>` nor only hand-offs (`follow-up on Pxxx`) to packages in `handed`, the ids with open follow-ups
    (02-structure.md, Names; user decision 2026-11-27): a list item with its indented continuation lines, a table body
    row or a paragraph; `none` items are skipped."""
    return [(i, t) for i, t in limit_items(doc) if not DECIDED.search(t) and (not HANDED.search(t) or stale(t, handed))]


def limit_items(doc):
    """(line number, text) of each item under a limits heading (LIMITS) of an evidence document, `none` items skipped."""
    out, level, item = [], 0, None
    lines = doc.read_text(errors='replace').splitlines() if doc.is_file() else []
    for i, line in enumerate(lines, 1):
        heading, text = re.match(r'(#+) ', line), line.strip()
        if heading and level and len(heading[1]) <= level:
            level = 0
        if heading and not level and LIMITS.match(line):
            level, item = len(heading[1]), None
        elif not level or heading or not text:
            item = None
        elif re.fullmatch(r'\|[\s|:-]+\|?', text):
            out = out[:-1] if item and item[1].startswith('|') else out  # the row above a rule is the table's header
            item = None
        elif item and not item[1].startswith('|') and (line[:1] in ' \t' or not re.match(r'([-*+]|\d+[.)]) |\|', text)):  # continued, nested items too
            item[1] += ' ' + text
        else:
            item = [i, text]
            out.append(item)
    return [(i, t) for i, t in out if not re.fullmatch(r'(([-*+]|\d+[.)]) )?none\.?', t, re.I)]


def package_docs(root, p, bmap):
    """The evidence documents of package p: its targets' and support targets' documents and its evidence paths."""
    return sorted({d for b in p['batches'] for t in bmap[b]['targets'] + bmap[b].get('support_targets', []) for d in docs_for(root, t)}
                  | set(p['evidence']))


def limit_line(d, i, t, handed):
    """`path:line: text` of a limit item, a hand-off without an open follow-up marked `[no open follow-up on Pxxx]`."""
    return f'{d}:{i}: ' + ''.join(f'[no open follow-up on {q}] ' for q in stale(t, handed)) + t[:140]


def open_limits(root, p, bmap, handed):
    """limit_line() of each undecided limit (undecided()) in the evidence documents of package p."""
    return [limit_line(d, i, t, handed) for d in package_docs(root, p, bmap) for i, t in undecided(root / d, handed)]


def citing(root, pid, packages, bmap):
    """limit_line() of each limit item in any package's evidence documents that hands work to pid (`follow-up on pid`),
    with whether it also names a `user decision <date>`, which keeps it decided once pid's follow-ups close."""
    docs = sorted({d for p in packages if all(b in bmap for b in p['batches']) for d in package_docs(root, p, bmap)})
    return [(f'{d}:{i}: {t[:140]}', bool(DECIDED.search(t))) for d in docs for i, t in limit_items(root / d) if pid in HANDED.findall(t)]


CANDIDATES, VERDICTS = '### Optimization Candidates', ('adopted', 'rejected', 'other owner', 'none found')
NO_SWEEP = ('01-Core/01-template.hpp', '01-Core/02-debug.hpp')  # boilerplate, not algorithms (01-principles.md, Profiles)


def sweeps(root=ROOT):
    """(pending, faults, named) for the verified Core full headers (05-testing.md, Benchmarks): those whose evidence
    lacks the candidates table, the faults of the tables that exist (shape, verdict words, undecided wording), and per
    header with a table the names its Operation column holds."""
    pending, faults, named = [], [], {}
    for target, (parts, _) in sorted(inventories(root).items()):
        if not target.startswith('01-Core/') or target.endswith('mini.hpp') or target in NO_SWEEP or row_status(parts) != 'verified':
            continue
        text = '\n'.join((root / d).read_text(encoding='utf-8') for d in docs_for(root, target))
        if CANDIDATES not in text:
            pending.append(target)
            continue
        rows = []
        for line in text.split(CANDIDATES, 1)[1].splitlines()[1:]:
            if line.startswith('#'):
                break
            if line.startswith('|'):
                rows.append(cells(line))
        where = f'Optimization Candidates (05-testing.md, Benchmarks): {target}: '
        if len(rows) < 3 or rows[0] != ['Operation', 'Candidate', 'Verdict', 'Evidence']:
            faults.append(where + 'no table `Operation | Candidate | Verdict | Evidence` with a row')
            continue
        named[target] = set()
        for r in rows[2:]:
            if len(r) != 4 or not r[2].lower().startswith(VERDICTS) or not r[3]:
                faults.append(where + 'row without a verdict (' + ', '.join(VERDICTS) + ') or its evidence: ' + ' | '.join(r)[:80])
            elif re.search(r'not tried|unmeasured|deferred|not measured|to be measured', ' '.join(r), re.I):
                faults.append(where + 'undecided wording: ' + ' | '.join(r)[:80])
            elif r[2].lower().startswith(VERDICTS[:2]) and not re.search(r'GCC \d+|argument:', r[3]):
                faults.append(where + 'verdict without its compiler (`GCC <n>`) or `argument:`: ' + ' | '.join(r)[:80])
            named[target] |= {x.strip() for x in re.sub(r'`', '', r[0]).split(',')}
    return pending, faults, named


def landed(root):
    """Keys whose `/finish` line in finish-log.tsv has the result `verified` (a contributor's line counts once merged)."""
    rows = [l.split('\t') for l in (root / PLAN / 'finish-log.tsv').read_text().splitlines()[1:]]
    return {r[1] for r in rows if len(r) > 7 and r[7] == 'verified'}


def batch_done(b, inv, done=frozenset()):
    """A batch with targets is done once every target's inventory row is verified; a support batch, once `/finish`
    has landed it (`done`, from `landed`)."""
    if not b['targets']:
        return b['id'] in done
    return all(row_status(inv.get(t, ([''],))[0]) == 'verified' for t in b['targets'])


def launch(root, model):
    """(launch command, session check) for a package model id: the `*-session` agent preset in `.claude/agents` with
    that model and its effort (one per preset, whatever the package size; CLAUDE.md, Models and Delegation), compared
    with the running session's preset and effort when Claude Code exports them."""
    preset = None
    for f in sorted((root / '.claude/agents').glob('*-session.md')):
        meta = dict(re.findall(r'^(\w+):[ \t]*(.+)$', f.read_text().split('---')[1], re.M))
        if meta.get('model') == model:
            preset = (meta.get('name', f.stem), meta.get('effort', '?'))
    if not preset:
        return f'no session preset uses {model}', ''
    agent, effort = os.environ.get('CLAUDE_CODE_AGENT'), os.environ.get('CLAUDE_EFFORT')
    differs = (agent and agent != preset[0]) or (effort and effort != preset[1])
    check = '' if not (agent or effort) else (f'this session: {agent or "no preset"}, effort {effort or "?"}'
                                              + (' — MISMATCH' if differs else ''))
    return f'claude --agent {preset[0]} --effort {preset[1]}', check


def batch_brief(root, b, inv):
    out = [f'### {b["id"]} ({b["size"]})', '', 'Focus: ' + b['focus'],
           'Batch prerequisites: ' + (', '.join(b['prerequisites']) or 'none')]
    every = b['targets'] + b.get('support_targets', [])
    for t in every:
        out += [f'- `{t}` [{"exists" if (root / t).exists() else "missing"}{", support" if t not in b["targets"] else ""}]',
                '  ' + ' — '.join(inv.get(t, (['(no inventory row)'],))[0])]
    tests = [x for t in every for x in tests_for(root, t)]
    docs = [x for t in every for x in docs_for(root, t)]
    legacy = [x for t in every for x in legacy_for(root, t)]
    out += ['Tests:' if tests else 'Tests: none'] + ['- `' + x + '`' for x in tests]
    out += ['Docs:' if docs else 'Docs: none (create Docs/<NN-name>.md)'] + ['- `' + x + '`' for x in docs]
    if legacy:
        out += ['Legacy:'] + ['- ' + x for x in dict.fromkeys(legacy)]
    return out + ['']


def hook_warning(root):
    """The brief's last line while git does not run the commit hook here (CLAUDE.md, Start Here), else None; /setup-check's
    hook_check decides, and a checkout without it gets none."""
    path = root / '.claude/skills/setup-check/setup_check.py'
    if not path.is_file():
        return None
    spec = importlib.util.spec_from_file_location('plan_setup_check', path)
    check = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(check)
    ok, detail = check.hook_check(root)
    return None if ok else f'WARNING: commit hook off ({detail}). Give the user that command; this warning does not stop the run.'


def room(skill_text):
    """Characters each `!` line of a skill may print: what its text leaves of the re-attached tokens, shared equally."""
    left = (REATTACH_TOKENS - len(skill_text) / SKILL_CHARS_PER_TOKEN) * OUTPUT_CHARS_PER_TOKEN
    return int(left // max(1, len(BANG.findall(skill_text))))


def room_of(skill_md):
    """The room a script fits its output to: room() of that SKILL.md (an empty one when missing), at least FLOOR."""
    return max(room(skill_md.read_text() if skill_md.is_file() else ''), FLOOR)


def spans(numbers):
    """'3, 7-40, 52' of ascending numbers, the first six runs only."""
    runs = []
    for n in numbers:
        if runs and runs[-1][1] == n - 1:
            runs[-1][1] = n
        else:
            runs.append([n, n])
    text = ', '.join(f'{a}-{b}' if b > a else str(a) for a, b in runs[:6])
    return text + (' ...' if len(runs) > 6 else '')


def cost(line):
    """The room a printed line takes: its characters and newline, at least SHORT."""
    return max(len(line) + 1, SHORT)


def fit(lines, size, rest, keep=lambda line: False):
    """`lines` printed one per line within `size` characters (at least len(rest) + 140 when they do not all fit), each
    counted by cost() (SUP34): all when they fit, else whole lines, the first and
    those `keep` marks chosen before the others, kept in their order; then the first and kept lines too long for that
    are cut to what is left, when at least CUT characters are (a brief's Focus); then one line numbering the lines cut
    or left out and naming `rest`, the file or command that holds them all."""
    if sum(map(cost, lines)) <= size:
        return list(lines)
    chosen, left = {}, size - len(rest) - 140  # the closing line: at most 133 characters besides `rest`
    order = sorted(range(len(lines)), key=lambda i: (i > 0 and not keep(lines[i]), i))
    for i in order:
        if cost(lines[i]) <= left:
            chosen[i] = lines[i]
            left -= cost(lines[i])
    for i in order:
        if i not in chosen and (i == 0 or keep(lines[i])) and left >= CUT:
            chosen[i] = lines[i][:left - 3] + ' …'
            left -= cost(chosen[i])
    cut = [i + 1 for i in range(len(lines)) if chosen.get(i) != lines[i]]
    return [chosen[i] for i in sorted(chosen)] + [f'... {len(cut)} of {len(lines)} lines cut or not printed (lines {spans(cut)}): {rest}']


def spill(name, text):
    """Write a cut output whole to <name>.txt in this session's scratchpad (CLAUDE.md, Output economy) when
    CLAUDE_CODE_SESSION_ID names one, else in the temp directory's claude-<uid> folder; its path, or None if unwritable."""
    base, sid = Path(tempfile.gettempdir()) / f'claude-{os.getuid()}', os.environ.get('CLAUDE_CODE_SESSION_ID', '')
    pads = sorted(p for p in base.glob(f'*/{sid}') if p.is_dir()) if re.fullmatch(r'[\w-]+', sid) else []
    path = (pads[0] if pads else base) / f'{name}.txt'
    try:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)
    except OSError:
        return None
    return path


def reattach(root):
    """03-consistency.py's skill_reattach warnings (SUP33, SUP34): a skill whose `!` lines get under MIN_FACTS characters
    each, any other above nine tenths of the characters re-attached, and an unpiped `plan.py show` line not fitted to its
    own skill with `--skill <name>`."""
    out, whole = [], REATTACH_TOKENS * SKILL_CHARS_PER_TOKEN
    for skill in sorted((root / '.claude/skills').glob('*/SKILL.md')):
        text, rel, name = skill.read_text(), skill.relative_to(root), skill.parent.name
        bangs = BANG.findall(text)
        if bangs and (r := room(text)) < MIN_FACTS:
            out.append(f'{rel}: leaves {r} characters for the output of each of its {len(bangs)} `!` line(s) (at least {MIN_FACTS})')
        elif not bangs and len(text) > 0.9 * whole:
            out.append(f'{rel}: {len(text)} characters, near the {whole:g} re-attached after a compaction')
        out += [f'{rel}: `plan.py show` not fitted to this skill: add --skill {name}' for line in bangs
                if re.search(r'plan\.py"? show\b', line) and '|' not in line and f'--skill {name} ' not in line + ' ']
    return out


def show(root, ident):
    batches, packages = load(root)
    bmap = {b['id']: b for b in batches['batches']}
    pmap = {p['id']: p for p in packages['packages']}
    inv, fin = inventories(root), landed(root)
    r, me, cl, line, block = gate(root, packages)
    if ident == 'next':
        p = ready(batches, packages, block)
        if not p:
            raise SystemExit('No package is ready')
        ident = p['id']
    owner = {b: p for p in packages['packages'] for b in p['batches']}
    if ident in bmap:
        p, shown = owner[ident], [ident]
    elif ident in pmap:
        p = pmap[ident]
        open_ = [b for b in p['batches'] if not batch_done(bmap[b], inv, fin)]
        shown = [] if len(p['batches']) > 1 and open_ and p['status'] != 'verified' else p['batches']
    else:
        raise SystemExit(f'Unknown package or batch: {ident}')
    command, check = launch(root, packages['models'][p['model']])
    fu = followups(p)
    out = [f'{p["id"]} — {p["label"]} | {p["phase"]} | {p["size"]} | {p["status"]}' + (f' with {len(fu)} open follow-up(s)' if fu else '')
           + f' | model: {p["model"]} ({packages["models"][p["model"]]}) | launch: {command}' + (f' | {check}' if check else ''), line]
    batch = shown[0] if len(shown) == 1 else next((b for b in p['batches'] if not batch_done(bmap[b], inv, fin)), p['batches'][0])
    if p['status'] == 'verified' and fu:  # a follow-up claim (user decision 2026-11-26)
        batch = f'FU{datetime.date.today():%y%m%d}'
    if p['id'] in block:
        out.append(f'STOP: {p["id"]} is {block[p["id"]]}; pick another package (`plan.py ready`).')
    elif p['id'] in cl:
        b = cl[p['id']][1]
        out.append(f'Claimed by you: pull request #{cl[p["id"]][2]} on {b}'
                   + ('.' if current(root) == b else f'; work on it after `git fetch origin {b} && git switch {b}`.'))
    elif r == 'contributor':
        b, here = claim_branch(p, batch, current(root), cl, me), current(root)
        out.append(('Claims unknown: check the open pull requests, then claim' if 'SKIP' in line else 'Unclaimed: claim')
                   + (' it on this branch: `' if here == b else f" it in the checkout the session will work in, from an up-to-date master: `git switch -c {b} && ")
                   + f"git commit "
                   f"--allow-empty -m '{p['id']}: claim {batch}' && CP_USER=1 git push -u origin {b} && gh pr create --draft --fill "
                   f"--repo {config(root, 'compprog.repo') or REPO}`.")
    out.append('')
    if fu:
        out += [('Follow-ups: the package stays verified; /package runs only these and closes each one done with '
                 f'`plan.py follow-up {p["id"]} --done <n>`:' if p['status'] == 'verified' else 'Follow-ups (part of this package\'s work):'),
                *(f'{i}. {x}' for i, x in enumerate(fu, 1)), '']
    if len(p['batches']) > 1:
        out += ['Batches: ' + ', '.join(f'{b} ({"done" if batch_done(bmap[b], inv, fin) else "this brief" if shown == [b] else "open"})'
                                        for b in p['batches'])
                + (f'. One batch per session, under its batch id: run `/package {batch}`, which lands it when nothing is open, else `/clear`, then `/finish {batch}`.' if not shown else ''), '']
    deps = prereqs(batches, packages)[p['id']]
    out.append('Prerequisite packages:' if deps else 'Prerequisite packages: none')
    out += [f'- {d} ({pmap[d]["status"]}) — {pmap[d]["label"]}' for d in deps]
    out += [f'WARNING: prerequisite {d} is {pmap[d]["status"]}; its APIs may be missing or unstable.'
            for d in deps if pmap[d]['status'] in ('planned', 'in-progress')]
    out.append('')
    for b in shown:
        out += batch_brief(root, bmap[b], inv)
    out.append('Evidence: ' + (' '.join(link(e) for e in p['evidence']) or 'none'))
    if p['note']:
        out.append('Note: ' + p['note'])
    if warning := hook_warning(root):
        out.append(warning)
    return '\n'.join(out)


def status_text(root):
    batches, packages = load(root)
    sts = packages['statuses']
    count = collections.Counter((p['phase'], p['status']) for p in packages['packages'])
    out = ['| Phase | ' + ' | '.join(sts) + ' | total |', '|---' * (len(sts) + 2) + '|']
    for ph in PHASES + ['Total']:
        row = [sum(n for (f, s), n in count.items() if s == st and ph in (f, 'Total')) for st in sts]
        out.append(f'| {ph} | ' + ' | '.join(map(str, row)) + f' | {sum(row)} |')
    _, _, _, line, block = gate(root, packages)
    p = ready(batches, packages, block)
    return '\n'.join(out + ['', line, f'Next: {p["id"]} — {p["label"]}' if p else 'Next: none ready (all verified, blocked or claimed).'])


def live(root, batches, packages):
    """Package ids being worked on in this clone: named by a linked worktree's branch or directory name (`P032`, `DS27`), or owning
    a file with uncommitted changes in the main checkout (a batch target, its tests and docs, or package evidence)."""
    git = lambda *a: subprocess.run(['git', *a], cwd=root, capture_output=True, text=True).stdout
    bmap = {b['id']: b for b in batches['batches']}
    owner = {b: p['id'] for p in packages['packages'] for b in p['batches']}
    names = [Path(v).name if k == 'worktree' else v.removeprefix('refs/heads/')
             for block in git('worktree', 'list', '--porcelain').split('\n\n')[1:]
             for k, _, v in (l.partition(' ') for l in block.splitlines()) if k in ('worktree', 'branch')]
    names = ' '.join(f'{m[1]}-{m[2]}' if (m := CLAIM.fullmatch(n)) else n for n in names)  # a claim's user part names nothing
    ids = {p['id'] for p in packages['packages']}
    out = {owner.get(t, t) for t in map(str.upper, re.findall(r'(?<![A-Za-z0-9])[A-Za-z]+\d+(?![A-Za-z0-9])', names)) if t in owner or t in ids}
    changed = set(filter(None, (git('diff', '--name-only', '-z', 'HEAD') + git('ls-files', '-oz', '--exclude-standard')).split('\0')))
    for p in packages['packages']:
        if pending(p) and p['id'] not in out:
            files = set(p['evidence']) | {f for b in p['batches'] for t in bmap[b]['targets'] + bmap[b].get('support_targets', [])
                                          for f in [t, *docs_for(root, t), *tests_for(root, t)]}
            if files & changed:
                out.add(p['id'])
    return out


def ready_list(root=ROOT):
    """Packages with work left whose prerequisites are all verified, verified ones with open follow-ups first, then in
    schedule order: (package, folders, live here, next open batch of a multi-batch package, its done batch count, claim or
    block text), plus the `gate` claims line."""
    batches, packages = load(root)
    bmap = {b['id']: b for b in batches['batches']}
    status = {p['id']: p['status'] for p in packages['packages']}
    deps, busy, inv = prereqs(batches, packages), live(root, batches, packages), inventories(root)
    fin, (_, _, cl, line, block) = landed(root), gate(root, packages)
    out = []
    for p in sorted(packages['packages'], key=lambda p: p['status'] != 'verified'):  # stable: schedule order within each part
        if not pending(p) or not all(status[d] == 'verified' for d in deps[p['id']]):
            continue
        folders = sorted({bmap[b]['folder'] for b in p['batches']} | {t.split('/')[0] for b in p['batches'] for t in bmap[b]['targets']})
        done = [b for b in p['batches'] if batch_done(bmap[b], inv, fin)]
        nxt = next((b for b in p['batches'] if b not in done), None) if len(p['batches']) > 1 else None
        held = block.get(p['id'], '')
        if p['id'] in cl and not held.startswith('claimed'):
            held = '; '.join(filter(None, [held, f'claimed by you in pull request #{cl[p["id"]][2]}']))
        out.append((p, [f for f in folders if f != 'support'] or ['support'], p['id'] in busy, nxt, len(done), held))
    return out, line


def ready_text(root, limit):
    """The ready list and a disjoint set: live and claimed packages reserve their folders, blocked ones are left out."""
    rows, line = ready_list(root)
    out = [line, 'Ready packages (prerequisites satisfied), schedule order:']
    out += [f'- {p["id"]} [{p["status"]}{f", {len(followups(p))} follow-up(s)" if followups(p) else ""}, {p["model"]}, {p["size"]}] {", ".join(f)} — {p["label"]}'
            + (' — live here' if busy else f' — next {nxt} ({done}/{len(p["batches"])} batches done)' if nxt and done else '')
            + (f' — {held}' if held else '')
            for p, f, busy, nxt, done, held in rows]
    used = {x for _, f, busy, _, _, held in rows if busy or 'claimed' in held for x in f}
    chosen = []
    for p, f, busy, nxt, done, held in rows:
        if len(chosen) == limit:
            break
        if not busy and not held and not (set(f) & used):
            chosen.append((p, nxt, done)); used |= set(f)
    out += ['', f'Disjoint set for {limit} parallel sessions (one folder each; use `claude -w <name>` worktrees and merge after):']
    out += [f'- {p["id"]} — ' + (f'`/package {nxt}` ({done}/{len(p["batches"])} batches done) — ' if nxt else '') + p['label']
            for p, nxt, done in chosen]
    busy = [p['id'] for p, _, b, _, _, _ in rows if b]
    out += [f'Live here (worktree or uncommitted files), folders reserved: {", ".join(busy)}.'] if busy else []
    out += ['', 'Shared files every session touches (expect small merges): 01-prompts.md, 00-Guidelines/13-Plan/packages.json, '
            '96-Local Testing/00-index.md, 96-Local Testing/01-run.py. Same-folder packages also share 00-index.md and the 98/99 aggregates; run those sequentially.']
    return '\n'.join(out)


def waiting(root):
    """What waits on this clone's user, then what waits on others, derived from git, gh and the skills' ledgers: the
    current branch's unpushed commits; open pull requests (the maintainer's `/merge-pr`, a contributor's next batch,
    `gh pr ready` once every batch has a `Pxxx: ... — verified` commit, requested changes); `ask` rows of /finish
    ledgers; verified Core full headers without a candidates table (maintainer); /merge-pr checkouts."""
    r, mine, others = role(root), [], []
    run = lambda *a: subprocess.run(['git', *a], cwd=root, capture_output=True, text=True)
    branch, up = current(root), run('rev-parse', '--abbrev-ref', '--symbolic-full-name', '@{upstream}')
    ahead = run('rev-list', '--count', '@{upstream}..HEAD').stdout.strip() if up.returncode == 0 else ''
    if ahead not in ('', '0'):
        mine.append(f'push: {branch} is {ahead} commit(s) ahead of {up.stdout.strip()}')
    elif up.returncode and CLAIM.fullmatch(branch or ''):
        mine.append(f'push: {branch} has no upstream yet')
    _, packages = load(root)
    pmap = {p['id']: p for p in packages['packages']}
    owner = {b: p['id'] for p in packages['packages'] for b in p['batches']}
    out, why = gh(root, 'pr', 'list', '--repo', config(root, 'compprog.repo') or REPO, '--state', 'open', '--limit', str(PR_PAGE),
                  '--json', 'number,title,headRefName,isDraft,author,reviewDecision,commits')
    try:
        prs = json.loads(out) if out is not None else None
    except ValueError:
        prs, why = None, 'gh pr list printed no JSON'
    if prs is None:
        mine.append(f'pull requests: SKIP ({why})')
    elif len(prs) >= PR_PAGE:
        others.append(f'only the newest {PR_PAGE} open pull requests are listed')
    me = login(root) if prs and r == 'contributor' else ''
    for pr in sorted(prs or [], key=lambda pr: pr['number']):
        n, who, m = pr['number'], pr['author']['login'], CLAIM.fullmatch(pr['headRefName'])
        fu = bool(m and FOLLOW.fullmatch(m[2].upper()))  # a follow-up claim is one unit, run as `/package Pxxx`
        p = pmap.get(m[1]) if m and (fu or owner.get(m[2].upper()) == m[1]) else None
        what, nxt, prog = f'{p["id"]} ({m[2].upper()})' if p else f'"{pr["title"]}"', None, ''
        if p and 'commits' in pr:
            heads = [c.get('messageHeadline', '') for c in pr['commits']]
            units = [m[2].upper()] if fu else p['batches']
            done = [b for b in units if any(h.startswith(p['id'] + ': ') and h.endswith(' — verified')
                                            and (len(units) == 1 or re.match(rf'{p["id"]}: {b}\b', h, re.I)) for h in heads)]
            nxt = next((p['id'] if fu else b for b in units if b not in done), None)
            prog = f', {len(done)} of {len(units)} {"follow-up runs" if fu else "batches"} verified' + (f', next {nxt}' if nxt else '')
        changes = pr.get('reviewDecision') == 'CHANGES_REQUESTED'
        if r == 'maintainer':
            line = f'#{n} {what} by @{who}'
            if pr['isDraft'] or changes:
                others.append(f'{line}: ' + ('draft' + prog if pr['isDraft'] else 'changes requested'))
            else:
                mine.append(f'`/merge-pr {n}`: {line}')
        elif who.lower() == me.lower():
            line = f'#{n} {what}'
            if changes:
                mine.append(f'{line}: changes requested; address the review')
            elif not pr['isDraft']:
                others.append(f"{line}: waiting for the maintainer's /merge-pr")
            elif prog and not nxt:
                mine.append(f'`gh pr ready {n}`: {line}, every batch verified')
            else:
                mine.append(f'`/package {nxt}`: {line}{prog}' if nxt else f'{line}: draft')
    for f in sorted((root / '.claude/finish').glob('*.txt')):
        k = sum(l.rsplit('|', 1)[-1].strip() == 'ask' for l in f.read_text().splitlines())
        if k:
            mine.append(f'answer: `/finish {f.stem}` has {k} open question(s)')
    pending = sweeps(root)[0]
    if pending and r == 'maintainer':
        mine.append('optimization sweep pending (05-testing.md, Benchmarks): ' + ', '.join(pending))
    for f in sorted((root / '.claude/merge-pr').glob('*.json'), key=lambda f: (len(f.stem), f.stem)):
        try:
            wt = json.loads(f.read_text()).get('worktree')
        except (ValueError, AttributeError):
            continue
        if wt and Path(wt).is_dir():
            mine.append(f'`/merge-pr {f.stem}`: checked out at {wt}; land or drop it')
    return '\n'.join([f'Role: {r}', 'Waiting on you:' + ('' if mine else ' nothing'), *('- ' + x for x in mine)]
                     + (['Waiting on others:', *('- ' + x for x in others)] if others else []))


def doctor(root=ROOT, fix=False):
    """Cross-check plan, inventories, files and tests; --fix adds include-derived batch prerequisites and re-renders the
    checklist. Returns (todo lines, fixed lines)."""
    import ast
    batches, packages = load(root)
    bmap = {b['id']: b for b in batches['batches']}
    inv = inventories(root)
    owner = {t: p['id'] for p in packages['packages'] for b in p['batches'] for t in bmap[b]['targets']}
    batch_of = {t: b for b in bmap for t in bmap[b]['targets']}
    pstatus = {p['id']: p['status'] for p in packages['packages']}
    pmap = {p['id']: p for p in packages['packages']}
    todo, fixed = collections.defaultdict(list), []
    missing = {(b, d) for b, d, _, _ in include_edges(root, batches) if d not in bmap[b]['prerequisites']}
    if fix and missing:
        for b, d in sorted(missing):
            bmap[b]['prerequisites'].append(d)
        save(root, batches, 'batches.json')
        fixed.append(f'Added {len(missing)} include-derived batch prerequisites: ' + ', '.join(f'{b} -> {d}' for b, d in sorted(missing)))
    for e in check(root):
        if 'out of sync' in e and fix:
            (root / '01-prompts.md').write_text(rendered_file(root, packages)); fixed.append('Re-rendered 01-prompts.md')
        else:
            todo['plan'].append(e)
    run = (root / '96-Local Testing/01-run.py').read_text()
    tree = ast.parse(run)
    quick = next((ast.literal_eval(n.value) for n in tree.body if isinstance(n, ast.Assign) and any(getattr(t, 'id', '') == 'QUICK' for t in n.targets)), set())
    quick_folders = set(re.findall(r"'(0\d-[^']+)'", run.split('x.parent.name in', 1)[1].split(']', 1)[0])) if 'x.parent.name in' in run else set()
    for t, (parts, _) in inv.items():
        folder, name = t.split('/')
        rstatus = row_status(parts)
        exists = (root / t).exists()
        pid = owner.get(t, '?'); ps = pstatus.get(pid)
        key = f'{pid} ({folder})'
        if rstatus == 'planned' and exists:
            todo[key].append(f'{name}: file exists but row is planned; set existing-unverified or partial')
        if rstatus in ('verified', 'partial', 'existing-unverified') and not exists:
            todo[key].append(f'{name}: row is {rstatus} but the file is missing')
        if rstatus == 'verified' and ps in ('planned', 'in-progress') and not (ps == 'in-progress' and batch_done(bmap[batch_of[t]], inv)):
            todo[key].append(f'{name}: row verified but package {pid} is {ps}; finish the package or downgrade the row')
        if ps == 'verified' and rstatus not in ('verified', 'partial'):
            todo[key].append(f'{name}: package verified but row is {rstatus}')
        if ps == 'verified' and rstatus == 'partial' and not followups(pmap[pid]):
            todo[key].append(f'{name}: partial row inside a verified package; operations still missing (see missing:)')
        tests = tests_for(root, t)
        if rstatus == 'verified' and not tests:
            todo[key].append(f'{name}: verified without a tester under 96-Local Testing/{folder}/')
        for x in tests:
            tname = x.split('/')[-1]
            if rstatus == 'verified' and tname.endswith('_tester.py') and '_compile_' not in tname and tname not in quick and folder not in quick_folders:
                todo[key].append(f'{tname}: tester not registered for quick mode in 96-Local Testing/01-run.py')
        if rstatus in ('verified', 'partial') and not re.search(r'\]\(', parts[-1]):
            todo[key].append(f'{name}: {rstatus} row has no evidence link')
    for p in packages['packages']:
        for e in p['evidence']:
            if not (root / e).exists():
                todo[p['id']].append(f'evidence path missing: {e}')
        if p['status'] == 'in-progress' and not p['note']:
            todo[p['id']].append('in-progress without a handoff note; add one with plan.py set --note')
    return todo, fixed


def doctor_text(root, fix):
    todo, fixed = doctor(root, fix)
    out = [f'Fixed: {f}' for f in fixed]
    if not todo:
        return '\n'.join(out + ['doctor: no inconsistencies'])
    for key in sorted(todo):
        out += ['', f'## {key}'] + ['- ' + x for x in todo[key]]
    out += ['', 'Resolve with /package Pxxx, /restyle Pxxx for pure style, or edit the row/plan; then rerun doctor.']
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sp = sub.add_parser('show')
    sp.add_argument('id')
    sp.add_argument('--skill', help="fit the brief to the room this skill's `!` line gets after a compaction")
    for name in ('status', 'next', 'render', 'rank', 'check', 'waiting'):
        sub.add_parser(name)
    sub.add_parser('ready').add_argument('--max', type=int, default=3, help='size of the disjoint parallel set')
    sub.add_parser('doctor').add_argument('--fix', action='store_true', help='add include-derived prerequisites, re-render the checklist')
    s = sub.add_parser('set')
    s.add_argument('id')
    s.add_argument('status')
    s.add_argument('--evidence', nargs='+', default=[], help='append repo-relative evidence paths')
    s.add_argument('--note', help='replace the package note')
    f = sub.add_parser('follow-up', help='file a follow-up on a package (its status stays), or close done ones')
    f.add_argument('id')
    f.add_argument('text', nargs='?', help='one line: the work, its reason and who filed it')
    f.add_argument('--done', type=int, nargs='+', metavar='N', help='close the follow-ups with these numbers (as `show` lists them)')
    a = ap.parse_args()
    if a.cmd == 'show':
        lines, skill = show(ROOT, a.id).split('\n'), ROOT / '.claude/skills' / (a.skill or '') / 'SKILL.md'
        if a.skill and not skill.is_file():
            raise SystemExit(f'No skill {a.skill!r}')
        print('\n'.join(fit(lines, room_of(skill), f"run `python3 '{PLAN}/plan.py' show {a.id}` for the whole brief before acting on it",
                            lambda line: line.startswith(('WARNING', 'STOP', 'Focus: '))) if a.skill else lines))
    elif a.cmd == 'status':
        print(status_text(ROOT))
    elif a.cmd == 'next':
        batches, packages = load(ROOT)
        p = ready(batches, packages, gate(ROOT, packages)[4])
        if not p:
            return 1
        print(p['id'])
    elif a.cmd == 'ready':
        print(ready_text(ROOT, a.max))
    elif a.cmd == 'waiting':
        print(waiting(ROOT))
    elif a.cmd == 'doctor':
        print(doctor_text(ROOT, a.fix))
        return bool(doctor(ROOT)[0])
    elif a.cmd == 'check':
        errors = check(ROOT)
        print('\n'.join(errors) or 'plan check: ok')
        return bool(errors)
    else:
        batches, packages = load(ROOT)
        if a.cmd == 'rank':
            rank(batches, packages)
            save(ROOT, packages)
        if a.cmd == 'follow-up':
            p = next((p for p in packages['packages'] if p['id'] == a.id), None)
            if p is None or (a.text is None) == (a.done is None):
                raise SystemExit(f'Unknown package {a.id}' if p is None else 'Give the follow-up text or --done N..., not both')
            fu, cites = followups(p), []
            if a.text is not None:
                text = ' '.join(a.text.split())
                if not text or text in fu:
                    raise SystemExit('Empty follow-up' if not text else f'{a.id} already has this follow-up')
                fu = [*fu, text]
            else:
                if not set(a.done) <= set(range(1, len(fu) + 1)):
                    raise SystemExit(f'{a.id} has follow-ups 1 to {len(fu)}' if fu else f'{a.id} has no open follow-up')
                fu = [x for i, x in enumerate(fu, 1) if i not in a.done]
                bmap, handed = {b['id']: b for b in batches['batches']}, open_ids(packages['packages']) - ({a.id} if not fu else set())
                if not fu and p['status'] == 'verified' and (left := open_limits(ROOT, p, bmap, handed)):
                    raise SystemExit(f'{a.id} keeps its last follow-up while {len(left)} limit(s) lack a `user decision <date>` '
                                     '(02-structure.md, Names):\n  ' + '\n  '.join(left))
                cites = citing(ROOT, a.id, packages['packages'], bmap)
                if not fu and (left := [c for c, decided in cites if not decided]):
                    raise SystemExit(f'{a.id} keeps its last follow-up while {len(left)} limit(s) hand work to it (`follow-up on {a.id}`) '
                                     'without a `user decision <date>`: delete each item the closed follow-ups settled, decide the rest '
                                     'or hand it on anew (02-structure.md, Names):\n  ' + '\n  '.join(left))
            p.pop('followups', None)
            if fu:
                p['followups'] = fu
            save(ROOT, packages)
            print('\n'.join([f'{a.id}: {len(fu)} open follow-up(s)', *(f'{i}. {x}' for i, x in enumerate(fu, 1))]
                            + ([f'Limits citing `follow-up on {a.id}`: delete or update each the closed follow-up(s) settled:',
                                *(f'  {c}' for c, _ in cites)] if cites else [])))
        if a.cmd == 'set':
            p = next((p for p in packages['packages'] if p['id'] == a.id), None)
            if p is None or a.status not in packages['statuses']:
                raise SystemExit(f'Unknown package {a.id}' if p is None else f'Status must be one of {packages["statuses"]}')
            missing = [e for e in a.evidence if not (ROOT / e).exists()]
            if missing:
                raise SystemExit(f'Missing evidence paths: {missing}')
            bmap = {b['id']: b for b in batches['batches']}
            covered = p['status'] == 'verified' and followups(p)  # open follow-ups cover its limits, as in check()
            if a.status == 'verified' and not covered and (left := open_limits(ROOT, {**p, 'evidence': p['evidence'] + a.evidence}, bmap,
                                                                                open_ids(packages['packages']))):
                raise SystemExit(f'{a.id} not set verified: {len(left)} limit(s) without a `user decision <date>` (02-structure.md, Names). '
                                 'Fix each that the package can do, move what the algorithm or contract implies to Contracts, and '
                                 'ask the user about the rest:\n  ' + '\n  '.join(left))
            p['status'] = a.status
            p['evidence'] += [e for e in a.evidence if e not in p['evidence']]
            if a.note is not None:
                p['note'] = a.note
            rank(batches, packages)  # a started package moves ahead of planned ones
            save(ROOT, packages)
        (ROOT / '01-prompts.md').write_text(rendered_file(ROOT, packages))
        print(f'Rendered 01-prompts.md ({len(packages["packages"])} packages)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
