# Guidelines

Rules `01`–`08` are loaded by Claude Code automatically through `.claude/rules/` symlinks: `01-principles.md` in every session, the others when a matching file is opened. Other agents read them by hand following `AGENTS.md`.

| File | Loads for | Content |
|---|---|---|
| [01-principles](01-principles.md) | always | Four requirements, profiles, evidence, scope discipline |
| [02-structure](02-structure.md) | inventories, evidence documents, aggregates, plan | Layout, naming, stable prefixes, tiers, aggregates, inventory row format, archive |
| [03-cpp](03-cpp.md) | `.hpp`, `.cpp`, C++ test scripts | Toolchain and judges, headers, types, contracts, one API per operation, style, comments, complexity comments |
| [04-python](04-python.md) | `08-Python` | Baseline, PyPy idioms, style, contracts |
| [05-testing](05-testing.md) | `96-Local Testing` | Suites, modes, memory safety, oracles, case classes, builds, benchmarks |
| [06-online](06-online.md) | `97-Online Testing`, `99-Workspace` | Judge sources, expander, generated workspace template |
| [07-notebook](07-notebook.md) | `98-Team Notebook` | Notebook selection, rendering, notes |
| [08-contest-tools](08-contest-tools.md) | `09-Contest Testing` | Stress, interactive, scored and shrinking tools |
| [09-sources](09-sources.md) | on request | Completeness sweep catalogs, provenance rules |
| [10-decisions](10-decisions.md) | on request | Decision log and rationale |
| [13-Plan](13-Plan/00-index.md) | via `plan.py` | Work manifest: batches, packages, statuses, models; worktree and context-usage tools |
| [23-Sweep Ledgers](<23-Sweep Ledgers/00-index.md>) | via skills | Sweep ledgers with the user's binding decisions: `duplicates.md` (`/duplicate-sweep`, `/prune`), `omissions.md` (`/omission-sweep`) |
| [History](History/2026-09-27-migration.md) | on request | Dated migration and audit records: migration, monolith transfer, inventory audit, Core migration |
| `Ledgers/` | never whole | Machine-readable maps `path-map`, `monolith-map`, `path-updates`, `core-path-updates`, `archive-map` (archive accounting), `library-checker-coverage` (judge families to owners), all checked by the validator, and the unchecked `research-sources` (read scopes); query with `rg` or Python |
