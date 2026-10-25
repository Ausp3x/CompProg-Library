#!/usr/bin/env python3
"""Documentation site for Zensical or MkDocs Material, generated from the canonical sources: one page per existing 01-08 header
or module from its inventory row, evidence document and include graph, one overview with an include chart and one
notes page per folder, and the hand-written articles in Articles/<NN-Folder>/<NN-stem>.md."""

from __future__ import annotations

import argparse
import html
import importlib.util
import json
import posixpath
import re
import shutil
import subprocess
import sys
import unicodedata
from pathlib import Path
from xml.etree.ElementTree import Element
from urllib.parse import quote, unquote

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
SITE = HERE / "site"  # generated and git-ignored: mkdocs.yml, docs/, html/
ARTICLES = "98-Team Notebook/Articles"
BRANCH = "master"
SECTIONS = ("Contracts", "Limits and handoffs", "Sources")  # copied from the evidence document, titles kept
ESCAPED = "\\`*_{}[]()>#+-.!|"  # characters Python-Markdown (with tables) unescapes after a backslash
LINK = re.compile(r"\[([^\]]+)\]\((<[^>]+>|[^\s)]+)\)")
TICK = re.compile(r"(?<!\\)((?:\\{2})+)(?=`+)|(?<!\\)`")  # Python-Markdown 3.11: a code span opener
RUN = re.compile("`+")
CODE_PIPES = re.compile(r"(\\\\)|(\\`+)|(`+)|(\\\|)|(\|)")  # the tables extension's row scanner
END_BORDER = re.compile(r"(?<!\\)(?:\\\\)*\|$")
FENCE = re.compile(r"""^\s*(`{3,}|~{3,})(?:[ \t]*\.?[\w#.+-]+(?=[\t ]|$))?(?:[ \t]*\{[^\n]*\}|((?:[ \t]*[a-zA-Z]\w*"""
                   r"""(?:=("|').*?\3)?(?=[\t ]|$))+))?[ \t]*$""")  # superfences: a language, then attributes or options
OPTION = re.compile(r"""([a-zA-Z]\w*)(?:=("|')(.*?)\2)?""")
HIGHLIGHT = {"hl_lines": r"\d+(?:-\d+)?(?:[ \t]+\d+(?:-\d+)?)*$", "linenums": r"\d+", "title": "", "title_mode": ""}  # option values
HEADING = re.compile(r"^(#{1,6})((?:\\.|[^\\])*?)#*$")  # Python-Markdown: no space needed, closing #s dropped
TAG = re.compile(r"""<(?:/?[a-zA-Z][^\s"'<>@]*(?:\s+[^\s"'=<>]+(?:\s*=\s*(?:"[^"]*"|'[^']*'|[^\s"'=<>]+))?)*\s*/?"""
                 r"|!--(?:(?!<!--|-->).)*--|[?](?:(?!<[?]|[?]>).)*[?]|!\[CDATA\[(?:(?!<!\[CDATA\[|\]\]>).)*\]\])>")  # inline HTML
MATHJAX = """window.MathJax = {
  tex: {inlineMath: [["\\\\(", "\\\\)"]], displayMath: [["\\\\[", "\\\\]"]], processEscapes: true, processEnvironments: true},
  options: {ignoreHtmlClass: ".*|", processHtmlClass: "arithmatex"}
};
document$.subscribe(() => {
  MathJax.startup.output.clearCache(); MathJax.typesetClear(); MathJax.texReset(); MathJax.typesetPromise();
});
"""
CONFIG = """site_name: CompProg Library
{repo}edit_uri: ""
docs_dir: docs
site_dir: html
use_directory_urls: false
validation:
  anchors: warn
theme:
  name: material
  features: [navigation.indexes, navigation.top, search.highlight, search.suggest, content.code.copy]
  palette:
    - scheme: default
      primary: cyan
      accent: cyan
      toggle: {{icon: material/brightness-4, name: Dark mode}}
    - scheme: slate
      primary: cyan
      accent: cyan
      toggle: {{icon: material/brightness-7, name: Light mode}}
plugins: [search, offline, privacy]  # privacy: fonts and scripts self-hosted under assets/external
markdown_extensions:
  - admonition
  - tables
  - pymdownx.details
  - pymdownx.highlight
  - pymdownx.arithmatex: {{generic: true}}
  - pymdownx.superfences:
      custom_fences:
        - name: mermaid
          class: mermaid
          format: !!python/name:pymdownx.superfences.fence_code_format
  - toc:
      permalink: true
      slugify: !!python/object/apply:pymdownx.slugs.slugify {{kwds: {{case: lower}}}}
extra_javascript: [javascripts/mathjax.js, "https://unpkg.com/mathjax@3/es5/tex-svg-full.js"]  # SVG: no font files to self-host
nav:
{nav}
"""


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


DEPS = load("site_dependencies", HERE / "05-dependencies.py")
PLAN = load("site_plan", ROOT / "00-Guidelines/13-Plan/plan.py")


class SiteError(ValueError):
    pass


Fragment = tuple[str, str | None, int | None]  # Markdown, repository file its links are relative to, its first line there


def pair(text: str, begin: int) -> tuple[int, int, int] | None:
    """Python-Markdown 3.11's code span opened by the backtick run at `begin`: (content start, content end, span
    end). It closes at the next run of the same length, else at the first longest later run, which then opens that
    many backticks before the content; None without a later run."""
    ticks, best = len(RUN.match(text, begin)[0]), None
    for r in RUN.finditer(text, begin + ticks):
        if len(r[0]) == ticks or not best or len(r[0]) > len(best[0]):
            best = r
        if len(r[0]) == ticks:
            break
    return best and (begin + len(best[0]), best.start(), best.end())


def spans(line: str) -> list[tuple[int, int, str | None]]:
    """(start, end, stripped code) of each inline code span of a line, and (start, end, None) of each escaped
    backslash run before backticks, in Python-Markdown 3.11's order (each search resumes after the last span)."""
    out, work, pos = [], line, 0
    while m := TICK.search(work, pos):
        if not m[1] and not (found := pair(work, m.start())):
            pos = m.end()
            continue
        out.append((m.start(), m.end(), None) if m[1] else (m.start(), found[2], line[found[0]:found[1]].strip()))
        work, pos = work[:out[-1][0]] + "\0" * (out[-1][1] - out[-1][0]) + work[out[-1][1]:], out[-1][1]
    return out


