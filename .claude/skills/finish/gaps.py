#!/usr/bin/env python3
"""Facts for /finish: target, ledger, brief, recorded gaps, branch changes, overlap with other worktrees.

Usage: gaps.py [Pxxx | batch id]. Without an argument the target comes from the batch or package id in the
branch or worktree name (a batch id wins); with neither, the target is the branch itself.
gaps.py --reviewed <Pxxx | batch id> records /package's review (step 10): the blobs of the targets, their tests and
evidence documents, and the reviewer's report read from standard input. /finish reuses it while those files are unchanged.
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


def section(title, lines, cap=40):
    print(f'\n### {title}')
    print('\n'.join(lines[:cap]) if lines else '(none)')
    if len(lines) > cap:
        print(f'... {len(lines) - cap} more')


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


def review_paths(root, key, pkg, scope, batches):
    """The files a package review covers: targets, their tests and evidence documents."""
    targets = [t for b in scope for t in batches[b]['targets'] + batches[b].get('support_targets', [])]
    docs = {evidence_doc(t) for t in targets} | (set() if key in batches else set(pkg['evidence']))
    tests = {x for t in targets if re.fullmatch(r'\d\d-[^/]+/[^/]+', t) for x in plan.tests_for(root, t)}
    return sorted(f for f in {*targets, *docs, *tests} if (root / f).is_file())


def blobs(root, paths):
    return dict(zip(paths, git('hash-object', '--', *paths, cwd=root).split())) if paths else {}


COMPARISON = re.compile(r'Comparison: (\d{4}-\d\d-\d\d), rig (.+?) ([0-9a-f]{12}), base [0-9a-f]{12}, headers (.*?), ((?:\S+ [\d.]+, )+)CPU')


def comparison(doc, root):
    """The newest `Comparison:` line of 06-compare.py in an evidence document against the current rig, Core headers and
    compilers (05-testing.md, Stale evidence): one line saying it is current, or what changed."""
    name = doc.relative_to(root)
    found = COMPARISON.findall(doc.read_text(errors='replace')) if doc.is_file() else []
    if not found:
        return f'{name}: no Comparison line (96-Local Testing/06-compare.py)'
    date, rig, rig_blob, headers, compilers = found[-1]
    blob = lambda path: git('hash-object', '--', str(path), cwd=root)[:12] if path.is_file() else 'missing'
    changed = [f'{label} changed' for label, path, old in [('the rig', root / rig, rig_blob), *((h, root / '01-Core' / h, b) for h, b in re.findall(r'(\S+) ([0-9a-f]{12})', headers))]
               if blob(path) != old]
    for exe, version in re.findall(r'(\S+) ([\d.]+), ', compilers):
        now = subprocess.run([exe, '-dumpfullversion'], capture_output=True, text=True).stdout.strip() if shutil.which(exe) else 'not installed'
        if now != version:
            changed.append(f'{exe} {version} is now {now}')
    return f'{name}: comparison of {date} ' + ('is current' if not changed else 'is STALE, rerun it: ' + ', '.join(changed))


def main():
    root = Path(git('rev-parse', '--show-toplevel', cwd='.'))
    if sys.argv[1:2] == ['--reviewed']:
        key, pkg, scope, _ = target(root, sys.argv[2] if len(sys.argv) > 2 else '')
        if not pkg:
            raise SystemExit('gaps.py --reviewed: no package or batch id')
        files = blobs(root, review_paths(root, key, pkg, scope, plan_data(root)[1]))
        record = root / '.claude/finish' / f'{key}.review.json'
        record.parent.mkdir(parents=True, exist_ok=True)
        record.write_text(json.dumps({'files': files, 'report': '' if sys.stdin.isatty() else sys.stdin.read().strip()}, indent=1))
        print(f'review of {key} recorded: {len(files)} files, {record.relative_to(root)}')
        return
    worktrees = [l.split(' ', 1)[1] for l in git('worktree', 'list', '--porcelain', cwd=root).splitlines() if l.startswith('worktree ')]
    branch = git('branch', '--show-current', cwd=root)
    packages, batches = plan_data(root)
    key, pkg, scope, warning = target(root, sys.argv[1] if len(sys.argv) > 1 else '')
    has_master = bool(git('rev-parse', '--verify', '--quiet', 'master', cwd=root))
    position = (f'{git("rev-list", "--count", "master..HEAD", cwd=root)} ahead, {git("rev-list", "--count", "HEAD..master", cwd=root)} behind master'
                if has_master else 'NO local master branch: branch changes and landing unavailable')
    print(f'### Target\nkey {key} | package {pkg["id"] + " (" + pkg["status"] + ")" if pkg else "none: the branch changes are the scope"}'
          f' | batches {" ".join(scope) or "-"} | branch {branch or "(detached)"} | {"worktree " + str(root) if Path(worktrees[0]) != root else "main checkout"}'
          f' | {position}' + (f'\nWARNING {warning}' if warning else ''))
    role = plan.role(root)
    if role == 'maintainer':
        print('role maintainer: step 7 lands on master')
    else:
        _, me, cl, _, _ = plan.gate(root, plan.load(root)[1])
        fin = plan.landed(root)  # finish-log lines, not rows: /package marks rows verified before /finish runs
        first = key if key in batches else pkg and next((b for b in pkg['batches'] if b not in fin), pkg['batches'][0])
        claim = plan.claim_branch(pkg, first, branch, cl, me) if pkg else branch
        last = not pkg or all(b == first or b in fin for b in pkg['batches'])
        own = cl.get(pkg['id']) if pkg else None
        print(f'role contributor: step 7 commits on branch {claim or "(detached)"} and stops before landing; repository '
              f'{plan.config(root, "compprog.repo") or plan.REPO}; '
              + (f'pull request #{own[2]}; ' if own and own[0].lower() == me.lower() else '')
              + ('last open batch: mark the pull request ready if verified' if last else 'more batches follow: the pull request stays a draft'))
    ledger = root / '.claude/finish' / f'{key}.txt'
    print(f'\n### Ledger {ledger.relative_to(root)}')
    print(ledger.read_text().strip() if ledger.is_file() else '(none: start at step 1)')

    if pkg:
        brief = subprocess.run([sys.executable, str(root / '00-Guidelines/13-Plan/plan.py'), 'show', key],
                               cwd=root, capture_output=True, text=True).stdout.strip()
        print('\n### Brief\n' + brief)
        targets = [t for b in scope for t in batches[b]['targets'] + batches[b].get('support_targets', [])]
        docs = sorted({root / evidence_doc(t) for t in targets} | (set() if key in batches else {root / d for d in pkg['evidence']}))
        bare = {re.sub(r'^\d+[-_]', '', Path(t).stem.lstrip('_')).lower() for t in targets}
        names = bare | {n.replace('_', '') for n in bare}  # file stems and struct names, any case
        tiny = [re.escape(n) for n in names if len(n) <= 3]  # `io`, `dp`: whole words only
        short = re.compile(r'\b(' + '|'.join(tiny) + r')\b', re.I) if tiny else None
        mentions = lambda line, extra: (any(n in line.lower() for n in names if len(n) > 3) or bool(short and short.search(line)) or extra in line)
        notes = sorted({root / t.split('/', 1)[0] / 'Docs/00-notes.md' for t in targets})
        section('Recorded limits (evidence headings)', [hit for d in docs for hit in grep(d, LIMIT_HEADING, root)])
        section('Agent-made omissions and deferrals (notes lines without a user decision)',
                [hit for f in notes for hit in grep(f, DEFERRAL, root, lambda line: mentions(line, pkg['id']) and not DECIDED.search(line))])
        record = root / '.claude/finish' / f'{key}.review.json'
        if record.is_file():  # /finish step 2: one full review per package (user decision 2026-11-17)
            data, now = json.loads(record.read_text()), blobs(root, review_paths(root, key, pkg, scope, batches))
            moved = sorted(f for f in {*now, *data['files']} if now.get(f) != data['files'].get(f))
            section("Review record (/package step 10)", ['changed since the review; the step 2 audit covers these files:', *moved] if moved
                    else ['covers the current files: step 2 reuses this report and starts no audit', *data['report'].splitlines()], 200)
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
        section('Mutation records (04-mutation.py check on changed headers; /package step 6)',
                [f'{h}: {why}' for h, why in mutation.stale(root, mine) + mutation.stale(root, [h for h in heads if h not in mine], True)])
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


if __name__ == '__main__':
    main()
