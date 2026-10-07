#!/usr/bin/env python3
"""Read-only inventory, ownership, path, provenance and Markdown consistency checks."""
import collections
import hashlib
import importlib.util
import json
import re
import sys
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

def strip_cpp(text):
    """Blank comments, string/character literals and preprocessor lines; keep every newline and column."""
    out, i, n, line_start = [], 0, len(text), True
    while i < n:
        c = text[i]
        if c == '\n':
            out.append(c); i += 1; line_start = True
            continue
        if line_start and c not in ' \t':
            line_start = False
            if c == '#':
                while i < n and text[i] != '\n':
                    if text[i] == '\\' and i + 1 < n and text[i + 1] == '\n':
                        out.append(' \n'); i += 2
                    else:
                        out.append(' '); i += 1
                continue
        if text.startswith('//', i):
            j = text.find('\n', i); j = n if j < 0 else j
            out.append(' ' * (j - i)); i = j
            continue
        if text.startswith('/*', i):
            j = text.find('*/', i + 2); j = n if j < 0 else j + 2
            out.append(''.join('\n' if ch == '\n' else ' ' for ch in text[i:j])); i = j
            continue
        if c == '"' and i > 0 and text[i - 1] == 'R':
            j = text.find('(', i); delim = text[i + 1:j]
            k = text.find(')' + delim + '"', j); k = n if k < 0 else k + len(delim) + 2
            out.append(''.join('\n' if ch == '\n' else ' ' for ch in text[i:k])); i = k
            continue
        k = i
        while k > 0 and (text[k - 1].isalnum() or text[k - 1] == '_'):
            k -= 1
        separator = c == "'" and k < i and text[k].isdigit() and i + 1 < n and text[i + 1].isalnum()
        if c in '"\'' and not separator:
            j = i + 1
            while j < n and text[j] != c and text[j] != '\n':
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            out.append(c + ' ' * (j - i - 2) + c if j - i >= 2 else ' ' * (j - i)); i = j
            continue
        out.append(c); i += 1
    return ''.join(out)

def brace_violations(path):
    """03-cpp.md closing-brace rule: a multi-line function, control or lambda block closes as ';}' on its
    last statement's line and nests as '}}'; struct/class/union/enum bodies close with '};' alone and
    namespaces with '} // namespace name' alone; 'else' starts a new line. Initializer braces (after '=',
    '(', ',', '[', '{', 'return', a range-for ':' or directly attached to a type name) are not blocks.
    Limits: braces inside preprocessor conditionals must balance per branch; raw strings need a plain delimiter."""
    rel = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
    original = path.read_text().split('\n')
    lines = strip_cpp('\n'.join(original)).split('\n')
    stack, out, stmt = [], [], ''  # stmt: text since the last ';', '{' or '}', across lines
    for i, line in enumerate(lines):
        stmt += ' '  # a line break separates tokens: ')\nstruct' is not ')struct'
        for j, ch in enumerate(line):
            if ch in ';{}':
                stmt, head = '', stmt
            else:
                stmt += ch
            if ch == '{':
                before = line[:j]
                prev = before.rstrip()
                adjacent = len(prev) == len(before) and bool(prev) and (prev[-1].isalnum() or prev[-1] in '_>]')
                kind = 'block'
                if re.search(r'\bnamespace\b', head):
                    kind = 'namespace'
                elif re.search(r'(?:^|[>\s])(?:struct|class|union|enum)\s+[^(){}=;]*$', head):
                    kind = 'type'
                elif adjacent or not prev or prev[-1] in '=(,[{' or re.search(r'\breturn$', prev) \
                        or (prev[-1] == ':' and not re.search(r'\b(?:case|default)\b', head)):
                    kind = 'init'
                stack.append((i, kind))
            elif ch == '}':
                if not stack:
                    out.append(f'Closing brace: {rel}:{i + 1}: unmatched closing brace')
                    continue
                opened, kind = stack.pop()
                if opened == i or kind == 'init':
                    continue
                after = line[j + 1:]
                if kind == 'namespace':
                    if not re.fullmatch(r'\s*} // namespace( \S+)?', original[i].rstrip()):
                        out.append(f'Closing brace: {rel}:{i + 1}: namespace must close as "}} // namespace name" on its own line')
                elif kind == 'type':
                    if line.strip() != '};':
                        out.append(f'Closing brace: {rel}:{i + 1}: struct/class/union/enum body must close as "}};" on its own line')
                elif not line[:j].strip() or line[j - 1] in ' \t':
                    out.append(f'Closing brace: {rel}:{i + 1}: multi-line block must close as ";}}" on its last statement line')
                elif re.match(r'\s*else\b', after):
                    out.append(f'Closing brace: {rel}:{i + 1}: "else" starts a new line after ";}}"')
    if stack:
        out.append(f'Closing brace: {rel}:{stack[-1][0] + 1}: unclosed brace')
    return out

