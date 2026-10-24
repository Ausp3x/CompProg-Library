#!/usr/bin/env python3
"""Checks for 98-Team Notebook/05-dependencies.py and the include-edge rules of 00-Guidelines/13-Plan/plan.py.

quick: every fixture test (strip_cpp, includes, scan, closure, compact, minis, plan, describe, chart, CLI), the
compiler oracles (`$CXX -MM` closure; the closure concatenated in typing order, quoted includes removed, compiles with
`-fsyntax-only`) on a fixed sample of 15 headers, every other repository oracle (scanner graph, typing order, line
counts, plan invariants) on all headers, the plan.py checks on a copy of the working tree, and 20 seeded random DAGs.
full: quick with both compiler oracles on every 01-07 header (thread pool; the compile oracle skips the 98/99
aggregates) and 100 random DAGs.
stress: full with --rounds random DAGs (default 1000), each also written to disk and checked through plan().
Oracles: an os.walk/line scanner for the include graph, the preprocessor's own dependency list for closures, an
iterative post-order and a fixed-point reachability set for random graphs, a newline count for line counts, and an
independent owner/edge computation for plan.py. Modes/seeds also come from CP_TEST_MODE/CP_TEST_SEED; CXX picks the
compiler (default g++). Failing tests print the seed, mode and retained fixture directory.

Feature map: headers -> test_headers_scan_*, test_repo_graph_matches_scanner; closure -> test_closure_*,
test_repo_mm_closure, test_repo_typing_order, test_random_dags; lines -> test_lines_count, test_repo_line_counts;
strip_cpp/includes -> test_strip_cpp_*, test_includes_*; scan/headers(found) -> test_headers_conditional_*;
uses_reduction -> test_uses_reduction; compact -> test_compact_detection; minis -> test_minis_mapping; plan -> test_plan_*, test_repo_plan_invariants,
test_random_dags (stress: on disk); describe -> test_describe_text; chart -> test_chart_*; CLI -> test_cli_*;
plan.py include_edges -> test_plan_include_edges_*; set re-ranks -> test_plan_set_reranks; check (undeclared edges, late prerequisites, batch cycles) ->
test_plan_check_*; doctor -> test_plan_doctor_fix_*; rank -> test_plan_rank_*. The notebook page's JS tick handler is
run under node in 03-notebook_tester.py (explicit SKIP when node is absent).
"""
from __future__ import annotations

import argparse
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
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "98-Team Notebook" / "05-dependencies.py"
PLAN = ROOT / "00-Guidelines" / "13-Plan" / "plan.py"
CXX = os.environ.get("CXX", "g++")
MODE, SEED, ROUNDS = "full", 42, 1000
SAMPLE = ["05-Mathematics/12-combinatorics_advanced.hpp", "03-Geometry/14-minimumenclosingcircle.hpp",
          "05-Mathematics/07-primality_factorization.hpp", "01-Core/16-poly.hpp", "05-Mathematics/99-all.hpp",
          "01-Core/01-template.hpp", "01-Core/05-modint.hpp", "01-Core/09-rational.hpp", "01-Core/99-all.hpp",
          "02-Data Structures/01-dsu.hpp", "02-Data Structures/99-all.hpp", "04-Graphs/99-all.hpp",
          "06-Miscellaneous/02-customhash.hpp", "07-Strings/99-all.hpp", "03-Geometry/98-basic.hpp"]
QUICK_SKIP = {"test_repo_mm_closure_all", "test_repo_compile_closure_all"}


def load(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


DEPS = load(TOOL, "dependencies_under_test")


def write(root: Path, files: dict[str, str]) -> None:
    for rel, text in files.items():
        path = root / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("utf-8"))


def inc(*names: str) -> str:
    return "".join(f'#include "{n}"\n' for n in names)


def quoted_include(line: str) -> str | None:
    """`[ws]#[ws]include[ws]"x"` -> x, by successive prefix tests (no regex)."""
    rest = line.lstrip(" \t")
    if not rest.startswith("#"):
        return None
    rest = rest[1:].lstrip(" \t")
    if not rest.startswith("include"):
        return None
    rest = rest[7:].lstrip(" \t")
    return rest[1:rest.index('"', 1)] if rest.startswith('"') and '"' in rest[1:] else None


def scan(root: Path) -> dict[str, list[str]]:
    """Independent include scanner: os.walk, line prefix tests, os.path normalisation."""
    out = {}
    for top in sorted(p for p in root.iterdir() if p.is_dir() and re.fullmatch(r"0[1-7]-.*", p.name)):
        for current, dirs, files in os.walk(top):
            dirs[:] = [d for d in dirs if d != "97-Legacy"]
            for f in files:
                if not f.endswith(".hpp"):
                    continue
                rel = os.path.relpath(os.path.join(current, f), root)
                found = []
                for line in Path(current, f).read_text(encoding="utf-8").split("\n"):
                    if (name := quoted_include(line)) is not None:
                        found.append(os.path.normpath(os.path.join(os.path.dirname(rel), name)))
                out[rel.replace(os.sep, "/")] = [x.replace(os.sep, "/") for x in found]
    return out


def count_lines(path: Path) -> int:
    data = path.read_bytes()
    return data.count(b"\n") + (bool(data) and not data.endswith(b"\n"))


def post_order(graph, root, skip=frozenset(), swap=None):
    """Iterative DFS post-order oracle with the same skip/swap semantics."""
    swap, seen, order = swap or {}, set(), []
    start = swap.get(root, root)
    stack = [(start, iter(graph.get(start, [])))]
    seen.add(start)
    while stack:
        node, it = stack[-1]
        for d in it:
            if (node, d) in skip:
                continue
            d = swap.get(d, d)
            if d not in seen:
                seen.add(d)
                stack.append((d, iter(graph.get(d, []))))
                break
        else:
            stack.pop()
            order.append(node)
    return order


def reach(graph, root, skip=frozenset(), swap=None):
    """Fixed-point reachability oracle."""
    swap = swap or {}
    found, changed = {swap.get(root, root)}, True
    while changed:
        changed = False
        for u in list(found):
            for d in graph.get(u, []):
                v = swap.get(d, d)
                if (u, d) not in skip and v not in found:
                    found.add(v)
                    changed = True
    return found


def mm(header: str) -> set[str]:
    """Repository files the preprocessor reads for one header (`-MM`, Make escapes undone)."""
    r = subprocess.run([CXX, "-std=gnu++20", "-MM", "-MT", "x", "-x", "c++", header], cwd=ROOT,
                       capture_output=True, text=True, timeout=120)
    if r.returncode:
        raise AssertionError(f"{CXX} -MM {header} exited {r.returncode}: {r.stderr[-800:]}")
    text, toks, cur, i = r.stdout.replace("\\\n", " "), [], "", 0
    while i < len(text):
        if text[i] == "\\" and i + 1 < len(text) and text[i + 1] in " #":
            cur, i = cur + text[i + 1], i + 2
        elif text[i].isspace():
            toks, cur, i = toks + [cur] * bool(cur), "", i + 1
        else:
            cur, i = cur + text[i], i + 1
    toks += [cur] * bool(cur)
    if toks[:1] != ["x:"]:
        raise AssertionError(f"unexpected {CXX} -MM output for {header}: {r.stdout[:200]!r}")
    return {Path(os.path.normpath(ROOT / t)).relative_to(ROOT).as_posix() for t in toks[1:]}


class Base(unittest.TestCase):
    def setUp(self):
        self.root = Path(tempfile.mkdtemp(prefix="deps-test-"))
        self.addCleanup(self.clean)

    def clean(self):
        result = self._outcome.result
        if any(t is self or getattr(t, "test_case", None) is self for t, _ in result.failures + result.errors):
            print(f"\nREPRO seed={SEED} mode={MODE} rounds={ROUNDS} CXX={CXX} test={self.id()} fixture={self.root}",
                  file=sys.stderr)
        else:
            shutil.rmtree(self.root)

    def assert_typing_order(self, graph, order, skip=frozenset(), swap=None, ctx=""):
        swap = swap or {}
        self.assertEqual(len(order), len(set(order)), f"duplicates {ctx}: {order}")
        pos = {h: i for i, h in enumerate(order)}
        for h in order:
            for d in graph.get(h, []):
                if (h, d) not in skip:
                    d2 = swap.get(d, d)
                    self.assertIn(d2, pos, f"{ctx}: {h} includes {d2}, missing from {order}")
                    self.assertLess(pos[d2], pos[h], f"{ctx}: {d2} not before {h} in {order}")


