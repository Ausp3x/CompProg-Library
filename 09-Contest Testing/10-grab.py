#!/usr/bin/env python3
"""Extract the library declarations some names reach, without the template and debug headers.

  10-grab.py NAME...        print the declarations NAME... reach (Type::member names one member) and copy them
  10-grab.py --into FILE    keep one marked block above solve in FILE holding what grab prints for the library names
                            FILE uses (--min: those names pruned to the members FILE calls)
  10-grab.py --find TEXT    list the indexed names containing TEXT

The index (libclang, both ISA configurations) is cached per header and include-closure hash in .grab-cache beside this
script. Without libclang, whole headers are inlined instead (SKIP). Rules: 00-Guidelines/08-contest-tools.md.
"""
from __future__ import annotations

import argparse
import bisect
import concurrent.futures
import difflib
import hashlib
import importlib.util
import inspect
import json
import multiprocessing
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CACHE = Path(__file__).resolve().parent / '.grab-cache'
VERSION = 8  # bump when the cached record format changes (the analysis code and libclang's version are hashed in)
EXCLUDED = ('01-Core/01-template.hpp', '01-Core/02-debug.hpp')  # the contest template already holds both
CONFIGS = {'scalar': [], 'isa': ['-mavx2', '-mfma', '-mbmi', '-mbmi2', '-mpclmul', '-mpopcnt', '-mlzcnt']}
PROTOCOL = {'begin', 'end', 'rbegin', 'rend', 'cbegin', 'cend', 'size', 'empty', 'data', 'swap', 'get', 'hash'}
BEGIN, END = '// grab begin: ', '// grab end'
DIRECTIVE = re.compile(r'\s*#\s*(if|ifdef|ifndef|elif|elifdef|elifndef|else|endif)\b')
SYSTEM_INCLUDE = re.compile(r'\s*#\s*include\s*<')
IDENT = re.compile(r'[A-Za-z_]\w*')
KEYWORDS = {'return', 'case', 'else', 'do', 'goto', 'throw', 'new', 'delete', 'co_return', 'co_yield', 'sizeof', 'not', 'and', 'or', 'operator'}
RESERVED = KEYWORDS | set('''alignas alignof asm auto bitand bitor bool break catch char char8_t char16_t char32_t class compl concept
    const consteval constexpr constinit const_cast continue co_await decltype default double dynamic_cast enum explicit export
    extern false float for friend if inline int long mutable namespace noexcept not_eq nullptr private protected public
    register reinterpret_cast requires short signed static static_assert static_cast struct switch template this
    thread_local true try typedef typeid typename union unsigned using virtual void volatile wchar_t while xor or_eq
    and_eq xor_eq'''.split())

_spec = importlib.util.spec_from_file_location('grab_dependencies', ROOT / '98-Team Notebook/05-dependencies.py')
DEPS = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(DEPS)


class GrabError(Exception):
    pass


def cindex():
    """The libclang Python module, or None when it cannot load (then grab inlines whole headers)."""
    try:
        import clang.cindex as ci
        ci.Index.create()
        return ci
    except Exception:
        return None


def resource_dir() -> list[str]:
    """-resource-dir for libclang (its builtin headers such as float.h), found beside the loaded library."""
    ci, libs = cindex(), []  # loads libclang first
    try:
        libs = [Path(l.split()[-1]) for l in Path('/proc/self/maps').read_text().splitlines() if 'libclang' in l.split()[-1]]
    except OSError:
        pass
    if ci is not None:
        libs.append(Path(ci.conf.lib._name))  # where /proc is missing (macOS)
    for lib in libs:
        for base in (lib.parent / 'clang', lib.parent.parent / 'lib' / 'clang'):
            found = sorted(base.glob('*/include/stddef.h'))
            if found:
                return ['-resource-dir', str(found[-1].parents[1])]
    return []


def library(root: Path = ROOT) -> dict[str, list[str]]:
    """Library header -> its quoted includes (repository-relative), aggregates excluded."""
    return {h: incs for h, incs in DEPS.headers(root).items() if not re.search(r'/9[89]-', h)}


def closure(graph: dict[str, list[str]], header: str) -> list[str]:
    return [h for h in DEPS.closure(graph, header) if h in graph]


_ANALYSIS: list[str] = []


def analysis_id() -> str:
    """libclang's version, the constants and the source of the functions the analysis reads: a change to any re-indexes
    everything (system headers are checked by their stamps, Index)."""
    if not _ANALYSIS:
        ci = cindex()
        version = str(ci._CXString.from_result(ci.conf.lib.clang_getClangVersion())) if ci else 'none'
        consts = repr((sorted(PROTOCOL), EXCLUDED, CONFIGS, DIRECTIVE.pattern, SYSTEM_INCLUDE.pattern))
        _ANALYSIS.append(version + consts + ''.join(inspect.getsource(f) for f in (analyze, scan_lines, merge, index_header, resource_dir, library, stamp, DEPS.headers, DEPS.closure)))
    return _ANALYSIS[0]


def stamp(path: str) -> list[int] | None:
    """Size and modification time of a system header, None when it is gone."""
    try:
        st = os.stat(path)
    except OSError:
        return None
    return [st.st_size, st.st_mtime_ns]


def key(root: Path, graph: dict[str, list[str]], header: str) -> str:
    h = hashlib.sha256(f'{VERSION}\0{CONFIGS}\0{analysis_id()}'.encode())
    for dep in closure(graph, header):
        h.update(dep.encode() + b'\0' + (root / dep).read_bytes() + b'\0')
    return h.hexdigest()


# ---------------------------------------------------------------- analysis (one header, one configuration)

