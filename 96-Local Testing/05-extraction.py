#!/usr/bin/env python3
"""Extraction harness for 09-Contest Testing/10-grab.py (rules: 05-testing.md, Extraction; evidence: Docs/00-extraction.md).

  ops      every operation of every implemented 01-07 inventory row extracted alone (Type::member for a struct's
           operation, base members and operators included; a nested struct or enum that spells it whole) and compiled
           with -fsyntax-only after the template and debug headers, on $CXX (default g++) and g++-14, with probes that
           instantiate each template it reaches with the first candidate arguments compiling after the whole header;
           identical extractions compile once; a public 02-07 function template whose type parameter grab must
           infer fails (03-cpp.md, Headers: it takes a default), and -v lists templates no candidate instantiates
  testers  every C++ tester's quick mode on its extracted build: a scratch tree where every library header except the
           template and debug headers is replaced by the declarations the tester's files name (grab's --into --min rule), so
           C++ the tester writes itself extracts too
  both     (default) ops, then testers

--mode quick samples --sample rows and testers with --seed; --mode full takes them all. Each compile and each tester
runs memory-capped (the testers cap themselves); a tester that fails beside others is rerun alone, and only a failure
alone counts. Without libclang it prints SKIP and exits 0. Exit 0 pass, 1 failure.
"""
import argparse
import ast
import concurrent.futures
import importlib.util
import itertools
import json
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
sys.path.insert(0, str(HERE))
import _00_memory_cap  # noqa: E402

_spec = importlib.util.spec_from_file_location('extraction_grab', ROOT / '09-Contest Testing/10-grab.py')
grab = importlib.util.module_from_spec(_spec)
sys.modules[_spec.name] = grab  # forked index workers find grab's functions by module name
_spec.loader.exec_module(grab)
GREEN, RED, RESET = '\033[1;32m', '\033[1;31m', '\033[0m'
COMPILE_MB = 2048
PRELUDE = '#include "{root}/01-Core/01-template.hpp"\n#include "{root}/01-Core/02-debug.hpp"\n'
MONOIDS = '02-Data Structures/00-monoids.hpp'  # its presets are candidate arguments of monoid templates
PRESETS = ('Sum', 'RangeAddRangeSum')
INTS = {'int', 'unsigned', 'uint', 'lng', 'ulng', 'size_t', 'char', 'unsigned char', 'long long', 'auto'}


def cells(line):
    return [x.strip() for x in re.split(r'(?<!\\)\|', line.strip())[1:-1]]


def split(text, sep, angle=False):
    """text split at sep outside parentheses and brackets, a half-open [l, r) included, and with angle outside template
    brackets (a `<` right after a name other than `operator`)."""
    out, depth, angles, cur = [], 0, 0, ''
    for ch in text:
        depth += (ch in '([') - (ch in ')]')
        if angle and ch == '<' and re.search(r'\w$', cur) and not re.search(r'\boperator$', cur):
            angles += 1
        elif angle and ch == '>' and angles:
            angles -= 1
        if ch == sep and depth <= 0 and not angles:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    return [x.strip() for x in out + [cur] if x.strip()]


def unqualified(text):
    """text without its parenthesized qualifiers (balanced, brackets counted as in split)."""
    out, depth = '', 0
    for ch in text:
        depth += ch in '(['
        if depth <= 0:
            out += ch
        depth -= ch in ')]'
    return re.sub(r'\s+(?=[,:]|$)', '', out)


def depths(text):
    """Bracket depth before each character (parentheses and brackets, as in split)."""
    out, depth = [], 0
    for ch in text:
        out.append(depth)
        depth += (ch in '([') - (ch in ')]')
    return out


def operations(cell):
    """(group types, operation) pairs of an Operations cell: 'A, B: x, y; z' -> ([A, B], x), ([A, B], y), ([], z)."""
    out = []
    for item in split(cell.replace('internals:', ';'), ';'):
        cut = next((k for k, d in enumerate(depths(item)) if d == 0 and item.startswith(': ', k)), None)
        head, ops = (item[:cut], item[cut + 2:]) if cut is not None and re.fullmatch(r'[\w:<>, ]+', unqualified(item[:cut])) else ('', item)
        types = [re.sub(r'<.*', '', unqualified(t)).strip() for t in split(head, ',', True)] if head else []
        for op in split(ops, ',', True):
            op = op.replace('\\|', '|')
            sym = re.match(r'operator\s*(\(\)|\[\]|[^\w\s(\[]+)|([^\w\s(\[]+)(?:\s|$)', op)  # operator(), +=, ==
            out.append(([t for t in types if re.fullmatch(r'[\w:]+', t)], 'operator' + (sym[1] or sym[2]) if sym else re.sub(r'\s*\(.*', '', op).strip()))
    return out