CORE = {"01-Core/01-template.hpp": "#pragma once\n#include <bits/stdc++.h>\n",
        "01-Core/03-barrett.hpp": "#pragma once\n" + inc("01-template.hpp") + "struct B {};\n",
        "01-Core/04-montgomery.hpp": "#pragma once\n" + inc("01-template.hpp", "03-barrett.hpp") + "struct M {};\n" * 3,
        "01-Core/05-modint.hpp": "#pragma once\n" + inc("01-template.hpp", "03-barrett.hpp", "04-montgomery.hpp")
        + "// x\n" * 40,
        "01-Core/06-modintmini.hpp": "#pragma once\n" + inc("01-template.hpp") + "// m\n" * 5}


class Fixtures(Base):
    def test_headers_scan_normalisation_and_exclusions(self):
        write(self.root, {
            **CORE,
            "02-Data Structures/01-a.hpp": inc("../01-Core/01-template.hpp", "./sub/../02-b.hpp") + "#include <vector>\n"
            + '  #include "indented.hpp"\n#  include <set>\n// #include "commented.hpp"\n#include "01-a.hpp"\n',
            "02-Data Structures/02-b.hpp": inc("../01-Core/05-modint.hpp", "../01-Core/01-template.hpp"),
            "02-Data Structures/sub/deep/03-c.hpp": inc("../../../01-Core/01-template.hpp", "../../02-b.hpp", "../x.hpp"),
            "02-Data Structures/97-Legacy/04-old.hpp": inc("../01-a.hpp"),
            "03-Geometry/x/97-Legacy/05-old.hpp": inc("../../01-p.hpp"),
            "03-Geometry/01-p.hpp": "#pragma once\n",
            "03-Geometry/02-q.cpp": inc("01-p.hpp"),
            "03-Geometry/03-r.py": inc("01-p.hpp"),
            "08-Python/01-s.hpp": inc("x.hpp"),
            "09-Contest Testing/01-t.hpp": inc("x.hpp"),
            "00-Guidelines/01-u.hpp": "",
            "01-Core/CRLF.hpp": '#include "01-template.hpp"\r\n',
        })
        graph = DEPS.headers(self.root)
        self.assertEqual(graph["02-Data Structures/01-a.hpp"],
                         ["01-Core/01-template.hpp", "02-Data Structures/02-b.hpp", "02-Data Structures/indented.hpp",
                          "02-Data Structures/01-a.hpp"])
        self.assertEqual(graph["02-Data Structures/sub/deep/03-c.hpp"],
                         ["01-Core/01-template.hpp", "02-Data Structures/02-b.hpp", "02-Data Structures/sub/x.hpp"])
        self.assertEqual(graph["01-Core/CRLF.hpp"], ["01-Core/01-template.hpp"])
        self.assertEqual(graph, scan(self.root))
        for absent in ("02-Data Structures/97-Legacy/04-old.hpp", "03-Geometry/x/97-Legacy/05-old.hpp",
                       "08-Python/01-s.hpp", "09-Contest Testing/01-t.hpp", "00-Guidelines/01-u.hpp"):
            self.assertNotIn(absent, graph)
        self.assertFalse(any(h.endswith((".cpp", ".py")) for h in graph))

    def test_headers_scan_empty_root(self):
        self.assertEqual(DEPS.headers(self.root), {})
        self.assertEqual(DEPS.chart({}, "01-Core").splitlines(), ["flowchart LR",
                                                                  "%% 0 other headers in 01-Core include only the template"])

    def test_closure_chain_diamond_dedup(self):
        chain = {"a": ["b"], "b": ["c"], "c": []}
        self.assertEqual(DEPS.closure(chain, "a"), ["c", "b", "a"])
        self.assertEqual(DEPS.closure(chain, "c"), ["c"])
        diamond = {"a": ["b", "c", "b"], "b": ["d"], "c": ["d", "e"], "d": [], "e": ["d"]}
        self.assertEqual(DEPS.closure(diamond, "a"), ["d", "b", "e", "c", "a"])
        self.assert_typing_order(diamond, DEPS.closure(diamond, "a"))
        self.assertEqual(DEPS.closure({"a": ["x"]}, "a"), ["x", "a"])  # unknown include is a leaf
        self.assertEqual(DEPS.closure({}, "z"), ["z"])

    def test_closure_skip_and_swap(self):
        g = {"a": ["r", "m"], "m": ["r", "t"], "r": ["t"], "t": [], "mm": ["t"], "q": ["a"]}
        self.assertEqual(DEPS.closure(g, "a", {("a", "r")}), ["t", "r", "m", "a"])  # still required via m
        self.assertEqual(DEPS.closure(g, "a", {("a", "r"), ("m", "r")}), ["t", "m", "a"])
        self.assertEqual(DEPS.closure(g, "a", swap={"m": "mm"}), ["t", "r", "mm", "a"])
        self.assertEqual(DEPS.closure(g, "m", swap={"m": "mm"}), ["t", "mm"])  # swap applies to the root
        self.assertEqual(DEPS.closure(g, "q", {("a", "m")}, {"m": "mm"}), ["t", "r", "a", "q"])
        self.assertEqual(DEPS.closure(g, "q", {("q", "a")}), ["q"])
        self.assertEqual(DEPS.closure(g, "q", {("r", "t")}), ["r", "t", "m", "a", "q"])

    def test_lines_count(self):
        for name, text, n in (("e.hpp", "", 0), ("one.hpp", "x", 1), ("nl.hpp", "x\n", 1), ("two.hpp", "x\n\ny", 3),
                              ("crlf.hpp", "a\r\nb\r\n", 2), ("blank.hpp", "\n\n", 2), ("nonl.hpp", "a\nb", 2),
                              ("crlfnonl.hpp", "a\r\nb", 2), ("nl1.hpp", "\n", 1),
                              ("other-breaks.hpp", "a\fb\x0bc\x1cd\x85e\u2028f\n", 1),  # only \n ends a line
                              ("lone-cr.hpp", "a\rb\n", 1), ("cr-end.hpp", "a\r", 1)):
            write(self.root, {name: text})
            self.assertEqual(DEPS.lines(self.root, name), n, name)
            self.assertEqual(count_lines(self.root / name), n, name)

    def test_compact_detection(self):
        red = '#include "../01-Core/03-barrett.hpp"\n'
        cases = {
            "05-Mathematics/01-struct.hpp": (red + "template <class T>\nstruct ModCompact {\n};\n", True),
            "05-Mathematics/02-inline.hpp": (red + "inline unsigned long long mulCompact(int a,\n int b) {\n return a; }\n", True),
            "05-Mathematics/03-comment.hpp": (red + "// inline int fCompact() {\nint f() { return 0; }\n", False),
            "05-Mathematics/04-trailing.hpp": (red + "int g(); // struct XCompact { }\n", False),
            "05-Mathematics/05-decl.hpp": (red + "struct FooCompact;\ninline int hCompact(int);\n", False),
            "05-Mathematics/06-plain.hpp": (red + "inline int f() { return 0; }\nstruct Foo {};\n", False),
            "05-Mathematics/07-member.hpp": (red + "struct S { int xCompact; };\n", False),
            "05-Mathematics/08-block.hpp": (red + "/* struct XCompact { }; */\nint f();\n", False),
            "05-Mathematics/09-block-lines.hpp": (red + "/*\ninline int fCompact() {\n return 0;\n}\n*/\nstruct S {};\n", False),
            "05-Mathematics/10-block-then-twin.hpp": (red + "/* a */ int g(); /* b\n */ struct GCompact { };\n", True),
            "05-Mathematics/11-line-comment-of-block.hpp": (red + "// /*\nstruct HCompact {};\n// */\n", True),
            "05-Mathematics/12-define-trick.hpp": (red + '#define S "/*"\nstruct ICompact {};\n// */\n', True),
            "05-Mathematics/13-string.hpp": (red + 'const char *s = "struct JCompact {";\n', False),
            "05-Mathematics/14-no-reduction.hpp": ("struct KCompact { int n; };\ninline int mCompact() { return 0; }\n", False),
            "05-Mathematics/15-names-montgomery.hpp": ("Montgomery64 m;\nstruct LCompact {};\n", True),
            "05-Mathematics/16-reduction-in-comment.hpp": ('// #include "../01-Core/04-montgomery.hpp" Barrett\n'
                                                           "struct NCompact {};\n", False),
            "01-Core/05-core.hpp": ('#include "03-barrett.hpp"\ninline int mulCompact() { return 1; }\nstruct ACompact {};\n', False),
            "02-Data Structures/01-ds.hpp": ('#include "../01-Core/04-montgomery.hpp"\nstruct DsuCompact { int n; };\n', True),
        }
        write(self.root, {k: v for k, (v, _) in cases.items()})
        for rel, (_, want) in cases.items():
            self.assertIs(DEPS.compact(self.root, rel), want, rel)

    def test_uses_reduction(self):
        for text, want in (('#include "../01-Core/03-barrett.hpp"\n', True), ('#include "04-montgomery.hpp"\n', True),
                           ("BarrettReduction b;\n", True), ("using M = Montgomery64;\n", True),
                           ("// Montgomery\n/* Barrett */\n", False), ('const char *s = "Montgomery";\n', False),
                           ('/* #include "../01-Core/03-barrett.hpp" */\n', False), ("struct Barret {};\n", False),
                           ('#include "../01-Core/05-modint.hpp"\nint x;\n', False), ("", False),
                           ('#include "../01-Core/13-barrett.hpp"\n', False), ("int notMontgomery;\n", False),
                           ('  #  include "../01-Core/04-montgomery.hpp"\n', True), ('#include"03-barrett.hpp"\n', True)):
            self.assertIs(DEPS.uses_reduction(text), want, text)

    def test_strip_cpp_keeps_layout_and_directives(self):
        text = ('#include "a.hpp" // keep\nint x = 1; // gone\n/* block\n spans */ int y;\nconst char *s = "/* no";\n'
                "char c = '\"'; int z = 1'000;\n#define M(a) \\\n  a /* m */\nauto r = R\"(raw // x)\";\n"
                '#define D 1 /* multi\n line */ + 2 "lit"\n#define S "/*"\n')
        for directives in (False, True):
            out = DEPS.strip_cpp(text, directives)
            self.assertEqual(len(out), len(text))
            self.assertEqual([i for i, c in enumerate(out) if c == "\n"], [i for i, c in enumerate(text) if c == "\n"])
            for gone in ("gone", "block", "spans", "/* no", "raw", "keep", "/* m */", "multi", "line */"):
                self.assertNotIn(gone, out, (directives, gone))
            for kept in ("int x = 1;", "int y;", "const char *s = \"", "int z = 1'000;", "auto r = R"):
                self.assertIn(kept, out, (directives, kept))
            for kept in ('#include "a.hpp"', "#define M(a) \\", "#define D 1", '+ 2 "lit"', '#define S "/*"'):
                self.assertEqual(kept in out, directives, (directives, kept))
            self.assertEqual("#" in out, directives)  # without directives every preprocessor line is blanked

    def test_strip_cpp_splices_and_unterminated(self):
        cases = ['#define MSG "a\\\nb"\nint f() { return 0; }\n', '#define S "abc\\', "#define T 'x\\\ny'\nint g;\n",
                 '#define F 1 // note \\\n  #include "z.hpp"\n#include "y.hpp"\n', '#define U "x', "#define C /* open"]
        for text in cases:
            for directives in (False, True):
                with self.subTest(text=text, directives=directives):
                    out = DEPS.strip_cpp(text, directives)
                    self.assertEqual(len(out), len(text))
                    self.assertEqual([i for i, c in enumerate(out) if c == "\n"], [i for i, c in enumerate(text) if c == "\n"])
        for directives in (False, True):
            self.assertIn("int f() { return 0; }", DEPS.strip_cpp(cases[0], directives))
            self.assertIn("int g;", DEPS.strip_cpp(cases[2], directives))
            self.assertNotIn("z.hpp", DEPS.strip_cpp(cases[3], directives))
            self.assertEqual('#include "y.hpp"' in DEPS.strip_cpp(cases[3], directives), directives)
        self.assertIn('"a\\\nb"', DEPS.strip_cpp(cases[0], True))  # literal kept whole on a directive line
        self.assertNotIn("a", DEPS.strip_cpp(cases[0], False).split("\n")[0])
        self.assertEqual(DEPS.includes(cases[3]), [("y.hpp", False)])

    def test_strip_cpp_directive_comment_regressions(self):
        # A block comment opened on a directive line hides the following lines (as the preprocessor does).
        self.assertEqual(DEPS.includes('#include "a.hpp" /* note\n#include "b.hpp"\n*/\nint x;\n'), [("a.hpp", False)])
        self.assertEqual(DEPS.includes('#include "x.hpp" // note\n'), [("x.hpp", False)])
        self.assertEqual(DEPS.includes('#define D /* a\n b */ 1\n#include "y.hpp"\n'), [("y.hpp", False)])
        write(self.root, {
            "05-Mathematics/01-hidden.hpp": '#include "../01-Core/03-barrett.hpp" /* old\nstruct XCompact {};\n*/\n',
            "05-Mathematics/02-define.hpp": '#include "../01-Core/03-barrett.hpp"\n#define S "/*"\nstruct YCompact {};\n',
            "05-Mathematics/03-both.hpp": '#include "../01-Core/03-barrett.hpp" /* old\nstruct XCompact {};\n*/\n'
                                          "struct ZCompact {};\n"})
        self.assertFalse(DEPS.compact(self.root, "05-Mathematics/01-hidden.hpp"))
        self.assertTrue(DEPS.compact(self.root, "05-Mathematics/02-define.hpp"))
        self.assertTrue(DEPS.compact(self.root, "05-Mathematics/03-both.hpp"))
        self.assertEqual(DEPS.TWIN.findall(DEPS.strip_cpp((self.root / "05-Mathematics/03-both.hpp").read_text())), ["ZCompact"])

    def test_includes_comments_literals_and_depth(self):
        text = ('#include "a.hpp"\n/* #include "b.hpp" */\n/*\n#include "c.hpp"\n*/\n// #include "d.hpp"\n'
                'const char *s = "/*";\n#include "e.hpp"\n#if 0\n#include "f.hpp"\n#ifdef X\n#include "g.hpp"\n#else\n'
                '#include "h.hpp"\n#endif\n#elif 1\n#include "i.hpp"\n#endif\n#include "j.hpp"\n  #include "k.hpp"\n'
                '#ifndef Y\n#  include "l.hpp"\n#include "m.hpp" // trailing\n  #  endif\n#include "n.hpp"\n'
                '#include <vector>\n\t#\tinclude\t"o.hpp"\n#include"p.hpp"\n')
        self.assertEqual(DEPS.includes(text), [("a.hpp", False), ("e.hpp", False), ("f.hpp", True), ("g.hpp", True),
                                               ("h.hpp", True), ("i.hpp", True), ("j.hpp", False), ("k.hpp", False),
                                               ("l.hpp", True), ("m.hpp", True), ("n.hpp", False), ("o.hpp", False),
                                               ("p.hpp", False)])
        self.assertEqual(DEPS.includes('  # include "w.hpp"\n'), [("w.hpp", False)])
        self.assertEqual(DEPS.includes('#define F 1 // note \\\n  #include "z.hpp"\n#include "y.hpp"\n'), [("y.hpp", False)])
        self.assertEqual(DEPS.includes(""), [])

    def test_headers_conditional_and_legacy_root(self):
        files = {"01-Core/01-template.hpp": "#pragma once\n",
                 "01-Core/02-debug.hpp": '#include "01-template.hpp"\n',
                 "02-Data Structures/01-a.hpp": '#include "../01-Core/01-template.hpp"\n#ifdef LOCAL\n'
                                                '#include "../01-Core/02-debug.hpp"\n#endif\n'}
        for base in (self.root, self.root / "97-Legacy" / "copy"):
            write(base, files)
            self.assertEqual(DEPS.headers(base), {"01-Core/01-template.hpp": [], "01-Core/02-debug.hpp": ["01-Core/01-template.hpp"],
                                                  "02-Data Structures/01-a.hpp": ["01-Core/01-template.hpp", "01-Core/02-debug.hpp"]})
            found = DEPS.scan(base)
            self.assertEqual(found, {"01-Core/01-template.hpp": [], "01-Core/02-debug.hpp": [("01-Core/01-template.hpp", False)],
                                     "02-Data Structures/01-a.hpp": [("01-Core/01-template.hpp", False),
                                                                     ("01-Core/02-debug.hpp", True)]})
            self.assertEqual(DEPS.headers(base, found), DEPS.headers(base))
            self.assertEqual(DEPS.headers(base, {"x.hpp": [("y.hpp", True)]}), {"x.hpp": ["y.hpp"]})
        write(self.root / "97-Legacy" / "copy", {"03-Geometry/97-Legacy/01-x.hpp": ""})
        self.assertNotIn("03-Geometry/97-Legacy/01-x.hpp", DEPS.headers(self.root / "97-Legacy" / "copy"))

    def test_minis_mapping(self):
        g = dict.fromkeys(["01-Core/05-modint.hpp", "01-Core/06-modintmini.hpp", "01-Core/07-infint.hpp",
                           "01-Core/09-ratmini.hpp", "01-Core/16-poly.hpp", "01-Core/17-polymini.hpp",
                           "02-Data Structures/01-x.hpp", "02-Data Structures/02-xmini.hpp", "01-Core/01-template.hpp"], [])
        self.assertEqual(DEPS.minis(g), {"01-Core/05-modint.hpp": "01-Core/06-modintmini.hpp",
                                         "01-Core/16-poly.hpp": "01-Core/17-polymini.hpp"})
        self.assertEqual(DEPS.minis({"01-Core/06-modintmini.hpp": []}), {})
        self.assertEqual(DEPS.minis({}), {})

    def compact_fixture(self):
        write(self.root, {
            **CORE,
            "05-Mathematics/01-x.hpp": inc("../01-Core/01-template.hpp", "../01-Core/03-barrett.hpp",
                                           "../01-Core/04-montgomery.hpp") + "inline int mulCompact(int a) { return a; }\n",
            "05-Mathematics/02-y.hpp": inc("../01-Core/01-template.hpp", "../01-Core/03-barrett.hpp",
                                           "../01-Core/05-modint.hpp") + "struct YCompact {};\n",
            "05-Mathematics/03-z.hpp": inc("../01-Core/01-template.hpp", "../01-Core/03-barrett.hpp") + "struct Z {};\n",
            "05-Mathematics/04-w.hpp": inc("../01-Core/01-template.hpp", "01-x.hpp") + "struct WCompact {};\n",
            "05-Mathematics/05-v.hpp": inc("../01-Core/01-template.hpp", "../01-Core/05-modint.hpp") + "// v\n",
        })
        return DEPS.headers(self.root)

    def test_plan_compact_optional_and_required_through_other_path(self):
        g = self.compact_fixture()
        t, b, m, mi = "01-Core/01-template.hpp", "01-Core/03-barrett.hpp", "01-Core/04-montgomery.hpp", "01-Core/05-modint.hpp"
        p = DEPS.plan(self.root, g, "05-Mathematics/01-x.hpp")
        self.assertEqual(p["order"], [t, "05-Mathematics/01-x.hpp"])
        self.assertEqual(p["optional"], [b, m])
        p = DEPS.plan(self.root, g, "05-Mathematics/02-y.hpp")  # barrett and montgomery still needed through modint
        self.assertEqual(p["order"], [t, b, m, mi, "05-Mathematics/02-y.hpp"])
        self.assertEqual(p["optional"], [])
        p = DEPS.plan(self.root, g, "05-Mathematics/03-z.hpp")  # no twin: nothing skipped
        self.assertEqual((p["order"], p["optional"]), ([t, b, "05-Mathematics/03-z.hpp"], []))
        p = DEPS.plan(self.root, g, "05-Mathematics/04-w.hpp")  # skip covers only the header's own includes
        self.assertEqual(p["order"], [t, b, m, "05-Mathematics/01-x.hpp", "05-Mathematics/04-w.hpp"])
        self.assertEqual(p["optional"], [])
        p = DEPS.plan(self.root, g, mi)  # Core header is never compact
        self.assertEqual((p["order"], p["optional"]), ([t, b, m, mi], []))
        for h in g:
            p, full = DEPS.plan(self.root, g, h), DEPS.closure(g, h)
            self.assertEqual(set(p["order"]) | set(p["optional"]), set(full), h)
            self.assertFalse(set(p["order"]) & set(p["optional"]), h)
            self.assertEqual(p["optional"], [x for x in full if x not in p["order"]], h)

    def test_plan_minis_untested_and_tested(self):
        g = self.compact_fixture()
        v, mi, mm_ = "05-Mathematics/05-v.hpp", "01-Core/05-modint.hpp", "01-Core/06-modintmini.hpp"
        size = lambda hs: sum(count_lines(self.root / h) for h in hs)
        swapped = ["01-Core/01-template.hpp", mm_, v]
        p = DEPS.plan(self.root, g, v)
        self.assertEqual(p["minis"], {mi: (mm_, size(swapped))})
        self.assertLess(size(swapped), size(p["order"]))
        self.assertEqual(DEPS.plan(self.root, g, "05-Mathematics/03-z.hpp")["minis"], {})  # full type not in order
        text = DEPS.describe(self.root, g, v)
        self.assertIn(f"Mini exists, not tested with this header: {mm_} for {mi} ({size(swapped)} lines in total with it)", text)
        with patch.dict(DEPS.TESTED_MINIS, {v: {mi: mm_}}):
            p = DEPS.plan(self.root, g, v)
            self.assertEqual((p["order"], p["optional"], p["minis"]), (swapped, [], {}))
            self.assertIn(f"{v}: 3 files, {size(swapped)} lines", DEPS.describe(self.root, g, v))
        # A mini that does not strictly shorten the closure is not noted.
        target = size(DEPS.closure(g, v))
        for extra in (0, 1):
            body = "#pragma once\n" + inc("01-template.hpp") + "// m\n" * 2
            need = target - size(["01-Core/01-template.hpp", v]) - len(body.splitlines()) + extra
            write(self.root, {mm_: body + "// pad\n" * need})
            self.assertEqual(size(swapped) - target, extra)
            self.assertEqual(DEPS.plan(self.root, g, v)["minis"], {}, f"extra={extra}")
            self.assertNotIn("Mini exists", DEPS.describe(self.root, g, v))

    def test_plan_minis_not_self_or_present(self):
        g = self.compact_fixture()
        write(self.root, {"05-Mathematics/06-both.hpp": inc("../01-Core/05-modint.hpp", "../01-Core/06-modintmini.hpp")})
        g = DEPS.headers(self.root)
        self.assertEqual(DEPS.plan(self.root, g, "01-Core/05-modint.hpp")["minis"], {})  # not for the full type itself
        self.assertEqual(DEPS.plan(self.root, g, "05-Mathematics/06-both.hpp")["minis"], {})  # mini already typed
        self.assertNotIn("Mini exists", DEPS.describe(self.root, g, "01-Core/05-modint.hpp"))
        self.assertIn("01-Core/05-modint.hpp", DEPS.plan(self.root, g, "05-Mathematics/05-v.hpp")["minis"])

    def test_describe_text(self):
        g = self.compact_fixture()
        x = "05-Mathematics/01-x.hpp"
        order = ["01-Core/01-template.hpp", x]
        lines = DEPS.describe(self.root, g, x).splitlines()
        total = sum(count_lines(self.root / h) for h in order)
        self.assertEqual(lines[0], f"{x}: 2 files, {total} lines")
        self.assertEqual(lines[1:3], [f"{count_lines(self.root / h):6}  {h}" for h in order])
        self.assertEqual(lines[3], "Skippable when typing only the *Compact twins and code that needs no Core reduction: "
                         f"01-Core/03-barrett.hpp ({count_lines(self.root / '01-Core/03-barrett.hpp')} lines), "
                         f"01-Core/04-montgomery.hpp ({count_lines(self.root / '01-Core/04-montgomery.hpp')} lines)"
                         f"; {total + count_lines(self.root / '01-Core/03-barrett.hpp') + count_lines(self.root / '01-Core/04-montgomery.hpp')}"
                         " lines in total with them")
        self.assertEqual(len(lines), 4)

    def chart_fixture(self):
        write(self.root, {
            **CORE,
            "06-Miscellaneous/02-hash.hpp": inc("../01-Core/01-template.hpp"),
            "03-Geometry/01-point.hpp": inc("../01-Core/01-template.hpp", "../01-Core/05-modint.hpp"),
            "03-Geometry/02-line.hpp": inc("../01-Core/01-template.hpp", "01-point.hpp", "../06-Miscellaneous/02-hash.hpp"),
            "03-Geometry/03-alone.hpp": inc("../01-Core/01-template.hpp"),
            "03-Geometry/04-alone.hpp": "",
            "03-Geometry/05-leaf.hpp": inc("../01-Core/01-template.hpp"),
            "03-Geometry/06-uses.hpp": inc("05-leaf.hpp"),
            "03-Geometry/sub/07-nested.hpp": inc("../01-point.hpp"),
            "03-Geometry/98-basic.hpp": inc("01-point.hpp", "02-line.hpp"),
            "03-Geometry/99-all.hpp": inc("98-basic.hpp", "03-alone.hpp"),
        })
        return DEPS.headers(self.root)

    def parse_chart(self, text):
        lines = text.splitlines()
        self.assertEqual(lines[0], "flowchart LR")
        labels, groups, edges, group = {}, {}, set(), None
        for line in lines[1:-1]:
            if m := re.fullmatch(r'  subgraph g(\d\d)\["([^"]+)"\]', line):
                self.assertIsNone(group, line)
                group = m[2]
                self.assertEqual(m[1], group[:2])
                self.assertNotIn(group, groups)
                groups[group] = []
            elif line == "  end":
                self.assertIsNotNone(group)
                group = None
            elif m := re.fullmatch(r'    (n\d+)\["([^"]+)"\]', line):
                self.assertNotIn(m[1], labels)
                labels[m[1]] = f"{group}/{m[2]}"
                groups[group].append(m[1])
            elif m := re.fullmatch(r"  (n\d+) --> (n\d+)", line):
                edges.add((labels[m[1]], labels[m[2]]))
            else:
                self.fail(f"unexpected chart line {line!r}")
        self.assertIsNone(group)
        return labels, groups, edges, lines[-1]

    def test_chart_edges_groups_and_isolated_count(self):
        g = self.chart_fixture()
        own = [h for h in g if h.startswith("03-Geometry/")]
        labels, groups, edges, tail = self.parse_chart(DEPS.chart(g, "03-Geometry"))
        want = {(h, d) for h in own if not re.match(r"03-Geometry/9[89]-", h) for d in g[h] if d != DEPS.TEMPLATE}
        self.assertEqual(edges, want)
        self.assertEqual(set(labels.values()), {x for e in want for x in e})
        self.assertNotIn(DEPS.TEMPLATE, labels.values())
        self.assertFalse(any("/98-" in x or "/99-" in x for x in labels.values()))
        self.assertEqual(set(groups), {"03-Geometry", "01-Core", "06-Miscellaneous"})
        self.assertEqual(list(groups)[0], "03-Geometry")
        alone = {"03-Geometry/03-alone.hpp", "03-Geometry/04-alone.hpp"}
        self.assertEqual(tail, f"%% {len(alone)} other headers in 03-Geometry include only the template")
        labels, _, edges, tail = self.parse_chart(DEPS.chart(g, "01-Core"))
        self.assertEqual(edges, {(h, d) for h in CORE for d in g[h] if d != DEPS.TEMPLATE})
        alone = set(CORE) - set(labels.values())
        self.assertEqual(alone, {DEPS.TEMPLATE, "01-Core/06-modintmini.hpp"})
        self.assertEqual(tail, "%% 2 other headers in 01-Core include only the template")

    def run_cli(self, *args, root=None):
        return subprocess.run([sys.executable, str(root or ROOT) + "/98-Team Notebook/05-dependencies.py", *args],
                              cwd=self.root, capture_output=True, text=True, timeout=60)

    def test_cli_fixture_exit_codes_and_output(self):
        g = self.chart_fixture()
        write(self.root, {"98-Team Notebook/05-dependencies.py": TOOL.read_text(encoding="utf-8")})
        r = self.run_cli("closure", "03-Geometry/02-line.hpp", root=self.root)
        self.assertEqual((r.returncode, r.stderr), (0, ""))
        self.assertEqual(r.stdout, DEPS.describe(self.root, g, "03-Geometry/02-line.hpp") + "\n")
        r = self.run_cli("closure", root=self.root)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(r.stdout, "\n\n".join(DEPS.describe(self.root, g, h) for h in g) + "\n")
        r = self.run_cli("closure", "03-Geometry/02-line.hpp", "03-Geometry/nope.hpp", "x.hpp", root=self.root)
        self.assertEqual((r.returncode, r.stdout), (1, ""))
        self.assertIn("Unknown header", r.stderr)
        self.assertIn("03-Geometry/nope.hpp, x.hpp", r.stderr)
        r = self.run_cli("chart", "03-Geometry", root=self.root)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(r.stdout, "%% 03-Geometry\n" + DEPS.chart(g, "03-Geometry") + "\n")
        r = self.run_cli("chart", root=self.root)
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(re.findall(r"^%% (0\d-[^\n]+)$", r.stdout, re.M), ["01-Core", "03-Geometry", "06-Miscellaneous"])
        for bad in (["chart", "09-Nope"], ["chart", "03-Geometry", "03-Geo"]):
            r = self.run_cli(*bad, root=self.root)
            self.assertEqual((r.returncode, r.stdout), (1, ""), bad)
            self.assertIn("Unknown folder: " + bad[-1], r.stderr)
        self.assertEqual(self.run_cli(root=self.root).returncode, 2)
        self.assertEqual(self.run_cli("bogus", root=self.root).returncode, 2)

    def test_cli_repository(self):
        g = DEPS.headers(ROOT)
        h = "05-Mathematics/07-primality_factorization.hpp"
        r = self.run_cli("closure", h)
        self.assertEqual((r.returncode, r.stdout), (0, DEPS.describe(ROOT, g, h) + "\n"), r.stderr)
        r = self.run_cli("chart")
        self.assertEqual(r.returncode, 0, r.stderr)
        self.assertEqual(len(re.findall(r"^flowchart LR$", r.stdout, re.M)), len({x.split("/")[0] for x in g}))
        self.assertEqual(self.run_cli("closure", "05-Mathematics/07-nope.hpp").returncode, 1)

    def test_random_dags(self):
        rounds = ROUNDS if MODE == "stress" else 100 if MODE == "full" else 20
        rng = random.Random(SEED)
        for case in range(rounds):
            n = rng.randint(1, 14)
            names = [f"h{i}" for i in range(n)]
            graph = {names[i]: [names[rng.randrange(i)] for _ in range(rng.randint(0, min(i, 5)) if i else 0)]
                     for i in range(n)}
            swap = {}
            for i in rng.sample(range(n), rng.randint(0, n // 3)):  # mini of h_i includes only lower nodes
                graph[f"m{i}"] = [names[rng.randrange(i)] for _ in range(rng.randint(0, min(i, 3)) if i else 0)]
                swap[names[i]] = f"m{i}"
            edges = [(u, d) for u, ds in graph.items() for d in ds]
            skip = set(rng.sample(edges, rng.randint(0, len(edges) // 2))) if edges else set()
            root = names[rng.randrange(n)] if rng.random() < 0.9 else f"m{rng.randrange(n)}"
            for sk, sw in ((frozenset(), None), (skip, None), (frozenset(), swap), (skip, swap)):
                with self.subTest(seed=SEED, case=case, graph=graph, root=root, skip=sorted(sk), swap=sw):
                    got = DEPS.closure(graph, root, sk, sw)
                    self.assertEqual(set(got), reach(graph, root, sk, sw))
                    self.assertEqual(got, post_order(graph, root, sk, sw))
                    self.assertEqual(got[-1], (sw or {}).get(root, root))
                    self.assert_typing_order(graph, got, sk, sw, f"case {case}")
            if MODE == "stress":
                self.random_plan(rng, case)

    def random_plan(self, rng, case):
        """Random on-disk fixture: plan() against a brute-force recomputation from the oracles above."""
        base = self.root / f"r{case}"
        names = ["01-Core/01-template.hpp", "01-Core/03-barrett.hpp", "01-Core/04-montgomery.hpp",  # minis precede fulls
                 "01-Core/06-modintmini.hpp", "01-Core/05-modint.hpp", "01-Core/08-infintmini.hpp", "01-Core/07-infint.hpp"]
        names += [f"0{rng.choice('2345')}-F/{i:02}-h.hpp" for i in range(10, 10 + rng.randint(1, 8))]
        files = {}
        for i, h in enumerate(names):
            deps = sorted({names[rng.randrange(i)] for _ in range(rng.randint(0, min(i, 4)))}) if i else []
            twin = not h.startswith("01-Core/") and rng.random() < 0.5
            body = "".join(f'#include "{os.path.relpath(d, os.path.dirname(h))}"\n' for d in deps)
            files[h] = body + "// pad\n" * rng.randint(0, 30) + ("inline int fCompact() { return 0; }\n" if twin else "")
        write(base, files)
        g = DEPS.headers(base)
        self.assertEqual(g, scan(base))
        self.assertEqual(DEPS.minis(g), {"01-Core/05-modint.hpp": "01-Core/06-modintmini.hpp",
                                         "01-Core/07-infint.hpp": "01-Core/08-infintmini.hpp"})
        size = lambda hs: sum(count_lines(base / h) for h in hs)
        mini = {"01-Core/05-modint.hpp": "01-Core/06-modintmini.hpp", "01-Core/07-infint.hpp": "01-Core/08-infintmini.hpp"}
        for h in g:
            tested = {f: m for f, m in mini.items() if rng.random() < 0.3}
            with self.subTest(seed=SEED, case=case, header=h, tested=tested, fixture=str(base)), \
                    patch.dict(DEPS.TESTED_MINIS, {h: tested}):
                twin = "fCompact" in files[h] and any(d in DEPS.REDUCTION for d in g[h])
                skip = {(h, d) for d in g[h] if twin and d in DEPS.REDUCTION}
                order, full = post_order(g, h, skip, tested), post_order(g, h, frozenset(), tested)
                want = {f: (m, size(post_order(g, h, skip, {**tested, f: m}))) for f, m in mini.items()
                        if f in order and f != h and m not in order}
                p = DEPS.plan(base, g, h)
                self.assertIs(DEPS.compact(base, h), twin)
                self.assertEqual(p, {"order": order, "optional": [x for x in full if x not in order],
                                     "minis": {f: x for f, x in want.items() if x[1] < size(order)}})


class Repository(Base):
    graph = DEPS.headers(ROOT)

    def test_repo_graph_matches_scanner(self):
        self.assertEqual(self.graph, scan(ROOT))
        self.assertGreater(len(self.graph), 100)
        for h in SAMPLE:
            self.assertIn(h, self.graph)

    def check_mm(self, hs):
        if not shutil.which(CXX):
            self.skipTest(f"SKIP compiler {CXX} not found")
        with ThreadPoolExecutor(min(8, os.cpu_count() or 1)) as pool:
            for h, files in zip(hs, pool.map(mm, hs)):
                with self.subTest(header=h, CXX=CXX):
                    c = DEPS.closure(self.graph, h)
                    self.assertEqual(files, set(c), f"{CXX} -std=gnu++20 -MM -x c++ '{h}': only preprocessor "
                                     f"{sorted(files - set(c))}, only closure {sorted(set(c) - files)}")

    def test_repo_mm_closure_sample(self):
        self.check_mm(SAMPLE)

    def test_repo_mm_closure_all(self):
        self.check_mm(sorted(set(self.graph) - set(SAMPLE)))

    def check_compile(self, hs):
        if not shutil.which(CXX):
            self.skipTest(f"SKIP compiler {CXX} not found")
        hs = [h for h in hs if not re.search(r"/9[89]-", h)]

        def run(h):
            order = DEPS.closure(self.graph, h, swap=DEPS.TESTED_MINIS.get(h, {}))
            tu = "".join(f'#line 1 "{x}"\n' + re.sub(r'^[ \t]*#[ \t]*(include[ \t]*"[^"\n]+"|pragma[ \t]+once\b).*$', "",
                                                    (ROOT / x).read_text(encoding="utf-8"), flags=re.M) + "\n"
                         for x in order)
            r = subprocess.run([CXX, "-std=gnu++20", "-fsyntax-only", "-x", "c++", "-"], input=tu, cwd=self.root,
                               capture_output=True, text=True, timeout=600)
            return order, r
        with ThreadPoolExecutor(min(4, os.cpu_count() or 1)) as pool:  # about 400 MB per compiler; stays under the 2048 MB pre-flight cap
            for h, (order, r) in zip(hs, pool.map(run, hs)):
                with self.subTest(header=h, CXX=CXX):
                    self.assertEqual(r.returncode, 0, f"typing order {order} does not compile as one TU "
                                     f"({CXX} -std=gnu++20 -fsyntax-only -x c++ -):\n{r.stderr[-1500:]}")
        print(f"compile oracle: {len(hs)} TUs with {CXX}", file=sys.stderr, flush=True)

    def test_repo_compile_closure_sample(self):
        self.check_compile(SAMPLE)

    def test_repo_compile_closure_all(self):
        self.check_compile(sorted(set(self.graph) - set(SAMPLE)))

    def test_repo_typing_order(self):
        for h in self.graph:
            c = DEPS.closure(self.graph, h)
            with self.subTest(header=h):
                self.assertEqual(c[-1], h)
                self.assertEqual(set(c), reach(self.graph, h))
                self.assert_typing_order(self.graph, c, ctx=h)

    def test_repo_line_counts(self):
        for h in self.graph:
            self.assertEqual(DEPS.lines(ROOT, h), count_lines(ROOT / h), h)

    def test_repo_plan_invariants(self):
        minis = DEPS.minis(self.graph)
        self.assertEqual(minis, {"01-Core/05-modint.hpp": "01-Core/06-modintmini.hpp",
                                 "01-Core/07-infint.hpp": "01-Core/08-infintmini.hpp",
                                 "01-Core/16-poly.hpp": "01-Core/17-polymini.hpp"})
        size = lambda hs: sum(count_lines(ROOT / h) for h in hs)
        for h in self.graph:
            with self.subTest(header=h):
                p, tested = DEPS.plan(ROOT, self.graph, h), DEPS.TESTED_MINIS.get(h, {})
                full = post_order(self.graph, h, swap=tested)
                skip = {(h, d) for d in self.graph[h] if d in DEPS.REDUCTION} if DEPS.compact(ROOT, h) else set()
                order = post_order(self.graph, h, skip, tested)
                self.assertEqual(p["order"], order)
                self.assertEqual(p["optional"], [x for x in full if x not in order])
                self.assertTrue(set(p["optional"]) <= set(DEPS.REDUCTION))
                want = {f: (m, size(post_order(self.graph, h, skip, {**tested, f: m}))) for f, m in minis.items()
                        if f in order and f != h and m not in order}
                self.assertEqual(p["minis"], {f: x for f, x in want.items() if x[1] < size(order)})
        p = DEPS.plan(ROOT, self.graph, "05-Mathematics/07-primality_factorization.hpp")
        self.assertEqual(p["optional"], ["01-Core/04-montgomery.hpp"])
        self.assertNotIn("01-Core/04-montgomery.hpp", p["order"])


def copy_tree(dst: Path) -> None:
    shutil.copytree(ROOT, dst, symlinks=True, ignore=shutil.ignore_patterns(
        ".git", "OLD", "95-Resources", "build", "__pycache__", "worktrees"))


class PlanRules(Base):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="deps-plan-")
        cls.copy = Path(cls.temp.name) / "library copy"
        copy_tree(cls.copy)
        cls.saved = {n: (cls.copy / n).read_bytes() for n in
                     ("00-Guidelines/13-Plan/batches.json", "00-Guidelines/13-Plan/packages.json", "01-prompts.md")}
        cls.plan = load(cls.copy / "00-Guidelines/13-Plan/plan.py", "plan_under_test")

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def setUp(self):
        super().setUp()
        self.restore()

    def restore(self):
        for n, data in self.saved.items():
            (self.copy / n).write_bytes(data)

    def json(self, name):
        return json.loads((self.copy / "00-Guidelines/13-Plan" / name).read_text())

    def dump(self, name, data):
        (self.copy / "00-Guidelines/13-Plan" / name).write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n")

    def oracle(self):
        """Independent include edges between owner batches and package prerequisites."""
        batches, packages = self.json("batches.json"), self.json("packages.json")
        owner = {t: b["id"] for b in batches["batches"] for t in b["targets"]}
        edges = {(owner[h], owner[d], h, d) for h, ds in scan(self.copy).items() if h in owner for d in ds
                 if d in owner and owner[d] != owner[h]}
        bpk = {b: p["id"] for p in packages["packages"] for b in p["batches"]}
        pre = {b["id"]: b["prerequisites"] for b in batches["batches"]}
        deps = {p["id"]: {bpk[d] for b in p["batches"] for d in pre[b]} - {p["id"]} for p in packages["packages"]}
        return batches, packages, edges, deps

    def render(self):
        _, packages = self.plan.load(self.copy)
        (self.copy / "01-prompts.md").write_text(self.plan.rendered_file(self.copy, packages))

    def test_plan_check_real_copy_passes(self):
        self.assertEqual(self.plan.check(self.copy), [])
        r = subprocess.run([sys.executable, str(self.copy / "00-Guidelines/13-Plan/plan.py"), "check"],
                           capture_output=True, text=True, timeout=120)
        self.assertEqual((r.returncode, r.stdout), (0, "plan check: ok\n"), r.stderr)

    def test_plan_include_edges_oracle(self):
        batches, _, edges, _ = self.oracle()
        got = self.plan.include_edges(self.copy, batches)
        self.assertEqual(self.plan.include_edges(self.copy, batches, self.plan.dependencies(self.copy).scan(self.copy)), got)
        self.assertEqual(len(got), len(set(got)))
        self.assertEqual(set(got), edges)
        self.assertGreater(len(edges), 0)
        for b, d, _, _ in edges:
            self.assertIn(d, next(x for x in batches["batches"] if x["id"] == b)["prerequisites"])

    def pairs(self):
        """Include-derived (batch, prerequisite) pairs: (single-edge pairs, pairs also reachable transitively)."""
        batches, _, edges, _ = self.oracle()
        pre = {b["id"]: b["prerequisites"] for b in batches["batches"]}
        count = {}
        for b, d, h, i in edges:
            count.setdefault((b, d), []).append((h, i))

        def via(b, d):
            seen, todo = set(), [x for x in pre[b] if x != d]
            while todo:
                x = todo.pop()
                if x not in seen:
                    seen.add(x)
                    todo += pre[x]
            return d in seen
        return count, sorted(p for p in count if len(count[p]) == 1), sorted(p for p in count if via(*p))

    def remove(self, pairs):
        batches = self.json("batches.json")
        for b, d in pairs:
            entry = next(x for x in batches["batches"] if x["id"] == b)
            entry["prerequisites"].remove(d)
        self.dump("batches.json", batches)

    def undeclared(self, errors):
        out = set()
        for e in errors:
            m = re.fullmatch(r"Undeclared include edge: (.+) -> (.+); add (\S+) to the prerequisites of (\S+) "
                             r"\(plan\.py doctor --fix\)", e)
            self.assertIsNotNone(m, f"unexpected check error: {e}")
            out.add((m[4], m[3], m[1], m[2]))
        return out

    def test_plan_check_undeclared_include_edge(self):
        count, single, transitive = self.pairs()
        self.assertTrue(single and transitive, "fixture repository lacks single-edge or transitive include pairs")
        first = next((p for p in single if p not in transitive), single[0])
        chosen = [first, next(p for p in transitive if p != first)]
        for pair in chosen:
            with self.subTest(pair=pair, transitive=pair in transitive):
                self.restore()
                self.remove([pair])
                errors = self.plan.check(self.copy)
                self.assertEqual(len(errors), len(count[pair]), errors)
                self.assertEqual(self.undeclared(errors), {(*pair, h, i) for h, i in count[pair]})
        self.assertEqual(len(count[first]), 1)

    def test_plan_doctor_fix_restores_prerequisites(self):
        _, single, transitive = self.pairs()
        pairs = sorted({single[0], transitive[-1]})
        self.remove(pairs)
        before = (self.copy / "00-Guidelines/13-Plan/batches.json").read_bytes()
        _, fixed = self.plan.doctor(self.copy, fix=False)
        self.assertEqual(fixed, [])
        self.assertEqual((self.copy / "00-Guidelines/13-Plan/batches.json").read_bytes(), before)
        _, fixed = self.plan.doctor(self.copy, fix=True)
        self.assertEqual(fixed[0], f"Added {len(pairs)} include-derived batch prerequisites: "
                         + ", ".join(f"{b} -> {d}" for b, d in pairs))
        batches = self.json("batches.json")
        original = json.loads(self.saved["00-Guidelines/13-Plan/batches.json"])
        for new, old in zip(batches["batches"], original["batches"]):
            self.assertEqual(set(new["prerequisites"]), set(old["prerequisites"]), new["id"])
        self.assertEqual(self.plan.check(self.copy), [])
        self.assertEqual(self.plan.doctor(self.copy, fix=True)[1], [])  # idempotent

    def move(self, mover, before):
        packages = self.json("packages.json")
        pl = packages["packages"]
        item = next(p for p in pl if p["id"] == mover)
        pl.remove(item)
        pl.insert(next(i for i, p in enumerate(pl) if p["id"] == before), item)
        self.dump("packages.json", packages)
        self.render()
        return [p["id"] for p in pl]

    def set_status(self, ident, status):
        packages = self.json("packages.json")
        next(p for p in packages["packages"] if p["id"] == ident)["status"] = status
        self.dump("packages.json", packages)
        self.render()

    def late_oracle(self):
        """Expected late errors: prerequisite after its dependent while neither is verified."""
        _, packages, _, deps = self.oracle()
        ids = [p["id"] for p in packages["packages"]]
        status = {p["id"]: p["status"] for p in packages["packages"]}
        return sorted(f"Prerequisite scheduled late: {p} -> {d}" for p in deps for d in deps[p]
                      if ids.index(d) > ids.index(p) and "verified" not in (status[d], status[p]))

    def test_plan_check_late_only_while_both_open(self):
        _, packages, _, deps = self.oracle()
        status = {p["id"]: p["status"] for p in packages["packages"]}
        pick = lambda sp, sd: next((p, d) for p in deps for d in sorted(deps[p]) if sp(status[p]) and sd(status[d]))
        verified = lambda s: s == "verified"
        scenarios = [("both open", pick(lambda s: s == "planned", lambda s: not verified(s)), None, True),
                     ("dependent planned, prerequisite verified", pick(lambda s: s == "planned", verified), None, False),
                     ("both verified", pick(verified, verified), None, False),
                     ("dependent verified, prerequisite in-progress", pick(verified, verified), "in-progress", False)]
        for label, (mover, before), prereq_status, error in scenarios:
            with self.subTest(label, mover=mover, before=before):
                self.restore()
                self.move(mover, before)
                if prereq_status:
                    self.set_status(before, prereq_status)
                errors, msg = self.plan.check(self.copy), f"Prerequisite scheduled late: {mover} -> {before}"
                self.assertEqual(sorted(errors), self.late_oracle())
                self.assertEqual(msg in errors, error, errors)

    def test_plan_rank_keeps_schedule_and_waived_inversions(self):
        batches, packages = self.plan.load(self.copy)
        ids = [p["id"] for p in packages["packages"]]
        self.plan.rank(batches, packages)
        self.assertEqual([p["id"] for p in packages["packages"]], ids)
        _, _, _, deps = self.oracle()
        status = {p["id"]: p["status"] for p in packages["packages"]}
        mover, before = next((p, d) for p in deps for d in sorted(deps[p]) if status[p] == status[d] == "verified")
        moved = self.move(mover, before)
        batches, packages = self.plan.load(self.copy)
        self.plan.rank(batches, packages)
        ranked = [p["id"] for p in packages["packages"]]
        self.assertEqual(ranked, moved)
        self.assertLess(ranked.index(mover), ranked.index(before))
        self.restore()  # an open inversion is repaired
        mover, before = next((p, d) for p in deps for d in sorted(deps[p]) if status[p] == "planned" and status[d] != "verified")
        self.move(mover, before)
        batches, packages = self.plan.load(self.copy)
        self.plan.rank(batches, packages)
        ranked = [p["id"] for p in packages["packages"]]
        self.assertGreater(ranked.index(mover), ranked.index(before))
        self.assertEqual(sorted(ranked), sorted(moved))

    def test_plan_set_reranks(self):
        batches, packages = self.plan.load(self.copy)
        deps, status = self.plan.prereqs(batches, packages), {p["id"]: p["status"] for p in packages["packages"]}
        ready = [p for p in packages["packages"] if p["status"] == "planned" and p["phase"] == "Contest core"
                 and all(status[d] == "verified" for d in deps[p["id"]])]
        mover = ready[-1]["id"]
        subprocess.run([sys.executable, self.copy / "00-Guidelines/13-Plan/plan.py", "set", mover, "in-progress"],
                       check=True, capture_output=True)
        batches, packages = self.plan.load(self.copy)
        ids = [p["id"] for p in packages["packages"]]
        planned = [p["id"] for p in packages["packages"] if p["phase"] == "Contest core" and p["status"] == "planned"]
        self.assertLess(ids.index(mover), min(map(ids.index, planned)), "a started package precedes the planned ones")
        self.plan.rank(batches, packages)
        self.assertEqual([p["id"] for p in packages["packages"]], ids, "set leaves the schedule ranked")

    def add_prerequisite(self, batch, prereq):
        batches = self.json("batches.json")
        next(x for x in batches["batches"] if x["id"] == batch)["prerequisites"].append(prereq)
        self.dump("batches.json", batches)

    def cycle_nodes(self, errors):
        self.assertEqual(len(errors), 1, errors)
        m = re.fullmatch(r"Batch prerequisite cycle: (.+)", errors[0])
        self.assertIsNotNone(m, errors)
        nodes = m[1].split(" -> ")
        pre = {b["id"]: b["prerequisites"] for b in self.json("batches.json")["batches"]}
        self.assertEqual(nodes[0], nodes[-1])
        self.assertEqual(len(nodes) - 1, len(set(nodes)))
        for x, y in zip(nodes, nodes[1:]):
            self.assertIn(y, pre[x], f"{x} -> {y} is not a prerequisite edge")
        return set(nodes)

    def test_plan_check_batch_cycles(self):
        self.assertFalse([e for e in self.plan.check(self.copy) if "cycle" in e])
        self.add_prerequisite("ST06", "ST03")  # ST03 -> ST06 exists: a 2-cycle
        self.assertEqual(self.cycle_nodes(self.plan.check(self.copy)), {"ST03", "ST06"})
        self.restore()
        batches, packages, _, _ = self.oracle()
        pre = {b["id"]: b["prerequisites"] for b in batches["batches"]}
        pkg = {b: p["id"] for p in packages["packages"] for b in p["batches"]}
        status = {p["id"]: p["status"] for p in packages["packages"]}
        paths = {}

        def count(x, target):  # number of prerequisite paths x ->* target (the real graph is acyclic)
            if (x, target) not in paths:
                paths[x, target] = 1 if x == target else sum(count(y, target) for y in pre[x])
            return paths[x, target]
        a, b, c = next((a, b, c) for a in sorted(pre) for b in pre[a] for c in pre[b]
                       if len({pkg[a], pkg[b], pkg[c]}) == 3 and count(a, c) == 1
                       and "verified" in (status[pkg[a]], status[pkg[c]]))
        self.add_prerequisite(c, a)
        self.assertEqual(self.cycle_nodes(self.plan.check(self.copy)), {a, b, c})

    def test_plan_check_conditional_include(self):
        batches, _, _, _ = self.oracle()
        owned = sorted(t for b in batches["batches"] for t in b["targets"] if t.startswith("05-Mathematics/"))
        host = next(t for t in owned if (self.copy / t).exists())
        original = (self.copy / host).read_bytes()
        self.addCleanup((self.copy / host).write_bytes, original)
        for block, n in (('#ifdef LOCAL\n#include "../01-Core/01-template.hpp"\n#endif\n', 1),
                         ('#if 0\n#else\n#include "../01-Core/01-template.hpp"\n#ifndef X\n'
                          '#include "../01-Core/01-template.hpp"\n#endif\n#endif\n', 2),
                         ('/*\n#ifdef LOCAL\n*/\n#include "../01-Core/01-template.hpp"\n', 0),
                         ('#ifdef LOCAL\n  #  include "../01-Core/01-template.hpp"\n#endif\n', 1)):
            with self.subTest(block=block):
                (self.copy / host).write_bytes(original + block.encode())
                self.assertEqual(self.plan.check(self.copy), [
                    f"Conditional include (the dependency graph treats every include as active): {host} -> "
                    "01-Core/01-template.hpp"] * n)

    def test_plan_include_edges_unowned_files(self):
        batches, _, edges, _ = self.oracle()
        owned = {t for b in batches["batches"] for t in b["targets"]}
        host = next(t for t in sorted(owned) if t.startswith("05-Mathematics/") and (self.copy / t).exists())
        folder = self.copy / "05-Mathematics"
        self.assertNotIn("05-Mathematics/99-all.hpp", owned)
        original = (self.copy / host).read_bytes()
        self.addCleanup((self.copy / host).write_bytes, original)
        self.addCleanup((folder / "90-unowned.hpp").unlink, missing_ok=True)
        (folder / "90-unowned.hpp").write_text('#pragma once\n#include "' + host.split("/")[1] + '"\n')
        (self.copy / host).write_bytes(original + b'#include "90-unowned.hpp"\n#include "99-all.hpp"\n')
        got = self.plan.include_edges(self.copy, batches)
        self.assertEqual(set(got), edges)
        self.assertEqual(self.plan.check(self.copy), [])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--rounds", type=int, default=1000, help="random DAGs in stress mode")
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Fixtures, Repository, PlanRules):
        names = [n for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != "quick" or n not in QUICK_SKIP]
        suite.addTests(case(n) for n in names)
    print(f"dependency checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} CXX={CXX} "
          f"tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f"\033[{32 if ok else 31}m" if tty else "") + ("PASS" if ok else "FAIL") + ("\033[0m" if tty else ""), flush=True)
    raise SystemExit(not result.wasSuccessful())