def analyze(root: str, header: str, flags: list[str]) -> tuple[list[dict], list[str]]:
    """Units of one header under one configuration (namespace-scope declarations and struct members with extents,
    resolved references (file, line, column) and unresolved names), and the files outside root it includes."""
    ci = cindex()
    K = ci.CursorKind
    path = os.path.normpath(Path(root) / header)
    args = ['-x', 'c++', '-std=gnu++20', *resource_dir(), *flags]
    tu = ci.Index.create().parse(path, args=args)
    errors = [str(d) for d in tu.diagnostics if d.severity >= 3]
    if errors:
        raise GrabError(f'{header}: libclang cannot parse it: ' + '; '.join(errors[:3]))
    lib = {os.path.normpath(Path(root) / h) for h in library(Path(root)) if h not in EXCLUDED}
    units: list[dict] = []
    TYPES = {K.STRUCT_DECL, K.CLASS_DECL, K.UNION_DECL, K.CLASS_TEMPLATE, K.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION}
    SKIP = {K.TEMPLATE_TYPE_PARAMETER, K.TEMPLATE_NON_TYPE_PARAMETER, K.TEMPLATE_TEMPLATE_PARAMETER,
            K.CXX_BASE_SPECIFIER, K.TYPE_REF, K.TEMPLATE_REF, K.NAMESPACE_REF, K.CXX_FINAL_ATTR}
    SKIP |= {k for k in K.get_all_kinds() if k.is_attribute() or k.is_expression() or k.is_statement()}

    def ext(c):
        e = c.extent
        return [e.start.line, e.start.column, e.end.line, e.end.column]

    def brace(c):
        """Line of the opening brace of a namespace or struct body: the first brace after its name (a requires-clause
        before the name may hold braces)."""
        name = (c.location.line, c.location.column)
        for t in c.get_tokens():
            if t.spelling == '{' and (t.location.line, t.location.column) > name:
                return t.location.line
        return c.extent.start.line

    def last(c):
        """The last identifier of an unresolved (dependent) reference such as pc.inverseFactorial or T::f."""
        if c.kind not in (K.MEMBER_REF_EXPR, K.DECL_REF_EXPR):
            return ''
        name, after = '', True  # the identifier after the last '.', '->' or '::' (b.template cast<W> names cast)
        for t in c.get_tokens():
            if t.spelling in ('.', '->', '::'):
                after = True
            elif t.kind == ci.TokenKind.IDENTIFIER and after:
                name, after = t.spelling, False
        return name

    def references(c, unit, inner):
        """Collect references inside c into unit, skipping children that are units of their own (inner)."""
        for ch in c.get_children():
            if ch.hash in inner:
                continue
            kind = ch.kind
            if kind.is_reference() or kind.is_expression():
                r = ch.referenced
                f = os.path.normpath(r.location.file.name) if r is not None and r.location.file else None
                spelling = ch.spelling or last(ch)
                if kind == K.OVERLOADED_DECL_REF or (r is not None and r.kind == K.OVERLOADED_DECL_REF):
                    unit['names'].add(spelling)
                elif f is None:
                    if kind == K.MEMBER_REF_EXPR and spelling:
                        unit['members'].add(spelling)
                    elif kind in (K.DECL_REF_EXPR, K.CALL_EXPR) and spelling:
                        unit['names'].add(spelling)
                        unit['members'].add(spelling)
                elif f in lib:
                    unit['refs'].add((str(Path(f).relative_to(root)), r.location.line, r.location.column))
            references(ch, unit, inner)

    def add(c, parent, depth, qual):
        name = c.spelling
        member = parent is not None and units[parent]['kind'] == 'type'
        if c.kind == K.FRIEND_DECL:
            inner = next((x for x in c.get_children() if x.kind in (K.FUNCTION_DECL, K.FUNCTION_TEMPLATE)), None)
            name = inner.spelling if inner is not None else ''
        unit = {'name': name, 'qual': '::'.join([*qual, name]) if name else '', 'parent': parent, 'ext': ext(c),
                'kind': 'leaf', 'refs': set(), 'names': set(), 'members': set(), 'always': False, 'attach': None,
                'detail': any(q.endswith('_detail') for q in qual),
                'alias': c.kind in (K.TYPE_ALIAS_DECL, K.TYPE_ALIAS_TEMPLATE_DECL, K.TYPEDEF_DECL)}
        sem = c.semantic_parent
        if c.kind == K.NAMESPACE:
            unit['kind'] = 'ns'
        elif c.kind in TYPES and not member:
            unit['kind'] = 'type'
        if member:
            ctor = c.kind in (K.CONSTRUCTOR, K.DESTRUCTOR, K.CONVERSION_FUNCTION) or name.split('<')[0] == units[parent]['name'].split('<')[0]
            func = c.kind in (K.CXX_METHOD, K.FUNCTION_TEMPLATE, K.FRIEND_DECL)
            unit['always'] = ctor or not func or not name or name.startswith('operator') or name in PROTOCOL
        elif sem is not None and sem.kind in TYPES and c.kind not in TYPES:
            unit['attach'] = 'owner'  # out-of-line member definition: kept with its class
        elif name.startswith('operator') or 'std' in qual or (sem is not None and sem.spelling == 'std'):
            unit['attach'] = 'refs'  # free operator or std specialization: kept with a library type it names
        elif c.kind in (K.STATIC_ASSERT, K.USING_DIRECTIVE, K.USING_DECLARATION) or name.startswith('<deduction'):
            unit['attach'] = 'refs'
        elif sem is not None and sem.kind == K.NAMESPACE and c.lexical_parent is not None and sem != c.lexical_parent and c.kind not in TYPES:
            unit['attach'] = 'owner'  # out-of-line namespace member (`T ns::f(...) {...}`): kept with its declaration
        if c.kind == K.ENUM_DECL:
            unit['enum'] = True  # argument-dependent lookup looks into its namespace
        if sem is not None and c.lexical_parent is not None and sem != c.lexical_parent:
            scope, s = [], sem  # the namespaces it belongs to, not those it is written in
            while s is not None and s.kind != K.TRANSLATION_UNIT:
                if s.kind == K.NAMESPACE:
                    scope.append(s.spelling)
                s = s.semantic_parent
            unit['scope'] = scope[::-1]
            unit['detail'] = unit['detail'] or any(s.endswith('_detail') for s in scope)
        if unit['kind'] != 'leaf':
            unit['head'] = [c.extent.start.line, brace(c)]
        units.append(unit)
        me = len(units) - 1
        if unit['kind'] == 'leaf':
            references(c, unit, set())
            if unit['attach'] == 'owner':  # it needs its class and declaration; only the declaration brings it
                unit['owner'] = []
                here = lambda d: (d.location.file.name if d.location.file else None, d.location.line, d.location.column)
                own = c.canonical if here(c.canonical) != here(c) else sem if sem.kind in TYPES else None  # an explicit specialization is its own canonical declaration
                for d in ((sem, c.canonical) if sem.kind in TYPES else (c.canonical,)):
                    f = os.path.normpath(d.location.file.name) if d.location.file else None
                    if f in lib:
                        r = (str(Path(f).relative_to(root)), d.location.line, d.location.column)
                        unit['refs'].add(r)
                        if d == own:
                            unit['owner'].append(r)
            return
        children = [x for x in c.get_children() if x.location.file and x.location.file.name == path and x.kind not in SKIP]
        for x in children:
            add(x, me, depth + 1, [*qual, name])
        references(c, unit, {x.hash for x in children})

    for c in tu.cursor.get_children():
        if c.location.file and c.location.file.name == path and c.kind not in SKIP:
            add(c, None, 0, [])
    for u in units:
        for k in ('refs', 'names', 'members'):
            u[k] = sorted(u[k])
    inside = os.path.normpath(root) + os.sep
    system = sorted({os.path.normpath(i.include.name) for i in tu.get_includes() if i.include and not os.path.normpath(i.include.name).startswith(inside)})
    return units, system


def scan_lines(text: str) -> tuple[list[tuple[int, ...]], list[int]]:
    """Preprocessor groups (the line numbers of #if, #elif/#else and #endif) and system include lines."""
    groups, stack, includes = [], [], []
    for i, line in enumerate(text.split('\n'), 1):
        m = DIRECTIVE.match(line)
        if m:
            if m[1].startswith('if'):
                stack.append([i])
            elif stack:
                stack[-1].append(i)
                if m[1] == 'endif':
                    groups.append(tuple(stack.pop()))
        elif SYSTEM_INCLUDE.match(line):
            includes.append(i)
    return groups, includes


def merge(header: str, text: str, configs: list[list[dict]]) -> dict:
    """One record from the per-configuration units: a unit seen in several configurations is one unit."""
    units, ids = [], {}
    for found in configs:
        local = {}
        for i, u in enumerate(found):
            parent = local.get(u['parent'])
            k = (tuple(u['ext'][:2]), u['name'], u['kind'], parent)
            if k not in ids:
                ids[k] = len(units)
                units.append(dict(u, parent=parent, refs=[tuple(r) for r in u['refs']]))
            else:
                v = units[ids[k]]
                v['refs'] = sorted(set(v['refs']) | {tuple(r) for r in u['refs']})
                v['names'] = sorted(set(v['names']) | set(u['names']))
                v['members'] = sorted(set(v['members']) | set(u['members']))
            local[i] = ids[k]
    groups, includes = scan_lines(text)
    return {'header': header, 'units': units, 'groups': groups, 'includes': includes}


def index_header(root: Path, header: str) -> dict:
    """One header's cached record (both configurations), in JSON form, with the stamps of the system headers it reads."""
    text = (root / header).read_text(encoding='utf-8')
    runs = [analyze(str(root), header, flags) for flags in CONFIGS.values()]
    rec = merge(header, text, [units for units, _ in runs])
    rec['system'] = {f: stamp(f) for f in sorted(set().union(*(system for _, system in runs)))}
    return json.loads(json.dumps(rec))


