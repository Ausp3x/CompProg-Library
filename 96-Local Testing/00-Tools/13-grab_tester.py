#!/usr/bin/env python3
"""Checks for 09-Contest Testing/10-grab.py (the symbol extractor) and the pure functions of 96-Local Testing/05-extraction.py.

Fixture libraries are written to per-process temporary roots: a light template header (cstdint, cstdio, cstdlib,
functional, `using lng`), a debug header, `02-Data Structures/01-alpha.hpp` (units of every kind, a detail namespace,
an `#ifdef __AVX2__` pair, a pruned struct with an unparsed `#ifdef` branch, a class template with a constructor
template, friends and a dependent member call), `05-Mathematics/01-beta.hpp` (includes alpha and the debug header)
and `05-Mathematics/02-unrelated.hpp` (a same-named `gammaF`, `boom` with a static_assert). Except in the Cache tests,
fixture indexes are built with Index(build=False) plus 10-grab.index_header per stale header, so a defect in Index's own
build step fails the Cache tests alone. Compiles and runs go through _00_memory_cap.capped.
quick: every fixture test (units, selection table compiled and run on $CXX, rendering, resolve, CLI, token mode,
--into, bundle, fallback, cache, harness functions), bellTable from the repository on $CXX, and 1 random library.
full: quick on $CXX and g++-14 (deduplicated, SKIP when missing), plus the repository DSU and isPrime oracles, the
real `--into` CLI on a solution file, and 3 random libraries. stress: full with --rounds random libraries (default 8).
Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED.
Oracles (independent of grab): hand-written kept and dropped source lines per root and expected program output of each
extraction compiled after the template; the fixture headers' own text for every `#line` run and a static_assert
diagnostic's file:line; hand-listed units, groups and includes; a Bell triangle in Python (validated on its first
terms), a relabelling union-find and trial division plus a Miller-Rabin cross-checked against a sieve; for random
libraries a generator-side model (reachability, values mod 1000003, which names must be absent); hand-built inventory,
plan and tester trees for the harness. Failures print seed, mode, fixture root, root names and compiler output.

Feature map:
units and merge (namespace-scope functions, overloads, detail namespace, concept, alias, variable, class template and
specialization, struct members with `always`, out-of-line definition attach owner, free operator and std::hash attach
refs, both `#ifdef __AVX2__` definitions, a branch no configuration parsed absent, one Point unit across configurations,
groups, system includes) -> test_units_alpha;
selection (root whole; references to functions, types, aliases in template defaults, concepts in requires-clauses;
pruned dependency structs: fields, constructors incl. constructor templates, destructor, conversion, operators incl.
friends, static data, typedefs, nested types, protocol names; other methods and friends dropped; member reached by a
resolved call; member called on a dependent object, also as a member template (s.template as<M>()); a struct whose
requires-clause holds braces before its name; ADL friend with dependent arguments; overload sets and
specializations together; free operator and std::hash kept with Point and dropped without; out-of-line member with its
declaration; dependent names only within the include closures of headers with kept code, also across them
(argument-dependent, test_dependent_lookup_across_kept_headers); two declarations on one line; comment lines above;
system includes kept, quoted includes and pragma once not; template and debug headers absent; the unparsed
`#ifdef SOME_UNDEFINED_MACRO` branch kept and compiled with the macro; the AVX2 group checked with -mavx2) ->
test_selection_table, test_unparsed_branch_compiles, test_std_hash_with_type, test_out_of_line_member, test_avx2_group;
rendering (typing order across headers, one contiguous run per header, blank line only where a blank was skipped,
--lines runs equal the header lines, a static_assert diagnostic at its header line) -> test_render_order,
test_render_lines, test_lines_diagnostic;
resolve (exact qualified, qualified suffix, top-level first, Type::member, unknown; Alias::member keeps the alias,
test_alias_member_and_whole) -> test_resolve, test_grab_unknown;
suggest (did you mean: case ignored, closest first, at most five, resolvable, top-level for bare names, members for
qualified ones, no detail or far names) -> test_grab_suggest, test_grab_unknown;
CLI (names to stdout, --lines, unknown name exit 1, --find on a library without inventories, --into, mutually
exclusive arguments, --no-copy, a fake wl-copy receiving the text, no clipboard tool) -> test_cli_*;
token mode (declared, every comma declarator, no type at a line end, roots_of with and without allowed, qualified
detail pairs, names a file declares at namespace scope but not members or typename X<..>::y, #define names excluded and
#define bodies counted, names used only as locals, parameters or members excluded, structs pruned by the file's
identifiers) -> test_declared, test_roots_of, test_roots_of_macros_and_locals, test_grab_text;
--into (roots whole as grab keeps them, --min pruned to the members the file names, insert above solve, compile and
run, rerun identical, stale block replaced, block removed, no solve, begin
without end, end before begin, above a helper, never inside a user namespace) -> test_into_*;
bundle (template and debug whole with #line, extraction emitted at the include that reaches it, a #define between
includes acting on the same code, an include under #if refused, source numbering restored and checked by a compile
error's line, an include adding nothing blanked, non-library include, no includes unchanged, template-only include,
aggregates) -> test_bundle_*;
fallback without libclang (whole closures, unknown name, grab_text, bundle, SKIP message, --find without the index) ->
test_fallback_*;
cache (first build writes one file per header and builds each once, reuse builds nothing, an edit rebuilds exactly the
headers whose closure holds it, a corrupt file rebuilds that header, build=False reports stale and builds nothing,
serial jobs from an unregistered module and forked workers once registered in sys.modules, a key changed by every
analysis constant and helper, system-header stamps stored and checked) -> test_cache_*;
input types (a template default, inference from the members a template uses: own header first, base members counted,
a type lacking one skipped; sequences, monoids, detail templates and non-templates none; kept whole by a whole template
and pruned by a pruned one; undefaulted in the harness) -> test_input_types; file mode keeping members called on structs
the file never names and no type the file declares, however reached (--into and --min; on the repository, dijkstra(...).path)
-> test_into_called_members_and_own_types, test_repo_into_cli; a using-declaration of a dependent base's members keeping
them -> test_using_declaration_members; no over-reach (an out-of-line member only with its class, `C::mul` through a
template parameter and an unqualified call in one detail namespace keeping no other namespace's function, while
argument-dependent lookup and a using-directive still reach one) -> test_no_over_reach;
lookup rules (enum ADL, out-of-line namespace members by their declaration and namespace, namespace aliases, `::name`,
namespace-scope and block-scope using-declarations, out-of-line class members only with their in-class declaration,
nested namespaces, `ns::template f`, explicit specializations in and out of their namespace and of class members,
aliases per include closure and in chains, ADL in a type's innermost namespace only, no out-of-line definition as a
command-line name, names in member access or trailing return types) and the harness's out-of-closure check -> test_lookup_rules;
harness: split (template brackets), operations (operators, operator()), rows, targets (bases, nested structs and enums,
operators; test_targets_nested_enum_bases_operators), core_full (fixture and repository), final, full_only, cpp_testers,
scratch (per-header stubs under their quoted includes, copies, compiled use), own_names (prose excluded), aggregate
closures, mini-first parsing (scope-aware full_type_names, wrapped full_defaults, mini_first_split strict and pending)
-> test_harness_*; probes (candidate arguments, control rounds, presets, a dropped member caught, check_ops end to end on
a fixture inventory) -> test_ops_instantiation_oracle;
repository oracles: bellTable (n <= 60), DSU on random operations, isPrime, `--into` through the real CLI ->
test_repo_*;
random libraries (functions, overloads, structs, class templates, template functions with dependent member and
dependent free calls, same-named decoys outside the closure, kept when named on the command line or when a header
holding kept code includes them (instantiation-time lookup); every reachable declaration kept, every unreachable
function and method absent, values equal to the model, --lines runs equal the headers) -> test_random_libraries.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import contextlib
import importlib.util
import io
import json
import os
import random
import re
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
GRAB_PY = ROOT / '09-Contest Testing/10-grab.py'
HARNESS_PY = ROOT / '96-Local Testing/05-extraction.py'
CXX = os.environ.get('CXX', 'g++')
MODE, SEED, ROUNDS = 'full', 42, 8
FULL = {'test_repo_dsu', 'test_repo_is_prime', 'test_repo_into_cli'}
COMPILE_MB, RUN_MB = 2048, 1024
A, B, U = '02-Data Structures/01-alpha.hpp', '05-Mathematics/01-beta.hpp', '05-Mathematics/02-unrelated.hpp'
T, D = '01-Core/01-template.hpp', '01-Core/02-debug.hpp'


def load(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


G = load(GRAB_PY, 'grab_under_test')  # not in sys.modules: test_cache_parallel_jobs registers it to fork workers
H = load(HARNESS_PY, 'extraction_under_test')
HAVE_CLANG = G.cindex() is not None

TEMPLATE = ('#pragma once\n#include <cstdint>\n#include <cstdio>\n#include <cstdlib>\n#include <functional>\n'
            'using namespace std;\nusing lng = int64_t;\n')
DEBUG = '#pragma once\n#include "01-template.hpp"\n#define trace(...) 0\n'
ALPHA = r'''#pragma once
#include <cassert>
#include "../01-Core/01-template.hpp"

// helperA adds.
// second comment line
inline int helperA(int x) { return x + 1; }
inline int helperA(int x, int y) { return x + y; }
inline int unusedA(int x) { return x * 7; }
inline int gammaF(int x) { return x * 3; }
inline int helperB(int x) { return x - 4; }

namespace alpha_detail {
    inline int twice(int x) { return 2 * x; }
    inline int thrice(int x) { return 3 * x; }
} // namespace alpha_detail

template<typename T>
concept Small = sizeof(T) <= 8;

using Cell = lng;

constexpr int ALPHA_K = 5;

template<typename T>
struct Trait { static constexpr int v = 1; };
template<>
struct Trait<int> { static constexpr int v = 2; };

struct Point {
    int x, y;
#ifdef SOME_UNDEFINED_MACRO
    int debugField = 7;
#else
    int plainField = 0;
#endif
    Point(int a = 0, int b = 0) : x(a), y(b) {}
    ~Point() {}
    explicit operator bool() const { return x || y; }
    bool operator<(const Point &o) const { return x < o.x; }
    friend Point operator+(Point a, Point b) { return Point(a.x + b.x, a.y + b.y); }
    friend int dotP(Point a, Point b) { return a.x * b.x + a.y * b.y; }
    static inline int counter = 0;
    using value_type = int;
    struct Inner { int z; };
    int size() const { return 2; }
    int norm() const { return x * x + y * y; }
    int twiceNorm() const { return 2 * norm(); }
    int manhattan() const;
    static bool isOdd(int v) { return v % 2 != 0; }
};
inline int Point::manhattan() const { return abs(x) + abs(y); }
inline bool operator==(Point a, Point b) { return a.x == b.x && a.y == b.y; }
template<>
struct std::hash<Point> { size_t operator()(const Point &p) const { return size_t(p.x * 31 + p.y); } };

inline bool isOdd(int v) { return v & 1; }
inline int useNorm(Point p) { return p.norm(); }

template<Small T = Cell>
T useAlias(T x) { return T(alpha_detail::twice(int(x)) + ALPHA_K); }

template<typename T> requires Small<T>
T useConcept(T x) { return x + T(Trait<T>::v); }

inline int a1 = 1; inline int a2 = helperB(3);

#ifdef __AVX2__
inline int simdSum(int a, int b) { return a + b + 0; }
#else
inline int simdSum(int a, int b) { return b + a; }
#endif

template<class M>
struct Box {
    M v;
    Box() : v() {}
    template<class U>
    Box(U u) : v(M(u)) {}
    M get() const { return v; }
    M foo() const { return v + 1; }
    M bar() const { return v + 2; }
    friend M peek(const Box &b) { return b.v * 10; }
    friend M poke(const Box &b) { return b.v * 20; }
    template<class U> U as() const { return U(v * 3); }
};
template<class M>
M callFoo(M x) { Box<M> s(x); return s.foo(); }
template<class M>
M callAs(M x) { Box<M> s(x); return s.template as<M>() + 1; }
template<class T>
requires requires(T x) { x + x; }
struct Req {
    T v;
    int used() const { return int(v) + 1; }
    int unused() const { return 2; }
};
inline int useReq() { Req<int> r{4}; return r.used(); }
template<class M>
M callPeek(M x) { Box<M> s(x); return peek(s); }

namespace outer::inner {
    inline int deep(int x) { return x + 100; }
}
'''
BETA = r'''#pragma once
#include <numeric>
#include "../02-Data Structures/01-alpha.hpp"
#include "../01-Core/02-debug.hpp"

inline int betaUse(int x) { return helperA(x) * 2; }
template<class T>
T betaDep(T x) { return gammaF(x) + 1; }
inline int betaPoint(int a) { Point p(a, a); return p.manhattan(); }
'''
UNRELATED = r'''#pragma once
#include "../01-Core/01-template.hpp"

inline int gammaF(lng x) { return int(x) * 1000; }
inline int lonely(int x) { return x; }
template<class T>
int boom(T x) {
    static_assert(sizeof(T) == 0, "boom fired");
    return int(x);
}
'''
FILES = {T: TEMPLATE, D: DEBUG, A: ALPHA, B: BETA, U: UNRELATED}
GLOBAL_DROPPED = ['#pragma once', 'using lng = int64_t;', '#define trace(...) 0', '#include <cstdint>']
DECL, DEF = 'int manhattan() const;', 'inline int Point::manhattan() const { return abs(x) + abs(y); }'
EQ = 'inline bool operator==(Point a, Point b) { return a.x == b.x && a.y == b.y; }'
HASH = 'struct std::hash<Point> { size_t operator()(const Point &p) const { return size_t(p.x * 31 + p.y); } };'
PRUNED_POINT = ['int x, y;', '#ifdef SOME_UNDEFINED_MACRO', 'int debugField = 7;', '#else', 'int plainField = 0;', '#endif',
                'Point(int a = 0, int b = 0) : x(a), y(b) {}', '~Point() {}', 'explicit operator bool() const { return x || y; }',
                'bool operator<(const Point &o) const { return x < o.x; }',
                'friend Point operator+(Point a, Point b) { return Point(a.x + b.x, a.y + b.y); }',
                'static inline int counter = 0;', 'using value_type = int;', 'struct Inner { int z; };',
                'int size() const { return 2; }', EQ, HASH]
POINT_METHODS = {'norm': 'int norm() const { return x * x + y * y; }', 'twiceNorm': 'int twiceNorm() const { return 2 * norm(); }',
                 'isOdd': 'static bool isOdd(int v) { return v % 2 != 0; }',
                 'dotP': 'friend int dotP(Point a, Point b) { return a.x * b.x + a.y * b.y; }'}
HELPER_A = ['// helperA adds.', '// second comment line', 'inline int helperA(int x) { return x + 1; }',
            'inline int helperA(int x, int y) { return x + y; }']


def point(*methods):
    """Kept and dropped lines of a pruned Point keeping methods."""
    return PRUNED_POINT + [POINT_METHODS[m] for m in methods], [POINT_METHODS[m] for m in POINT_METHODS if m not in methods]


# root -> kept lines (stripped), dropped lines, main body, expected output
_PK, _PD = point('norm')
SELECTION = {
    'helperA': (['#include <cassert>', *HELPER_A], ['inline int unusedA(int x) { return x * 7; }', 'struct Point {', EQ, HASH],
                'printf("%d %d\\n", helperA(1), helperA(2, 3));', '2 5\n'),
    'useAlias': (['namespace alpha_detail {', 'inline int twice(int x) { return 2 * x; }', '} // namespace alpha_detail',
                  'concept Small = sizeof(T) <= 8;', 'using Cell = lng;', 'constexpr int ALPHA_K = 5;'],
                 ['inline int thrice(int x) { return 3 * x; }', 'struct Trait { static constexpr int v = 1; };', *HELPER_A[2:]],
                 'printf("%lld %d\\n", (long long)useAlias(lng(4)), useAlias(3));', '13 11\n'),
    'useConcept': (['concept Small = sizeof(T) <= 8;', 'struct Trait { static constexpr int v = 1; };',
                    'struct Trait<int> { static constexpr int v = 2; };'], ['using Cell = lng;'],
                   'printf("%d %lld\\n", useConcept(5), (long long)useConcept(lng(5)));', '7 6\n'),
    'callFoo': (['M v;', 'Box() : v() {}', 'template<class U>', 'Box(U u) : v(M(u)) {}', 'M get() const { return v; }',
                 'M foo() const { return v + 1; }'],
                ['M bar() const { return v + 2; }', 'friend M peek(const Box &b) { return b.v * 10; }',
                 'friend M poke(const Box &b) { return b.v * 20; }'], 'printf("%d\\n", callFoo(4));', '5\n'),
    'useReq': (['requires requires(T x) { x + x; }', 'struct Req {', 'int used() const { return int(v) + 1; }'],
               ['int unused() const { return 2; }'], 'printf("%d\\n", useReq());', '5\n'),
    'callAs': (['template<class U> U as() const { return U(v * 3); }'], ['M foo() const { return v + 1; }'],
               'printf("%d\\n", callAs(4));', '13\n'),
    'callPeek': (['friend M peek(const Box &b) { return b.v * 10; }', 'Box(U u) : v(M(u)) {}'],
                 ['M foo() const { return v + 1; }', 'M bar() const { return v + 2; }', 'friend M poke(const Box &b) { return b.v * 20; }'],
                 'printf("%d\\n", callPeek(4));', '40\n'),
    'useNorm': (_PK + ['inline int useNorm(Point p) { return p.norm(); }'], _PD + ['inline bool isOdd(int v) { return v & 1; }'],
                'Point p(3, 4);\nprintf("%d %d\\n", useNorm(p), int(p == Point(3, 4)));\n#ifdef SOME_UNDEFINED_MACRO\nprintf("%d\\n", p.debugField);\n#endif',
                '25 1\n'),
    'Point::norm': (*point('norm'), 'printf("%d\\n", Point(1, 2).norm());', '5\n'),
    'Point::twiceNorm': (*point('norm', 'twiceNorm'), 'printf("%d\\n", Point(1, 2).twiceNorm());', '10\n'),
    'Point::isOdd': (point('isOdd')[0], point('isOdd')[1] + ['inline bool isOdd(int v) { return v & 1; }'],
                     'printf("%d\\n", int(Point::isOdd(3)));', '1\n'),
    'isOdd': (['inline bool isOdd(int v) { return v & 1; }'], ['struct Point {', POINT_METHODS['isOdd'], EQ, HASH],
              'printf("%d\\n", int(isOdd(4)));', '0\n'),
    'Point': (PRUNED_POINT + list(POINT_METHODS.values()) + [DECL, DEF], ['inline int useNorm(Point p) { return p.norm(); }'],
              'Point a(1, 2), b(3, 4);\nprintf("%d %d %d %d\\n", dotP(a, b), (a + b).manhattan(), int(Point::isOdd(5)), a.twiceNorm());',
              '11 10 1 10\n'),
    'a1': (['inline int helperB(int x) { return x - 4; }', 'inline int a1 = 1; inline int a2 = helperB(3);'], [],
           'printf("%d %d\\n", a1, a2);', '1 -1\n'),
    'simdSum': (['#ifdef __AVX2__', 'inline int simdSum(int a, int b) { return a + b + 0; }', '#else',
                 'inline int simdSum(int a, int b) { return b + a; }', '#endif'], [], 'printf("%d\\n", simdSum(2, 3));', '5\n'),
    'betaUse': (['#include <cassert>', '#include <numeric>', *HELPER_A, 'inline int betaUse(int x) { return helperA(x) * 2; }'],
                ['inline int gammaF(int x) { return x * 3; }'], 'printf("%d\\n", betaUse(3));', '8\n'),
    'betaDep': (['inline int gammaF(int x) { return x * 3; }', 'T betaDep(T x) { return gammaF(x) + 1; }'],
                ['inline int gammaF(lng x) { return int(x) * 1000; }'], 'printf("%d\\n", betaDep(2));', '7\n'),
    'betaPoint': (PRUNED_POINT + [DECL, DEF, 'inline int betaPoint(int a) { Point p(a, a); return p.manhattan(); }'],
                  list(POINT_METHODS.values()), 'printf("%d\\n", betaPoint(-3));', '6\n'),
    'outer::inner::deep': (['namespace outer::inner {', 'inline int deep(int x) { return x + 100; }', '}'], [],
                           'printf("%d\\n", outer::inner::deep(1));', '101\n'),
    'lonely': (['inline int lonely(int x) { return x; }'], ['#include <cassert>', 'int boom(T x) {', *HELPER_A[2:]],
               'printf("%d\\n", lonely(9));', '9\n'),
    'Trait': (['struct Trait { static constexpr int v = 1; };', 'struct Trait<int> { static constexpr int v = 2; };'], [],
              'printf("%d %d\\n", Trait<int>::v, Trait<char>::v);', '2 1\n'),
    'Box': (['M bar() const { return v + 2; }', 'friend M poke(const Box &b) { return b.v * 20; }',
             'friend M peek(const Box &b) { return b.v * 10; }'], ['M callFoo(M x) { Box<M> s(x); return s.foo(); }'],
            'Box<int> b(3);\nprintf("%d %d %d %d\\n", b.bar(), poke(b), peek(b), b.get());', '5 60 30 3\n'),
}


def plain(x):
    """x as JSON reads it back (tuples become lists)."""
    return json.loads(json.dumps(x))


def write_tree(root: Path, files: dict[str, str]) -> Path:
    for rel, text in files.items():
        p = root / rel
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text, encoding='utf-8')
    return root


def built(root: Path, cache: Path | None = None):
    """Index of root whose stale records are built here (two threads), not by Index's own build step."""
    ix = G.Index(root, cache, quiet=True, build=False)
    with concurrent.futures.ThreadPoolExecutor(2) as pool:
        for h, rec in zip(ix.stale, pool.map(lambda h: G.index_header(root, h), ix.stale)):
            ix.records[h] = rec
    ix.link()
    return ix


