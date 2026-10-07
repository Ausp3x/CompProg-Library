---
paths:
  - "**/00-index.md"
  - "00-Guidelines/13-Plan/**"
  - "**/98-basic.hpp"
  - "**/99-all.hpp"
  - "08-Python/_98_basic.py"
  - "08-Python/_99_all.py"
---

# Structure, naming, inventories

## Layout

Algorithm folders are `01-Core` … `07-Strings` and `08-Python`; `09-Contest Testing` holds contest tools; `95-Resources` holds reference PDFs; `96`–`99` hold tests, judge solutions, the notebook and the workspace; `OLD` is the archive. Do not add top-level folders.

## Names

- Files and folders sort by a two-digit prefix and hyphen: `05-modint.hpp`, `04-Graphs`. Folders are Title Case with spaces (`02-Data Structures`, `13-Plan`, `Docs`, `Ledgers`, `History`); files are lowercase (`98-basic.hpp`, `00-notes.md`, `p016.md`).
- Python modules use `_05_name.py` (importable identifier); executable scripts may use `05-name.py`.
- One main algorithm: lowercase established name without underscores (`01-dsu.hpp`). Several related algorithms: lowercase use-case stem with underscores (`05-prime_algorithms.hpp`).
- Folder-level files inside a folder take prefix `00` (`00-index.md`, `Docs/00-notes.md`, `00-foundations_benchmark.py`); header-level files take the header's prefix and stem (`Docs/05-modint.md`, `05-modint_tester.py`, `05-modint_benchmark.py`).
- Explicit exceptions: `CLAUDE.md`, `AGENTS.md`, `__init__.py`, `OLD` and its contents, `95-Resources` user-provided filenames, `src` and `expanded` under `97-Online Testing`, `template.cpp`, `.claude`, tool and config names.
- A prefix is a stable identifier. It is assigned once and never reused; a new row takes the next free number in its folder. Sections list rows in importance order, not numeric order. Rename or renumber an existing file only as an explicitly scoped task that updates includes, imports, tests, judge sources, notebook selection, maps and inventory together.
- Companion documents live in the folder's `Docs/` subdirectory: `Docs/00-notes.md` (contracts and cross-folder ownership), `Docs/00-sources.md` (source ledger), and one `Docs/<NN-name>.md` per header with the same prefix and stem as the header (contracts, feature-to-test map, commands and results, benchmarks, provenance). An evidence document records the latest run only: current contracts, the feature-to-test map, the commands with their most recent results and date, current benchmark tables, sources and open limits. Earlier runs collapse to one line each under `## History` (`2026-10-07: re-audit, 3 findings fixed, full suite passed on g++ and g++-14`). Resolved findings, superseded tables and old hashes are deleted, not kept. No other document shape exists: a package's evidence is the set of its headers' documents. `97-Legacy/` holds unchanged `.cpp` excerpts with an index until their rows are verified; `98-basic` and `99-all` are aggregates.

## Tiers and Core order

- Basic, Advanced and Esoteric are sections of each inventory outside Core, never directories or filename labels. Basic is what a strong contestant uses in most contests; Advanced is used in harder rounds; Esoteric is research-level or rarely needed. A header belongs to exactly one tier.
- Core keeps its fixed order 01 template, 02 debug, 03 barrett, 04 montgomery, 05 modint, 06 modintmini, 07 infint, 08 infintmini, 09 rational, 10 matrix, 11 matrixmini, 12 bitmatrix, 13 bitmatrixmini, 14 sparsematrix, 15 sparsematrixmini, 16 poly, 17 polymini, 18 bitset. Add a Core type only for a demonstrated shared need.
- Core Poly owns the accelerated polynomial/FPS engine; Mathematics may intentionally duplicate scalar FFT/NTT/FPS. No circular dependencies; Core never includes a topic aggregate.

## Aggregates

- `98-basic.hpp` includes every Basic header of its folder; Core Basic includes minis where they exist plus full types that Basic consumers need.
- `99-all.hpp` includes every implemented header of its folder once, full and mini, transitively; never aggregates, tests or planned files.
- Algorithms include their direct dependencies, never an aggregate. Full and mini names coexist (`iint`, `iintmini`).
- Python `_98_basic.py` and `_99_all.py` re-export explicit public names only; importing them performs no work.

## Inventory contract

Each algorithm folder has `00-index.md` with this exact shape:

1. Title line `# NN Folder — inventory`.
2. One `Scope` paragraph of at most four sentences: what the folder owns and what neighbouring folders own instead.
3. Sections `## Basic`, `## Advanced`, `## Esoteric` (Core: one untitled table in Core order), each a table with columns `Header | Operations | Status` (Core: `Header | Basic | Operations | Status`; Python: `Module | Operations | Status`).
4. Optional `## Notes` of at most ten lines for cross-folder ownership rules. Contracts, proofs, source ledgers and audit narrative live in companion documents, not in the index.

Row rules:

- `Header`: the filename in a code span.
- `Operations`: every public struct, function, operator and named variant the row must provide, as short identifier-like names separated by `;`, grouped by struct where several exist (`DSU: find, unite, same, size, count, groups`). Give a domain qualifier in parentheses only when it changes the algorithm (`kth (persistent)`). Name variants explicitly instead of writing "and extensions", "where meaningful" or "research". A planned research item lists the concrete operations it would provide or does not appear.
- `Status`: exactly one of `planned`, `partial`, `existing-unverified`, `legacy-reference`, `verified`, followed by evidence links for verified/partial rows and by `legacy:` paths or short source keys otherwise. `verified` means every operation in the row has evidence. `partial` lists the missing operations after `missing:`.

All-solution and enumeration APIs return compact parameterizations or explicitly bounded enumerations and are charged by output size.

## Archive

`OLD` keeps original bytes of superseded code and notebooks and is excluded from aggregates, discovery and notebook selection; it is never deleted. A `97-Legacy/` excerpt is a working copy of an `OLD` range: delete it (`git rm`, drop its row from the legacy index, remove the folder when empty) as soon as the row it informs is `verified`, because its features are then accounted for and `OLD` still holds the bytes.