class Index:
    """All library headers' records, loaded from the cache or built (in parallel) with libclang."""

    def __init__(self, root: Path = ROOT, cache: Path | None = CACHE, jobs: int | None = None, quiet: bool = False,
                 build: bool = True):
        self.root, self.graph = root, library(root)
        self.ci = cindex()
        self.records: dict[str, dict] = {}
        self.stale: list[str] = []
        self._text: dict[str, list[str]] = {}
        if self.ci is None:
            return
        todo, stamps = [], {}
        fresh = lambda rec: 'system' in rec and all((stamps[f] if f in stamps else stamps.setdefault(f, stamp(f))) == s for f, s in rec['system'].items())
        for h in self.graph:
            if h in EXCLUDED:
                continue
            k = key(root, self.graph, h)
            f = cache / (hashlib.sha256(h.encode()).hexdigest()[:24] + '.json') if cache else None
            try:
                data = json.loads(f.read_text()) if f else None
            except (OSError, ValueError):
                data = None
            if data and data.get('key') == k and fresh(data['record']):
                self.records[h] = data['record']
            else:
                todo.append((h, k, f))
        self.stale = [] if build else [t[0] for t in todo]
        if not build:
            todo = []
        if todo and not quiet:
            print(f'grab: indexing {len(todo)} header(s) with libclang', file=sys.stderr)
        workers = jobs or min(6, os.cpu_count() or 1)
        forked = len(todo) > 1 and workers > 1 and sys.modules.get(__name__) is not None and 'fork' in multiprocessing.get_all_start_methods()
        with concurrent.futures.ProcessPoolExecutor(workers, mp_context=multiprocessing.get_context('fork')) if forked else _Serial() as pool:
            for (h, k, f), rec in zip(todo, pool.map(index_header, [root] * len(todo), [t[0] for t in todo])):
                self.records[h] = rec
                if f:
                    f.parent.mkdir(exist_ok=True)
                    tmp = f.with_suffix(f'.{os.getpid()}.tmp')
                    tmp.write_text(json.dumps({'key': k, 'record': rec}))
                    os.replace(tmp, f)
        self.link()

    def link(self):
        """Global unit ids, name tables and the position lookup used to resolve references."""
        self.units, self.where = [], {}
        for h in self.graph:
            rec = self.records.get(h)
            if not rec:
                continue
            base = len(self.units)
            for u in rec['units']:
                self.units.append(dict(u, header=h, parent=None if u['parent'] is None else base + u['parent'], children=[]))
        for i, u in enumerate(self.units):
            if u['parent'] is not None:
                self.units[u['parent']]['children'].append(i)
            self.where.setdefault(u['header'], []).append(i)
        self.by_name: dict[str, list[int]] = {}
        for i, u in enumerate(self.units):
            if u['name']:
                self.by_name.setdefault(u['name'], []).append(i)
        self.closures = {h: set(closure(self.graph, h)) for h in self.graph}
        self.attached = [i for i, u in enumerate(self.units) if u['attach'] and u['kind'] != 'ns']
        self._lines: dict[str, dict[int, list[int]]] = {}
        self._targets: dict[int, list[int]] = {}
        self._aliases: dict[str, dict[str, str]] | None = None

    def covering(self, header: str) -> dict[int, list[int]]:
        """Line -> units of header whose extent covers it (built once per header)."""
        if header not in self._lines:
            table: dict[int, list[int]] = {}
            for i in self.where.get(header, ()):
                for l in range(self.units[i]['ext'][0], self.units[i]['ext'][2] + 1):
                    table.setdefault(l, []).append(i)
            self._lines[header] = table
        return self._lines[header]

    def at(self, header: str, line: int, col: int) -> int | None:
        """Innermost unit of header whose extent contains (line, col)."""
        best = None
        for i in self.covering(header).get(line, ()):
            sl, sc, el, ec = self.units[i]['ext']
            if (sl, sc) <= (line, col) <= (el, ec) and (best is None or self.units[i]['ext'] >= self.units[best]['ext']):
                best = i
        return best

    def targets(self, i: int) -> list[int]:
        """Units the resolved references of unit i point to."""
        if i not in self._targets:
            found = {self.at(h, l, c) for h, l, c in self.units[i]['refs'] if h in self.where}
            self._targets[i] = sorted(found - {None, i})
        return self._targets[i]

    def namespace_aliases(self, header: str) -> dict[str, str]:
        """Namespace alias visible in header (declared in its include closure) -> the innermost namespace it names, through
        further aliases (`namespace cd = convolution_detail;`)."""
        if self._aliases is None:
            pat = re.compile(r'\bnamespace\s+(\w+)\s*=\s*(?:::)?\s*((?:\w+\s*::\s*)*\w+)\s*;')
            self._aliases = {h: {m[1]: re.split(r'\s*::\s*', m[2])[-1] for m in pat.finditer(DEPS.strip_cpp('\n'.join(self.text(h))))}
                             for h in self.where}
        own = {}
        for h in [*self.closures.get(header, ()), header]:
            own.update(self._aliases.get(h, {}))
        out = {}
        for a in own:
            q, seen = a, set()
            while q in own and q not in seen:
                seen.add(q); q = own[q]
            out[a] = q
        return out

    def anchors(self, i: int) -> list[int]:
        """Units whose keeping keeps attached unit i: an out-of-line member's declaration (in its class or namespace), the
        library types a free operator, std specialization or using-declaration names."""
        if self.units[i]['attach'] == 'owner':
            return sorted({self.at(h, l, c) for h, l, c in self.units[i].get('owner', ()) if h in self.where} - {None, i})
        return [t for t in self.targets(i) if self.units[t]['kind'] == 'type']

    def code(self, i: int) -> list[int]:
        """Lines unit i keeps of its own code: a leaf's extent, a container's head and closing line."""
        u = self.units[i]
        return list(range(u['ext'][0], u['ext'][2] + 1)) if u['kind'] == 'leaf' else [*range(u['ext'][0], u['head'][1] + 1), u['ext'][2]]

    def text(self, header: str) -> list[str]:
        if header not in self._text:
            self._text[header] = (self.root / header).read_text(encoding='utf-8').split('\n')
        return self._text[header]

    def toplevel(self, i: int) -> bool:
        p = self.units[i]['parent']
        return p is None or self.units[p]['kind'] == 'ns'

    def public(self) -> dict[str, list[int]]:
        """Simple name -> namespace-scope units outside detail namespaces: what a file's identifiers may name."""
        return {n: [i for i in ids if self.toplevel(i) and not self.units[i]['detail'] and self.units[i]['kind'] != 'ns'
                    and not self.units[i]['attach']] for n, ids in self.by_name.items()}

    def aliased(self, i: int) -> list[int]:
        """The types an alias unit names, through further aliases (ModInt -> modint_detail::Value)."""
        out, seen, work = [], set(), [i]
        while work:
            j = work.pop()
            if j in seen:
                continue
            seen.add(j)
            if self.units[j]['kind'] == 'type':
                out.append(j)
            elif self.units[j].get('alias'):
                work += self.targets(j)
        return sorted(out)

    def resolve(self, name: str) -> list[int]:
        """Units a CLI name means: an exact qualified name, else a qualified suffix, namespace-scope units first;
        Alias::member names the member of the type the alias names, through further aliases."""
        parts = name.split('::')
        hits = [i for i in self.by_name.get(parts[-1], []) if self.units[i]['qual'].split('::')[-len(parts):] == parts
                and self.units[i]['attach'] != 'owner']  # an out-of-line definition is reached through its declaration
        exact = [i for i in hits if self.units[i]['qual'] == name]
        top = [i for i in hits if self.toplevel(i)]
        if hits or len(parts) < 2:
            return exact or top or hits
        types = {t for a in self.resolve('::'.join(parts[:-1])) for t in (self.aliased(a) if self.units[a].get('alias') else [a])
                 if self.units[t]['kind'] == 'type'}
        return sorted(c for t in types for c in self.units[t]['children'] if self.units[c]['name'] == parts[-1])


