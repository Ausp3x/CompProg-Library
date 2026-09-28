# Ultra competitive programming library

Start with [agent routing](AGENTS.md) or the [guideline index](00-Guidelines/00-index.md). The [decision log](00-Guidelines/10-decisions.md) records the agreed design and rationale; it is not required reading for routine work. Use [implementation prompts and batch plans](01-prompts.md) to assign a focused task to a new session.

| Folder | Purpose |
|---|---|
| [00-Guidelines](00-Guidelines/00-index.md) | Focused standing instructions and decision history |
| [01-Core](01-Core/00-index.md) | Foundational full and mini types; full types receive extreme optimization |
| [02-Data Structures](<02-Data Structures/00-index.md>) | Contest data structures |
| [03-Geometry](03-Geometry/00-index.md) | Exact and approximate geometry |
| [04-Graphs](04-Graphs/00-index.md) | Graph and tree algorithms |
| [05-Mathematics](05-Mathematics/00-index.md) | Contest mathematics, including scalar FFT/NTT/FPS |
| [06-Miscellaneous](06-Miscellaneous/00-index.md) | General contest utilities and techniques |
| [07-Strings](07-Strings/00-index.md) | String algorithms |
| [08-Python](08-Python/00-index.md) | Deliberate Python subset |
| [09-Contest Testing](<09-Contest Testing/00-index.md>) | Small problem-specific batch/interactive/scored testing tools |
| [95-Resources](95-Resources/00-index.md) | Offline books, papers, team notebooks and lecture references |
| [96-Local Testing](<96-Local Testing/00-index.md>) | Maintained library regression suites |
| [97-Online Testing](<97-Online Testing/00-index.md>) | Judge solutions and generated submission files |
| [98-Team Notebook](<98-Team Notebook/00-index.md>) | Configurable canonical-source notebook and contest notes |
| [99-Workspace](99-Workspace/) | Standalone `template.cpp` and temporary contest work |
| [OLD](OLD/00-index.md) | Preserved legacy sources/materials; not active library dependencies |

Each algorithm folder has a `00-index.md` feature inventory. Planned entries are not implementations or claims of verification. Use [migration notes](00-Guidelines/11-migration.md) for the status of existing code and tools.

The [2026-09-27 research audit and follow-up review](00-Guidelines/16-inventory-audit.md) established the expanded scope; the [Core family migration](00-Guidelines/22-core-migration.md) now tracks **428 algorithm targets** and map all **250** Library Checker families in the recorded snapshot. The follow-up fills operation-level gaps, reconciles ownership/dependencies and standardizes the inventories without changing the **226 package IDs**. Start future implementation from a package checkbox at the bottom of [01-prompts.md](01-prompts.md); researched scope remains distinct from verified code.
