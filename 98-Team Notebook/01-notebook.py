#!/usr/bin/env python3
"""Build a selectable LuaLaTeX notebook from this project's canonical sources."""

from __future__ import annotations

import argparse
import html
import json
import os
import re
import secrets
import shutil
import subprocess
import sys
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.resolve()
CONFIG = HERE / "02-selection.json"
TEX = HERE / "04-notebook.tex"
NOTES = HERE / "03-notes.md"
SUFFIXES = {".h", ".hpp", ".hh", ".cpp", ".cc", ".cxx", ".py"}
DEFAULT = {
    "version": 1,
    "title": "Team Notebook",
    "selected": ["01-Core/01-template.hpp", "01-Core/02-debug.hpp"],
    "include_notes": True,
    "max_pages": 25,
    "paper": "a4",
    "columns": 2,
    "body_size": 8.5,
    "code_size": 6.8,
    "body_font": "Inter",
    "code_font": "JetBrains Mono",
    "print_mode": False,
}
PAPERS = {"a4": "a4paper", "letter": "letterpaper", "b5": "b5paper"}
SAFE_PART = re.compile(r"^[^\\{}%#\r\n\x00]+$")
SKIP_DIRS = {"old", "97-legacy", "build", "__pycache__"}


class NotebookError(ValueError):
    pass


def sources() -> dict[str, Path]:
    """Only regular C++/Python files in numbered canonical source folders."""
    found: dict[str, Path] = {}
    for category in sorted(ROOT.iterdir()):
        if not category.is_dir() or category.is_symlink() or not re.match(r"^0[1-8]-", category.name):
            continue
        for current, dirs, files in os.walk(category, followlinks=False):
            base = Path(current)
            dirs[:] = sorted(d for d in dirs if d.casefold() not in SKIP_DIRS and not (base / d).is_symlink())
            for filename in sorted(files):
                path = base / filename
                if not path.is_file() or path.is_symlink() or path.suffix.lower() not in SUFFIXES:
                    continue
                rel = path.relative_to(ROOT)
                if not all(SAFE_PART.fullmatch(part) and part not in (".", "..") for part in rel.parts):
                    continue
                if not path.resolve().is_relative_to(ROOT):
                    continue
                found[rel.as_posix()] = path
    return found


def validate(raw: object, available: dict[str, Path]) -> dict:
    if not isinstance(raw, dict) or set(raw) != set(DEFAULT) or raw.get("version") != 1:
        raise NotebookError("Config must be a version 1 object with the documented keys")
    cfg = dict(raw)
    if not isinstance(cfg["selected"], list) or not all(isinstance(s, str) for s in cfg["selected"]):
        raise NotebookError("selected must be a list of source IDs")
    if len(cfg["selected"]) != len(set(cfg["selected"])):
        raise NotebookError("selected contains duplicate source IDs")
    unknown = [s for s in cfg["selected"] if s not in available]
    if unknown:
        raise NotebookError("Unknown or unsafe source ID: " + ", ".join(unknown[:3]))
    for key in ("include_notes", "print_mode"):
        if type(cfg[key]) is not bool:
            raise NotebookError(f"{key} must be true or false")
    if type(cfg["max_pages"]) is not int or not 1 <= cfg["max_pages"] <= 1000:
        raise NotebookError("max_pages must be an integer from 1 to 1000")
    if not isinstance(cfg["paper"], str) or cfg["paper"] not in PAPERS or type(cfg["columns"]) is not int or cfg["columns"] not in (1, 2, 3):
        raise NotebookError("paper must be a4, letter or b5; columns must be 1, 2 or 3")
    for key in ("body_size", "code_size"):
        value = cfg[key]
        if type(value) not in (int, float) or not 4 <= value <= 20:
            raise NotebookError(f"{key} must be a number from 4 to 20 points")
    for key in ("body_font", "code_font"):
        value = cfg[key]
        if not isinstance(value, str) or not 1 <= len(value) <= 100 or not re.fullmatch(r"[\w .,+-]+", value, re.UNICODE):
            raise NotebookError(f"{key} contains unsupported characters")
    if (not isinstance(cfg["title"], str) or not 1 <= len(cfg["title"]) <= 120
            or any(ord(c) < 32 for c in cfg["title"])):
        raise NotebookError("title must contain 1 to 120 printable characters")
    return cfg


