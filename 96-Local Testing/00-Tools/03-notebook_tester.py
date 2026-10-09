#!/usr/bin/env python3
"""Offline fixture checks for the Team Notebook generator."""

from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path
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


if __name__ == "__main__":
    unittest.main()
