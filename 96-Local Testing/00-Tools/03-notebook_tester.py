#!/usr/bin/env python3
"""Offline fixture checks for the Team Notebook generator."""

from __future__ import annotations

import importlib.util
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from html.parser import HTMLParser
from types import SimpleNamespace
from unittest.mock import patch
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "98-Team Notebook" / "01-notebook.py"
SPEC = importlib.util.spec_from_file_location("team_notebook", TOOL)
assert SPEC and SPEC.loader
NOTEBOOK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(NOTEBOOK)


class Page(HTMLParser):
    """Checkbox value -> (data-deps, label title, checked, warning text), and the order rows, of a page()."""

    def __init__(self, html: str) -> None:
        super().__init__()
        self.title, self.last, self.warn, self.found, self.rows, self.script = None, None, False, {}, [], ""
        self.feed(html)

    def handle_starttag(self, tag, attrs) -> None:
        attrs = dict(attrs)
        if tag == "label" and attrs.get("class") == "file":
            self.title = attrs.get("title")
        elif tag == "input" and attrs.get("name") == "selected":
            self.last = attrs["value"]
            self.found[self.last] = [json.loads(attrs["data-deps"]), self.title, "checked" in attrs, ""]
        elif tag == "small" and attrs.get("class") == "warn":
            self.warn = True
        elif tag == "li" and "data-id" in attrs:
            self.rows.append(attrs["data-id"])
        self.in_script = tag == "script"

    def handle_endtag(self, tag) -> None:
        self.warn = self.warn and tag != "small"
        self.in_script = False

    def handle_data(self, data) -> None:
        if self.warn:
            self.found[self.last][3] += data
        if getattr(self, "in_script", False):
            self.script += data


SHIM = r"""const data = JSON.parse(require('fs').readFileSync(0, 'utf8'));
class El {
  constructor(tag, props = {}) {
    this.tagName = tag; this.children = []; this.parentNode = null; this.dataset = {}; this.listeners = {};
    this.attributes = {}; this.textContent = ''; Object.assign(this, props);
  }
  appendChild(x) { if (x.parentNode) x.remove(); x.parentNode = this; this.children.push(x); return x; }
  append(...xs) { for (const x of xs) this.appendChild(x); }
  insertBefore(x, ref) {
    if (x.parentNode) x.remove();
    const i = this.children.indexOf(ref);
    if (i < 0) throw new Error('shim: insertBefore reference is not a child');
    x.parentNode = this; this.children.splice(i, 0, x); return x;
  }
  remove() { if (this.parentNode) { const c = this.parentNode.children; c.splice(c.indexOf(this), 1); this.parentNode = null; } }
  sibling(d) { if (!this.parentNode) return null; const c = this.parentNode.children; return c[c.indexOf(this) + d] || null; }
  get previousElementSibling() { return this.sibling(-1); }
  get nextElementSibling() { return this.sibling(1); }
  setAttribute(k, v) { this.attributes[k] = String(v); }
  addEventListener(type, f) { (this.listeners[type] = this.listeners[type] || []).push(f); }
  descendants() { return this.children.flatMap(c => [c, ...c.descendants()]); }
  test(sel) {
    let m = /^input\[name="([^"]+)"\]$/.exec(sel);
    if (m) return this.tagName === 'input' && this.name === m[1];
    if (sel === 'button[data-move]') return this.tagName === 'button' && 'move' in this.dataset;
    if (/^[a-z]+$/.test(sel)) return this.tagName === sel;
    throw new Error('shim: unsupported selector ' + sel);
  }
  matches(sel) { return sel.split(',').some(s => this.test(s.trim())); }
  closest(sel) { for (let x = this; x; x = x.parentNode) if (x.matches(sel)) return x; return null; }
  querySelectorAll(sel) { return this.descendants().filter(x => x.matches(sel)); }
  querySelector(sel) { return this.querySelectorAll(sel)[0] || null; }
}
const body = new El('body');
const form = body.appendChild(new El('form'));
const checks = data.boxes.map(([value, deps, checked]) => form.appendChild(
  new El('input', {type: 'checkbox', name: 'selected', value, checked, dataset: {deps: JSON.stringify(deps)}})));
const ordered = form.appendChild(new El('ol', {id: 'selected-order'}));
for (const id of data.rows) {
  const li = ordered.appendChild(new El('li', {dataset: {id}}));
  const controls = new El('div');
  for (const move of ['up', 'down']) controls.appendChild(new El('button', {type: 'button', dataset: {move}}));
  li.append(new El('span', {textContent: id}), controls);
}
const document = {
  querySelector: sel => body.querySelector(sel),
  getElementById: id => body.descendants().find(x => x.id === id) || null,
  createElement: tag => new El(tag),
};
new Function('document', data.script)(document);
const fire = (el, type, target) => {
  if (!(el.listeners[type] || []).length) throw new Error('shim: no ' + type + ' listener');
  for (const f of el.listeners[type]) f({target});
};
const box = value => { const x = checks.find(c => c.value === value); if (!x) throw new Error('shim: no box ' + value); return x; };
const out = [];
for (const [action, value] of data.steps) {
  if (action === 'tick' || action === 'untick') {
    const c = box(value); c.checked = action === 'tick'; fire(form, 'change', c);
  } else if (action === 'up' || action === 'down') {
    const li = ordered.children.find(x => x.dataset.id === value);
    fire(ordered, 'click', li.descendants().find(x => x.dataset.move === action));
  } else if (action === 'submit') {
    fire(form, 'submit', form);
  } else throw new Error('shim: unknown step ' + action);
  out.push({rows: ordered.children.map(li => li.dataset.id), checked: checks.filter(c => c.checked).map(c => c.value)});
}
out.push(form.children.filter(x => x.type === 'hidden').map(x => [x.name, x.value]));
process.stdout.write(JSON.stringify(out));
"""


class NotebookTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.here = self.root / "98-Team Notebook"
        self.here.mkdir()
        self.notes = self.here / "03-notes.md"
        self.notes.write_text("# Checklist\n- Check bounds\n", encoding="utf-8")
        for name, value in (("ROOT", self.root), ("HERE", self.here),
                            ("NOTES", self.notes), ("CONFIG", self.here / "02-selection.json"),
                            ("TEX", self.here / "04-notebook.tex")):
            temporary = patch.object(NOTEBOOK, name, value)
            temporary.start()
            self.addCleanup(temporary.stop)

    def source(self, relative: str, content: str = "// canonical\n") -> Path:
        path = self.root / relative
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def config(self, selected: list[str]) -> dict:
        cfg = dict(NOTEBOOK.DEFAULT)
        cfg["selected"] = selected
        return cfg

    def test_discovery_prunes_old_build_cache_and_symlinks(self) -> None:
        self.source("01-Core/a.hpp")
        self.source("03-Geometry/mini hull-v2.hpp")
        self.source("08-Python/fast input.py", "print(1)\n")
        for directory in ("OLD", "97-Legacy", "build", "__pycache__", "nested/Old", "nested/97-legacy"):
            self.source(f"01-Core/{directory}/hidden.hpp")
        self.source("01-Core/97-Legacy/old.cpp")
        self.source("09-Contest Testing/test.cpp")
        self.source("01-Core/readme.md")
        outside = self.source("outside.hpp")
        (self.root / "01-Core" / "linked.hpp").symlink_to(outside)
        (self.root / "01-Core" / "linked_dir").symlink_to(self.root / "03-Geometry", target_is_directory=True)

        self.assertEqual(set(NOTEBOOK.sources()), {
            "01-Core/a.hpp", "03-Geometry/mini hull-v2.hpp", "08-Python/fast input.py",
        })

    def test_selection_order_and_rejection(self) -> None:
        self.source("01-Core/a.hpp")
        self.source("01-Core/b.hpp")
        available = NOTEBOOK.sources()
        selected = ["01-Core/a.hpp", "01-Core/b.hpp"]
        values = {"selected": selected, "order_enabled": ["1"],
                  "ordered_selected": list(reversed(selected))}
        self.assertEqual(NOTEBOOK.form_selection(values), list(reversed(selected)))
        self.assertEqual(NOTEBOOK.form_selection({"selected": selected}), selected)
        with self.assertRaises(NOTEBOOK.NotebookError):
            NOTEBOOK.form_selection({"selected": selected, "order_enabled": ["1"],
                                     "ordered_selected": [selected[0]]})
        for bad in (["../outside.hpp"], ["01-Core/OLD/hidden.hpp"], ["01-Core/97-Legacy/old.cpp"], selected + [selected[0]]):
            with self.subTest(bad=bad), self.assertRaises(NOTEBOOK.NotebookError):
                NOTEBOOK.validate(self.config(bad), available)

    def test_config_round_trip_and_ordered_single_column_tex(self) -> None:
        self.source("01-Core/first source.hpp", "// unique algorithm sentinel\n")
        self.source("08-Python/quick-input.py", "print(1)\n")
        available = NOTEBOOK.sources()
        selected = ["08-Python/quick-input.py", "01-Core/first source.hpp"]
        cfg = self.config(selected)
        cfg.update(columns=1, print_mode=True, body_size=9.0, code_size=7.0)
        NOTEBOOK.save(cfg, available)
        self.assertEqual(NOTEBOOK.load(available), cfg)
        self.assertFalse((self.here / "02-selection.json.tmp").exists())

        tex = NOTEBOOK.generate(cfg, available).read_text(encoding="utf-8")
        body = tex.split(r"\begin{document}", 1)[1]
        self.assertNotIn(r"\begin{multicols}", body)
        self.assertNotIn(r"\end{multicols}", body)
        self.assertLess(tex.index("../08-Python/quick-input.py"),
                        tex.index("../01-Core/first source.hpp"))
        self.assertIn(r"\lstinputlisting[language=Python]{\detokenize{../08-Python/quick-input.py}}", tex)
        self.assertIn(r"\lstinputlisting[language=C++]{\detokenize{../01-Core/first source.hpp}}", tex)
        self.assertIn(r"\definecolor{accent}{HTML}{333333}", tex)
        self.assertIn(r"\sourceheading{Checklist}", tex)
        self.assertNotIn("unique algorithm sentinel", tex)
        html = NOTEBOOK.page(cfg, available, "test-token")
        self.assertLess(html.index('data-id="08-Python/quick-input.py"'),
                        html.index('data-id="01-Core/first source.hpp"'))
        self.assertIn('name="ordered_selected"', html)

    def test_build_reports_missing_prerequisites_without_running_tex(self) -> None:
        self.source("01-Core/a.hpp")
        available = NOTEBOOK.sources()
        cfg = self.config(["01-Core/a.hpp"])
        with patch.object(NOTEBOOK.shutil, "which", return_value=None), \
                patch.object(NOTEBOOK, "font_status", side_effect=lambda font: font != "Inter"), \
                patch.object(NOTEBOOK.subprocess, "run") as run:
            message, status = NOTEBOOK.build(cfg, available)
        self.assertEqual(status, 1)
        self.assertIn("LuaLaTeX executable", message)
        self.assertIn("font 'Inter'", message)
        self.assertTrue((self.here / "04-notebook.tex").exists())
        run.assert_not_called()

    def test_build_reads_real_page_count_and_keeps_over_budget_output(self) -> None:
        self.source("01-Core/a.hpp")
        available = NOTEBOOK.sources()
        cfg = self.config(["01-Core/a.hpp"])
        cfg["max_pages"] = 3
        result = SimpleNamespace(returncode=0,
                                 stdout="Output written on 04-notebook.pdf (4 pages, 123 bytes).",
                                 stderr="")
        with patch.object(NOTEBOOK.shutil, "which", return_value="/usr/bin/lualatex"), \
                patch.object(NOTEBOOK, "font_status", return_value=True), \
                patch.object(NOTEBOOK.subprocess, "run", return_value=result) as run:
            message, status = NOTEBOOK.build(cfg, available)
        self.assertEqual(status, 2)
        self.assertIn("4 page(s)", message)
        self.assertIn("Over budget by 1 page(s)", message)
        self.assertIn("../01-Core/a.hpp", (self.here / "04-notebook.tex").read_text())
        self.assertEqual(run.call_args.kwargs["cwd"], self.here)

    TEMPLATE, MODINT, BARRETT = "01-Core/01-template.hpp", "01-Core/05-modint.hpp", "01-Core/03-barrett.hpp"
    A, B, C = "05-Mathematics/01-mod_a.hpp", "05-Mathematics/02-b.hpp", "05-Mathematics/03-c.hpp"

    def dependency_fixture(self) -> None:
        """b -> a -> modint -> barrett -> template; c is a Compact header whose barrett include is optional."""
        inc = lambda *names: "".join(f'#include "{n}"\n' for n in names)
        self.source(self.TEMPLATE, "#pragma once\n")
        self.source(self.BARRETT, inc("01-template.hpp"))
        self.source(self.MODINT, inc("01-template.hpp", "03-barrett.hpp"))
        self.source(self.A, inc("../01-Core/01-template.hpp", "../01-Core/05-modint.hpp"))
        self.source(self.B, inc("../01-Core/01-template.hpp", "01-mod_a.hpp"))
        self.source(self.C, inc("../01-Core/01-template.hpp", "../01-Core/03-barrett.hpp")
                    + "inline int mulCompact(int a) { return a; }\n")
        self.source("05-Mathematics/97-Legacy/04-old.hpp", inc("../02-b.hpp"))
        self.source("08-Python/x.py", "print(1)\n")
        self.source("05-Mathematics/05-tool.cpp", inc("02-b.hpp"))

    def test_with_dependencies_typing_order_and_user_order(self) -> None:
        self.dependency_fixture()
        t, m, r, a, b, c = self.TEMPLATE, self.MODINT, self.BARRETT, self.A, self.B, self.C
        cases = [
            ([], [], []),
            ([b], [t, r, m, a, b], [t, r, m, a]),
            ([t], [t], []),
            (["08-Python/x.py", b], ["08-Python/x.py", t, r, m, a, b], [t, r, m, a]),
            # A dependency selected later keeps its place and is not duplicated.
            ([b, m], [t, r, a, b, m], [t, r, a]),
            ([m, b, "05-Mathematics/05-tool.cpp"], [t, r, m, a, b, "05-Mathematics/05-tool.cpp"], [t, r, a]),
            ([c, b], [t, r, c, m, a, b], [t, r, m, a]),
            ([a, t, c], [r, m, a, t, c], [r, m]),
        ]
        for selected, want, added in cases:
            with self.subTest(selected=selected):
                self.assertEqual(NOTEBOOK.with_dependencies(selected, NOTEBOOK.sources()), (want, added))
                self.assertEqual(NOTEBOOK.with_dependencies(want, NOTEBOOK.sources()), (want, []))  # idempotent
                self.assertEqual(len(want), len(set(want)))
        out, _ = NOTEBOOK.with_dependencies([b, c], NOTEBOOK.sources())
        graph = NOTEBOOK.DEPS.headers(self.root)
        pos = {x: i for i, x in enumerate(out)}
        for item in out:  # with no user-placed dependency, every include precedes its user
            for dep in graph.get(item, []):
                self.assertLess(pos[dep], pos[item], (dep, item, out))
        NOTEBOOK.validate(self.config(out), NOTEBOOK.sources())

    def test_needs_marks_unselected_template_and_optional(self) -> None:
        self.dependency_fixture()
        graph = NOTEBOOK.DEPS.headers(self.root)
        t, m, r, a, b, c = self.TEMPLATE, self.MODINT, self.BARRETT, self.A, self.B, self.C
        self.assertEqual(NOTEBOOK.needs(graph, b, [t, a, b]), a)
        self.assertEqual(NOTEBOOK.needs(graph, b, [b]), f"{t} (not selected), {a} (not selected)")
        self.assertEqual(NOTEBOOK.needs(graph, m, [t, m]), f"{r} (not selected)")
        self.assertEqual(NOTEBOOK.needs(graph, c, [t, r, c]), f"{r} (not for the Compact twins)")
        self.assertEqual(NOTEBOOK.needs(graph, c, [c]),
                         f"{t} (not selected), {r} (not for the Compact twins) (not selected)")
        self.assertEqual(NOTEBOOK.needs(graph, r, [t, r]), "")
        self.assertEqual(NOTEBOOK.needs(graph, t, [t]), "")
        self.assertEqual(NOTEBOOK.needs(graph, "08-Python/x.py", ["08-Python/x.py"]), "")

    def test_generate_writes_needs_lines(self) -> None:
        self.dependency_fixture()
        available = NOTEBOOK.sources()
        selected = ["08-Python/x.py", self.TEMPLATE, self.BARRETT, self.C, self.B]
        tex = NOTEBOOK.generate(self.config(selected), available).read_text(encoding="utf-8")
        lines = tex.splitlines()
        heads = [i for i, line in enumerate(lines) if line.startswith(r"\sourceheading{") and "/" in line]
        self.assertEqual(len(heads), len(selected))
        needs = {}
        for i, item in zip(heads, selected):
            self.assertEqual(lines[i], r"\sourceheading{" + NOTEBOOK.tex_escape(item) + "}")
            if lines[i + 1].startswith(r"{\footnotesize"):
                needs[item] = lines[i + 1]
                self.assertTrue(lines[i + 2].startswith(r"\lstinputlisting"), lines[i + 2])
            else:
                self.assertTrue(lines[i + 1].startswith(r"\lstinputlisting"), lines[i + 1])
        prefix, suffix = r"{\footnotesize\textcolor{muted}{Needs: ", r"}\par}"
        self.assertEqual(needs, {
            self.C: prefix + r"01-Core/03-barrett.hpp (not for the Compact twins)" + suffix,
            self.B: prefix + r"05-Mathematics/01-mod\_a.hpp (not selected)" + suffix,
        })

    def test_page_dependency_attributes_and_script(self) -> None:
        self.dependency_fixture()
        available = NOTEBOOK.sources()
        html = NOTEBOOK.page(self.config([self.B]), available, "token")
        found = Page(html).found
        self.assertEqual(set(found), set(available))
        graph = NOTEBOOK.DEPS.headers(self.root)
        for item, (deps, title, checked, warn) in found.items():
            with self.subTest(item=item):
                want = NOTEBOOK.DEPS.closure(graph, item)[:-1] if item in graph else []
                self.assertEqual(deps, want)
                self.assertEqual(title, "Needs: " + ", ".join(want) if want else None)
                self.assertEqual(checked, item == self.B)
                self.assertEqual(warn, "")
        self.assertEqual(found[self.B][0], [self.TEMPLATE, self.BARRETT, self.MODINT, self.A])
        self.assertIn("JSON.parse(event.target.dataset.deps)", html)
        self.assertIn("if (dep && !dep.checked) { dep.checked = true; addRow(id); }", html)

    def test_page_script_ticks_dependencies_under_node(self) -> None:
        node = shutil.which("node") or shutil.which("nodejs")
        if not node:
            print("\nSKIP node: not installed; the page's JS tick handler was not executed", flush=True)
            self.skipTest("SKIP node not installed")
        self.dependency_fixture()
        t, m, r, a, b, c, x = self.TEMPLATE, self.MODINT, self.BARRETT, self.A, self.B, self.C, "08-Python/x.py"
        parsed = Page(NOTEBOOK.page(self.config([t]), NOTEBOOK.sources(), "token"))
        self.assertEqual(parsed.rows, [t])
        steps = [["tick", b], ["untick", m], ["tick", x], ["tick", c], ["up", c], ["down", t], ["submit", ""]]
        data = {"boxes": [[k, v[0], v[2]] for k, v in parsed.found.items()], "rows": parsed.rows,
                "script": parsed.script, "steps": steps}
        run = subprocess.run([node, "-e", SHIM], input=json.dumps(data), capture_output=True, text=True, timeout=60)
        self.assertEqual(run.returncode, 0, f"node shim failed:\n{run.stderr}")
        states = json.loads(run.stdout)
        rows = [[t, r, m, a, b], [t, r, a, b], [t, r, a, b, x], [t, r, a, b, x, c], [t, r, a, b, c, x],
                [r, t, a, b, c, x], [r, t, a, b, c, x]]
        for step, state, want in zip(steps, states, rows):
            with self.subTest(step=step):
                self.assertEqual(state["rows"], want)  # deps first, then the header; an unticked dep stays out
                self.assertEqual(set(state["checked"]), set(want))
        self.assertEqual(states[-1], [["order_enabled", "1"]] + [["ordered_selected", i] for i in rows[-1]])

    def test_unselectable_dependency_errors_and_warns(self) -> None:
        self.dependency_fixture()
        dangling, legacy = "05-Mathematics/06-dangling.hpp", "05-Mathematics/07-legacy-user.hpp"
        self.source(dangling, '#include "../01-Core/01-template.hpp"\n#include "missing.hpp"\n')
        self.source(legacy, '#include "97-Legacy/04-old.hpp"\n')
        available = NOTEBOOK.sources()
        for selected, item, dep in (([dangling], dangling, "05-Mathematics/missing.hpp"),
                                    ([self.B, legacy], legacy, "05-Mathematics/97-Legacy/04-old.hpp")):
            with self.subTest(selected=selected), self.assertRaises(NOTEBOOK.NotebookError) as raised:
                NOTEBOOK.with_dependencies(selected, available)
            self.assertEqual(str(raised.exception), f"{item} needs {dep}, which is not a selectable source")
        found = Page(NOTEBOOK.page(self.config([]), available, "token")).found
        self.assertEqual(found[dangling][3], "needs 05-Mathematics/missing.hpp, which is not selectable")
        self.assertEqual(found[legacy][3], "needs 05-Mathematics/97-Legacy/04-old.hpp, which is not selectable")
        self.assertEqual([k for k, v in found.items() if v[3]], [dangling, legacy])
        NOTEBOOK.save(self.config([self.TEMPLATE]), available)
        saved = NOTEBOOK.CONFIG.read_bytes()
        code, out, err = self.run_main("save", "--select", self.B, "--select", dangling)
        self.assertEqual((code, out), (1, ""))
        self.assertIn(f"{dangling} needs 05-Mathematics/missing.hpp", err)
        self.assertEqual(NOTEBOOK.CONFIG.read_bytes(), saved)
        code, out, err = self.run_main("save", "--select", dangling, "--no-deps")
        self.assertEqual((code, out, err), (0, "Saved 02-selection.json: 1 source file(s).\n", ""))

    def run_main(self, *args: str) -> tuple[int, str, str]:
        import contextlib
        import io
        out, err = io.StringIO(), io.StringIO()
        with patch.object(sys, "argv", ["01-notebook.py", *args]), contextlib.redirect_stdout(out), \
                contextlib.redirect_stderr(err):
            try:
                code = NOTEBOOK.main()
            except SystemExit as exc:  # argparse errors
                code = exc.code
        return code, out.getvalue(), err.getvalue()

    def test_cli_save_adds_dependencies_unless_disabled(self) -> None:
        self.dependency_fixture()
        available = NOTEBOOK.sources()
        NOTEBOOK.save(self.config([]), available)
        t, m, r, a, b = self.TEMPLATE, self.MODINT, self.BARRETT, self.A, self.B
        code, out, err = self.run_main("save", "--select", b)
        self.assertEqual((code, err), (0, ""))
        self.assertEqual(out, f"Added dependencies: {t}, {r}, {m}, {a}\nSaved 02-selection.json: 5 source file(s).\n")
        self.assertEqual(NOTEBOOK.load(available)["selected"], [t, r, m, a, b])
        code, out, err = self.run_main("save", "--select", b, "--no-deps")
        self.assertEqual((code, out, err), (0, "Saved 02-selection.json: 1 source file(s).\n", ""))
        self.assertEqual(NOTEBOOK.load(available)["selected"], [b])
        code, out, _ = self.run_main("save", "--select", m, "--select", b, "--deps")
        self.assertEqual((code, out.splitlines()[0]), (0, f"Added dependencies: {t}, {r}, {a}"))
        self.assertEqual(NOTEBOOK.load(available)["selected"], [t, r, m, a, b])
        code, out, _ = self.run_main("save", "--select", t, "--select", "08-Python/x.py")
        self.assertEqual((code, out), (0, "Saved 02-selection.json: 2 source file(s).\n"))
        code, out, _ = self.run_main("save", "--pages", "7")  # no --select keeps the selection
        self.assertEqual(NOTEBOOK.load(available)["selected"], [t, "08-Python/x.py"])
        code, out, err = self.run_main("save", "--select", "05-Mathematics/97-Legacy/04-old.hpp")
        self.assertEqual(code, 1)
        self.assertIn("Unknown or unsafe source ID", err)
        self.assertEqual(NOTEBOOK.load(available)["selected"], [t, "08-Python/x.py"])
        saved = NOTEBOOK.CONFIG.read_bytes()
        for bad, message in ((("--pages", "0"), "max_pages"), (("--body-size", "99"), "body_size"),
                             (("--title", ""), "title")):
            with self.subTest(bad=bad):  # dependencies would be added, but the save fails
                code, out, err = self.run_main("save", "--select", b, *bad)
                self.assertEqual((code, out), (1, ""))
                self.assertIn(message, err)
                self.assertEqual(NOTEBOOK.CONFIG.read_bytes(), saved)
        code, out, err = self.run_main("save", "--select", b, "--columns", "5")  # argparse rejects it first
        self.assertEqual((code, out), (2, ""))
        self.assertIn("--columns", err)
        self.assertEqual(NOTEBOOK.CONFIG.read_bytes(), saved)


if __name__ == "__main__":
    result = unittest.main(exit=False, verbosity=2).result
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    summary = (f"notebook: {result.testsRun} tests, {len(result.failures)} failures, {len(result.errors)} errors, "
               f"{len(result.skipped)} skipped")
    print((f"\033[{32 if ok else 31}m" if tty else "") + ("PASS " if ok else "FAIL ") + summary + ("\033[0m" if tty else ""),
          flush=True)
    raise SystemExit(not ok)