def comment_violations(path):
    """03-cpp.md comment cap for verified headers: at most two consecutive comment-only lines and at most 8% comment-only lines, but always one (the required complexity line of a one-function header)."""
    rel = str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path)
    lines = path.read_text().splitlines()
    out, run, start = [], 0, 0
    for i, line in enumerate(lines + ['']):
        if re.match(r'\s*//', line):
            run += 1
            start = i if run == 1 else start
        else:
            if run > 2:
                out.append(f'Comment cap: {rel}:{start + 1}: {run} consecutive comment lines (max 2); move the contract to the evidence document')
            run = 0
    comment = sum(bool(re.match(r'\s*//', l)) for l in lines)
    nonblank = sum(bool(l.strip()) for l in lines)
    if nonblank and comment > max(1, 0.08 * nonblank):
        out.append(f'Comment cap: {rel}: {comment} comment-only lines of {nonblank} ({100 * comment // nonblank}%, max 8%)')
    return out


if len(sys.argv) > 1 and sys.argv[1] == '--braces':
    found = [v for arg in sys.argv[2:] for f in (Path(arg).resolve(),) for v in brace_violations(f) + comment_violations(f)]
    print('\n'.join(found) if found else 'no closing-brace or comment-cap violations')
    raise SystemExit(bool(found))

inventories = {}
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
        rows.append((int(re.search(r'\d{2}', name)[0]), tier))
    # Prefixes are stable IDs: unique within a folder; new rows take the next free number.
    require(len({n for n, _ in rows}) == len(rows), 'Repeated target prefix: ' + folder.name)
    if folder.name != '01-Core':
        order = {'Basic': 0, 'Advanced': 1, 'Esoteric': 2}
        require([order[t] for _, t in rows] == sorted(order[t] for _, t in rows), 'Tier order: ' + folder.name)
    counts[folder.name] = len(rows)

plan_spec = importlib.util.spec_from_file_location('library_plan', ROOT / '00-Guidelines/13-Plan/plan.py')
plan = importlib.util.module_from_spec(plan_spec)
plan_spec.loader.exec_module(plan)
errors.extend(plan.check(ROOT))
batch_map, package_map = plan.load(ROOT)
batches = {b['id']: b for b in batch_map['batches']}
support = {b for b, v in batches.items() if v['folder'] == 'support'}
support_targets = [t for b in batches.values() for t in b.get('support_targets', [])]
require(len(support_targets) == len(set(support_targets)), 'Helper ownership mismatch')
for t in support_targets:
    require((ROOT / t).is_file(), 'Missing shared helper: ' + t)

