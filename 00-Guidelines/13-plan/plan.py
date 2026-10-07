#!/usr/bin/env python3
"""Work-plan manifest CLI: show, status, next, set, render and check packages/batches."""
import argparse
import collections
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PLAN = '00-Guidelines/13-plan'
BEGIN, END = '<!-- plan:begin -->', '<!-- plan:end -->'
PHASES = ['Foundations re-audit', 'Core full types', 'Contest core',
          'Python contest layer', 'Specialist and research families', 'Final support integration']
LEGEND = ('Legend: `[x]` verified, `[ ]` otherwise; **package — batches in order** '
          '(model, size): label. Status: planned | in-progress | audit | verified.')
TARGET = re.compile(r'`((?:\d{2}-[^`]+\.hpp)|(?:_\d{2}_[^`]+\.py))`')


def load(root=ROOT):
    return [json.loads((root / PLAN / n).read_text()) for n in ('batches.json', 'packages.json')]


def save(root, packages):
    (root / PLAN / 'packages.json').write_text(json.dumps(packages, indent=2, ensure_ascii=False) + '\n')


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


def prereqs(batches, packages):
    """Package id -> earlier-owner package ids derived from batch prerequisites."""
    owner = {b: p['id'] for p in packages['packages'] for b in p['batches']}
    order = {p['id']: i for i, p in enumerate(packages['packages'])}
    bmap = {b['id']: b for b in batches['batches']}
    return {p['id']: sorted({owner[d] for b in p['batches'] if b in bmap for d in bmap[b]['prerequisites'] if d in owner} - {p['id']},
                            key=order.get) for p in packages['packages']}


def link(path):
    return f'[evidence](<{path}>)' if ' ' in path else f'[evidence]({path})'


def render_text(packages):
    out = [BEGIN, '', LEGEND]
    for phase in PHASES:
        out += ['', '### ' + phase, '']
        for p in packages['packages']:
            if p['phase'] == phase:
                tail = ''.join([f' Note: {p["note"]}' if p['note'] else '', *(' ' + link(e) for e in p['evidence'])])
                out.append(f'- [{"x" if p["status"] == "verified" else " "}] **{p["id"]} — {", ".join(p["batches"])}** '
                           f'({p["model"]}, {p["size"]}): {p["label"]}. Status: {p["status"]}.{tail}')
    return '\n'.join(out + ['', END])


def rendered_file(root, packages):
    text = (root / '01-prompts.md').read_text() if (root / '01-prompts.md').exists() else ''
    if BEGIN in text and END in text:
        return text[:text.index(BEGIN)] + render_text(packages) + text[text.index(END) + len(END):]
    return text.rstrip('\n') + ('\n\n' if text.strip() else '') + render_text(packages) + '\n'


def ready(batches, packages):
    status = {p['id']: p['status'] for p in packages['packages']}
    deps = prereqs(batches, packages)
    return next((p for p in packages['packages'] if p['status'] != 'verified'
                 and all(status[d] in ('verified', 'audit') for d in deps[p['id']])), None)


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
    ids = [p['id'] for p in pl]
    require(len(set(ids)) == len(ids) and all(re.fullmatch(r'P\d{3}', i) for i in ids), 'Package ids must be unique Pnnn; list order is the schedule')
    scheduled = collections.Counter(b for p in pl for b in p['batches'])
    require(scheduled == collections.Counter(list(bmap)), 'Batches not scheduled exactly once: '
            + str(sorted(set(scheduled.items()) ^ {(b, 1) for b in bmap})))
    order = {p: i for i, p in enumerate(ids)}
    deps = prereqs(batches, packages)
    for p in pl:
        for d in deps[p['id']]:
            require(order[d] < order[p['id']], f'Prerequisite scheduled late: {p["id"]} -> {d}')
        local = {b: i for i, b in enumerate(p['batches'])}
        for b in p['batches']:
            require(b in bmap, f'Unknown batch in {p["id"]}: {b}')
            for d in bmap.get(b, {}).get('prerequisites', []):
                require(d not in local or local[d] < local[b], f'Within-package batch order: {p["id"]} {d} before {b}')
        require(p['status'] in packages['statuses'], f'Bad status: {p["id"]} {p["status"]}')
        require(p['model'] in packages['models'], f'Bad model: {p["id"]} {p["model"]}')
        require(p['phase'] in PHASES, f'Bad phase: {p["id"]} {p["phase"]}')
        for e in p['evidence']:
            require((root / e).exists(), f'Missing evidence: {p["id"]} -> {e}')
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


