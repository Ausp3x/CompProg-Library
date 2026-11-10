---
paths:
  - "**/00-index.md"
  - "**/Docs/*.md"
  - "00-Guidelines/13-Plan/**"
  - "**/98-basic.hpp"
  - "**/99-all.hpp"
  - "08-Python/_98_basic.py"
  - "08-Python/_99_all.py"
---

# Structure, naming, inventories

## Layout

Algorithm folders are `01-Core` … `07-Strings` and `08-Python`; `09-Contest Testing` holds contest tools; `95-Resources` holds reference PDFs; `96`–`99` hold tests, judge solutions, the notebook with the documentation site, and the workspace; `OLD` is the archive. Do not add top-level folders.

## Names

- Files and folders sort by a two-digit prefix and hyphen: `05-modint.hpp`, `04-Graphs`. Folders are Title Case with spaces (`02-Data Structures`, `13-Plan`, `Docs`, `Ledgers`, `History`); files are lowercase (`98-basic.hpp`, `00-notes.md`, `duplicates.md`).
- Python modules use `_05_name.py` (importable identifier); executable scripts may use `05-name.py`.
- One main algorithm: lowercase established name without underscores (`01-dsu.hpp`). Several related algorithms: lowercase use-case stem with underscores (`05-prime_algorithms.hpp`).
- Folder-level files inside a folder take prefix `00` (`00-index.md`, `Docs/00-notes.md`, `00-foundations_benchmark.py`); header-level files take the header's prefix and stem (`Docs/05-modint.md`, `05-modint_tester.py`, `05-modint_benchmark.py`). `02-Data Structures/00-monoids.hpp` is the one folder-level header with an inventory row; its files match prefix and stem.
- Explicit exceptions: `CLAUDE.md`, `AGENTS.md`, `README.md`, `CONTRIBUTING.md`, `LICENSE`, `THIRD-PARTY.md`, `__init__.py`, `OLD` and its contents, `95-Resources` user-provided filenames, `src` and `expanded` under `97-Online Testing`, `template.cpp`, `Articles` (with its image files) and the generated `site` under `98-Team Notebook`, the contents of `00-Guidelines/13-Plan`, `23-Sweep Ledgers`, `Ledgers` and `History`, `.claude`, tool and config names.
- Before a folder's two-digit prefixes run out, group a topic into a numbered subfolder; avoid other nesting.
- A prefix is a stable identifier. It is assigned once and never reused; a new row takes the next free number in its folder. Sections list rows in importance order, not numeric order. Rename or renumber an existing file only as an explicitly scoped task that updates includes, imports, tests, judge sources, notebook selection, maps and inventory together.
- Companion documents live in the folder's `Docs/` subdirectory: `Docs/00-notes.md` (folder-wide contracts, ownership inside the folder and cross-folder ownership), `Docs/00-sources.md` (source ledger), and one `Docs/<NN-name>.md` per header with the same prefix and stem as the header (contracts, feature-to-test map, commands and results, benchmarks, provenance). An evidence document records the latest run only: current contracts, the feature-to-test map, the commands with their most recent results and date, current benchmark tables, sources and open limits. `## History` lists every run, the latest included, one line each (`2026-10-07: re-audit, 3 findings fixed, full suite passed on g++ and g++-14`). Resolved findings, superseded tables and old hashes are deleted, not kept. No other document shape exists: a package's evidence is the set of its headers' documents, and a support pass records its evidence in `Docs/00-<topic>.md` of the folder it owns. Site articles (`07-notebook.md`) are not evidence.

## Operation names

- An operation takes the established name of its task or algorithm in the catalogs (`minCostFlow`, `hungarian`) in `03-cpp.md` (Style) case, with no `get` prefix except a point read named `get`. A word means the same thing in every folder.
- A variant keeps its operation's name and appends what distinguishes it: `Approx` (floating-point result) or `Exact` (big-integer or rational result beside a modular or floating default); `Real` (real-valued domain, `binSearchRealBracket`); `Mod` (result modulo a parameter); `Count`, `All` or `Witness` (only the count, every solution, a reconstruction); `Table` (every value up to a bound); `InRange` (a subrange query); `Sorted` (presorted input); `Dense` or `Sparse` (input representation); `Offline` or `Online`; `2`, `3` or `D` (dimension). A further algorithm for the same task appends its bound when that is the difference (`garsiaWachsQuadratic`), `Naive` when it is the direct definition (`convolutionNaive`), `Simple` when it is another lighter algorithm `01-principles.md` (Priority) keeps (`dominatorTreeSimple`), and otherwise its short name (`convexHullGraham`, `closestPairRandomized`), unless its own name is the established one (`huntSzymanski`, `edmondsKarp`).
- A type name belongs to one inventory row, and a name `01-template.hpp` defines to that row alone (a member may reuse it), since every header includes it; a free function another row also names must take other argument types (`distanceApprox` on 2D and 3D points). `03-consistency.py` rejects the first two and reports the third.
- An existing name that breaks these rules changes only in a scoped rename that updates rows, code, tests, call sites, `08-Python` mirrors, judge sources and notebook selection together.