def rows(root, status=False):
    """(header, Operations cell, or Status cell with status) of every inventory row whose header exists."""
    out = []
    for index in sorted(root.glob('0[1-7]-*/00-index.md')):
        for line in index.read_text(encoding='utf-8').splitlines():
            c = cells(line) if line.startswith('| `') else []
            m = re.fullmatch(r'`(\d{2}-[^`]+\.hpp)`', c[0]) if c else None
            h = f'{index.parent.name}/{m[1]}' if m else None
            if h and (root / h).is_file() and h not in grab.EXCLUDED and not c[-1].startswith('planned'):
                out.append((h, c[-1] if status else c[-2]))
    return out


def bases(ix, k):
    """Base class names in the head of type unit k."""
    u = ix.units[k]
    head = '\n'.join(ix.text(u['header'])[u['ext'][0] - 1:u['head'][1]]).split('{')[0]
    m = re.search(r'\b(?:struct|class)\s+\w+(?:\s*<[^:]*>)?\s*:(?!:)(.*)$', grab.DEPS.strip_cpp(head), re.S)
    return [re.sub(r'<.*|\b(?:public|protected|private|virtual)\b', '', b).strip() for b in split(m[1], ',', True)] if m else []


def targets(ix, header, types, op):
    """Root units for one operation in header: a group type's member (through aliases and base classes; `ctor` its
    constructors; `operator+=` for `+=`), or the group type itself when it is a nested struct or enum, kept whole, whose
    text spells the operation; else a unit of header with that name, namespace-scope first, else any unit named by its
    last component (an operation listed outside its group, a `std::hash` specialization); None when header declares
    none (a type never stands in for an operation it does not spell)."""
    if not re.fullmatch(r'[\w:]+|operator\S+', op):
        return None
    mine = lambda i: ix.units[i]['header'] == header
    for t in types:
        kinds = {k for a in ix.resolve(t) for k in (ix.aliased(a) if ix.units[a].get('alias') else [a])}
        if op == 'ctor':  # the constructors of the type (through an alias)
            found = [c for k in kinds if ix.units[k]['kind'] == 'type' for c in ix.units[k]['children'] if ix.units[c]['name'].split('<')[0] == ix.units[k]['name'] and mine(c)]
        else:
            found = [i for i in ix.resolve(f'{t}::{op}') if mine(i)]
            found = found or [i for k in kinds if ix.units[k]['kind'] == 'type' for b in bases(ix, k) for i in ix.resolve(f'{b}::{op}')]
        word = ix.units[next(iter(kinds))]['name'] if op == 'ctor' and kinds else op
        found = found or [k for k in kinds if ix.units[k]['kind'] == 'leaf' and mine(k) and re.search(
            rf'(?<!\w){re.escape(word)}(?!\w)', '\n'.join(ix.text(header)[ix.units[k]['ext'][0] - 1:ix.units[k]['ext'][2]]))]
        if found:
            return found
    return [i for i in ix.resolve(op) if mine(i)] or [i for i in ix.by_name.get(op.split('::')[-1], ()) if mine(i)] or None


def final(root):
    """Targets whose mini-first findings fail (the rest are reported): a verified row of a verified package that is not
    a support target of an open 2026-11-05 batch (03-consistency.py shares this)."""
    spec = importlib.util.spec_from_file_location('extraction_plan', root / '00-Guidelines/13-Plan/plan.py')
    plan = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(plan)
    batch_map, package_map = plan.load(root)
    batches = {b['id']: b for b in batch_map['batches']}
    open_ = {b for p in package_map['packages'] if p['status'] != 'verified' for b in p['batches']}
    retrofit = {t for b in open_ if '2026-11-05' in batches.get(b, {}).get('focus', '') for t in batches[b].get('support_targets', [])}
    done = {t for p in package_map['packages'] if p['status'] == 'verified' for b in p['batches'] for t in batches.get(b, {}).get('targets', [])}
    verified = {h for h, c in rows(root, True) if c.startswith('verified')}
    return (done & verified) - retrofit