# Closing-brace rule for headers whose inventory row and owning package are both verified, and their C++ testers.
owner = {t: p['id'] for p in package_map['packages'] for b in p['batches'] for t in batches.get(b, {}).get('targets', [])}
package_status = {p['id']: p['status'] for p in package_map['packages']}
brace_checked = 0
for target, parts in inventories.items():
    if not target.endswith('.hpp') or not parts[-1].startswith('verified') or package_status.get(owner.get(target)) != 'verified':
        continue
    errors.extend(comment_violations(ROOT / target))
    for f in [ROOT / target, *(ROOT / t for t in plan.tests_for(ROOT, target) if t.endswith(('.cpp', '.hpp')))]:
        errors.extend(brace_violations(f))
        brace_checked += 1

coverage = read_json('00-Guidelines/Ledgers/library-checker-coverage.json')
require(len(coverage['records']) == coverage['problem_families'], 'Judge family count')
require(len({r['problem'] for r in coverage['records']}) == len(coverage['records']), 'Duplicate judge family')
for record in coverage['records']:
    require(bool(record['targets']) and set(record['targets']) <= inventories.keys(), 'Unmapped judge family: ' + record['problem'])

archive = read_json('00-Guidelines/Ledgers/archive-map.json')
actual_archive = {str(p.relative_to(ROOT)) for p in (ROOT / 'OLD').rglob('*') if p.is_file()}
require(actual_archive == {r['path'] for r in archive['files']}, 'Archive manifest coverage mismatch')
require(archive['summary']['files'] == archive['summary']['classified'] == len(archive['files']), 'Archive summary count mismatch')
require(archive['summary']['classifications'] == dict(collections.Counter(r['classification'] for r in archive['files'])), 'Archive classification totals mismatch')
require(archive['summary']['canonical_inventory_targets_referenced'] == len({t for r in archive['files'] for t in r['targets']}), 'Archive referenced-target count mismatch')
for record in archive['files']:
    require(hashlib.sha256((ROOT / record['path']).read_bytes()).hexdigest() == record['sha256'], 'Archive hash mismatch: ' + record['path'])
    require(set(record['targets']) <= inventories.keys(), 'Archive target mismatch: ' + record['path'])

monolith = read_json('00-Guidelines/Ledgers/monolith-map.json')
for source, meta in monolith['sources'].items():
    require(hashlib.sha256((ROOT / source).read_bytes()).hexdigest() == meta['sha256'], 'Monolith source hash: ' + source)
for entry in monolith['entries']:
    if 'body_sha256' not in entry:
        continue
    source = (ROOT / entry['source']).read_bytes().splitlines(keepends=True)
    body = b''.join(source[entry['start_line'] - 1:entry['end_line']])
    require(hashlib.sha256(body).hexdigest() == entry['body_sha256'], 'Original excerpt hash: ' + entry['source'] + ':' + str(entry['start_line']))
    if 'target_body_start_line' in entry and not ('97-Legacy' in entry['target'] and not (ROOT / entry['target']).exists()):
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
for name, key in [('Ledgers/path-map.json', 'headers'), ('Ledgers/path-updates.json', 'old_to_current'),
                  ('Ledgers/path-updates.json', 'changed'), ('Ledgers/core-path-updates.json', 'old_to_current'),
                  ('Ledgers/core-path-updates.json', 'moved_artifacts')]:
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
all_text = (ROOT / '01-Core/99-all.hpp').read_text()
all_includes = re.findall(r'^#include "([^"\n]+)"', all_text, re.M)
require(set(all_includes) == core_headers and len(all_includes) == len(set(all_includes)), 'Core All aggregate coverage/duplicates')
for name in ('98-basic.hpp', '99-all.hpp'):
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

print(json.dumps({'targets': counts, 'total_targets': len(inventories), 'batches': len(batches) - len(support), 'packages': len(package_map['packages']), 'support_passes': len(support), 'package_status': dict(collections.Counter(p['status'] for p in package_map['packages'])), 'judge_families': len(coverage['records']), 'archive_files': len(archive['files']), 'monolith_entries': len(monolith['entries']), 'markdown_files': markdown_count, 'local_links': links, 'brace_checked_files': brace_checked, 'errors': errors}, indent=2))
raise SystemExit(bool(errors))