class _Serial:
    def __enter__(self):
        return self

    def __exit__(self, *a):
        return False

    def map(self, f, *its):
        return map(f, *its)


# ---------------------------------------------------------------- selection

# Members every standard container or string offers, and the monoid and action protocol of
# 02-Data Structures/00-monoids.hpp: a parameter used only through these takes any sequence or a problem-chosen monoid.
SEQUENCE = frozenset('size empty begin end rbegin rend cbegin cend data front back push_back pop_back emplace_back insert '
                     'erase clear reserve resize max_size find count at substr length assign swap first second operator[]'.split())
MONOID = frozenset('op e id inv mapping composition'.split())


def split_top(text: str) -> list[str]:
    """text split at commas outside brackets."""
    out, depth, cur = [], 0, ''
    for ch in text:
        depth += (ch in '<([{') - (ch in '>)]}')
        if ch == ',' and depth == 0:
            out.append(cur)
            cur = ''
        else:
            cur += ch
    return out + [cur] if cur.strip() else out


def head_end(code: str, start: int) -> int:
    """Index just past the `>` closing the template parameter list opened before start (`<<` is a shift, `>>` closes
    two, brackets inside parentheses do not count)."""
    depth, paren, k = 1, 0, start
    while k < len(code) and depth:
        if code[k] in '([{':
            paren += 1
        elif code[k] in ')]}':
            paren -= 1
        elif not paren and code.startswith('<<', k):
            k += 1
        elif not paren:
            depth += (code[k] == '<') - (code[k] == '>')
        k += 1
    return k


def declares(ix: Index, t: int, seen: set[int] | None = None) -> set[str]:
    """Member names type unit t declares, its base classes' included (found by name in its include closure)."""
    U, seen = ix.units, seen if seen is not None else set()
    seen.add(t)
    out = {U[c]['name'] for c in U[t]['children']}
    head = DEPS.strip_cpp('\n'.join(ix.text(U[t]['header'])[U[t]['ext'][0] - 1:U[t]['head'][1]]))
    m = re.search(r'\b(?:struct|class)\s+\w+[^:{;]*?:(?!:)([^{;]*)\{', head)
    for n in set(IDENT.findall(m[1])) - {'public', 'private', 'protected', 'virtual'} if m else ():
        for b in ix.by_name.get(n, ()):
            if b not in seen and U[b]['kind'] == 'type' and U[b]['header'] in ix.closures[U[t]['header']]:
                out |= declares(ix, b, seen)
    return out


def inputs(ix: Index, i: int, inferred_only: bool = False) -> list[int]:
    """Library types function template unit i takes as arguments, which a whole unit keeps whole (the caller builds
    them): the types its template parameter defaults name (`template<class G = Graph>`) and, for a type parameter without
    a default of a unit outside detail namespaces, the first namespace-scope type outside them declaring (base classes
    included) every member the unit uses on an object of that parameter (`g.arcs`, `g[u]`), searched in the unit's own
    header, then its includes, nearest first, unless a standard sequence or the monoid protocol has them all
    (`s.size()`, `m.op`). 03-cpp.md (Headers) asks for the default; the inference covers a parameter still without one
    (user decision 2026-11-06). inferred_only: the inferred types alone."""
    U = ix.units
    u = U[i]
    if u['kind'] != 'leaf':
        return []
    sl, sc, el, _ = u['ext']
    text = ix.text(u['header'])[sl - 1:el]
    first = text[0].encode()[sc - 1:].decode(errors='replace')  # libclang columns count bytes
    code = DEPS.strip_cpp('\n'.join([first, *text[1:]]))
    m = re.match(r'\s*template\s*<', code)
    if not m:
        return []
    end = head_end(code, m.end())
    defaults, needs = set(), []
    for p in split_top(code[m.end():end - 1]):
        q = re.fullmatch(r'\s*(?:class|typename)\s+(\w+)\s*(?:=\s*(.*\S))?\s*', p, re.S)
        if not q:
            continue
        if q[2]:
            defaults |= set(IDENT.findall(q[2]))
            continue
        used = set()
        for x in re.findall(rf'\b{q[1]}\b\s*&{{0,2}}\s*(\w+)\s*[,)=]', code[end:]):
            used |= set(re.findall(rf'\b{x}\s*(?:\.|->)\s*(?:template\s+)?(\w+)', code[end:]))
            if re.search(rf'\b{x}\s*\[', code[end:]):
                used.add('operator[]')
        if used - SEQUENCE - MONOID and not u['detail']:
            needs.append(used)
    out = [] if inferred_only else [t for t in ix.targets(i) if U[t]['name'] in defaults and (U[t]['kind'] == 'type' or U[t].get('alias'))]
    order, frontier = [u['header']], [u['header']]
    while frontier:
        frontier = [d for h in frontier for d in ix.graph.get(h, []) if d not in order and not order.append(d)]
    for used in needs:
        out += next(([t] for h in order for t in ix.where.get(h, ())
                     if U[t]['kind'] == 'type' and ix.toplevel(t) and not U[t]['detail'] and used <= declares(ix, t)), [])
    return list(dict.fromkeys(out))


def scopes(ix: Index, i: int) -> tuple[str, ...]:
    """The namespaces unit i belongs to, outermost first (an out-of-line definition's are its declaration's)."""
    if 'scope' in ix.units[i]:
        return tuple(ix.units[i]['scope'])
    out, p = [], ix.units[i]['parent']
    while p is not None:
        if ix.units[p]['kind'] == 'ns':
            out.append(ix.units[p]['name'])
        p = ix.units[p]['parent']
    return tuple(reversed(out))


def home(ix: Index, i: int) -> tuple:
    """Where unit i's name lives, for overload sets and specializations: its class (`in`), its namespaces (`ns`), or
    nowhere (`def`, an out-of-line class member's definition, which comes with its declaration)."""
    U = ix.units
    p = U[i]['parent']
    if p is not None and U[p]['kind'] == 'type':
        return ('in', p)
    if U[i]['attach'] == 'owner' and any(U[a]['kind'] == 'type' or (U[a]['parent'] is not None and U[U[a]['parent']]['kind'] == 'type')
                                         for a in ix.anchors(i)):
        return ('def', i)
    return ('ns', scopes(ix, i))


def findable(ix: Index, j: int, name: str, callers: set, kept: set[int]) -> bool:
    """Whether namespace-scope unit j can be what name means in one of the kept units spelling it (callers; None: an
    unparsed branch, any scope): written `ns::name` with ns enclosing j, or unqualified where ordinary lookup (j's
    namespace encloses the caller's, or a `using namespace` the caller spells names it) or argument-dependent lookup (a
    kept type lives in j's namespace; the argument may come from the instantiating code) reaches it. So `C::mul`
    through a template parameter is a member, never `x_detail::mul`, and an unqualified `key` in one detail namespace
    is never another's."""
    U, nj = ix.units, scopes(ix, j)
    for c in callers:
        if c is None:
            return True
        sl, _, el, _ = U[c]['ext']
        code = DEPS.strip_cpp('\n'.join(ix.text(U[c]['header'])[sl - 1:el]))
        real = lambda q, aliases=ix.namespace_aliases(U[c]['header']): aliases.get(q, q)
        spots = [m.start() for m in re.finditer(rf'\b{re.escape(name)}\b', code)]
        if not spots:
            return True
        for k, s in enumerate(spots):
            before = re.sub(r'\btemplate\s*$', '', code[:s].rstrip()).rstrip()  # ns::template f<3>(x)
            if before.endswith('.') or before.endswith('->'):
                continue
            if before.endswith('::'):  # qualified: the qualifier names j's namespace (`::name`: the global one)
                q = re.search(r'(\w*)\s*$', before[:-2])[1]
                q = '' if q in RESERVED else real(q)
                if q == (nj[-1] if nj else '') and not before[:-2].rstrip().endswith('>'):
                    return True
                continue
            if k == 0 and name == U[c]['name']:
                continue  # its own declarator
            if scopes(ix, c)[:len(nj)] == nj or nj[-1] in {real(m[1]) for m in re.finditer(r'\busing\s+namespace\s+(?:\w+\s*::\s*)*(\w+)\s*;', code)} or any(
                    (U[k2]['kind'] == 'type' or U[k2].get('enum')) and scopes(ix, k2) == nj for k2 in kept):  # ADL: a type's innermost namespace
                return True
    return False


