# Team Notebook

This folder builds a printable, selectable LuaLaTeX notebook from the **canonical** C++ and Python source files in the repository's numbered `01`–`08` folders. The generated TeX uses `\lstinputlisting` to read those files directly. It does not store another copy of the algorithms. Each selected file is included in full, so a `mini` variant can be chosen on its own.

## Start

Run these commands from this folder with Python 3.10 or newer:

```bash
python3 01-notebook.py serve
```

Open the printed `http://127.0.0.1:<port>/` address. Check the files to include (checking a header also checks its include dependencies, placed before it), use the arrows in **Notebook order** to arrange them, set paper, columns, fonts, point sizes, page budget and color mode, then **Save**, **Generate .tex** or **Build PDF**. The page displays the build result. The server listens only on the local loopback interface and chooses an available port by default. Stop it with Ctrl+C.

The same operations are available without a browser:

```bash
python3 01-notebook.py list
python3 01-notebook.py save --select '01-Core/01-template.hpp' --select '01-Core/05-modint.hpp' --pages 25 --paper a4 --columns 2
python3 01-notebook.py generate
python3 01-notebook.py build
```

`save --select` adds the include dependencies of the selected headers, each before the first source that needs it unless it is selected later, and prints them; `--no-deps` keeps the selection as given. `save --clear` removes all selected sources. Use `--notes` or `--no-notes` and `--print-mode` or `--no-print-mode` to control the two independent checkboxes. Run `save --help` for all controls.

The checked-in [selection](02-selection.json) starts with contest notes, the core template and debug header. Select the files the team needs for its rules and page budget. Source order in the JSON array is print order; repeated `--select` flags also preserve their order. [Contest notes](03-notes.md) are an editable, short Markdown subset (`#`, `##`, `-`, and plain paragraphs). The generated `04-notebook.tex` and PDF are build artifacts. Rebuild after changing canonical source or notes.

## Requirements and behavior

`generate` uses only Python's standard library. `build` requires `lualatex` and the configured fonts installed for LuaLaTeX/fontconfig (defaults **Inter** for text and **JetBrains Mono** for code) and reports a missing executable or font by name. On Arch these are `texlive-basic`, `texlive-latex`, `texlive-latexrecommended`, `texlive-luatex`, `texlive-fontsrecommended` (Latin Modern, which LuaLaTeX loads before the configured fonts), `inter-font` and `ttf-jetbrains-mono`. After LuaLaTeX runs, it reports the actual page count and any excess over the budget; an over-budget PDF is retained and the CLI exits with status 2. Build, font and page-budget rules: [07-notebook.md](../00-Guidelines/07-notebook.md).

Source IDs must match a discovered regular C++ or Python file inside a numbered `01`–`08` source folder. `OLD`, topic-local `97-Legacy`, `build` and `__pycache__` subtrees are skipped. A selected path outside those folders, a symlink, or a stale ID is rejected. This avoids accidental inclusion of test programs, workspace files, archived notebook material, and arbitrary filesystem paths. Only the fixed notebook artifact paths are written.

Code listings are for reference; the notebook is a source collection, not a C++ compilation unit. Under each heading a `Needs:` line names the header's direct includes (the template only when it is not selected), marking unselected ones and the reduction headers that only the non-`Compact` functions need.

## Dependency graph

[05-dependencies.py](05-dependencies.py) reads the `#include "…"` lines of the `01`–`07` headers. It serves the selector above, `plan.py check` (every include edge between batches is a declared prerequisite) and the documentation site below:

```bash
python3 05-dependencies.py closure '05-Mathematics/12-combinatorics_advanced.hpp'   # typing order with line counts; no argument: every header
python3 05-dependencies.py chart '07-Strings'                                       # Mermaid include chart without template edges; no argument: every folder
```

Contracts, including which mini and `Compact` substitutes a listing offers, and evidence: [Docs/00-dependencies.md](Docs/00-dependencies.md).

Some sources contain non-ASCII comment characters (`rg -l '[^\x00-\x7F]' -g '*.{hpp,cpp,py}' 0[1-8]-*/` from the repository root lists them). The first build (2026-10-15) rendered the set then present except the template's Chinese motto, which shows no glyphs because Inter and JetBrains Mono have none (P226 adds a CJK fallback font).

## Documentation site

[06-site.py](06-site.py) generates a documentation site for [Zensical](https://zensical.org/) or [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) from the canonical sources. Each existing header or module gets one page: operations from its inventory row; complexity from its comments; contracts, limits and sources from its evidence document; then its include line, typing order, includers, tests and source listing. Each folder gets an overview with its inventory tables and include chart, plus its notes. Hand-written articles go in `Articles/<NN-Folder>/<NN-stem>.md` (the evidence document's name) and are merged into that header's page; `Articles/<NN-Folder>/00-index.md` goes on the folder overview. Other files in the same folder are copied beside the pages as images. An article or file without an existing header or folder fails the generation.

```bash
python3 06-site.py generate          # site/mkdocs.yml and site/docs/; standard library only
python3 06-site.py build             # then a strict build into site/html/; open site/html/index.html, which works offline
python3 06-site.py serve --port 8000 # then serve on 127.0.0.1; restart it after editing sources or articles
python3 06-site.py build --engine mkdocs   # force an engine (auto: Zensical, else MkDocs)
```

`build` and `serve` need Zensical (`pipx install zensical`) or MkDocs Material (`pacman -S mkdocs-material python-pymdown-extensions`, or `pipx install mkdocs-material --include-deps`). They name a missing engine or module. The first build downloads the fonts and scripts the site self-hosts. Everything under `site/` is generated and git-ignored. Contracts and evidence: [Docs/00-site.md](Docs/00-site.md).

## Selection and validation backlog

The inventory is a catalog to curate from, not a request to print every header. Review practical Basic/Advanced coverage and choose compact/full variants for the team's rules; include rare algorithms only when their value justifies their printed lines. Planned files cannot be selected before they exist. The current selector discovers source paths and does not display inventory tiers or verification status; adding those labels is **planned**.

After renames or completed batches, review stale selection IDs, dependency needs, duplicate listings and contest-note references. Dependencies are selected automatically (above); repeated-header-content deduplication is not implemented. Keep fixed Core ordering separate from tier ordering elsewhere. Check PDF glyphs, line wrapping, clipping, column balance, page count, chosen fonts and monochrome readability after a real LuaLaTeX build; generation alone is not print validation. Those PDF checks are still outstanding.

Use the [saved KACTL/Stanford notebooks and other resources](../95-Resources/00-index.md) for comparison, with provenance for adopted material; [09-sources.md](../00-Guidelines/09-sources.md) lists the excluded sources. Archived local notebook sources in `OLD` may reveal missing algorithms; account for them in the [archive map](../00-Guidelines/Ledgers/archive-map.json). The [completeness audit](../00-Guidelines/History/2026-09-27-inventory-audit.md) records source scope and remaining limitations.