class Emphasis:
    """Python-Markdown 3.11's `*` and `_` emphasis (`inlinepatterns.DelimiterProcessor`, `_` smart), ported line for
    line; its state persists between matches. Regions are (open start, open end, close start, close end, size)."""

    def __init__(self):
        self.boundary, self.stacks = {}, {"*": [], "_": []}
        for t, e in (("*", r"\*"), ("_", "_")):
            if t == "_":  # smart: never inside a word
                xs, xe = r"(?<![\w_])", r"(?![\w_])"
                amb, end, start = rf"(?<!^)(?<![\s_]){xs}_+{xe}(?![\s_])(?!$)", rf"(?<!^)(?<![\s_])_+{xe}", rf"{xs}_+(?![\s_])(?!$)"
            else:
                amb, end, start = r"(?<!^)(?<![\s*])\*+(?![\s*])(?!$)", r"(?<!^)(?<![\s*])\*+", r"\*+(?![\s*])(?!$)"
            self.boundary[t] = re.compile(f"(?P<ambiguous>{amb})|(?P<end>{end})|(?P<start>{start})")
        self.reset()

    def reset(self) -> None:
        for stack in self.stacks.values():
            stack.clear()
        self.regions, self.stack, self.index, self.pos = [], [], 0, 0

    def element(self, data: str, start: int = 0, offset: int = 0) -> tuple[Element, int]:
        regions, el, last, previous, outer, outer_r, idx = self.regions, None, None, None, [], [], 0
        for idx, i in enumerate(range(start, len(regions)), 1):
            r = regions[i]
            if r[0] >= regions[start][3]:
                idx -= 1
                break
            new = Element("x")
            if idx > 1:
                if last.text is None:
                    last.text = data[previous[1] + offset:min(previous[2], r[0]) + offset]
                if last is not outer[-1] and last.tail is None:
                    if r[0] < outer_r[-1][3]:
                        last.tail = data[previous[3] + offset:r[0] + offset]
                    else:
                        last.tail = data[previous[3] + offset:outer_r[-1][2] + offset]
                        outer[-1].tail = data[outer_r[-1][3] + offset:r[0] + offset]
            if el is None:
                el = last = new
                outer, outer_r = [el], [r]
            else:
                while len(outer_r) > 1 and r[3] > outer_r[-1][3]:
                    outer.pop()
                    outer_r.pop()
                outer[-1].append(new)
                if i + 1 < len(regions) and r[3] > regions[i + 1][3]:
                    outer.append(new)
                    outer_r.append(r)
                last = new
            previous = r
        while outer:
            if last.text is None:
                last.text = data[previous[1] + offset:previous[2] + offset]
            if last.tail is None and last is not outer[-1]:
                last.tail = data[previous[3] + offset:outer_r[-1][2] + offset]
            last, previous = outer.pop(), outer_r.pop()
        return el, idx

    def advance(self, start: int, count: int) -> None:
        self.index += count
        if self.index >= len(self.regions):
            return self.reset()
        self.pos = self.regions[self.index][0]
        while self.stack:
            entry = self.stack.pop(0)
            if start < entry[0] <= self.pos:
                self.pos = entry[0]
                break

    def search(self, data: str, start: int) -> re.Match | None:
        for i in range(start, len(data)):
            if data[i] in self.stacks and (m := self.boundary[data[i]].match(data, i)):
                if self.stacks[data[i]] or m.lastgroup[0] != "e":
                    return m
        return None

    def region(self, a: int, b: int, c: int, d: int, size: int) -> None:
        self.regions.append((a, b, c, d, size))
        for stack in self.stacks.values():
            while stack and max(stack[-1][0], a) <= min(stack[-1][0], d - 1):
                stack.pop()

    def match(self, m: re.Match, data: str) -> tuple[Element | None, int, int]:
        """DelimiterProcessor.handleMatch for both tokens (neither is `double`; both have two tags)."""
        if self.regions:
            offset = m.start() - self.pos if m.start() != self.pos else m.start() - self.regions[self.index][0]
            start, end = self.regions[self.index][0], self.regions[self.index][3]
            el, count = self.element(data, self.index, offset)
            self.advance(start, count)
            return el, start + offset, end + offset
        m2 = self.boundary["*"].match(data, m.start()) or self.boundary["_"].match(data, m.start())
        if m2 is None or m2.lastgroup[0] == "e":
            m2 = m2 or re.compile(re.escape(m[0]) + "+").match(data, m.start())
            return None, m2.start(), m2.end()
        stack, start, end = self.stacks[m[0]], m2.start(), m2.end()
        stack.append((start, end, m2.lastgroup[0] != "s", end - start))
        while any(self.stacks.values()) and (m2 := self.search(data, end)):
            t = data[m2.start()]
            stack, start, end, current = self.stacks[t], m2.start(), m2.end(), len(m2[0])
            is_start = m2.lastgroup[0] != "e"
            is_end = not is_start or m2.lastgroup[0] != "s"
            ambiguous = is_start and is_end
            last = stack[-1][-1] if stack else 0
            if stack and is_end and ((not ambiguous and current > last) or current == last or current >= 3):
                is_start, original = False, current
                while stack and current and last <= current:
                    opening = stack.pop()
                    size = min(opening[-1], 1 if opening[-1] == 3 else 2)
                    self.region(opening[1] - size, opening[1], start, start + size, size)
                    start, current = start + size, current - size
                    if size < opening[-1]:
                        stack.append((opening[0], opening[1] - size, opening[2], opening[-1] - size))
                    if not stack:
                        if any(s for k, s in self.stacks.items() if k != t):
                            stack.clear()
                            continue
                        is_end = False
                        break
                    last = stack[-1][-1]
                if original >= 3 and current and ambiguous:
                    stack.append((m2.start() + original - current, end, False, current))
                    is_end = False
                else:
                    is_end = current and stack and last > current
            if stack and is_end and (last >= 3 or not ambiguous) and last > current:
                opening = stack.pop()
                while stack and opening[-1] != 3 and opening[2]:
                    opening = stack.pop()
                    last = opening[-1]
                if opening[2] and opening[-1] != 3:
                    if any(s for k, s in self.stacks.items() if k != t):
                        stack.clear()
                        continue
                    break
                is_start, (ds, de) = False, opening[:2]
                while current:
                    size = min(current, 1 if current == 3 else 2)
                    self.region(ds + last - size, de, start, start + size, size)
                    start, current, last, de = start + size, current - size, last - size, de - size
                if last:
                    stack.append((ds, de, False, last))
            if is_start:
                stack.append((start, end, ambiguous, current))
        self.stack = sorted((e for s in self.stacks.values() for e in s), key=lambda e: e[0])
        if self.regions:
            self.regions.sort(key=lambda r: r[0])
            start, end = self.regions[0][0], self.regions[0][3]
            el, count = self.element(data)
            self.advance(start, count)
            return el, start, end
        start, end = m.start(), self.stack[-1][1] if self.stack else m.end()
        self.reset()
        return None, start, end

