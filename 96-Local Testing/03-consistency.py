#!/usr/bin/env python3
"""Read-only inventory, ownership, path, provenance and Markdown consistency checks."""
import collections
import hashlib
import importlib.util
import json
import re
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
errors = []
counts = {}

def require(condition, message):
    if not condition:
        errors.append(message)

def read_json(name):
    return json.loads((ROOT / name).read_text())

def cells(line):
    return [x.strip() for x in re.split(r'(?<!\\)\|', line.strip())[1:-1]]

inventories = {}
tiers = {}
for folder in sorted(ROOT.iterdir()):
    if not re.match(r'0[1-8]-', folder.name) or not folder.is_dir():
        continue
    tier = 'Core' if folder.name == '01-Core' else None
    rows = []
    for line in (folder / '00-index.md').read_text().splitlines():
        if line in ('## Basic', '## Advanced', '## Esoteric'):
            tier = line[3:]
        if not line.startswith('|'):
            continue
        parts = cells(line)
        match = re.fullmatch(r'`((?:\d{2}-[^`]+\.hpp)|(?:_\d{2}_[^`]+\.py))`', parts[0])
        if not match:
            continue
        name = match[1]
        target = folder.name + '/' + name
        require(target not in inventories, 'Repeated inventory target: ' + target)
        inventories[target] = parts
        tiers[target] = tier
        rows.append((int(re.search(r'\d{2}', name)[0]), tier))
    require([n for n, _ in rows] == list(range(1, len(rows) + 1)), 'Nonconsecutive target prefixes: ' + folder.name)
    if folder.name != '01-Core':
        order = {'Basic': 0, 'Advanced': 1, 'Esoteric': 2}
        require([order[t] for _, t in rows] == sorted(order[t] for _, t in rows), 'Tier order: ' + folder.name)
    counts[folder.name] = len(rows)

batch_map = read_json('00-Guidelines/13-Work Batches/99-batches.json')
plan = read_json('00-Guidelines/13-Work Batches/98-session-plan.json')
batches = {b['id']: b for b in batch_map['batches']}
require(len(batches) == len(batch_map['batches']) == batch_map['algorithm_batches'] == plan['algorithm_batches'], 'Batch count mismatch')
ownership = collections.Counter(t for b in batches.values() for t in b['targets'])
require(set(ownership) == set(inventories), 'Inventory/batch targets disagree: ' + str(set(ownership) ^ set(inventories)))
require(all(n == 1 for n in ownership.values()), 'Nonunique target ownership')
require(len(inventories) == batch_map['numbered_targets'] == plan['numbered_targets'], 'Numbered target count mismatch')
all_table_ids = []
for table in sorted({b['table'] for b in batches.values()}):
    for line in (ROOT / table).read_text().splitlines():
        if not line.startswith('|'):
            continue
        row = cells(line)
        if not re.fullmatch(r'(?:C|DS|GE|GR|MA|MI|ST|PY)\d{2}', row[0]):
            continue
        require(len(row) == 5, 'Batch table width: ' + table)
        ident, names, size, prereqs, focus = row
        all_table_ids.append(ident)
        b = batches[ident]
        paths = [b['folder'] + '/' + n for n in re.findall(r'`([^`]+)`', names)]
        require(paths == b['targets'], 'Batch target mismatch: ' + ident)
        require(size == b['size'] and focus == b['focus'], 'Batch scope/size mismatch: ' + ident)
        require(re.findall(r'\b(?:C|DS|GE|GR|MA|MI|ST|PY)\d{2}\b', prereqs) == b['prerequisites'], 'Batch dependency mismatch: ' + ident)
        require(set(b['tiers']) == {tiers[t] for t in b['targets']}, 'Batch tier mismatch: ' + ident)
        require(b['table'] == table, 'Batch table pointer: ' + ident)
        for dep in b['prerequisites']:
            require(dep in batches and dep != ident, 'Unknown/self prerequisite: ' + ident + ' -> ' + dep)