def compilers():
    want = [CXX] if MODE == 'quick' else list(dict.fromkeys([CXX, 'g++-14']))
    return [c for c in want if shutil.which(c)], [c for c in want if not shutil.which(c)]


def prelude(root: Path) -> str:
    return f'#include "{root}/{T}"\n'


def compile_cmd(cxx, cpp, out=None, flags=(), opt='-O0'):
    cmd = [cxx, '-std=gnu++20', opt, '-Wall', '-Wextra', *flags, str(cpp)]
    return _00_memory_cap.capped(cmd + (['-o', str(out)] if out else ['-fsyntax-only']), COMPILE_MB)


def build_run(tmp: Path, source: str, cxx: str, flags=(), stdin='', opt='-O0') -> str:
    """Compile source and run it; stdout, or AssertionError with the command and diagnostics."""
    tmp.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(suffix='.cpp', dir=tmp)
    os.close(fd)
    cpp = Path(name)
    cpp.write_text(source, encoding='utf-8')
    exe = cpp.with_suffix('.out')
    cmd = compile_cmd(cxx, cpp, exe, flags, opt)
    r = subprocess.run(cmd, capture_output=True, text=True, timeout=900)
    if r.returncode or not exe.exists():
        raise AssertionError(f'compile failed (exit {r.returncode}): {" ".join(cmd)}\n{r.stderr[-4000:]}')
    run = subprocess.run(_00_memory_cap.capped([str(exe)], RUN_MB), input=stdin, capture_output=True, text=True, timeout=120)
    if run.returncode:
        raise AssertionError(f'{exe} exited {run.returncode}: {run.stderr[-2000:]}')
    return run.stdout


def syntax(tmp: Path, source: str, cxx: str, flags=()) -> subprocess.CompletedProcess:
    tmp.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(suffix='.cpp', dir=tmp)
    os.close(fd)
    Path(name).write_text(source, encoding='utf-8')
    return subprocess.run(compile_cmd(cxx, name, None, flags), capture_output=True, text=True, timeout=900)


def stripped(text: str) -> set[str]:
    return {l.strip() for l in text.split('\n')}


def o_expected_line(line: str) -> str:
    """What an emitted copy of a header line reads: pragma once and quoted includes become blank."""
    return '' if re.match(r'\s*#\s*(pragma\s+once|include\s*")', line) else line


def o_line_problems(out: str, texts: dict[str, list[str]], start: tuple[str, int] | None = None) -> list[str]:
    """Every line after `#line N "h"` must be line N, N+1, ... of h; a blank line directly before a directive only
    separates runs. start names the file of lines before the first directive."""
    lines = out.split('\n')
    cur = list(start) if start else None
    bad = []
    for k, l in enumerate(lines):
        m = re.fullmatch(r'#line (\d+) "([^"]+)"', l)
        if m:
            cur = [m[2], int(m[1])]
            continue
        if (l == '' and k + 1 < len(lines) and lines[k + 1].startswith('#line ')) or (l == '' and k == len(lines) - 1):
            continue
        if cur is None:
            bad.append(f'output line {k + 1} {l!r} before any #line')
            continue
        text = texts.get(cur[0])
        want = o_expected_line(text[cur[1] - 1]) if text is not None and 0 < cur[1] <= len(text) else None
        if want != l:
            bad.append(f'output line {k + 1} {l!r} != {cur[0]}:{cur[1]} {want!r}')
        cur[1] += 1
    return bad


def o_sections(out: str) -> list[str]:
    """Header of each #line run in order."""
    return re.findall(r'^#line \d+ "([^"]+)"$', out, re.M)


class Base(unittest.TestCase):
    def setUp(self):
        self.tmp = Path(tempfile.mkdtemp(prefix='grab-tester-')).resolve()
        self.addCleanup(lambda: shutil.rmtree(self.tmp, ignore_errors=True) if self._outcome.success else None)
        self.rng = random.Random(f'{SEED}:{self.id()}')

    def info(self, **kw):
        return f'seed={SEED} mode={MODE} tmp={self.tmp} ' + ' '.join(f'{k}={v!r}' for k, v in kw.items())

    def eq(self, actual, expected, **kw):
        self.assertEqual(actual, expected, self.info(**kw))

    def true(self, cond, **kw):
        self.assertTrue(cond, self.info(**kw))

    def compilers(self):
        found, missing = compilers()
        if not found:
            self.skipTest(f'SKIP compilers: {", ".join(missing)} not installed')
        for c in missing:
            print(f'SKIP {c}: not installed', flush=True)
        return found


class FixtureBase(Base):
    @classmethod
    def setUpClass(cls):
        if not HAVE_CLANG:
            raise unittest.SkipTest('SKIP libclang: python3 cannot import clang.cindex')
        cls.fx_dir = Path(tempfile.mkdtemp(prefix='grab-fixture-')).resolve()
        cls.fx = write_tree(cls.fx_dir / 'lib', FILES)
        cls.ix = built(cls.fx)

    @classmethod
    def tearDownClass(cls):
        shutil.rmtree(cls.fx_dir, ignore_errors=True)

    def info(self, **kw):
        return super().info(fixture=str(self.fx), **kw)

    def texts(self):
        return {h: t.split('\n') for h, t in FILES.items()}

    def grab(self, *names, directives=False):
        return G.grab(list(names), self.fx, self.ix, directives)

    def lineno(self, header, text):
        hits = [i for i, l in enumerate(FILES[header].split('\n'), 1) if l.strip() == text]
        self.eq(len(hits), 1, what='fixture line is unique', text=text)
        return hits[0]


class Units(FixtureBase):
    def test_units_alpha(self):
        rec = self.ix.records[A]
        units = rec['units']
        top = sorted((u['name'], u['kind'], u['ext'][0]) for u in units if u['parent'] is None)
        L = lambda t: self.lineno(A, t)
        want = sorted([('helperA', 'leaf', L(HELPER_A[2])), ('helperA', 'leaf', L(HELPER_A[3])),
                       ('unusedA', 'leaf', L('inline int unusedA(int x) { return x * 7; }')),
                       ('gammaF', 'leaf', L('inline int gammaF(int x) { return x * 3; }')),
                       ('helperB', 'leaf', L('inline int helperB(int x) { return x - 4; }')),
                       ('alpha_detail', 'ns', L('namespace alpha_detail {')),
                       ('Small', 'leaf', L('concept Small = sizeof(T) <= 8;') - 1), ('Cell', 'leaf', L('using Cell = lng;')),
                       ('ALPHA_K', 'leaf', L('constexpr int ALPHA_K = 5;')),
                       ('Trait', 'type', L('struct Trait { static constexpr int v = 1; };') - 1),
                       ('Trait', 'type', L('struct Trait<int> { static constexpr int v = 2; };') - 1),
                       ('Point', 'type', L('struct Point {')), ('manhattan', 'leaf', L(DEF)), ('operator==', 'leaf', L(EQ)),
                       ('hash', 'type', L(HASH) - 1), ('isOdd', 'leaf', L('inline bool isOdd(int v) { return v & 1; }')),
                       ('useNorm', 'leaf', L('inline int useNorm(Point p) { return p.norm(); }')),
                       ('useAlias', 'leaf', L('template<Small T = Cell>')), ('useConcept', 'leaf', L('template<typename T> requires Small<T>')),
                       ('a1', 'leaf', L('inline int a1 = 1; inline int a2 = helperB(3);')),
                       ('a2', 'leaf', L('inline int a1 = 1; inline int a2 = helperB(3);')),
                       ('simdSum', 'leaf', L('inline int simdSum(int a, int b) { return a + b + 0; }')),
                       ('simdSum', 'leaf', L('inline int simdSum(int a, int b) { return b + a; }')),
                       ('Box', 'type', L('struct Box {') - 1), ('callFoo', 'leaf', L('M callFoo(M x) { Box<M> s(x); return s.foo(); }') - 1),
                       ('callAs', 'leaf', L('M callAs(M x) { Box<M> s(x); return s.template as<M>() + 1; }') - 1),
                       ('Req', 'type', L('requires requires(T x) { x + x; }') - 1),
                       ('useReq', 'leaf', L('inline int useReq() { Req<int> r{4}; return r.used(); }')),
                       ('callPeek', 'leaf', L('M callPeek(M x) { Box<M> s(x); return peek(s); }') - 1),
                       ('outer', 'ns', L('namespace outer::inner {'))])
        self.eq(top, want)
        by = {u['qual']: u for u in units}
        always = {q for q, u in by.items() if q.startswith(('Point::', 'Box::')) and u['always']}
        self.eq(always, {'Point::x', 'Point::y', 'Point::plainField', 'Point::Point', 'Point::~Point', 'Point::operator bool',
                         'Point::operator<', 'Point::operator+', 'Point::counter', 'Point::value_type', 'Point::Inner', 'Point::size',
                         'Box::v', 'Box::Box<M>', 'Box::get'})
        self.eq(sorted(q for q, u in by.items() if q.startswith(('Point::', 'Box::')) and not u['always']),
                ['Box::as', 'Box::bar', 'Box::foo', 'Box::peek', 'Box::poke', 'Point::dotP', 'Point::isOdd', 'Point::manhattan',
                 'Point::norm', 'Point::twiceNorm'])
        self.eq(sum(u['name'] == 'Box<M>' for u in units), 2, what='constructor and constructor template')
        self.eq([(u['name'], u['attach']) for u in units if u['attach']], [('manhattan', 'owner'), ('operator==', 'refs'), ('hash', 'refs')])
        self.true('debugField' not in {u['name'] for u in units}, what='a branch no configuration parsed has no unit')
        self.eq(sum(u['qual'] == 'Point' for u in units), 1, what='Point merged across configurations')
        self.eq(sorted(u['qual'] for u in units if u['detail']), ['alpha_detail::thrice', 'alpha_detail::twice'])
        self.eq(sorted(u['qual'] for u in units if u['parent'] is not None and units[u['parent']]['kind'] == 'ns'),
                ['alpha_detail::thrice', 'alpha_detail::twice', 'outer::inner', 'outer::inner::deep'])
        lines = FILES[A].split('\n')
        dirs = [i for i, l in enumerate(lines, 1) if re.match('#(if|else|endif)', l)]
        groups = [tuple(dirs[:3]), tuple(dirs[3:])]
        self.eq([lines[i - 1] for i in dirs], ['#ifdef SOME_UNDEFINED_MACRO', '#else', '#endif', '#ifdef __AVX2__', '#else', '#endif'])
        self.eq((sorted(map(tuple, rec['groups'])), rec['includes']), (groups, [2]))


class Selection(FixtureBase):
    def test_selection_table(self):
        cxxs = self.compilers()
        texts = self.texts()
        for root, (kept, dropped, main, want) in SELECTION.items():
            with self.subTest(root=root):
                out = self.grab(root)
                lines = stripped(out)
                missing = [k for k in kept if k not in lines]
                present = [d for d in dropped + GLOBAL_DROPPED if d in lines] + [l for l in out.split('\n') if l.lstrip().startswith('#include "')]
                self.eq((missing, present), ([], []), root=root, what='kept lines missing, dropped lines present', out=out)
                self.eq(DECL in lines, DEF in lines, root=root, what='out-of-line member and its declaration together', out=out)
                self.eq(o_line_problems(self.grab(root, directives=True), texts), [], root=root)
                for cxx in cxxs:
                    got = build_run(self.tmp / root.replace(':', '_'), prelude(self.fx) + out + f'int main() {{\n{main}\n}}\n', cxx)
                    self.eq(got, want, root=root, cxx=cxx, out=out)

    def test_unparsed_branch_compiles(self):
        out = self.grab('useNorm')
        for cxx in self.compilers():
            got = build_run(self.tmp, prelude(self.fx) + out + 'int main() {\n' + SELECTION['useNorm'][2] + '\n}\n', cxx,
                            ['-DSOME_UNDEFINED_MACRO'])
            self.eq(got, '25 1\n7\n', cxx=cxx, out=out)

    def test_avx2_group(self):
        units = [u for u in self.ix.records[A]['units'] if u['name'] == 'simdSum']
        self.eq(sorted(u['ext'][0] for u in units), sorted([self.lineno(A, 'inline int simdSum(int a, int b) { return a + b + 0; }'),
                                                            self.lineno(A, 'inline int simdSum(int a, int b) { return b + a; }')]))
        out = self.grab('simdSum')
        body = '\n'.join(l for l in out.split('\n') if l and not l.startswith('#include'))
        self.eq(body, '#ifdef __AVX2__\ninline int simdSum(int a, int b) { return a + b + 0; }\n#else\n'
                      'inline int simdSum(int a, int b) { return b + a; }\n#endif', out=out)
        for cxx in self.compilers():
            r = syntax(self.tmp, prelude(self.fx) + out + 'int main() { return simdSum(1, 2) - 3; }\n', cxx, ['-mavx2'])
            self.eq(r.returncode, 0, cxx=cxx, err=r.stderr[-2000:])

    def test_std_hash_with_type(self):
        for root in ('Point', 'useNorm', 'betaPoint'):
            out = self.grab(root)
            self.true(HASH in stripped(out) and EQ in stripped(out), root=root, what='std::hash and operator== kept with Point', out=out)
        for root in ('helperA', 'isOdd', 'useAlias'):
            out = self.grab(root)
            self.true(HASH not in stripped(out) and EQ not in stripped(out), root=root, what='dropped without Point', out=out)

    def test_out_of_line_member(self):
        for root in ('betaPoint', 'Point::manhattan', 'useNorm', 'Point::norm'):
            out = self.grab(root)
            lines = stripped(out)
            if root in ('betaPoint', 'Point::manhattan'):
                self.true(DECL in lines and DEF in lines, root=root, what='called member keeps declaration and definition', out=out)
            self.eq(DECL in lines, DEF in lines, root=root, what='an out-of-line definition needs its declaration', out=out)


