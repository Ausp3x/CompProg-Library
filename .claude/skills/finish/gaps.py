#!/usr/bin/env python3
"""Facts for /finish: target, ledger, brief, recorded gaps, branch changes, overlap with other worktrees.

Usage: gaps.py [Pxxx | batch id]. Without an argument the target comes from the batch or package id in the
branch or worktree name (a batch id wins); with neither, the target is the branch itself.
gaps.py --reviewed <Pxxx | batch id> [--wrong] records /package's review (step 10): the blobs of the targets, their tests
and evidence documents (stored in git's object database), the reviewer's report read from standard input, and with --wrong
that the review confirmed a wrong answer or undefined behavior. /finish reuses the report while those files are unchanged,
reviews only their diff since the review otherwise (written to .claude/finish/<key>.since-review.diff), and runs a full
audit only without a record or after --wrong (user decision 2026-11-26).
gaps.py --start <Pxxx | batch id>, run by /package's `!` line, records once per key the files uncommitted when the batch
starts (.claude/finish/<key>.start.json, deleted with the ledger), which a batch without targets leaves out of its review
while they keep that content: in the main checkout they are other work's (SUP34).

The facts go in full to .claude/finish/<key>.facts.txt; the printed copy, which /finish's SKILL.md inlines, holds the target,
the ledger's open rows and then whole sections in order of need until the room the skill leaves in the 5,000 tokens Claude
Code re-attaches after a compaction runs out (SUP33; plan.room_of). A ledger line that is not an `id | defect or decision | file:line |
gap | source | state` row is reported as stray: experiments go one row per attempt to .claude/finish/<key>.lab.tsv.
"""
import importlib.util
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / '00-Guidelines/13-Plan'))
import plan  # noqa: E402
from worktree import uncommitted  # noqa: E402

LIMIT_HEADING = re.compile(r'^#+ .*\b(limit|handoff|open|gap|omission|todo|remaining|known)', re.I)
DEFERRAL = re.compile(r'left out|not adopted|omit|out of scope|defer|not implemented|not fetched|later|filed', re.I)
DECIDED = re.compile(r'by the user|user (decision|decided|chose|asked)', re.I)
SETTLED = ('Recorded Decisions', 'Kept by the Sweep', 'Conventions')
TABLE_RULE = re.compile(r'^\|[\s|:-]+\|?$')
ROW_REF = re.compile(r'\brows?\s+(\d+(?:\s*(?:,|and|/|–|-)\s*\d+)*)')
PREFIX_REF = re.compile(r'(?<![\w.])_(\d{2})(?!\d)|\b(\d{2})-[a-z]')  # `_03` modules, `21-lichao` stems
CORE_FULL = re.compile(r'01-Core/\d+-(?!\w*mini\.hpp)[\w-]+\.hpp')  # 01-principles.md, Profiles
UNDECIDED = re.compile(r'\b(not tried|untried|unmeasured|not measured|never measured|not re-?measured|deferred|until a benchmark|to be measured)\b', re.I)
MARKER = re.compile(r'(#|//|<!--|^\s*([-*]\s*)?)\s*(TODO|FIXME|XXX)\b')
ROW_ID = re.compile(r'[A-Za-z0-9][\w.-]*')
STATES = ('open', 'ask', 'fixed', 'answered')  # SKILL.md, Ledger; the first two are open
LINES = 12  # printed lines per section; the facts file holds up to 400, and every recorded limit


def row(line):
    """The state of a well-formed ledger row (SKILL.md, Ledger), else None."""
    f = [x.strip() for x in line.split('|')]
    state = (re.match(r'[A-Za-z]*', f[5])[0].lower() if len(f) == 6 else '')  # `open:` and `fixed.` read as their word
    ok = len(f) == 6 and ROW_ID.fullmatch(f[0]) and f[1] in ('defect', 'decision') and all(f) and state in STATES
    return state if ok else None


