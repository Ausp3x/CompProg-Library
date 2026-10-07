# Guidelines

Rules `01`–`08` are loaded by Claude Code automatically through `.claude/rules/` symlinks: `01-principles.md` in every session, the others when a matching file is opened. Other agents read them by hand following `AGENTS.md`.

| File | Loads for | Content |
|---|---|---|
| [01-principles](01-principles.md) | always | Four requirements, profiles, evidence, scope discipline |
| [02-structure](02-structure.md) | inventories, aggregates, plan | Layout, naming, stable prefixes, tiers, aggregates, inventory row format |
| [03-cpp](03-cpp.md) | `.hpp`, `.cpp`, C++ test scripts | Toolchain and judges, headers, types, contracts, style, comments, complexity comments |
| [04-python](04-python.md) | `08-Python` | Baseline, PyPy idioms, style, contracts |
| [05-testing](05-testing.md) | `96-Local Testing` | Suites, modes, oracles, case classes, builds, benchmarks |
| [06-online](06-online.md) | `97-Online Testing`, `99-Workspace` | Judge sources, expander, generated workspace template |
| [07-notebook](07-notebook.md) | `98-Team Notebook` | Notebook selection, rendering, notes |
| [08-contest-tools](08-contest-tools.md) | `09-Contest Testing` | Stress, interactive, scored and shrinking tools |
| [09-sources](09-sources.md) | on request | Completeness sweep catalogs, provenance rules |
| [10-decisions](10-decisions.md) | on request | Decision log and rationale |
| [13-Plan](13-Plan/00-index.md) | via `plan.py` | Work manifest: batches, packages, statuses, models |
| [23-Reaudit Findings](<23-Reaudit Findings/00-index.md>) | via package notes | Confirmed `/reaudit-review` defects per package; each file is deleted once its re-audit fixes it |
| [History](History/2026-09-27-migration.md) | on request | Dated migration and audit records: migration, monolith transfer, inventory audit, Core migration |
| `Ledgers/` | never whole | Machine-readable maps checked by the validator: `path-map`, `monolith-map`, `research-sources`, `path-updates`, `archive-map`, `library-checker-coverage`, `core-path-updates`; query with `rg` or Python |