def mini_first_split(findings, strict):
    """(errors, pending) messages of mini-first findings (target, message): a strict target's fail."""
    return [m for t, m in findings if t in strict], [m for t, m in findings if t not in strict]


def core_full(root):
    """Core full headers other than scalar ones: a non-mini Core type header with a mini (a file or a Core inventory row,
    planned included), ISA code or a non-mini include
    (03-cpp.md, Core type parameters: a full type without a mini, ISA code or other includes, today 09-rational, counts
    as a mini)."""
    out, index = set(), root / '01-Core/00-index.md'
    rows = index.read_text(encoding='utf-8') if index.is_file() else ''
    for p in sorted((root / '01-Core').glob('[0-9][0-9]-*.hpp')):
        if p.name[:2] in ('01', '02', '98', '99') or 'mini' in p.name:
            continue
        text = p.read_text(encoding='utf-8')
        incs = [i for i, _ in grab.DEPS.includes(text)]
        mini = rf'[0-9][0-9]-{re.escape(p.stem[3:])}mini\.hpp'
        has_mini = any(re.fullmatch(mini, f.name) for f in (root / '01-Core').iterdir()) or re.search(rf'\| `{mini}` \|', rows)
        if has_mini or re.search(r'__(AVX|FMA|BMI|PCLMUL|SSE)', text) or any('mini' not in i and Path(i).name != '01-template.hpp' for i in incs):
            out.add(f'01-Core/{p.name}')
    return out


def full_type_names(root, full):
    """Types, aliases and concepts declared at namespace scope (detail namespaces included) in the Core full headers
    full -> their file names; a qualified head (`struct std::hash<...>`) or a specialization declares none."""
    out = {}
    for h in sorted(full):
        code = grab.DEPS.strip_cpp((root / h).read_text(encoding='utf-8'))
        at = grab.scope(code)
        for m in re.finditer(r'\b(?:struct|class|union|concept|using|enum(?:\s+class)?)\s+(?!namespace\b)([A-Za-z_]\w*)\b(?!\s*(?:<|::))', code):
            if at(m.start()) and not code[:m.start()].rstrip().endswith(('<', ',')):  # not a template parameter
                out[m[1]] = Path(h).name
    return out


def full_defaults(text, names):
    """Names among names (Core full types) that a header's template parameter defaults spell anywhere, wrapped ones
    (`std::conditional_t<..., X, ...>`) included."""
    out = set()
    for params in re.findall(r'\btemplate\s*<([^;{]*?)>\s*(?=[a-zA-Z])', grab.DEPS.strip_cpp(text)):
        for p in split(params, ',', True):
            if '=' in p:
                out |= {n for n in re.findall(r'[A-Za-z_][\w:]*', p.split('=', 1)[1]) if n.split('::')[-1] in names}
    return sorted(out)


def operation_names(cell, status=''):
    """Operation names of a row without group types and the Mini suffix, its internals and missing ones included."""
    cell += '; ' + (status.split('missing:', 1)[1] if 'missing:' in status else '')
    return {re.sub(r'Mini|mini', '', op) for _, op in operations(cell) if op != 'internals'}


def full_only(ix, found, full):
    """(header, operation, Core full headers) for each contest operation whose default form extracts a Core full header
    while its evidence document has no line naming it with `full-only until`."""
    out = []
    for header, cell in found:
        if header.startswith('01-Core/'):
            continue
        doc = ROOT / header.split('/')[0] / 'Docs' / (Path(header).stem + '.md')
        records = [l for l in (doc.read_text(encoding='utf-8').splitlines() if doc.is_file() else []) if 'full-only until' in l]
        for types, op in operations(cell):
            roots = targets(ix, header, types, op)
            reached = sorted(full & set(grab.select(ix, roots))) if roots else []
            if reached and not any(f'`{op}`' in l for l in records):
                out.append((header, op, reached))
    return out