def load(available: dict[str, Path]) -> dict:
    try:
        raw = json.loads(CONFIG.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        raise NotebookError(f"Cannot read {CONFIG.name}: {exc}") from exc
    return validate(raw, available)


def save(cfg: dict, available: dict[str, Path]) -> None:
    validate(cfg, available)
    temp = CONFIG.with_suffix(".json.tmp")
    temp.write_text(json.dumps(cfg, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    temp.replace(CONFIG)


def tex_escape(value: str) -> str:
    table = {"\\": r"\textbackslash{}", "{": r"\{", "}": r"\}", "%": r"\%", "#": r"\#",
             "$": r"\$", "&": r"\&", "_": r"\_", "^": r"\textasciicircum{}", "~": r"\textasciitilde{}"}
    return "".join(table.get(c, c) for c in value)


def notes_tex() -> str:
    """Small Markdown subset for the bundled, editable contest notes."""
    lines = []
    for line in NOTES.read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            lines.append(r"\sourceheading{" + tex_escape(line[2:]) + "}")
        elif line.startswith("## "):
            lines.append(r"\par\smallskip\noindent\textbf{" + tex_escape(line[3:]) + r"}\par")
        elif line.startswith("- "):
            lines.append(r"\noindent\textcolor{accent}{\textbullet}\ " + tex_escape(line[2:]) + r"\par")
        elif line.strip():
            lines.append(tex_escape(line) + r"\par")
        else:
            lines.append(r"\smallskip")
    return "\n".join(lines)


def generate(cfg: dict, available: dict[str, Path]) -> Path:
    validate(cfg, available)
    accent = "333333" if cfg["print_mode"] else "00AFC4"
    faint = "666666" if cfg["print_mode"] else "53717A"
    body = f'{cfg["body_size"]:g}'
    code = f'{cfg["code_size"]:g}'
    columns = cfg["columns"]
    chunks = [
        "% Generated from canonical source files by 01-notebook.py; do not edit this file.\n",
        rf"\documentclass[{PAPERS[cfg['paper']]}]{{article}}" + "\n",
        r"\usepackage[margin=9mm,headheight=13pt,headsep=4mm,footskip=6mm]{geometry}" + "\n",
        r"\usepackage{fontspec,multicol,listings,xcolor,fancyhdr}" + "\n",
        rf"\setmainfont{{{cfg['body_font']}}}" + "\n",
        rf"\setmonofont{{{cfg['code_font']}}}" + "\n",
        rf"\definecolor{{accent}}{{HTML}}{{{accent}}}" + "\n",
        rf"\definecolor{{muted}}{{HTML}}{{{faint}}}" + "\n",
        r"\setlength{\parindent}{0pt}\setlength{\parskip}{2pt}" + "\n",
        r"\setlength{\columnsep}{5mm}" + "\n",
        r"\pagestyle{fancy}\fancyhf{}" + "\n",
        r"\renewcommand{\headrulewidth}{0pt}" + "\n",
        r"\fancyhead[L]{\textcolor{accent}{\rule{2mm}{1.2mm}}\hspace{2mm}\textcolor{muted}{TEAM NOTEBOOK}}" + "\n",
        r"\fancyfoot[R]{\textcolor{muted}{\thepage}}" + "\n",
        r"\newcommand{\sourceheading}[1]{\par\vspace{5pt}\noindent\textcolor{accent}{\rule{1.5mm}{1.5mm}}\hspace{1.5mm}{\bfseries #1}\par\nobreak}" + "\n",
        r"\lstset{" + "\n",
        rf"  basicstyle=\ttfamily\fontsize{{{code}}}{{{float(code)*1.15:g}}}\selectfont," + "\n",
        r"  breaklines=true,breakatwhitespace=false,columns=fullflexible,keepspaces=true," + "\n",
        r"  showstringspaces=false,tabsize=2,numbers=left,numbersep=3pt," + "\n",
        r"  numberstyle=\tiny\color{muted},keywordstyle=\color{accent}," + "\n",
        r"  frame=none,aboveskip=2pt,belowskip=4pt}" + "\n",
        r"\begin{document}" + "\n",
        rf"\fontsize{{{body}}}{{{float(body)*1.18:g}}}\selectfont" + "\n",
        rf"{{\Large\bfseries {tex_escape(cfg['title'])}}}\hfill{{\small\textcolor{{muted}}{{SOURCE INDEX}}}}\par" + "\n",
        r"\textcolor{accent}{\rule{\linewidth}{0.5pt}}\vspace{-5pt}" + "\n",
    ]
    if columns > 1:
        chunks.append(rf"\begin{{multicols}}{{{columns}}}" + "\n")
    if cfg["include_notes"]:
        chunks.extend([notes_tex(), "\n"])
    for item in cfg["selected"]:
        path = available[item]
        # The source path is resolved by TeX at build time, so edits to canonical files appear automatically.
        relative = path.relative_to(ROOT).as_posix()
        title = tex_escape(relative)
        language = "Python" if path.suffix.lower() == ".py" else "C++"
        chunks.append(r"\sourceheading{" + title + "}\n")
        chunks.append(rf"\lstinputlisting[language={language}]{{\detokenize{{../{relative}}}}}" + "\n")
    if columns > 1:
        chunks.extend([r"\end{multicols}", "\n"])
    chunks.extend([r"\end{document}", "\n"])
    TEX.write_text("".join(chunks), encoding="utf-8")
    return TEX


def font_status(font: str) -> bool | None:
    if not shutil.which("fc-list"):
        return None
    result = subprocess.run(["fc-list", "-f", "%{family}\n"], capture_output=True, text=True, check=False)
    if result.returncode:
        return None
    return any(font.casefold() == family.strip().casefold()
               for line in result.stdout.splitlines() for family in line.split(","))


def build(cfg: dict, available: dict[str, Path]) -> tuple[str, int]:
    generate(cfg, available)
    missing = []
    if not shutil.which("lualatex"):
        missing.append("LuaLaTeX executable (lualatex)")
    for key in ("body_font", "code_font"):
        status = font_status(cfg[key])
        if status is False:
            missing.append(f"font {cfg[key]!r} ({key})")
    if missing:
        return "Build prerequisites missing: " + ", ".join(missing) + ". The .tex file was generated.", 1
    result = subprocess.run(["lualatex", "-interaction=nonstopmode", "-halt-on-error", "-file-line-error",
                             TEX.name], cwd=HERE, capture_output=True, text=True, check=False, timeout=180)
    output = result.stdout + "\n" + result.stderr
    if result.returncode:
        tail = "\n".join(output.splitlines()[-20:])
        return "LuaLaTeX failed. Check 04-notebook.log.\n" + tail, 1
    match = re.search(r"Output written on .*?\((\d+) pages?,", output)
    if not match:
        return "PDF created, but page count could not be read from LuaLaTeX output.", 1
    pages = int(match.group(1))
    excess = max(0, pages - cfg["max_pages"])
    message = f"04-notebook.pdf: {pages} page(s); budget {cfg['max_pages']}."
    if excess:
        message += f" Over budget by {excess} page(s); all selected sources remain included."
    return message, 2 if excess else 0


def form_selection(values: dict[str, list[str]]) -> list[str]:
    selected = values.get("selected", [])
    if "order_enabled" not in values:
        return selected
    ordered = values.get("ordered_selected", [])
    if len(ordered) != len(selected) or set(ordered) != set(selected):
        raise NotebookError("Selection order does not match checked sources")
    return ordered


def page(cfg: dict, available: dict[str, Path], token: str, message: str = "") -> str:
    esc = html.escape
    groups: dict[str, list[str]] = {}
    for item in available:
        group = item.split("/", 1)[0]
        groups.setdefault(group, []).append(item)
    source_html = []
    for group, items in groups.items():
        source_html.append(f"<details open><summary>{esc(group)} <small>{len(items)} files</small></summary>")
        for item in items:
            checked = " checked" if item in cfg["selected"] else ""
            source_html.append(f'<label class="file"><input type="checkbox" name="selected" value="{esc(item, quote=True)}"{checked}><span>{esc(item.split("/", 1)[1])}</span></label>')
        source_html.append("</details>")
    ordered_html = "".join(
        f'<li data-id="{esc(item, quote=True)}"><span>{esc(item)}</span><div>'
        '<button type="button" data-move="up" aria-label="Move up">↑</button>'
        '<button type="button" data-move="down" aria-label="Move down">↓</button></div></li>'
        for item in cfg["selected"]
    )
    def field(name: str, label: str, kind: str = "text", extra: str = "") -> str:
        return f'<label class="field">{label}<input name="{name}" type="{kind}" value="{esc(str(cfg[name]), quote=True)}" {extra}></label>'
    def check(name: str, label: str) -> str:
        checked = " checked" if cfg[name] else ""
        return f'<label class="toggle"><input name="{name}" type="checkbox"{checked}> {label}</label>'
    def select(name: str, choices: list[str]) -> str:
        options = "".join(f'<option value="{esc(x)}"{" selected" if str(cfg[name]) == x else ""}>{esc(x)}</option>' for x in choices)
        return f'<label class="field">{name.replace("_", " ").title()}<select name="{name}">{options}</select></label>'
    notice = f'<div class="notice">{esc(message)}</div>' if message else ""
    script = r'''const form = document.querySelector('form');
const ordered = document.getElementById('selected-order');
const checks = Array.from(form.querySelectorAll('input[name="selected"]'));
function addRow(id) {
  const li = document.createElement('li');
  li.dataset.id = id;
  const label = document.createElement('span');
  label.textContent = id;
  const controls = document.createElement('div');
  for (const [direction, symbol] of [['up', '↑'], ['down', '↓']]) {
    const button = document.createElement('button');
    button.type = 'button';
    button.dataset.move = direction;
    button.setAttribute('aria-label', 'Move ' + direction);
    button.textContent = symbol;
    controls.appendChild(button);
  }
  li.append(label, controls);
  ordered.appendChild(li);
}
function syncOrder() {
  const checked = new Set(checks.filter(x => x.checked).map(x => x.value));
  for (const li of Array.from(ordered.children)) if (!checked.has(li.dataset.id)) li.remove();
  const existing = new Set(Array.from(ordered.children).map(x => x.dataset.id));
  for (const input of checks) if (input.checked && !existing.has(input.value)) addRow(input.value);
}
form.addEventListener('change', event => {
  if (event.target.matches('input[name="selected"]')) syncOrder();
});
ordered.addEventListener('click', event => {
  const button = event.target.closest('button[data-move]');
  if (!button) return;
  const li = button.closest('li');
  if (button.dataset.move === 'up' && li.previousElementSibling) ordered.insertBefore(li, li.previousElementSibling);
  if (button.dataset.move === 'down' && li.nextElementSibling) ordered.insertBefore(li.nextElementSibling, li);
});
form.addEventListener('submit', () => {
  syncOrder();
  for (const input of form.querySelectorAll('input[name="order_enabled"],input[name="ordered_selected"]')) input.remove();
  const marker = document.createElement('input');
  marker.type = 'hidden'; marker.name = 'order_enabled'; marker.value = '1';
  form.appendChild(marker);
  for (const li of ordered.children) {
    const input = document.createElement('input');
    input.type = 'hidden'; input.name = 'ordered_selected'; input.value = li.dataset.id;
    form.appendChild(input);
  }
});'''
    return f'''<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Team Notebook</title><style>
:root{{--ink:#152b33;--muted:#60747a;--line:#d9e7e9;--cyan:#00a7bd;--bg:#f5f9fa}}
*{{box-sizing:border-box}}body{{margin:0;background:var(--bg);color:var(--ink);font:15px Inter,system-ui,sans-serif}}
main{{max-width:1100px;margin:0 auto;padding:36px 20px 80px}}header{{display:flex;gap:16px;align-items:center;border-bottom:1px solid var(--line);padding-bottom:20px;margin-bottom:24px}}
.marker{{background:var(--cyan);width:8px;height:34px;box-shadow:10px 0 #b4e9ee}}h1{{font-size:27px;margin:0 0 3px}}p{{margin:0;color:var(--muted)}}
.grid{{display:grid;grid-template-columns:minmax(0,1fr) 290px;gap:24px}}.card{{background:#fff;border:1px solid var(--line);border-radius:9px;padding:20px}}h2{{font-size:15px;text-transform:uppercase;letter-spacing:.12em;margin:0 0 14px}}
details{{border-top:1px solid var(--line);padding:8px 0}}summary{{cursor:pointer;font-weight:600}}summary small{{color:var(--muted);font-weight:400}}.file{{display:flex;gap:8px;align-items:start;padding:5px 4px 5px 16px;font:13px 'JetBrains Mono',monospace;overflow-wrap:anywhere;cursor:pointer}}input[type=checkbox]{{accent-color:var(--cyan)}}
.field{{display:block;color:var(--muted);font-size:12px;margin-bottom:13px;text-transform:uppercase;letter-spacing:.06em}}.field input,.field select{{display:block;width:100%;margin-top:5px;padding:8px;border:1px solid var(--line);border-radius:5px;background:#fff;color:var(--ink);font:14px Inter,system-ui,sans-serif}}
.toggle{{display:block;margin:14px 0}}.actions{{display:grid;gap:8px;margin-top:18px}}button{{background:var(--ink);color:#fff;border:0;border-radius:5px;padding:10px;cursor:pointer;font-weight:600}}button:hover{{background:#006e7d}}.notice{{background:#e6f7f9;border-left:3px solid var(--cyan);padding:12px;margin-bottom:20px;white-space:pre-wrap;overflow-wrap:anywhere}}
.order-title{{margin-top:22px}}.order-hint{{margin-bottom:8px;font-size:12px}}#selected-order{{list-style:none;padding:0;margin:0}}#selected-order li{{display:flex;align-items:center;justify-content:space-between;gap:10px;border-top:1px solid var(--line);padding:7px 4px;font:12px 'JetBrains Mono',monospace;overflow-wrap:anywhere}}#selected-order li span{{min-width:0}}#selected-order li div{{display:flex;flex:none;gap:3px}}#selected-order button{{padding:3px 8px;background:#e6f7f9;color:#006e7d}}
@media(max-width:760px){{.grid{{grid-template-columns:1fr}}}}
</style></head><body><main><header><div class="marker"></div><div><h1>Team Notebook</h1><p>Canonical sources · curated print layout</p></div></header>{notice}
<form method="post" action="/action"><input type="hidden" name="token" value="{esc(token)}"><div class="grid"><section class="card"><h2>Source selection</h2>{''.join(source_html) or '<p>No canonical 01–08 source files found yet.</p>'}<h2 class="order-title">Notebook order</h2><p class="order-hint">Use the arrows to arrange checked files. New selections go last.</p><ol id="selected-order">{ordered_html}</ol></section>
<aside class="card"><h2>Layout</h2>{field('title','Title')}{field('max_pages','Page budget','number','min="1" max="1000"')}{select('paper',list(PAPERS))}{select('columns',['1','2','3'])}{field('body_size','Body size (pt)','number','min="4" max="20" step="0.1"')}{field('code_size','Code size (pt)','number','min="4" max="20" step="0.1"')}{field('body_font','Body font')}{field('code_font','Code font')}{check('include_notes','Include contest notes')}{check('print_mode','Monochrome print mode')}
<div class="actions"><button name="action" value="save">Save selection</button><button name="action" value="generate">Generate .tex</button><button name="action" value="build">Build PDF</button></div></aside></div></form><script>{script}</script></main></body></html>'''


def serve(host: str, port: int) -> None:
    token = secrets.token_urlsafe(24)
    class Handler(BaseHTTPRequestHandler):
        def local_host(self) -> bool:
            return self.headers.get("Host") in (f"127.0.0.1:{self.server.server_address[1]}",
                                                f"localhost:{self.server.server_address[1]}")

        def do_GET(self) -> None:
            if self.path != "/" or not self.local_host():
                self.send_error(404)
                return
            self.respond(page(load(sources()), sources(), token))

        def do_POST(self) -> None:
            if self.path != "/action" or not self.local_host():
                self.send_error(404)
                return
            try:
                length = int(self.headers.get("Content-Length", "0"))
            except ValueError:
                self.send_error(400)
                return
            origin = self.headers.get("Origin")
            expected = f"http://{self.headers.get('Host', '')}"
            if not 0 < length <= 262144 or (origin and origin != expected):
                self.send_error(403)
                return
            values = urllib.parse.parse_qs(self.rfile.read(length).decode("utf-8"), keep_blank_values=True)
            if values.get("token") != [token]:
                self.send_error(403)
                return
            available = sources()
            try:
                cfg = dict(load(available))
                selected = form_selection(values)
                cfg.update({"selected": selected, "include_notes": "include_notes" in values,
                            "print_mode": "print_mode" in values, "title": values["title"][0],
                            "max_pages": int(values["max_pages"][0]), "paper": values["paper"][0],
                            "columns": int(values["columns"][0]), "body_size": float(values["body_size"][0]),
                            "code_size": float(values["code_size"][0]), "body_font": values["body_font"][0],
                            "code_font": values["code_font"][0]})
                action = values.get("action", ["save"])[0]
                if action not in ("save", "generate", "build"):
                    raise NotebookError("Unknown action")
                save(cfg, available)
                if action == "generate":
                    message = f"Generated {generate(cfg, available).name} with {len(cfg['selected'])} source file(s)."
                elif action == "build":
                    message, _ = build(cfg, available)
                elif action == "save":
                    message = "Selection saved."
            except (NotebookError, KeyError, IndexError, ValueError, OSError, subprocess.TimeoutExpired) as exc:
                message = f"Error: {exc}"
                cfg = load(available)
            self.respond(page(cfg, available, token, message))

        def respond(self, content: str) -> None:
            data = content.encode("utf-8")
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.end_headers()
            self.wfile.write(data)

    with ThreadingHTTPServer((host, port), Handler) as server:
        print(f"Notebook UI: http://{server.server_address[0]}:{server.server_address[1]}/", flush=True)
        server.serve_forever()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("list", help="List selectable canonical source IDs")
    update = sub.add_parser("save", help="Update 02-selection.json")
    update.add_argument("--select", action="append", metavar="SOURCE_ID", help="Source ID; repeat to select several")
    update.add_argument("--clear", action="store_true", help="Clear source selection")
    update.add_argument("--title")
    update.add_argument("--pages", type=int)
    update.add_argument("--paper", choices=list(PAPERS))
    update.add_argument("--columns", type=int, choices=(1, 2, 3))
    update.add_argument("--body-size", type=float)
    update.add_argument("--code-size", type=float)
    update.add_argument("--body-font")
    update.add_argument("--code-font")
    update.add_argument("--notes", action=argparse.BooleanOptionalAction, default=None)
    update.add_argument("--print-mode", action=argparse.BooleanOptionalAction, default=None)
    sub.add_parser("generate", help="Generate standalone 04-notebook.tex")
    sub.add_parser("build", help="Generate .tex and run LuaLaTeX")
    server = sub.add_parser("serve", help="Serve a local browser selector")
    server.add_argument("--port", type=int, default=0)
    args = parser.parse_args()
    try:
        available = sources()
        if args.command == "list":
            print("\n".join(available))
            return 0
        if args.command == "serve":
            serve("127.0.0.1", args.port)
            return 0
        cfg = load(available)
        if args.command == "save":
            if args.clear and args.select:
                raise NotebookError("Use either --clear or --select")
            if args.clear:
                cfg["selected"] = []
            elif args.select is not None:
                cfg["selected"] = args.select
            for flag, key in (("title", "title"), ("pages", "max_pages"), ("paper", "paper"),
                              ("columns", "columns"), ("body_size", "body_size"), ("code_size", "code_size"),
                              ("body_font", "body_font"), ("code_font", "code_font"),
                              ("notes", "include_notes"), ("print_mode", "print_mode")):
                value = getattr(args, flag)
                if value is not None:
                    cfg[key] = value
            save(cfg, available)
            print(f"Saved {CONFIG.name}: {len(cfg['selected'])} source file(s).")
        elif args.command == "generate":
            print(f"Generated {generate(cfg, available)} with {len(cfg['selected'])} source file(s).")
        else:
            message, status = build(cfg, available)
            print(message)
            return status
        return 0
    except (NotebookError, OSError, subprocess.TimeoutExpired) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
