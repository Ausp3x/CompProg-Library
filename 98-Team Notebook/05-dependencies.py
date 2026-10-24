#!/usr/bin/env python3
"""Include graph of the 01-07 C++ headers: typing-order closures with line counts and per-folder Mermaid charts."""

from __future__ import annotations

import argparse
import posixpath
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TEMPLATE = "01-Core/01-template.hpp"
REDUCTION = ("01-Core/03-barrett.hpp", "01-Core/04-montgomery.hpp")
INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*"([^"\n]+)"')
CONDITION = re.compile(r"^[ \t]*#[ \t]*(if|ifdef|ifndef|endif)\b")
TWIN = re.compile(r"\b(?:inline|struct)\b[^;{]*?\b(\w+Compact)\b[^;{]*\{")  # a Compact twin definition (03-cpp.md, Headers)
# Header -> {full Core type: mini} it is tested with; add a pair only once that header's tester passes with the swap.
# Until then a closure only notes that the mini exists.
TESTED_MINIS: dict[str, dict[str, str]] = {}


def strip_cpp(text: str, directives: bool = False) -> str:
    """Blank comments, string/character literals and, unless `directives`, preprocessor lines; keep every newline and
    column. Shared with `96-Local Testing/03-consistency.py`."""
    out, i, n, line_start = [], 0, len(text), True
    while i < n:
        c = text[i]
        if c == '\n':
            out.append(c); i += 1; line_start = True
            continue
        if line_start and c not in ' \t':
            line_start = False
            if c == '#':  # a directive runs to its newline; its comments are blanked, its literals kept whole
                while i < n and text[i] != '\n':
                    if text.startswith('/*', i):
                        j = text.find('*/', i + 2); j = n if j < 0 else j + 2
                    elif text.startswith('//', i):  # runs past spliced newlines
                        j = text.find('\n', i)
                        while 0 < j and text[j - 1] == '\\':
                            j = text.find('\n', j + 1)
                        j = n if j < 0 else j
                    elif text[i] in '"\'':
                        j = i + 1
                        while j < n and text[j] not in (text[i], '\n'):
                            j += 2 if text[j] == '\\' else 1
                        j = min(n, j + (j < n and text[j] == text[i]))
                        out.append(text[i:j] if directives else ''.join('\n' if ch == '\n' else ' ' for ch in text[i:j])); i = j
                        continue
                    elif text[i] == '\\' and i + 1 < n and text[i + 1] == '\n':
                        out.append('\\\n' if directives else ' \n'); i += 2
                        continue
                    else:
                        out.append(text[i] if directives else ' '); i += 1
                        continue
                    out.append(''.join('\n' if ch == '\n' else ' ' for ch in text[i:j])); i = j
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


def includes(text: str) -> list[tuple[str, bool]]:
    """Quoted includes outside comments in source order, each with whether it sits inside an `#if` block."""
    out, depth = [], 0
    for line in strip_cpp(text, directives=True).split("\n"):
        cond, inc = CONDITION.match(line), INCLUDE.match(line)
        depth += (cond[1] != "endif") - (cond[1] == "endif") if cond else 0
        if inc:
            out.append((inc[1], depth > 0))
    return out


def uses_reduction(text: str) -> bool:
    """Whether a header includes or names Core Barrett or Montgomery (the Compact-twin rule, 03-cpp.md, Headers)."""
    code = strip_cpp(text, directives=True)
    return bool(re.search(r'^[ \t]*#[ \t]*include[ \t]*"[^"\n]*0[34]-(barrett|montgomery)\.hpp"', code, re.M)
                or re.search(r"\b(Montgomery|Barrett)\w*", code))


def scan(root: Path = ROOT) -> dict[str, list[tuple[str, bool]]]:
    """Repository-relative header -> its `includes`, as repository-relative paths, for 01-07 outside 97-Legacy."""
    out = {}
    for path in sorted(root.glob("0[1-7]-*/**/*.hpp")):
        rel = path.relative_to(root)
        if "97-Legacy" not in rel.parts:
            out[rel.as_posix()] = [(posixpath.normpath((rel.parent / inc).as_posix()), cond)
                                   for inc, cond in includes(path.read_text(encoding="utf-8"))]
    return out


def headers(root: Path = ROOT, found: dict[str, list[tuple[str, bool]]] | None = None) -> dict[str, list[str]]:
    """Header -> its quoted includes in source order (from `found`, a `scan`, when given)."""
    return {h: [inc for inc, _ in incs] for h, incs in (found or scan(root)).items()}


def lines(root: Path, header: str) -> int:
    data = (root / header).read_bytes()
    return data.count(b"\n") + (data[-1:] not in (b"", b"\n"))


def compact(root: Path, header: str) -> bool:
    text = (root / header).read_text(encoding="utf-8")
    return not header.startswith("01-Core/") and uses_reduction(text) and bool(TWIN.search(strip_cpp(text)))


