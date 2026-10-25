#!/usr/bin/env python3
"""Checks for 98-Team Notebook/06-site.py, the documentation-site generator for Zensical and MkDocs Material.

quick: every unit and fixture test (pure helpers, git helpers on throw-away repositories, Site pages and targets, href,
link, relink, per-source heading ids, folder overviews and folder articles, header pages with Complexity, dependencies,
home page, files/nav/YAML, orphan articles, generate, engine() on fake engines, CLI with the engine call patched out),
5 seeded random fixture trees, the Python-Markdown heading-id oracle, Renderer (fixed and seeded fuzzed headings and
table rows checked against Python-Markdown itself; 400 headings per alphabet, 300 token headings and 300 tables, 3000,
2000 and 2000 in full, 40, 30 and 20 per --rounds in stress), the strict fixture builds (one per engine
executable: valid site, broken anchor and missing page fail, privacy plugin leaves no CDN URL when online), and every
repository oracle on a temporary generation of the real tree (inventory page set, evidence sections, Complexity
bullets, link and anchor resolution, per-source anchored links, Markdown rules, typing orders and line counts, includes
and includers, last-commit dates, heading ids) without the repository build.
full: quick plus 20 random trees and the strict build of the repository site with its built heading ids, per engine.
stress: full plus --rounds random trees (default 200; random include DAGs, tiers, notes, articles, folder articles,
complexity lines, evidence documents with random unicode, repeated and page-colliding headings, code spans, fences of
both kinds and links of every kind), three of them also built strictly with every engine.
Engines: every distinct executable (by real path) named `zensical` or `mkdocs` in any PATH directory, not only the
first, plus the paths in CP_SITE_ENGINES (os.pathsep-separated); each build test runs once per executable and SKIPs an
engine with none or an executable whose `#!` Python lacks the engine's modules. Builds run in a temporary directory,
which is also their working directory (the privacy plugin caches there). engine()/CLI tests use fake engine scripts in a
temporary bin directory (PATH = that directory) whose `#!` names bare venvs, with stub zensical, material or pymdownx
packages, created once per run; they never start an engine or a server.
Oracles: hand-computed fixture pages and engine messages; an independent heading-id scanner (pymdownx.superfences fence
rule: a bare closing line of the same run, an unclosed opener is text; Python-Markdown hash headings and toc text with
the pre-3.11 BACKTICK_RE code-span pass, a formulation independent of the generator's 3.11 scanner) validated against Python-Markdown (the `#!` Python of an engine, else this one, when it
imports markdown, pymdownx and yaml) and against built HTML; per-source anchors located by finding each copied document's
lines (links as labels) on its page; an independent link resolver over the generated files; a regex inventory parse; an
os.walk include scanner; an independent complexity-line parser; byte newline counts; `git log -1` per file. Modes/seeds
also come from CP_TEST_MODE/CP_TEST_SEED. Without Python-Markdown and yaml the oracle and YAML tests SKIP. No test
writes into the repository: generation goes to temporary directories, CLI tests use a module copy whose ROOT is a
fixture. Failing tests print the seed, mode and retained fixture directory. Tests whose docstrings start with
"Regression (P239)" reproduce fixed defects of 06-site.py.

Feature map: Emphasis/Heading -> test_emphasis_python_markdown, test_heading_pipeline_python_markdown; cells/table_rows
-> test_table_cell_links, test_math_and_escaped_links;
slug -> test_slug_cases, test_code_span_rule; heading_ids/anchors -> test_anchors_cases,
test_anchors_python_markdown_domain, test_anchors_python_markdown_edge_cases, test_repo_heading_ids,
test_build_fixture_strict, test_repo_strict_build_and_ids, test_random_trees; sections -> test_sections_cases,
test_repo_evidence_sections, test_random_trees; trimmed -> test_trimmed_cases, test_trimmed_title_after_blank_lines;
complexity -> test_complexity_cases, test_header_complexity, test_repo_complexity, test_repo_complexity_code_spans,
test_random_trees; items -> test_items_cases, test_header_page_dsu, test_repo_operations; page_name/folder_slug ->
test_names, test_repo_pages_match_inventory; repository/git/tracked -> test_repository_urls, test_missing_git,
test_tracked_files_only; changed -> test_changed_dates, test_header_last_commit_git_fixture, test_repo_last_commit; Site
pages/targets -> test_site_pages_targets, test_random_trees; href/link -> test_href_and_link, test_anchors_per_source;
map_ids/per-source anchors -> test_anchors_per_source, test_folder_article, test_inventory_anchor_beside_folder_article,
test_repo_copied_anchor_links, test_random_trees; relink -> test_relink_cases, test_relink_code_span_rule,
test_header_page_dsu, test_repo_links_resolve, test_random_trees; folder overview and folder article ->
test_overview_pages, test_folder_article; header page -> test_header_page_dsu, test_header_page_python,
test_header_section_order, test_header_complexity; dependencies -> test_dependencies_sections, test_repo_dependencies;
home -> test_home_page; files/nav/YAML/plugins/assets -> test_files_nav_yaml, test_files_orphan_articles,
test_article_assets_and_images; generate -> test_generate_writes_only_out; engine -> test_engine_choice; run/CLI ->
test_cli_generate, test_cli_engine_commands, test_cli_missing_engines, test_cli_orphan_exit,
test_cli_honours_patched_site; Markdown rules -> test_fixture_markdown_rules_and_links, test_repo_markdown_rules;
builds -> test_build_fixture_strict, test_build_broken_anchor_fails, test_build_privacy_self_hosted,
test_repo_strict_build_and_ids.
"""
from __future__ import annotations

import argparse
import contextlib
import html
import importlib.util
import io
import json
import os
import posixpath
import random
import re
import shutil
import socket
import subprocess
import sys
import tempfile
import unicodedata
import unittest
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import quote, unquote
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import _00_memory_cap

_00_memory_cap.ensure()

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "98-Team Notebook" / "06-site.py"
SCRIPTS = ("98-Team Notebook/06-site.py", "98-Team Notebook/05-dependencies.py", "00-Guidelines/13-Plan/plan.py")
ARTICLES = "98-Team Notebook/Articles"
REPO = "https://github.com/u/r"
MODE, SEED, ROUNDS = "full", 42, 200
FULL_ONLY = {"test_repo_strict_build_and_ids"}
LINK = re.compile(r"\[([^\]]+)\]\((<[^>]+>|[^\s)]+)\)")
LABELED = re.compile(r"\[((?:[^\[\]`\n]|`[^`\n]*`)+)\]\((?:<[^>\n]+>|[^\s)]+)\)")  # label without brackets, one line
TIERS = ["Basic", "Advanced", "Esoteric"]


