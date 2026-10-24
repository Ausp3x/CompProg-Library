# Work plan

The plan is one compact manifest plus CLIs. Batches are stable ownership units; packages are assignable sessions that run batches in order. The rendered checklist lives in [01-prompts.md](../../01-prompts.md).

| File | Contents |
|---|---|
| [batches.json](batches.json) | Each batch's folder, owned targets, support targets, size, focus, prerequisite batches and tiers, including support passes SUP01–SUP15. |
| [packages.json](packages.json) | Packages P001–Pn in schedule order: label, batches, phase, size, model, status, evidence and note. Package prerequisites are derived from batch prerequisites. |
| [plan.py](plan.py) | Standard-library CLI; runs from any directory. |
| [worktree.py](worktree.py) | Package worktrees: report (default), `--sync` (`--integration` also runs the integration build), `--finish` (sync, fast-forward `master`, remove) and `--remove`; all but the report run from the main checkout. |
| [finish-log.tsv](finish-log.tsv) | One line per `/finish` landing: date, key, session model and effort, confirmed findings of its review, defects fixed, decisions asked, result. Merged as a union (`.gitattributes`). |
| [context_usage.py](context_usage.py) | Token and context usage of past sessions from the local transcripts, with an auto-compact window replay (`--windows`) and, with `--split`, calls by model and effort beside `finish-log.tsv`. |

Commands (`python3 '00-Guidelines/13-Plan/plan.py' <command>`):

- `show Pxxx`, `show next` or `show <batch>`: compact brief with the launch command (and a session MISMATCH flag), prerequisites, focus, inventory rows, tests, legacy references and evidence; a package with several batches shows only its next open batch.
- `status`: counts per status and phase, plus the next ready package; `next` prints only its ID.
- `set Pxxx STATUS [--evidence PATH ...] [--note TEXT]`: update a package (evidence is appended) and re-render the checklist.
- `render`: rewrite the checklist between the `plan:begin`/`plan:end` markers in `01-prompts.md`; a verified package shows only its label.
- `rank`: reorder the schedule and render. Phases keep their order; in each, started packages keep theirs, then planned ones go support passes first, then by most basic tier (Basic or Core, Advanced, Esoteric), then by most transitive open dependents, then by current position, each after its prerequisites.
- `check`: manifest, schedule, ownership, inventory and checklist-sync checks; `96-Local Testing/03-consistency.py` runs the same `check(root)`.
- `ready [--max K]`: every package whose prerequisites are satisfied, plus a disjoint set of K packages from different folders for parallel worktree sessions.
- `doctor [--fix]`: cross-checks plan, inventory rows, files on disk, testers and evidence, grouped by package with the command that resolves each item; `--fix` re-renders the checklist.

Statuses: planned, in-progress, verified; `verified` follows `01-principles.md`, Evidence.