def probe_units(ix, roots):
    """Template units whose instantiation probes roots: the class template of a root type or member, a namespace-scope
    function template root (-fsyntax-only instantiates nothing else)."""
    out = []
    head = lambda j: ix.text(ix.units[j]['header'])[ix.units[j]['ext'][0] - 1].lstrip().startswith('template')
    for i in roots:
        u = ix.units[i]
        t = i if u['kind'] == 'type' else u['parent'] if u['parent'] is not None and ix.units[u['parent']]['kind'] == 'type' else None
        if t is not None:
            out += [t] if head(t) else []
        elif u['kind'] == 'leaf' and ix.toplevel(i) and not u.get('alias') and head(i):
            out.append(i)
    return list(dict.fromkeys(out))


def parameters(ix, k):
    """Kinds of unit k's template parameters before its first default ('type', 'int', 'bool'), None when one is of
    another kind."""
    u = ix.units[k]
    text = grab.DEPS.strip_cpp('\n'.join(ix.text(u['header'])[u['ext'][0] - 1:u['ext'][2]]))
    m = re.match(r'\s*template\s*<', text)
    depth, end = 1, m.end() if m else 0
    while m and end < len(text) and depth:
        depth += (text[end] == '<') - (text[end] == '>')
        end += 1
    out = []
    for p in (split(text[m.end():end - 1], ',', True) if m else []):
        if '=' in p or '...' in p:
            break
        kind = re.sub(r'\s*\b[A-Za-z_]\w*$', '', p.strip())
        out.append('bool' if kind == 'bool' else 'int' if kind in INTS else 'type' if kind in ('typename', 'class') or re.fullmatch(r'[\w:]+', kind) else None)
    return None if None in out else out


def candidates(ix, k):
    """Explicit uses instantiating template unit k, most likely first: `template struct X<args>;` for a class template,
    `&f<args>` for a function template, with lng, the monoid presets and ulng for type parameters without defaults."""
    kinds = parameters(ix, k)
    if kinds is None:
        return []
    presets = [f'{n}<lng>' for n in PRESETS if any(ix.units[i]['header'] == MONOIDS for i in ix.resolve(n))]
    values = {'type': ['lng', *presets, 'ulng'], 'int': ['4'], 'bool': ['false']}
    combos = sorted(itertools.product(*(range(len(values[x])) for x in kinds)), key=lambda c: (sum(c), c))[:12]
    qual = ix.units[k]['qual']
    return [f'template struct {qual}<{a}>;' if ix.units[k]['kind'] == 'type' else f'&{qual}<{a}>'
            for a in (', '.join(values[x][c] for x, c in zip(kinds, combo)) for combo in combos)]


def probes(ix, roots):
    """The first candidate of each template unit roots instantiate."""
    return [c[0] for k in probe_units(ix, roots) for c in [candidates(ix, k)] if c]


def probe_text(ps):
    return ''.join(p + '\n' if p.startswith('template') else f'[[maybe_unused]] static auto *grab_probe_{k} = {p};\n'
                   for k, p in enumerate(ps))


def preset_roots(ix, ps):
    """Units of the monoid presets the probes ps name, which their extraction also needs."""
    return [i for n in PRESETS if any(re.search(rf'\b{n}<', p) for p in ps) for i in ix.resolve(n) if ix.units[i]['header'] == MONOIDS]


def compile_check(cxx, source):
    """None when source passes -fsyntax-only, else the first error lines."""
    cmd = _00_memory_cap.capped([cxx, '-std=gnu++20', '-fsyntax-only', '-x', 'c++', source], COMPILE_MB)
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=600)
    if r.returncode == 0:
        return None
    errs = [l for l in r.stderr.splitlines() if 'error' in l] or r.stderr.splitlines()
    return '\n'.join(errs[:4]) or f'exit {r.returncode}'