def tests_for(root, target):
    folder, name = target.split('/')
    prefix, base = re.search(r'\d{2}', name)[0], root / '96-Local Testing' / folder
    return sorted(str(p.relative_to(root)) for p in base.iterdir() if p.is_file()
                  and re.match(r'_?(\d{2})[-_]', p.name) and re.match(r'_?(\d{2})', p.name)[1] == prefix) if base.is_dir() else []


def legacy_for(root, target):
    folder, name = target.split('/')
    index = root / folder / '97-Legacy/00-index.md'
    stem = re.sub(r'^_?\d{2}[-_]|\.(hpp|py)$', '', name).lower()
    keys = {stem, stem.replace('_', ''), stem.replace('_', ' ')}
    rows = [cells(l) for l in index.read_text().splitlines() if l.startswith('| [') and any(k in l.lower() for k in keys)] if index.exists() else []
    return [' — '.join([re.sub(r'\[[^]]*\]\(<?([^)>]+)>?\)', rf'`{folder}/97-Legacy/\1`', r[0]), *r[1:]]) for r in rows]


def batch_brief(root, b, inv):
    out = [f'### {b["id"]} ({b["size"]})', '', 'Focus: ' + b['focus'],
           'Batch prerequisites: ' + (', '.join(b['prerequisites']) or 'none')]
    for t in b['targets']:
        out += [f'- `{t}` [{"exists" if (root / t).exists() else "missing"}]', '  ' + ' — '.join(inv.get(t, (['(no inventory row)'],))[0])]
    tests = [x for t in b['targets'] for x in tests_for(root, t)]
    legacy = [x for t in b['targets'] for x in legacy_for(root, t)]
    out += ['Tests:' if tests else 'Tests: none'] + ['- `' + x + '`' for x in tests]
    if legacy:
        out += ['Legacy:'] + ['- ' + x for x in dict.fromkeys(legacy)]
    return out + ['']


def show(root, ident):
    batches, packages = load(root)
    bmap = {b['id']: b for b in batches['batches']}
    pmap = {p['id']: p for p in packages['packages']}
    inv = inventories(root)
    if ident in bmap:
        return '\n'.join(batch_brief(root, bmap[ident], inv))
    if ident not in pmap:
        raise SystemExit(f'Unknown package or batch: {ident}')
    p = pmap[ident]
    out = [f'{p["id"]} — {p["label"]} | {p["phase"]} | {p["size"]} | {p["status"]} | '
           f'model: {p["model"]} ({packages["models"][p["model"]]})', '']
    deps = prereqs(batches, packages)[ident]
    out.append('Prerequisite packages:' if deps else 'Prerequisite packages: none')
    out += [f'- {d} ({pmap[d]["status"]}) — {pmap[d]["label"]}' for d in deps]
    out += [f'WARNING: prerequisite {d} is {pmap[d]["status"]}; its APIs may be missing or unstable.'
            for d in deps if pmap[d]['status'] in ('planned', 'in-progress')]
    out.append('')
    for b in p['batches']:
        out += batch_brief(root, bmap[b], inv)
    out.append('Evidence: ' + (' '.join(link(e) for e in p['evidence']) or 'none'))
    if p['note']:
        out.append('Note: ' + p['note'])
    return '\n'.join(out)


def status_text(root):
    batches, packages = load(root)
    sts = packages['statuses']
    count = collections.Counter((p['phase'], p['status']) for p in packages['packages'])
    out = ['| Phase | ' + ' | '.join(sts) + ' | total |', '|---' * (len(sts) + 2) + '|']
    for ph in PHASES + ['Total']:
        row = [sum(n for (f, s), n in count.items() if s == st and ph in (f, 'Total')) for st in sts]
        out.append(f'| {ph} | ' + ' | '.join(map(str, row)) + f' | {sum(row)} |')
    p = ready(batches, packages)
    return '\n'.join(out + ['', f'Next: {p["id"]} — {p["label"]}' if p else 'Next: none ready (all verified or blocked).'])


def ready_list(root=ROOT):
    """Packages whose prerequisites are all verified or audit, in schedule order, with their folders."""
    batches, packages = load(root)
    bmap = {b['id']: b for b in batches['batches']}
    status = {p['id']: p['status'] for p in packages['packages']}
    deps = prereqs(batches, packages)
    out = []
    for p in packages['packages']:
        if p['status'] == 'verified' or not all(status[d] in ('verified', 'audit') for d in deps[p['id']]):
            continue
        folders = sorted({bmap[b]['folder'] for b in p['batches']} | {t.split('/')[0] for b in p['batches'] for t in bmap[b]['targets']})
        out.append((p, [f for f in folders if f != 'support'] or ['support']))
    return out