class Render(FixtureBase):
    def test_render_order(self):
        closures = {h: set(G.closure(self.ix.graph, h)) for h in self.ix.graph}
        for names in (['lonely', 'betaUse', 'helperA'], ['betaDep', 'lonely'], ['betaPoint', 'simdSum', 'deep']):
            out = self.grab(*names, directives=True)
            secs = o_sections(out)
            order = list(dict.fromkeys(secs))
            self.eq(secs, sorted(secs, key=order.index), names=names, what='one contiguous run per header', out=out)
            for i, h in enumerate(order):
                self.true(not any(h in closures[e] for e in order[:i]), names=names, header=h, what='after every header including it', order=order)
            self.eq(re.sub(r'^#line .*\n', '', out, flags=re.M), self.grab(*names), names=names, what='directives only added')
            self.true('\n\n\n' not in out and not out.startswith('\n'), names=names, out=out)
        out = self.grab('betaUse')
        self.eq(out, '#include <cassert>\n\n' + '\n'.join(HELPER_A) + '\n\n#include <numeric>\n\n'
                     'inline int betaUse(int x) { return helperA(x) * 2; }\n')
        self.eq(self.grab('a1'), '#include <cassert>\n\ninline int helperB(int x) { return x - 4; }\n\n'
                                 'inline int a1 = 1; inline int a2 = helperB(3);\n', what='blank line only where one was skipped')

    def test_render_lines(self):
        texts = self.texts()
        for names in (['Point'], ['betaPoint', 'lonely'], ['callFoo', 'useAlias', 'outer::inner::deep'], ['useNorm']):
            out = self.grab(*names, directives=True)
            self.eq(o_line_problems(out, texts), [], names=names, out=out)
            self.true(out.startswith('#line '), names=names)

    def test_lines_diagnostic(self):
        out = self.grab('boom', directives=True)
        line = self.lineno(U, 'static_assert(sizeof(T) == 0, "boom fired");')
        for cxx in self.compilers():
            r = syntax(self.tmp, prelude(self.fx) + out + 'int main() { return boom(1); }\n', cxx)
            hits = re.findall(rf'^{re.escape(U)}:(\d+):\d+: error', r.stderr, re.M)
            self.true(r.returncode != 0 and hits and int(hits[0]) == line, cxx=cxx, want=line, hits=hits, err=r.stderr[-2000:])


class Resolve(FixtureBase):
    def quals(self, name):
        return sorted((self.ix.units[i]['qual'], self.ix.units[i]['header'], self.ix.units[i]['ext'][0]) for i in self.ix.resolve(name))

    def test_resolve(self):
        L = lambda t: self.lineno(A, t)
        self.eq(self.quals('Point::isOdd'), [('Point::isOdd', A, L(POINT_METHODS['isOdd']))])
        self.eq(self.quals('isOdd'), [('isOdd', A, L('inline bool isOdd(int v) { return v & 1; }'))], what='namespace scope first')
        deep = [('outer::inner::deep', A, L('inline int deep(int x) { return x + 100; }'))]
        self.eq([self.quals(n) for n in ('outer::inner::deep', 'inner::deep', 'deep')], [deep] * 3)
        self.eq(self.quals('alpha_detail::thrice'), [('alpha_detail::thrice', A, L('inline int thrice(int x) { return 3 * x; }'))])
        self.eq(self.quals('gammaF'), sorted([('gammaF', A, L('inline int gammaF(int x) { return x * 3; }')),
                                              ('gammaF', U, self.lineno(U, 'inline int gammaF(lng x) { return int(x) * 1000; }'))]))
        self.eq(self.quals('helperA'), [('helperA', A, L(HELPER_A[2])), ('helperA', A, L(HELPER_A[3]))])
        self.eq([self.quals(n) for n in ('nosuch', 'Point::nosuch', 'inner::isOdd', 'Box::norm')], [[]] * 4)

    def test_grab_unknown(self):
        with self.assertRaises(G.GrabError) as e:
            self.grab('helperA', 'nosuch')
        self.eq(str(e.exception), "unknown name 'nosuch' (try --find)")
        with self.assertRaises(G.GrabError) as e:
            self.grab('helpera')
        self.assertTrue(str(e.exception).startswith("unknown name 'helpera' (did you mean helperA") and str(e.exception).endswith('? try --find)'),
                        self.info(error=str(e.exception)))

    def test_grab_suggest(self):
        # case ignored, closest first, at most five, each one a name grab resolves; bare typos get top-level names only,
        # qualified ones members too; detail names and far names never
        S = lambda n: G.suggest(self.ix, n)
        cases = {'helpera': 'helperA', 'HELPERA': 'helperA', 'isodd': 'isOdd', 'Point::twicenorm': 'Point::twiceNorm', 'Point::nrom': 'Point::norm', 'point': 'Point'}
        for typo, first in cases.items():
            got = S(typo)
            self.assertTrue(got[:1] == [first] and len(got) <= 5 and all(self.ix.resolve(g) for g in got), self.info(typo=typo, got=got))
        for typo in ('twicenorm', 'helpera', 'isodd', 'thrise', 'twise'):
            self.assertTrue(not any('::' in g or g in ('twiceNorm', 'norm', 'dotP') for g in S(typo)), self.info(typo=typo, got=S(typo)))
        for typo in ('thrise', 'twise', 'alpha_detail::thrise', 'alpha_detail::twise'):
            self.assertTrue(not any('thrice' in g or 'twice' == g.split('::')[-1] for g in S(typo)), self.info(typo=typo, got=S(typo)))
        self.eq(S('qqqqqqqq'), [])


class Cli(FixtureBase):
    def main(self, *argv, path=None):
        out, err = io.StringIO(), io.StringIO()
        env = {'PATH': str(path)} if path else {}
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err), patch.object(G, 'Index', lambda *a, **k: self.ix), \
                patch.object(G, 'ROOT', self.fx), patch.dict(os.environ, env):
            try:
                code = G.main(list(argv))
            except SystemExit as e:
                code = e.code
        return code, out.getvalue(), err.getvalue()

    def empty_path(self):
        p = self.tmp / 'bin'
        p.mkdir(exist_ok=True)
        return p

    def test_cli_names(self):
        code, out, err = self.main('--no-copy', 'helperA')
        self.eq((code, out, err), (0, '#include <cassert>\n\n' + '\n'.join(HELPER_A) + '\n', ''))
        code, out, err = self.main('--no-copy', '--lines', 'helperA')
        self.eq((code, out.split('\n')[0], o_line_problems(out, self.texts())), (0, f'#line 2 "{A}"', []), err=err)
        code, out, err = self.main('--no-copy', 'helperA', 'nosuch')
        self.eq((code, out, err), (1, '', "grab: unknown name 'nosuch' (try --find)\n"))

    def test_cli_find(self):
        self.eq(self.main('--find', 'odd'), (0, 'no match\n', ''), what='--find reads inventory rows (14-search_tester.py), never the index')

    def test_cli_arguments(self):
        f = self.tmp / 'a.cpp'
        f.write_text('void solve(int t) {}\n')
        for argv in ([], ['--no-copy'], ['x', '--find', 'y'], ['--into', str(f), '--find', 'y'], ['x', '--into', str(f)]):
            code, out, err = self.main(*argv)
            self.true(code == 2 and 'give names, --into FILE or --find WORDS' in err and out == '', argv=argv, err=err)
        self.eq(f.read_text(), 'void solve(int t) {}\n', what='argument errors leave --into files alone')

    def test_cli_into(self):
        f = self.tmp / 'sol.cpp'
        f.write_text('void solve(int t) { printf("%d\\n", helperA(t)); }\n')
        code, out, err = self.main('--into', str(f))
        self.eq((code, out, err), (0, '', f'{f}: block holds helperA\n'))
        self.true(f.read_text().startswith('// grab begin: helperA\n'), text=f.read_text())
        f.write_text('void solve(int t) { printf("%d\\n", t); }\n')
        self.eq(self.main('--into', str(f)), (0, '', f'{f}: block holds nothing (no library names used)\n'))
        g = self.tmp / 'nosolve.cpp'
        g.write_text('int main() { return helperA(1); }\n')
        code, out, err = self.main('--into', str(g))
        self.eq((code, err), (1, f'grab: {g}: no "void solve(" line to put the block above\n'))
        f.write_text('void solve(int t) { Point p(t, 1); printf("%d\\n", p.norm()); }\n')
        self.eq(self.main('--into', str(f), '--min')[0], 0)
        self.true(POINT_METHODS['isOdd'] not in f.read_text(), text=f.read_text(), what='--min prunes')
        self.eq(self.main('--into', str(f))[0], 0)
        self.true(POINT_METHODS['isOdd'] in f.read_text(), text=f.read_text(), what='default keeps the root whole')
        code, out, err = self.main('--min', 'helperA')
        self.true(code == 2 and '--min needs --into' in err, err=err)
        code, out, err = self.main('--into', str(self.tmp / 'missing.cpp'))
        self.true(code == 1 and err.startswith('grab: ') and 'missing.cpp' in err, err=err, what='OSError reported')

    def test_cli_copy(self):
        code, out, err = self.main('helperA', path=self.empty_path())
        self.eq((code, err), (0, 'grab: 6 lines, not copied (no wl-copy, xclip, xsel or pbcopy)\n'))
        fake = self.tmp / 'fake'
        fake.mkdir()
        sink = self.tmp / 'clipboard.txt'
        tool = fake / 'wl-copy'
        tool.write_text(f"#!/bin/sh\nexec /bin/cat > '{sink}'\n")
        tool.chmod(0o755)
        code, out, err = self.main('helperA', path=fake)
        self.eq((code, err, sink.read_text()), (0, 'grab: 6 lines, copied (wl-copy)\n', out))
        sink.unlink()
        code, out2, err = self.main('--no-copy', 'helperA', path=fake)
        self.eq((code, out2, err, sink.exists()), (0, out, '', False), what='--no-copy prints only')


class Tokens(FixtureBase):
    def test_declared(self):
        code = 'mt19937_64 rng(1); DSU dsu(n); x = a * inv(b);'
        self.eq(G.declared(code) & {'rng', 'dsu', 'inv', 'x', 'a', 'b', 'n', 'DSU', 'mt19937_64'}, {'rng', 'dsu'})
        cases = {'int lonely = 3;': {'lonely'}, 'vector<int> helperA{1};': {'helperA'}, 'struct Point { int q; };': {'Point'},
                 'using Cell = int;': {'Cell'}, 'return useNorm(p);': set(), 'p.norm(); q->size(); A::helperA(2);': set(),
                 'auto f = [&](int k) { return k; };': {'f'}, 'for (int i = 0; i < n; ++i) {}': set(),
                 # members, locals and parameters only shadow; anonymous and named namespaces are namespace scope
                 'struct Brute { ulng phi = 1; };': {'Brute'}, 'int main() { int local = 2; }': {'main'},
                 'namespace { lng cnt = 0; } namespace a::b { int deep; }': {'cnt', 'deep'},
                 'auto pos = [&](typename SortedList<T, C>::iterator it) {};': {'pos'},
                 'struct MyB : public Base, virtual Other { int z; };': {'MyB'},
                 # every declarator of a declaration; a type at the end of a line declares nothing
                 'int n, rng, k;': {'n', 'rng', 'k'}, 'lng a = 1, dot = f(1, 2), *p = nullptr;': {'a', 'dot', 'p'},
                 'std::tuple<int, lng, int> t;': {'t'}, 'std::pair<int, int> p, q;': {'p', 'q'}, 'vector<int> v = {1, 2}, w;': {'v', 'w'},
                 'int x = a < b, y;': {'x', 'y'}, 'bool ok = a == b, z;': {'ok', 'z'}, 'void f() { int a, b; }\nint u, w;': {'f', 'u', 'w'},
                 'static DSU\nd(5);': {'d'}, 'printf("%d", g(a, b));': set()}
        for text, want in cases.items():
            self.eq(G.declared(text), want, text=text)

    def test_roots_of(self):
        text = ('void solve(int t) { int n = 3; Point p(1, 2); printf("%d %d %d %d %d", useNorm(p), p.size(), betaUse(n), '
                'alpha_detail::thrice(2), lonely(1)); } // helperA in a comment, "isOdd" in a string')
        q = lambda roots: sorted(self.ix.units[i]['qual'] for i in roots)
        roots, ids = G.roots_of(self.ix, text)
        self.eq(q(roots), ['Point', 'alpha_detail::thrice', 'betaUse', 'lonely', 'useNorm'])
        self.true({'p', 'size', 'useNorm'} <= ids and 'helperA' not in ids and 'isOdd' not in ids, ids=sorted(ids))
        self.eq(q(G.roots_of(self.ix, text, {A})[0]), ['Point', 'alpha_detail::thrice', 'useNorm'])
        self.eq(q(G.roots_of(self.ix, text, {U, B})[0]), ['betaUse', 'lonely'])
        self.eq(q(G.roots_of(self.ix, 'int lonely = 2; void solve(int t) { lonely += twice(t) + deep(1) + Point::isOdd(1); }')[0]),
                ['Point', 'isOdd', 'outer::inner::deep'], what='own names and unqualified detail names out; Point::isOdd spells isOdd')

    def test_roots_of_macros_and_locals(self):
        q = lambda text: sorted(self.ix.units[i]['qual'] for i in G.roots_of(self.ix, text)[0])
        self.eq(q('#define PT Point // lonely\n#define F(x, helperA) helperA(x) + useNorm(x) + "isOdd"\nvoid solve() { PT p(1, 2); }'),
                ['Point', 'useNorm'], what='#define bodies name roots; their parameters, literals and comments do not')
        self.eq(q('#define lonely 3\nvoid solve() { int x = lonely; }'), [], what='a macro is the file\'s own name')
        self.eq(q('void solve() { auto useNorm = [&](auto &&useNorm, int u) -> int { return u ? useNorm(useNorm, u - 1) : 0; }; useNorm(useNorm, 3); }'), [],
                what='a local lambda')
        self.eq(q('int go(int lonely) { return lonely; }\nstruct S { int betaUse; int f() const { return betaUse; } };\nvoid solve() { S s; s.betaUse = go(1); }'), [],
                what='a parameter and a member')
        self.eq(q('void solve() { { int lonely = 1; } printf("%d", lonely(2)); for (int useNorm = 0; useNorm < 1; useNorm++) {} }'), ['lonely'],
                what='a use outside the local scope names the library')
        self.eq(sorted(G.local_names('void f() { for (auto v : g) h(v); int a = 1, b = a; x = c * d; }')), ['a', 'b', 'v'])

    def test_grab_text(self):
        text = 'void solve(int t) { Point p(t, 1); printf("%d %d\\n", p.norm(), alpha_detail::thrice(t)); }\nint main() { solve(2); }\n'
        out, names = G.grab_text(text, self.fx, self.ix)
        self.eq(names, ['Point', 'alpha_detail::thrice'])
        kept, dropped = point('norm')
        lines = stripped(out)
        self.eq(([k for k in [k for k in kept if k != HASH] + ['namespace alpha_detail {', 'inline int thrice(int x) { return 3 * x; }'] if k not in lines],
                 [d for d in dropped + ['inline int twice(int x) { return 2 * x; }'] if d in lines]), ([], []), out=out)
        self.eq(G.grab_text('int main() { return 0; }\n', self.fx, self.ix), ('', []))
        out, names = G.grab_text(text, self.fx, self.ix, allowed={U})
        self.eq((out, names), ('', []))