def control(compilers, wanted, work, jobs):
    """(header, unit) -> the first of its candidate probes (wanted) that compiles after the whole header and the monoid
    presets: the probe its extraction must pass too. Each round tries every pending unit's next candidate, all of a
    header's in one build and one at a time where that build fails."""
    chosen, r = {}, 0
    monoids = f'#include "{ROOT}/{MONOIDS}"\n' if (ROOT / MONOIDS).is_file() else ''

    def run(h, ps, k):
        path = work / f'control{k}.cpp'
        path.write_text(PRELUDE.format(root=ROOT) + monoids + f'#include "{ROOT}/{h}"\n' + probe_text(ps))
        return compile_check(compilers[0], str(path)) is None
    while compilers:
        batch = {key: c[r] for key, c in wanted.items() if key not in chosen and r < len(c)}
        if not batch:
            return chosen
        by_header = {}
        for (h, k), p in sorted(batch.items()):
            by_header.setdefault(h, []).append(((h, k), p))
        retry = []
        with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
            whole = {pool.submit(run, h, [p for _, p in kp], f'r{r}h{n}'): kp for n, (h, kp) in enumerate(sorted(by_header.items()))}
            for f in concurrent.futures.as_completed(whole):
                if f.result():
                    chosen.update(whole[f])
                else:
                    retry += whole[f]
            single = {pool.submit(run, key[0], [p], f'r{r}p{n}'): (key, p) for n, (key, p) in enumerate(sorted(retry))}
            chosen.update(single[f] for f in concurrent.futures.as_completed(single) if f.result())
        r += 1
    return chosen


def undefaulted(ix, headers):
    """(header, template, types) for each public function template of the contest headers among headers (02-07) with a
    type parameter grab must infer
    (grab.inputs): 03-cpp.md (Headers) asks for its default."""
    U = ix.units
    return [(h, U[i]['qual'], [U[t]['qual'] for t in ts]) for h in headers for i in ix.where.get(h, ())
            if re.match(r'0[2-7]-', h) and U[i]['kind'] == 'leaf' and not U[i]['detail'] for ts in [grab.inputs(ix, i, True)] if ts]


def check_ops(ix, a, work):
    found = [r for r in rows(ROOT) if a.filter in r[0]]
    if a.mode == 'quick':
        found = random.Random(a.seed).sample(found, min(a.sample, len(found)))
    entries, unresolved = [], []
    for header, cell in found:
        for types, op in operations(cell):
            roots = targets(ix, header, types, op)
            if roots is None:
                unresolved.append(f'{header}: {op}')
            else:
                entries.append((header, f'{header}: {"::".join(types[:1] + [op]) if types else op}', roots))
    compilers = [c for c in dict.fromkeys([os.environ.get('CXX', 'g++'), 'g++-14']) if shutil.which(c)]
    for c in {os.environ.get('CXX', 'g++'), 'g++-14'} - set(compilers):
        print(f'SKIP {c}: not installed')
    wanted = {(h, k): candidates(ix, k) for h, _, roots in entries for k in probe_units(ix, roots)}
    chosen = control(compilers[:1], {key: c for key, c in wanted.items() if c}, work, a.jobs) if compilers else {}
    jobs = {}
    for header, label, roots in entries:
        ps = sorted({chosen[(header, k)] for k in probe_units(ix, roots) if (header, k) in chosen})
        jobs.setdefault((grab.render(ix, grab.select(ix, roots + preset_roots(ix, ps))), tuple(ps)), []).append(label)
    tasks, probed = [], 0
    for k, ((text, ps), names) in enumerate(jobs.items()):
        path = work / f'op{k}.cpp'
        probed += bool(ps)
        path.write_text(PRELUDE.format(root=ROOT) + text + probe_text(ps))
        if '#include "' in text:
            tasks.append((None, names, 'extraction keeps a quoted include', None))
        tasks += [(cxx, names, None, str(path)) for cxx in compilers]
    failures = 0
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        futures = {pool.submit(compile_check, cxx, p): (cxx, names) for cxx, names, err, p in tasks if p}
        failures += sum(1 for cxx, names, err, p in tasks if err and not print(f'{RED}FAIL{RESET} {names[0]}: {err}'))
        for f in concurrent.futures.as_completed(futures):
            cxx, names = futures[f]
            err = f.result()
            if err:
                failures += 1
                print(f'{RED}FAIL{RESET} {cxx}: {", ".join(names[:3])}{" ..." if len(names) > 3 else ""}\n{err}')
    lacking, strict = full_only(ix, found, core_full(ROOT)), final(ROOT)
    for header, op, reached in lacking:
        label = f'{RED}FAIL{RESET}' if header in strict else 'PENDING'
        print(f'{label} {header}: {op}: default form extracts {", ".join(reached)} without a `full-only until` record')
    failures += sum(h in strict for h, _, _ in lacking)
    loose = undefaulted(ix, sorted({h for h, _ in found}))
    for header, qual, types in loose:
        print(f'{RED + "FAIL" + RESET if header in strict else "PENDING"} {header}: {qual}: undefaulted template parameter '
              f'grab infers as {", ".join(types)} (03-cpp.md, Headers)')
    failures += sum(h in strict for h, _, _ in loose)
    total = sum(len(n) for n in jobs.values())
    unprobed = sorted({f'{h}: {ix.units[k]["qual"]}' for h, k in wanted if (h, k) not in chosen})  # 03-cpp.md, Headers: library-type defaults
    print(f'ops: {len(found)} rows, {total} operations in {len(jobs)} distinct extractions on {", ".join(compilers)} '
          f'({probed} instantiating their default form); {failures} failure(s); {len(unresolved)} names declare nothing in their header; '
          f'{len(unprobed)} templates no candidate instantiates')
    if a.verbose:
        print('\n'.join(['  unresolved ' + u for u in unresolved] + ['  unprobed ' + u for u in unprobed]))
    return failures