def ready_text(root, limit):
    rows = ready_list(root)
    out = ['Ready packages (prerequisites satisfied), schedule order:']
    out += [f'- {p["id"]} [{p["status"]}, {p["model"]}, {p["size"]}] {", ".join(f)} — {p["label"]}' for p, f in rows]
    chosen, used = [], set()
    for p, f in rows:
        if p['status'] != 'in-progress' and not (set(f) & used):
            chosen.append(p); used |= set(f)
        if len(chosen) == limit:
            break
    out += ['', f'Disjoint set for {limit} parallel sessions (one folder each; use `claude -w <name>` worktrees and merge after):']
    out += [f'- {p["id"]} — {p["label"]}' for p in chosen]
    out += ['', 'Shared files every session touches (expect small merges): 01-prompts.md, 00-Guidelines/13-plan/packages.json, '
            '96-Local Testing/00-index.md, 96-Local Testing/01-run.py. Same-folder packages also share 00-index.md and the 98/99 aggregates; run those sequentially.']
    return '\n'.join(out)


def doctor(root=ROOT, fix=False):
    """Cross-check plan, inventories, files and tests; --fix re-renders the checklist. Returns (todo lines, fixed lines)."""
    import ast
    batches, packages = load(root)
    bmap = {b['id']: b for b in batches['batches']}
    inv = inventories(root)
    owner = {t: p['id'] for p in packages['packages'] for b in p['batches'] for t in bmap[b]['targets']}
    pstatus = {p['id']: p['status'] for p in packages['packages']}
    todo, fixed = collections.defaultdict(list), []
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
        rstatus = parts[-1].split(';')[0].split(' ')[0].strip('*').lower()
        exists = (root / t).exists()
        pid = owner.get(t, '?'); ps = pstatus.get(pid)
        key = f'{pid} ({folder})'
        if rstatus == 'planned' and exists:
            todo[key].append(f'{name}: file exists but row is planned; set existing-unverified or partial')
        if rstatus in ('verified', 'partial', 'existing-unverified') and not exists:
            todo[key].append(f'{name}: row is {rstatus} but the file is missing')
        if rstatus == 'verified' and ps in ('planned', 'in-progress'):
            todo[key].append(f'{name}: row verified but package {pid} is {ps}; finish the package or downgrade the row')
        if ps == 'verified' and rstatus not in ('verified', 'partial'):
            todo[key].append(f'{name}: package verified but row is {rstatus}')
        if ps == 'verified' and rstatus == 'partial':
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
    cmd = {'audit': '/package', 'in-progress': '/package', 'planned': '/package'}
    out += ['', 'Resolve with /package Pxxx (re-audit or continue), /restyle Pxxx for pure style, or edit the row/plan; then rerun doctor.']
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest='cmd', required=True)
    sub.add_parser('show').add_argument('id')
    for name in ('status', 'next', 'render', 'check'):
        sub.add_parser(name)
    sub.add_parser('ready').add_argument('--max', type=int, default=3, help='size of the disjoint parallel set')
    sub.add_parser('doctor').add_argument('--fix', action='store_true', help='re-render the checklist when out of sync')
    s = sub.add_parser('set')
    s.add_argument('id')
    s.add_argument('status')
    s.add_argument('--evidence', nargs='+', default=[], help='append repo-relative evidence paths')
    s.add_argument('--note', help='replace the package note')
    a = ap.parse_args()
    if a.cmd == 'show':
        print(show(ROOT, a.id))
    elif a.cmd == 'status':
        print(status_text(ROOT))
    elif a.cmd == 'next':
        p = ready(*load(ROOT))
        if not p:
            return 1
        print(p['id'])
    elif a.cmd == 'ready':
        print(ready_text(ROOT, a.max))
    elif a.cmd == 'doctor':
        print(doctor_text(ROOT, a.fix))
        return bool(doctor(ROOT)[0])
    elif a.cmd == 'check':
        errors = check(ROOT)
        print('\n'.join(errors) or 'plan check: ok')
        return bool(errors)
    else:
        packages = load(ROOT)[1]
        if a.cmd == 'set':
            p = next((p for p in packages['packages'] if p['id'] == a.id), None)
            if p is None or a.status not in packages['statuses']:
                raise SystemExit(f'Unknown package {a.id}' if p is None else f'Status must be one of {packages["statuses"]}')
            missing = [e for e in a.evidence if not (ROOT / e).exists()]
            if missing:
                raise SystemExit(f'Missing evidence paths: {missing}')
            p['status'] = a.status
            p['evidence'] += [e for e in a.evidence if e not in p['evidence']]
            if a.note is not None:
                p['note'] = a.note
            save(ROOT, packages)
        (ROOT / '01-prompts.md').write_text(rendered_file(ROOT, packages))
        print(f'Rendered 01-prompts.md ({len(packages["packages"])} packages)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