def ledger_rows(text):
    """(open rows, closed rows, numbers of stray lines) of a ledger's text; blank lines are neither."""
    opened, closed, stray = [], [], []
    for i, line in enumerate(text.splitlines(), 1):
        if (state := row(line)) is None:
            stray += [i] * bool(line.strip())
        else:
            (opened if state in STATES[:2] else closed).append(line.strip())
    return opened, closed, stray


def ledger_lines(ledger, root):
    """The ledger section: open rows, the count of closed ones, and a warning naming stray lines."""
    if not ledger.is_file():
        return ['(none: start at step 1)']
    opened, closed, stray = ledger_rows(ledger.read_text(errors='replace'))
    out = []  # the warning and the count first: open rows fill whatever room is left
    if stray:
        shown = ', '.join(map(str, stray[:10])) + (' ...' if len(stray) > 10 else '')
        out.append(f'WARNING {len(stray)} stray line(s) in {ledger.relative_to(root)} (lines {shown}): keep only rows; move each '
                   f'experiment to {ledger.with_suffix(".lab.tsv").relative_to(root)} as one row per attempt (SKILL.md, Ledger)')
    out.append(f'{len(closed)} fixed or answered rows not shown')
    return out + (opened or ['(no open rows)'])


def git(*args, cwd, raw=False):
    out = subprocess.run(['git', *args], cwd=cwd, capture_output=True, text=True).stdout
    return out if raw else out.rstrip()  # porcelain lines start with a space


def plan_data(root):
    plan = root / '00-Guidelines/13-Plan'
    packages = json.loads((plan / 'packages.json').read_text())['packages']
    return packages, {b['id']: b for b in json.loads((plan / 'batches.json').read_text())['batches']}


def target(root, arg=''):
    """(key, package, batch ids, warning): the batch or package id in arg, else in the branch and worktree names
    (a batch id wins); with neither, the key is the branch."""
    packages, batches = plan_data(root)
    owner = {b: p for p in packages for b in p['batches']}
    ids = {p['id']: p for p in packages}
    branch = git('branch', '--show-current', cwd=root)

    def find(*names):
        tokens = [t.upper() for name in names for t in re.findall(r'[A-Za-z]+\d+', name)]
        batch = next((t for t in tokens if t in batches), None)
        return batch, owner.get(batch) if batch else next((ids[t] for t in tokens if t in ids), None)

    batch, pkg = find(arg)
    warning = f'no package or batch id in "{arg}": the branch and worktree names decide' if arg and not pkg else ''
    if not pkg:
        batch, pkg = find(branch, root.name)
        if pkg and plan.CLAIM.fullmatch(branch):  # a claim branch keeps its first batch's name: take the first not landed
            fin = plan.landed(root)
            batch = next((b for b in pkg['batches'] if b not in fin), batch)
    key = batch or (pkg['id'] if pkg else re.sub(r'[^\w.-]', '_', branch or 'detached'))
    return key, pkg, [batch] if batch else pkg['batches'] if pkg else [], warning


SECTIONS = []  # (title, lines) in the order main() adds them


def section(title, lines, cap=400):
    SECTIONS.append((title, (lines[:cap] + [f'... {len(lines) - cap} more'] if len(lines) > cap else lines) or ['(none)']))


ORDER = ('Ledger', 'Review record', 'Undecided limits', 'Follow-ups', 'Sources', 'Undecided optimization', 'Mutation records', 'Agent-made',
         'Open sweep', 'Recorded limits', 'Comparison record', 'Packages filed', 'TODO markers', 'Branch and', 'Other worktrees',
         'Brief')  # most needed first