def cpp_testers(root):
    """C++ testers: entries whose folder holds C++ files of their prefix or whose text writes an #include."""
    out = []
    for t in sorted((root / '96-Local Testing').glob('0[1-7]-*/*_tester.py')):
        if '_compile_' in t.name:
            continue
        prefix = t.name[:3]
        files = [f for f in t.parent.iterdir() if f.name.startswith(prefix) and f.suffix in ('.cpp', '.hpp')]
        if files or '#include' in t.read_text(encoding='utf-8'):
            out.append((t, files))
    return out


def own_names(paths):
    """Names a tester's files declare: declared() of its C++ files and of the C++ in its Python string literals
    (literals holding `;`, a brace or `#include`; docstrings, comments and messages are prose)."""
    code = []
    for f in paths:
        text = f.read_text(encoding='utf-8')
        if f.suffix != '.py':
            code.append(text)
            continue
        tree = ast.parse(text)
        docs = {id(n.body[0].value) for n in ast.walk(tree) if isinstance(n, (ast.Module, ast.ClassDef, ast.FunctionDef))
                and n.body and isinstance(n.body[0], ast.Expr) and isinstance(n.body[0].value, ast.Constant)}
        code += [n.value for n in ast.walk(tree) if isinstance(n, ast.Constant) and isinstance(n.value, str)
                 and id(n) not in docs and re.search(r'[;{}]|#include', n.value)]
    return grab.declared(grab.DEPS.strip_cpp('\n'.join(code)))


def scratch(ix, tester, files, base):
    """A scratch tree for one tester: its folder, the shared runners, the template and debug headers, and every other
    library header replaced by its own lines of the tester's extraction under its original quoted includes.
    Returns (tester path in it, names extracted)."""
    rel = tester.relative_to(ROOT)
    tree = Path(tempfile.mkdtemp(prefix='cp-extraction-', dir=base))
    text = '\n'.join(f.read_text(encoding='utf-8') for f in [tester, *files])
    named = {h for h in ix.graph if Path(h).name in text and h not in grab.EXCLUDED}  # the headers its builds include
    for a in sorted(ROOT.glob('0[1-7]-*/9[89]-*.hpp')):
        if a.relative_to(ROOT).as_posix() in text or (a.parent.name == rel.parts[1] and a.name in text):
            named |= set(grab.aggregate(ROOT, ix.graph, a.relative_to(ROOT).as_posix()))
    allowed = {d for h in named for d in grab.closure(ix.graph, h)} or None
    lines, names = grab.lines_of(ix, text, allowed, raw=True, own=own_names([tester, *files]))  # testers write C++ in literals
    for h in ix.graph:
        dst = tree / h
        dst.parent.mkdir(parents=True, exist_ok=True)
        if h in grab.EXCLUDED:
            shutil.copy2(ROOT / h, dst)
            continue
        incs = [l for l in ix.text(h) if re.match(r'\s*#\s*include\s*"', l)]
        dst.write_text('#pragma once\n' + ''.join(l + '\n' for l in incs) + (grab.render(ix, {h: lines[h]}) if h in lines else ''))
    for a in ROOT.glob('0[1-7]-*/9[89]-*.hpp'):
        shutil.copy2(a, tree / a.relative_to(ROOT))
    tests = tree / '96-Local Testing'
    shutil.copytree(ROOT / '96-Local Testing' / rel.parts[1], tests / rel.parts[1],
                    ignore=shutil.ignore_patterns('__pycache__', '.core_test_*', '*_benchmark.json*'))
    for f in (ROOT / '96-Local Testing').glob('_*.py'):
        shutil.copy2(f, tests / f.name)
    return tree / rel, names


