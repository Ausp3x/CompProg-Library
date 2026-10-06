# Guidelines

Rules `01`–`08` are loaded by Claude Code automatically through `.claude/rules/` symlinks: `01-principles.md` in every session, the others when a matching file is opened. Other agents read them by hand following `AGENTS.md`.

| File | Loads for | Content |
|---|---|---|
| [01-principles](01-principles.md) | always | Four requirements, profiles, evidence, scope discipline |
| [02-structure](02-structure.md) | inventories, aggregates, plan | Layout, naming, stable prefixes, tiers, aggregates, inventory row format |
| [03-cpp](03-cpp.md) | `.hpp`, `.cpp`, C++ test scripts | Toolchain and judges, headers, types, contracts, style, complexity comments |
| [04-python](04-python.md) | `08-Python` | Baseline, PyPy idioms, style, contracts |
| [05-testing](05-testing.md) | `96-Local Testing` | Suites, modes, oracles, case classes, builds, benchmarks |
| [06-online](06-online.md) | `97-Online Testing`, `99-Workspace` | Judge sources, expander, generated workspace template |
| [07-notebook](07-notebook.md) | `98-Team Notebook` | Notebook selection, rendering, notes |
| [08-contest-tools](08-contest-tools.md) | `09-Contest Testing` | Stress, interactive, scored and shrinking tools |
| [09-sources](09-sources.md) | on request | Completeness sweep catalogs, provenance rules |
| [10-decisions](10-decisions.md) | on request | Decision log and rationale |
| [11-migration](11-migration.md), [14-monolith-transfer](14-monolith-transfer.md), [16-inventory-audit](16-inventory-audit.md), [22-core-migration](22-core-migration.md) | on request | Historical migration and audit records |
| [13-plan](13-plan/00-index.md) | via `plan.py` | Work manifest: batches, packages, statuses, models |
| [23-reaudit-findings](23-reaudit-findings/00-index.md) | via package notes | Confirmed `/reaudit-review` defects per Foundations package; each file is deleted once its package re-audit fixes it |

Machine-readable ledgers (never read whole; query with `rg` or Python): `12-path-map.json`, `15-monolith-map.json`, `17-research-sources.json`, `18-path-updates.json`, `19-archive-map.json`, `20-library-checker-coverage.json`, `21-core-path-updates.json`. The consistency validator checks them.