class Into(FixtureBase):
    SOL = ('#include <cstdio>\n// solution\nint helperLocal = 1;\n\nvoid solve(int t) {\n'
           '    printf("%d %d %d\\n", helperA(t), useAlias(t), alpha_detail::thrice(t));\n}\nint main() { solve(2); }\n')

    def into(self, f):
        return G.into(f, self.fx, self.ix)

    def test_into_insert_and_rerun(self):
        f = self.tmp / 'sol.cpp'
        f.write_text(self.SOL)
        self.eq(self.into(f), ['alpha_detail::thrice', 'helperA', 'useAlias'])
        text = f.read_text()
        lines = text.split('\n')
        b, e, s = lines.index('// grab begin: alpha_detail::thrice helperA useAlias'), lines.index('// grab end'), lines.index('void solve(int t) {')
        self.true(b == 4 and b < e and e + 1 == s and lines[:4] == self.SOL.split('\n')[:4], text=text)
        self.eq('\n'.join(lines[:b] + lines[e + 1:]), self.SOL, what='only the block was added')
        block = stripped('\n'.join(lines[b:e]))
        self.true({'inline int thrice(int x) { return 3 * x; }', 'inline int twice(int x) { return 2 * x; }', HELPER_A[2]} <= block
                  and 'inline int unusedA(int x) { return x * 7; }' not in block, text=text)
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(self.fx) + text, cxx), '3 9 6\n', cxx=cxx, text=text)
        self.eq(self.into(f), ['alpha_detail::thrice', 'helperA', 'useAlias'])
        self.eq(f.read_text(), text, what='rerun is identical')

    def test_into_stale_and_removed(self):
        f = self.tmp / 'sol.cpp'
        f.write_text(self.SOL)
        self.into(f)
        f.write_text(f.read_text().replace('helperA(t), useAlias(t), alpha_detail::thrice(t)', 'lonely(t), t, t'))
        self.eq(self.into(f), ['lonely'])
        text = f.read_text()
        self.true(text.count('// grab begin:') == 1 and '// grab begin: lonely\n' in text and 'useAlias' not in text.split('// grab end')[0]
                  and 'inline int lonely(int x) { return x; }' in text, text=text)
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(self.fx) + text, cxx), '2 2 2\n', cxx=cxx, text=text)
        f.write_text(text.replace('lonely(t), t, t', 't, t, t'))
        self.eq(self.into(f), [])
        self.eq(f.read_text(), self.SOL.replace('helperA(t), useAlias(t), alpha_detail::thrice(t)', 't, t, t'))

    def test_into_whole_and_min(self):
        src = 'void solve(int t) { Point p(t, 1); printf("%d\\n", p.norm()); }\nint main() { solve(2); }\n'
        f = self.tmp / 'sol.cpp'
        for prune, (kept, dropped) in ((False, (PRUNED_POINT + list(POINT_METHODS.values()) + [DECL, DEF], [])), (True, point('norm'))):
            f.write_text(src)
            self.eq(G.into(f, self.fx, self.ix, prune), ['Point'], prune=prune)
            text = f.read_text()
            block = stripped(text.split('// grab end')[0])
            self.eq(([k for k in kept if k not in block], [d for d in dropped if d in block]), ([], []), prune=prune, text=text)
            for cxx in self.compilers():
                self.eq(build_run(self.tmp, prelude(self.fx) + text, cxx), '5\n', cxx=cxx, prune=prune, text=text)

    def test_into_errors(self):
        f = self.tmp / 'sol.cpp'
        for text, msg in (('int main() { return helperA(1); }\n', 'no "void solve(" line to put the block above'),
                          ('// grab begin: helperA\nvoid solve(int t) {}\n', '"// grab begin:" without a following "// grab end"'),
                          ('// grab end\n// grab begin: helperA\nvoid solve(int t) {}\n', '"// grab begin:" without a following "// grab end"')):
            f.write_text(text)
            with self.assertRaises(G.GrabError) as e:
                self.into(f)
            self.eq((str(e.exception), f.read_text()), (f'{f}: {msg}', text))


class Bundle(FixtureBase):
    SRC = ('// sol\n#include "../../../05-Mathematics/01-beta.hpp"\n#include "../../../02-Data Structures/01-alpha.hpp"\n\n'
           'void solve(int t) { printf("%d %d\\n", betaUse(t), useAlias(t)); }\nint main() { solve(3); }\n')
    REL = '97-Online Testing/src/j/sol.cpp'

    def source(self, text, name='sol.cpp'):
        p = self.fx / '97-Online Testing/src/j' / name
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(text)
        self.addCleanup(p.unlink)
        return p

    def test_bundle_compiles_with_lines(self):
        out = G.bundle(self.source(self.SRC), self.fx, self.ix)
        lines = out.split('\n')
        self.eq(lines[:2], ['// sol', f'#line 1 "{T}"'])
        self.true(f'#line 1 "{D}"' in lines and f'#line 3 "{self.REL}"' in lines and '#include "' not in out, out=out)
        texts = self.texts() | {self.REL: [o_expected_line(l) for l in self.SRC.split('\n')]}
        self.eq(o_line_problems(out, texts, (self.REL, 1)), [], out=out)
        self.eq(o_sections(out)[:2], [T, D])
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, out, cxx), '8 11\n', cxx=cxx, out=out)

    def test_bundle_error_line(self):
        src = self.SRC.replace('\nvoid solve', '\nint broken = undefined_name_xyz;\nvoid solve')
        want = src.split('\n').index('int broken = undefined_name_xyz;') + 1
        out = G.bundle(self.source(src), self.fx, self.ix)
        for cxx in self.compilers():
            r = syntax(self.tmp, out, cxx)
            hits = re.findall(rf'^{re.escape(self.REL)}:(\d+):\d+: error', r.stderr, re.M)
            self.true(r.returncode and hits == [str(want)], cxx=cxx, want=want, hits=hits, err=r.stderr[-2000:])

    def test_bundle_non_library_include(self):
        (self.fx / '97-Online Testing/src/j').mkdir(parents=True, exist_ok=True)
        local = self.source('#pragma once\n', 'local.hpp')
        self.assertTrue(local.exists())
        p = self.source('#include "../../../02-Data Structures/01-alpha.hpp"\n#include "local.hpp"\nint main() {}\n', 'bad.cpp')
        with self.assertRaises(G.GrabError) as e:
            G.bundle(p, self.fx, self.ix)
        self.eq(str(e.exception), '97-Online Testing/src/j/bad.cpp:2: local.hpp is not a library header (use --bundler oj-bundle)')

    def test_bundle_passthrough_and_template_only(self):
        text = '#include <cstdio>\nint main() { printf("1\\n"); }\n'
        self.eq(G.bundle(self.source(text, 'plain.cpp'), self.fx, self.ix), text)
        src = '#include "../../../01-Core/01-template.hpp"\nint main() { lng x = 4; printf("%lld\\n", (long long)x); }\n'
        out = G.bundle(self.source(src, 'tpl.cpp'), self.fx, self.ix)
        self.true(out.startswith(f'#line 1 "{T}"\n') and f'#line 2 "97-Online Testing/src/j/tpl.cpp"' in out and D not in out, out=out)
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, out, cxx), '4\n', cxx=cxx, out=out)


WRAP = '''#pragma once
#include "../01-Core/01-template.hpp"

namespace wrap_detail {
    template<int K>
    struct Value {
        int v;
        Value(int x) : v(x) {}
        int pow() const { return v * K; }
        int sqrt() const { return v + K; }
    };
} // namespace wrap_detail

template<int K>
using Wrap = wrap_detail::Value<K>;
using wrap7 = Wrap<7>;

struct Base {
    int b = 1;
    int get2() const { return 2 * b; }
    bool operator==(const Base &o) const { return b == o.b; }
    struct Inner {
        int w;
        Inner(int x) : w(x) {}
        int twice() const { return 2 * w; }
    };
};

struct Derived : public Base {
    int d = 3;
};

enum class Kind { Plain, Odd };

template<class T>
struct Box {
    T v;
    T get() const { return v; }
};

template<class T>
struct UOnly {
    static_assert(std::is_unsigned_v<T>);
    T v;
};

template<class T>
int callG(T t) { return g(t); }

inline constexpr int POISONW = 5;
#ifdef WRAP_EXTRA
inline int pick() { return POISONW + 1; }
#else
inline int pick() { return 1; }
#endif

template<int K = 2>
int twiceK(int x) { return K * x; }

template<int K = 2>
int useValue(int x) { wrap_detail::Value<K> v(x); return v.pow(); }
'''


MODE_HPP = '#pragma once\n#include "../01-Core/01-template.hpp"\n\ninline int mode() {\n#ifdef B_FAST\n    return 1;\n#else\n    return 2;\n#endif\n}\n'
Y_HPP = '#pragma once\n#include "../01-Core/01-template.hpp"\n\nstruct YT { int y = 5; };\ninline int g(YT t) { return t.y; }\n'
FOLD_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\ntemplate<typename M>\nstruct Folder {\n'
            '    typename M::S fold(typename M::S a) const { return M::op(a, M::e()); }\n};\n')
MONOIDS_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\ntemplate<typename T>\nstruct Sum {\n    using S = T;\n'
               '    static S op(S a, S b) { return a + b; }\n    static S e() { return S(0); }\n};\n')
NET_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\nstruct NetLike { int n = 0; };\n'
           'struct Net {\n    int n = 0, adj[4] = {};\n    void link(int v) { adj[n++] = v; }\n'
           '    int operator[](int v) const { return adj[v]; }\n    int unusedNet() const { return 1; }\n};\n'
           'template<class G = Net> int netSize(const G &g) { return g.n; }\n'
           'template<class G> int netFirst(const G &g) { return g[0] + g.n; }\n'
           'template<class S> int seqLen(const S &s) { return int(s.size()); }\n'
           'struct Walker {\n    int s;\n    template<class G = Net> Walker(const G &g) : s(g.n) {}\n};\n'
           'struct NetStat {\n    int a;\n    int twice() const { return 2 * a; }\n    int unusedStat() const { return 0; }\n};\n'
           'inline NetStat netStat(int a) { return NetStat{a}; }\n'
           'struct AddM {\n    int op(int a, int b) const { return a + b; }\n    int e() const { return 0; }\n};\n'
           'template<class M> int foldM(const M &m, int a) { return m.op(a, m.e()); }\n'
           'namespace net_detail {\ntemplate<class G> int peek(const G &g) { return g[0] + g.n; }\n} // namespace net_detail\n'
           'template<class T>\nstruct BaseOps {\n    T v{};\n    T add(T x) const { return v + x; }\n    T sub(T x) const { return v - x; }\n'
           '    T unusedOps() const { return v; }\n};\n'
           'template<class T>\nstruct UsesBase : BaseOps<T> {\n    using B = BaseOps<T>;\n    using B::add, B::sub;\n'
           '    T twiceAdd(T x) const { return add(x) * 2; }\n};\n')
NET_USE_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n#include "09-net.hpp"\n\n'
               'struct BaseG { int n = 1; };\nstruct DerivedG : BaseG {\n    int extra = 2;\n};\n'
               'struct Gr {\n    int n = 2, at[2] = {7, 8};\n    int operator[](int k) const { return at[k]; }\n};\n'
               'template<class G> int netLast(const G &g) { return g[g.n - 1]; }\n'
               'template<class G> int derivedSum(const G &g) { return g.extra + g.n; }\n')
REACH_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n#include "09-net.hpp"\n\n'
             'struct Holder {\n    int f(const NetStat &s) const;\n};\n'
             'inline int Holder::f(const NetStat &s) const { return s.a + 1000; }\n'
             'namespace reach_detail {\n    template<class T> T mul(T a, T b) { return a * b + 1000; }\n'
             '    template<class T> T key(T x) { return x + 1000; }\n    struct Box { int v; };\n'
             '    inline int open(Box b) { return b.v; }\n    template<class T> T twice(T x) { return 2 * x; }\n'
             '} // namespace reach_detail\n')
REACH_USE_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n#include "11-reach.hpp"\n\n'
                 'template<class C> struct Ctx {\n    static int twice(int a) { return C::mul(a, 2); }\n};\n'
                 'struct M2 {\n    static int mul(int a, int b) { return a * b; }\n};\n'
                 'inline int useCtx(int a) { return Ctx<M2>::twice(a); }\n'
                 'namespace use_detail {\n    template<class T> T key(T x) { return x + 1; }\n'
                 '    template<class T> T run(T x) { return key(x); }\n} // namespace use_detail\n'
                 'inline int useKey(int x) { return use_detail::run(x); }\n'
                 'template<class B> int peekBox(B b) { return open(b); }\n'
                 'inline int useBox() { return peekBox(reach_detail::Box{9}); }\n'
                 'template<class T> T viaUsing(T x) { using namespace reach_detail; return twice(x); }\n')
LOOKUP_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\n'
              'namespace e_detail {\n'
              '    enum class Color { R, G };\n'
              '    inline int code(Color c) { return int(c) + 7; }\n'
              '} // namespace e_detail\n'
              'template<class T> int codeOf(T t) { return code(t); }\n'
              'inline int useColor() { return codeOf(e_detail::Color::G); }\n'
              'namespace q_detail {\n'
              '    template<class T> T helper(T x) { return x + 1; }\n'
              '    template<class T> T g(T x);\n'
              '} // namespace q_detail\n'
              'template<class T> T q_detail::g(T x) { return helper(x); }\n'
              'inline int useG() { return q_detail::g(3); }\n'
              'namespace al_detail {\n'
              '    template<class T> T af(T x) { return x * 5; }\n'
              '    template<class T> T af(T x, T y) { return x * y; }\n'
              '} // namespace al_detail\n'
              'namespace al = al_detail;\n'
              'template<class T> T callAl(T x) { return al::af(x); }\n'
              'inline int useAl() { return callAl(2); }\n'
              'template<class T> T gf(T x) { return x + 5; }\n'
              'template<class T> T gf(T x, T y) { return x + y; }\n'
              'template<class T> T callGf(T x) { return ::gf(x); }\n'
              'inline int useGf() { return callGf(2); }\n'
              'namespace ud_detail {\n'
              '    template<class T> T uf(T x) { return x * 3; }\n'
              '    template<class T> T uf(T x, T y) { return x - y; }\n'
              '} // namespace ud_detail\n'
              'using ud_detail::uf;\n'
              'template<class T> T callUf(T x) { return uf(x); }\n'
              'inline int useUf() { return callUf(2); }\n'
              'namespace inner_ns {\n'
              '    template<class T> T callUf2(T x) { using ud_detail::uf; return uf(x, x + 1); }\n'
              '} // namespace inner_ns\n'
              'inline int useUf2() { return inner_ns::callUf2(2); }\n'
              'struct Other { int v = 4; };\n'
              'template<class T> struct A {\n'
              '    T x{};\n'
              '    T f() const;\n'
              '    template<class U> U g(U u) const;\n'
              '    static int cnt;\n'
              '    int unused() const;\n'
              '};\n'
              'template<class T> T A<T>::f() const { return x + 1; }\n'
              'template<class T> template<class U> U A<T>::g(U u) const { return u + U(x); }\n'
              'template<class T> int A<T>::cnt = 11;\n'
              'template<class T> int A<T>::unused() const { return 99; }\n'
              'struct B {\n'
              '    int v;\n'
              '    explicit B(int x);\n'
              '    struct Inner { int h() const; };\n'
              '    int take(const Other &o) const;\n'
              '};\n'
              'inline B::B(int x) : v(x) {}\n'
              'inline int B::Inner::h() const { return 21; }\n'
              'inline int B::take(const Other &o) const { return o.v + v; }\n'
              'inline int useOther() { return Other{}.v; }\n'
              'inline int useA() { return A<int>{}.f() + A<int>{}.g(2) + A<int>::cnt; }\n'
              'inline int useB() { return B(3).v + B::Inner{}.h(); }\n'
              'namespace outer {\n'
              '    template<class T> T nf(T x) { return x + 100; }\n'
              '    template<class T> T nf(T x, T y) { return x + y; }\n'
              '    namespace mid {\n'
              '        template<class T> T mf(T x) { return x + 200; }\n'
              '        template<class T> T mf(T x, T y) { return x + y; }\n'
              '        template<class T> T callNf(T x) { return nf(x); }\n'
              '    } // namespace mid\n'
              '    template<class T> T callMf(T x) { return mid::mf(x); }\n'
              '} // namespace outer\n'
              'inline int useNest() { return outer::mid::callNf(1) + outer::callMf(1); }\n'
              'namespace tk_detail {\n'
              '    template<int K, class T> T tf(T x) { return x + K; }\n'
              '    template<int K, class T> T tf(T x, T y) { return x + y + K; }\n'
              '} // namespace tk_detail\n'
              'template<class T> T callTk(T x) { return tk_detail::template tf<3>(x); }\n'
              'inline int useTk() { return callTk(2); }\n'
              'namespace r_detail {\n'
              '    template<int J> int digit(int x) { return x + J; }\n'
              '    template<class T> T rg(T x);\n'
              '    template<class T> T rg(T x, T y) { return x + y; }\n'
              '    template<int K> int sum(int x);\n'
              '} // namespace r_detail\n'
              'template<class T> T r_detail::rg(T x) { return x * 7; }\n'
              'template<int K> int r_detail::sum(int x) { return digit<K>(x); }\n'
              'template<class T> T callRg(T x) { return r_detail::rg(x); }\n'
              'inline int useRg() { return callRg(2); }\n'
              'inline int useSum() { using namespace r_detail; return sum<3>(4); }\n')
LOOKUP2_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\n'
              'namespace ov_detail {\n'
              '    struct Heavy { int v = 42; int unusedHeavy() const { return 1; } };\n'
              '    template<class T> T twice(T x);\n'
              '} // namespace ov_detail\n'
              'template<class T> T ov_detail::twice(T x) { return x * Heavy{}.v; }\n'
              'inline int twice(int x) { return 2 * x; }\n'
              'namespace cc_detail {\n'
              '    template<class T> T step(T x) { return x * 3; }\n'
              '} // namespace cc_detail\n'
              'namespace c1 = cc_detail;\n'
              'namespace c2 = c1;\n'
              'template<class T> T useC(T x) { return c2::step(x); }\n'
              'inline int runC() { return useC(2); }\n'
              'template<class T> T useD(T x) { using namespace c1; return step(x); }\n'
              'inline int runD() { return useD(2); }\n'
              'namespace ao_detail {\n'
              '    namespace deep {\n'
              '        struct Far { int v = 5; };\n'
              '    } // namespace deep\n'
              '    template<class T> T far(T x) { return x + 900; }\n'
              '} // namespace ao_detail\n'
              'template<class T> struct W {\n'
              '    T far(T x) const { return x; }\n'
              '    T go(T x) const { return far(x); }\n'
              '};\n'
              'inline int runFar() { ao_detail::deep::Far f; return W<int>{}.go(f.v); }\n'
              'namespace fd_detail {\n'
              '    struct Tok { int v; friend int peek(Tok t); };\n'
              '    inline int peek(Tok t);\n'
              '} // namespace fd_detail\n'
              'inline int usePeek() { return fd_detail::peek(fd_detail::Tok{6}); }\n'
              'inline int fd_detail::peek(Tok t) { return t.v + 1; }\n'
              'namespace es_detail {\n'
              '    template<class T> int f(T) { return 1; }\n'
              '} // namespace es_detail\n'
              'template<> inline int es_detail::f<int>(int) { return 2; }\n'
              'inline int useEs() { return es_detail::f(5) * 10 + es_detail::f(1.5); }\n'
              'template<class T> int callEs(T x) { return es_detail::f(x); }\n'
              'inline int useEs2() { return callEs(5); }\n')
