# Ultra competitive programming library

Start with [CLAUDE.md](CLAUDE.md) (Claude Code) or [AGENTS.md](AGENTS.md) (other agents), then the [guideline index](00-Guidelines/00-index.md). Work is scheduled in the [session checklist](01-prompts.md), rendered from the plan manifest in `00-Guidelines/13-plan/`. The [decision log](00-Guidelines/10-decisions.md) is on demand only.

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

Each algorithm folder has a `00-index.md` inventory whose rows name every public operation and carry one status word. Planned rows are not implementations. Start work with `/package Pxxx`; [migration notes](00-Guidelines/11-migration.md) record the history of existing code.