def emit(head, key, root):
    """Write every section to the facts file and print head, then sections by ORDER within the room SKILL.md leaves."""
    facts = root / '.claude/finish' / f'{key}.facts.txt'
    full = '\n'.join(head + [f'\n### {t}\n' + '\n'.join(lines) for t, lines in SECTIONS]) + '\n'
    try:
        facts.parent.mkdir(parents=True, exist_ok=True)
        facts.write_text(full)
        where = str(facts.relative_to(root))
    except OSError as e:
        where = f'not written ({e.strerror})'
    room = plan.room_of(Path(__file__).with_name('SKILL.md'))
    cost = lambda item: sum(map(plan.cost, item.split('\n')))  # an item of several lines pays each line's floor
    rank = lambda t: next((i for i, p in enumerate(ORDER) if t[0].startswith(p)), len(ORDER))
    out = [*head, f'Full facts: {where} ({len(full)} characters); read it at step 1 when sections below are cut or not printed.']
    head_skip, ordered = '\n### Not printed (in the facts file)\n', sorted(SECTIONS, key=rank)
    left, skipped = room - sum(map(cost, out)), []
    for k, (title, lines) in enumerate(ordered):
        # kept back: the Not printed block naming every section not yet placed, in case none of them fits
        reserve = len(head_skip) + sum(map(cost, [*skipped, *(t for t, _ in ordered[k + 1:])])) + 1
        block = [f'\n### {title}']
        for line in lines if title.startswith('Ledger') else lines[:LINES]:  # open rows: as many as fit
            if left - reserve - sum(map(cost, block)) - cost(line) - 59 < 0:
                break
            block.append(line)
        if len(block) == 1:
            skipped.append(title)
            continue
        if len(block) - 1 < len(lines):
            block.append(f'... {len(lines) - len(block) + 1} more in the facts file')
        out += block
        left -= sum(map(cost, block))
    if skipped:
        out.append(head_skip + '\n'.join(skipped))  # one per line: titles hold semicolons
    print('\n'.join(out))


def limits(doc, root):
    """Each heading LIMIT_HEADING matches as `path:line: heading`, then every non-blank line of its section in full, up to
    the next heading of its level or above: /finish step 9 lists each recorded limit (user decision 2026-11-24)."""
    if not doc.is_file():
        return []
    out, level = [], 0
    for i, line in enumerate(doc.read_text(errors='replace').splitlines(), 1):
        if (m := re.match(r'(#+) ', line)) and level and len(m[1]) <= level:
            level = 0
        if m and not level and LIMIT_HEADING.match(line):
            level = len(m[1])
            out.append(f'{doc.relative_to(root)}:{i}: {line.strip()[:160]}')
        elif level and line.strip():
            out.append(line.rstrip())
    return out


def grep(path, pattern, root, keep=lambda line: True):
    if not path.is_file():
        return []
    return [f'{path.relative_to(root)}:{i}: {line.strip()[:160]}'
            for i, line in enumerate(path.read_text(errors='replace').splitlines(), 1) if pattern.search(line) and keep(line)]


def sweep_rows(path, pkg, root, keep):
    """Lines about the package outside the settled sections of a sweep ledger: its own section in full, and lines
    elsewhere naming the package or one of its batches."""
    if not path.is_file():
        return []
    res, own, settled = [], False, False
    named = re.compile(r'\b(' + '|'.join([pkg['id'], *pkg['batches']]) + r')\b')
    lines = path.read_text(errors='replace').splitlines() + ['']
    for i, line in enumerate(lines[:-1], 1):
        if line.startswith('## '):
            own, settled = line[3:].startswith(pkg['id'] + ' '), line[3:].startswith(SETTLED)
        elif (line.strip(' |-:') and not TABLE_RULE.match(lines[i])  # skip table rules and header rows
              and not settled and (own or named.search(line)) and keep(line)):
            res.append(f'{path.relative_to(root)}:{i}: {line.strip()[:160]}')
    return res


def evidence_doc(target_path):
    folder, name = target_path.split('/', 1)
    stem = re.sub(r'^(\d+)_', r'\1-', Path(name).stem.lstrip('_'))
    return f'{folder}/Docs/{stem}.md'