SPEC_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\n'
            'struct Sel { template<int K> int get() const { return K; } };\n'
            'template<> inline int Sel::get<1>() const { return 100; }\n'
            'template<class T> struct Bx { int f() const { return 1; } };\n'
            'template<> inline int Bx<int>::f() const { return 2; }\n'
            'inline int f(int) { return 0; }\n')
ALIAS_A_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\n'
              'namespace aa_detail {\n'
              '    template<class T> T pick(T x) { return x + 1; }\n'
              '} // namespace aa_detail\n'
              'namespace h = aa_detail;\n'
              'template<class T> T useA(T x) { return h::pick(x); }\n'
              'inline int runA() { return useA(1); }\n')
ALIAS_B_HPP = ('#pragma once\n#include "../01-Core/01-template.hpp"\n\n'
              'namespace bb_detail {\n'
              '    template<class T> T pick(T x) { return x + 2; }\n'
              '} // namespace bb_detail\n'
              'namespace h = bb_detail;\n')
GR_LINE = 'int operator[](int k) const { return at[k]; }'
NET_LINES = ['void link(int v) { adj[n++] = v; }', 'int unusedNet() const { return 1; }', 'int operator[](int v) const { return adj[v]; }']


class Extras(Base):
    """Aliases, unparsed-branch references, --into placement, bundles (aggregates, directives between includes),
    dependent lookup across kept headers, harness targets and probes, and the CLI's index error, on a fixture of their own."""
    W = '02-Data Structures/05-wrap.hpp'
    MODE, Y, FOLD = '02-Data Structures/06-mode.hpp', '02-Data Structures/07-ytype.hpp', '02-Data Structures/08-folder.hpp'
    NET, NET_USE = '02-Data Structures/09-net.hpp', '02-Data Structures/10-netuse.hpp'
    REACH, REACH_USE = '02-Data Structures/11-reach.hpp', '02-Data Structures/12-reachuse.hpp'
    LOOKUP = '02-Data Structures/13-lookup.hpp'
    LOOKUP2, ALIAS_A, ALIAS_B = '02-Data Structures/14-lookup2.hpp', '02-Data Structures/15-alias_a.hpp', '02-Data Structures/16-alias_b.hpp'
    SPEC = '02-Data Structures/17-spec.hpp'

    def setUp(self):
        super().setUp()
        if not HAVE_CLANG:
            self.skipTest('SKIP libclang: python3 cannot import clang.cindex')
        self.root = write_tree(self.tmp / 'extras', {T: FILES[T] + 'using ulng = uint64_t;\n', D: FILES[D], self.W: WRAP,
                                                     '02-Data Structures/98-basic.hpp': '#pragma once\n#include "05-wrap.hpp"\n',
                                                     self.MODE: MODE_HPP, self.Y: Y_HPP, self.FOLD: FOLD_HPP, H.MONOIDS: MONOIDS_HPP,
                                                     self.NET: NET_HPP, self.NET_USE: NET_USE_HPP,
                                                     self.REACH: REACH_HPP, self.REACH_USE: REACH_USE_HPP, self.LOOKUP: LOOKUP_HPP,
                                                     self.LOOKUP2: LOOKUP2_HPP, self.ALIAS_A: ALIAS_A_HPP, self.ALIAS_B: ALIAS_B_HPP,
                                                     self.SPEC: SPEC_HPP})
        self.ix = built(self.root)

    def run_with(self, out, main, want, flags=()):
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(self.root) + out + main, cxx, flags), want, cxx=cxx, out=out)

    def test_alias_member_and_whole(self):
        self.eq([self.ix.units[i]['qual'] for i in self.ix.resolve('Wrap::sqrt')], ['wrap_detail::Value::sqrt'])
        self.eq([self.ix.units[i]['qual'] for i in self.ix.resolve('wrap7::pow')], ['wrap_detail::Value::pow'], what='through two aliases')
        self.eq({self.ix.units[i]['name'] for i in H.targets(self.ix, self.W, ['wrap7'], 'ctor')}, {'Value<K>'}, what="libclang spells a class template's constructor Value<K>")
        out = G.grab(['Wrap::sqrt'], self.root, self.ix)
        self.true('int sqrt() const { return v + K; }' in out and 'pow()' not in out, out=out)
        self.run_with(out, 'int main() { printf("%d\\n", Wrap<7>(3).sqrt()); }\n', '10\n')
        out = G.grab(['wrap7'], self.root, self.ix)
        self.true('pow()' in out and 'sqrt()' in out, what='an alias root keeps the type it names whole', out=out)
        self.run_with(out, 'int main() { printf("%d %d\\n", wrap7(3).pow(), wrap7(3).sqrt()); }\n', '21 10\n')

    def test_unparsed_branch_refs(self):
        out = G.grab(['pick'], self.root, self.ix)
        self.true('inline constexpr int POISONW = 5;' in out and 'inline int pick() { return POISONW + 1; }' in out, out=out)
        self.run_with(out, 'int main() { printf("%d\\n", pick()); }\n', '6\n', ['-DWRAP_EXTRA'])
        self.run_with(out, 'int main() { printf("%d\\n", pick()); }\n', '1\n')

    def test_into_above_helper(self):
        f = self.tmp / 'helper.cpp'
        sol = ('#include <cstdio>\nint plain(int x) { return x; }\n\n// counts\nint comps(int n) {\n    Base b;\n'
               '    return b.get2() + plain(n);\n}\n\nvoid solve(int t) { printf("%d\\n", comps(t)); }\nint main() { solve(1); }\n')
        f.write_text(sol)
        self.eq(G.into(f, self.root, self.ix), ['Base'])
        lines = f.read_text().split('\n')
        self.eq(lines.index('// grab begin: Base'), 3, what='above the comment of the first declaration using it', text=f.read_text())
        self.eq(lines[lines.index('// grab end') + 1], '// counts')
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(self.root) + f.read_text(), cxx), '3\n', cxx=cxx)
        self.eq(G.into(f, self.root, self.ix), ['Base'])
        self.eq(f.read_text().split('\n'), lines, what='rerun is identical')

    def test_bundle_aggregate(self):
        src = self.root / '97-Online Testing/src/j/agg.cpp'
        write_tree(self.root, {src.relative_to(self.root).as_posix():
                               '#include "../../../02-Data Structures/98-basic.hpp"\nint main() { printf("%d\\n", Base().get2()); }\n'})
        out = G.bundle(src, self.root, self.ix)
        self.true('#include "' not in out and 'struct Base {' in out and 'twiceK' not in out, out=out)
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, out, cxx), '2\n', cxx=cxx, out=out)

    def test_ops_instantiation_oracle(self):
        unit = lambda n: self.ix.resolve(n)[0]
        self.eq(H.probes(self.ix, self.ix.resolve('Wrap::sqrt')), ['template struct wrap_detail::Value<4>;'])
        self.eq(H.probes(self.ix, self.ix.resolve('twiceK') + self.ix.resolve('Base') + self.ix.resolve('pick')), ['&twiceK<>'])
        self.eq([H.candidates(self.ix, unit(n)) for n in ('Box', 'Folder', 'twiceK')],
                [['template struct Box<lng>;', 'template struct Box<Sum<lng>>;', 'template struct Box<ulng>;'],
                 ['template struct Folder<lng>;', 'template struct Folder<Sum<lng>>;', 'template struct Folder<ulng>;'], ['&twiceK<>']])
        cxx = self.compilers()[:1]
        work = self.tmp / 'oracle'
        work.mkdir()
        with patch.object(H, 'ROOT', self.root):
            wanted = {(self.ix.units[unit(n)]['header'], unit(n)): H.candidates(self.ix, unit(n)) for n in ('wrap_detail::Value', 'twiceK', 'useValue', 'Box', 'UOnly', 'Folder')}
            chosen = {self.ix.units[k]['qual']: p for (h, k), p in H.control(cxx, wanted, work, 2).items()}
            self.eq(chosen, {'wrap_detail::Value': 'template struct wrap_detail::Value<4>;', 'twiceK': '&twiceK<>', 'useValue': '&useValue<>',
                             'Box': 'template struct Box<lng>;', 'UOnly': 'template struct UOnly<ulng>;', 'Folder': 'template struct Folder<Sum<lng>>;'},
                    what='the first candidate that compiles after the whole header, over rounds')
            fold = ['template struct Folder<Sum<lng>>;']
            self.eq([self.ix.units[i]['qual'] for i in H.preset_roots(self.ix, fold)], ['Sum'])
            src = work / 'fold.cpp'
            src.write_text(H.PRELUDE.format(root=self.root) + G.render(self.ix, G.select(self.ix, self.ix.resolve('Folder') + H.preset_roots(self.ix, fold))) + H.probe_text(fold))
            self.eq(H.compile_check(cxx[0], str(src)), None, what='the extraction carries the preset its probe names')
            index = '| Header | Operations | Status |\n|---|---|---|\n| `08-folder.hpp` | Folder: fold | verified |\n| `05-wrap.hpp` | Base: ==, get2; Derived: get2; Kind: Odd | verified |\n'
            write_tree(self.root, {'02-Data Structures/00-index.md': index})
            out = io.StringIO()
            with patch.object(H, 'final', return_value=set()), patch.object(H, 'core_full', return_value=set()), contextlib.redirect_stdout(out):
                failures = H.check_ops(self.ix, argparse.Namespace(filter='', mode='full', seed=1, sample=8, jobs=2, verbose=True), work)
            self.true(failures == 0 and 'ops: 2 rows, 5 operations' in out.getvalue() and '(1 instantiating' in out.getvalue()
                      and 'unresolved' not in out.getvalue(), out=out.getvalue())
            dropped = G.grab(['useValue'], self.root, self.ix).replace('        int pow() const { return v * K; }\n', '')
            src = work / 'dropped.cpp'
            src.write_text(H.PRELUDE.format(root=self.root) + dropped)
            self.eq(H.compile_check(cxx[0], str(src)), None, what='syntax-only misses a dropped member')
            src.write_text(H.PRELUDE.format(root=self.root) + dropped + H.probe_text(['&useValue<>']))
            self.true(H.compile_check(cxx[0], str(src)) is not None, what='the probe instantiates it')
            src.write_text(H.PRELUDE.format(root=self.root) + G.grab(['useValue'], self.root, self.ix) + H.probe_text(['&useValue<>']))
            self.eq(H.compile_check(cxx[0], str(src)), None, what='the real extraction passes the probe')

    def test_targets_nested_enum_bases_operators(self):
        q = lambda r: None if r is None else sorted(self.ix.units[i]['qual'] for i in r)
        t = lambda types, op: q(H.targets(self.ix, self.W, types, op))
        self.eq([t(['Base::Inner'], 'twice'), t(['Base::Inner'], 'ctor'), t(['Base::Inner'], 'thrice')], [['Base::Inner'], ['Base::Inner'], None],
                what='a nested struct is kept whole and stands in for what it spells')
        self.eq([t(['Kind'], 'Odd'), t(['Kind'], 'Even')], [['Kind'], None], what='an enumerator')
        self.eq([t(['Derived'], 'get2'), t(['Base'], 'operator=='), t(['Base'], 'operator!=')], [['Base::get2'], ['Base::operator=='], None],
                what='a base member; an operator')

    def test_into_user_namespace(self):
        f = self.tmp / 'ns.cpp'
        sol = ('#include <cstdio>\nnamespace sol {\nint f() { Base b; return b.get2(); }\n} // namespace sol\n\n'
               'void solve(int t) { printf("%d\\n", sol::f() + t); }\nint main() { solve(1); }\n')
        f.write_text(sol)
        self.eq(G.into(f, self.root, self.ix), ['Base'])
        lines = f.read_text().split('\n')
        self.eq((lines.index('// grab begin: Base'), lines[lines.index('// grab end') + 1]), (1, 'namespace sol {'), what='above the namespace, never inside it')
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(self.root) + f.read_text(), cxx), '3\n', cxx=cxx)

    def test_bundle_directives_between_includes(self):
        src = self.root / '97-Online Testing/src/j/mode.cpp'
        inc = '../../../02-Data Structures/'
        text = (f'#include "../../../01-Core/01-template.hpp"\n#define B_FAST\n#include "{inc}06-mode.hpp"\n#include "{inc}07-ytype.hpp"\n'
                f'#include "{inc}05-wrap.hpp"\nint main() {{ printf("%d %d\\n", mode(), callG(YT{{}})); }}\n')
        write_tree(self.root, {src.relative_to(self.root).as_posix(): text})
        out = G.bundle(src, self.root, self.ix)
        lines = out.split('\n')
        self.true(lines.index('#define B_FAST') < lines.index('inline int mode() {') and 'inline int g(YT t) { return t.y; }' in lines, out=out)
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, out, cxx), '1 5\n', cxx=cxx, out=out)
            self.eq(build_run(self.tmp, text, cxx, [f'-I{src.parent}']), '1 5\n', cxx=cxx, what='the source itself agrees')
        write_tree(self.root, {src.relative_to(self.root).as_posix(): f'#include "../../../01-Core/01-template.hpp"\n#ifdef LOCAL\n#include "{inc}06-mode.hpp"\n#endif\nint main() {{}}\n'})
        with self.assertRaises(G.GrabError) as e:
            G.bundle(src, self.root, self.ix)
        self.eq(str(e.exception), f'97-Online Testing/src/j/mode.cpp:3: {inc}06-mode.hpp is included under #if (use --bundler oj-bundle)')

    def test_dependent_lookup_across_kept_headers(self):
        out = G.grab(['callG', 'YT'], self.root, self.ix)
        self.true('inline int g(YT t) { return t.y; }' in out, what='g is found by ADL in a header callG does not include', out=out)
        self.run_with(out, 'int main() { printf("%d\\n", callG(YT{})); }\n', '5\n')

    def test_input_types(self):
        q = lambda n, only=False: [self.ix.units[t]['qual'] for i in self.ix.resolve(n) for t in G.inputs(self.ix, i, only)]
        self.eq([q('netSize'), q('netSize', True), q('netFirst', True), q('netLast', True), q('derivedSum', True), q('seqLen'), q('foldM'),
                 q('net_detail::peek'), q('Net')], [['Net'], [], ['Net'], ['Gr'], ['DerivedG'], [], [], [], []],
                what='default; inferred (NetLike lacks operator[]); own header before its includes; base members; sequence, monoid, detail and non-template none')
        main = ('int main() { Net g; g.link(3); g.link(4); printf("%d %d %d %d %d\\n", netSize(g), netFirst(g), netLast(g), Walker(g).s, '
                'derivedSum(DerivedG())); }\n')
        for names in (['netSize', 'netFirst', 'Walker'], ['netSize'], ['netFirst'], ['Walker']):
            out = G.grab(names, self.root, self.ix)
            lines = stripped(out)
            self.true(all(l in lines for l in NET_LINES) and 'struct NetLike { int n = 0; };' not in lines and GR_LINE not in lines, names=names, out=out,
                      what='a whole template keeps its input type whole, and only the type its members need')
        out = G.grab(['netLast'], self.root, self.ix)
        self.true(GR_LINE in out and NET_LINES[1] not in out, out=out, what='netLast brings Gr')
        self.run_with(G.grab(['netSize', 'netFirst', 'netLast', 'Walker', 'derivedSum'], self.root, self.ix), main, '2 5 4 2 3\n')
        pruned = G.render(self.ix, G.select(self.ix, self.ix.resolve('netSize'), set()))
        self.true('int unusedNet() const { return 1; }' not in pruned and 'struct Net {' in pruned, out=pruned, what='a pruned root keeps its input pruned')
        self.eq(sorted(H.undefaulted(self.ix, [self.NET, self.NET_USE])),
                [(self.NET, 'netFirst', ['Net']), (self.NET_USE, 'derivedSum', ['DerivedG']), (self.NET_USE, 'netLast', ['Gr'])])

    def test_no_over_reach(self):
        out = G.grab(['netStat'], self.root, self.ix)
        self.true('Holder' not in out, out=out, what='an out-of-line member comes with its class only, never with a type it takes')
        for names, gone, main, want in (
                (['useCtx'], 'a * b + 1000', 'int main() { printf("%d\\n", useCtx(3)); }\n', '6\n'),
                (['useKey'], 'x + 1000', 'int main() { printf("%d\\n", useKey(3)); }\n', '4\n'),
                (['useBox', 'viaUsing'], None, 'int main() { printf("%d %d\\n", useBox(), viaUsing(4)); }\n', '9 8\n')):
            out = G.grab(names, self.root, self.ix)
            if gone:
                self.true(gone not in out, names=names, out=out,
                          what="C::mul through a template parameter and an unqualified call in one detail namespace never keep another namespace's function")
            else:
                self.true('inline int open(Box b)' in out and 'T twice(T x) { return 2 * x; }' in out, names=names, out=out,
                          what='argument-dependent lookup and a using-directive still reach a detail namespace')
            self.run_with(out, main, want)

    def test_lookup_rules(self):
        for name, want in (('useColor', 8), ('useG', 4), ('useAl', 10), ('useGf', 7), ('useUf', 6), ('useUf2', -1), ('useA', 14),
                           ('useB', 24), ('useOther', 4), ('useNest', 302), ('useTk', 5), ('useRg', 14), ('useSum', 7)):
            out = G.grab([name], self.root, self.ix)
            self.run_with(out, f'int main() {{ printf("%d\\n", {name}()); }}\n', f'{want}\n')
            if name == 'useA':
                self.true('unused' not in out, out=out, what='an out-of-line member comes only when its in-class declaration does')
            if name == 'useOther':
                self.true('take' not in out, out=out, what='an out-of-line member never comes with a type it takes')
        for name, want in (('runA', 2), ('runC', 6), ('runD', 6), ('runFar', 5), ('usePeek', 7), ('useEs', 21), ('useEs2', 2)):
            out = G.grab([name], self.root, self.ix)
            self.run_with(out, f'int main() {{ printf("%d\\n", {name}()); }}\n', f'{want}\n')
            if name == 'runFar':
                self.true('x + 900' not in out, out=out, what='argument-dependent lookup looks only in a type\'s innermost namespace')
        self.eq([self.ix.units[i]['header'] for i in self.ix.resolve('twice') if self.ix.units[i]['header'] == self.LOOKUP2], [self.LOOKUP2],
                what='an out-of-line namespace member is no command-line name of its own')
        out = G.grab_text('int main() { printf("%d\\n", twice(3)); }\n', self.root, self.ix)[0]
        self.true('Heavy' not in out and 'inline int twice(int x)' in out, out=out,
                  what='a global function and an out-of-line namespace member of one name are no overload set')
        text = 'struct Edge {\n    int u;\n    operator YT() const { return YT{u}; }\n};\nvoid solve() { Edge e{1}; printf("%d\\n", int(sizeof e) + YT(e).y); }\nint main() { solve(); }\n'
        self.eq(G.lines_of(self.ix, text)[1], ['YT'], what='a conversion operator or sizeof naming a library type declares nothing')
        self.eq(G.lines_of(self.ix, 'struct S { int twice() const { return 2; } };\nint main() { S s; printf("%d\\n", s.twice()); }\n')[1], [],
                what='a name spelled only in member access roots no free function')
        main = 'int main() { Sel s; Bx<int> b; printf("%d %d\\n", s.get<1>(), b.f()); }\n'
        for out in (G.grab(['Sel', 'Bx'], self.root, self.ix), G.grab_text(main, self.root, self.ix)[0]):
            self.run_with(out, main, '100 2\n')
        self.true('Bx<int>::f' not in G.grab(['f'], self.root, self.ix), what="a class member's explicit specialization pairs with no free function")
        text = 'void solve() { auto g = [&](int x) -> YT { return YT{x}; }; printf("%d\\n", g(3).y); }\nint main() { solve(); }\n'
        self.eq(G.lines_of(self.ix, text)[1], ['YT'], what='a trailing return type is no member access')
        self.eq(H.beyond(self.ix, self.NET, self.ix.resolve('netStat')), [])
        self.eq(H.beyond(self.ix, self.NET, self.ix.resolve('useCtx')), [self.REACH_USE], what='code outside the include closure is reported')

    def test_using_declaration_members(self):
        out = G.grab(['UsesBase::twiceAdd'], self.root, self.ix)
        self.true('T sub(T x) const { return v - x; }' in out and 'unusedOps' not in out, out=out,
                  what='a kept using-declaration keeps the base members it names')
        self.run_with(out, 'template struct UsesBase<lng>;\nint main() { printf("%lld\\n", (long long)UsesBase<lng>().twiceAdd(3)); }\n', '6\n')

    def test_into_called_members_and_own_types(self):
        f = self.tmp / 'stat.cpp'
        for prune in (False, True):
            f.write_text('void solve(int t) { auto s = netStat(t); printf("%d\\n", s.twice()); }\nint main() { solve(3); }\n')
            self.eq(G.into(f, self.root, self.ix, prune), ['netStat'], prune=prune)
            text = f.read_text()
            self.true('int twice() const { return 2 * a; }' in text and 'unusedStat' not in text, text=text, prune=prune,
                      what='a member called on a struct the file never names stays')
            self.run_with(text, '', '6\n')
        f.write_text('struct Net { int n = 4; };\nvoid solve(int t) { printf("%d\\n", netSize(Net{}) + t); }\nint main() { solve(1); }\n')
        self.eq(G.into(f, self.root, self.ix), ['netSize'])
        text = f.read_text()
        self.true(not any(l in text for l in NET_LINES), text=text, what="no type the file declares itself, input or reference")
        self.run_with(text, '', '5\n')

    def test_cli_index_error(self):
        err = io.StringIO()
        with patch.object(G, 'Index', side_effect=G.GrabError('x.hpp: libclang cannot parse it: e')), contextlib.redirect_stderr(err):
            self.eq(G.main(['helperA', '--no-copy']), 1)
        self.eq(err.getvalue(), 'grab: x.hpp: libclang cannot parse it: e\n')