class Atomic(str):
    """Text no inline pattern processes again (Python-Markdown's `AtomicString`)."""


class Heading:
    """A heading's inline rendering by Python-Markdown 3.11 with the site's extensions, ported: the patterns in
    priority order (code span, arithmatex math, escape, link, image, autolink, inline HTML, entity, emphasis), the
    inline tree processor (`__handleInline`, `__applyPattern`, `__processPlaceholders`, `__processElementText` and
    `run`'s breadth-first walk, which reruns the patterns on element texts and tails), then the serialization and
    postprocessing the toc reads. Links are the plain `[label](url)` form only."""
    KEY = re.compile("\x02klzzwxh:([0-9]+)\x03")
    MATH = re.compile(r"(?:(?<!\\)((?:\\{2})+)(?=\$)|(?<!\\)(\$)(?!\s)((?:\\.|[^\\$\x02\x03])+?)(?<!\s)(?:\$))|"
                      r"(?:(?<!\\)((?:\\{2})+?)(?=\\\()|(?<!\\)(\\\()((?:\\[^)]|[^\\\x02\x03])+?)(?:\\\)))")
    AUTOLINK = re.compile(r"<((?:[Ff]|[Hh][Tt])[Tt][Pp][Ss]?://[^<>]*)>")
    ENTITY = re.compile(r"(&(?:\#[0-9]+|\#x[0-9a-fA-F]+|[a-zA-Z0-9]+);)")
    AMP = re.compile(r"&(?!(?:\#[0-9]+|\#x[0-9a-f]+|[0-9a-z]+);)", re.I)
    BSLASH = f"\x02{ord(chr(92))}\x03"

    def __init__(self):
        self.stash, self.raw, self.emphasis = [], [], Emphasis()
        self.patterns = [self.code, self.math, self.escape, self.link, self.image, self.autolink, self.tag,
                         self.entity, self.emphasize]

    def code(self, data: str, start: int):
        while m := TICK.search(data, start):
            if m[1]:
                return m[1].replace("\\\\", self.BSLASH), m.start(), m.end()
            if found := pair(data, m.start()):
                el = Element("code")
                el.text = Atomic(data[found[0]:found[1]].strip().replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;"))
                return el, m.start(), found[2]
            start = m.end()
        return None

    def math(self, data: str, start: int):
        if not (m := self.MATH.search(data, start)):
            return None
        if escapes := m[1] or m[4]:
            return escapes.replace("\\\\", self.BSLASH), m.start(), m.end()
        el = Element("span")
        el.text = Atomic(f"\\({m[3] or m[6]}\\)")
        return el, m.start(), m.end()

    def escape(self, data: str, start: int):
        if not (m := re.compile(r"\\(.)").search(data, start)):
            return None
        return (f"\x02{ord(m[1])}\x03" if m[1] in ESCAPED + "$" else None), m.start(), m.end()

    def link(self, data: str, start: int, image: bool = False):
        if not (m := re.compile(("!" if image else "(?<!!)") + LINK.pattern).search(data, start)):
            return None
        el = Element("img" if image else "a")
        el.text = None if image else m[1]
        return el, m.start(), m.end()

    def image(self, data: str, start: int):
        return self.link(data, start, True)

    def autolink(self, data: str, start: int):
        if not (m := self.AUTOLINK.search(data, start)):
            return None
        el = Element("a")
        el.text = Atomic(m[1])
        return el, m.start(), m.end()

    def tag(self, data: str, start: int, pattern: re.Pattern = TAG):
        """Inline HTML (and entities) go to the raw stash, placeholders inside them rendered first."""
        if not (m := pattern.search(data, start)):
            return None

        def inner(text: str) -> str:
            return self.KEY.sub(lambda k: inner(self.html(n)) if isinstance(n := self.stash[int(k[1])], Element) else "\\" + n, text)
        self.raw.append(re.sub("\x02([0-9]+)\x03", lambda e: chr(int(e[1])), inner(m[0])))
        return f"\x02wzxhzdk:{len(self.raw) - 1}\x03", m.start(), m.end()

    def entity(self, data: str, start: int):
        return self.tag(data, start, self.ENTITY)

    def emphasize(self, data: str, start: int):
        return (m := re.compile(r"[*_]").search(data, start)) and self.emphasis.match(m, data)

    def handle(self, data: str, index: int = 0) -> str:
        """`__handleInline`: every pattern from `index` on, each searched again after its last placeholder."""
        start = 0
        while not isinstance(data, Atomic) and index < len(self.patterns):
            if not (found := self.patterns[index](data, start)):
                index, start = index + 1, 0
                continue
            node, a, b = found
            if node is None:
                start = b
                continue
            if isinstance(node, Element) and not isinstance(node.text, Atomic):
                for child in [node, *node]:
                    if child.text:
                        child.text = self.handle(child.text, index + 1)
                    if child.tail:
                        child.tail = self.handle(child.tail, index)
            self.stash.append(node)
            key = f"\x02klzzwxh:{len(self.stash) - 1:04d}\x03"
            data, start = data[:a] + key + data[b:], a + len(key)
        return data

    def expand(self, data: str, parent: Element, is_text: bool) -> list[Element]:
        """`__processPlaceholders`: the elements placed in `data`, its text and strings linked to `parent` or the
        last element."""
        out, start = [], 0

        def link(text: str) -> None:
            if not text:
                return
            if out or not is_text:
                el = out[-1] if out else parent
                el.tail = (el.tail or "") + text
            else:
                parent.text = parent.text + text if parent.text else text
        while m := self.KEY.search(data, start):
            node = self.stash[int(m[1])]
            link(data[start:m.start()])
            start = m.end()
            if isinstance(node, str):
                link(node)
                continue
            for child in [node, *node]:
                if child.tail and child.tail.strip():
                    self.place(node, child, False)
                if child.text and child.text.strip():
                    self.place(child, child, True)
            out.append(node)
        link(Atomic(data[start:]) if isinstance(data, Atomic) else data[start:])
        return out

    def place(self, node: Element, sub: Element, is_text: bool) -> None:
        """`__processElementText`."""
        text = sub.text if is_text else sub.tail
        if is_text:
            sub.text = None
        else:
            sub.tail = None
        at = list(node).index(sub) + 1 if not is_text and node is not sub else 0
        for new in reversed(self.expand(text, sub, is_text)):
            node.insert(at, new)

    def html(self, el: Element) -> str:
        """The serializer: text escaped (`&` only outside entities), elements as tags."""
        esc = lambda t: self.AMP.sub("&amp;", t).replace("<", "&lt;").replace(">", "&gt;") if t else ""
        inner = esc(el.text) + "".join(f"<{c.tag}>" + ("" if c.tag == "img" else self.html(c) + f"</{c.tag}>") + esc(c.tail) for c in el)
        return f"<{el.tag}>{inner}</{el.tag}>"

    def name(self, data: str) -> str:
        """The toc's text for a heading holding `data`: `run`'s walk, then the inner HTML with escapes and raw HTML
        restored (`render_inner_html`) and `strip_tags` (comments, then `<` to the next `>`; whitespace collapsed)."""
        root, head = Element("r"), Element("h")
        head.text = data
        root.append(head)
        stack = [root]
        while stack:
            current, queue = stack.pop(0), []
            for child in current:  # tail elements are inserted after `child` and visited in this loop
                if child.text and not isinstance(child.text, Atomic):
                    text, child.text = child.text, None
                    found = self.expand(self.handle(text), child, True)
                    stack += found
                    queue.append((child, found))
                if child.tail:
                    dummy, tail = Element("d"), self.handle(child.tail)
                    child.tail = None
                    found = self.expand(tail, dummy, False)
                    child.tail = dummy.tail
                    for new in reversed(found):
                        current.insert(list(current).index(child) + 1, new)
                if len(child):
                    stack.append(child)
            for el, found in queue:
                for i, new in enumerate(found):
                    el.insert(i, new)
        text = self.html(head)[len("<h>"):-len("</h>")].strip()
        unescape = lambda t: re.sub("\x02([0-9]+)\x03", lambda e: chr(int(e[1])), t)
        raw = lambda t: re.sub("<p>\x02wzxhzdk:([0-9]+)\x03</p>|\x02wzxhzdk:([0-9]+)\x03", lambda m: raw(self.raw[int(m[1] or m[2])]), t)
        text = unescape(raw(unescape(text)).replace("\x02amp\x03", "&")).strip()
        while (a := text.find("<!--")) != -1 and (b := text.find("-->", a)) != -1:
            text = text[:a] + text[b + 3:]
        while (a := text.find("<")) != -1 and (b := text.find(">", a)) != -1:
            text = text[:a] + text[b + 1:]
        return " ".join(text.split())


def slug(text: str) -> str:
    """Heading id as mkdocs renders it: the toc's text of the heading (`Heading.name`), entities decoded, through
    `pymdownx.slugs.slugify(case="lower")` (tags dropped, lowercased, all but word characters, hyphens and spaces removed, spaces as hyphens)."""
    text = html.unescape(Heading().name(text.strip()))
    text = re.sub(r"</?[^>]*>", "", unicodedata.normalize("NFC", text)).strip().lower()
    return re.sub(r"[^\w\- ]", "", text).replace(" ", "-")


def literal(text: str) -> str:
    """`text` as one inline code span, its delimiters longer than any backtick run inside."""
    ticks = "`" * (1 + max(map(len, RUN.findall(text)), default=0))
    return f"{ticks} {text} {ticks}" if len(ticks) > 1 else f"`{text}`"


def cells(line: str, border: bool) -> list[tuple[int, int]]:
    """(start, end) of each cell of a table row, as the tables extension splits it: a pipe inside a pair of equal
    backtick runs (an escaped run opens one backtick shorter) is no boundary; with a border the outer pipes go."""
    start = len(line) - len(line.lstrip(" "))
    row = line.strip(" ")
    if border:
        start += row.startswith("|")
        row = END_BORDER.sub("", row[1:] if row.startswith("|") else row)
    tics, pipes, regions, k = [], [], [], 0
    for m in CODE_PIPES.finditer(row):
        if m[2] or m[3]:
            tics.append((m.start(), m.end() - 1, m.end() - m.start() - 1 if m[2] else m.end() - m.start(), bool(m[2])))
        elif m[5]:
            pipes.append(m.start())
    while k < len(tics):
        size = tics[k][2] - tics[k][3]
        close = next((j for j in range(k + 1, len(tics)) if tics[j][2] == size), None) if size else None
        if close is None:
            k += 1
            continue
        regions.append((tics[k][0], tics[close][1]))
        k = close + 1
    bounds = [-1, *(p for p in pipes if not any(a <= p <= b for a, b in regions)), len(row)]
    return [(start + bounds[i] + 1, start + bounds[i + 1]) for i in range(len(bounds) - 1)]


def table_rows(lines: list[tuple[str, bool]]) -> dict[int, bool]:
    """Line index -> border flag for every line of a table block (`fenced` lines; a block is a run of non-blank
    lines outside fences), with the tables extension's test: a header of two or more cells (or one, bordered, when
    every row has a border pipe) and a separator of as many cells holding only `|:- `. Tables are tested first; a
    block that is none splits at its first hash heading, and both parts are blocks again."""
    out, i = {}, 0

    def block(i: int, j: int) -> None:
        rows = [line.strip(" ") for line, _ in lines[i:j]]
        if len(rows) > 1:
            border = rows[0].startswith("|") or bool(END_BORDER.search(rows[0]))
            head = cells(rows[0], border)
            if len(head) > 1 or len(head) == 1 and border and all(r.startswith("|") or END_BORDER.search(r) for r in rows[1:]):
                sep = [rows[1][a:b] for a, b in cells(rows[1], border)]
                if len(sep) == len(head) and set("".join(sep)) <= set("|:- "):
                    return out.update(dict.fromkeys(range(i, j), border))
        if (k := next((k for k in range(i, j) if HEADING.match(lines[k][0])), None)) is not None:
            block(i, k)
            block(k + 1, j)
    while i < len(lines):
        j = i
        while j < len(lines) and not lines[j][1] and lines[j][0].strip():
            j += 1
        block(i, j)
        i = max(j, i + 1)
    return out


def fenced(markdown: str):
    """Each line with whether it belongs to a code fence, as `pymdownx.superfences` reads them: three or more
    backticks or tildes, an optional language, then `{attributes}` or only options `pymdownx.highlight` accepts, open
    a fence when a later bare line of exactly that run closes it; an unclosed one is text."""
    lines, fence = markdown.split("\n"), ""
    for i, line in enumerate(lines):
        m = None if fence else FENCE.match(line)
        options = {o[1]: o[3] if o[2] else o[1] for o in OPTION.finditer(m[2] or "")} if m else {}  # the last value counts
        if m and all(k in HIGHLIGHT and re.match(HIGHLIGHT[k], v) for k, v in options.items()) \
                and any(l.strip() == m[1] for l in lines[i + 1:]):
            fence = m[1]
        elif fence and line.strip() == fence:
            fence = ""
            yield line, True
            continue
        yield line, bool(fence)


def heading_ids(markdown: str) -> dict[int, str]:
    """Line index -> id of each heading outside code fences and tables, made unique as Python-Markdown's toc does
    (`x`, `x_1`)."""
    out, lines = {}, list(fenced(markdown))
    rows = table_rows(lines)
    for i, (line, code) in enumerate(lines):
        if not code and i not in rows and (h := HEADING.match(line)):
            a = slug(h[2])
            while a in out.values() or not a:
                m = re.fullmatch(r"(.*)_([0-9]+)", a)
                a = f"{m[1]}_{int(m[2]) + 1}" if m else f"{a}_1"
            out[i] = a
    return out


def anchors(markdown: str) -> set[str]:
    return set(heading_ids(markdown).values())


def sections(markdown: str) -> dict[str, tuple[int, str]]:
    """Level-2 section title -> (line index of its heading, the heading line through its body, trailing blank lines
    dropped), as written."""
    lines, out, title = markdown.split("\n"), {}, None
    for i, (line, code) in enumerate(fenced(markdown)):
        if not code and line.startswith("## "):
            title = line[3:].strip()
            out[title] = [i, i + 1]
        elif title:
            out[title][1] = i + 1
    return {t: (a, "\n".join(lines[a:b]).rstrip("\n")) for t, (a, b) in out.items()}


def trimmed(text: str, title: bool) -> tuple[str, int]:
    """Copied document text without leading blank lines (and its `# ` title when `title`), with the number of lines
    removed in front."""
    m = re.match(r"\n*(?:# .*(?:\n\n*|$))?" if title else r"\n*", text)
    return text[m.end():].rstrip("\n"), text[:m.end()].count("\n")


def complexity(code: str) -> list[tuple[str, str]]:
    """(declaration, comment) for each `// T:` or `// S:` complexity line (03-cpp.md) of a header; the declaration is
    the code after it up to the first `{` or `;` outside parentheses."""
    lines, out = code.split("\n"), []
    for i, line in enumerate(lines):
        if m := re.match(r"\s*// ([TS]: .*)$", line):
            text, depth, decl = " ".join(l.strip() for l in lines[i + 1:i + 12] if not l.strip().startswith("//")), 0, ""
            for c in text:
                if not depth and c in "{;":
                    break
                depth += (c == "(") - (c == ")")
                decl += c
            out.append((" ".join(decl.split()).rstrip(" :="), m[1]))
    return out


def items(operations: str) -> list[str]:
    """An Operations cell split at its top-level `;`, outside brackets (a half-open `[l, r)` opens and closes one)."""
    out, depth, start = [], 0, 0
    for i, c in enumerate(operations + ";"):
        depth += (c in "([") - (c in ")]")
        if c == ";" and not depth:
            out.append(operations[start:i].strip()); start = i + 1
    return [x for x in out if x]


def page_name(target: str) -> str:
    """`02-Data Structures/01-dsu.hpp` -> `01-dsu`; `08-Python/_01_io.py` -> `01-io` (the evidence document stem)."""
    return re.sub(r"^_?(\d{2})[-_](.*)\.(hpp|py)$", r"\1-\2", target.split("/")[1])


def folder_slug(folder: str) -> str:
    return folder.lower().replace(" ", "-")


def git(root: Path, *args: str) -> str | None:
    """Output of a git command in `root`, or None when git or the checkout is missing."""
    try:
        run = subprocess.run(["git", "-C", str(root), *args], capture_output=True, text=True)
    except OSError:
        return None
    return run.stdout if run.returncode == 0 else None


def repository(root: Path) -> str | None:
    """https URL of the `origin` remote without credentials, or None (links to files without a page then keep only
    their text)."""
    url = (git(root, "config", "--get", "remote.origin.url") or "").strip()
    url = re.sub(r"^(?:(?:[^@/:]+@)?([^@/:]+):(?!//)|ssh://(?:[^@/]+@)?([^:/]+)(?::\d+)?/)", lambda m: f"https://{m[1] or m[2]}/", url)
    url = re.sub(r"^https://[^/@]+@", "https://", url).rstrip("/").removesuffix(".git")
    return url if url.startswith("https://") else None


def changed(root: Path) -> dict[str, str]:
    """Repository path -> date of the last commit that touched it (empty outside a git checkout)."""
    out = {}
    for entry in (git(root, "log", "--format=%x00%cs", "--name-only", "--", "0[1-8]-*") or "").split("\0")[1:]:
        date, *paths = entry.strip("\n").split("\n")
        for p in paths:
            if p:
                out.setdefault(p, date)
    return out


def tracked(root: Path) -> set[str] | None:
    """Tracked files and their directories, or None outside a git checkout (then every existing path counts)."""
    files = git(root, "ls-files", "-z")
    if files is None:
        return None
    paths = set(files.split("\0")) - {""}
    return paths | {p[:m.start()] for p in paths for m in re.finditer("/", p)}


class Site:
    def __init__(self, root: Path = ROOT, repo: str | None = None):
        self.root, self.repo, self.graph = root, repo, DEPS.headers(root)
        self.rows = PLAN.inventories(root)
        self.folders = list(dict.fromkeys(t.split("/")[0] for t in self.rows))
        self.pages = {t: f"{folder_slug(t.split('/')[0])}/{page_name(t)}.md" for t in self.rows if (root / t).is_file()}
        self.targets = {**self.pages, **{f"{t.split('/')[0]}/Docs/{page_name(t)}.md": p for t, p in self.pages.items()},
                        **{f"{ARTICLES}/{t.split('/')[0]}/{page_name(t)}.md": p for t, p in self.pages.items()}}
        for f in self.folders:
            self.targets.update({f: f"{folder_slug(f)}/index.md", f"{f}/00-index.md": f"{folder_slug(f)}/index.md",
                                 f"{ARTICLES}/{f}/00-index.md": f"{folder_slug(f)}/index.md"})
            if (root / f / "Docs/00-notes.md").is_file():
                self.targets[f"{f}/Docs/00-notes.md"] = f"{folder_slug(f)}/notes.md"
        articles = self.root / ARTICLES
        self.assets = {a.relative_to(root).as_posix(): f"{folder_slug(a.relative_to(articles).parts[0])}/"
                       + "/".join(a.relative_to(articles).parts[1:]) for a in articles.rglob("*")
                       if a.is_file() and a.suffix != ".md" and a.relative_to(articles).parts[0] in self.folders}
        self.targets.update(self.assets)
        self.tracked, self.anchors = tracked(root), {}
        self.ids: dict[str, dict[str, dict[str, str]]] = {}  # page -> copied document -> its own id -> page id

    def known(self, path: str) -> bool:
        """Whether a repository path is in the checkout a clone gets (tracked; any existing path outside git)."""
        return not path.startswith(("..", "/")) and (
            path in self.tracked if self.tracked is not None else (self.root / path).exists())

    def status(self, target: str) -> str:
        return PLAN.row_status(self.rows[target][0])

    def href(self, target: str, page: str, anchor: str = "", image: bool = False) -> str | None:
        """Link from `page` to a repository path: its site page or asset when it has one with that anchor, else
        GitHub (the raw file for an image)."""
        site = self.targets.get(target)
        own = self.ids.get(site, {}).get(target)  # a document copied onto the page: its ids map to the page's
        if not anchor:
            local = ""
        else:
            local = own.get(anchor) if own is not None else anchor if anchor in self.anchors.get(site, ()) else None
        if site and local is not None:
            return posixpath.relpath(site, posixpath.dirname(page)) + (f"#{local}" if local else "")
        if self.repo and self.known(target):
            kind = "tree" if (self.root / target).is_dir() else "blob"
            return f"{self.repo}/{kind}/{BRANCH}/{quote(target)}" + ("?raw=true" if image else "") + (
                f"#{anchor}" if anchor else "")
        return None

    def link(self, label: str, target: str, page: str) -> str:
        url = self.href(target, page)
        return f"[{label}]({url})" if url else label

    def relink(self, text: str, source: str, page: str) -> str:
        """Rewrite the local links of copied Markdown, written relative to repository file `source`, for `page`."""
        def one(label: str, raw: str, bang: str) -> str:
            url = raw.strip("<>")
            if "://" in url or url.startswith("mailto:"):
                return f"{bang}[{label}]({raw})"
            path, _, anchor = unquote(url).partition("#")
            target = posixpath.normpath(posixpath.join(posixpath.dirname(source), path)) if path else source
            new = self.href(target, page, anchor, bool(bang))
            return f"{bang}[{label}]({new})" if new else label
        lines, out = list(fenced(text)), []
        rows = table_rows(lines)
        for i, (line, code) in enumerate(lines):
            parts, last = [], 0
            for a, b in [] if code else cells(line, rows[i]) if i in rows else [(0, len(line))]:
                masked = line[a:b]  # code spans are blanked for matching, so links inside them stay literal
                for x, y, _ in spans(masked):
                    masked = masked[:x] + "\x02" * (y - x) + masked[y:]
                masked = Heading.MATH.sub(lambda e: "\x02" * len(e[0]) if e[2] or e[5] else e[0], masked)  # math is atomic
                masked = re.sub(r"\\.", lambda e: "\x02\x02" if e[0][1] in ESCAPED + "$" else e[0], masked)  # `\[` opens no link
                for m in LINK.finditer(masked):
                    bang = "!" if m.start() and masked[m.start() - 1] == "!" else ""  # an image
                    parts += [line[last:a + m.start() - len(bang)], one(line[a + m.start(1):a + m.end(1)], line[a + m.start(2):a + m.end(2)], bang)]
                    last = a + m.end()
            out.append("".join(parts) + line[last:])
        return "\n".join(out)

    def table(self, folder: str, targets: list[str]) -> list[str]:
        """Inventory rows as written in `00-index.md`, the file cell made a link; relinked with the other copied text."""
        head = ["| Header | Basic | Operations | Status |", "|---|---|---|---|"] if folder == "01-Core" else [
            f"| {'Module' if targets[0].endswith('.py') else 'Header'} | Operations | Status |", "|---|---|---|"]
        return head + ["| " + " | ".join([f"[`{t.split('/')[1]}`](<{t.split('/')[1]}>)", *self.rows[t][0]]) + " |"
                       for t in targets]

    def document(self, path: str, title: bool) -> list[Fragment]:
        """A copied article or notes document (without its `# ` title when `title`), as one fragment if it exists."""
        if not (self.root / path).is_file():
            return []
        text, offset = trimmed((self.root / path).read_text(encoding="utf-8"), title)
        return [(text, path, offset)] if text else []

    def overview(self, folder: str) -> list[Fragment]:
        index = (self.root / folder / "00-index.md").read_text(encoding="utf-8")
        scope = next((l for l in index.split("\n") if l.startswith("Scope:")), "")
        out = [(f"# {folder.replace('-', ' ', 1)}", None, None), (scope, f"{folder}/00-index.md", None)]
        if f"{folder}/Docs/00-notes.md" in self.targets:
            out.append(("[Folder notes](notes.md): contracts and ownership rules shared by the folder's headers.", None, None))
        out += self.document(f"{ARTICLES}/{folder}/00-index.md", True)
        if any(h.startswith(folder + "/") for h in self.graph):
            chart = DEPS.chart(self.graph, folder)
            out.append(("## Include chart\n\n" + (f"Arrows point from a header to the headers it includes; the template "
                        f"is left out.\n\n```mermaid\n{chart}\n```" if "-->" in chart else
                        "Every header of this folder includes only the template."), None, None))
        tiers = {}
        for t, (_, tier) in self.rows.items():
            if t.startswith(folder + "/"):
                tiers.setdefault(tier, []).append(t)
        lines = index.split("\n")
        for tier, targets in tiers.items():  # the tier heading's line, so its id maps like a copied document's
            out.append(("\n".join(([] if tier == "Core" else [f"## {tier}", ""]) + self.table(folder, targets)),
                        f"{folder}/00-index.md", None if tier == "Core" else lines.index(f"## {tier}")))
        return out

    def header(self, target: str, dates: dict[str, str]) -> list[Fragment]:
        folder, file = target.split("/")
        page, name, (cells, tier) = self.pages[target], page_name(target), self.rows[target]
        evidence = f"{folder}/Docs/{name}.md"
        code = (self.root / target).read_text(encoding="utf-8").rstrip("\n")
        lines = DEPS.lines(self.root, target)
        facts = [self.link(folder.replace("-", " ", 1), folder, page), tier] + (
            [f"Basic: {cells[0]}"] if folder == "01-Core" else []) + [self.status(target), f"{lines} lines"] + (
            [f"last commit {dates[target]}"] if target in dates else [])
        github = [f"[{label}]({self.repo}/blob/{BRANCH}/{quote(p)})" for label, p in (("source", target), ("evidence", evidence))
                  if self.repo and self.known(p)]
        out = [(f"# {file}", None, None), (" · ".join(facts + github), None, None)]
        if file.endswith(".hpp"):
            out.append((f'```cpp\n#include "{target}"\n```', None, None))
        out += self.document(f"{ARTICLES}/{folder}/{name}.md", True)
        out.append(("## Operations\n\n" + "\n".join(f"- {i}" for i in items(cells[-2])), f"{folder}/00-index.md", None))
        if bounds := complexity(code) if file.endswith(".hpp") else []:
            out.append(("## Complexity\n\n" + "\n".join(f"- {literal(d)}: {literal(c)}" for d, c in bounds), None, None))
        found = sections((self.root / evidence).read_text(encoding="utf-8")) if (self.root / evidence).is_file() else {}
        for title in SECTIONS:
            if title == "Limits and handoffs" and target in self.graph:
                out.append((self.dependencies(target, page), None, None))
            if title in found and found[title][1] != f"## {title}":
                out.append((found[title][1], evidence, found[title][0]))
        tests = [t for t in PLAN.tests_for(self.root, target) if self.known(t)]
        if tests:
            out.append(("## Tests\n\n" + "\n".join(f"- {self.link(f'`{t}`', t, page)}" for t in tests), None, None))
        fence = "`" * max([3] + [len(r) + 1 for r in re.findall(r"`{3,}", code)])
        body = "\n".join(f"    {l}" if l.strip() else "" for l in
                         [fence + ("cpp" if file.endswith(".hpp") else "python"), *code.split("\n"), fence])
        out.append((f'## Source\n\n??? abstract "{file}, {lines} lines"\n\n{body}', None, None))
        return out

    def dependencies(self, target: str, page: str) -> str:
        plan = DEPS.plan(self.root, self.graph, target)
        lines = lambda h: DEPS.lines(self.root, h)
        name = lambda h, *more: self.link(f"`{h}`", h, page) + (
            f" ({', '.join(([self.status(h)] if h in self.rows else []) + list(more))})" if h in self.rows or more else "")
        direct = self.graph[target]
        out = ["## Dependencies", "", "Includes: " + (", ".join(name(h) for h in direct) if direct else "nothing") + ".", "",
               f"Typing order (each file after the files it includes): {len(plan['order'])} files, "
               f"{sum(map(lines, plan['order']))} lines.", "", "| Lines | File |", "|---|---|"]
        out += [f"| {lines(h)} | {name(h)} |" for h in plan["order"]]
        if plan["optional"]:
            out += ["", "Skippable when typing only the `*Compact` twins and code that needs no Core reduction: "
                    + ", ".join(name(h, f"{lines(h)} lines") for h in plan["optional"]) + "."]
        out += [f"\nA mini exists but is not tested with this header: {name(m)} for {name(f)} ({n} lines in total with it)."
                for f, (m, n) in plan["minis"].items()]
        users = [h for h, ds in self.graph.items() if target in ds and not re.search(r"/9[89]-", h)]
        out += ["", "Included by: " + (", ".join(name(h) for h in users) if users else "no header") + "."]
        return "\n".join(out)

    def home(self) -> list[Fragment]:
        rows = ["| Folder | Rows | Pages | Verified |", "|---|---|---|---|"]
        for f in self.folders:
            targets = [t for t in self.rows if t.startswith(f + "/")]
            rows.append(f"| [{f.replace('-', ' ', 1)}]({folder_slug(f)}/index.md) | {len(targets)} | "
                        f"{sum(t in self.pages for t in targets)} | {sum(self.status(t) == 'verified' for t in targets)} |")
        return [("# CompProg Library", None, None),
                ("Generated from the library's inventories, evidence documents, include graph and sources by "
                 "`98-Team Notebook/06-site.py`. Each header page lists its operations, complexity, contracts, "
                 "dependencies in typing order, tests and source. Compile a header from the repository root with "
                 "`g++ -std=gnu++20 -I.`, or type its typing order from the top.", None, None), ("\n".join(rows), None, None)]

    def map_ids(self, page: str, parts: list[Fragment]) -> None:
        """Record the page's heading ids and, for each copied document, which page id each of its own ids became."""
        ids, start = heading_ids("\n\n".join(text for text, _, _ in parts)), 0
        self.anchors[page], self.ids[page] = set(ids.values()), {}
        for text, src, offset in parts:
            if offset is not None:
                own = heading_ids((self.root / src).read_text(encoding="utf-8"))
                mapped = self.ids[page].setdefault(src, {})
                for k in range(text.count("\n") + 1):
                    if start + k in ids and offset + k in own:
                        mapped[own[offset + k]] = ids[start + k]
            start += text.count("\n") + 2

    def files(self) -> dict[str, str]:
        """Site path (relative to the site directory) -> content of every generated text file; the article assets
        to copy are `self.assets`."""
        known = {f"{ARTICLES}/{t.split('/')[0]}/{page_name(t)}.md" for t in self.pages} | set(self.assets) | {
            f"{ARTICLES}/{f}/00-index.md" for f in self.folders}
        orphans = sorted(a for p in (self.root / ARTICLES).rglob("*") if p.is_file()
                         and (a := p.relative_to(self.root).as_posix()) not in known)
        if orphans:
            raise SiteError("Article or asset without an existing inventory header, module or folder: " + ", ".join(orphans))
        dates = changed(self.root)
        raw = {"index.md": self.home()}
        for f in self.folders:
            raw[f"{folder_slug(f)}/index.md"] = self.overview(f)
            if f"{f}/Docs/00-notes.md" in self.targets:
                raw[f"{folder_slug(f)}/notes.md"] = self.document(f"{f}/Docs/00-notes.md", False)
        raw.update({p: self.header(t, dates) for t, p in self.pages.items()})
        for p, parts in raw.items():
            self.map_ids(p, parts)
        for t, p in self.pages.items():  # a document with no heading copied still owns its ids: none are here
            self.ids[p].setdefault(f"{t.split('/')[0]}/Docs/{page_name(t)}.md", {})
        for f in self.folders:
            self.ids[f"{folder_slug(f)}/index.md"].setdefault(f"{f}/00-index.md", {})
        out = {f"docs/{p}": "\n\n".join(self.relink(text, src, p) if src else text for text, src, _ in parts) + "\n"
               for p, parts in raw.items()}
        nav = ['  - Home: index.md']
        for f in self.folders:
            nav += [f"  - {json.dumps(f.replace('-', ' ', 1))}:", f"    - {folder_slug(f)}/index.md"]
            if f"{f}/Docs/00-notes.md" in self.targets:
                nav.append(f"    - Notes: {folder_slug(f)}/notes.md")
            tiers = {}
            for t, (_, tier) in self.rows.items():
                if t in self.pages and t.startswith(f + "/"):
                    tiers.setdefault(tier, []).append(f"{json.dumps(t.split('/')[1])}: {self.pages[t]}")
            for tier, items in tiers.items():
                nav += [f"    - {i}" for i in items] if tier == "Core" else [f"    - {tier}:"] + [f"      - {i}" for i in items]
        repo = f"repo_url: {json.dumps(self.repo)}\n" if self.repo else ""
        out["mkdocs.yml"] = CONFIG.format(repo=repo, nav="\n".join(nav))
        out["docs/javascripts/mathjax.js"] = MATHJAX
        return out


def generate(root: Path = ROOT, out: Path | None = None) -> int:
    out = out or SITE
    site = Site(root, repository(root))
    files = site.files()
    shutil.rmtree(out / "docs", ignore_errors=True)
    for name, text in files.items():
        (out / name).parent.mkdir(parents=True, exist_ok=True)
        (out / name).write_text(text, encoding="utf-8")
    for asset, name in site.assets.items():
        (out / "docs" / name).parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(root / asset, out / "docs" / name)
    return sum(name.endswith(".md") for name in files)


ENGINES = {"zensical": {"zensical": "zensical"},  # executable -> {module its Python needs: package to install}
           "mkdocs": {"material": "mkdocs-material", "pymdownx": "pymdown-extensions"}}
INSTALL = ("install Zensical (pipx install zensical) or MkDocs Material (pacman -S mkdocs-material "
           "python-pymdown-extensions, or pipx install mkdocs-material --include-deps)")


def engine(name: str = "auto") -> tuple[str, str] | str:
    """(engine, executable) for the first usable engine of `name` (`auto`: Zensical, then MkDocs), or why none is:
    not found, or its Python (from the script's `#!` line) lacks a module the site needs."""
    reasons = []
    for e in ENGINES if name == "auto" else [name]:
        if not (exe := shutil.which(e)):
            reasons.append(f"{e} not found")
            continue
        try:
            head = Path(exe).read_bytes()[:256].split(b"\n")[0].decode(errors="replace")
            py = head[2:].split() if head.startswith("#!") and "python" in head else None
            lacks = [pkg for mod, pkg in ENGINES[e].items()
                     if py and subprocess.run([*py, "-c", f"import {mod}"], capture_output=True).returncode]
        except OSError as err:
            reasons.append(f"{e} at {exe} cannot run its Python ({err.strerror})")
            continue
        if not lacks:
            return e, exe
        reasons.append(f"{e} at {exe} lacks {', '.join(lacks)} (install them for its Python)")
    return "; ".join(reasons) + f"; {INSTALL}"


def run(name: str, command: str, *args: str) -> int:
    found = engine(name)
    if isinstance(found, str):
        print(found, file=sys.stderr)
        return 1
    print(f"Using {found[0]} ({found[1]})", flush=True)
    # The privacy plugin caches downloads under the working directory.
    return subprocess.run([found[1], command, "-f", str(SITE / "mkdocs.yml"), *args], cwd=SITE).returncode


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("generate", help="write site/mkdocs.yml and site/docs/")
    for name, text in (("build", "generate, then build site/html/ strictly"), ("serve", "generate, then serve on 127.0.0.1")):
        cmd = sub.add_parser(name, help=f"{text} with Zensical, or MkDocs when Zensical is missing")
        cmd.add_argument("--engine", choices=["auto", *ENGINES], default="auto")
        if name == "serve":
            cmd.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()
    try:
        pages = generate()
    except SiteError as e:
        print(e, file=sys.stderr)
        return 1
    print(f"Generated {pages} pages in {SITE}", flush=True)
    if args.command == "build":
        return run(args.engine, "build", "--strict")
    if args.command == "serve":
        return run(args.engine, "serve", "--dev-addr", f"127.0.0.1:{args.port}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
