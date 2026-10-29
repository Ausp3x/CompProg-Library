# Work plan

The plan is one compact manifest plus CLIs. Batches are stable ownership units; packages are assignable sessions that run batches in order. The rendered checklist lives in [01-prompts.md](../../01-prompts.md).

| File | Contents |
|---|---|
| [batches.json](batches.json) | Each batch's folder, owned targets, support targets, size, focus, prerequisite batches and tiers, including support passes SUP01–SUP19. |
| [packages.json](packages.json) | Packages P001–Pn in schedule order: label, batches, phase, size, model, status, evidence and note. Package prerequisites are derived from batch prerequisites. |
| [plan.py](plan.py) | Standard-library CLI; runs from any directory. |
| [worktree.py](worktree.py) | Package worktrees: report (default), `--sync` (`--integration` also runs the integration build), `--finish` (sync, fast-forward `master`, remove) and `--remove`; all but the report run from the main checkout. |
| [finish-log.tsv](finish-log.tsv) | One line per `/finish` landing, with the columns its step 7 defines; merged as a union (`.gitattributes`). A support batch, having no inventory rows, is done once a line with its key and the result `verified` exists (user decision 2026-10-29). |
| [Docs/](Docs/00-contribution.md) | Support-pass evidence for the plan tooling: [00-contribution.md](Docs/00-contribution.md) (roles and claims). |
| [context_usage.py](context_usage.py) | Token and context usage of past sessions from the local transcripts, with an auto-compact window replay (`--windows`) and, with `--split`, calls by model and effort beside `finish-log.tsv`. |

Commands (`python3 '00-Guidelines/13-Plan/plan.py' <command>`):

- `show Pxxx`, `show next` or `show <batch>`: compact brief with the launch command (and a session MISMATCH flag), prerequisites, focus, inventory rows, tests, legacy references and evidence; a package id with several open batches names the batch to run instead. Its second line is the claims line, followed by whether this clone may start the package (Claims, below).
- `status`: counts per status and phase, the claims line and the next ready package that is not blocked (Claims, below); `next` prints only its ID.
- `set Pxxx STATUS [--evidence PATH ...] [--note TEXT]`: update a package (evidence is appended), re-rank the schedule as `rank` does, and re-render the checklist.
- `render`: rewrite the checklist between the `plan:begin`/`plan:end` markers in `01-prompts.md`; a verified package shows only its label.
- `rank`: reorder the schedule and render. Phases keep their order; in each, started packages keep theirs, then planned ones go support passes first, then by most basic tier (Basic or Core, Advanced, Esoteric), then by most transitive open dependents, then by current position, each after its prerequisites unless one of the two is verified.
- `check`: manifest, schedule, ownership, inventory and checklist-sync checks, and that every `#include` between headers of different batches is a direct batch prerequisite (the graph comes from [05-dependencies.py](<../../98-Team Notebook/05-dependencies.py>)); batch prerequisites form no cycle; no quoted include sits inside an `#if` block; and each open package comes after its open prerequisites; `96-Local Testing/03-consistency.py` runs the same `check(root)`.
- `ready [--max K]`: every open package whose prerequisites are satisfied, plus a disjoint set of K packages from different folders for parallel worktree sessions. A package is live when a linked worktree's branch or directory name names it or one of its batches, or when the main checkout has uncommitted changes to its targets, tests, docs or evidence; live packages are skipped and reserve their folders, and a multi-batch package is offered as `/package <next open batch>`. Blocked and claimed packages are marked and skipped (Claims, below).
- Claims (`show`, `status`, `next`, `ready`): roles and claims follow [CLAUDE.md](../../CLAUDE.md), Git; the exact contracts (claim branch, claims line, blocked packages, `gh` lookup) are in [00-contribution.md](Docs/00-contribution.md).
- `doctor [--fix]`: cross-checks plan, inventory rows, files on disk, testers and evidence, grouped by package with the command that resolves each item; `--fix` adds the missing include-derived batch prerequisites and re-renders the checklist.

Statuses: planned, in-progress, verified; `verified` follows `01-principles.md`, Evidence.