def select(ix: Index, roots: list[int], member_names: set[str] | None = None, visible: set[str] = frozenset(),
           partial: list[int] = (), whole_roots: bool | None = None, exclude: set[str] = frozenset()) -> dict[str, set[int]]:
    """Header -> kept lines for roots and everything they reach. member_names (a file's identifiers) keeps the members
    they name of every struct kept, and prunes the roots like dependency structs (data, special members, operators and
    protocol names, plus the members something names) unless whole_roots; without it roots are kept whole. A whole
    function template keeps its input types whole (inputs). No global-namespace type or alias named in exclude (a
    file's own, which would be redefined) is kept; namespaced ones and functions of those names stay. partial units
    are kept like dependencies (an alias a member was named through, its type pruned). Dependent names are looked up in the include closures of every header
    with kept code and in visible (the headers a bundled source includes), as ADL at instantiation would."""
    U = ix.units
    whole, kept = set(), set()
    names: dict[str, set] = {}  # dependent name -> the kept units spelling it, None for an unparsed branch
    members = set(member_names or ())
    work = []

    def keep(i, entire=False):
        if i is None or (i in kept and (not entire or i in whole)) or (
                U[i]['qual'] in exclude and (U[i]['kind'] == 'type' or U[i].get('alias'))):  # a global-namespace type
            return
        kept.add(i)
        if entire:
            whole.add(i)
        work.append(i)

    def process(i):
        u = U[i]
        if u['parent'] is not None:
            keep(u['parent'])
        if u['kind'] != 'leaf':
            for c in u['children']:
                if i in whole or U[c]['always']:
                    keep(c, i in whole)
        for j in ix.by_name.get(u['name'], ()):
            if j != i and U[j]['kind'] == u['kind'] and home(ix, j) == home(ix, i) and (home(ix, i)[0] == 'in' or U[j]['header'] in ix.closures[u['header']] or u['header'] in ix.closures[U[j]['header']]):
                keep(j, i in whole)  # overload set or specialization
        for t in ix.targets(i):
            keep(t, i in whole and bool(u.get('alias')))  # a whole alias keeps the type it names whole
        table = ix.covering(u['header'])
        for l in (ix.code(i) if u['kind'] == 'leaf' or i not in whole else ()):
            for j in table.get(l, ()):  # a leaf sharing a line with kept code is kept too
                if U[j]['kind'] == 'leaf' or l in ix.code(j):
                    keep(j)
        if i in whole:
            for t in inputs(ix, i):
                keep(t, True)
        if u['kind'] == 'leaf' and u['parent'] is not None and U[u['parent']]['kind'] == 'type':
            text = DEPS.strip_cpp(' '.join(ix.text(u['header'])[u['ext'][0] - 1:u['ext'][2]]))
            m = re.match(r'\s*using\s+(?!namespace\b)([^=;]*::[^=;]*);', text)  # using B::add, B::prefix; (a dependent base)
            if m:
                members.update(q.rsplit('::', 1)[1].strip() for q in m[1].split(',') if '::' in q)
        for n in u['names']:
            names.setdefault(n, set()).add(i)
        members.update(u['members'])

    def lookup():
        seen = set(visible).union(*(ix.closures[h] for h in {U[k]['header'] for k in kept}))
        for n, callers in names.items():
            for j in ix.by_name.get(n, ()):
                if j in kept:
                    continue
                p = U[j]['parent']
                if p is not None and U[p]['kind'] == 'type':
                    if p in kept:
                        keep(j)
                elif U[j]['attach'] != 'owner' and U[j]['header'] in seen and findable(ix, j, n, callers, kept):
                    keep(j, True)
        for n in members:
            for j in ix.by_name.get(n, ()):
                p = U[j]['parent']
                if j not in kept and p is not None and p in kept and U[p]['kind'] == 'type':
                    keep(j)
        for j in ix.attached:
            if j not in kept and any(t in kept for t in ix.anchors(j)):  # not a parameter type: relax(GraphArc) is no Graph code
                keep(j, U[j]['kind'] == 'type')

    for r in roots:
        keep(r, member_names is None if whole_roots is None else whole_roots)
    for r in partial:
        keep(r)
    looked: set[tuple[str, str]] = set()
    while True:
        while work:
            while work:
                process(work.pop())
            lookup()
        unseen: dict[str, set[int]] = {}
        out = expand(ix, kept, whole, unseen)
        new = {(n, h) for h, ls in unseen.items() for l in ls for n in IDENT.findall(DEPS.strip_cpp(ix.text(h)[l - 1]))} - looked
        if not new:
            return out
        looked |= new  # identifiers of branches no configuration parsed are looked up like dependent names
        for n, h in new:
            names.setdefault(n, set()).add(None)
            members.add(n)
        lookup()
        if not work:
            return out


def own_lines(ix: Index, i: int, text: list[str]) -> tuple[int, int]:
    """First and last line of a unit, comment lines directly above included."""
    sl, _, el, _ = ix.units[i]['ext']
    while sl > 1 and re.match(r'\s*//', text[sl - 2]) and not DIRECTIVE.match(text[sl - 2]):
        sl -= 1
    return sl, el


def expand(ix: Index, kept: set[int], whole: set[int], unseen: dict[str, set[int]] | None = None) -> dict[str, set[int]]:
    """Header -> lines of the kept units, with their comments, system includes and conditional groups (the branch
    lines no configuration parsed are also recorded in unseen)."""
    out: dict[str, set[int]] = {}
    texts = {}
    for i in kept:
        u = ix.units[i]
        text = texts.setdefault(u['header'], ix.text(u['header']))
        lines = out.setdefault(u['header'], set())
        sl, el = own_lines(ix, i, text)
        if u['kind'] == 'leaf' or i in whole:
            lines.update(range(sl, el + 1))
        else:
            lines.update(range(sl, u['head'][1] + 1))
            lines.add(el)
    for h, lines in out.items():
        rec, text = ix.records[h], texts[h]
        if lines:
            lines.update(rec['includes'])
        covered = set()
        for i in ix.where[h]:
            u = ix.units[i]
            if u['kind'] == 'leaf':
                covered.update(range(u['ext'][0], u['ext'][2] + 1))
            else:
                covered.update(range(u['ext'][0], u['head'][1] + 1))
                covered.add(u['ext'][2])
        changed = True
        while changed:  # reproduce each conditional group around kept code, with branches no configuration parsed
            changed = False
            for g in rec['groups']:
                inside = range(g[0] + 1, g[-1])
                if any(l in lines for l in inside if l not in g) and not set(g) <= lines:
                    lines.update(g)
                    unseen_lines = [l for l in inside if l not in covered and text[l - 1].strip() and not re.match(r'\s*(//|#)', text[l - 1])]
                    lines.update(unseen_lines)
                    if unseen is not None:
                        unseen.setdefault(h, set()).update(unseen_lines)
                    changed = True
    return out