require(collections.Counter(all_table_ids) == collections.Counter(batches.keys()), 'Missing or repeated table IDs')
support = set(batch_map['support_passes'])
support_targets = [t for b in batches.values() for t in b.get('support_targets', [])]
require(len(support_targets) == len(set(support_targets)) == batch_map['additional_support_targets'], 'Helper ownership mismatch')
for t in support_targets:
    require((ROOT / t).is_file(), 'Missing shared helper: ' + t)

sessions = plan['sessions']
ids = [s['id'] for s in sessions]
require(ids == [f'P{i:03}' for i in range(1, len(sessions) + 1)], 'Package numbering/order mismatch')
require(len(sessions) == plan['packages'], 'Package count mismatch')
scheduled = collections.Counter(b for s in sessions for b in s['batches'])
require(scheduled == collections.Counter(batches.keys() | support), 'Missing/duplicate scheduled batches')
owner = {b: s['id'] for s in sessions for b in s['batches']}
package_order = {p: i for i, p in enumerate(ids)}
for s in sessions:
    bs = [batches[b] for b in s['batches'] if b in batches]
    require(s['targets'] == [t for b in bs for t in b['targets']], 'Package target mismatch: ' + s['id'])
    expected_tables = sorted({b['table'] for b in bs} | ({'00-Guidelines/13-Work Batches/09-support.md'} if support & set(s['batches']) else set()))
    require(s['tables'] == expected_tables, 'Package table pointer mismatch: ' + s['id'])
    deps = {owner[d] for b in bs for d in b['prerequisites']} - {s['id']}
    if 'SUP05' in s['batches']:
        deps.add(owner['C01'])
    require(set(s['prerequisite_packages']) == deps, 'Package dependency mismatch: ' + s['id'])
    for dep in s['prerequisite_packages']:
        require(package_order[dep] < package_order[s['id']], 'Prerequisite scheduled late: ' + s['id'] + ' -> ' + dep)
    local_order = {b: i for i, b in enumerate(s['batches'])}
    for b in bs:
        for dep in b['prerequisites']:
            if dep in local_order:
                require(local_order[dep] < local_order[b['id']], 'Within-package dependency order: ' + s['id'])

prompt = (ROOT / '01-prompts.md').read_text()
checklist = re.findall(r'^- \[([ xX])\] \*\*(P\d{3}) — ([^*]+)\*\*:', prompt, re.M)
require([p for _, p, _ in checklist] == ids, 'Checklist/package IDs disagree')
for (_, p, bs), s in zip(checklist, sessions):
    require(bs.split(', ') == s['batches'], 'Checklist batch ownership: ' + p)

coverage = read_json('00-Guidelines/20-library-checker-coverage.json')
require(len(coverage['records']) == coverage['problem_families'], 'Judge family count')
require(len({r['problem'] for r in coverage['records']}) == len(coverage['records']), 'Duplicate judge family')
for record in coverage['records']:
    require(bool(record['targets']) and set(record['targets']) <= inventories.keys(), 'Unmapped judge family: ' + record['problem'])

archive = read_json('00-Guidelines/19-archive-map.json')
actual_archive = {str(p.relative_to(ROOT)) for p in (ROOT / 'OLD').rglob('*') if p.is_file()}
require(actual_archive == {r['path'] for r in archive['files']}, 'Archive manifest coverage mismatch')
require(archive['summary']['files'] == archive['summary']['classified'] == len(archive['files']), 'Archive summary count mismatch')
require(archive['summary']['classifications'] == dict(collections.Counter(r['classification'] for r in archive['files'])), 'Archive classification totals mismatch')
require(archive['summary']['canonical_inventory_targets_referenced'] == len({t for r in archive['files'] for t in r['targets']}), 'Archive referenced-target count mismatch')
for record in archive['files']:
    require(hashlib.sha256((ROOT / record['path']).read_bytes()).hexdigest() == record['sha256'], 'Archive hash mismatch: ' + record['path'])
    require(set(record['targets']) <= inventories.keys(), 'Archive target mismatch: ' + record['path'])