class Fallback(FixtureBase):
    def setUp(self):
        super().setUp()
        p = patch.object(G, 'cindex', return_value=None)
        p.start()
        self.addCleanup(p.stop)
        self.ixn = G.Index(self.fx, None)

    def whole(self, *headers):
        out = []
        for h in headers:
            out += [o_expected_line(l) for l in FILES[h].split('\n')]
        return '\n'.join(out).rstrip('\n') + '\n'

    def test_fallback_whole(self):
        self.eq((self.ixn.ci, self.ixn.records), (None, {}))
        self.eq(G.grab(['helperA'], self.fx, self.ixn), self.whole(A))
        self.eq(G.grab(['Point::norm'], self.fx, self.ixn), self.whole(A), what='last component of a qualified name')
        out = G.grab(['betaUse'], self.fx, self.ixn)
        self.eq(out, self.whole(A, B))
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(self.fx) + out + 'int main() { printf("%d\\n", betaUse(3)); }\n', cxx), '8\n', cxx=cxx)
        self.eq(G.grab(['lonely'], self.fx, self.ixn, True), f'#line 1 "{U}"\n' + self.whole(U))
        with self.assertRaises(G.GrabError) as e:
            G.grab(['nosuch'], self.fx, self.ixn)
        self.eq(str(e.exception), 'no header defines nosuch')
        self.eq(G.grab_text('void solve() { lonely(1); }', self.fx, self.ixn), (self.whole(U), [U]))

    def test_fallback_namespace_scope_names(self):
        text = 'void solve(int t) { Point p(1, 2); int size = p.size(); printf("%d", size + t); }'
        self.eq(G.grab_text(text, self.fx, self.ixn), (self.whole(A), [A]), what='members, keywords and own names match no header')

    def test_fallback_bundle(self):
        src = self.fx / '97-Online Testing/src/j/fb.cpp'
        src.parent.mkdir(parents=True, exist_ok=True)
        src.write_text(Bundle.SRC)
        self.addCleanup(src.unlink)
        out = G.bundle(src, self.fx, self.ixn)
        self.eq(o_sections(out), [T, D, A, B, '97-Online Testing/src/j/fb.cpp'])
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, out, cxx), '8 11\n', cxx=cxx, out=out)

    def test_fallback_cli(self):
        out, err = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err), patch.object(G, 'Index', lambda *a, **k: self.ixn), \
                patch.object(G, 'ROOT', self.fx):
            codes = [G.main(['--no-copy', 'helperA']), G.main(['--find', 'odd'])]
        lines = err.getvalue().splitlines()
        self.eq((codes, [l.split(':')[0] for l in lines], out.getvalue()), ([0, 0], ['SKIP libclang'], self.whole(A) + 'no match\n'),
                what='--find needs no libclang')


class Cache(Base):
    def setUp(self):
        super().setUp()
        if not HAVE_CLANG:
            self.skipTest('SKIP libclang: python3 cannot import clang.cindex')
        self.fx = write_tree(self.tmp / 'lib', FILES)
        self.cache = self.tmp / 'cache'

    def index(self, **kw):
        with patch.object(G, "index_header", wraps=G.index_header) as spy:
            err = io.StringIO()
            with contextlib.redirect_stderr(err):
                ix = G.Index(self.fx, self.cache, jobs=1, **kw)
        return ix, sorted(c.args[1] for c in spy.call_args_list), err.getvalue()

    def records(self):
        return {h: G.index_header(self.fx, h) for h in (A, B, U)}

    def test_cache_build_and_reuse(self):
        ix, built_, err = self.index()
        self.eq((built_, err), (sorted([A, B, U]), 'grab: indexing 3 header(s) with libclang\n'))
        want = plain(self.records())
        self.eq(plain(ix.records), want, what='fresh records equal build()')
        self.eq(sorted(json.loads(f.read_text())['record']['header'] for f in self.cache.glob('*.json')), sorted([A, B, U]))
        ix, built_, err = self.index()
        self.eq((built_, err, plain(ix.records), ix.stale), ([], '', want, []))
        self.eq([ix.units[i]['qual'] for i in ix.resolve('Point::isOdd')], ['Point::isOdd'], what='the reused index resolves')

    def test_cache_edit_rebuilds_closure(self):
        self.index()
        (self.fx / A).write_text(ALPHA + 'inline int addedLater(int x) { return x; }\n')
        ix, built_, _ = self.index()
        self.eq(built_, sorted([A, B]), what='alpha and beta, whose closure holds alpha')
        self.true(ix.resolve('addedLater'), what='edited header reindexed')
        (self.fx / D).write_text(DEBUG + '// edited\n')
        self.eq(self.index()[1], [B], what='the excluded debug header is in beta\'s closure only')

    def test_cache_key_inputs_and_system_stamps(self):
        graph = G.library(self.fx)
        self.addCleanup(G._ANALYSIS.clear)
        G._ANALYSIS.clear()
        base = G.key(self.fx, graph, A)
        for name, value in (('PROTOCOL', G.PROTOCOL | {'zz'}), ('EXCLUDED', G.EXCLUDED + ('x.hpp',)), ('CONFIGS', {'scalar': []}),
                            ('DIRECTIVE', re.compile(r'\s*#\s*(if)\b')), ('SYSTEM_INCLUDE', re.compile(r'\s*#\s*import\b'))):
            G._ANALYSIS.clear()
            with patch.object(G, name, value):
                self.true(G.key(self.fx, graph, A) != base, name=name, what='an analysis input changes the key')
        G._ANALYSIS.clear()
        self.eq(G.key(self.fx, graph, A), base)
        ix, _, _ = self.index()
        system = ix.records[U]['system']
        self.true(system and all(not f.startswith(str(self.fx)) and v == G.stamp(f) for f, v in system.items()), system=sorted(system)[:5])
        self.true({'cstdio', 'functional'} <= {Path(f).name for f in system}, what='the template\'s system headers are stamped')
        f = next(f for f in self.cache.glob('*.json') if json.loads(f.read_text())['record']['header'] == U)
        data = json.loads(f.read_text())
        data['record']['system'][next(iter(system))] = [0, 0]
        f.write_text(json.dumps(data))
        self.eq(self.index()[1], [U], what='a changed system header rebuilds')
        data = json.loads(f.read_text())
        del data['record']['system']
        f.write_text(json.dumps(data))
        self.eq(self.index()[1], [U], what='a record without stamps rebuilds')

    def test_cache_corrupt_file(self):
        self.index()
        bad = [f for f in self.cache.glob('*.json') if json.loads(f.read_text())['record']['header'] == U]
        self.eq(len(bad), 1)
        bad[0].write_text('{not json')
        ix, built_, _ = self.index()
        self.eq((built_, plain(ix.records[U])), ([U], plain(G.index_header(self.fx, U))))
        bad[0].write_text(json.dumps({'key': 'other', 'record': {}}))
        self.eq(self.index()[1], [U], what='a key mismatch rebuilds')

    def test_cache_stale_without_build(self):
        ix, built_, err = self.index(build=False)
        self.eq((sorted(ix.stale), built_, ix.records, err, list(self.cache.glob('*'))), (sorted([A, B, U]), [], {}, '', []))
        self.index()
        (self.fx / B).write_text(BETA + '\n')
        ix, built_, _ = self.index(build=False)
        self.eq((ix.stale, built_, sorted(ix.records)), ([B], [], sorted([A, U])))

    def test_cache_parallel_jobs(self):
        real, pools = concurrent.futures.ProcessPoolExecutor, []

        def spy(*a, **kw):
            pools.append(kw.get('mp_context') or a[1:2])
            return real(*a, **kw)
        for registered in (False, True):
            shutil.rmtree(self.cache, ignore_errors=True)
            pools.clear()
            with patch.object(concurrent.futures, 'ProcessPoolExecutor', spy), \
                    patch.dict(sys.modules, {G.__name__: G} if registered else {}):
                ix = G.Index(self.fx, self.cache, jobs=2, quiet=True)
            self.eq(plain(ix.records), plain(self.records()), registered=registered)
            self.eq(len(pools), int(registered), registered=registered,
                    what='forked workers only when the module is registered in sys.modules, as the expander and harness do')