def render(ix: Index, lines: dict[str, set[int]], directives: bool = False) -> str:
    """Kept lines in typing order of their headers, one blank line where code was skipped across a blank line."""
    order = topo(ix, list(lines))
    out = []
    for h in order:
        text = ix.text(h)
        prev = None
        for l in sorted(lines[h]):
            if prev is None or l != prev + 1:
                if out and out[-1] != '' and (prev is None or any(not text[k - 1].strip() for k in range(prev + 1, l))):
                    out.append('')
                if directives:
                    out.append(f'#line {l} "{h}"')
            out.append(text[l - 1])
            prev = l
    return '\n'.join(out) + '\n' if out else ''


def topo(ix: Index, headers: list[str]) -> list[str]:
    seen, order = set(), []

    def visit(h):
        if h not in seen:
            seen.add(h)
            for d in ix.graph.get(h, []):
                visit(d)
            order.append(h)
    for h in sorted(headers):
        visit(h)
    want = set(headers)
    return [h for h in order if h in want]


# ---------------------------------------------------------------- whole-header fallback (no libclang)

def provides(root: Path, header: str, memo: dict[str, set[str]] = {}) -> set[str]:
    """Namespace-scope names a header declares, template parameters excluded: the fallback's lookup table."""
    path = str(root / header)
    if path not in memo:
        code = DEPS.strip_cpp((root / header).read_text(encoding='utf-8'))
        params = set(re.findall(r'\b(?:class|typename)\s+([A-Za-z_]\w*)\s*(?=[,>=]|\.\.\.)', code))
        memo[path] = declared(code) - params
    return memo[path]


def whole(root: Path, graph: dict[str, list[str]], headers: list[str], directives: bool = False,
          seen: set[str] | None = None) -> str:
    """The include closures of headers, each header whole minus #pragma once and quoted includes (headers in seen
    left out, and added to it)."""
    order, seen = [], set() if seen is None else seen
    for h in headers:
        for d in closure(graph, h):
            if d not in seen and d not in EXCLUDED:
                seen.add(d)
                order.append(d)
    out = []
    for h in order:
        text = (root / h).read_text(encoding='utf-8').split('\n')
        if directives:
            out.append(f'#line 1 "{h}"')
        out += ['' if re.match(r'\s*#\s*(pragma\s+once|include\s*")', l) else l for l in text]
    return '\n'.join(out).rstrip('\n') + '\n' if out else ''


# ---------------------------------------------------------------- front ends

def tokens(text: str, raw: bool = False) -> tuple[set[str], set[tuple[str, str]]]:
    """Identifiers of C++ text outside comments and literals (inside them too with raw), #define bodies included, and its
    qualified pairs (a::b)."""
    code = text if raw else DEPS.strip_cpp(text) + '\n' + '\n'.join(macros(text)[1])
    return set(IDENT.findall(code)), set(re.findall(r'\b([A-Za-z_]\w*)\s*::\s*([A-Za-z_]\w*)', code))


def macros(text: str) -> tuple[set[str], list[str]]:
    """Names text #defines, and their bodies without parameters and literals: a body naming a library declaration
    (`#define MI ModIntMini<998244353>`) uses it."""
    names, bodies = set(), []
    lines = DEPS.strip_cpp(text, True).replace('\\\n', ' ')
    for m in re.finditer(r'^[ \t]*#[ \t]*define[ \t]+([A-Za-z_]\w*)(?:\(([^)]*)\))?(.*)$', lines, re.M):
        names.add(m[1])
        body = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', ' ', m[3])
        bodies.append(re.sub(r'\b(?:%s)\b' % '|'.join(IDENT.findall(m[2] or '')), ' ', body) if m[2] and IDENT.findall(m[2]) else body)
    return names, bodies


def scope(code: str):
    """A test whether a position of code is at namespace scope: outside every brace and parenthesis but namespaces'."""
    marks, stack = [0], []  # positions where namespace scope starts or ends, alternating
    for m in re.finditer(r'[{}()]', code):
        if m[0] in '{(':
            stack.append(m[0] == '{' and bool(re.search(r'\bnamespace(?:\s+[\w:]+)?\s*$', code[max(0, m.start() - 200):m.start()])))
        elif stack:
            stack.pop()
        if (len(marks) % 2 == 1) != all(stack):
            marks.append(m.end())
    return lambda p: bisect.bisect_right(marks, p) % 2 == 1


def declared(code: str) -> set[str]:
    """Names a file declares at namespace scope (a variable, function or type after a type or keyword, not
    `typename X<..>::y`), which it does not take from the library: `mt19937_64 rng(1);` keeps a library rng out.
    Members, locals and parameters only shadow a library name, so they do not count."""
    at_scope = scope(code)
    out = {m[1] for m in re.finditer(r'\b(?:struct|class|union|enum|using|namespace|concept|typename)\s+([A-Za-z_]\w*)\b(?!\s*(?:<|::))', code)
           if at_scope(m.start())}
    first = {}  # start of a declaration statement -> whether its first declarator declares a name
    for m in re.finditer(r'\b([A-Za-z_]\w*)(?=\s*[;=({\[,)])', code):
        if m[1] in RESERVED or not at_scope(m.start()):
            continue
        before = code[max(0, m.start() - 200):m.start()].rstrip()
        word = re.search(r'\b([A-Za-z_]\w*)$', before)
        if (word and word[1] not in KEYWORDS | {'public', 'protected', 'private', 'virtual'}
                and not before[:word.start()].rstrip().endswith(('.', '->', '::'))) \
                or re.search(r'\b[A-Za-z_][\w:]*\s*<[\w\s,:<>]*>$', before):
            out.add(m[1])
        elif re.search(r',[\s*&]*$', before) and declarator(code, m.start(), out):
            out.add(m[1])  # a later declarator: int n, m; lng a = 1, b = 2;
    return out


def statement_start(code: str, pos: int) -> int:
    """Start of the namespace-scope statement holding pos: after the last `;`, the namespace brace that opens its scope or
    the closing brace of a function body before it (initializer, struct and lambda braces stay inside it)."""
    depth, closes = 0, []
    for k in range(pos - 1, -1, -1):
        c = code[k]
        if c in ')]':
            depth += 1
        elif c in '([':
            depth -= 1
        elif c == '}':
            closes.append(k)
            depth += 1
        elif c == '{':
            if depth == 0:
                return k + 1
            depth -= 1
            end = closes.pop()
            before = code[:k].rstrip()
            if depth == 0 and (before.endswith(')') or re.search(r'\b(?:const|noexcept|override|final|mutable)$', before)):
                return end + 1
        elif c == ';' and depth == 0:
            return k + 1
    return 0


def declarator(code: str, pos: int, out: set[str]) -> bool:
    """Whether the comma before pos separates declarators of one namespace-scope declaration whose first declarator
    declared a name in out: a comma outside brackets and outside the template brackets of the type."""
    stmt, depth, angle, assigned, commas = code[statement_start(code, pos):pos], 0, 0, False, []
    for k, ch in enumerate(stmt):
        depth += (ch in '([{') - (ch in ')]}')
        if depth == 0 and not assigned:
            angle += (ch == '<') - (ch == '>')
        if depth == 0 and ch == '=' and stmt[k - 1:k] not in ('<', '>', '!', '=') and stmt[k + 1:k + 2] != '=':
            assigned = True
        if depth == 0 and angle == 0 and ch == ',':
            commas.append(k)
            assigned = False
    name = re.match(r'[^=({\[]*?\b([A-Za-z_]\w*)\s*(?:$|[=({\[])', stmt[:commas[0]].strip()) if commas else None
    return bool(name) and commas[-1] == len(stmt.rstrip(' \t\n*&')) - 1 and name[1] in out


