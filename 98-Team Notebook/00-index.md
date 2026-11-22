# Team Notebook

This folder builds a printable LuaLaTeX notebook from the **canonical** C++ and Python sources in the numbered `01`–`08` folders. The generated TeX reads those files directly with `\lstinputlisting`, so no second copy of the algorithms exists. Each selected file is printed in full, so a `mini` variant can be chosen on its own.

## Start

Run these commands from this folder with Python 3.10 or newer:

```bash
python3 01-notebook.py serve
```

Open the printed `http://127.0.0.1:<port>/` address and check the files to include; checking a header also checks its include dependencies, placed before it. Arrange the files with the arrows in **Notebook order** and set paper, columns, fonts, point sizes, page budget and color mode. Then **Save**, **Generate .tex** or **Build PDF**; the page shows the build result. The server listens on the loopback interface only, on a free port by default. Stop it with Ctrl+C.

The same operations are available without a browser:

```bash
python3 01-notebook.py list
python3 01-notebook.py save --select '01-Core/01-template.hpp' --select '01-Core/05-modint.hpp' --pages 25 --paper a4 --columns 2
python3 01-notebook.py generate
python3 01-notebook.py build
```

`save --select` adds the include dependencies of the selected headers, each before the first source that needs it unless it is selected later, and prints them; `--no-deps` keeps the selection as given. `save --clear` removes every selected source. `--notes`/`--no-notes` and `--print-mode`/`--no-print-mode` set the two checkboxes. `save --help` lists every control.

The checked-in [selection](02-selection.json) starts with the contest notes, the template and the debug header; add what the team needs for its rules and page budget. The order of the JSON array, like the order of repeated `--select` flags, is print order. [Contest notes](03-notes.md) use a short Markdown subset (`#`, `##`, `-` and plain paragraphs). The generated `04-notebook.tex` and the PDF are build artifacts; rebuild after changing a source or the notes.

## Requirements and Behavior

`generate` uses only Python's standard library. `build` needs `lualatex` and the configured fonts installed for LuaLaTeX and fontconfig (by default **Inter** for text and **JetBrains Mono** for code), and names a missing executable or font; `/setup-check` prints the packages. After LuaLaTeX runs, `build` reports the page count and any excess over the budget; an over-budget PDF is kept and the CLI exits with status 2. Build, font and page-budget rules: [07-notebook.md](../00-Guidelines/07-notebook.md).

A source ID must name a regular C++ or Python file in a numbered `01`–`08` folder; `OLD`, `97-Legacy`, `build` and `__pycache__` subtrees are skipped. A path outside those folders, a symlink or a stale ID is rejected, so tests, workspace files, archived material and arbitrary paths cannot be printed. Only the fixed notebook artifact paths are written.

The notebook is a source collection, not a compilation unit. Under each heading a `Needs:` line names the header's direct includes (the template only when it is not selected) and marks the unselected ones.

## Dependency Graph

[05-dependencies.py](05-dependencies.py) reads the `#include "…"` lines of the `01`–`07` headers. It serves the selector above, `plan.py check` (every include edge between batches is a declared prerequisite) and the documentation site below:

```bash
python3 05-dependencies.py closure '05-Mathematics/12-combinatorics_advanced.hpp'   # typing order with line counts; no argument: every header
python3 05-dependencies.py chart '07-Strings'                                       # Mermaid include chart without template edges; no argument: every folder
```

Contracts and evidence: [Docs/00-dependencies.md](Docs/00-dependencies.md). To type only what one name needs, extract it with [10-grab.py](<../09-Contest Testing/10-grab.py>).

Some sources contain non-ASCII comment characters (`rg -l '[^\x00-\x7F]' -g '*.{hpp,cpp,py}' 0[1-8]-*/` from the repository root lists them). The first build (2026-10-15) rendered all of them except the template's Chinese motto, which has no glyphs in Inter or JetBrains Mono; P226 adds a CJK fallback font.

## Documentation Site

[06-site.py](06-site.py) generates a documentation site for [Zensical](https://zensical.org/) or [MkDocs Material](https://squidfunk.github.io/mkdocs-material/) from the canonical sources. Each existing header or module gets one page: operations from its inventory row; complexity from its comments; contracts, limits and sources from its evidence document; then its include line, typing order, includers, tests and source listing. Each folder gets an overview with its inventory tables and include chart, plus its notes. Hand-written articles go in `Articles/<NN-Folder>/<NN-stem>.md` (the evidence document's name) and are merged into that header's page; `Articles/<NN-Folder>/00-index.md` goes on the folder overview. Other files in the same folder are copied beside the pages as images. An article or file without an existing header or folder fails the generation.

```bash
python3 06-site.py generate          # site/mkdocs.yml and site/docs/; standard library only
python3 06-site.py build             # then a strict build into site/html/; open site/html/index.html, which works offline
python3 06-site.py serve --port 8000 # then serve on 127.0.0.1; restart it after editing sources or articles
python3 06-site.py build --engine mkdocs   # force an engine (auto: Zensical, else MkDocs)
```

`build` and `serve` need Zensical (`pipx install zensical`) or MkDocs Material with pymdown-extensions (`pipx install mkdocs-material --include-deps`). They name a missing engine or module. The first build downloads the fonts and scripts the site self-hosts. Everything under `site/` is generated and git-ignored. Contracts and evidence: [Docs/00-site.md](Docs/00-site.md).

## Selection and Validation Backlog

The inventory is a catalog to curate from, not a request to print every header. Choose Basic and Advanced coverage and compact or full variants for the team's rules; a rare algorithm earns its printed lines only by its value. Planned files cannot be selected before they exist. The selector shows source paths only; showing inventory tiers and verification status is **planned**.

After renames or completed batches, review stale selection IDs, dependency needs, duplicate listings and contest-note references. Dependencies are selected automatically; content repeated across headers is not deduplicated. Keep the fixed Core order separate from tier order elsewhere. Print validation needs a real LuaLaTeX build checked for glyphs, line wrapping, clipping, column balance, page count, fonts and monochrome readability; generation alone is not print validation, and these checks are still outstanding.

Use the [saved KACTL/Stanford notebooks and other resources](../95-Resources/00-index.md) for comparison, with provenance for adopted material; [09-sources.md](../00-Guidelines/09-sources.md) lists the excluded sources. Archived local notebook sources in `OLD` may reveal missing algorithms; account for them in the [archive map](../00-Guidelines/Ledgers/archive-map.json). The [completeness audit](../00-Guidelines/History/2026-09-27-inventory-audit.md) records source scope and remaining limitations.