def minis(graph: dict[str, list[str]]) -> dict[str, str]:
    """Full Core type -> its existing mini (`NN-stem.hpp` -> `MM-stemmini.hpp`)."""
    stems = {re.sub(r"^\d{2}-|\.hpp$", "", h.split("/")[1]): h for h in graph if h.startswith("01-Core/")}
    return {stems[s[:-4]]: h for s, h in stems.items() if s.endswith("mini") and s[:-4] in stems}


def closure(graph: dict[str, list[str]], header: str, skip: set[tuple[str, str]] = frozenset(),
            swap: dict[str, str] | None = None) -> list[str]:
    """Header and its transitive includes, each after its own includes (typing order), skipping `skip` edges and
    replacing each key of `swap` by its value."""
    swap, order, seen = swap or {}, [], set()

    def visit(h: str) -> None:
        h = swap.get(h, h)
        if h not in seen:
            seen.add(h)
            for d in graph.get(h, []):
                if (h, d) not in skip:
                    visit(d)
            order.append(h)
    visit(header)
    return order


def plan(root: Path, graph: dict[str, list[str]], header: str) -> dict:
    """Preferred typing list for one header: tested minis swapped in, Compact-only reduction includes skipped, and the
    untested minis that would shorten it."""
    swap = TESTED_MINIS.get(header, {})
    skip = {(header, d) for d in graph[header] if d in REDUCTION} if compact(root, header) else set()
    full, order = closure(graph, header, swap=swap), closure(graph, header, skip, swap)
    size = lambda hs: sum(lines(root, h) for h in hs)
    untested = {f: (m, size(closure(graph, header, skip, {**swap, f: m}))) for f, m in minis(graph).items()
                if f in order and f != header and m not in order}
    return {"order": order, "optional": [h for h in full if h not in order],
            "minis": {f: x for f, x in untested.items() if x[1] < size(order)}}


def describe(root: Path, graph: dict[str, list[str]], header: str) -> str:
    p = plan(root, graph, header)
    total = sum(lines(root, h) for h in p["order"])
    out = [f"{header}: {len(p['order'])} files, {total} lines"] + [f"{lines(root, h):6}  {h}" for h in p["order"]]
    if p["optional"]:
        out.append("Skippable when typing only the *Compact twins and code that needs no Core reduction: "
                   + ", ".join(f"{h} ({lines(root, h)} lines)" for h in p["optional"])
                   + f"; {total + sum(lines(root, h) for h in p['optional'])} lines in total with them")
    for full, (mini, swapped) in p["minis"].items():
        out.append(f"Mini exists, not tested with this header: {mini} for {full} ({swapped} lines in total with it)")
    return "\n".join(out)


def chart(graph: dict[str, list[str]], folder: str) -> str:
    """Mermaid flowchart of a folder's headers (arrow: includes), without the template edge and aggregates."""
    own = [h for h in graph if h.startswith(folder + "/") and not re.search(r"/9[89]-", h)]
    edges = [(h, d) for h in own for d in graph[h] if d != TEMPLATE]
    nodes = list(dict.fromkeys(x for e in edges for x in e))
    ids = {h: f"n{i}" for i, h in enumerate(nodes)}
    out = ["flowchart LR"]
    for group in dict.fromkeys(h.split("/")[0] for h in nodes):
        out += [f'  subgraph g{group[:2]}["{group}"]'] + [f'    {ids[h]}["{h.split("/", 1)[1]}"]' for h in nodes
                                                         if h.split("/")[0] == group] + ["  end"]
    out += [f"  {ids[h]} --> {ids[d]}" for h, d in edges]
    alone = len(own) - sum(h in ids for h in own)
    return "\n".join(out + [f"%% {alone} other headers in {folder} include only the template"])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("closure", help="typing order with line counts; every header when none is given").add_argument("header", nargs="*")
    sub.add_parser("chart", help="Mermaid include chart; every 01-07 folder when none is given").add_argument("folder", nargs="*")
    args = parser.parse_args()
    graph = headers()
    if args.command == "closure":
        bad = [h for h in args.header if h not in graph]
        if bad:
            print("Unknown header (use a repository-relative path such as 02-Data Structures/01-dsu.hpp): " + ", ".join(bad), file=sys.stderr)
            return 1
        print("\n\n".join(describe(ROOT, graph, h) for h in args.header or graph))
    else:
        folders = args.folder or sorted(dict.fromkeys(h.split("/")[0] for h in graph))
        bad = [f for f in folders if not any(h.startswith(f + "/") for h in graph)]
        if bad:
            print("Unknown folder: " + ", ".join(bad), file=sys.stderr)
            return 1
        print("\n\n".join(f"%% {f}\n" + chart(graph, f) for f in folders))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