## Tiers and Core order

- Basic, Advanced and Esoteric are sections of each inventory outside Core, never directories or filename labels. Basic is what a strong contestant uses in most contests; Advanced is used in harder rounds; Esoteric is research-level or rarely needed, and holds every standalone one-problem algorithm; a one-problem composition of a row's own operations stays in that row. A header belongs to exactly one tier.
- Core keeps its fixed order 01 template, 02 debug, 03 barrett, 04 montgomery, 05 modint, 06 modintmini, 07 infint, 08 infintmini, 09 rational, 10 matrix, 11 matrixmini, 12 bitmatrix, 13 bitmatrixmini, 14 sparsematrix, 15 sparsematrixmini, 16 poly, 17 polymini, 18 bitset, 19 fastio. Add a Core type only for a demonstrated shared need.
- Core Poly owns the accelerated polynomial/FPS engine; Mathematics may intentionally duplicate scalar FFT/NTT/FPS, Matrix and linear-recurrence functionality.

## Aggregates

- `98-basic.hpp` includes every Basic header of its folder; Core Basic includes minis where they exist plus full types that Basic consumers need.
- `99-all.hpp` includes every implemented header of its folder once, full and mini, transitively; never aggregates, tests or planned files.
- Algorithms include their direct dependencies, never an aggregate.
- Python `_98_basic.py` and `_99_all.py` re-export explicit public names only; importing them performs no work.

## Inventory contract

Each algorithm folder has `00-index.md` with this exact shape:

1. Title line `# NN Folder — inventory`.
2. One `Scope` paragraph of at most four sentences: what the folder owns and what neighbouring folders own instead.
3. Sections `## Basic`, `## Advanced`, `## Esoteric` (Core: one untitled table in Core order), each a table with columns `Header | Operations | Status` (Core: `Header | Basic | Operations | Status`; Python: `Module | Operations | Status`).
4. No other section: ownership rules beyond the Scope paragraph, contracts, proofs, source ledgers and audit narrative live in companion documents (`Docs/00-notes.md` owns ownership rules), not in the index.

Row rules:

- `Header`: the filename in a code span.
- `Operations`: every public struct, function, operator and named variant the row must provide, as short identifier-like names separated by `;`, grouped by struct where several exist (`DSU: find, unite, same, size, count, groups`). Give a domain qualifier in parentheses only when it changes the algorithm (`kth (persistent)`). A struct's constructors are `ctor` (`ctor (vector)`) and its operators their symbols (`+=`, `==`), so the extraction check (`05-testing.md`, Extraction) finds each entry in the header. Name variants explicitly instead of writing "and extensions", "where meaningful" or "research". A planned research item lists the concrete operations it would provide or does not appear.
- A mini row (`*mini.hpp`) lists every operation of its full type's row under the same operation names (its types, companion, concepts and detail namespace keep mini names, `03-cpp.md`, API contracts), except the full type's backend internals (ISA kernels, reduction contexts, arenas and the backend variants its dispatch chooses among), which it names after `internals:` at the end of its Operations cell.
- `Status`: exactly one of `planned`, `partial`, `existing-unverified`, `legacy-reference`, `verified`, followed by evidence links for verified/partial rows; rows without an evidence link carry `legacy:` paths or source keys (`09-sources.md`, Source keys), while a verified row's archive accounting lives in `Ledgers/archive-map.json` and its evidence document. `partial` lists the missing operations after `missing:`.

All-solution and enumeration APIs return compact parameterizations or explicitly bounded enumerations and are charged by output size.

## Archive

`OLD` keeps original bytes of superseded code and notebooks and is excluded from aggregates, discovery and notebook selection. A `97-Legacy/` excerpt is a working copy of an `OLD` range; deleting it (`01-principles.md`) means `git rm`, dropping its row from the legacy index and removing an empty folder; its verified row has accounted for its features and `OLD` keeps the bytes. Accounted for means each archived feature maps to a kept operation or is recorded as not adopted, never that an old name, signature or convention survives (`03-cpp.md`, One API per operation).