def local_names(code: str) -> set[str]:
    """Names code uses only as locals, parameters or members: each use outside member access (a.x, p->x, A::x) lies in
    the scope of one of its declarations inside a body, parameter list or struct (`auto dfs = [&](...)` in solve), so
    it names no library declaration."""
    match, inner, stack = {}, [], []
    for i, ch in enumerate(code):
        if ch in ')]}' and stack:
            match[stack.pop()] = i
        inner.append(stack[-1] if stack else -1)
        if ch in '([{':
            stack.append(i)

    def end(o):  # where a declaration directly inside the bracket opened at o goes out of scope
        c = match.get(o, len(code))
        if code[o] != '(':
            return c
        body = re.match(r'\s*(?:(?:const|noexcept|mutable|override)\b\s*|->[^{};]*?)*\{', code[c + 1:])
        if body:
            return match.get(c + body.end(), len(code))
        semi = code.find(';', c)  # for (int i ...) statement; or a prototype
        return len(code) if semi < 0 else semi

    scopes: dict[str, list[tuple[int, int]]] = {}
    for m in re.finditer(r'\b([A-Za-z_]\w*)(?=\s*(?:[;=({\[,)]|:(?!:)))', code):
        o = inner[m.start()]
        if o < 0 or m[1] in RESERVED or code[o] == '[':
            continue
        before = code[max(0, m.start() - 200):m.start()].rstrip()
        word = re.search(r'\b([A-Za-z_]\w*)$', before)
        stmt = code[max(code.rfind(';', 0, m.start()), code.rfind('{', 0, m.start()), code.rfind('}', 0, m.start()), code.rfind('(', 0, m.start()), code.rfind(',', 0, m.start())) + 1:m.start()]
        if ((word and word[1] not in KEYWORDS | {'public', 'protected', 'private', 'virtual', 'co_await'}
                and not before[:word.start()].rstrip().endswith(('.', '->', '::')))
                or re.search(r'\b[A-Za-z_][\w:]*\s*<[\w\s,:<>]*>$', before)
                or re.fullmatch(r'\s*(?:const\s+)?[A-Za-z_][\w:]*(?:\s*<[\w\s,:<>*&]*>)?\s*(?:const\s*)?[*&]+\s*', stmt)
                or code[o] == '{' and re.search(r',[\s*&]*$', before) and declarator(code, m.start(), set(scopes))):
            scopes.setdefault(m[1], []).append((m.start(), end(o)))
    uses: dict[str, list[int]] = {}
    for m in IDENT.finditer(code):
        if m[0] in scopes and not code[max(0, m.start() - 200):m.start()].rstrip().endswith(('.', '->', '::')):
            uses.setdefault(m[0], []).append(m.start())
    return {n for n, ss in scopes.items() if all(any(a <= q <= b for a, b in ss) for q in uses.get(n, ()))}


def roots_of(ix: Index, text: str, allowed: set[str] | None = None, raw: bool = False,
             own: set[str] | None = None) -> tuple[list[int], set[str]]:
    """Library units a file's identifiers name (namespace-scope, public or qualified through a detail namespace),
    except names it declares itself (own, else declared() of its code), #defines, or uses only as locals, parameters
    or members (local_names), or spells only in member access (`s.twice()` roots no free `twice`); with allowed, only
    units of those headers."""
    ids, pairs = tokens(text, raw)
    pub = ix.public()
    code = DEPS.strip_cpp(text)
    own = (declared(code) if own is None else own) | local_names(code) | macros(text)[0]
    access, scan = r'(?:\.|->)\s*(?:template\s+)?([A-Za-z_]\w*)', (text if raw else code) + '\n' + '\n'.join(macros(text)[1])
    accessed = set(re.findall(access, scan)) - set(IDENT.findall(re.sub(access, ' ', scan)))  # `-> mint` may be a trailing return type
    roots = {i for n in ids - own for i in pub.get(n, ()) if n not in accessed or ix.units[i]['kind'] == 'type' or ix.units[i].get('alias')}
    for a, b in pairs:
        roots |= {i for i in ix.by_name.get(b, ()) if ix.toplevel(i) and ix.units[i]['qual'].split('::')[-2:] == [a, b]}
    return sorted(i for i in roots if allowed is None or ix.units[i]['header'] in allowed), ids


def grab(names: list[str], root: Path = ROOT, ix: Index | None = None, directives: bool = False) -> str:
    """Declarations names reach (raises GrabError for an unknown name)."""
    ix = ix or Index(root)
    if ix.ci is None:
        hits = [h for h in ix.graph if h not in EXCLUDED and any(p in provides(root, h) for n in names for p in n.split('::'))]
        if not hits:
            raise GrabError('no header defines ' + ', '.join(names))
        return whole(root, ix.graph, hits, directives)
    roots, via = [], []
    for n in names:
        found = ix.resolve(n)
        if not found:
            near = suggest(ix, n)
            raise GrabError(f"unknown name {n!r} ({'did you mean ' + ', '.join(near) + '? ' if near else ''}try --find)")
        roots += found
        if '::' in n:  # Alias::member: the alias the code will spell comes too
            via += [a for a in ix.resolve(n.rsplit('::', 1)[0]) if ix.units[a].get('alias')]
    return render(ix, select(ix, roots, partial=via), directives)


def lines_of(ix: Index, text: str, allowed: set[str] | None = None, raw: bool = False,
             own: set[str] | None = None, prune: bool = True) -> tuple[dict[str, set[int]], list[str]]:
    """Kept lines per header for the library names text uses (roots_of), and those names; prune keeps of those names
    only the members text names, else they stay whole as grab keeps them. Either way every kept struct keeps the
    members text names (`dijkstra(g, s).path(g, t)` keeps `path`), and no type or alias text declares itself comes."""
    roots, ids = roots_of(ix, text, allowed, raw, own)
    mine = (declared(DEPS.strip_cpp(text)) if own is None else own) | macros(text)[0]
    return (select(ix, roots, ids, whole_roots=not prune, exclude=mine) if roots else {}), sorted({ix.units[i]['qual'] for i in roots})


def grab_text(text: str, root: Path = ROOT, ix: Index | None = None, directives: bool = False,
              allowed: set[str] | None = None, raw: bool = False, prune: bool = True) -> tuple[str, list[str]]:
    """Declarations the identifiers of text name (within allowed headers when given; pruned as lines_of says), and
    those names (whole headers without libclang)."""
    ix = ix or Index(root)
    if ix.ci is None:
        ids = tokens(text)[0] - declared(DEPS.strip_cpp(text))
        hits = [h for h in ix.graph if h not in EXCLUDED and (allowed is None or h in allowed) and provides(root, h) & ids]
        return whole(root, ix.graph, hits, directives), hits
    lines, names = lines_of(ix, text, allowed, raw, prune=prune)
    return (render(ix, lines, directives) if lines else ''), names


def placement(lines: list[str], used: set[str]) -> int:
    """Line to put the block above: the start of the first global-scope declaration (a whole user namespace counts as
    one) whose code spells a name in used, or that holds the `void solve(` line, whichever comes first (comment lines
    directly above go below the block)."""
    code = DEPS.strip_cpp('\n'.join(lines)).split('\n')
    stack, start, prev, at = [], None, '', None
    for i, l in enumerate(code):
        if not stack and l.strip() and (not prev or prev[-1] in ';}{'):
            start = i  # a global-scope declaration starts here
        if re.match(r'\s*void\s+solve\s*\(', l) and all(stack) or any(n in used for n in IDENT.findall(l)):
            at = start if start is not None else i
            break
        for m in re.finditer(r'[{}]', l):
            if m[0] == '{':
                stack.append(bool(re.search(r'\bnamespace(?:\s+[\w:]+)?\s*$', l[:m.start()])))
            elif stack:
                stack.pop()
        prev = l.strip() or prev
    at = len(lines) if at is None else at
    while at and re.match(r'\s*//', lines[at - 1]):
        at -= 1
    return at