BOOKKEEPING = ('01-prompts.md', '00-Guidelines/13-Plan/packages.json', '00-Guidelines/13-Plan/batches.json',
               '00-Guidelines/13-Plan/finish-log.tsv')  # plan.py rewrites these after the review (/package step 11)


def review_paths(root, key, pkg, scope, batches):
    """The files a package review covers: targets, their tests and evidence documents. A scope without targets (a support
    batch) covers the package's evidence and every file changed on the branch or uncommitted instead, the plan files and
    the run's own records aside, so a later edit reopens the review (user decision 2026-11-23), and of the uncommitted
    ones only those absent from the --start record or changed since (SUP34)."""
    targets = [t for b in scope for t in batches[b]['targets'] + batches[b].get('support_targets', [])]
    docs = {evidence_doc(t) for t in targets} | (set() if key in batches and targets else set(pkg['evidence']))
    tests = {x for t in targets if re.fullmatch(r'\d\d-[^/]+/[^/]+', t) for x in plan.tests_for(root, t)}
    changed = set()
    if not targets:
        if git('rev-parse', '--verify', '--quiet', 'master', cwd=root):
            changed |= set(git('diff', '--name-only', '-z', 'master...HEAD', cwd=root, raw=True).split('\0'))
        work = sorted(p for _, p in uncommitted(root) if (root / p).is_file())
        try:
            before = dict(json.loads((root / '.claude/finish' / f'{key}.start.json').read_text())['files'])
        except (OSError, ValueError, KeyError, TypeError):
            before = {}
        now = blobs(root, [p for p in work if p in before])
        changed |= {p for p in work if p not in before or now[p] != before[p]}
        changed = {p for p in changed if p and p not in BOOKKEEPING and not p.startswith(('.claude/finish/', '.claude/pause/'))}
    return sorted(f for f in {*targets, *docs, *tests, *changed} if (root / f).is_file())


def blobs(root, paths, store=False):
    """{path: blob} of paths as they are now; store writes the blobs to git's object database, so a later diff finds them."""
    return dict(zip(paths, git('hash-object', *(['-w'] if store else []), '--', *paths, cwd=root).split())) if paths else {}


COMPARISON = re.compile(r'Comparison: (\d{4}-\d\d-\d\d), rig (.+?) ([0-9a-f]{12}), base [0-9a-f]{12}, headers (.*?), ((?:\S+ [\d.]+, )+)CPU')


def comparison(doc, root):
    """The newest `Comparison:` line of 06-compare.py in an evidence document against the current rig, Core headers and
    deciding compiler, the first it lists (05-testing.md, Stale evidence; user decision 2026-11-26): one line saying it is
    current, or what changed."""
    name = doc.relative_to(root)
    found = COMPARISON.findall(doc.read_text(errors='replace')) if doc.is_file() else []
    if not found:
        return f'{name}: no Comparison line (96-Local Testing/06-compare.py)'
    date, rig, rig_blob, headers, compilers = found[-1]
    blob = lambda path: git('hash-object', '--', str(path), cwd=root)[:12] if path.is_file() else 'missing'
    changed = [f'{label} changed' for label, path, old in [('the rig', root / rig, rig_blob), *((h, root / '01-Core' / h, b) for h, b in re.findall(r'(\S+) ([0-9a-f]{12})', headers))]
               if blob(path) != old]
    for exe, version in re.findall(r'(\S+) ([\d.]+), ', compilers)[:1]:
        now = subprocess.run([exe, '-dumpfullversion'], capture_output=True, text=True).stdout.strip() if shutil.which(exe) else 'not installed'
        if now != version:
            changed.append(f'{exe} {version} is now {now}')
    return f'{name}: comparison of {date} ' + ('is current' if not changed else 'is STALE, rerun it: ' + ', '.join(changed))