def load(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


SITE = load(TOOL, "site_under_test")
DEPS = SITE.DEPS


def write(root: Path, files: dict[str, str]) -> None:
    for rel, text in files.items():
        (root / rel).parent.mkdir(parents=True, exist_ok=True)
        (root / rel).write_bytes(text.encode("utf-8"))


def count_lines(path: Path) -> int:
    data = path.read_bytes()
    return data.count(b"\n") + (bool(data) and not data.endswith(b"\n"))


def delink(text: str) -> str:
    """Every one-line `[label](url)` replaced by its label (labels hold no brackets, code spans allowed)."""
    return LABELED.sub(r"\1", text)


def fence_run(line: str) -> tuple[str, int, str]:
    s = line.lstrip(" \t")
    c = s[:1]
    n = len(s) - len(s.lstrip(c)) if c in ("`", "~") else 0
    return c, n, s[n:]


HEADER = re.compile(r"""(?:\s*\.?[\w#.+-]+(?!\S))?(?:\s*\{.*\}|((?:\s*[A-Za-z]\w*(?:=(?:"[^"]*"|'[^']*'))?(?!\S))+))?\s*""")


def header_ok(rest: str) -> bool:
    """A superfences fence header: language, then `{...}` or only options pymdownx.highlight accepts with valid values
    (hl_lines ranges, linenums digits, title, title_mode; a bare key's value is the key itself)."""
    m = HEADER.fullmatch(rest)
    if not m:
        return False
    options = {}  # superfences keeps the last value of a repeated option
    for opt in re.finditer(r"""([A-Za-z]\w*)(=(?:"([^"]*)"|'([^']*)'))?""", m[1] or ""):
        options[opt[1]] = opt[3] if opt[3] is not None else opt[4] if opt[2] else opt[1]
    for key, value in options.items():
        if key not in ("hl_lines", "linenums", "title", "title_mode") or key == "hl_lines" and not re.fullmatch(r"[\d\- \t]+", value) \
                or key == "linenums" and not value[:1].isdigit():
            return False
    return True


def in_fence(md: str) -> list[bool]:
    """Per line, whether it belongs to a fenced block under superfences' rules: a run of 3+ backticks or tildes with a
    valid header (HEADER: an optional language, then attributes or `name[=quoted]` options) opens a block only if a
    later bare line of exactly that run closes it (an unclosed opener is text)."""
    lines, out, fence = md.split("\n"), [], None
    for i, line in enumerate(lines):
        c, n, rest = fence_run(line)
        if fence:
            out.append(True)
            if (c, n) == fence and not rest.strip():
                fence = None
        elif n >= 3 and header_ok(rest) and any(fence_run(l)[:2] == (c, n) and not fence_run(l)[2].strip() for l in lines[i + 1:]):
            out.append(True); fence = (c, n)
        else:
            out.append(False)
    return out


def outside(md: str):
    """Lines outside fenced blocks (fence lines excluded)."""
    return [line for line, f in zip(md.split("\n"), in_fence(md)) if not f]


BACKTICK = re.compile(r"(?:(?<!\\)((?:\\{2})+)(?=`+)|(?<!\\)(`+)(.+?)(?<!`)\2(?!`))")  # Python-Markdown's BACKTICK_RE
ESCAPABLE = "\\`*_{}[]()>#+-.!|"  # Python-Markdown's escaped characters with the tables extension


def code_pass(text: str) -> list[tuple[int, int, str, bool]]:
    """Python-Markdown's first inline pass, which searches BACKTICK_RE from the start again after stashing each match
    behind a placeholder (so a backtick after an escaped-backslash pair opens a span, and an opening run may backtrack
    to fewer backticks): (start, end, literal text, whether a code span) in text order; a pair run is literal halved
    backslashes."""
    work, out = text, []
    while m := BACKTICK.search(work):
        out.append((m.start(), m.end(), "\\" * (len(m[1]) // 2) if m[1] else m[3].strip(), not m[1]))
        work = work[:m.start()] + "\0" * (m.end() - m.start()) + work[m.end():]
    return sorted(out)


def heading_id(text: str) -> str:
    """Python-Markdown toc text (code spans and backslash-escaped characters literal; elsewhere `_`/`__` emphasis
    dropped, then links as labels, entities decoded; whitespace collapsed), then pymdownx slugify(case="lower")."""
    lit, plain, last = [], [], 0  # literal pieces become numbered private-use tokens while the markup is processed
    for a, b, literal, _ in code_pass(text) + [(len(text), len(text), None, False)]:
        i = last
        while i < a:
            if text[i] == "\\" and i + 1 < a and text[i + 1] in ESCAPABLE:
                lit.append(text[i + 1]); plain.append(chr(0x100000 + len(lit) - 1)); i += 2
            else:
                plain.append(text[i]); i += 1
        if literal is not None:
            lit.append(literal); plain.append(chr(0x100000 + len(lit) - 1))
        last = b
    text = re.sub(r"(?<![0-9A-Za-z_])(__?)(?=\S)(.+?)(?<=\S)\1(?![0-9A-Za-z_])", r"\2", "".join(plain))
    text = html.unescape(LINK.sub(r"\1", text))
    text = "".join(lit[ord(c) - 0x100000] if ord(c) >= 0x100000 else c for c in text)
    text, out, i = " ".join(text.split()), [], 0
    while i < len(text):
        if text[i] == "<" and ">" in text[i:]:
            i = text.index(">", i) + 1
        else:
            out.append(text[i]); i += 1
    text = unicodedata.normalize("NFC", "".join(out)).strip().lower()
    return "".join("-" if c == " " else c for c in text if c.isalnum() or c in "_- ")


def line_ids(md: str) -> dict[int, str]:
    """Line index -> id of each heading outside fences, made unique in document order (`x`, `x_1`, `x_2`)."""
    used, out = set(), {}
    for i, (line, fenced) in enumerate(zip(md.split("\n"), in_fence(md))):
        level = 0 if fenced else min(6, len(line) - len(line.lstrip("#")))
        if level:  # Python-Markdown: no space needed after the hashes; a closing run of hashes at the very end is dropped
            a = heading_id(line[level:].rstrip("#").strip())
            while not a or a in used:
                head, sep, tail = a.rpartition("_")
                a = f"{head}_{int(tail) + 1}" if sep and re.fullmatch("[0-9]+", tail) else f"{a}_1"
            used.add(a)
            out[i] = a
    return out


def heading_ids(md: str) -> set[str]:
    return set(line_ids(md).values())


def h2(md: str) -> list[str]:
    return [l[3:].strip() for l in outside(md) if l.startswith("## ")]


def own_sections(md: str) -> dict[str, tuple[int, str]]:
    """Level-2 title -> (line index of its heading, the heading line through its body without trailing empty lines);
    a repeated title keeps its last section."""
    lines, spans, cur = md.split("\n"), {}, None
    for i, (line, fenced) in enumerate(zip(lines, in_fence(md))):
        if not fenced and line.startswith("## "):
            cur = line[3:].strip(); spans[cur] = [i, i + 1]
        elif cur is not None:
            spans[cur][1] = i + 1
    out = {}
    for title, (a, b) in spans.items():
        while b > a + 1 and not lines[b - 1]:
            b -= 1
        out[title] = (a, "\n".join(lines[a:b]))
    return out


def body_range(text: str, title: bool) -> tuple[int, int]:
    """Lines a copied article or notes document keeps: from the first non-empty line, past its `# ` title line and
    the empty lines after it when `title`, to the last non-empty line."""
    lines, i = text.split("\n"), 0
    skip = lambda i: next((k for k in range(i, len(lines)) if lines[k]), len(lines))
    i = skip(0)
    if title and i < len(lines) and lines[i].startswith("# "):
        i = skip(i + 1)
    end = len(lines)
    while end > i and not lines[end - 1]:
        end -= 1
    return i, end


SECTION_TITLES = ("Contracts", "Limits and handoffs", "Sources")
TIER_HEADS = ("## Basic", "## Advanced", "## Esoteric")


def span(text: str) -> str:
    """`text` as one code span: delimiters one backtick longer than its longest run, padded when it has one."""
    n = max([len(r) for r in re.findall("`+", text)] + [0])
    return f"`{text}`" if not n else f"{'`' * (n + 1)} {text} {'`' * (n + 1)}"


def layout(root: Path, targets: list[str], folders: list[str]) -> dict[str, tuple[str, list[tuple[int, int]]]]:
    """Repository document -> (site page, line ranges copied there) for every document a page copies: each existing
    header's article body and evidence document (its non-empty Contracts, Limits and handoffs and Sources sections; no
    ranges when nothing or no file is there), and each folder's notes and folder-article body."""
    out = {}

    def document(path: str, page: str, title: bool):
        if (root / path).is_file():
            a, b = body_range((root / path).read_text(encoding="utf-8"), title)
            if a < b:
                out[path] = (page, [(a, b)])
    for t in targets:
        f, name = t.split("/")[0], pname(t)
        page, ev = f"{fslug(f)}/{name}.md", root / f / "Docs" / f"{name}.md"
        document(f"{ARTICLES}/{f}/{name}.md", page, True)
        found = own_sections(ev.read_text(encoding="utf-8")) if ev.is_file() else {}
        out[f"{f}/Docs/{name}.md"] = (page, [(a, a + s.count("\n") + 1) for title, (a, s) in found.items()
                                             if title in SECTION_TITLES and s != f"## {title}"])
    for f in folders:
        document(f"{f}/Docs/00-notes.md", f"{fslug(f)}/notes.md", False)
        document(f"{ARTICLES}/{f}/00-index.md", f"{fslug(f)}/index.md", True)
    return out


def placement(doc: str, ranges: list[tuple[int, int]], page: str, ctx: str = "") -> dict[int, int]:
    """Document line -> page line for the copied ranges, each found as the one place where its lines, every link
    replaced by its label, appear contiguously on the page."""
    d, p, out = [delink(l) for l in doc.split("\n")], [delink(l) for l in page.split("\n")], {}
    for a, b in ranges:
        at = [s for s in range(len(p) - (b - a) + 1) if p[s] == d[a] and p[s:s + b - a] == d[a:b]]
        if len(at) != 1:
            raise AssertionError(f"{ctx}copied lines {a}-{b} appear {len(at)} times on the page")
        out.update({a + k: at[0] + k for k in range(b - a)})
    return out


def id_map(doc: str, lines: dict[int, int], page: str) -> dict[str, str]:
    """A copied document's own heading id -> the id its copied heading has on the page."""
    own, ids = line_ids(doc), line_ids(page)
    return {own[k]: ids[j] for k, j in lines.items() if k in own and j in ids}


def bounds(code: str) -> list[tuple[str, str]]:
    """(declaration, comment) for each `// T:` / `// S:` complexity line: the next 11 lines that are not `//` comments,
    joined, cut at the first `{` or `;` outside parentheses, whitespace collapsed, trailing spaces, `:` and `=` dropped."""
    lines, out = code.split("\n"), []
    for i, line in enumerate(lines):
        s = line.lstrip()
        if re.fullmatch(r"// [TS]: .*", s):
            text = " ".join([l.strip() for l in lines[i + 1:] if not l.strip().startswith("//")][:11])
            depth, cut = 0, len(text)
            for j, c in enumerate(text):
                if c in "{;" and depth == 0:
                    cut = j
                    break
                depth += (c == "(") - (c == ")")
            out.append((re.sub(r"[\s:=]+$", "", " ".join(text[:cut].split())), s[3:]))
    return out


def code_spans(line: str) -> list[tuple[int, int]]:
    return [(a, b) for a, b, _, code in code_pass(line) if code]


def line_links(line: str) -> list[tuple[str, str]]:
    """(label, URL) of each link of one line outside code spans."""
    masked = line
    for a, b in code_spans(line):
        masked = masked[:a] + "\0" * (b - a) + masked[b:]
    return [(line[m.start(1):m.end(1)], line[m.start(2):m.end(2)]) for m in LINK.finditer(masked)]


def links(md: str) -> list[str]:
    """URLs of the links outside fences and code spans."""
    out = []
    for line in outside(md):
        for a, b in code_spans(line):
            line = line[:a] + " " * (b - a) + line[b:]
        out += [m[2] for m in LINK.finditer(line)]
    return out


def md_violations(text: str) -> list[str]:
    """The Markdown rules of 03-consistency.py (fences tracked per CommonMark rather than by naive toggling)."""
    out, fence, width, lines = [], None, None, text.splitlines()
    for i, line in enumerate(lines):
        c, n, rest = fence_run(line)
        if fence:
            if c == fence[0] and n >= fence[1] and not rest.strip():
                fence = None
            continue
        if n >= 3:
            fence, width = (c, n), None
            continue
        if line != line.rstrip():
            out.append(f"trailing whitespace at {i + 1}")
        if re.match(r"#+ ", line) and ((i and lines[i - 1]) or (i + 1 < len(lines) and lines[i + 1])):
            out.append(f"heading without blank lines at {i + 1}")
        if line.startswith("|"):
            w = len(re.split(r"(?<!\\)\|", line.strip())[1:-1])
            if width is not None and w != width:
                out.append(f"table width {w} != {width} at {i + 1}")
            width = w
        else:
            width = None
    return out + ["unclosed fence"] * bool(fence)


def quoted_include(line: str) -> str | None:
    rest = line.lstrip(" \t")
    if not rest.startswith("#"):
        return None
    rest = rest[1:].lstrip(" \t")
    if not rest.startswith("include"):
        return None
    rest = rest[7:].lstrip(" \t")
    return rest[1:rest.index('"', 1)] if rest.startswith('"') and '"' in rest[1:] else None


def scan(root: Path) -> dict[str, list[str]]:
    out = {}
    for top in sorted(p for p in root.iterdir() if p.is_dir() and re.fullmatch(r"0[1-7]-.*", p.name)):
        for cur, dirs, files in os.walk(top):
            dirs[:] = [d for d in dirs if d != "97-Legacy"]
            for f in files:
                if f.endswith(".hpp"):
                    rel = os.path.relpath(os.path.join(cur, f), root)
                    out[rel] = [os.path.normpath(os.path.join(os.path.dirname(rel), x)) for l in
                                Path(cur, f).read_text(encoding="utf-8").split("\n") if (x := quoted_include(l))]
    return out


def post_order(graph: dict[str, list[str]], h: str, order=None) -> list[str]:
    order = [] if order is None else order
    if h not in order:
        for d in graph.get(h, []):
            post_order(graph, d, order)
        if h not in order:
            order.append(h)
    return order


def pname(target: str) -> str:
    name = target.split("/")[1]
    return f"{name[1:3]}-{name[4:-3]}" if name.startswith("_") else name[:-4]


def fslug(folder: str) -> str:
    return folder.lower().replace(" ", "-")


def git(root: Path, *args: str, date: str | None = None) -> str:
    env = {**os.environ, "GIT_CONFIG_GLOBAL": os.devnull, "GIT_CONFIG_NOSYSTEM": "1"}
    if date:
        env.update(GIT_AUTHOR_DATE=date, GIT_COMMITTER_DATE=date)
    r = subprocess.run(["git", "-C", str(root), "-c", "user.name=t", "-c", "user.email=t@example.com",
                        "-c", "commit.gpgsign=false", *args], capture_output=True, text=True, env=env)
    if r.returncode:
        raise AssertionError(f"git {' '.join(args)} exited {r.returncode}: {r.stderr}")
    return r.stdout


ENGINES = {"zensical": ("zensical",), "mkdocs": ("material", "pymdownx")}  # modules each engine's Python imports
SEARCH = {"zensical": "search.json", "mkdocs": "search/search_index.json"}  # each engine's built search index
EXTERNAL = re.compile(r"https?://(?:unpkg\.com|fonts\.googleapis\.com|cdn\.jsdelivr\.net)(?:[/:?#]|$)")
_PY, _ENGINES, _NET, _VENVS = [], [], [], []


def shebang(exe: str) -> list[str] | None:
    """The command on a script's `#!` line when it names a Python, else None."""
    with open(exe, "rb") as f:
        head = f.readline(512).decode(errors="replace").strip()
    return head[2:].split() if head.startswith("#!") and "python" in head else None


def engines() -> list[tuple[str, str, str | None]]:
    """(engine, executable, why unusable or None) for every distinct executable (by real path) named `zensical` or
    `mkdocs` in any PATH directory, not only the first, or listed in CP_SITE_ENGINES (os.pathsep-separated paths); an
    executable whose `#!` Python cannot import the engine's modules is unusable."""
    if not _ENGINES:
        extra = [Path(p) for p in os.environ.get("CP_SITE_ENGINES", "").split(os.pathsep) if p]
        found, seen = [], set()
        for name, mods in ENGINES.items():
            for c in [Path(d) / name for d in os.environ.get("PATH", "").split(os.pathsep) if d] + [
                    p for p in extra if p.name == name]:
                if c.is_file() and os.access(c, os.X_OK) and (real := os.path.realpath(c)) not in seen:
                    seen.add(real)
                    py, why = shebang(str(c)), None
                    if py and subprocess.run([*py, "-c", f"import {', '.join(mods)}"], capture_output=True).returncode:
                        why = f"{c}: its Python ({' '.join(py)}) cannot import {', '.join(mods)}"
                    found.append((name, str(c), why))
        _ENGINES.append(found)
    return _ENGINES[0]


def venv_python() -> list[str] | None:
    """The `#!` Python of an engine on PATH, else this interpreter, whichever first imports markdown, pymdownx, yaml."""
    if not _PY:
        cands = [py for _, exe, why in engines() if not why and (py := shebang(exe))] + [[sys.executable]]
        _PY.append(next((py for py in cands if subprocess.run(
            [*py, "-I", "-c", "import markdown, pymdownx.slugs, yaml"], capture_output=True).returncode == 0), None))
    return _PY[0]


def online() -> bool:
    """Whether the hosts the privacy plugin downloads from are reachable."""
    if not _NET:
        try:
            for host in ("unpkg.com", "fonts.googleapis.com", "cdn.jsdelivr.net"):
                socket.create_connection((host, 443), timeout=5).close()
            _NET.append(True)
        except OSError:
            _NET.append(False)
    return _NET[0]


def pythons() -> dict[str, str]:
    """Bare venv Pythons, created once per run: `bare` (no packages), `good` (stub zensical, material and pymdownx
    packages) and `material` (stub material only)."""
    if not _VENVS:
        temp = tempfile.TemporaryDirectory(prefix="site-venvs-")
        out = {}
        for name, mods in (("bare", ()), ("good", ("zensical", "material", "pymdownx")), ("material", ("material",))):
            env = Path(temp.name) / name
            subprocess.run([sys.executable, "-m", "venv", "--without-pip", str(env)], check=True, capture_output=True)
            py = env / "bin" / "python"
            lib = subprocess.run([str(py), "-c", "import sysconfig; print(sysconfig.get_paths()['purelib'])"],
                                 check=True, capture_output=True, text=True).stdout.strip()
            write(Path(lib), {f"{m}/__init__.py": "" for m in mods})
            out[name] = str(py)
        _VENVS.extend([out, temp])  # the TemporaryDirectory is removed at exit
    return _VENVS[0]


PM = """import json, re, sys, markdown
from pymdownx.slugs import slugify
out = []
for d in json.load(sys.stdin):
    md = markdown.Markdown(extensions=["admonition", "tables", "pymdownx.details", "pymdownx.superfences",
                                       "pymdownx.arithmatex", "toc"],
                           extension_configs={"toc": {"slugify": slugify(case="lower")}, "pymdownx.arithmatex": {"generic": True}})
    h = md.convert(d)
    out.append(h if sys.argv[1:] == ["html"] else re.findall(r'<h[1-6][^>]*?\\sid="([^"]*)"', h))
print(json.dumps(out))
"""
YAML = """import json, sys, yaml
class L(yaml.SafeLoader): pass
def tagged(loader, suffix, node):
    value = loader.construct_mapping(node, deep=True) if isinstance(node, yaml.MappingNode) else loader.construct_scalar(node)
    return {"tag": suffix, "value": value}
L.add_multi_constructor("tag:yaml.org,2002:python/", tagged)
print(json.dumps(yaml.load(sys.stdin.read(), Loader=L)))
"""


def pm_ids(docs: list[str], kind: str = "ids") -> list:
    """Per document, the heading ids Python-Markdown renders with the site's extensions (or its HTML: kind "html")."""
    r = subprocess.run([*venv_python(), "-I", "-c", PM, kind], input=json.dumps(docs), capture_output=True, text=True, timeout=600)
    if r.returncode:
        raise AssertionError(f"Python-Markdown oracle exited {r.returncode}: {r.stderr[-1500:]}")
    return json.loads(r.stdout)


def parse_yaml(text: str):
    """mkdocs.yml as JSON data (python tags as {"tag", "value"}), or None when no yaml module is available."""
    try:
        import yaml  # noqa: F401
        py = [sys.executable]
    except ImportError:
        py = venv_python()
    if not py:
        return None
    r = subprocess.run([*py, "-c", YAML], input=text, capture_output=True, text=True)
    if r.returncode:
        raise AssertionError(f"mkdocs.yml is not valid YAML: {r.stderr[-1500:]}")
    return json.loads(r.stdout)


def build(exe: str, out: Path) -> subprocess.CompletedProcess:
    """The strict build 06-site.py runs, in `out` (the privacy plugin caches under the working directory)."""
    return subprocess.run([exe, "build", "-f", str(out / "mkdocs.yml"), "--strict"], cwd=out, capture_output=True,
                          text=True, timeout=900, env={**os.environ, "NO_MKDOCS_2_WARNING": "1"})


def plain(r: subprocess.CompletedProcess) -> str:
    """A build's output without ANSI colour codes (Zensical colours every character even when piped)."""
    return re.sub(r"\x1b\[[0-9;]*m", "", r.stdout + r.stderr)


class Attrs(HTMLParser):
    def __init__(self):
        super().__init__()
        self.urls = []

    def handle_starttag(self, tag, attrs):
        self.urls += [v for k, v in attrs if k in ("src", "href") and v]


def fake_engines(bin: Path, **kinds: str | None) -> Path:
    """bin/<engine> fake scripts by kind: `bare`, `good` or `material` (`#!` names that venv's Python), `sh` (a
    non-Python `#!`), `env` (`#!/usr/bin/env python`, bin/python running the good venv), `noexec` (good but not
    executable) or `gone` (its `#!` Python does not exist); None or absent: no such engine. Running one only creates bin/ran."""
    bin.mkdir(parents=True, exist_ok=True)
    py, ran = pythons(), str(bin / "ran")
    for name, kind in kinds.items():
        if kind is None:
            continue
        if kind == "env":
            (bin / "python").write_text(f"#!/bin/sh\nexec {py['good']} \"$@\"\n")
            (bin / "python").chmod(0o755)
        head = {"sh": "/bin/sh", "env": "/usr/bin/env python", "noexec": py["good"], "gone": str(bin / "gone/python")}.get(kind) or py[kind]
        (bin / name).write_text(f"#!{head}\n" + (f"touch '{ran}'\n" if kind == "sh" else f"open({ran!r}, 'w').close()\n"))
        (bin / name).chmod(0o644 if kind == "noexec" else 0o755)
    return bin


class Ids(HTMLParser):
    def __init__(self):
        super().__init__()
        self.depth, self.ids = 0, []

    def handle_starttag(self, tag, attrs):
        if tag == "article":
            self.depth += 1
        elif self.depth and re.fullmatch(r"h[1-6]", tag) and (a := dict(attrs).get("id")) is not None:
            self.ids.append(a)

    def handle_endtag(self, tag):
        self.depth -= tag == "article" and self.depth > 0


def built_ids(html: Path) -> list[str]:
    p = Ids()
    p.feed(html.read_text(encoding="utf-8"))
    return p.ids


def read_docs(out: Path) -> dict[str, str]:
    return {p.relative_to(out / "docs").as_posix(): p.read_text(encoding="utf-8") for p in sorted((out / "docs").rglob("*.md"))}


T = "#pragma once\n"


def inc(*names: str) -> str:
    return "".join(f'#include "{n}"\n' for n in names)


DSU_EVIDENCE = """# 01-dsu.hpp — evidence

Intro paragraph [ignored](../01-dsu.hpp).

## Contracts

### DSU

- `findSet(x)` returns the root; see [adv](../02-adv.hpp) and [its contracts](02-adv.md#contracts).
- Same document: [limits](#limits-and-handoffs) and [second](#dsu_1).
- [notes](<00-notes.md>), [core](../../01-Core/05-thing.hpp), [pct](../../02-Data%20Structures/01-dsu.hpp).
- [line](<../../02-Data Structures/02-adv.hpp#L1>), [web](https://cp-algorithms.com/x.html), [mail](mailto:a@b.c).
- Literal `[code](../01-dsu.hpp)` and ``[two](../01-dsu.hpp) ` tick``; label [`DSU::findSet`](../01-dsu.hpp).
- [gone](../nothere.hpp), [folder](..), [index](../00-index.md#advanced), [bad anchor](../00-index.md#nope).
- [tester](../../96-Local%20Testing/02-Data%20Structures/01-dsu_tester.py), [tests](<../../96-Local Testing/>).
- [escape](../../../outside.md), [absolute](ABS).

```text
[fenced](../01-dsu.hpp)
## Not a heading
```

~~~~
```
[tilde](../01-dsu.hpp)
~~~
## Still fenced
~~~~

### DSU

Second DSU heading; after the fences [again](../01-dsu.hpp).

## Limits and handoffs

- Recursion-free; [geometry](../../03-Geometry/01-point.hpp#point).

## Tests

Not copied [hidden](../01-dsu.hpp).

## Sources

- [KACTL](https://github.com/kactl/kactl/blob/main/content/data-structures/UnionFind.h)
"""
ARTICLE = """# DSU article title

## Overview

Union by size keeps the depth below $\\log_2 n$; the cost is $x^2$ at worst.

$$
\\sum_{i} a_i
$$

See [the contracts](<../../../02-Data Structures/Docs/01-dsu.md#contracts>), [this overview](#overview) and
[the advanced header](../../../02-Data%20Structures/02-adv.hpp).
"""


def fixture(root: Path) -> None:
    thing = T + inc("01-template.hpp", "03-barrett.hpp") + "struct Thing {\n" + "  int f;\n" * 20 + "};\n"
    write(root, {
        "01-Core/00-index.md": "# 01 Core — inventory\n\nScope: fixture Core; contracts in [00-notes.md](Docs/00-notes.md), "
        "sources in [00-sources.md](Docs/00-sources.md).\n\n| Header | Basic | Operations | Status |\n|---|---|---|---|\n"
        "| `01-template.hpp` | yes | macros: fi, se; chmin (a; b) | verified; [01-template.md](Docs/01-template.md) |\n"
        "| `03-barrett.hpp` | no | Barrett: mul; reduce | verified |\n"
        "| `05-thing.hpp` | no | Thing: f (x; y); g;; h | partial; missing h |\n"
        "| `06-thingmini.hpp` | yes | ThingMini: f | existing-unverified |\n"
        "| `07-planned.hpp` | no | Planned: q | planned |\n",
        "01-Core/01-template.hpp": "#pragma once\n#include <bits/stdc++.h>\nusing namespace std;\n",
        "01-Core/03-barrett.hpp": T + inc("01-template.hpp") + "struct Barrett {};\n",
        "01-Core/05-thing.hpp": thing,
        "01-Core/06-thingmini.hpp": T + inc("01-template.hpp") + "struct ThingMini {};",
        "01-Core/99-all.hpp": inc("01-template.hpp", "03-barrett.hpp", "05-thing.hpp", "06-thingmini.hpp"),
        "01-Core/Docs/00-notes.md": "# 01 Core — notes\n\n## Shared\n\nSee [the thing](../05-thing.hpp) and "
        "[DS](<../../02-Data Structures/00-index.md#advanced>).\n",
        "01-Core/Docs/00-sources.md": "# Sources\n",
        "01-Core/Docs/05-thing.md": "# 05-thing.hpp — evidence\n\n## Contracts\n\n- Thing pairs with "
        "[DSU contracts](../../02-Data%20Structures/Docs/01-dsu.md#dsu).\n\n## Sources\n\n- Original.\n",
        "02-Data Structures/00-index.md": "# 02 Data Structures — inventory\n\nScope: DS fixture; notes in "
        "[00-notes.md](Docs/00-notes.md), [sources](Docs/00-sources.md) and [Core](../01-Core/00-index.md).\n\n## Basic\n\n"
        "| Header | Operations | Status |\n|---|---|---|\n"
        "| `01-dsu.hpp` | DSU: findSet, uniteSets; groups (sorted; stable); makeSet | verified; [evidence](Docs/01-dsu.md) |\n"
        "| `03-red.hpp` | Red: mul | verified |\n\n## Advanced\n\n| Header | Operations | Status |\n|---|---|---|\n"
        "| `02-adv.hpp` | Adv: q | in-progress |\n| `04-missing.hpp` | M: r | planned |\n",
        "02-Data Structures/01-dsu.hpp": T + inc("../01-Core/01-template.hpp") + "struct DSU {};\n",
        "02-Data Structures/02-adv.hpp": T + inc("01-dsu.hpp", "../01-Core/05-thing.hpp") + "struct Adv {};\n",
        "02-Data Structures/03-red.hpp": T + inc("../01-Core/01-template.hpp", "../01-Core/03-barrett.hpp")
        + "struct RedCompact {\n};\n",
        "02-Data Structures/99-all.hpp": inc("01-dsu.hpp", "02-adv.hpp", "03-red.hpp"),
        "02-Data Structures/Docs/00-notes.md": "# 02 Data Structures — notes\n\n## Rules\n\n- See [dsu](../01-dsu.hpp) and "
        "[core notes](../../01-Core/Docs/00-notes.md#shared).\n",
        "02-Data Structures/Docs/01-dsu.md": DSU_EVIDENCE.replace("ABS", str(root / "01-Core/01-template.hpp")),
        "02-Data Structures/Docs/02-adv.md": "# 02-adv.hpp — evidence\n\n## Contracts\n\n- Adv uses [DSU](01-dsu.md#dsu).\n\n"
        "## Limits and handoffs\n\n## Sources\n",
        "03-Geometry/00-index.md": "# 03 Geometry — inventory\n\nScope: geometry fixture.\n\n## Basic\n\n"
        "| Header | Operations | Status |\n|---|---|---|\n| `01-point.hpp` | Point: dot; cross | verified |\n",
        "03-Geometry/01-point.hpp": T + inc("../01-Core/01-template.hpp") + "struct Point {};\n",
        "08-Python/00-index.md": "# 08 Python — inventory\n\nScope: Python fixture.\n\n## Basic\n\n"
        "| Module | Operations | Status |\n|---|---|---|\n| `_01_x.py` | readInts; Fast: read, write | planned; pyrival |\n"
        "| `_02_y.py` | y | planned |\n",
        "08-Python/_01_x.py": 'DOC = """\n````\n"""\n\n   \ndef readInts():\n    return 1\n',
        "08-Python/Docs/01-x.md": "# _01_x.py — evidence\n\n## Contracts\n\n- readInts returns [ints](../_01_x.py).\n",
        "96-Local Testing/02-Data Structures/01-dsu_tester.py": "",
        "96-Local Testing/02-Data Structures/01-dsu_tester.cpp": "",
        "96-Local Testing/02-Data Structures/02-adv_tester.py": "",
        f"{ARTICLES}/02-Data Structures/01-dsu.md": ARTICLE,
    })
    (root.parent / "outside.md").write_text("# outside\n")


PAGES = {"01-Core/01-template.hpp": "01-core/01-template.md", "01-Core/03-barrett.hpp": "01-core/03-barrett.md",
         "01-Core/05-thing.hpp": "01-core/05-thing.md", "01-Core/06-thingmini.hpp": "01-core/06-thingmini.md",
         "02-Data Structures/01-dsu.hpp": "02-data-structures/01-dsu.md",
         "02-Data Structures/03-red.hpp": "02-data-structures/03-red.md",
         "02-Data Structures/02-adv.hpp": "02-data-structures/02-adv.md", "03-Geometry/01-point.hpp": "03-geometry/01-point.md",
         "08-Python/_01_x.py": "08-python/01-x.md"}
NOTES = {"01-Core": "01-core/notes.md", "02-Data Structures": "02-data-structures/notes.md"}
FOLDERS = ["01-Core", "02-Data Structures", "03-Geometry", "08-Python"]


class Base(unittest.TestCase):
    def setUp(self):
        self.base = Path(tempfile.mkdtemp(prefix="site-test-"))
        self.root = self.base / "library"
        self.root.mkdir()
        self.addCleanup(self.clean)

    def clean(self):
        result = self._outcome.result
        if any(t is self or getattr(t, "test_case", None) is self for t, _ in result.failures + result.errors):
            print(f"\nREPRO seed={SEED} mode={MODE} rounds={ROUNDS} test={self.id()} fixture={self.base}", file=sys.stderr)
        else:
            shutil.rmtree(self.base)

    def per_engine(self, check):
        """check(engine, executable) once per engine executable from engines(), each in its own subtest; an engine
        with no executable and an unusable executable are SKIPs."""
        found = engines()
        for name in ENGINES:
            if not any(e == name for e, _, _ in found):
                with self.subTest(engine=name):
                    self.skipTest(f"SKIP: {name} not on PATH or in CP_SITE_ENGINES")
            for _, exe, why in (e for e in found if e[0] == name):
                with self.subTest(engine=name, exe=exe):
                    if why:
                        self.skipTest(f"SKIP: {why}")
                    check(name, exe)

    def need_venv(self):
        if not venv_python():
            self.skipTest("SKIP: no python with markdown, pymdownx and yaml (an engine's or this one)")

    def check_copied_links(self, pages: dict[str, str], repo: str | None, root: Path, targets: list[str],
                           folders: list[str], ctx: str = "") -> int:
        """Every anchored link of a copied document into a copied document, found on the page through the copied
        line and its (line-unique) label: a site link to the id of the page heading copied from the target's heading
        of that id, with the same text, or else (that heading not copied) a repository link or only its label.
        Returns the number of links checked."""
        lay = layout(root, targets, folders)
        texts = {d: (root / d).read_text(encoding="utf-8") for d in lay if (root / d).is_file()}
        where = {d: placement(texts[d], lay[d][1], pages[lay[d][0]], f"{ctx}{d}: ") for d in texts}
        run = subprocess.run(["git", "-C", str(root), "ls-files", "-z"], capture_output=True, text=True)
        tracked = set(run.stdout.split("\0")) if run.returncode == 0 else None
        known = lambda p: p in tracked if tracked is not None else (root / p).is_file()
        n = 0
        for d, text in texts.items():
            page, fence, lines = lay[d][0], in_fence(text), text.split("\n")
            for k, j in where[d].items():
                if fence[k]:
                    continue
                src, dst, paired, it = line_links(lines[k]), line_links(pages[page].split("\n")[j]), {}, 0
                for i, (label, _) in enumerate(src):  # relink keeps or drops each link in order
                    if it < len(dst) and dst[it][0] == label:
                        paired[i], it = dst[it][1], it + 1
                for i, (label, url) in enumerate(src):
                    raw = unquote(url[1:-1] if url.startswith("<") else url)
                    path, _, anchor = raw.partition("#")
                    target = posixpath.normpath(posixpath.join(posixpath.dirname(d), path)) if path else d
                    if [l for l, _ in src].count(label) > 1 or "://" in raw or not anchor or target not in texts:
                        continue
                    tp, own = lay[target][0], {a: x for x, a in line_ids(texts[target]).items()}
                    got, at = paired.get(i), where[target].get(own.get(anchor, -1))
                    msg = f"{ctx}{d} line {k + 1}: [{label}]({url}) became {got!r} on {page}"
                    if at is not None and at in line_ids(pages[tp]):
                        self.assertEqual(got, posixpath.relpath(tp, posixpath.dirname(page)) + "#" + line_ids(pages[tp])[at], msg)
                        self.assertEqual(delink(pages[tp].split("\n")[at]).strip(), delink(texts[target].split("\n")[own[anchor]]).strip(), msg)
                    else:
                        self.assertEqual(got, f"{repo}/blob/master/{quote(target)}#{anchor}" if repo and known(target) else None, msg)
                    n += 1
        return n

    def check_links(self, pages: dict[str, str], repo: str | None, root: Path, ctx: str = "", assets=frozenset()):
        """Every link of every generated page: a generated page and one of its heading ids, a copied asset, or an
        existing repo path."""
        ids = {p: heading_ids(t) for p, t in pages.items()}
        run = subprocess.run(["git", "-C", str(root), "ls-files", "-z"], capture_output=True, text=True)
        files = set(run.stdout.split("\0")) - {""} if run.returncode == 0 else None  # None: not a checkout
        for p, text in pages.items():
            for url in links(text):
                u = url[1:-1] if url.startswith("<") else url
                if u.startswith("mailto:"):
                    continue
                if "://" in u:
                    for kind in ("blob", "tree"):
                        if repo and u.startswith(f"{repo}/{kind}/master/"):
                            path = unquote(u[len(f"{repo}/{kind}/master/"):].partition("#")[0].removesuffix("?raw=true"))
                            self.assertTrue((root / path).is_file() if kind == "blob" else (root / path).is_dir(),
                                            f"{ctx}{p}: {u} names no repository {kind}")
                            if files is not None:
                                self.assertTrue(path in files if kind == "blob" else any(f.startswith(path + "/") for f in files),
                                                f"{ctx}{p}: {u} names an untracked {kind}")
                            self.assertFalse(path.startswith(("..", "/")), f"{ctx}{p}: {u}")
                    continue
                path, _, anchor = u.partition("#")
                target = os.path.normpath(os.path.join(os.path.dirname(p), path)) if path else p
                if target in assets and not anchor:
                    continue
                self.assertIn(target, set(pages), f"{ctx}{p}: link {u} resolves to no generated page")
                if anchor:
                    self.assertIn(anchor, ids[target], f"{ctx}{p}: link {u}: no heading id {anchor!r} in {target}")


class Units(Base):
    def test_slug_cases(self):
        for text, want in [("Hello World", "hello-world"), ("A  B", "a-b"), ("A &amp; B", "a--b"), ("_a_ b __c__ d_e", "a-b-c-d_e"), ("`a  b` c", "a-b-c"), ("`&amp;` x", "amp-x"), ("  padded  ", "padded"),
                           ("[`DSU::find`](x.md) and <em>it</em>", "dsufind-and-it"), ("Café", "café"),
                           ("Café", "café"), ("Ünïcödé Σ", "ünïcödé-σ"), ("x_y-z.w", "x_y-zw"),
                           ("C++ (2020)", "c-2020"), ("PolyMini<T>", "polymini"), ("[a](<b c.md>) d", "a-d"),
                           ("", ""), ("...", ""), ("中文 x2", "中文-x2"), ("\\_\\_init\\_\\_", "__init__"),
                           ("_x_[link](y.md)", "xlink"), ("\\*a\\* `\\_b\\_`", "a-_b_"), ("``a`b`` c", "ab-c"),
                           ("\\`_x_\\`", "x"), ("\\`[a](b.md)\\` c", "a-c"), ("`a\\`_b_`", "ab")]:
            with self.subTest(text=text):
                self.assertEqual(SITE.slug(text), want)
                self.assertEqual(heading_id(text), want)

    def test_anchors_cases(self):
        md = ("# Title\n\n## Title\n\n## Title\n\n## Title_1\n\n```\n## In fence\n```\n\n~~~\n## in tilde\n```\n"
              "## still tilde\n~~~\n\n````\n```\n## inner short\n````\n\n## `code` [link](x.md) <b>bold</b>\n\n## ...\n\n"
              "## !!!\n\n#### Deep\n\n####### seven\n\n#no space\n\n  ## indented two\n\n## Trailing   \n")
        want = {"title", "title_1", "title_2", "title_3", "code-link-bold", "_1", "_2", "deep", "-seven", "no-space", "trailing"}
        self.assertEqual(SITE.anchors(md), want)
        self.assertEqual(heading_ids(md), want)
        self.assertEqual(SITE.heading_ids(md), line_ids(md))
        self.assertEqual(SITE.heading_ids("x\n## A\n\n```\n## B\n```\n## A\n"), {1: "a", 6: "a_1"})
        self.assertEqual(SITE.anchors("```\n## a\n````\n## b\n"), {"a", "b"}, "an unclosed fence is text")
        self.assertEqual(SITE.anchors("```\n## a\n````\n## b\n```\n"), set(), "a longer fence does not close")
        self.assertEqual(SITE.anchors("## T ##\n\n## x ##  \n\n#\n"), {"t", "x-", "_1"}, "closing hashes, empty heading")
        self.assertEqual(SITE.anchors("````\n## a\n```\n## b\n````\n## c\n"), {"c"}, "a shorter fence does not close")
        self.assertEqual(SITE.anchors("~~~\n## a\n```\n## b\n~~~\n## c\n"), {"c"}, "the other fence char does not close")
        self.assertEqual(SITE.anchors(""), set())

    def test_sections_cases(self):
        md = ("# T\n\nintro\n\n## A\n\n\nbody a\n```\n## not\n```\n\n\n## B\n~~~~\n## x\n~~~\n## still\n~~~~\nb\n"
              "  ## indented\n### sub\n\n## Empty\n\n\n## Last\nz")
        want = {"A": (4, "## A\n\n\nbody a\n```\n## not\n```"), "B": (13, "## B\n~~~~\n## x\n~~~\n## still\n~~~~\nb\n"
                                                             "  ## indented\n### sub"), "Empty": (23, "## Empty"), "Last": (26, "## Last\nz")}
        self.assertEqual(SITE.sections(md), want)
        self.assertEqual(own_sections(md), want)
        self.assertEqual(SITE.sections("no sections\n"), {})
        self.assertEqual(SITE.sections("## A\nx\n## B\n\n## A\ny\n\n"), {"A": (4, "## A\ny"), "B": (2, "## B")}, "the last A wins")
        self.assertEqual(SITE.sections("##  Spaced  title\nx"), {"Spaced  title": (0, "##  Spaced  title\nx")})

    def test_trimmed_cases(self):
        for text, title, want in [("# T\n\nbody\n\n", True, ("body", 2)), ("# T\n\nbody\n", False, ("# T\n\nbody", 0)),
                                  ("\n\nbody\n", False, ("body", 2)), ("\n\nbody\n", True, ("body", 2)), ("# T\n", True, ("", 1)),
                                  ("body", True, ("body", 0)), ("## H\nx\n", True, ("## H\nx", 0)), ("", True, ("", 0)),
                                  ("# T\n\n\n## H\n\nx\n\n\n", True, ("## H\n\nx", 3)), ("#T\nx", True, ("#T\nx", 0)),
                                  ("# Only a title", True, ("", 0))]:
            with self.subTest(text=text, title=title):
                self.assertEqual(SITE.trimmed(text, title), want)
                a, b = body_range(text, title)
                self.assertEqual(("\n".join(text.split("\n")[a:b]), a if a < b else want[1]), want, "body_range oracle")

    def test_trimmed_title_after_blank_lines(self):
        """Regression (P239): trimmed() drops a `# ` title preceded by blank lines, so a page gets no second h1."""
        text = "\n\n# T\n\nbody\n"
        a, b = body_range(text, True)
        self.assertEqual(("\n".join(text.split("\n")[a:b]), a), ("body", 4), "body_range oracle")
        self.assertEqual(SITE.trimmed(text, True), ("body", 4))

    def test_complexity_cases(self):
        code = "\n".join(["#pragma once", "// T: O(1)", "template <class T>", "T id(T x) { return x; }",
                         "// S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)", "// n in [0, 2^31); find returns -1 when absent.",
                         "struct Fenwick : Base<int> {", "    // T: O(n)", "    Fenwick(int n,",
                         "            int init = {}) : a(n, init) {}", "    // T: O(log(n))", "    template <class U>",
                         "        requires (std::is_integral_v<U> && requires(U x) { x + x; })", "    U get(U x) const;", "};",
                         "// T: O(1)", "inline std::array<int, 3> table = {1, 2, 3};", "// T: O(1)",
                         "constexpr auto sq = [](int x) { return x * x; };", "  //   T: O(1) two spaces", "// M: O(1) not T or S",
                         "x; // T: O(1) not alone", "//T: O(1) no space", "// T: O(k)", "int f(",
                         *[f"    int a{k}," for k in range(9)], "    int z)", "    const {", "}"])
        want = [("template <class T> T id(T x)", "T: O(1)"), ("struct Fenwick : Base<int>", "S: O(n), U: O(log(n)), Q: O(log(n)), M: O(n)"),
                ("Fenwick(int n, int init = {}) : a(n, init)", "T: O(n)"),
                ("template <class U> requires (std::is_integral_v<U> && requires(U x) { x + x; }) U get(U x) const", "T: O(log(n))"),
                ("inline std::array<int, 3> table", "T: O(1)"), ("constexpr auto sq = [](int x)", "T: O(1)"),
                ("int f( " + " ".join(f"int a{k}," for k in range(9)) + " int z)", "T: O(k)")]
        self.assertEqual(bounds(code), want, "bounds oracle")
        self.assertEqual(SITE.complexity(code), want)
        self.assertEqual(SITE.complexity("#pragma once\nint x;\n"), [])
        self.assertEqual(SITE.complexity("// T: O(1)\n// domain\n\nstruct A\n{\n};"), [("struct A", "T: O(1)")])

    def test_items_cases(self):
        for cell, want in [("a; b", ["a", "b"]), ("f (x; y); g;; h", ["f (x; y)", "g", "h"]),
                           ("A: x, y (p; (q; r)); B", ["A: x, y (p; (q; r))", "B"]), (" ; ;", []), ("one", ["one"]),
                           ("", []), ("a;", ["a"]),
                           ("randDouble ([0, 1) and [l, r)), shuffle; rng (global instance)",
                            ["randDouble ([0, 1) and [l, r)), shuffle", "rng (global instance)"]),
                           ("f [a; b]; g (x; [l, r)); h", ["f [a; b]", "g (x; [l, r))", "h"])]:
            with self.subTest(cell=cell):
                self.assertEqual(SITE.items(cell), want)

    def test_names(self):
        for target, want in [("02-Data Structures/01-dsu.hpp", "01-dsu"), ("08-Python/_01_io.py", "01-io"),
                             ("08-Python/_03_number_theory.py", "03-number_theory"), ("01-Core/16-poly.hpp", "16-poly"),
                             ("05-Mathematics/12-combinatorics_advanced.hpp", "12-combinatorics_advanced")]:
            self.assertEqual(SITE.page_name(target), want)
            self.assertEqual(pname(target), want)
        self.assertEqual(SITE.folder_slug("02-Data Structures"), "02-data-structures")
        self.assertEqual(SITE.folder_slug("01-Core"), "01-core")

    def test_repository_urls(self):
        self.assertIsNone(SITE.repository(self.root), "outside git")
        git(self.root, "init", "-q")
        self.assertIsNone(SITE.repository(self.root), "no origin")
        git(self.root, "remote", "add", "origin", "x")
        for url, want in [("git@github.com:u/r.git", "https://github.com/u/r"), ("https://github.com/u/r.git", REPO),
                          ("https://github.com/u/r", REPO), ("git@gitlab.example.org:g/s/p.git", "https://gitlab.example.org/g/s/p"),
                          ("https://user:token@github.com/u/r.git", REPO), ("ssh://git@github.com/u/r.git", REPO),
                          ("ssh://github.com/u/r", REPO), ("ssh://git@github.com:2222/u/r.git", REPO),
                          ("ssh://user@git.example.org:22/g/s/p.git", "https://git.example.org/g/s/p"), ("http://github.com/u/r.git", None), ("/srv/r.git", None),
                          ("alice@example.org:g/p.git", "https://example.org/g/p"), ("github.com:u/r.git", REPO), ("https://github.com/u/r/", REPO),
                          ("https://github.com/u/r.git/", REPO), ("file:///srv/r.git", None)]:
            with self.subTest(url=url):
                git(self.root, "remote", "set-url", "origin", url)
                self.assertEqual(SITE.repository(self.root), want)

    def test_changed_dates(self):
        self.assertEqual(SITE.changed(self.root), {})
        write(self.root, {"01-Core/a.hpp": "a\n", "02-Data Structures/b c.hpp": "b\n", "README.md": "r\n",
                          "09-Contest Testing/x.py": "x\n", "08-Python/_01_x.py": "p\n"})
        git(self.root, "init", "-q")
        git(self.root, "add", "-A")
        git(self.root, "commit", "-q", "-m", "one", date="2020-01-02T03:04:05+00:00")
        write(self.root, {"01-Core/a.hpp": "a2\n", "README.md": "r2\n"})
        git(self.root, "commit", "-q", "-am", "two", date="2021-03-04T05:06:07+00:00")
        want = {"01-Core/a.hpp": "2021-03-04", "02-Data Structures/b c.hpp": "2020-01-02", "08-Python/_01_x.py": "2020-01-02"}
        self.assertEqual(SITE.changed(self.root), want)
        for p, d in want.items():
            self.assertEqual(git(self.root, "log", "-1", "--format=%cs", "--", p).strip(), d)

    def test_anchors_python_markdown_domain(self):
        self.need_venv()
        fixture(self.root)
        docs = list(SITE.Site(self.root, REPO).files().values())
        docs = [d for d in docs if not d.startswith(("site_name", "window."))] + [
            "## x\n\n## x\n\n## x_1\n", "## ...\n\n## !!!\n", "## Café é\n", "## PolyMini<T>\n", "## a<b\n",
            "## [`x`](y.md) <b>z</b>\n", "## $x^2$ y\n", "## C++ (2020)\n", "## 中文 Σ\n", "## rational_detail kernels\n",
            "~~~~\n```\n## no\n~~~\n## no\n~~~~\n\n## yes\n"]
        for d, ids in zip(docs, pm_ids(docs)):
            with self.subTest(doc=d[:80]):
                self.assertEqual(len(ids), len(set(ids)))
                self.assertEqual(SITE.anchors(d), set(ids))
                self.assertEqual(heading_ids(d), set(ids))

    def test_anchors_python_markdown_edge_cases(self):
        """Headings whose rendered ids (Python-Markdown toc + pymdownx slugify, as mkdocs builds) differ from anchors()."""
        self.need_venv()
        docs = ["## \\_\\_init\\_\\_\n", "## _x_[link](y.md)\n", "## \\*a\\* `\\_b\\_`\n", "## ``a`b`` c\n", "## a  b\n", "## T ##\n", "```\nx\n````\n## h\n```\n", "```\n```python\n## inside\n```\n", "## A &amp; B\n",
                "## _a_ b\n", "#nospace\n", "## \\`_x_\\`\n", "## \\`[a](y.md)\\` c\n", "## `a\\`_b_`\n",
                "````| x |\n## h\n````\n", "```cpp title=\"a b\"\n## in\n```\n", "~~~ {.cpp}\n## in2\n~~~\n", "``` a b\n## out\n```\n", "```cpp linenums=\"1\"\n## in3\n```\n", "```cpp hl_lines=\"x\"\n## out2\n```\n",
                "```mermaid title=\"t\"\n## in4\n```\n", "``` title\n## in5\n```\n",
                "## [A &amp; _B_](y.md) c\n", "## &a*c_;\n", "## a <b>_c_</b>\n"]
        for d, ids in zip(docs, pm_ids(docs)):
            with self.subTest(doc=d):
                self.assertEqual(SITE.anchors(d), set(ids), "anchors() differs from the rendered heading ids")
                self.assertEqual(heading_ids(d), set(ids), "the heading_id oracle differs from Python-Markdown")

    def test_code_span_rule(self):
        """Regression (P239): slug() follows Python-Markdown's code-span rule, which opens a span after an escaped-backslash
        pair and lets an unclosed longer run open a shorter span; ids below were rendered by Python-Markdown 3.11."""
        cases = [("a \\\\`_b_`", "a-_b_"), ("\\``_x_`", "_x_"), ("``_a_`", "_a_"), ("x ``` y `_a_` z", "x--ya-z"),
                 ("a\\\\\\\\`_c_`", "a_c_")]
        for text, want in cases:
            self.assertEqual(heading_id(text), want, f"heading_id oracle on {text!r}")
        if venv_python():
            self.assertEqual([i for d in pm_ids([f"## {t}\n" for t, _ in cases]) for i in d], [w for _, w in cases])
        for text, want in cases:
            with self.subTest(text=text):
                self.assertEqual(SITE.slug(text), want)


class Renderer(Base):
    """Heading ids and table links against Python-Markdown itself (SKIP without it): fixed cases and seeded fuzz."""

    def setUp(self):
        super().setUp()
        self.need_venv()

    def fuzz(self, alphabet, per: int) -> list[str]:
        """`per` seeded strings of 1 to 24 picks from `alphabet` (characters or tokens)."""
        rng = random.Random(SEED)
        return ["".join(rng.choice(alphabet) for _ in range(rng.randint(1, 24))) for _ in range(per)]

    def test_heading_pipeline_python_markdown(self):
        """slug()'s inline pipeline as rendered: code spans, escapes, math, entities and inline HTML are placeholders
        and elements that emphasis and the toc see as the renderer does (strip_tags cuts at the first `>`, entities
        decode after whitespace collapses); a hash line inside a table is a row, one after a table-less line splits the
        block; ids count with ASCII digits; superfences keeps a repeated option's last value."""
        docs = ["## ***`y`_\\*__*\n", "## *x _a_ b_ `y` _c_\n", "## O(n&nbsp;log&nbsp;n)\n", "## a &#32; b\n",
                "## $x _a$ y $b_ z$\n", '## <span title="x>y">z</span>\n', "## <![CDATA[a>b]]> c\n", "## \\<b\\>x\n",
                "## x ![a](b) y\n", "## <http://a_b_c> d\n", "| a | b |\n|---|---|\n## x\n", "## T\n| h | h |\n|---|---|\n| c | d |\n## U\n",
                "p\n## V\n| h | h |\n|---|---|\n", '```cpp hl_lines="x" hl_lines="1"\n## in\n```\n', "## x_\u0663\n## x_\u0663\n"]
        self.assertEqual([sorted(SITE.anchors(d)) for d in docs], [sorted(i) for i in pm_ids(docs)])
        per = {"quick": 300, "full": 2000}.get(MODE, 30 * ROUNDS)
        tokens = ["_", "*", "a", " ", "`y`", "\\*", "&amp;", "<i>", "[l](u)", "$", "\\", "&nbsp;", "`", '<span title="x>y">', "\\("]
        docs = [f"## {t}\n" for t in self.fuzz(tokens, per)]
        bad = [(d, w, sorted(SITE.anchors(d))) for d, w in zip(docs, pm_ids(docs)) if sorted(SITE.anchors(d)) != sorted(w)]
        self.assertEqual(bad[:5], [], f"{len(bad)} of {len(docs)} headings differ (seed {SEED})")

    def test_emphasis_python_markdown(self):
        """slug() follows Python-Markdown 3.11's delimiter emphasis (runs, intraword `_`, nesting, `*` and `_` mixed)
        and the hash-header rule (an unpaired trailing backslash makes no heading)."""
        fixed = [("___", ["___"]), ("_init _ab_", ["_init-ab"]), ("Limits \\", []), ("__init__", ["init"]), ("a_b_c", ["a_b_c"]),
                 ("*a _b* c_", ["a-_b-c_"]), ("***a***", ["a"]), ("_**a  _b_* *____ba__b_*", ["_a-b-__ba_"])]
        self.assertEqual(pm_ids([f"## {t}\n" for t, _ in fixed]), [w for _, w in fixed], "the fixed cases themselves")
        for t, want in fixed:
            self.assertEqual(sorted(SITE.anchors(f"## {t}\n")), want, t)
        per = {"quick": 400, "full": 3000}.get(MODE, 40 * ROUNDS)
        texts = self.fuzz("__**ab  _", per) + self.fuzz("_*a b`\\_*", per) + self.fuzz("___***a b c&;", per)
        docs = [f"## {t}\n" for t in texts]
        bad = [(t, w, sorted(SITE.anchors(d))) for t, d, w in zip(texts, docs, pm_ids(docs)) if sorted(SITE.anchors(d)) != w]
        self.assertEqual(bad[:5], [], f"{len(bad)} of {len(docs)} headings differ (seed {SEED})")

    def test_math_and_escaped_links(self):
        """relink() leaves links inside inline math (`$…$`, `\\(…\\)`) and escaped brackets literal, as rendered."""
        fixture(self.root)
        s = SITE.Site(self.root, REPO)
        docs = ["see $c[l](../01-dsu.hpp)$ and [l](../01-dsu.hpp)\n", "\\(c [l](../01-dsu.hpp)\\) y [l](../01-dsu.hpp)\n",
                "\\[l](../01-dsu.hpp) and \\$[l](../01-dsu.hpp)$\n"]
        new = [s.relink(d, "02-Data Structures/Docs/01-dsu.md", "02-data-structures/01-dsu.md") for d in docs]
        html = pm_ids(docs + new, "html")
        for d, h, g in zip(docs, html, html[len(docs):]):
            self.assertIn('href="../01-dsu.hpp"', h, d)
            self.assertEqual(g, h.replace('href="../01-dsu.hpp"', 'href="01-dsu.md"'), d)

    def test_table_cell_links(self):
        """Regression (P239): relink() finds code spans per table cell, as the tables extension splits rows, so a link
        Python-Markdown renders is rewritten and one inside a cell's code span stays literal (rendered tables are
        compared, dropped extra cells included; other blocks hit the per-line limit)."""
        src, page = "02-Data Structures/Docs/01-dsu.md", "02-data-structures/01-dsu.md"
        toks = ["`", "``", "|", "\\|", "\\`", "[l](../01-dsu.hpp)", " ", "x"]
        rng = random.Random(SEED)
        docs = ["| h | h |\n|---|---|\n| ``a | [l](../01-dsu.hpp) ` |\n", "| h |\n|---|\n| `a|b` [l](../01-dsu.hpp) |\n"]
        for _ in range({"quick": 300, "full": 2000}.get(MODE, 20 * ROUNDS)):
            row = lambda: "|".join("".join(rng.choice(toks) for _ in range(rng.randint(0, 4))) for _ in range(rng.randint(1, 4)))
            docs.append("\n".join([rng.choice(["| h | h |", "h | h", "| h |"]), rng.choice(["|---|---|", "---|---", "|---|", "- | -"]), row()]) + "\n")
        fixture(self.root)
        s = SITE.Site(self.root, REPO)
        new = [s.relink(d, src, page) for d in docs]
        html = pm_ids(docs + new, "html")
        tables = [(d, n, h, g) for d, n, h, g in zip(docs, new, html, html[len(docs):]) if "<table>" in h]
        bad = [t for t in tables if t[3] != t[2].replace('href="../01-dsu.hpp"', 'href="01-dsu.md"')]
        self.assertGreater(len(tables), len(docs) // 4)
        self.assertIn('<a href="01-dsu.md">l</a>', html[len(docs)], "the reviewer's row")
        self.assertEqual(bad[:3], [], f"{len(bad)} of {len(tables)} tables render differently after relink (seed {SEED})")


class Fixture(Base):
    def setUp(self):
        super().setUp()
        fixture(self.root)

    def site(self, repo=REPO, files=True):
        s = SITE.Site(self.root, repo)
        return s, (s.files() if files else None)

    def test_site_pages_targets(self):
        s, _ = self.site(files=False)
        self.assertEqual(s.pages, PAGES)
        self.assertEqual(list(s.pages), [t for t in s.rows if t in PAGES], "inventory order")
        want = dict(PAGES)
        for t, p in PAGES.items():
            f = t.split("/")[0]
            want[f"{f}/Docs/{pname(t)}.md"] = want[f"{ARTICLES}/{f}/{pname(t)}.md"] = p
        for f in ("01-Core", "02-Data Structures", "03-Geometry", "08-Python"):
            want[f] = want[f"{f}/00-index.md"] = want[f"{ARTICLES}/{f}/00-index.md"] = f"{fslug(f)}/index.md"
        want.update({f"{f}/Docs/00-notes.md": p for f, p in NOTES.items()})
        self.assertEqual(s.targets, want)
        self.assertEqual(s.folders, ["01-Core", "02-Data Structures", "03-Geometry", "08-Python"])

    def test_href_and_link(self):
        for repo in (REPO, None):
            s, _ = self.site(repo)
            page = "02-data-structures/01-dsu.md"
            gh = lambda kind, p, a="": f"{REPO}/{kind}/master/{quote(p)}" + (f"#{a}" if a else "") if repo else None
            cases = [("02-Data Structures/01-dsu.hpp", "index.md", "", "02-data-structures/01-dsu.md"),
                     ("02-Data Structures/01-dsu.hpp", page, "dsu_1", "01-dsu.md#dsu_1"),
                     ("01-Core/05-thing.hpp", page, "", "../01-core/05-thing.md"),
                     ("02-Data Structures/01-dsu.hpp", page, "nope", gh("blob", "02-Data Structures/01-dsu.hpp", "nope")),
                     ("02-Data Structures/04-missing.hpp", page, "", None), ("02-Data Structures", "index.md", "",
                                                                             "02-data-structures/index.md"),
                     ("01-Core/Docs/00-notes.md", page, "shared", "../01-core/notes.md#shared"),
                     ("96-Local Testing", page, "", gh("tree", "96-Local Testing")),
                     ("96-Local Testing/02-Data Structures/01-dsu_tester.py", page, "L3",
                      gh("blob", "96-Local Testing/02-Data Structures/01-dsu_tester.py", "L3")),
                     ("01-Core/Docs/00-sources.md", page, "", gh("blob", "01-Core/Docs/00-sources.md")),
                     ("../outside.md", page, "", None), (str(self.root / "01-Core/01-template.hpp"), page, "", None),
                     ("/etc", page, "", None)]
            for target, frm, anchor, want in cases:
                with self.subTest(repo=repo, target=target, anchor=anchor):
                    self.assertEqual(s.href(target, frm, anchor), want)
            self.assertEqual(s.link("lbl", "02-Data Structures/04-missing.hpp", page), "lbl")
            self.assertEqual(s.link("lbl", "02-Data Structures/02-adv.hpp", page), "[lbl](02-adv.md)")
            self.assertEqual(s.link("lbl", "01-Core/Docs/00-sources.md", page),
                             f"[lbl]({REPO}/blob/master/01-Core/Docs/00-sources.md)" if repo else "lbl")

    def test_relink_cases(self):
        src, page = "02-Data Structures/Docs/01-dsu.md", "02-data-structures/01-dsu.md"
        absolute = str(self.root / "01-Core/01-template.hpp")
        for repo in (REPO, None):
            s, _ = self.site(repo)
            gh = lambda kind, p, a, label: f"[{label}]({REPO}/{kind}/master/{quote(p)}{'#' + a if a else ''})" if repo else label
            cases = [
                (src, page, "[a](../02-adv.hpp)", "[a](02-adv.md)"),
                (src, page, "[a](02-adv.md#contracts)", "[a](02-adv.md#contracts)"),
                (src, page, "[a](#limits-and-handoffs)", "[a](01-dsu.md#limits-and-handoffs)"),
                (src, page, "[a](#dsu_1)", "[a](01-dsu.md#dsu_1)"), (src, page, "[a](<00-notes.md>)", "[a](notes.md)"),
                (src, page, "[a](../../01-Core/05-thing.hpp)", "[a](../01-core/05-thing.md)"),
                (src, page, "[a](../../02-Data%20Structures/01-dsu.hpp)", "[a](01-dsu.md)"),
                (src, page, "[a](<../../02-Data Structures/02-adv.hpp#L1>)", gh("blob", "02-Data Structures/02-adv.hpp", "L1", "a")),
                (src, page, "[a](https://x.org/a.md) [m](mailto:a@b.c)", "[a](https://x.org/a.md) [m](mailto:a@b.c)"),
                (src, page, "`[a](../01-dsu.hpp)` ``x [b](../01-dsu.hpp) ` x``", "`[a](../01-dsu.hpp)` ``x [b](../01-dsu.hpp) ` x``"),
                (src, page, "`[a](../01-dsu.hpp)` and [b](../01-dsu.hpp)", "`[a](../01-dsu.hpp)` and [b](01-dsu.md)"),
                (src, page, "[`DSU::findSet`](../01-dsu.hpp)", "[`DSU::findSet`](01-dsu.md)"),
                (src, page, "[a](../nothere.hpp)", "a"), (src, page, "[a](..) [b](../)", "[a](index.md) [b](index.md)"),
                (src, page, "[a](../00-index.md#advanced)", "[a](index.md#advanced)"),
                (src, page, "[a](../00-index.md#nope)", gh("blob", "02-Data Structures/00-index.md", "nope", "a")),
                (src, page, "[a](../../96-Local%20Testing/02-Data%20Structures/01-dsu_tester.py)",
                 gh("blob", "96-Local Testing/02-Data Structures/01-dsu_tester.py", "", "a")),
                (src, page, "[a](<../../96-Local Testing/>)", gh("tree", "96-Local Testing", "", "a")),
                (src, page, "[a](../../../outside.md)", "a"), (src, page, f"[a]({absolute})", "a"), (src, page, "[a](/etc)", "a"),
                (src, page, "```\n[a](../01-dsu.hpp)\n```\n[b](../01-dsu.hpp)", "```\n[a](../01-dsu.hpp)\n```\n[b](01-dsu.md)"),
                (src, page, "~~~~\n```\n[a](x)\n~~~\n[b](y)\n~~~~\n[c](..)", "~~~~\n```\n[a](x)\n~~~\n[b](y)\n~~~~\n[c](index.md)"),
                (f"{ARTICLES}/02-Data Structures/01-dsu.md", page,
                 "[a](<../../../02-Data Structures/Docs/01-dsu.md#contracts>) [b](#overview)",
                 "[a](01-dsu.md#contracts) [b](01-dsu.md#overview)"),
                ("02-Data Structures/00-index.md", "02-data-structures/index.md",
                 "[n](Docs/00-notes.md) [e](Docs/01-dsu.md) [s](Docs/00-sources.md)", "[n](notes.md) [e](01-dsu.md) s"),
                ("01-Core/Docs/05-thing.md", "01-core/05-thing.md", "[x](../../02-Data%20Structures/Docs/01-dsu.md#dsu)",
                 "[x](../02-data-structures/01-dsu.md#dsu)"),
                ("01-Core/00-index.md", "01-core/index.md", "[s](Docs/00-sources.md)", gh("blob", "01-Core/Docs/00-sources.md", "", "s")),
                (src, page, "Vertices `[0, n)` and [a](../02-adv.hpp).", "Vertices `[0, n)` and [a](02-adv.md)."),
                (src, page, "\\`[a](../01-dsu.hpp)\\` and `\\`[b](../01-dsu.hpp)` `c\\` [d](..)`", "\\`[a](01-dsu.md)\\` and "
                 "`\\`[b](01-dsu.md)` `c\\` [d](index.md)`"),  # no escapes inside a code span: `\\` is one
            ]
            for source, frm, text, want in cases:
                with self.subTest(repo=repo, text=text):
                    self.assertEqual(s.relink(text, source, frm), want)

    def test_overview_pages(self):
        s, files = self.site()
        chart = DEPS.chart(DEPS.headers(self.root), "02-Data Structures")
        ds = ("# 02 Data Structures\n\nScope: DS fixture; notes in [00-notes.md](notes.md), sources and "
              "[Core](../01-core/index.md).\n\n[Folder notes](notes.md): contracts and ownership rules shared by the folder's "
              "headers.\n\n## Include chart\n\nArrows point from a header to the headers it includes; the template is left "
              f"out.\n\n```mermaid\n{chart}\n```\n\n## Basic\n\n| Header | Operations | Status |\n|---|---|---|\n"
              "| [`01-dsu.hpp`](01-dsu.md) | DSU: findSet, uniteSets; groups (sorted; stable); makeSet | verified; "
              "[evidence](01-dsu.md) |\n| [`03-red.hpp`](03-red.md) | Red: mul | verified |\n\n## Advanced\n\n"
              "| Header | Operations | Status |\n|---|---|---|\n| [`02-adv.hpp`](02-adv.md) | Adv: q | in-progress |\n"
              "| `04-missing.hpp` | M: r | planned |\n")
        self.assertEqual(files["docs/02-data-structures/index.md"], ds)
        names = dict(re.findall(r'(n\d+)\["([^"]+)"\]', chart))
        edges = {(names[a], names[b]) for a, b in re.findall(r"(n\d+) --> (n\d+)", chart)}
        self.assertEqual(edges, {("02-adv.hpp", "01-dsu.hpp"), ("02-adv.hpp", "05-thing.hpp"), ("03-red.hpp", "03-barrett.hpp")})
        core = files["docs/01-core/index.md"]
        self.assertIn("\n\n| Header | Basic | Operations | Status |\n|---|---|---|---|\n| [`01-template.hpp`](01-template.md) "
                      "| yes | macros: fi, se; chmin (a; b) | verified; [01-template.md](01-template.md) |\n", core)
        self.assertIn("| `07-planned.hpp` | no | Planned: q | planned |\n", core)
        self.assertIn(f"sources in [00-sources.md]({REPO}/blob/master/01-Core/Docs/00-sources.md).", core)
        self.assertNotIn("## Core", core)
        self.assertIn("```mermaid\n", core)
        geo = files["docs/03-geometry/index.md"]
        self.assertIn("## Include chart\n\nEvery header of this folder includes only the template.\n\n## Basic", geo)
        self.assertNotIn("mermaid", geo)
        self.assertNotIn("notes", geo.lower())
        py = files["docs/08-python/index.md"]
        self.assertNotIn("## Include chart", py)
        self.assertIn("## Basic\n\n| Module | Operations | Status |\n|---|---|---|\n| [`_01_x.py`](01-x.md) | readInts; Fast: "
                      "read, write | planned; pyrival |\n| `_02_y.py` | y | planned |\n", py)
        self.assertEqual(files["docs/02-data-structures/notes.md"], "# 02 Data Structures — notes\n\n## Rules\n\n- See "
                         "[dsu](01-dsu.md) and [core notes](../01-core/notes.md#shared).\n")
        self.assertIn("[DS](../02-data-structures/index.md#advanced)", files["docs/01-core/notes.md"])

    def test_relink_code_span_rule(self):
        """Regression (P239): a link in a code span that Python-Markdown opens after an escaped-backslash pair, an
        escaped backtick or one backtick into an unclosed longer run stays literal (see Units.test_code_span_rule)."""
        s, _ = self.site(files=False)
        src, page = "02-Data Structures/Docs/01-dsu.md", "02-data-structures/01-dsu.md"
        for text in ("a \\\\`[a](../01-dsu.hpp)`", "``[a](../01-dsu.hpp)`", "\\``[a](../01-dsu.hpp)`"):
            with self.subTest(text=text):
                self.assertEqual(links(text), [], "the oracle sees the link in a code span")
                self.assertEqual(s.relink(text, src, page), text)

    def test_anchors_per_source(self):
        """A heading id on a page belongs to the document it was copied from: an anchored link into a copied document
        lands on the page id its own heading got; a link to any other target uses the page's ids."""
        art = self.root / ARTICLES / "02-Data Structures/01-dsu.md"
        art.write_text(ARTICLE + "\n### DSU\n\nArticle DSU: [own](#dsu), [evidence](<../../../02-Data Structures/Docs/01-dsu.md#dsu>), "
                       "[tests](<../../../02-Data Structures/Docs/01-dsu.md#tests>), [header](<../../../02-Data Structures/01-dsu.hpp#dsu>).\n")
        for repo in (REPO, None):
            s, files = self.site(repo)
            dsu = files["docs/02-data-structures/01-dsu.md"]
            ids = line_ids(dsu)
            self.assertEqual(SITE.heading_ids(dsu), ids)
            at = {v: k for k, v in ids.items()}
            self.assertEqual([dsu.split("\n")[at[i]] for i in ("dsu", "dsu_1", "dsu_2")], ["### DSU"] * 3)
            self.assertLess(at["dsu"], at["contracts"], "the article's DSU comes first and takes `dsu`")
            tests = f"[tests]({REPO}/blob/master/02-Data%20Structures/Docs/01-dsu.md#tests)" if repo else "tests"
            self.assertIn(f"Article DSU: [own](01-dsu.md#dsu), [evidence](01-dsu.md#dsu_1), {tests}, [header](01-dsu.md#dsu).", dsu)
            self.assertIn("[second](01-dsu.md#dsu_2)", dsu, "the evidence's own #dsu_1")
            self.assertIn("[DSU contracts](../02-data-structures/01-dsu.md#dsu_1)", files["docs/01-core/05-thing.md"])
            self.assertIn("[DSU](01-dsu.md#dsu_1)", files["docs/02-data-structures/02-adv.md"])
            self.assertEqual(s.ids["02-data-structures/01-dsu.md"], {
                f"{ARTICLES}/02-Data Structures/01-dsu.md": {"overview": "overview", "dsu": "dsu"},
                "02-Data Structures/Docs/01-dsu.md": {"contracts": "contracts", "dsu": "dsu_1", "dsu_1": "dsu_2",
                                                      "limits-and-handoffs": "limits-and-handoffs", "sources": "sources"}})
            self.assertEqual(s.ids["02-data-structures/03-red.md"], {"02-Data Structures/Docs/03-red.md": {}}, "no evidence file")
            pages = {k[5:]: v for k, v in files.items() if k.endswith(".md")}
            self.assertGreaterEqual(self.check_copied_links(pages, repo, self.root, list(PAGES), FOLDERS), 6)
            self.check_links(pages, repo, self.root)

    def test_folder_article(self):
        """Articles/<NN-Folder>/00-index.md: merged without its title after the scope and notes link, before the include
        chart; relinked relative to itself; its anchors map per source; not an orphan."""
        before = {repo: self.site(repo)[1] for repo in (REPO, None)}
        art = f"{ARTICLES}/02-Data Structures"
        write(self.root, {f"{art}/00-index.md": "# Data structures guide\n\nStart with [the DSU](<../../../02-Data Structures/01-dsu.hpp>)"
                          " and [usage](#usage).\n\n## Usage\n\nPick by [contracts](<../../../02-Data Structures/Docs/01-dsu.md#contracts>);"
                          " ![fig](fig.png).\n", f"{art}/fig.png": "PNG",
                          f"{ARTICLES}/08-Python/00-index.md": "# Python guide\n\n## Python usage\n\nSee [x](../../../08-Python/_01_x.py).\n",
                          f"{ARTICLES}/03-Geometry/00-index.md": "# Only a title\n"})
        notes = self.root / "02-Data Structures/Docs/00-notes.md"
        notes.write_text(notes.read_text() + "- [guide](<../../98-Team Notebook/Articles/02-Data Structures/00-index.md#usage>), "
                         "[top](<../../98-Team Notebook/Articles/02-Data Structures/00-index.md>).\n")
        for repo in (REPO, None):
            s, files = self.site(repo)
            body = "Start with [the DSU](01-dsu.md) and [usage](index.md#usage).\n\n## Usage\n\nPick by [contracts](01-dsu.md#contracts); ![fig](fig.png)."
            ds = before[repo]["docs/02-data-structures/index.md"]
            self.assertIn("headers.\n\n## Include chart", ds)
            self.assertEqual(files["docs/02-data-structures/index.md"], ds.replace("headers.\n\n## Include chart", f"headers.\n\n{body}\n\n## Include chart"))
            self.assertTrue(files["docs/08-python/index.md"].startswith("# 08 Python\n\nScope: Python fixture.\n\n## Python usage\n\n"
                                                                     "See [x](01-x.md).\n\n## Basic\n\n| Module |"))
            self.assertEqual(files["docs/03-geometry/index.md"], before[repo]["docs/03-geometry/index.md"], "a title-only article adds nothing")
            self.assertIn("- [guide](index.md#usage), [top](index.md).", files["docs/02-data-structures/notes.md"])
            self.assertEqual(s.ids["02-data-structures/index.md"][f"{art}/00-index.md"], {"usage": "usage"})
            self.assertEqual(s.assets, {f"{art}/fig.png": "02-data-structures/fig.png"})
            pages = {k[5:]: v for k, v in files.items() if k.endswith(".md")}
            for p, text in pages.items():
                self.assertEqual(md_violations(text), [], p)
            self.check_links(pages, repo, self.root, assets={"02-data-structures/fig.png"})
            self.assertGreaterEqual(self.check_copied_links(pages, repo, self.root, list(PAGES), FOLDERS), 2)

    def test_inventory_anchor_beside_folder_article(self):
        """Regression (P239): the inventory's tier headings carry their source line, so a link to `00-index.md#basic`
        lands on the tier heading (page id `basic_1`), not on a folder-article heading `## Basic` (page id `basic`)."""
        write(self.root, {f"{ARTICLES}/02-Data Structures/00-index.md": "# Guide\n\n## Basic\n\nStart here.\n"})
        notes = self.root / "02-Data Structures/Docs/00-notes.md"
        notes.write_text(notes.read_text() + "- [tier](../00-index.md#basic), "
                         "[article](<../../98-Team Notebook/Articles/02-Data Structures/00-index.md#basic>).\n")
        _, files = self.site()
        ov = files["docs/02-data-structures/index.md"]
        tier = next(k for k, a in line_ids(ov).items() if ov.split("\n")[k + 2].startswith("| Header |"))
        self.assertEqual((line_ids(ov)[tier], ov.split("\n")[tier]), ("basic_1", "## Basic"))
        self.assertIn("[article](index.md#basic)", files["docs/02-data-structures/notes.md"])
        self.assertIn("[tier](index.md#basic_1)", files["docs/02-data-structures/notes.md"])

    def test_header_complexity(self):
        write(self.root, {"02-Data Structures/02-adv.hpp": T + inc("01-dsu.hpp", "../01-Core/05-thing.hpp") +
                          "// S: O(n), Q: O(1), M: O(n)\n// q in [0, n).\nstruct Adv {\n    // T: O(log(n))\n    int q(int i,\n"
                          "          int d = {}) const { return i; }\n};\n",
                          "08-Python/_01_x.py": 'S = """\n// T: O(1)\nint f() {}\n"""\n'})
        _, files = self.site()
        adv = files["docs/02-data-structures/02-adv.md"]
        self.assertIn("\n\n## Operations\n\n- Adv: q\n\n## Complexity\n\n- `struct Adv`: `S: O(n), Q: O(1), M: O(n)`\n"
                      "- `int q(int i, int d = {}) const`: `T: O(log(n))`\n\n## Contracts\n", adv)
        self.assertEqual(h2(adv), ["Operations", "Complexity", "Contracts", "Dependencies", "Tests", "Source"])
        for p in ("02-data-structures/01-dsu.md", "08-python/01-x.md", "01-core/05-thing.md"):
            self.assertNotIn("## Complexity", files["docs/" + p], p)

    def test_overview_cell_anchor_links(self):
        """An anchored link in an inventory cell is a site link like any other relinked text, on the first files()."""
        index = self.root / "02-Data Structures/00-index.md"
        index.write_text(index.read_text().replace("| Adv: q | in-progress |", "| Adv: q | in-progress; "
                                                   "[contracts](Docs/02-adv.md#contracts) |"))
        s = SITE.Site(self.root, None)
        first, second = s.files(), s.files()
        self.assertIn("| [`02-adv.hpp`](02-adv.md) | Adv: q | in-progress; [contracts](02-adv.md#contracts) |\n",
                      first["docs/02-data-structures/index.md"])
        self.assertEqual(first, second, "files() depends on an earlier call")

    def test_header_page_dsu(self):
        for repo in (REPO, None):
            s, files = self.site(repo)
            page = files["docs/02-data-structures/01-dsu.md"]
            gh = lambda p: f"{REPO}/blob/master/{quote(p)}"
            facts = "[02 Data Structures](index.md) · Basic · verified · 3 lines" + (
                f" · [source]({gh('02-Data Structures/01-dsu.hpp')}) · [evidence]({gh('02-Data Structures/Docs/01-dsu.md')})"
                if repo else "")
            self.assertTrue(page.startswith(f'# 01-dsu.hpp\n\n{facts}\n\n```cpp\n#include "02-Data Structures/01-dsu.hpp"\n'
                                            "```\n\n## Overview\n\nUnion by size"), page[:600])
            self.assertNotIn("DSU article title", page)
            self.assertIn("\n\n## Operations\n\n- DSU: findSet, uniteSets\n- groups (sorted; stable)\n- makeSet\n\n## Contracts", page)
            literal = ["`[code](../01-dsu.hpp)`", "``[two](../01-dsu.hpp) ` tick``", "[fenced](../01-dsu.hpp)",
                       "[tilde](../01-dsu.hpp)", "[web](https://cp-algorithms.com/x.html)", "[mail](mailto:a@b.c)",
                       "[KACTL](https://github.com/kactl/kactl/blob/main/content/data-structures/UnionFind.h)"]
            linked = ["[adv](02-adv.md)", "[its contracts](02-adv.md#contracts)", "[second](01-dsu.md#dsu_1)",
                      "[notes](notes.md)", "[core](../01-core/05-thing.md)", "[pct](01-dsu.md)", "[`DSU::findSet`](01-dsu.md)",
                      "[folder](index.md)", "[index](index.md#advanced)", "[again](01-dsu.md)",
                      "[the contracts](01-dsu.md#contracts)", "[this overview](01-dsu.md#overview)",
                      "[the advanced header](02-adv.md)", "[limits](01-dsu.md#limits-and-handoffs)"]
            github = {"line": gh("02-Data Structures/02-adv.hpp") + "#L1",
                      "bad anchor": gh("02-Data Structures/00-index.md") + "#nope",
                      "tester": gh("96-Local Testing/02-Data Structures/01-dsu_tester.py"),
                      "tests": f"{REPO}/tree/master/96-Local%20Testing", "geometry": gh("03-Geometry/01-point.hpp") + "#point"}
            for x in literal + linked + ([f"[{k}]({v})" for k, v in github.items()] if repo else []):
                self.assertIn(x, page)
            for label in ["gone", "escape", "absolute"] + ([] if repo else list(github)):
                self.assertIn(label, page)
                self.assertNotIn(f"[{label}](", page, label)
            for hidden in ("ignored", "hidden", "Not copied", "DSU article title"):
                self.assertNotIn(hidden, page)
            want = {"01-dsuhpp", "overview", "operations", "contracts", "dsu", "dsu_1", "dependencies", "limits-and-handoffs", "sources",
                    "tests", "source"}
            self.assertEqual(SITE.anchors(page), want)
            self.assertEqual(heading_ids(page), want)
            tests = ["96-Local Testing/02-Data Structures/01-dsu_tester.cpp", "96-Local Testing/02-Data Structures/01-dsu_tester.py"]
            self.assertIn("## Tests\n\n" + "\n".join(f"- [`{t}`]({gh(t)})" if repo else f"- `{t}`" for t in tests) + "\n\n", page)
            self.assertTrue(page.endswith('## Source\n\n??? abstract "01-dsu.hpp, 3 lines"\n\n    ```cpp\n    #pragma once\n'
                                          '    #include "../01-Core/01-template.hpp"\n    struct DSU {};\n    ```\n'), page[-300:])
            self.assertIn("\n\n## Limits and handoffs\n\n- Recursion-free; ", page)
            self.assertIn("\n\n## Sources\n\n- [KACTL]", page)

    def test_header_page_python(self):
        _, files = self.site()
        page = files["docs/08-python/01-x.md"]
        self.assertTrue(page.startswith(f"# _01_x.py\n\n[08 Python](index.md) · Basic · planned · 7 lines · [source]({REPO}/"
                                        f"blob/master/08-Python/_01_x.py) · [evidence]({REPO}/blob/master/08-Python/Docs/01-x.md)"
                                        "\n\n## Operations\n\n- readInts\n- Fast: read, write\n\n## Contracts\n\n- readInts "
                                        "returns [ints](01-x.md).\n\n## Source\n\n"), page)
        self.assertNotIn("#include", page)
        self.assertTrue(page.endswith('??? abstract "_01_x.py, 7 lines"\n\n    `````python\n    DOC = """\n    ````\n    """\n'
                                      '\n\n    def readInts():\n        return 1\n    `````\n'), page[-300:])
        self.assertEqual(count_lines(self.root / "08-Python/_01_x.py"), 7)

    def test_header_section_order(self):
        _, files = self.site()
        order = {"02-data-structures/01-dsu.md": ["Overview", "Operations", "Contracts", "Dependencies", "Limits and handoffs", "Sources",
                                                  "Tests", "Source"],
                 "02-data-structures/02-adv.md": ["Operations", "Contracts", "Dependencies", "Tests", "Source"],
                 "02-data-structures/03-red.md": ["Operations", "Dependencies", "Source"],
                 "01-core/05-thing.md": ["Operations", "Contracts", "Dependencies", "Sources", "Source"],
                 "08-python/01-x.md": ["Operations", "Contracts", "Source"]}
        for p, want in order.items():
            self.assertEqual(h2(files["docs/" + p]), want, p)
        self.assertIn(f"[01 Core](index.md) · Core · Basic: no · partial · {count_lines(self.root / '01-Core/05-thing.hpp')} "
                      "lines · [source]", files["docs/01-core/05-thing.md"])
        self.assertIn("· Basic: yes · existing-unverified · 3 lines ·", files["docs/01-core/06-thingmini.md"])

    def test_dependencies_sections(self):
        _, files = self.site()
        graph, n = scan(self.root), lambda h: count_lines(self.root / h)
        link = {h: f"[`{h}`]({os.path.relpath(p, '02-data-structures')})" for h, p in PAGES.items()}
        status = {"01-Core/01-template.hpp": "verified", "02-Data Structures/01-dsu.hpp": "verified",
                  "01-Core/03-barrett.hpp": "verified", "01-Core/05-thing.hpp": "partial",
                  "02-Data Structures/02-adv.hpp": "in-progress", "01-Core/06-thingmini.hpp": "existing-unverified"}
        name = lambda h: f"{link[h]} ({status[h]})"
        adv = "02-Data Structures/02-adv.hpp"
        order = post_order(graph, adv)
        self.assertEqual(order, ["01-Core/01-template.hpp", "02-Data Structures/01-dsu.hpp", "01-Core/03-barrett.hpp",
                                 "01-Core/05-thing.hpp", adv])
        mini = sum(map(n, ["01-Core/01-template.hpp", "02-Data Structures/01-dsu.hpp", "01-Core/06-thingmini.hpp", adv]))
        want = ("## Dependencies\n\nIncludes: " + ", ".join(name(h) for h in graph[adv]) + ".\n\nTyping order (each file after "
                f"the files it includes): 5 files, {sum(map(n, order))} lines.\n\n| Lines | File |\n|---|---|\n"
                + "".join(f"| {n(h)} | {name(h)} |\n" for h in order)
                + f"\nA mini exists but is not tested with this header: {name('01-Core/06-thingmini.hpp')} for "
                f"{name('01-Core/05-thing.hpp')} ({mini} lines in total with it).\n\nIncluded by: no header.\n\n")
        self.assertIn(want, files["docs/02-data-structures/02-adv.md"])
        red = files["docs/02-data-structures/03-red.md"]
        self.assertIn("Typing order (each file after the files it includes): 2 files, "
                      f"{n('01-Core/01-template.hpp') + n('02-Data Structures/03-red.hpp')} lines.", red)
        self.assertIn("\n\nSkippable when typing only the `*Compact` twins and code that needs no Core reduction: "
                      f"[`01-Core/03-barrett.hpp`](../01-core/03-barrett.md) (verified, 3 lines).\n\nIncluded by: no header.", red)
        self.assertIn(f"Included by: {name(adv)}.", files["docs/02-data-structures/01-dsu.md"])
        tpl = files["docs/01-core/01-template.md"]
        self.assertIn("Includes: nothing.", tpl)
        users = [h for h, ds in graph.items() if "01-Core/01-template.hpp" in ds and not re.search(r"/9[89]-", h)]
        self.assertEqual(sorted(re.findall(r"`([^`]+)`", re.search(r"Included by: (.*)\.\n", tpl)[1])), sorted(users))
        self.assertNotIn("## Dependencies", files["docs/08-python/01-x.md"])

    def test_home_page(self):
        _, files = self.site()
        self.assertTrue(files["docs/index.md"].startswith("# CompProg Library\n\nGenerated from"))
        self.assertTrue(files["docs/index.md"].endswith(
            "| Folder | Rows | Pages | Verified |\n|---|---|---|---|\n| [01 Core](01-core/index.md) | 5 | 4 | 2 |\n"
            "| [02 Data Structures](02-data-structures/index.md) | 4 | 3 | 2 |\n| [03 Geometry](03-geometry/index.md) | 1 | 1 | 1 |\n"
            "| [08 Python](08-python/index.md) | 2 | 1 | 0 |\n"))

    def test_files_nav_yaml(self):
        for repo in (REPO, None):
            _, files = self.site(repo)
            md = {"index.md", *(f"{fslug(f)}/index.md" for f in ("01-Core", "02-Data Structures", "03-Geometry", "08-Python")),
                  *NOTES.values(), *PAGES.values()}
            self.assertEqual(set(files), {"mkdocs.yml", "docs/javascripts/mathjax.js", *("docs/" + p for p in md)})
            self.assertIn("MathJax", files["docs/javascripts/mathjax.js"])
            data = parse_yaml(files["mkdocs.yml"])
            if data is None:
                self.skipTest("SKIP: no yaml module (system python or the MkDocs venv)")
            page = lambda t: {t.split("/")[1]: PAGES[t]}
            nav = [{"Home": "index.md"},
                   {"01 Core": ["01-core/index.md", {"Notes": "01-core/notes.md"}] + [page(t) for t in PAGES if t.startswith("01-")]},
                   {"02 Data Structures": ["02-data-structures/index.md", {"Notes": "02-data-structures/notes.md"},
                                           {"Basic": [page("02-Data Structures/01-dsu.hpp"), page("02-Data Structures/03-red.hpp")]},
                                           {"Advanced": [page("02-Data Structures/02-adv.hpp")]}]},
                   {"03 Geometry": ["03-geometry/index.md", {"Basic": [page("03-Geometry/01-point.hpp")]}]},
                   {"08 Python": ["08-python/index.md", {"Basic": [page("08-Python/_01_x.py")]}]}]
            self.assertEqual(data["nav"], nav)
            flat = []
            walk = lambda x: [walk(v) for v in x] if isinstance(x, list) else [walk(v) for v in x.values()] if isinstance(
                x, dict) else flat.append(x)
            walk(data["nav"])
            self.assertEqual(sorted(flat), sorted(md), "every page exactly once")
            self.assertEqual(data.get("repo_url"), repo)
            self.assertEqual((data["docs_dir"], data["site_dir"], data["use_directory_urls"], data["validation"]),
                             ("docs", "html", False, {"anchors": "warn"}))
            ext = {next(iter(e)) if isinstance(e, dict) else e: e for e in data["markdown_extensions"]}
            self.assertEqual(ext["toc"]["toc"]["slugify"], {"tag": "object/apply:pymdownx.slugs.slugify",
                                                            "value": {"kwds": {"case": "lower"}}})
            self.assertEqual(ext["pymdownx.superfences"]["pymdownx.superfences"]["custom_fences"][0]["format"],
                             {"tag": "name:pymdownx.superfences.fence_code_format", "value": ""})
            self.assertIn("javascripts/mathjax.js", data["extra_javascript"])
            self.assertEqual(data["plugins"], ["search", "offline", "privacy"])

    def test_files_orphan_articles(self):
        write(self.root, {f"{ARTICLES}/02-Data Structures/09-none.md": "# x\n", f"{ARTICLES}/01-Core/07-planned.md": "# p\n",
                          f"{ARTICLES}/Misc/x.md": "# m\n"})
        with self.assertRaises(SITE.SiteError) as e:
            SITE.Site(self.root, REPO).files()
        self.assertEqual(str(e.exception), "Article or asset without an existing inventory header, module or folder: "
                         f"{ARTICLES}/01-Core/07-planned.md, {ARTICLES}/02-Data Structures/09-none.md, {ARTICLES}/Misc/x.md")

    def test_generate_writes_only_out(self):
        out = self.base / "out"
        write(out, {"docs/stale.md": "old", "docs/sub/old.md": "old", "keep.txt": "k", "html/x.html": "h"})
        before = {p: p.read_bytes() for p in self.base.rglob("*") if p.is_file() and out not in p.parents}
        n = SITE.generate(self.root, out)
        after = {p: p.read_bytes() for p in self.base.rglob("*") if p.is_file() and out not in p.parents}
        self.assertEqual(after, before, "generate wrote outside out")
        files = SITE.Site(self.root, None).files()
        self.assertEqual(n, sum(k.endswith(".md") for k in files))
        written = {p.relative_to(out).as_posix() for p in out.rglob("*") if p.is_file()}
        self.assertEqual(written, set(files) | {"keep.txt", "html/x.html"})
        for k, v in files.items():
            self.assertEqual((out / k).read_text(encoding="utf-8"), v, k)

    def test_header_last_commit_git_fixture(self):
        git(self.root, "init", "-q")
        git(self.root, "remote", "add", "origin", "git@github.com:u/r.git")
        git(self.root, "add", "-A")
        git(self.root, "commit", "-q", "-m", "one", date="2020-01-02T03:04:05+00:00")
        (self.root / "02-Data Structures/01-dsu.hpp").write_text(T + inc("../01-Core/01-template.hpp") + "struct DSU { };\n")
        git(self.root, "commit", "-q", "-am", "two", date="2021-03-04T05:06:07+00:00")
        out = self.base / "out"
        SITE.generate(self.root, out)
        dsu, red = (out / "docs/02-data-structures/01-dsu.md").read_text(), (out / "docs/02-data-structures/03-red.md").read_text()
        self.assertIn(f"verified · 3 lines · last commit 2021-03-04 · [source]({REPO}/blob/master/02-Data%20Structures/01-dsu.hpp)", dsu)
        self.assertIn("· last commit 2020-01-02 · [source]", red)
        self.assertTrue(all(re.match(r"0[1-8]-", p) for p in SITE.changed(self.root)))

    def test_tracked_files_only(self):
        """Inside git, only tracked files get repository links and Tests entries (a clone generates the same pages)."""
        self.assertIsNone(SITE.tracked(self.root), "outside git")
        write(self.root, {".gitignore": "*_benchmark.jsonl\n", "96-Local Testing/02-Data Structures/01-dsu_benchmark.jsonl": "{}\n"})
        git(self.root, "init", "-q")
        git(self.root, "add", "-A")
        git(self.root, "commit", "-q", "-m", "one")
        write(self.root, {"96-Local Testing/02-Data Structures/01-dsu_untracked.py": "x\n"})
        known = SITE.tracked(self.root)
        self.assertIn("96-Local Testing/02-Data Structures/01-dsu_tester.py", known)
        self.assertTrue({"96-Local Testing", "96-Local Testing/02-Data Structures", "02-Data Structures"} <= known)
        self.assertFalse({"96-Local Testing/02-Data Structures/01-dsu_benchmark.jsonl",
                          "96-Local Testing/02-Data Structures/01-dsu_untracked.py", ""} & known)
        page = SITE.Site(self.root, REPO).files()["docs/02-data-structures/01-dsu.md"]
        tests = page.split("## Tests\n\n")[1].split("\n\n")[0]
        self.assertIn("01-dsu_tester.py", tests)
        self.assertNotIn("jsonl", page)
        self.assertNotIn("01-dsu_untracked", page)

    def test_missing_git(self):
        with patch.object(SITE.subprocess, "run", side_effect=FileNotFoundError("git")):
            self.assertIsNone(SITE.git(self.root, "status"))
            self.assertIsNone(SITE.repository(self.root))
            self.assertEqual(SITE.changed(self.root), {})
            self.assertIsNone(SITE.tracked(self.root))
            self.assertIn("docs/02-data-structures/01-dsu.md", SITE.Site(self.root, None).files())

    def test_article_assets_and_images(self):
        art = f"{ARTICLES}/02-Data Structures"
        write(self.root, {f"{art}/fig.png": "PNG", f"{art}/img/a.svg": "<svg/>", f"{art}/01-dsu.md": ARTICLE +
                          "\n![fig](fig.png) ![a](<img/a.svg>) ![t](<../../../96-Local Testing/02-Data Structures/01-dsu_tester.py>)"
                          " ![x](missing.png) [plain](fig.png) `![c](fig.png)`\n"})
        for repo in (REPO, None):
            s = SITE.Site(self.root, repo)
            self.assertEqual(s.assets, {f"{art}/fig.png": "02-data-structures/fig.png", f"{art}/img/a.svg": "02-data-structures/img/a.svg"})
            page = s.files()["docs/02-data-structures/01-dsu.md"]
            raw = f"![t]({REPO}/blob/master/{quote('96-Local Testing/02-Data Structures/01-dsu_tester.py')}?raw=true)" if repo else "t"
            self.assertIn(f"![fig](fig.png) ![a](img/a.svg) {raw} x [plain](fig.png) `![c](fig.png)`", page)
        out = self.base / "out"
        SITE.generate(self.root, out)
        self.assertEqual((out / "docs/02-data-structures/fig.png").read_text(), "PNG")
        self.assertEqual((out / "docs/02-data-structures/img/a.svg").read_text(), "<svg/>")
        write(self.root, {f"{ARTICLES}/Misc/b.png": "x", f"{ARTICLES}/c.png": "y"})
        with self.assertRaises(SITE.SiteError) as e:
            SITE.Site(self.root, REPO).files()
        self.assertEqual(str(e.exception), "Article or asset without an existing inventory header, module or folder: "
                         f"{ARTICLES}/Misc/b.png, {ARTICLES}/c.png")

    def test_fixture_markdown_rules_and_links(self):
        for repo in (REPO, None):
            _, files = self.site(repo)
            pages = {k[5:]: v for k, v in files.items() if k.endswith(".md")}
            for p, text in pages.items():
                self.assertEqual(md_violations(text), [], p)
                self.assertEqual(SITE.anchors(text), heading_ids(text), p)
                self.assertEqual(SITE.heading_ids(text), line_ids(text), p)
            self.check_links(pages, repo, self.root)
            self.assertGreaterEqual(self.check_copied_links(pages, repo, self.root, list(PAGES), FOLDERS), 4)


class Cli(Base):
    """06-site.py copied into a fixture, so its ROOT and SITE are the fixture's."""

    def setUp(self):
        super().setUp()
        fixture(self.root)
        for s in SCRIPTS:
            (self.root / s).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy(ROOT / s, self.root / s)
        self.mod = load(self.root / SCRIPTS[0], f"site_cli_{id(self)}")
        self.assertEqual(self.mod.SITE, self.root / "98-Team Notebook/site")

    def main(self, *argv):
        out, err = io.StringIO(), io.StringIO()
        with patch.object(sys, "argv", ["06-site.py", *argv]), contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
            rc = self.mod.main()
        return rc, out.getvalue(), err.getvalue()

    def test_cli_generate(self):
        rc, out, err = self.main("generate")
        n = sum(1 for _ in (self.mod.SITE / "docs").rglob("*.md"))
        self.assertEqual((rc, out, err), (0, f"Generated {n} pages in {self.mod.SITE}\n", ""))
        self.assertEqual(n, 1 + 4 + len(NOTES) + len(PAGES))
        self.assertTrue((self.mod.SITE / "mkdocs.yml").is_file())

    def test_cli_engine_commands(self):
        """build and serve run the chosen engine as [exe, command, -f, SITE/mkdocs.yml, *args] in SITE (patched
        subprocess for the engine; never a real server) and print `Using <engine> (<exe>)` after generating."""
        yml, n = str(self.mod.SITE / "mkdocs.yml"), 1 + 4 + len(NOTES) + len(PAGES)
        for k, (zs, ms, argv, used, want, rc) in enumerate([
                ("good", "good", ["build"], "zensical", ["build", "-f", yml, "--strict"], 0),
                ("good", "good", ["build", "--engine", "mkdocs"], "mkdocs", ["build", "-f", yml, "--strict"], 3),
                ("good", "good", ["build", "--engine", "zensical"], "zensical", ["build", "-f", yml, "--strict"], 1),
                ("bare", "good", ["build"], "mkdocs", ["build", "-f", yml, "--strict"], 0),
                ("material", "good", ["build", "--engine", "auto"], "mkdocs", ["build", "-f", yml, "--strict"], 0),
                (None, "good", ["build"], "mkdocs", ["build", "-f", yml, "--strict"], 0),
                ("good", "good", ["serve"], "zensical", ["serve", "-f", yml, "--dev-addr", "127.0.0.1:8000"], 0),
                ("good", "good", ["serve", "--engine", "mkdocs", "--port", "8123"], "mkdocs",
                 ["serve", "-f", yml, "--dev-addr", "127.0.0.1:8123"], 0),
                ("good", None, ["serve", "--port", "9"], "zensical", ["serve", "-f", yml, "--dev-addr", "127.0.0.1:9"], 0)]):
            with self.subTest(zensical=zs, mkdocs=ms, argv=argv):
                bin = fake_engines(self.base / f"bin{k}", zensical=zs, mkdocs=ms)
                got, out, err, calls = self.run_fake(bin, rc, *argv)
                exe = str(bin / used)
                self.assertEqual((got, err, calls), (rc, "", [([exe, *want], self.mod.SITE)]))
                self.assertEqual(out, f"Generated {n} pages in {self.mod.SITE}\nUsing {used} ({exe})\n")
                self.assertFalse((bin / "ran").exists(), "an engine ran outside the patched call")

    def run_fake(self, bin: Path, rc: int, *argv):
        """main() with PATH = bin, recording (command, cwd) of each call of a fake engine instead of running it."""
        calls, real = [], subprocess.run

        def run(cmd, *a, **k):
            if str(Path(cmd[0]).parent) == str(bin):
                calls.append((list(cmd), k.get("cwd")))
                return subprocess.CompletedProcess(cmd, rc)
            return real(cmd, *a, **k)
        with patch.dict(os.environ, {"PATH": str(bin)}), patch.object(self.mod.subprocess, "run", run):
            got, out, err = self.main(*argv)
        return got, out, err, calls

    def test_cli_missing_engines(self):
        hint = self.mod.INSTALL
        for k, (zs, ms, argv, want) in enumerate([
                (None, None, ["build"], f"zensical not found; mkdocs not found; {hint}"),
                (None, "good", ["build", "--engine", "zensical"], f"zensical not found; {hint}"),
                ("bare", "material", ["serve"], "zensical at {z} lacks zensical (install them for its Python); mkdocs at "
                 "{m} lacks pymdown-extensions (install them for its Python); " + hint),
                ("good", "bare", ["serve", "--engine", "mkdocs"], "mkdocs at {m} lacks mkdocs-material, pymdown-extensions "
                 "(install them for its Python); " + hint)]):
            with self.subTest(zensical=zs, mkdocs=ms, argv=argv):
                bin = fake_engines(self.base / f"bin{k}", zensical=zs, mkdocs=ms)
                rc, out, err, calls = self.run_fake(bin, 0, *argv)
                self.assertEqual((rc, err, calls), (1, want.replace("{z}", str(bin / "zensical")).replace("{m}", str(bin / "mkdocs")) + "\n", []))
                self.assertIn("Generated", out)
                self.assertNotIn("Using", out)
        for argv in (["build", "--engine", "sphinx"], ["generate", "--engine", "mkdocs"]):
            with self.subTest(argv=argv), self.assertRaises(SystemExit) as e:
                self.main(*argv)
            self.assertEqual(e.exception.code, 2)

    def test_cli_orphan_exit(self):
        write(self.root, {f"{ARTICLES}/03-Geometry/09-none.md": "# x\n"})
        rc, out, err = self.main("generate")
        self.assertEqual((rc, out), (1, ""))
        self.assertIn(f"{ARTICLES}/03-Geometry/09-none.md", err)
        self.assertFalse((self.mod.SITE / "docs").exists())

    def test_cli_honours_patched_site(self):
        """main() with the module's SITE patched (the documented way to keep a test out of the repository's site/)."""
        elsewhere = self.base / "elsewhere"
        with patch.object(self.mod, "SITE", elsewhere):
            rc, out, _ = self.main("generate")
        self.assertEqual(rc, 0)
        self.assertIn(str(elsewhere), out)
        self.assertTrue((elsewhere / "docs/index.md").is_file(), "generate() ignored the patched SITE")
        self.assertFalse((self.root / "98-Team Notebook/site").exists(), "main() wrote to the default site directory")


class Engines(Base):
    """engine() against fake engines in a temporary bin directory; PATH is only that directory."""

    def test_engine_choice(self):
        hint = SITE.INSTALL
        for word in ("pipx install zensical", "mkdocs-material", "pymdown-extensions"):
            self.assertIn(word, hint)
        lacks = lambda e, exe, pkgs: f"{e} at {exe} lacks {pkgs} (install them for its Python)"
        Z, M = ("zensical", "{z}"), ("mkdocs", "{m}")
        cases = [("auto", dict(zensical="good", mkdocs="good"), Z), ("zensical", dict(zensical="good", mkdocs="good"), Z),
                 ("mkdocs", dict(zensical="good", mkdocs="good"), M), ("auto", dict(zensical="bare", mkdocs="good"), M),
                 ("auto", dict(zensical="material", mkdocs="good"), M), ("auto", dict(zensical="noexec", mkdocs="good"), M),
                 ("auto", dict(mkdocs="good"), M), ("auto", dict(zensical="sh", mkdocs="bare"), Z),
                 ("auto", dict(zensical="env", mkdocs="bare"), Z), ("mkdocs", dict(mkdocs="sh"), M),
                 ("zensical", dict(zensical="bare", mkdocs="good"), f"{lacks('zensical', '{z}', 'zensical')}; {hint}"),
                 ("auto", dict(zensical="material", mkdocs="material"),
                  f"{lacks('zensical', '{z}', 'zensical')}; {lacks('mkdocs', '{m}', 'pymdown-extensions')}; {hint}"),
                 ("auto", dict(mkdocs="bare"), f"zensical not found; {lacks('mkdocs', '{m}', 'mkdocs-material, pymdown-extensions')}; {hint}"),
                 ("auto", {}, f"zensical not found; mkdocs not found; {hint}"), ("mkdocs", dict(zensical="good"), f"mkdocs not found; {hint}"),
                 ("auto", dict(zensical="noexec"), f"zensical not found; mkdocs not found; {hint}"),
                 ("auto", dict(zensical="gone", mkdocs="good"), M),
                 ("zensical", dict(zensical="gone"), f"zensical at {{z}} cannot run its Python (No such file or directory); {hint}")]
        for k, (name, kinds, want) in enumerate(cases):
            with self.subTest(name=name, kinds=kinds):
                bin = fake_engines(self.base / f"bin{k}", **kinds)
                with patch.dict(os.environ, {"PATH": str(bin)}):
                    got = SITE.engine(name)
                fill = lambda x: x.replace("{z}", str(bin / "zensical")).replace("{m}", str(bin / "mkdocs"))
                self.assertEqual(got, (want[0], fill(want[1])) if isinstance(want, tuple) else fill(want))
                self.assertFalse((bin / "ran").exists(), "engine() ran an engine")
        bin = fake_engines(self.base / "default", zensical="good", mkdocs="good")
        with patch.dict(os.environ, {"PATH": str(bin)}):
            self.assertEqual(SITE.engine(), ("zensical", str(bin / "zensical")), "auto, preferring Zensical, is the default")


class Build(Base):
    """Strict builds of the fixture site, once per engine executable from engines()."""

    def setUp(self):
        super().setUp()
        fixture(self.root)

    def generated(self) -> Path:
        out = Path(tempfile.mkdtemp(prefix="out-", dir=self.base))
        SITE.generate(self.root, out)
        return out

    def test_build_fixture_strict(self):
        def check(name, exe):
            out = self.generated()
            r = build(exe, out)
            self.assertEqual(r.returncode, 0, r.stdout[-2000:] + r.stderr[-3000:])
            for p, text in read_docs(out).items():
                ids = built_ids(out / "html" / Path(p).with_suffix(".html"))
                self.assertEqual(len(ids), len(set(ids)), p)
                self.assertEqual(set(ids), SITE.anchors(text), p)
            for f, chart in (("01-core", True), ("02-data-structures", True), ("03-geometry", False), ("08-python", False)):
                self.assertEqual('class="mermaid"' in (out / f"html/{f}/index.html").read_text(), chart, f)
            self.assertTrue((out / "html" / SEARCH[name]).is_file(), SEARCH[name])
            self.assertIn('class="arithmatex"', (out / "html/02-data-structures/01-dsu.html").read_text())
            self.assertFalse((self.root / ".cache").exists() or (self.root.parent / ".cache").exists(), "cache outside out")
        self.per_engine(check)

    def test_build_broken_anchor_fails(self):
        def check(name, exe):
            out = self.generated()
            with open(out / "docs/02-data-structures/02-adv.md", "a") as f:
                f.write("\n[broken](01-dsu.md#no-such-anchor)\n")
            r = build(exe, out)
            self.assertNotEqual(r.returncode, 0, "strict build accepted a broken anchor")
            self.assertIn("no-such-anchor", plain(r))
            (out / "docs/02-data-structures/02-adv.md").write_text("[gone](no-page.md)\n")
            r = build(exe, out)
            self.assertNotEqual(r.returncode, 0, "strict build accepted a link to a missing page")
            self.assertIn("no-page.md", plain(r))
        self.per_engine(check)

    def test_build_privacy_self_hosted(self):
        """plugins: [search, offline, privacy]: no unpkg.com, fonts.googleapis.com or cdn.jsdelivr.net URL is left in
        a src or href attribute of a built page; they point into assets/external, which holds MathJax's SVG build (no
        font files to fetch), Mermaid and the iframe-worker shim (needs network)."""
        if not online():
            self.skipTest("SKIP: offline (unpkg.com, fonts.googleapis.com or cdn.jsdelivr.net unreachable)")

        def check(name, exe):
            out = self.generated()
            r = build(exe, out)
            self.assertEqual(r.returncode, 0, r.stdout[-2000:] + r.stderr[-3000:])
            pages = sorted((out / "html").rglob("*.html"))
            self.assertGreaterEqual(len(pages), 1 + 4 + len(NOTES) + len(PAGES))
            for page in pages:
                parser = Attrs()
                parser.feed(page.read_text(encoding="utf-8"))
                self.assertEqual([u for u in parser.urls if EXTERNAL.match(u)], [], page)
                self.assertTrue(any("assets/external/" in u for u in parser.urls), page)
            ext = [p.relative_to(out / "html/assets/external").as_posix() for p in (out / "html/assets/external").rglob("*") if p.is_file()]
            self.assertIn("unpkg.com/mathjax@3/es5/tex-svg-full.js", ext, "MathJax SVG needs no font files")
            for part in ("mermaid", "iframe-worker"):
                self.assertTrue(any(part in e for e in ext), f"{part} is self-hosted: {ext}")
        self.per_engine(check)


class Repository(Base):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="site-repo-")
        cls.out = Path(cls.temp.name) / "site"
        cls.count = SITE.generate(ROOT, cls.out)
        cls.pages = read_docs(cls.out)
        url = subprocess.run(["git", "-C", str(ROOT), "config", "--get", "remote.origin.url"], capture_output=True,
                             text=True).stdout.strip()
        url = ("https://" + url[4:].replace(":", "/", 1) if url.startswith("git@") else url).removesuffix(".git")
        url = re.sub(r"^ssh://(?:[^@/]+@)?([^:/]+)(?::\d+)?/", r"https://\1/", re.sub(r"^https://[^/@]+@", "https://", url))
        cls.repo = url if url.startswith("https://") else None
        cls.graph = scan(ROOT)
        cls.rows = {}
        for index in sorted(ROOT.glob("0[1-8]-*/00-index.md")):
            for m in re.finditer(r"^\| `((?:\d{2}-[^`]+\.hpp)|(?:_\d{2}_[^`]+\.py))` \|", index.read_text(encoding="utf-8"), re.M):
                cls.rows.setdefault(f"{index.parent.name}/{m[1]}", None)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def page(self, t: str) -> str:
        return f"{fslug(t.split('/')[0])}/{pname(t)}.md"

    def test_repo_pages_match_inventory(self):
        self.assertEqual(SITE.repository(ROOT), self.repo)
        want = {self.page(t) for t in self.rows if (ROOT / t).is_file()}
        got = {p for p in self.pages if p != "index.md" and not p.endswith(("/index.md", "/notes.md"))}
        self.assertEqual(got, want)
        folders = {t.split("/")[0] for t in self.rows}
        self.assertEqual({p for p in self.pages if p.endswith("/index.md")}, {f"{fslug(f)}/index.md" for f in folders})
        self.assertEqual({p for p in self.pages if p.endswith("/notes.md")},
                         {f"{fslug(f)}/notes.md" for f in folders if (ROOT / f / "Docs/00-notes.md").is_file()})
        self.assertEqual(self.count, len(self.pages))
        for f in folders:
            text = self.pages[f"{fslug(f)}/index.md"]
            if f[:2] <= "07" and any(h.startswith(f + "/") for h in self.graph):
                self.assertTrue("```mermaid\n" in text or "Every header of this folder includes only the template." in text, f)
            else:
                self.assertNotIn("## Include chart", text, f)

    def test_repo_operations(self):
        """Each page's Operations bullets, joined by `;`, give back its row's Operations cell (no item lost or merged)."""
        for t, (cells, _) in SITE.PLAN.inventories(ROOT).items():
            if (ROOT / t).is_file():
                with self.subTest(target=t):
                    block = self.pages[self.page(t)].split("\n## Operations\n\n", 1)[1].split("\n\n", 1)[0]
                    bullets = [l[2:] for l in block.split("\n")]
                    self.assertTrue(all(l.startswith("- ") for l in block.split("\n")) and all(bullets), block)
                    squash = lambda x: re.sub(r";+", ";", "".join(x.split())).strip(";")  # whitespace and empty items ignored
                    self.assertEqual(squash(";".join(bullets)), squash(cells[-2]))
                    depth = [0]
                    for c in cells[-2]:
                        depth.append(depth[-1] + (c in "([") - (c in ")]"))
                    if min(depth) == 0 and depth[-1] == 0:  # balanced cells: every top-level `;` starts a bullet
                        self.assertEqual(len(bullets), sum(1 for i, c in enumerate(cells[-2]) if c == ";" and depth[i] == 0
                                                           and cells[-2][i + 1:].split(";")[0].strip()) + 1)

    def test_repo_evidence_sections(self):
        for t in self.rows:
            ev = ROOT / t.split("/")[0] / "Docs" / f"{pname(t)}.md"
            if not (ROOT / t).is_file() or not ev.is_file():
                continue
            page, found = delink(self.pages[self.page(t)]), own_sections(ev.read_text(encoding="utf-8"))
            for title in SECTION_TITLES:
                with self.subTest(target=t, section=title):
                    if title in found and found[title][1] != f"## {title}":  # copied verbatim from its heading line
                        self.assertIn(f"\n\n{delink(found[title][1])}\n\n", page)
                    else:
                        self.assertNotIn(f"\n## {title}\n", page)

    def test_repo_links_resolve(self):
        self.check_links(self.pages, self.repo, ROOT)

    def test_repo_heading_ids(self):
        for p, text in self.pages.items():
            self.assertEqual(SITE.anchors(text), heading_ids(text), p)
            self.assertEqual(SITE.heading_ids(text), line_ids(text), p)

    def test_repo_copied_anchor_links(self):
        """Every anchored link of a copied document into an evidence document (or another copied document) lands on
        the page heading copied from the heading with that id (same text), or on the repository when not copied."""
        targets = [t for t in self.rows if (ROOT / t).is_file()]
        n = self.check_copied_links(self.pages, self.repo, ROOT, targets, sorted({t.split("/")[0] for t in self.rows}))
        self.assertGreater(n, 0)

    def test_repo_complexity_code_spans(self):
        """Regression (P239): each Complexity bullet renders as two code spans holding exactly the declaration and the
        comment, also when either contains a backtick (05-Mathematics/10-crt.hpp: "mod `mod`")."""
        for t in self.rows:
            if t.endswith(".hpp") and (ROOT / t).is_file():
                got = own_sections(self.pages[self.page(t)]).get("Complexity")
                for line, (d, c) in zip(got[1].split("\n")[2:] if got else [], bounds((ROOT / t).read_text(encoding="utf-8"))):
                    with self.subTest(target=t, line=line):
                        self.assertEqual([x for _, _, x, code in code_pass(line) if code], [d.strip(), c.strip()])

    def test_repo_complexity(self):
        """Each header page lists every complexity line of its header, in order, under Complexity after Operations;
        modules and headers without one have no Complexity section."""
        n = 0
        for t in self.rows:
            if (ROOT / t).is_file():
                with self.subTest(target=t):
                    page = self.pages[self.page(t)]
                    want = bounds((ROOT / t).read_text(encoding="utf-8")) if t.endswith(".hpp") else []
                    got = own_sections(page).get("Complexity")
                    if want:
                        self.assertEqual(got[1], "## Complexity\n\n" + "\n".join(f"- {span(d)}: {span(c)}" for d, c in want))
                        self.assertEqual(h2(page)[h2(page).index("Operations") + 1], "Complexity")
                        n += len(want)
                    else:
                        self.assertIsNone(got)
        self.assertGreater(n, 0)

    def test_repo_anchor_ids_python_markdown(self):
        self.need_venv()
        names = list(self.pages)
        for p, ids in zip(names, pm_ids([self.pages[p] for p in names])):
            self.assertEqual(len(ids), len(set(ids)), p)
            self.assertEqual(set(ids), SITE.anchors(self.pages[p]), p)

    def test_repo_markdown_rules(self):
        for p, text in self.pages.items():
            self.assertEqual(md_violations(text), [], p)

    def test_repo_dependencies(self):
        graph = DEPS.headers(ROOT)
        users = {h: [] for h in self.graph}
        for h, ds in self.graph.items():
            for d in ds:
                if d in users and not re.search(r"/9[89]-", h):
                    users[d].append(h)
        for t in self.rows:
            if t not in self.graph:
                continue
            with self.subTest(target=t):
                dep = own_sections(self.pages[self.page(t)])["Dependencies"][1]
                self.assertEqual(re.findall(r"`([^`]+)`", re.search(r"^Includes: (.*)\.$", dep, re.M)[1]), self.graph[t])
                m = re.search(r"^Typing order \(each file after the files it includes\): (\d+) files, (\d+) lines\.$", dep, re.M)
                rows = [(int(a), b) for a, b in re.findall(r"^\| (\d+) \| [^`]*`([^`]+)`", dep, re.M)]
                order = [h for _, h in rows]
                self.assertEqual(order, DEPS.plan(ROOT, graph, t)["order"])
                self.assertEqual([n for n, _ in rows], [count_lines(ROOT / h) for h in order])
                self.assertEqual((int(m[1]), int(m[2])), (len(order), sum(n for n, _ in rows)))
                skip = re.search(r"^Skippable [^:]*: (.*)\.$", dep, re.M)
                skipped = set(re.findall(r"`([^`]+)`", skip[1])) if skip else set()
                pos = {h: i for i, h in enumerate(order)}
                for h in order:
                    for d in self.graph[h]:
                        self.assertTrue(d in skipped or pos.get(d, len(order)) < pos[h], f"{d} not before {h}")
                by = re.search(r"^Included by: (.*)\.$", dep, re.M)[1]
                self.assertEqual(sorted(re.findall(r"`([^`]+)`", by) if by != "no header" else []), sorted(users[t]))

    def test_repo_last_commit(self):
        rng = random.Random(SEED)
        for t in rng.sample(sorted(t for t in self.rows if (ROOT / t).is_file()), 12):
            want = git(ROOT, "log", "-1", "--format=%cs", "--", t).strip()
            facts = self.pages[self.page(t)].split("\n")[2]
            got = re.search(r" · last commit (\d{4}-\d\d-\d\d)", facts)
            self.assertEqual(got[1] if got else "", want, t)

    def test_repo_strict_build_and_ids(self):
        def check(name, exe):
            out = Path(tempfile.mkdtemp(prefix=f"{name}-", dir=self.base))
            shutil.copytree(self.out / "docs", out / "docs")
            shutil.copy(self.out / "mkdocs.yml", out)
            r = build(exe, out)
            self.assertEqual(r.returncode, 0, r.stdout[-2000:] + r.stderr[-3000:])
            for p, text in self.pages.items():
                ids = built_ids(out / "html" / Path(p).with_suffix(".html"))
                self.assertEqual(len(ids), len(set(ids)), p)
                self.assertEqual(set(ids), SITE.anchors(text), p)
                if p.endswith("/index.md") and "```mermaid" in text:
                    self.assertIn('class="mermaid"', (out / "html" / Path(p).with_suffix(".html")).read_text(), p)
            self.assertTrue((out / "html" / SEARCH[name]).is_file())
        self.per_engine(check)


WORDS = ["alpha", "Beta", "γάμμα", "Größe", "naïve", "été", "中文", "x2", "C++", "a.b", "(p, q)", "1-2", "Σum",
         "ok:", "x/y", "it's", "DSU", "dsu"]
BOUNDS = ["// T: O(1)\nint f(int a) { return a; }\n", "// S: O(n), Q: O(1), M: O(n)\n// n in [0, 2^31).\nstruct S : B {\n};\n",
          "// T: O(n)\ntemplate <class T>\nT g(T x,\n    T y = {}) {\n    return x;\n}\n"]
FIXED = ["contracts", "operations", "limits", "limits-and-handoffs", "sources", "tests", "source", "dependencies", "nope",
         "basic", "advanced", "esoteric", "include-chart", "overview", "dsu", "dsu_1", "_1"]


class Tree:
    """A random fixture repository and the independent expectations for its site."""

    def __init__(self, rng: random.Random, root: Path):
        self.rng, self.root, self.files, self.links, self.copied, self.anchor_pool = rng, root, {}, [], {}, list(FIXED)
        self.seen = []
        self.folders = ["01-Core"] + sorted(rng.sample(["02-Data Structures", "03-Geometry", "05-Mathematics"],
                                                       rng.randint(1, 3))) + ["08-Python"] * (rng.random() < .6)
        self.rows, self.tables, self.exists, self.ops = {}, {}, set(), {}
        for f in self.folders:
            tiers = ["Core"] if f == "01-Core" else sorted(rng.sample(TIERS, rng.randint(1, 3)), key=TIERS.index)
            names = ["01-template.hpp"] * (f == "01-Core")
            for i in range(rng.randint(1, 4)):
                num = i + 1 + (f == "01-Core")
                names.append(f"_{num:02d}_m{i}.py" if f == "08-Python" else f"{num:02d}-h{i}.hpp")
            table = {t: [] for t in tiers}
            for n in names:
                t = f"{f}/{n}"
                table[rng.choice(tiers)].append(t)
                if n == "01-template.hpp" or rng.random() < .8:
                    self.exists.add(t)
            self.tables[f] = {k: v for k, v in table.items() if v}
            for tier, ts in self.tables[f].items():
                for t in ts:
                    self.rows[t] = tier
        self.order = [t for t in self.rows if t in self.exists]
        self.pages = {t: f"{fslug(t.split('/')[0])}/{pname(t)}.md" for t in self.order}
        self.notes = {f for f in self.folders if rng.random() < .5}
        self.targets = dict(self.pages)
        for t, p in self.pages.items():
            f = t.split("/")[0]
            self.targets[f"{f}/Docs/{pname(t)}.md"] = self.targets[f"{ARTICLES}/{f}/{pname(t)}.md"] = p
        for f in self.folders:
            self.targets[f] = self.targets[f"{f}/00-index.md"] = self.targets[f"{ARTICLES}/{f}/00-index.md"] = f"{fslug(f)}/index.md"
            if f in self.notes:
                self.targets[f"{f}/Docs/00-notes.md"] = f"{fslug(f)}/notes.md"
        self.write_headers()
        self.write_docs()

    def write_headers(self):
        rng, done = self.rng, []
        for t in self.order:
            if t.endswith(".py"):
                self.files[t] = "def f():\n    return 1\n" + "x = '```'\n" * rng.randint(0, 2)
                continue
            deps = ([] if t == "01-Core/01-template.hpp" else ["01-Core/01-template.hpp"]) + rng.sample(done, min(len(done), rng.randint(0, 2)))
            rel = [os.path.relpath(d, os.path.dirname(t)) for d in dict.fromkeys(deps)]
            self.files[t] = T + inc(*rel) + "int x;\n" * rng.randint(0, 5) + "".join(rng.sample(BOUNDS, rng.randint(0, 2))) + (
                "// end" if rng.random() < .3 else "")
            done.append(t)
        for f in self.folders:
            own = [t for t in self.order if t.startswith(f + "/") and t.endswith(".hpp")]
            if own and rng.random() < .5:
                self.files[f"{f}/99-all.hpp"] = inc(*(t.split("/")[1] for t in own))
            for t in own:
                if rng.random() < .4:
                    self.files[f"96-Local Testing/{f}/{pname(t)}_tester.py"] = ""

    def heading(self) -> str:
        """A random heading; a third repeat an earlier one or a generated page heading, so the ids of a copied
        document and of the page differ (`x` becomes `x_1`)."""
        rng, r = self.rng, self.rng.random()
        if r < .2 and self.seen:
            text = rng.choice(self.seen)
        elif r < .33:
            text = rng.choice(["Operations", "Complexity", "Overview", "Tests", "Source", "Dependencies", "Include chart", "Basic"])
        else:
            parts = ["..."] if rng.random() < .08 else rng.choices(WORDS, k=rng.randint(1, 3))
            if rng.random() < .2:
                parts.append(f"`{rng.choice(WORDS)}`")
            if rng.random() < .1:
                parts.append("[lab](../nothere.md)")
            text = " ".join(parts)
        self.seen.append(text)
        self.anchor_pool.append(heading_id(text) or "_1")
        return text

    def link(self, src: str, copied: bool, literal: bool = False) -> str:
        """A link with a unique label token, in copied text or an inventory cell."""
        rng, k = self.rng, len(self.links)
        label = f"T{k}T" if rng.random() < .8 else f"`T{k}T`"
        anchor = "#" + rng.choice(self.anchor_pool) if rng.random() < .5 else ""
        all_targets = list(self.rows) + [f"{f}/99-all.hpp" for f in self.folders]
        kind = rng.randrange(12)
        if kind == 0:
            url = "https://example.org/x.md" + anchor if rng.random() < .7 else "mailto:a@b.c"
        elif kind == 1:
            url = anchor or "#nope"
        else:
            if kind <= 3:
                path = rng.choice(all_targets)
            elif kind == 4:
                path = f"{rng.choice(all_targets).split('/')[0]}/Docs/{pname(rng.choice(list(self.rows)))}.md"
            elif kind == 5:
                path = rng.choice(self.folders) + rng.choice(["", "/00-index.md"])
            elif kind == 6:
                path = f"{rng.choice(self.folders)}/Docs/00-notes.md"
            elif kind == 7:
                t = rng.choice(list(self.rows))
                path = f"{ARTICLES}/{t.split('/')[0]}/" + (f"{pname(t)}.md" if rng.random() < .7 else "00-index.md")
            elif kind == 8:
                path = rng.choice(self.folders) + "/Docs/99-nothing.md"
            elif kind == 9:
                path = rng.choice([p for p in self.files if p.startswith("96-")] or ["96-Local Testing/none.py"])
            elif kind == 10:
                path = "../" * (src.count("/") + 1) + "outside.md"
            else:
                path = str(self.root / rng.choice(self.order))
            rel = path if path.startswith("/") or path.startswith("..") else os.path.relpath(path, os.path.dirname(src))
            form = rng.randrange(3)
            url = f"<{rel}{anchor}>" if form == 0 or (" " in rel and form == 1) else quote(rel) + anchor
        text = f"[{label}]({url})"
        self.links.append((label, src, url, literal, copied))
        return text

    def para(self, src: str, copied: bool) -> str:
        rng = self.rng
        out = ["Text " + rng.choice(WORDS)] + [self.link(src, copied) for _ in range(rng.randint(1, 3))]
        if rng.random() < .3:
            out.append("`" + self.link(src, copied, True) + "`")
        if rng.random() < .2:
            out.append("``a ` " + self.link(src, copied, True) + "``")
        return " ".join(out) + "."

    def block(self, src: str, copied: bool) -> str:
        rng, r = self.rng, self.rng.random()
        if r < .3:
            return f"{rng.choice(['###', '####'])} {self.heading()}"
        if r < .5:
            c, n = rng.choice("`~"), rng.randint(3, 5)
            inner = [f"## fake {rng.choice(WORDS)}", self.link(src, copied, True), "~~~" if c == "`" else "```"]
            if n > 3:
                inner.append(c * (n - 1))
            rng.shuffle(inner)
            return "\n".join([c * n + rng.choice(["", "text"]), *inner, c * n])
        return self.para(src, copied)

    def doc(self, src: str, title: str, sections: list[tuple[str, bool]], whole: bool = False) -> str:
        out = [f"# {title}", self.para(src, whole)]
        for name, copied in sections:
            out.append(f"## {name}")
            out += [self.block(src, copied) for _ in range(self.rng.randint(0, 4))]
        return "\n\n".join(out) + "\n"

    def write_docs(self):
        rng, self.orphans = self.rng, []
        for f in self.folders:
            src = f"{f}/00-index.md"
            out = [f"# {f} — inventory", "Scope: random " + self.link(src, True) + "."]
            core = f == "01-Core"
            for tier, ts in self.tables[f].items():
                if not core:
                    out.append(f"## {tier}")
                head = ["| Header | Basic | Operations | Status |", "|---|---|---|---|"] if core else [
                    f"| {'Module' if f == '08-Python' else 'Header'} | Operations | Status |", "|---|---|---|"]
                for t in ts:
                    ops = [rng.choice(["Op: a, b", "f (x; y)", "g", "", "h (p; (q; r))", "r ([0, n); [l, r))", "s [a; b)"]) for _ in range(rng.randint(1, 4))]
                    self.ops[t] = [o for o in ops if o]
                    status = rng.choice(["verified", "partial", "planned", "existing-unverified"]) + (
                        "; " + self.link(src, True) if rng.random() < .3 else "")
                    head.append(f"| `{t.split('/')[1]}` | " + (rng.choice(["yes", "no"]) + " | ") * core
                                + "; ".join(ops) + f" | {status} |")
                out.append("\n".join(head))
            self.files[src] = "\n\n".join(out) + "\n"
        for f in self.folders:
            if f in self.notes:
                src = f"{f}/Docs/00-notes.md"
                self.files[src] = self.doc(src, f"{f} notes", [("Rules", True), ("More", True)], True)
        names = ["Contracts", "Limits and handoffs", "Sources", "Tests", "Moves"]
        for t in self.rows:
            if rng.random() < .7:
                src = f"{t.split('/')[0]}/Docs/{pname(t)}.md"
                secs = rng.sample(names, rng.randint(1, 5))
                self.files[src] = self.doc(src, f"{t.split('/')[1]} — evidence",
                                           [(s, t in self.exists and s in names[:3]) for s in secs])
        for t in self.order:
            if rng.random() < .5:
                src = f"{ARTICLES}/{t.split('/')[0]}/{pname(t)}.md"
                self.files[src] = self.doc(src, "Article", [("Article " + rng.choice(WORDS[:5]), True)], True).replace(
                    "\n\n## Article", "\n\nMath $x^2$.\n\n## Article", 1)
        for f in self.folders:
            if rng.random() < .3:  # its section may share an id with a tier heading or the include chart
                src = f"{ARTICLES}/{f}/00-index.md"
                self.files[src] = self.doc(src, "Folder guide", [(rng.choice(TIERS + ["Include chart", "Guide"]), True)], True)
        for t in self.rows:
            if t not in self.exists and rng.random() < .1:
                self.orphans.append(f"{ARTICLES}/{t.split('/')[0]}/{pname(t)}.md")
        if rng.random() < .05:
            self.orphans.append(f"{ARTICLES}/{rng.choice(self.folders)}/77-ghost.md")
        self.files.update({o: "# Orphan\n" for o in self.orphans})
        self.files["../outside.md"] = "# outside\n"

    def expect(self, src: str, url: str, page: str, repo: str | None, ids: dict[str, set[str]],
               maps: dict[str, dict[str, str]]) -> str | None:
        """The rewritten URL: a site link when the target has a page and the anchor an id there (through the
        document's own id map for a copied document), else GitHub for an existing path, else None (label only)."""
        raw = url[1:-1] if url.startswith("<") else url
        if "://" in raw or raw.startswith("mailto:"):
            return url
        path, _, anchor = unquote(raw).partition("#")
        target = os.path.normpath(os.path.join(os.path.dirname(src), path)) if path else src
        local = "" if not anchor else maps[target].get(anchor) if target in maps else anchor if (
            target in self.targets and anchor in ids[self.targets[target]]) else None
        if target in self.targets and local is not None:
            return os.path.relpath(self.targets[target], os.path.dirname(page)) + (f"#{local}" if local else "")
        if repo and not target.startswith(("..", "/")) and (self.root / target).exists():
            kind = "tree" if (self.root / target).is_dir() else "blob"
            return f"{repo}/{kind}/master/{quote(target)}" + (f"#{anchor}" if anchor else "")
        return None


class Random(Base):
    def test_random_trees(self):
        rounds = {"quick": 5, "full": 20}.get(MODE, ROUNDS)
        builds = {0, rounds // 2, rounds - 1} if MODE == "stress" else set()
        pm_docs = []
        for r in range(rounds):
            rng = random.Random(SEED * 1_000_003 + r)
            root = self.base / f"r{r}" / "library"
            tree = Tree(rng, root)
            write(root, tree.files)
            repo = REPO if rng.random() < .6 else None
            ctx = f"seed={SEED} round={r} root={root}: "
            if tree.orphans:
                with self.assertRaises(SITE.SiteError, msg=ctx) as e:
                    SITE.Site(root, repo).files()
                self.assertEqual(str(e.exception), "Article or asset without an existing inventory header, module or folder: "
                                 + ", ".join(sorted(tree.orphans)), ctx)
                for o in tree.orphans:
                    (root / o).unlink()
            site = SITE.Site(root, repo)
            files = site.files()
            pages = {k[5:]: v for k, v in files.items() if k.endswith(".md")}
            self.assertEqual(site.pages, tree.pages, ctx)
            self.assertEqual(site.targets, tree.targets, ctx)
            self.assertEqual({p for p in pages if p != "index.md" and not p.endswith(("/index.md", "/notes.md"))},
                             set(tree.pages.values()), ctx)
            ids = {p: heading_ids(t) for p, t in pages.items()}
            for p, text in pages.items():
                self.assertEqual(SITE.anchors(text), ids[p], ctx + p)
                self.assertEqual(SITE.heading_ids(text), line_ids(text), ctx + p)
                self.assertEqual(md_violations(text), [], ctx + p)
            self.check_links(pages, repo, root, ctx)
            lay, maps = layout(root, tree.order, tree.folders), {}
            for d, (p, ranges) in lay.items():
                text = (root / d).read_text(encoding="utf-8") if (root / d).is_file() else ""
                maps[d] = id_map(text, placement(text, ranges, pages[p], ctx + d + ": "), pages[p])
                self.assertEqual(site.ids[p].get(d), maps[d], ctx + d)
            for f in tree.folders:  # an inventory's own ids on its overview: the tier headings above the tables
                d, p = f"{f}/00-index.md", f"{fslug(f)}/index.md"
                doc, lines, pids = (root / d).read_text(encoding="utf-8").split("\n"), pages[p].split("\n"), line_ids(pages[p])
                maps[d] = {a: pids[at[0]] for k, a in line_ids("\n".join(doc)).items() if doc[k] in TIER_HEADS and (at := [
                    j for j in pids if lines[j] == doc[k] and lines[j + 2:j + 3] and lines[j + 2].startswith(("| Header |", "| Module |"))])}
                self.assertEqual(site.ids[p].get(d), maps[d], ctx + d)
            self.check_copied_links(pages, repo, root, tree.order, tree.folders, ctx)
            for label, src, url, literal, copied in tree.links:
                token = label.strip("`")
                where = [p for p, t in pages.items() if re.search(rf"(?<![0-9]){token}(?![0-9])", t)]
                self.assertEqual(len(where), int(copied), f"{ctx}{token} from {src} ({url}) on {where}")
                if not copied:
                    continue
                text, page = pages[where[0]], where[0]
                if literal:
                    self.assertIn(f"[{label}]({url})", text, ctx + token)
                    continue
                new = tree.expect(src, url, page, repo, ids, maps)
                if new:
                    self.assertIn(f"[{label}]({new})", text, f"{ctx}{src}: {url}")
                else:
                    self.assertIn(label, text, ctx + token)
                    self.assertNotIn(f"[{label}](", text, f"{ctx}{src}: {url} should keep only its label")
            graph = scan(root)
            for t in tree.order:
                text = pages[tree.pages[t]]
                self.assertIn("## Operations\n\n" + "\n".join(f"- {o}" for o in tree.ops[t]), text, ctx + t)
                ev = root / t.split("/")[0] / "Docs" / f"{pname(t)}.md"
                found = own_sections(ev.read_text(encoding="utf-8")) if ev.is_file() else {}
                for title in SECTION_TITLES:
                    if title in found and found[title][1] != f"## {title}":
                        self.assertIn(f"\n\n{delink(found[title][1])}\n\n", delink(text), ctx + t)
                    else:
                        self.assertNotIn(f"\n## {title}\n", text, ctx + t)
                want = bounds(tree.files[t]) if t.endswith(".hpp") else []
                got = own_sections(text).get("Complexity")
                self.assertEqual(got[1] if got else None, "## Complexity\n\n" + "\n".join(f"- {span(d)}: {span(c)}" for d, c in want)
                                 if want else None, ctx + t)
                if t.endswith(".hpp"):
                    order = post_order(graph, t)
                    rows = re.findall(r"^\| (\d+) \| [^`]*`([^`]+)`", own_sections(text)["Dependencies"][1], re.M)
                    self.assertEqual(rows, [(str(count_lines(root / h)), h) for h in order], ctx + t)
            if len(pm_docs) < 400:
                pm_docs += list(pages.values())
            if r in builds:
                def check(name, exe):
                    out = Path(tempfile.mkdtemp(prefix=f"out-{name}-", dir=self.base / f"r{r}"))
                    SITE.generate(root, out)
                    b = build(exe, out)
                    self.assertEqual(b.returncode, 0, ctx + b.stdout[-2000:] + b.stderr[-3000:])
                    for p, text in read_docs(out).items():
                        self.assertEqual(set(built_ids(out / "html" / Path(p).with_suffix(".html"))), SITE.anchors(text), ctx + p)
                self.per_engine(check)
            shutil.rmtree(self.base / f"r{r}")
        if venv_python():
            for d, got in zip(pm_docs, pm_ids(pm_docs)):
                self.assertEqual(set(got), SITE.anchors(d), f"seed={SEED} Python-Markdown ids: {d[:200]!r}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--mode", choices=("quick", "full", "stress"), default=os.environ.get("CP_TEST_MODE", "full"))
    parser.add_argument("--seed", type=int, default=int(os.environ.get("CP_TEST_SEED", "42")))
    parser.add_argument("--rounds", type=int, default=200, help="random fixture trees in stress mode")
    args = parser.parse_args()
    MODE, SEED, ROUNDS = args.mode, args.seed, args.rounds
    suite = unittest.TestSuite()
    for case in (Units, Renderer, Fixture, Engines, Cli, Build, Repository, Random):
        names = [n for n in unittest.defaultTestLoader.getTestCaseNames(case) if MODE != "quick" or n not in FULL_ONLY]
        suite.addTests(case(n) for n in names)
    found = ", ".join(f"{e}={x}" + (" (unusable)" if why else "") for e, x, why in engines()) or "none"
    print(f"site checks: mode={MODE} seed={SEED} rounds={ROUNDS if MODE == 'stress' else '-'} engines: {found} "
          f"tests={suite.countTestCases()}", flush=True)
    result = unittest.TextTestRunner(verbosity=2).run(suite)
    ok, tty = result.wasSuccessful(), sys.stdout.isatty()
    print((f"\033[{32 if ok else 31}m" if tty else "") + ("PASS" if ok else "FAIL") + ("\033[0m" if tty else ""), flush=True)
    raise SystemExit(not ok)