def into(path: Path, root: Path = ROOT, ix: Index | None = None, prune: bool = False) -> list[str]:
    """Replace or insert the marked block above solve in path: what grab gives for the library names path uses, those
    names pruned to the members path names when prune is set (user decision 2026-11-06); returns the names."""
    lines = path.read_text(encoding='utf-8').split('\n')
    begin = next((i for i, l in enumerate(lines) if l.startswith(BEGIN)), None)
    end = next((i for i, l in enumerate(lines) if l.startswith(END)), None) if begin is not None else None
    if begin is not None and (end is None or end < begin):
        raise GrabError(f'{path}: "{BEGIN.strip()}" without a following "{END}"')
    rest = lines[:begin] + lines[end + 1:] if begin is not None else lines
    body, names = grab_text('\n'.join(rest), root, ix, prune=prune)
    if not any(re.match(r'\s*void\s+solve\s*\(', l) for l in rest):
        raise GrabError(f'{path}: no "void solve(" line to put the block above')
    at = placement(rest, {p for n in names for p in n.split('::')})
    if begin is not None:
        at = min(at, begin)
    block = [BEGIN + ' '.join(names), *body.rstrip('\n').split('\n'), END] if names else []
    out = rest[:at] + block + rest[at:]
    path.write_text('\n'.join(out), encoding='utf-8')
    return names


def bundle(source: Path, root: Path = ROOT, ix: Index | None = None) -> str:
    """source as one standalone file: each library include becomes the headers it reaches first, in typing order: the
    template (and debug) headers whole, then the declarations source names from them, with #line directives naming
    every origin. So directives between includes act on the same code as before; an include under #if is refused."""
    ix = ix or Index(root)
    lines = source.read_text(encoding='utf-8').split('\n')
    rel = os.path.relpath(source, root)
    found, depth = [], 0
    for i, line in enumerate(lines):
        d = DIRECTIVE.match(line)
        depth += bool(d and d[1].startswith('if')) - bool(d and d[1] == 'endif')
        m = re.match(r'\s*#\s*include\s*"([^"]+)"', line)
        if m and depth:
            raise GrabError(f'{rel}:{i + 1}: {m[1]} is included under #if (use --bundler oj-bundle)')
        if m:
            h = Path(os.path.normpath(source.parent / m[1]))
            h = h.relative_to(root).as_posix() if h.is_relative_to(root) else ''
            if h in ix.graph:
                found.append((i, [h]))
            elif re.search(r'^0[1-7]-[^/]+/9[89]-[^/]*\.hpp$', h) and (root / h).is_file():
                found.append((i, aggregate(root, ix.graph, h)))
            else:
                raise GrabError(f'{rel}:{i + 1}: {m[1]} is not a library header (use --bundler oj-bundle)')
    if not found:
        return '\n'.join(lines)
    reach = {d for _, hs in found for h in hs for d in closure(ix.graph, h)}
    skip = {i for i, _ in found}
    rest = '\n'.join('' if i in skip else l for i, l in enumerate(lines))
    kept = {} if ix.ci is None else select(ix, *roots_of(ix, rest, reach), reach)
    blocks, seen, done = {}, set(), set()
    for i, hs in found:
        new = [d for h in hs for d in closure(ix.graph, h) if d not in done and not done.add(d)]
        block = []
        for h in EXCLUDED:
            if h in new:
                block.append(f'#line 1 "{h}"')
                block += ['' if re.match(r'\s*#\s*(pragma\s+once|include\s*")', l) else l for l in ix.text(h)]
        body = whole(root, ix.graph, new, True, seen) if ix.ci is None else render(ix, {h: kept[h] for h in new if h in kept}, True)
        blocks[i] = [*block, *body.rstrip('\n').split('\n'), f'#line {i + 2} "{rel}"'] if block or body.strip() else ['']
    out = []
    for i, l in enumerate(lines):
        out += blocks[i] if i in blocks else [l]
    return '\n'.join(out)


def aggregate(root: Path, graph: dict[str, list[str]], rel: str) -> list[str]:
    """Library headers an aggregate (`98-basic.hpp`, `99-all.hpp`) includes, through nested aggregates."""
    out = []
    for inc, _ in DEPS.includes((root / rel).read_text(encoding='utf-8')):
        h = os.path.normpath(os.path.join(os.path.dirname(rel), inc))
        if h in graph:
            out.append(h)
        elif re.search(r'(^|/)9[89]-[^/]*\.hpp$', h) and (root / h).is_file():
            out += aggregate(root, graph, h)
    return list(dict.fromkeys(out))


def suggest(ix: Index, name: str) -> list[str]:
    """Up to five names grab accepts closest to an unknown one (difflib ratio at least 0.6, case ignored), closest first:
    top-level names for a bare name, qualified names (members included) for one with ::; detail names never."""
    key, spelt = 'qual' if '::' in name else 'name', {}
    for i, u in enumerate(ix.units):
        if u['name'] and u['kind'] != 'ns' and not u['detail'] and u['attach'] != 'owner' and (key == 'qual' or ix.toplevel(i)):
            spelt.setdefault(u[key].lower(), set()).add(u[key])
    return [s for m in difflib.get_close_matches(name.lower(), spelt, n=5, cutoff=0.6) for s in sorted(spelt[m])][:5]


def find(ix: Index, text: str) -> list[str]:
    t = text.lower()
    out = []
    for i, u in enumerate(ix.units):
        if u['name'] and t in u['qual'].lower() and u['kind'] != 'ns' and u['attach'] != 'owner':
            out.append(f"{u['qual']:40} {u['header']}:{u['ext'][0]}")
    return sorted(set(out))


def copy(text: str) -> str | None:
    """Copy text to the clipboard; the tool used, or None when there is none."""
    for cmd in (['wl-copy'], ['xclip', '-selection', 'clipboard'], ['xsel', '--clipboard', '--input'], ['pbcopy']):
        if shutil.which(cmd[0]):
            try:
                if subprocess.run(cmd, input=text, text=True, timeout=10).returncode == 0:
                    return cmd[0]
            except (OSError, subprocess.TimeoutExpired):
                pass
    return None


def main(argv: list[str] | None = None) -> int:
    p = argparse.ArgumentParser(description=__doc__.splitlines()[0], formatter_class=argparse.RawDescriptionHelpFormatter,
                                epilog='\n'.join(__doc__.splitlines()[2:6]))
    p.add_argument('names', nargs='*', help='library names (Type::member for one member)')
    p.add_argument('--into', metavar='FILE', type=Path, help='keep the marked block above solve in FILE current')
    p.add_argument('--min', action='store_true', help='with --into, keep only the members FILE calls')
    p.add_argument('--find', metavar='TEXT', help='search the index')
    p.add_argument('--no-copy', action='store_true', help='print only')
    p.add_argument('--lines', action='store_true', help='emit #line directives naming the source headers')
    a = p.parse_args(argv)
    if sum(map(bool, (a.names, a.into, a.find))) != 1:
        p.error('give names, --into FILE or --find TEXT')
    if a.min and not a.into:
        p.error('--min needs --into')
    try:
        ix = Index()
        if ix.ci is None:
            print('SKIP libclang: python3 cannot import clang.cindex; inlining whole headers (/setup-check prints the install command)', file=sys.stderr)
        if a.find is not None:
            if ix.ci is None:
                raise GrabError('--find needs libclang')
            print('\n'.join(find(ix, a.find)) or 'no match')
            return 0
        if a.into:
            names = into(a.into, ROOT, ix, a.min)
            print(f'{a.into}: block holds ' + (', '.join(names) if names else 'nothing (no library names used)'), file=sys.stderr)
            return 0
        out = grab(a.names, ROOT, ix, a.lines)
    except (GrabError, OSError) as e:
        print(f'grab: {e}', file=sys.stderr)
        return 1
    sys.stdout.write(out)
    if not a.no_copy:
        tool = copy(out)
        print(f'grab: {len(out.splitlines())} lines, ' + (f'copied ({tool})' if tool else 'not copied (no wl-copy, xclip, xsel or pbcopy)'), file=sys.stderr)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