monolith = read_json('00-Guidelines/15-monolith-map.json')
for source, meta in monolith['sources'].items():
    require(hashlib.sha256((ROOT / source).read_bytes()).hexdigest() == meta['sha256'], 'Monolith source hash: ' + source)
for entry in monolith['entries']:
    if 'body_sha256' not in entry:
        continue
    source = (ROOT / entry['source']).read_bytes().splitlines(keepends=True)
    body = b''.join(source[entry['start_line'] - 1:entry['end_line']])
    require(hashlib.sha256(body).hexdigest() == entry['body_sha256'], 'Original excerpt hash: ' + entry['source'] + ':' + str(entry['start_line']))
    if 'target_body_start_line' in entry:
        target = (ROOT / entry['target']).read_bytes().splitlines(keepends=True)
        body = b''.join(target[entry['target_body_start_line'] - 1:entry['target_body_end_line']])
        require(hashlib.sha256(body).hexdigest() == entry['body_sha256'], 'Transferred excerpt hash: ' + entry['target'])

markdown_count = 0
links = 0
for p in sorted(ROOT.rglob('*.md')):
    if any(part in ('.git', '.agents', '.codex') for part in p.relative_to(ROOT).parts):
        continue
    markdown_count += 1
    lines = p.read_text().splitlines()
    fence = False
    previous_table = None
    for i, line in enumerate(lines):
        if re.match(r'\s*(```|~~~)', line):
            fence = not fence
            previous_table = None
            continue
        if fence:
            continue
        where = str(p.relative_to(ROOT)) + ':' + str(i + 1)
        require(line == line.rstrip(), 'Trailing whitespace: ' + where)
        if re.match(r'^#+ ', line):
            require(i == 0 or not lines[i-1], 'No blank before heading: ' + where)
            require(i + 1 == len(lines) or not lines[i+1], 'No blank after heading: ' + where)
        if line.startswith('|'):
            width = len(cells(line))
            require(previous_table is None or previous_table == width, 'Inconsistent table width: ' + where)
            previous_table = width
        else:
            previous_table = None
        for _, url in re.findall(r'\[([^\]]+)\]\((<[^>]+>|[^\s)]+)\)', re.sub(r'`[^`]*`', '', line)):
            url = unquote(url.strip('<>')).split('#')[0]
            if not url or '://' in url or url.startswith('mailto:'):
                continue
            links += 1
            require((p.parent / url).exists(), 'Broken link: ' + where + ' -> ' + url)
    require(not fence, 'Unclosed code fence: ' + str(p))

for folder in counts:
    for p in (ROOT / folder).rglob('*'):
        if not p.is_file() or '97-Legacy' in p.parts or p.suffix not in ('.hpp', '.py'):
            continue
        target = str(p.relative_to(ROOT))
        if p.name.startswith(('98-', '99-', '_98_', '_99_')) or p.name == '__init__.py':
            continue
        require(target in inventories or target in support_targets, 'Uninventoried active source: ' + target)
        if p.suffix == '.hpp':
            for inc in re.findall(r'^#include "([^"]+)"', p.read_text(), re.M):
                require((p.parent / inc).is_file(), 'Broken direct include: ' + target + ' -> ' + inc)

# Current path maps preserve historical source keys while their destinations stay live.
for name, key in [('12-path-map.json', 'headers'), ('18-path-updates.json', 'old_to_current'),
                  ('18-path-updates.json', 'changed'), ('21-core-path-updates.json', 'old_to_current'),
                  ('21-core-path-updates.json', 'moved_artifacts')]:
    mapping = read_json('00-Guidelines/' + name)[key]
    for old, target in mapping.items():
        require(target in inventories or (ROOT / target).is_file(), 'Unresolved current map destination: ' + name + ': ' + target)

notebook_spec = importlib.util.spec_from_file_location('library_notebook_check', ROOT / '98-Team Notebook/01-notebook.py')
notebook = importlib.util.module_from_spec(notebook_spec)
notebook_spec.loader.exec_module(notebook)
try:
    notebook.validate(read_json('98-Team Notebook/02-selection.json'), notebook.sources())