class Harness(Base):
    def test_harness_split(self):
        self.eq(H.split('a(b, c), d , ,e', ','), ['a(b, c)', 'd', 'e'])
        self.eq(H.split('A<B, C>, D(x, y), E', ',', True), ['A<B, C>', 'D(x, y)', 'E'])
        self.eq(H.split('A<B, C>, D', ','), ['A<B', 'C>', 'D'])
        self.eq(H.split('x (a; b); y', ';'), ['x (a; b)', 'y'])
        self.eq(H.split('', ','), [])
        self.eq(H.split('H (ids [0, n); B = H<2>): build, push; R', ';'), ['H (ids [0, n); B = H<2>): build, push', 'R'])
        self.eq(H.split('f [l, r), g', ','), ['f [l, r)', 'g'])
        self.eq(H.split('operator<, <<=, a<b, c>, x < y, z', ',', True), ['operator<', '<<=', 'a<b, c>', 'x < y', 'z'],
                what='template brackets only after a name other than operator')

    def test_harness_operations(self):
        cases = {
            'A, B: x, y; z': [(['A', 'B'], 'x'), (['A', 'B'], 'y'), ([], 'z')],
            'SparseTable (M with S and op, no e): prod (idempotent), associativeProd (associative), operator<<; SparseTable2D: prod (rect)':
                [(['SparseTable'], 'prod'), (['SparseTable'], 'associativeProd'), (['SparseTable'], 'operator<<'), (['SparseTable2D'], 'prod')],
            'ModInt<MOD>, mint (alias ModInt<9>): pow, inv': [(['ModInt', 'mint'], 'pow'), (['ModInt', 'mint'], 'inv')],
            'Mo: solve (add, del; ret), f': [(['Mo'], 'solve'), (['Mo'], 'f')],
            'Fast: mul; internals: kernel, red': [(['Fast'], 'mul'), ([], 'kernel'), ([], 'red')],
            'gcd, lcm (any sign), x << y': [([], 'gcd'), ([], 'lcm'), ([], 'x << y')],
            'Barrett32::Multiplier, Barrett64::Multiplier (Shoup): ctor, mul': [(['Barrett32::Multiplier', 'Barrett64::Multiplier'], 'ctor'),
                                                                                (['Barrett32::Multiplier', 'Barrett64::Multiplier'], 'mul')],
            'f(a: b), g': [([], 'f'), ([], 'g')],
            'P<T>: explicit bool cast, +=': [(['P'], 'explicit bool cast'), (['P'], 'operator+=')],
            'R: operator() (pointer, nullptr), \\|=, - (binary, unary), ==, [l, r) sum': [(['R'], 'operator()'), (['R'], 'operator|='), (['R'], 'operator-'),
                                                                               (['R'], 'operator=='), (['R'], '[l, r) sum')],
            'StaticMatrix<T, N, M>: x; W<A, B>, V: y': [(['StaticMatrix'], 'x'), (['W', 'V'], 'y')],
            'B: <<=, >>=, combineShift<Op> (fused), operator<, a': [(['B'], 'operator<<='), (['B'], 'operator>>='), (['B'], 'combineShift<Op>'),
                                                                 (['B'], 'operator<'), (['B'], 'a')],
            'DAryHeap (ids [0, n); BinaryHeap = DAryHeap<2>): build, push; IntervalMap ([l, r) keys): assign':
                [(['DAryHeap'], 'build'), (['DAryHeap'], 'push'), (['IntervalMap'], 'assign')],
        }
        for cell, want in cases.items():
            got = [p for p in H.operations(cell) if p != ([], 'internals')]
            self.eq(got, want, cell=cell)

    def index_tree(self):
        return write_tree(self.tmp / 'lib', {
            T: '#pragma once\n', D: '#pragma once\n', '01-Core/03-fast.hpp': '#pragma once\n', '01-Core/04-pipe.hpp': '', '02-Data Structures/01-a.hpp': '',
            '02-Data Structures/03-p.hpp': '',
            '01-Core/00-index.md': '| Header | Mini | Operations | Status |\n|---|---|---|---|\n| `01-template.hpp` | no | aliases | verified |\n'
                                   '| `03-fast.hpp` | no | Fast: mul | verified; [evidence](Docs/03-fast.md) |\n'
                                   '| `04-pipe.hpp` | no | a \\| b, c | existing-unverified |\n',
            '02-Data Structures/00-index.md': '# Data\n\n| Header | Operations | Status |\n|---|---|---|\n| `01-a.hpp` | x, y | verified |\n'
                                              '| `02-missing.hpp` | z | verified |\n| `03-p.hpp` | w | planned |\n| not a row | q | verified |\n'})

    def test_harness_rows(self):
        root = self.index_tree()
        self.eq(H.rows(root), [('01-Core/03-fast.hpp', 'Fast: mul'), ('01-Core/04-pipe.hpp', 'a \\| b, c'), ('02-Data Structures/01-a.hpp', 'x, y')])
        self.eq([s for _, s in H.rows(root, True)], ['verified; [evidence](Docs/03-fast.md)', 'existing-unverified', 'verified'])

    def test_harness_final(self):
        root = self.index_tree()
        plan = root / '00-Guidelines/13-Plan'
        plan.mkdir(parents=True)
        shutil.copy2(ROOT / '00-Guidelines/13-Plan/plan.py', plan / 'plan.py')
        a, f, p = '02-Data Structures/01-a.hpp', '01-Core/03-fast.hpp', '01-Core/04-pipe.hpp'
        (plan / 'batches.json').write_text(json.dumps({'batches': [
            {'id': 'B1', 'focus': 'one', 'targets': [a, f, p]},
            {'id': 'B2', 'focus': 'retrofit (2026-11-05)', 'targets': [], 'support_targets': [f]},
            {'id': 'B3', 'focus': 'retrofit (2026-11-05)', 'targets': [], 'support_targets': [a]}]}))
        (plan / 'packages.json').write_text(json.dumps({'packages': [{'status': 'verified', 'batches': ['B1']},
                                                                     {'status': 'in-progress', 'batches': ['B2']},
                                                                     {'status': 'verified', 'batches': ['B3']}]}))
        self.eq(H.final(root), {a}, what='verified rows of verified packages minus support targets of open 2026-11-05 batches')
        (plan / 'batches.json').write_text((plan / 'batches.json').read_text().replace('retrofit (2026-11-05)', 'retrofit (2026-11-04)'))
        self.eq(H.final(root), {a, f})

    def test_harness_core_full(self):
        root = write_tree(self.tmp / 'lib', {
            T: '#pragma once\n', D: '#ifdef __AVX2__\n#endif\n', '01-Core/03-fast.hpp': '#ifdef __AVX2__\n#endif\n',
            '01-Core/04-fastmini.hpp': '#ifdef __AVX2__\n#endif\n', '01-Core/05-uses.hpp': '#include "01-template.hpp"\n#include "04-fastmini.hpp"\n',
            '01-Core/06-wide.hpp': '#include "03-fast.hpp"\n', '01-Core/07-scalar.hpp': '#include "01-template.hpp"\nint f();\n',
            '01-Core/08-sse.hpp': '#if defined(__SSE4_2__)\n#endif\n', '01-Core/98-basic.hpp': '#include "03-fast.hpp"\n',
            '01-Core/notes.hpp': '#ifdef __AVX2__\n#endif\n', '01-Core/10-grid.hpp': '#include "01-template.hpp"\n',
            '01-Core/11-gridmini.hpp': '#include "01-template.hpp"\n', '01-Core/12-box.hpp': '#include "01-template.hpp"\n',
            '01-Core/00-index.md': '| `12-box.hpp` | yes | Box: x | verified |\n| `13-boxmini.hpp` | yes | BoxMini: x | planned |\n'})
        self.eq(H.core_full(root), {'01-Core/03-fast.hpp', '01-Core/06-wide.hpp', '01-Core/08-sse.hpp', '01-Core/10-grid.hpp', '01-Core/12-box.hpp'},
                what='a full type with a mini file or a planned mini row is full even when scalar')
        real = H.core_full(ROOT)
        self.true('01-Core/09-rational.hpp' not in real and {'01-Core/05-modint.hpp', '01-Core/16-poly.hpp', '01-Core/12-bitmatrix.hpp',
                                                              '01-Core/14-sparsematrix.hpp'} <= real
                  and not any('mini' in h for h in real), real=sorted(real))

    def test_harness_mini_checks(self):
        names = {'ModInt': '05-modint.hpp', 'Poly': '16-poly.hpp'}
        text = ('template<class T = ModInt<7>, typename U = typename Poly<T>::Cell, int K = 3>\nstruct A {};\n'
                'template<class T = mintmini> T f(T x) { return x; } // template<class Z = ModInt<5>> in a comment\n')
        self.eq(H.full_defaults(text, names), ['ModInt', 'Poly'])
        self.eq(H.full_defaults('template<class T = ModIntMini<7>>\nstruct B {};\n', names), [])
        self.eq(H.full_defaults('template<class T = std::conditional_t<true, ModInt<7>, int>, int K = 1>\nstruct C {};\n', names), ['ModInt'],
                what='a wrapped default')
        self.eq(H.mini_first_split([('a.hpp', 'm1'), ('b.hpp', 'm2'), ('a.hpp', 'm3')], {'a.hpp'}), (['m1', 'm3'], ['m2']),
                what='a strict target fails, the others are pending')
        self.eq(H.operation_names('ModIntMini: val, powMini (x [0, n)); internals: ctx', 'partial; missing: sqrt, log'),
                {'val', 'pow', 'ctx', 'sqrt', 'log'})
        self.eq(H.operation_names('A: x; internals:'), {'x'})
        root = write_tree(self.tmp / 'names', {'01-Core/05-full.hpp': 'struct Full {};\ntemplate<class T>\nusing Alias = T;\n'
                                                                       'namespace full_detail {\n    struct Hidden {};\n    enum class Mode { A };\n} // namespace full_detail\n'
                                                                       'template<typename U, class V> struct Pair { struct Inner {}; using W = U; };\n'
                                                                       'namespace std { template<> struct hash<Full> {}; }\nstruct std::less<Full> {};\n'})
        self.eq(H.full_type_names(root, {'01-Core/05-full.hpp'}), {n: '05-full.hpp' for n in ('Full', 'Alias', 'Hidden', 'Mode', 'Pair')},
                what='namespace scope, detail namespaces included; no template parameters, members, specializations or std')

    def test_harness_cpp_testers(self):
        t = self.tmp / 'lib/96-Local Testing'
        write_tree(t, {'02-Data Structures/01-alpha_tester.py': 'print(1)\n', '02-Data Structures/01-alpha_helper.cpp': '',
                       '02-Data Structures/02-beta_tester.py': 'print(2)\n', '02-Data Structures/03-gamma_tester.py': 'x = "#include <x>"\n',
                       '02-Data Structures/04-d_compile_tester.py': '#include\n', '02-Data Structures/04-d.cpp': '',
                       '00-Tools/01-x_tester.py': '#include\n', '08-Python/01-y_tester.py': '#include\n',
                       '05-Mathematics/01-m_tester.py': '', '05-Mathematics/01-m.hpp': '', '05-Mathematics/02-n.cpp': ''})
        got = [(p.relative_to(t).as_posix(), sorted(f.name for f in fs)) for p, fs in H.cpp_testers(self.tmp / 'lib')]
        self.eq(got, [('02-Data Structures/01-alpha_tester.py', ['01-alpha_helper.cpp']), ('02-Data Structures/03-gamma_tester.py', []),
                      ('05-Mathematics/01-m_tester.py', ['01-m.hpp'])])


class HarnessIndex(FixtureBase):
    def test_harness_targets(self):
        ix = self.ix
        q = lambda r: None if r is None else sorted((ix.units[i]['qual'], ix.units[i]['ext'][0]) for i in r)
        pt = self.lineno(A, 'struct Point {')
        self.eq(q(H.targets(ix, A, ['Point'], 'norm')), [('Point::norm', self.lineno(A, POINT_METHODS['norm']))])
        self.eq([H.targets(ix, A, ['Point'], 'nonexistent'), H.targets(ix, A, ['Point'], 'explicit bool cast')], [None, None],
                what='never the group type alone')
        self.eq(q(H.targets(ix, A, ['Point'], 'helperA')), sorted([('helperA', self.lineno(A, HELPER_A[2])), ('helperA', self.lineno(A, HELPER_A[3]))]),
                what='a misgrouped name declared in the header')
        self.eq([n for n, _ in q(H.targets(ix, A, ['Point'], 'Point'))], ['Point::Point'], what='the constructor, a member')
        self.eq(q(H.targets(ix, A, [], 'Point')), [('Point', pt)])
        self.eq({n for n, _ in q(H.targets(ix, A, ['Point'], 'ctor'))}, {'Point::Point'}, what='ctor names the constructors')
        self.eq({n.split('::')[-1] for n, _ in q(H.targets(ix, A, [], 'std::hash'))}, {'hash'}, what='a std specialization')
        self.eq(q(H.targets(ix, A, ['Nope', 'Box'], 'foo')), [('Box::foo', self.lineno(A, 'M foo() const { return v + 1; }'))])
        self.eq(q(H.targets(ix, A, [], 'helperA')), sorted([('helperA', self.lineno(A, HELPER_A[2])), ('helperA', self.lineno(A, HELPER_A[3]))]))
        self.eq([H.targets(ix, A, [], 'betaUse'), H.targets(ix, A, [], 'a b'), H.targets(ix, B, ['Point'], 'norm')], [None, None, None])

    def test_harness_full_only(self):
        found = [(B, 'betaUse, betaDep, lonelyMissing'), (U, 'lonely'), ('01-Core/03-x.hpp', 'betaUse')]
        doc = self.fx / '05-Mathematics/Docs/01-beta.md'
        with patch.object(H, 'ROOT', self.fx):
            self.eq(H.full_only(self.ix, found, {A}), [(B, 'betaUse', [A]), (B, 'betaDep', [A])])
            self.eq(H.full_only(self.ix, found, {T}), [], what='the template is never extracted')
            doc.parent.mkdir(parents=True, exist_ok=True)
            self.addCleanup(doc.unlink)
            doc.write_text('# Beta\n\n- `betaUse` is full-only until `pow`.\n- betaDep full-only until x\n- `betaDep` is fine\n')
            self.eq(H.full_only(self.ix, found, {A}), [(B, 'betaDep', [A])])

    def test_harness_own_names(self):
        f = self.tmp / 'own_tester.py'
        f.write_text('"""Checks helperA and lonely, then useAlias."""\n# useNorm and isOdd;\n'
                     'def g():\n    """betaUse and lonely, x."""\n    raise Failure("struct Point not found; helperA, y")\n'
                     "CODE = '''\nint lonely = 3;\nint main() { int helperA = 1; return helperA; }\n'''\n")
        c = self.tmp / 'own_tester.cpp'
        c.write_text('// int useNorm;\nnamespace { lng betaUse(lng x) { return x; } }\n')
        self.eq(H.own_names([f, c]), {'Point', 'lonely', 'main', 'betaUse'},
                what='prose in docstrings and comments is not a declaration; literals with ; { } or #include are C++')

    def test_harness_aggregate(self):
        (self.fx / '05-Mathematics/99-all.hpp').write_text('#pragma once\n#include "98-basic.hpp"\n#include "02-unrelated.hpp"\n')
        (self.fx / '05-Mathematics/98-basic.hpp').write_text('#pragma once\n#include "01-beta.hpp"\n')
        self.addCleanup(lambda: [(self.fx / f'05-Mathematics/{n}').unlink() for n in ('99-all.hpp', '98-basic.hpp')])
        with patch.object(H, 'ROOT', self.fx):
            self.eq(G.aggregate(self.fx, self.ix.graph, '05-Mathematics/99-all.hpp'), [B, U], what='nested aggregates resolve to headers')

    def test_harness_scratch(self):
        base = self.tmp / 'work'
        base.mkdir()
        tests = self.fx / '96-Local Testing'
        files = {'02-Data Structures/01-alpha_tester.py': "# builds 01-alpha.hpp\nCODE = '''\nint main() { return helperA(1) + lonely(2); }\n'''\n",
                 '02-Data Structures/01-alpha_helper.cpp': 'int helper() { return useAlias(2); }\n', '_00_shared.py': 'X = 1\n',
                 '02-Data Structures/__pycache__/junk.pyc': ''}
        write_tree(tests, files)
        (self.fx / '01-Core/98-basic.hpp').write_text('#pragma once\n#include "01-template.hpp"\n')
        self.addCleanup(lambda: (shutil.rmtree(tests), (self.fx / '01-Core/98-basic.hpp').unlink()))
        tester = tests / '02-Data Structures/01-alpha_tester.py'
        with patch.object(H, 'ROOT', self.fx):
            path, names = H.scratch(self.ix, tester, [tests / '02-Data Structures/01-alpha_helper.cpp'], base)
        tree = path.parents[2]
        self.eq((path.relative_to(tree).as_posix(), names, tree.parent), ('96-Local Testing/02-Data Structures/01-alpha_tester.py',
                                                                          ['helperA', 'useAlias'], base), what='lonely is outside the named closure')
        self.eq([(tree / h).read_bytes() for h in (T, D, '01-Core/98-basic.hpp')], [(self.fx / h).read_bytes() for h in (T, D, '01-Core/98-basic.hpp')])
        for h in (A, B, U):  # each stub: its quoted includes, then its own kept lines in their original order
            orig = (self.fx / h).read_text().split('\n')
            incs = [l for l in orig if l.lstrip().startswith('#include "')]
            stub = (tree / h).read_text().split('\n')
            self.eq(stub[:1 + len(incs)], ['#pragma once'] + incs, h=h)
            rest, it = [l for l in stub[1 + len(incs):] if l.strip()], iter(orig)
            self.true(all(any(l == o for o in it) for l in rest), h=h, stub=stub)
            self.eq(bool(rest), h == A, h=h, what='only alpha contributes')
        self.true(HELPER_A[2] in (tree / A).read_text() and 'inline int lonely' not in (tree / A).read_text())
        self.eq(sorted(p.relative_to(tree / '96-Local Testing').as_posix() for p in (tree / '96-Local Testing').rglob('*') if p.is_file()),
                ['02-Data Structures/01-alpha_helper.cpp', '02-Data Structures/01-alpha_tester.py', '_00_shared.py'])
        for cxx in self.compilers():
            got = build_run(self.tmp, f'#include "{tree}/{A}"\nint main() {{ printf("%d %d\\n", helperA(1), useAlias(3)); }}\n', cxx)
            self.eq(got, '2 11\n', cxx=cxx)


def o_bell(n):
    """Bell numbers B_0..B_n by the Bell triangle."""
    row, out = [1], [1]
    for _ in range(n):
        new = [row[-1]]
        for v in row:
            new.append(new[-1] + v)
        row = new
        out.append(row[0])
    return out


def o_is_prime(n):
    if n < 2:
        return False
    for p in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):
        if n % p == 0:
            return n == p
    if n < 10 ** 10:
        return all(n % d for d in range(41, int(n ** 0.5) + 2, 2))
    d, s = n - 1, 0
    while d % 2 == 0:
        d, s = d // 2, s + 1
    for a in (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37):  # deterministic below 3.3e24
        x = pow(a, d, n)
        if x in (1, n - 1):
            continue
        for _ in range(s - 1):
            x = x * x % n
            if x == n - 1:
                break
        else:
            return False
    return True