def since(root, path, old, new):
    """The diff of path from its reviewed blob old to its blob new (None: absent then or now); a blob git no longer has
    gives the whole file."""
    have = lambda b: b and subprocess.run(['git', 'cat-file', '-e', b], cwd=root, capture_output=True).returncode == 0
    if have(old) and have(new):
        body = git('diff', '--no-color', old, new, cwd=root, raw=True).split('\n')  # four header lines name blobs, not files
        return f'=== {path}\n' + '\n'.join(body[4:])
    if not new:
        return f'=== {path}: deleted since the review\n'
    return f'=== {path}: ' + ('new since the review' if not old else 'its reviewed blob is gone, so the whole file') + '\n'


def main():
    root = Path(git('rev-parse', '--show-toplevel', cwd='.'))
    if sys.argv[1:2] == ['--start']:
        key, pkg, _, _ = target(root, sys.argv[2] if len(sys.argv) > 2 else '')
        start = root / '.claude/finish' / f'{key}.start.json'
        if pkg and not start.exists():  # once per key: a resumed /package keeps the first record
            start.parent.mkdir(parents=True, exist_ok=True)
            start.write_text(json.dumps({'files': blobs(root, sorted(p for _, p in uncommitted(root) if (root / p).is_file()))}, indent=1))
        return
    if sys.argv[1:2] == ['--reviewed']:
        key, pkg, scope, _ = target(root, sys.argv[2] if len(sys.argv) > 2 else '')
        if not pkg:
            raise SystemExit('gaps.py --reviewed: no package or batch id')
        files = blobs(root, review_paths(root, key, pkg, scope, plan_data(root)[1]), store=True)
        record = root / '.claude/finish' / f'{key}.review.json'
        record.parent.mkdir(parents=True, exist_ok=True)
        record.write_text(json.dumps({'files': files, 'wrong': '--wrong' in sys.argv[3:],
                                      'report': '' if sys.stdin.isatty() else sys.stdin.read().strip()}, indent=1))
        print(f'review of {key} recorded: {len(files)} files, {record.relative_to(root)}')
        return
    worktrees = [l.split(' ', 1)[1] for l in git('worktree', 'list', '--porcelain', cwd=root).splitlines() if l.startswith('worktree ')]
    branch = git('branch', '--show-current', cwd=root)
    packages, batches = plan_data(root)
    key, pkg, scope, warning = target(root, sys.argv[1] if len(sys.argv) > 1 else '')
    has_master = bool(git('rev-parse', '--verify', '--quiet', 'master', cwd=root))
    position = (f'{git("rev-list", "--count", "master..HEAD", cwd=root)} ahead, {git("rev-list", "--count", "HEAD..master", cwd=root)} behind master'
                if has_master else 'NO local master branch: branch changes and landing unavailable')
    head = [f'### Target\nkey {key} | package {pkg["id"] + " (" + pkg["status"] + ")" if pkg else "none: the branch changes are the scope"}'
          f' | batches {" ".join(scope) or "-"} | branch {branch or "(detached)"} | {"worktree " + str(root) if Path(worktrees[0]) != root else "main checkout"}'
          f' | {position}' + (f'\nWARNING {warning}' if warning else '')]
    role = plan.role(root)
    if role == 'maintainer':
        head.append('role maintainer: step 7 lands on master')
    else:
        _, me, cl, _, _ = plan.gate(root, plan.load(root)[1])
        fin = plan.landed(root)  # finish-log lines, not rows: /package marks rows verified before /finish runs
        first = key if key in batches else pkg and next((b for b in pkg['batches'] if b not in fin), pkg['batches'][0])
        claim = plan.claim_branch(pkg, first, branch, cl, me) if pkg else branch
        last = not pkg or all(b == first or b in fin for b in pkg['batches'])
        own = cl.get(pkg['id']) if pkg else None
        head.append(f'role contributor: step 7 commits on branch {claim or "(detached)"} and stops before landing; repository '
              f'{plan.config(root, "compprog.repo") or plan.REPO}; '
              + (f'pull request #{own[2]}; ' if own and own[0].lower() == me.lower() else '')
              + ('last open batch: mark the pull request ready if verified' if last else 'more batches follow: the pull request stays a draft'))
    ledger = root / '.claude/finish' / f'{key}.txt'
    section(f'Ledger {ledger.relative_to(root)} (open rows)', ledger_lines(ledger, root))

    if pkg:
        brief = subprocess.run([sys.executable, str(root / '00-Guidelines/13-Plan/plan.py'), 'show', key],
                               cwd=root, capture_output=True, text=True).stdout.strip()
        section('Brief', brief.splitlines())
        head += [line for k, line in enumerate(brief.splitlines()) if not k or re.match(r'(WARNING|STOP)\b', line)]  # step 1 acts on these
        targets = [t for b in scope for t in batches[b]['targets'] + batches[b].get('support_targets', [])]
        docs = sorted({root / evidence_doc(t) for t in targets} | (set() if key in batches and targets else {root / d for d in pkg['evidence']}))
        bare = {re.sub(r'^\d+[-_]', '', Path(t).stem.lstrip('_')).lower() for t in targets}
        names = bare | {n.replace('_', '') for n in bare}  # file stems and struct names, any case
        tiny = [re.escape(n) for n in names if len(n) <= 3]  # `io`, `dp`: whole words only
        short = re.compile(r'\b(' + '|'.join(tiny) + r')\b', re.I) if tiny else None
        mentions = lambda line, extra: (any(n in line.lower() for n in names if len(n) > 3) or bool(short and short.search(line)) or extra in line)
        notes = sorted({root / t.split('/', 1)[0] / 'Docs/00-notes.md' for t in targets})
        found = [line for d in docs for line in limits(d, root)]
        section('Recorded limits (evidence headings)', found, max(len(found), 1))
        section('Undecided limits (no `user decision <date>`: fix, move to Contracts or ask; plan.py set verified refuses them)',
                [f'{d.relative_to(root)}:{i}: {t[:140]}' for d in docs for i, t in plan.undecided(d)])
        fu = plan.followups(pkg)
        section(f'Follow-ups of {pkg["id"]} (close each one done: plan.py follow-up {pkg["id"]} --done <n>)',
                [f'{i}. {x}' for i, x in enumerate(fu, 1)])
        section('Agent-made omissions and deferrals (notes lines without a user decision)',
                [hit for f in notes for hit in grep(f, DEFERRAL, root, lambda line: mentions(line, pkg['id']) and not DECIDED.search(line))])
        record = root / '.claude/finish' / f'{key}.review.json'
        if record.is_file():  # /finish step 2: one full review per package (user decisions 2026-11-17, 2026-11-26)
            data, now = json.loads(record.read_text()), blobs(root, review_paths(root, key, pkg, scope, batches), store=True)
            moved = sorted(f for f in {*now, *data['files']} if now.get(f) != data['files'].get(f))
            if data.get('wrong'):
                lines = ['the review found a wrong answer or undefined behavior: step 2 runs the full audit']
            elif moved:
                diff = root / '.claude/finish' / f'{key}.since-review.diff'
                diff.write_text(''.join(since(root, f, data['files'].get(f), now.get(f)) for f in moved))
                lines = [f'changed since the review: step 2 reviews only these changes, in {diff.relative_to(root)}:', *moved]
            else:
                lines = ['covers the current files: step 2 reuses this report and starts no audit', *data['report'].splitlines()]
            section("Review record (/package step 10)", lines, 200)
        else:
            section("Review record (/package step 10)", ['none: step 2 runs the full audit'])
        full = sorted({root / evidence_doc(t) for t in targets if CORE_FULL.fullmatch(t)})
        if full:  # 05-testing.md, Benchmarks: each is measured in this run or closed with its argument
            section('Undecided optimization candidates (Core full evidence)',
                    [hit for d in full for hit in grep(d, UNDECIDED, root)] + [f'{d.relative_to(root)}: no "### Optimization Candidates" table'
                     for d in full if d.is_file() and '\n### Optimization Candidates\n' not in d.read_text(errors='replace')])
            section('Comparison record (05-testing.md, Stale evidence)', [comparison(d, root) for d in full])
        section(f'Packages filed by {pkg["id"]}', [f'{p["id"]} ({p["status"]}): {p["label"]}' for p in packages
                                                   if re.search(rf'Filed by {pkg["id"]}\b', p['note'] or '')])
        sweeps = root / '00-Guidelines/23-Sweep Ledgers'
        prefixes = {int(m.group(1)) for t in targets for m in [re.match(r'_?(\d+)', Path(t).name)] if m}
        numbers = lambda line: {int(n) for g in ROW_REF.findall(line) for n in re.findall(r'\d+', g)} | {int(a or b) for a, b in PREFIX_REF.findall(line)}
        in_batch = (lambda line: mentions(line, key) or bool(numbers(line) & prefixes)
                    ) if key in batches else (lambda line: True)
        section('Open sweep ledger rows', [hit for f in ('duplicates.md', 'omissions.md') for hit in sweep_rows(sweeps / f, pkg, root, in_batch)])

    committed = git('diff', '--name-only', '-z', 'master...HEAD', cwd=root, raw=True).split('\0') if has_master else []
    work = uncommitted(root)
    changed = sorted({p for p in committed if p} | {p for _, p in work})
    section('Branch and uncommitted changes', (git('diff', '--name-status', 'master...HEAD', cwd=root).splitlines() if has_master else [])
            + [f'uncommitted {code} {path}' for code, path in work], 60)
    script = root / '96-Local Testing/04-mutation.py'
    if script.is_file():
        spec = importlib.util.spec_from_file_location('finish_mutation', script)
        mutation = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(mutation)
        heads = [f for f in changed if mutation.HEADER.fullmatch(f) and (root / f).is_file()]
        own = set(pkg['evidence']) if pkg else set()
        others = {d for p in packages if not pkg or p['id'] != pkg['id'] for d in p['evidence']} - own  # their headers: existing records only
        mine = [h for h in heads if mutation.evidence(h) not in others]
        section('Mutation records (04-mutation.py check on changed headers; /package step 7)',
                [f'{h}: {why}' for h, why in mutation.stale(root, mine) + mutation.stale(root, [h for h in heads if h not in mine], True)])
    script = root / '95-Resources/00-sources.py'
    if script.is_file():  # every source the work used is filed and backed up (95-Resources/00-index.md, Storage)
        spec = importlib.util.spec_from_file_location('finish_sources', script)
        sources = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(sources)
        section('Sources (95-Resources/00-index.md, Storage; 00-sources.py check)', sources.problems({key, *(pkg and [pkg['id']] or [])}))
    section('TODO markers in changed files', [hit for f in changed if (root / f).suffix in ('.hpp', '.cpp', '.py', '.md')
                                                for hit in grep(root / f, MARKER, root)])
    overlap = []
    for block in git('worktree', 'list', '--porcelain', cwd=root).split('\n\n') if has_master else []:
        other = re.search(r'branch refs/heads/(\S+)', block)
        if other and other.group(1) not in ('master', branch):
            shared = set(changed) & set(git('diff', '--name-only', '-z', f'master...{other.group(1)}', cwd=root, raw=True).split('\0'))
            if shared:
                overlap.append(f'{other.group(1)}: {", ".join(sorted(shared)[:8])}' + (' ...' if len(shared) > 8 else ''))
    section('Other worktrees touching the same files (committed work; worktree.py --finish adds their uncommitted edits)', overlap)
    emit(head, key, root)


if __name__ == '__main__':
    main()
