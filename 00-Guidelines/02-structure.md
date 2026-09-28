# Structure, naming, inventories

The root layout is defined in `../00-index.md`. Algorithm folders are 01–08; 09 contains contest tools; 95 contains reference resources; 96–99 contain verification/notebook/workspace material. `95-Resources` needs only a short collection index and download metadata, not algorithm feature inventories or a separate guideline file. Existing user-provided resource filenames may be retained. Do not invent additional top-level categories without a concrete need.

## Names and ordering

- Use two-digit numeric prefixes and a hyphen for human-sorted files/folders: `01-template.hpp`, `05-Mathematics`. Keep the approved major-folder names.
- Importable Python modules/packages use a leading underscore and underscores: `_01_dijkstra.py`, `_04_graphs`. Neither `01-name` nor `01_name` is an ordinary Python identifier. The outer `08-Python` directory is an import root, not a package name.
- `AGENTS.md`, `__init__.py`, conventional tool/config names, `OLD`, `src`, `expanded`, build/artifact paths, and the requested `99-Workspace/template.cpp` are explicit naming exceptions. Executable Python scripts may use `01-name.py`.
- One main algorithm: lowercase established name/abbreviation without underscores, e.g. `01-dsu.hpp`. Multiple related algorithms: lowercase general use-case name with underscores, e.g. `05-prime_algorithms.hpp`. Numeric prefixes are separate from this stem rule.
- Basic, Advanced, Esoteric are sections in each folder inventory, **not** filename labels or tier directories. Sort by importance/rarity first, dependencies second. A header/module cannot mix tiers; split it when necessary. Mini/full is separate from tier.
- Numeric prefixes change when ordering changes. Update includes, imports, tests, online sources, notebook selection, and indexes in the same change. Use numbered topic subfolders before exhausting 01–97. Each topic may have its own aggregates. Avoid deep unnecessary nesting.

## Core fixed order

01 template; 02 debug; 03 barrett; 04 montgomery; 05 modint (all full static/dynamic 32/64-bit types); 06 modintmini (four independent mini structs); 07 infint; 08 infintmini; 09 rational; 10 matrix; 11 matrixmini; 12 bitmatrix; 13 bitmatrixmini; 14 sparsematrix; 15 sparsematrixmini; 16 poly; 17 polymini; 18 bitset.

Core does not use tier ordering. It contains separate foundational headers, not one giant required implementation. Add types only for a demonstrated general need. Core Poly covers polynomials, formal power series and their comprehensive extensions with extreme optimization. Intentional duplication of scalar FFT/NTT/FPS functionality in Mathematics is permitted. No circular dependencies; foundational Core headers must not depend on topic aggregates.

## Aggregates

`98-Basic.hpp` and `99-All.hpp` are include-only convenience headers in 01–07 and relevant topic folders. Basic includes all Basic entries; Core chooses minis where available/appropriate plus necessary full types. All includes every existing implementation header, full and mini, once transitively; exclude aggregates themselves, tests and planned nonexistent files. Algorithms include direct dependencies, never an aggregate. Full/mini names and aliases must coexist; use `iint` and `iintmini`, not two conflicting `iint` definitions. Aggregates advertise only current implemented coverage.

Python uses `_98_basic.py` and `_99_all.py` import/re-export modules and explicit public exports. No work, I/O, benchmarks or testing on import. Until algorithms exist, aggregates may be empty with an honest inventory status.

## Inventory contract

Each folder has `00-index.md`. Keep rows concise but cover public operations, variants, domains and outstanding research. Distinguish **planned**, **existing-unverified**, **partial**, **verified** (with evidence), and **legacy-reference**. List proposed filenames, tier or Core Basic eligibility, features, references and meaningful omissions. An inventory is not a claim that every known paper has been surveyed. Add newly discovered relevant variants as future research identifies them. Detailed API proofs/references belong in numbered companion documentation, not repeated across every guide.

Use the same table columns in all Basic, Advanced and Esoteric sections: `Proposed header | Feature checklist | Status and references` (`Proposed module` for Python). Core keeps its separate Basic-eligibility column and fixed order. Put filenames in code spans; keep implementation status separate from planned features, including when a whole section is planned. Support and resource indexes may use columns appropriate to their purpose. Use one document title, sequential heading levels, blank lines around headings/lists/tables/fences, and `|---|` table separators; preserve numeric alignment where useful. Notebook notes may retain the multiple `#`/`##` print sections supported by their renderer. Wrap local Markdown link destinations containing spaces in angle brackets or encode the spaces.

Topic-local `97-Legacy/` folders may hold unchanged `.cpp` reference excerpts with an index and source-range/hash provenance. They are excluded from aggregates and notebook discovery; they are not active implementations or a new algorithm tier. Preserve their original bugs/APIs until a specifically scoped implementation task audits them.

Archive superseded implementation/notebook copies and original monoliths in OLD. OLD is excluded from active aggregates, discovery and notebook selection. Keep original source bytes in the archive so behavior and lost features can be compared. Existing active legacy code can carry a migration gap; do not call it guideline-compliant until audited.

All-solution APIs return compact parameterizations, generators or explicitly bounded enumeration for infinite/huge answer sets. Charge explicit enumeration by output size; do not imply that an exponential-size witness list is generated within a polynomial bound.