def run_tester(path, timeout):
    env = {k: v for k, v in os.environ.items() if k != 'CP_MEMORY_CAPPED'}
    env.update(CP_TEST_MODE='quick', CP_TEST_PREFLIGHT='0')
    start = time.monotonic()
    try:
        r = subprocess.run([sys.executable, str(path), '--mode', 'quick'], capture_output=True, text=True,
                           timeout=timeout, env=env, stdin=subprocess.DEVNULL)
        return r.returncode, (r.stdout + r.stderr)[-3000:], time.monotonic() - start
    except subprocess.TimeoutExpired:
        return -1, f'timeout after {timeout} s', time.monotonic() - start


def check_testers(ix, a, work):
    found = [t for t in cpp_testers(ROOT) if a.filter in str(t[0].relative_to(ROOT))]
    if a.mode == 'quick':
        found = sorted(random.Random(a.seed).sample(found, min(a.sample, len(found))))
    prepared = [(t, *scratch(ix, t, files, work)) for t, files in found]
    results = {}
    with concurrent.futures.ThreadPoolExecutor(a.jobs) as pool:
        futures = {pool.submit(run_tester, path, a.timeout): t for t, path, _ in prepared}
        for f in concurrent.futures.as_completed(futures):
            results[futures[f]] = f.result()
    failures = 0
    for t, path, names in prepared:
        code, out, secs = results[t]
        if code and a.jobs > 1:
            code, out, secs = run_tester(path, a.timeout)  # only a failure alone counts
        label = t.relative_to(ROOT / '96-Local Testing')
        if code:
            failures += 1
            print(f'{RED}FAIL{RESET} {label} ({len(names)} names, exit {code})\n{out.strip()}')
        else:
            print(f'{GREEN}PASS{RESET} {label} ({len(names)} names, {secs:.0f} s)')
    print(f'testers: {len(prepared)} quick runs on extracted builds; {failures} failure(s)')
    return failures


def main(argv=None):
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0], formatter_class=argparse.RawDescriptionHelpFormatter,
                                epilog='\n'.join(__doc__.splitlines()[1:]))
    p.add_argument('part', nargs='?', choices=['ops', 'testers', 'both'], default='both')
    p.add_argument('--mode', choices=['quick', 'full'], default='full')
    p.add_argument('--seed', type=int, default=1)
    p.add_argument('--sample', type=int, default=8, help='rows and testers sampled in quick mode (default 8)')
    p.add_argument('--filter', default='', help='substring of the header or tester path')
    p.add_argument('-j', '--jobs', type=int, default=min(6, os.cpu_count() or 1), help='parallel compiles and testers (default min(6, CPUs))')
    p.add_argument('--timeout', type=int, default=1800, help='seconds per tester (default 1800)')
    p.add_argument('-v', '--verbose', action='store_true', help='list the operation names that declare nothing')
    a = p.parse_args(argv)
    if a.jobs < 1 or a.sample < 1:
        p.error('--jobs and --sample must be positive')
    ix = grab.Index(ROOT)
    if ix.ci is None:
        print('SKIP extraction: python3 cannot import clang.cindex (libclang); see /setup-check')
        return 0
    with tempfile.TemporaryDirectory(prefix='cp-extraction-') as work:
        failures = (check_ops(ix, a, Path(work)) if a.part in ('ops', 'both') else 0)
        failures += (check_testers(ix, a, Path(work)) if a.part in ('testers', 'both') else 0)
    print(f'{RED}FAIL{RESET}: {failures} failure(s)' if failures else f'{GREEN}PASS{RESET}: extraction')
    return int(bool(failures))


if __name__ == '__main__':
    raise SystemExit(main())