class Repo(Base):
    @classmethod
    def setUpClass(cls):
        if not HAVE_CLANG:
            raise unittest.SkipTest('SKIP libclang: python3 cannot import clang.cindex')
        cls.ix = G.Index(ROOT, quiet=True)

    def run_extraction(self, names, main, stdin=''):
        out = G.grab(names, ROOT, self.ix)
        self.true(not any(l.lstrip().startswith('#include "') for l in out.split('\n')) and 'using lng' not in out, names=names)
        res = {}
        for cxx in self.compilers():
            res[cxx] = build_run(self.tmp, prelude(ROOT) + out + main, cxx, stdin=stdin, opt='-O2')
        return res

    def test_repo_bell_table(self):
        bell = o_bell(60)
        self.eq(bell[:8], [1, 1, 2, 5, 15, 52, 203, 877], what='Bell triangle oracle')
        main = 'int main() { int n; cin >> n; auto b = bellTable(n); for (auto &x : b) { cout << x.val() << "\\n"; } }\n'
        for n in (0, 1, 60):
            for cxx, got in self.run_extraction(['bellTable'], main, f'{n}\n').items():
                self.eq(got.split(), [str(v % 998244353) for v in bell[:n + 1]], n=n, cxx=cxx)

    def test_repo_dsu(self):
        main = ('int main() { int n, q; cin >> n >> q; DSU d(n); while (q--) { char c; cin >> c;\n'
                '    if (c == \'u\') { int a, b; cin >> a >> b; cout << d.uniteSets(a, b) << "\\n"; }\n'
                '    else if (c == \'s\') { int a, b; cin >> a >> b; cout << d.isSameSet(a, b) << "\\n"; }\n'
                '    else if (c == \'z\') { int a; cin >> a; cout << d.getSize(a) << "\\n"; }\n'
                '    else { cout << d.count() << "\\n"; } } }\n')
        for r in range(4):
            n = self.rng.randint(1, 40)
            label = list(range(n))
            ops, want = [], []
            for _ in range(300):
                c = self.rng.choice('uuszc')
                a, b = self.rng.randrange(n), self.rng.randrange(n)
                if c == 'u':
                    ops.append(f'u {a} {b}')
                    want.append(int(label[a] != label[b]))
                    old = label[b]
                    label = [label[a] if x == old else x for x in label]
                elif c == 's':
                    ops.append(f's {a} {b}')
                    want.append(int(label[a] == label[b]))
                elif c == 'z':
                    ops.append(f'z {a}')
                    want.append(label.count(label[a]))
                else:
                    ops.append('c')
                    want.append(len(set(label)))
            for cxx, got in self.run_extraction(['DSU'], main, f'{n} {len(ops)}\n' + '\n'.join(ops) + '\n').items():
                self.eq(got.split(), [str(w) for w in want], round=r, n=n, cxx=cxx)

    def test_repo_is_prime(self):
        sieve = [True] * 100001
        sieve[0] = sieve[1] = False
        for i in range(2, 317):
            if sieve[i]:
                sieve[i * i::i] = [False] * len(sieve[i * i::i])
        self.eq([i for i in range(100001) if o_is_prime(i) != sieve[i]], [], what='oracle against the sieve')
        self.eq([o_is_prime(n) for n in (2 ** 61 - 1, 2 ** 64 - 59, 2 ** 64 - 1, 3215031751, 3825123056546413051)], [True, True, False, False, False])
        nums = list(range(2000)) + [2 ** 61 - 1, 2 ** 64 - 59, 2 ** 64 - 1, 3215031751, 3825123056546413051, 561, 1105, 1729,
                                    4294967291 * 4294967279, 65521 ** 2, 1000000007 * 998244353]
        nums += [self.rng.randrange(2 ** 64) for _ in range(300)] + [self.rng.randrange(10 ** 10) for _ in range(100)]
        main = 'int main() { ulng n; while (cin >> n) { cout << isPrime(n) << "\\n"; } }\n'
        for cxx, got in self.run_extraction(['isPrime'], main, '\n'.join(map(str, nums)) + '\n').items():
            bad = [(n, g) for n, g in zip(nums, got.split()) if g != str(int(o_is_prime(n)))]
            self.eq((len(got.split()), bad[:5]), (len(nums), []), cxx=cxx)

    def test_repo_into_cli(self):
        f = self.tmp / 'sol.cpp'
        src = ('void solve(int t) {\n    DSU d(3); d.uniteSets(0, 2);\n    cout << d.count() << " " << isPrime(ulng(t)) << "\\n";\n}\n'
               'int main() { solve(7); solve(9); }\n')
        f.write_text(src)
        r = subprocess.run([sys.executable, str(GRAB_PY), '--into', str(f)], capture_output=True, text=True, timeout=600)
        self.eq((r.returncode, r.stdout, r.stderr), (0, '', f'{f}: block holds DSU, isPrime\n'))
        text = f.read_text()
        self.true(text.startswith('// grab begin: DSU isPrime\n') and text.endswith('// grab end\n' + src), text=text[:300])
        for cxx in self.compilers():
            self.eq(build_run(self.tmp, prelude(ROOT) + text, cxx, opt='-O2'), '2 1\n2 0\n', cxx=cxx)
        g = self.tmp / 'dij.cpp'
        g.write_text('void solve() {\n    Graph g(3, true);\n    g.addEdge(0, 1, 5); g.addEdge(1, 2, 7);\n    auto r = dijkstra(g, {0});\n'
                     '    auto p = r.path(g, 2);\n    cout << (long long)r.dist[2] << " " << p.vertices.size() << " " << r.finite(2) << "\\n";\n}\n'
                     'int main() { solve(); }\n')
        for extra in ([], ['--min']):
            r = subprocess.run([sys.executable, str(GRAB_PY), '--into', str(g), *extra], capture_output=True, text=True, timeout=600)
            self.eq(r.returncode, 0, err=r.stderr, extra=extra)
            for cxx in self.compilers():
                self.eq(build_run(self.tmp, prelude(ROOT) + g.read_text(), cxx, opt='-O2'), '12 3 1\n', cxx=cxx, extra=extra,
                        what='members called on structs the file never names (ShortestPathResult::path) stay')
        r = subprocess.run([sys.executable, str(GRAB_PY), '--no-copy', 'nosuchname_xyz'], capture_output=True, text=True, timeout=600)
        self.eq((r.returncode, r.stdout, r.stderr), (1, '', "grab: unknown name 'nosuchname_xyz' (try --find)\n"))


P = 1000003


class Library:
    """A random fixture library and its model: entities with text, references and values mod P."""

    def __init__(self, rng: random.Random):
        self.rng = rng
        self.ents: dict[str, dict] = {}
        k = rng.randint(2, 4)
        self.headers = [f'0{2 + i}-F{i}/0{i + 1}-h{i}.hpp' for i in range(k)]
        self.incs = {h: [d for d in self.headers[:i] if rng.random() < 0.6] for i, h in enumerate(self.headers)}
        self.closure = {}
        for h in self.headers:
            c = {h}
            for d in self.incs[h]:
                c |= self.closure[d]
            self.closure[h] = c
        self.body = {h: [] for h in self.headers}
        n = 0
        for h in self.headers:
            for _ in range(rng.randint(3, 6)):
                kind = rng.choice(['fn', 'fn', 'ovl', 'struct', 'tstruct', 'tfn'])
                if kind == 'tfn' and not self.visible(h, {'tstruct'}):
                    kind = 'fn'
                getattr(self, 'make_' + kind)(h, n)
                n += 1
        decoyed = set()
        for h in self.headers:  # decoys: a same-named free function outside every closure that names it
            for name, e in list(self.ents.items()):
                if e['kind'] == 'fn' and e['dependent_users'] and name not in decoyed and rng.random() < 0.5 \
                        and h not in self.closure[e['header']] and e['header'] not in self.closure[h] \
                        and all(h not in self.closure[u] for u in e['dependent_users']):
                    decoyed.add(name)
                    marker = f'inline lng {name}(lng x, lng y, lng z) {{ return x + y + z; }}'
                    self.ents[f'decoy_{name}'] = dict(kind='decoy', header=h, marker=marker)
                    self.body[h].append(marker)

    def visible(self, h, kinds):
        return [n for n, e in self.ents.items() if e['header'] in self.closure[h] and e['kind'] in kinds]

    def calls(self, h, var, own=()):
        """Up to 2 calls from header h on argument var (a lng): (text, entity, how)."""
        out = []
        pool = self.visible(h, {'fn', 'ovl', 'struct'})
        for _ in range(self.rng.randint(0, 2)):
            if own and self.rng.random() < 0.4:
                m = self.rng.choice(own)
                out.append((f'{m}()', None, ('own', m)))
            elif pool:
                n = self.rng.choice(pool)
                e = self.ents[n]
                if e['kind'] == 'fn':
                    out.append((f'{n}({var})', n, ('fn',)))
                elif e['kind'] == 'ovl':
                    two = self.rng.random() < 0.5
                    out.append((f'{n}({var}, 7)' if two else f'{n}({var})', n, ('ovl', two)))
                else:
                    m = self.rng.choice(e['methods'])
                    out.append((f'{n}({var}).{m}()', n, ('method', m)))
        return out

    def make_fn(self, h, n):
        name, a, b = f'f{n}', self.rng.randint(1, 999), self.rng.randint(0, 999)
        cs = self.calls(h, 'x')
        marker = f'inline lng {name}(lng x) {{'
        self.body[h].append(f'{marker}\n    return (x * {a} + {b}' + ''.join(f' + {t}' for t, _, _ in cs) + f') % {P};}}')
        self.ents[name] = dict(kind='fn', header=h, a=a, b=b, calls=cs, marker=marker, dependent_users=[])

    def make_ovl(self, h, n):
        name, a, b, c = f'o{n}', self.rng.randint(1, 999), self.rng.randint(0, 999), self.rng.randint(1, 999)
        m1, m2 = f'inline lng {name}(lng x) {{ return (x * {a} + {b}) % {P}; }}', f'inline lng {name}(lng x, lng y) {{ return (x * {c} + y) % {P}; }}'
        self.body[h] += [m1, m2]
        self.ents[name] = dict(kind='ovl', header=h, a=a, b=b, c=c, marker=m1, marker2=m2)

    def methods(self, h, name, T):
        ms, defs = [], {}
        for j in range(self.rng.randint(1, 3)):
            m, a, b = f'm{name}_{j}', self.rng.randint(1, 999), self.rng.randint(0, 999)
            cs = self.calls(h, 'd', tuple(ms)) if self.rng.random() < 0.7 else []
            if T == 'T':
                cs = [c for c in cs if c[2][0] in ('own', 'fn')]
            marker = f'    {T} {m}() const {{ return (d * {a} + {b}' + ''.join(f' + {t}' for t, _, _ in cs) + f') % {P}; }}'
            defs[m] = dict(a=a, b=b, calls=cs, marker=marker.strip())
            ms.append(m)
        return ms, defs

    def make_struct(self, h, n):
        name = f'S{n}'
        ms, defs = self.methods(h, name, 'lng')
        lines = [f'struct {name} {{', '    lng d;', f'    {name}(lng v) : d(v % {P}) {{}}', '    lng size() const { return d; }',
                 f'    friend {name} operator+({name} p, {name} q) {{ return {name}(p.d + q.d); }}'] + ['    ' + defs[m]['marker'] for m in ms] + ['};']
        self.body[h].append('\n'.join(lines))
        self.ents[name] = dict(kind='struct', header=h, methods=ms, defs=defs, marker=f'struct {name} {{')

    def make_tstruct(self, h, n):
        name = f'B{n}'
        ms, defs = self.methods(h, name, 'T')
        lines = ['template<class T>', f'struct {name} {{', '    T d;', f'    {name}(T v) : d(v % {P}) {{}}'] + ['    ' + defs[m]['marker'] for m in ms] + ['};']
        self.body[h].append('\n'.join(lines))
        for m in ms:
            for c in defs[m]['calls']:
                if c[1]:
                    self.ents[c[1]]['dependent_users'].append(h)
        self.ents[name] = dict(kind='tstruct', header=h, methods=ms, defs=defs, marker=f'struct {name} {{')

    def make_tfn(self, h, n):
        name = f't{n}'
        bs = self.rng.choice(self.visible(h, {'tstruct'}))
        m = self.rng.choice(self.ents[bs]['methods'])
        fns = self.visible(h, {'fn'})
        f = self.rng.choice(fns) if fns and self.rng.random() < 0.7 else None
        if f:
            self.ents[f]['dependent_users'].append(h)
        marker = f'T {name}(T x) {{ {bs}<T> s(x); return (s.{m}()' + (f' + {f}(x)' if f else '') + f') % {P}; }}'
        self.body[h].append(f'template<class T>\n{marker}')
        self.ents[name] = dict(kind='tfn', header=h, bs=bs, m=m, f=f, marker=marker)

    def files(self):
        out = {T: '#pragma once\n#include <cstdint>\n#include <cstdio>\nusing lng = int64_t;\n'}
        for h in self.headers:
            incs = ''.join(f'#include "../{d}"\n' for d in self.incs[h])
            out[h] = '#pragma once\n#include "../01-Core/01-template.hpp"\n' + incs + '\n' + '\n\n'.join(self.body[h]) + '\n'
        return out

    # model
    def value(self, call, x):
        text, name, how = call
        if how[0] == 'fn':
            return self.fn(name, x)
        if how[0] == 'ovl':
            e = self.ents[name]
            return (x * e['c'] + 7) % P if how[1] else (x * e['a'] + e['b']) % P
        return self.method(name, how[1], x)

    def fn(self, name, x):
        e = self.ents[name]
        return (x * e['a'] + e['b'] + sum(self.value(c, x) for c in e['calls'])) % P

    def method(self, struct, m, x, d=None):
        e = self.ents[struct]
        d = x % P if d is None else d
        df = e['defs'][m]
        total = d * df['a'] + df['b']
        for c in df['calls']:
            total += self.method(struct, c[2][1], x, d) if c[2][0] == 'own' else self.value(c, d)
        return total % P

    def tfn(self, name, x):
        e = self.ents[name]
        return (self.method(e['bs'], e['m'], x) + (self.fn(e['f'], x) if e['f'] else 0)) % P

    def reach(self, roots):
        """(entities, methods {struct: set}) reachable from roots: names, or (struct, method). A dependent call's name also
        reaches a same-named decoy in the include closure of any header holding reached code (instantiation-time lookup)."""
        ents, methods, work, deps = set(), {}, list(roots), set()
        for w in roots:  # a root struct is whole even when another root reaches it first
            if not isinstance(w, tuple) and self.ents[w]['kind'] in ('struct', 'tstruct'):
                work += [(w, m) for m in self.ents[w]['methods']]
            if f'decoy_{w}' in self.ents:  # a command-line name keeps every same-named global declaration
                ents.add(f'decoy_{w}')

        def target(c, s):
            return (s, c[2][1]) if c[2][0] == 'own' else (c[1], c[2][1]) if c[2][0] == 'method' else c[1]

        def method(s, m):
            if m not in methods.setdefault(s, set()):
                methods[s].add(m)
                ents.add(s)
                if self.ents[s]['kind'] == 'tstruct':
                    deps.update(c[1] for c in self.ents[s]['defs'][m]['calls'] if c[1])
                work.extend(target(c, s) for c in self.ents[s]['defs'][m]['calls'])
        while work:
            w = work.pop()
            if isinstance(w, tuple):
                method(*w)
            elif w not in ents:
                ents.add(w)
                e = self.ents[w]
                if e['kind'] in ('struct', 'tstruct'):
                    methods.setdefault(w, set())
                work.extend(target(c, None) for c in e.get('calls', ()))
                if e['kind'] == 'tfn':
                    work.append((e['bs'], e['m']))
                    if e['f']:
                        work.append(e['f'])
                        deps.add(e['f'])
        while True:
            seen = set().union(*(self.closure[self.ents[e]['header']] for e in ents))
            new = {f'decoy_{n}' for n in deps if f'decoy_{n}' in self.ents and self.ents[f'decoy_{n}']['header'] in seen} - ents
            if not new:
                return ents, methods
            ents |= new

    def expr(self, root):
        """(C++ expression list, values for x) of a root."""
        if isinstance(root, tuple):
            s, m = root
            return [(f'{s}(x).{m}()', lambda x: self.method(s, m, x))]
        e = self.ents[root]
        if e['kind'] == 'fn':
            return [(f'{root}(x)', lambda x: self.fn(root, x))]
        if e['kind'] == 'ovl':
            return [(f'{root}(x)', lambda x: (x * e['a'] + e['b']) % P), (f'{root}(x, 7)', lambda x: (x * e['c'] + 7) % P)]
        if e['kind'] == 'tfn':
            return [(f'{root}(x)', lambda x: self.tfn(root, x))]
        ty = root if e['kind'] == 'struct' else f'{root}<lng>'
        return [(f'{ty}(x).{m}()', lambda x, m=m: self.method(root, m, x)) for m in e['methods']]


class Stress(Base):
    def test_random_libraries(self):
        if not HAVE_CLANG:
            self.skipTest('SKIP libclang: python3 cannot import clang.cindex')
        cxxs = self.compilers()
        rounds = ROUNDS if MODE == 'stress' else 3 if MODE == 'full' else 1
        xs = [0, 1, 7, 123456, 999999]
        for r in range(rounds):
            lib = Library(random.Random(f'{SEED}:{r}'))
            root = write_tree(self.tmp / f'r{r}', lib.files())
            ix = built(root)
            texts = {h: t.split('\n') for h, t in lib.files().items()}
            names = [n for n, e in lib.ents.items() if e['kind'] != 'decoy']
            picks = [[n] for n in lib.rng.sample(names, min(5, len(names)))]
            structs = [n for n in names if lib.ents[n]['kind'] in ('struct', 'tstruct')]
            if structs:
                s = lib.rng.choice(structs)
                picks.append([(s, lib.rng.choice(lib.ents[s]['methods']))])
            picks.append(lib.rng.sample(names, min(2, len(names))))
            for roots in picks:
                cli = [f'{x[0]}::{x[1]}' if isinstance(x, tuple) else x for x in roots]
                ctx = dict(round=r, roots=cli, fixture=str(root))
                out = G.grab(cli, root, ix)
                lines = stripped(out)
                ents, methods = lib.reach(roots)
                missing, extra = [], []
                for n, e in lib.ents.items():
                    want = n in ents
                    for mk in [e['marker']] + ([e['marker2']] if 'marker2' in e else []):
                        if (mk.strip() in lines) != want:
                            (missing if want else extra).append(mk.strip())
                    for m in e.get('methods', ()):
                        mk = e['defs'][m]['marker']
                        if (mk in lines) != (m in methods.get(n, ())):
                            (missing if m in methods.get(n, ()) else extra).append(mk)
                self.eq((missing, extra), ([], []), what='reachable kept, unreachable absent', out=out, **ctx)
                self.eq(o_line_problems(G.grab(cli, root, ix, True), texts), [], **ctx)
                exprs = [e for x in roots for e in lib.expr(x)]
                main = 'int main() { lng xs[] = {' + ', '.join(map(str, xs)) + '}; for (lng x : xs) { printf("' + ' '.join(['%lld'] * len(exprs)) + '\\n"' + \
                       ''.join(f', (long long)({c})' for c, _ in exprs) + '); } }\n'
                want = ''.join(' '.join(str(f(x)) for _, f in exprs) + '\n' for x in xs)
                for cxx in cxxs:
                    self.eq(build_run(self.tmp / f'b{r}', prelude(root) + out + main, cxx), want, cxx=cxx, out=out, **ctx)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--mode', choices=('quick', 'full', 'stress'), default=os.environ.get('CP_TEST_MODE', 'full'))
    parser.add_argument('--seed', type=int, default=int(os.environ.get('CP_TEST_SEED', '42')))
    parser.add_argument('--rounds', type=int, default=8, help='random libraries in stress mode')
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Units, Selection, Render, Resolve, Cli, Tokens, Into, Bundle, Extras, Fallback, Cache, Harness, HarnessIndex, Repo, Stress):
        suite.addTests(case(n) for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != 'quick' or n not in FULL)
    print(f"grab checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else 3 if MODE == 'full' else 1} "
          f"CXX={CXX} libclang={'yes' if HAVE_CLANG else 'SKIP'} tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f'\033[{32 if ok else 31}m' if tty else '') + ('PASS' if ok else 'FAIL') + ('\033[0m' if tty else ''), flush=True)
    raise SystemExit(not ok)
