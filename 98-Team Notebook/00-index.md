# Team Notebook

This folder builds a printable, selectable LuaLaTeX notebook from the **canonical** C++ and Python source files in the repository's numbered `01`–`08` folders. The generated TeX uses `\lstinputlisting` to read those files directly. It does not store another copy of the algorithms. Each selected file is included in full, so a `mini` variant can be chosen on its own.

## Start

Run these commands from this folder with Python 3.10 or newer:

```bash
python3 01-notebook.py serve
```

Open the printed `http://127.0.0.1:<port>/` address. Check the files to include, use the arrows in **Notebook order** to arrange them, set paper, columns, fonts, point sizes, page budget and color mode, then **Save**, **Generate .tex** or **Build PDF**. The page displays the build result. The server listens only on the local loopback interface and chooses an available port by default. Stop it with Ctrl+C.

The same operations are available without a browser:

```bash
python3 01-notebook.py list
python3 01-notebook.py save --select '01-Core/01-template.hpp' --select '01-Core/05-modint.hpp' --pages 25 --paper a4 --columns 2
python3 01-notebook.py generate
python3 01-notebook.py build
```

`save --clear` removes all selected sources. Use `--notes` or `--no-notes` and `--print-mode` or `--no-print-mode` to control the two independent checkboxes. Run `save --help` for all controls.

The checked-in [selection](02-selection.json) starts with contest notes, the core template and debug header. Select the files the team needs for its rules and page budget. Source order in the JSON array is print order; repeated `--select` flags also preserve their order. [Contest notes](03-notes.md) are an editable, short Markdown subset (`#`, `##`, `-`, and plain paragraphs). The generated `04-notebook.tex` and PDF are build artifacts. Rebuild after changing canonical source or notes.

## Requirements and behavior

`generate` uses only Python's standard library. `build` requires `lualatex` and the configured fonts installed for LuaLaTeX/fontconfig (defaults **Inter** for text and **JetBrains Mono** for code) and reports a missing executable or font by name. On Arch these are `texlive-basic`, `texlive-latex`, `texlive-latexrecommended`, `texlive-luatex`, `texlive-fontsrecommended` (Latin Modern, which LuaLaTeX loads before the configured fonts), `inter-font` and `ttf-jetbrains-mono`. After LuaLaTeX runs, it reports the actual page count and any excess over the budget; an over-budget PDF is retained and the CLI exits with status 2. Build, font and page-budget rules: [07-notebook.md](../00-Guidelines/07-notebook.md).

Source IDs must match a discovered regular C++ or Python file inside a numbered `01`–`08` source folder. `OLD`, topic-local `97-Legacy`, `build` and `__pycache__` subtrees are skipped. A selected path outside those folders, a symlink, or a stale ID is rejected. This avoids accidental inclusion of test programs, workspace files, archived notebook material, and arbitrary filesystem paths. Only the fixed notebook artifact paths are written.

Code listings are for reference: a full header may repeat dependencies that also appear in another selected file. The notebook is a source collection, not a C++ compilation unit.

Some sources contain non-ASCII comment characters: the template's Chinese motto, and en dashes or an accented letter in `01-Core/16-poly.hpp`, `01-Core/17-polymini.hpp` and `03-Geometry/10-halfplaneintersection.hpp` (`rg -l '[^\x00-\x7F]' -g '*.{hpp,cpp,py}' 0[1-8]-*/` from the repository root lists the current set). The first build (2026-10-15) renders them except the template's Chinese motto, which shows no glyphs because Inter and JetBrains Mono have none (P226 adds a CJK fallback font).

## Selection and validation backlog

The inventory is a catalog to curate from, not a request to print every header. Review practical Basic/Advanced coverage and choose compact/full variants for the team's rules; include rare algorithms only when their value justifies their printed lines. Planned files cannot be selected before they exist. The current selector discovers source paths and does not display inventory tiers or verification status; adding those labels is **planned**.

After renames or completed batches, review stale selection IDs, dependency needs, duplicate listings and contest-note references. Source listings currently require manual dependency curation; automatic dependency closure or repeated-header-content deduplication is not implemented. Keep fixed Core ordering separate from tier ordering elsewhere. Check PDF glyphs, line wrapping, clipping, column balance, page count, chosen fonts and monochrome readability after a real LuaLaTeX build; generation alone is not print validation. Those PDF checks are still outstanding.

Use the [saved KACTL/Stanford notebooks and other resources](../95-Resources/00-index.md) for comparison, with provenance for adopted material; [09-sources.md](../00-Guidelines/09-sources.md) lists the excluded sources. Archived local notebook sources in `OLD` may reveal missing algorithms; account for them in the [archive map](../00-Guidelines/Ledgers/archive-map.json). The [completeness audit](../00-Guidelines/History/2026-09-27-inventory-audit.md) records source scope and remaining limitations.