except (OSError, ValueError) as exc:
    require(False, 'Invalid notebook selection: ' + str(exc))

# All current quoted local includes resolve, including test fixtures and judge adapters.
for folder in [*counts, '96-Local Testing', '97-Online Testing/src', '99-Workspace']:
    for p in (ROOT / folder).rglob('*'):
        if not p.is_file() or '97-Legacy' in p.parts or p.suffix not in ('.hpp', '.cpp'):
            continue
        for inc in re.findall(r'^#include "([^"\n]+)"', p.read_text(), re.M):
            require((p.parent / inc).is_file(), 'Unresolved include: ' + str(p.relative_to(ROOT)) + ': ' + inc)

core_headers = {p.name for p in (ROOT / '01-Core').glob('*.hpp') if p.name[:2].isdigit() and int(p.name[:2]) < 98}
all_text = (ROOT / '01-Core/99-All.hpp').read_text()
all_includes = re.findall(r'^#include "([^"\n]+)"', all_text, re.M)
require(set(all_includes) == core_headers and len(all_includes) == len(set(all_includes)), 'Core All aggregate coverage/duplicates')
for name in ('98-Basic.hpp', '99-All.hpp'):
    incs = re.findall(r'^#include "([^"\n]+)"', (ROOT / '01-Core' / name).read_text(), re.M)
    require({'05-modint.hpp', '06-modintmini.hpp'} <= set(incs), 'Missing modular family in Core aggregate: ' + name)

# Moved suites must remain discoverable with one Python entry per verified header.
import ast
runner = ast.parse((ROOT / '96-Local Testing/01-run.py').read_text())
quick = next(ast.literal_eval(n.value) for n in runner.body if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'QUICK' for t in n.targets))
expected = {'01-template_tester.py', '02-debug_tester.py', '03-barrett_tester.py', '04-montgomery_tester.py', '05-modint_tester.py', '06-modintmini_tester.py', '18-bitset_tester.py'}
require(expected <= quick, 'Renamed/new verified suites missing from quick discovery')
for entry in quick:
    require((ROOT / '96-Local Testing/01-Core' / entry).is_file(), 'Quick entry missing: ' + entry)

# Generated expansions are optional, but any recorded manifest must be internally current.
expansion = ROOT / '97-Online Testing/02-expansion.json'
if expansion.exists():
    outputs = json.loads(expansion.read_text())['outputs']
    sources = {str(p.relative_to(ROOT / '97-Online Testing')) for p in (ROOT / '97-Online Testing/src').rglob('*.cpp') if not p.name.endswith('.expanded.cpp')}
    require(set(outputs) == sources, 'Expansion source manifest mismatch')
    for source, record in outputs.items():
        p = ROOT / '97-Online Testing' / record['output']
        require(p.is_file() and hashlib.sha256(p.read_bytes()).hexdigest() == record['sha256'], 'Expanded output missing/modified: ' + str(p))

# Validate missing-workspace reporting without touching the real generated snapshot.
import sys
import tempfile
spec = importlib.util.spec_from_file_location('library_integration_check', ROOT / '96-Local Testing/02-integration.py')
integration = importlib.util.module_from_spec(spec)
spec.loader.exec_module(integration)
with tempfile.TemporaryDirectory(prefix='cp-consistency-') as name:
    integration.ROOT = Path(name)
    argv = sys.argv
    try:
        sys.argv = ['02-integration.py']
        try:
            integration.main()
        except RuntimeError as exc:
            require('Required workspace snapshot missing:' in str(exc), 'Wrong missing-workspace failure: ' + str(exc))
        else:
            require(False, 'Missing workspace falsely passes integration')
    finally:
        sys.argv = argv

print(json.dumps({'targets': counts, 'total_targets': len(inventories), 'batches': len(batches), 'packages': len(sessions), 'support_passes': len(support), 'judge_families': len(coverage['records']), 'archive_files': len(archive['files']), 'monolith_entries': len(monolith['entries']), 'markdown_files': markdown_count, 'local_links': links, 'errors': errors}, indent=2))
raise SystemExit(bool(errors))
