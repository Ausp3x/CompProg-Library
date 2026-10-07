# Work plan

The plan is one compact manifest plus a CLI. Batches are stable ownership units; packages are assignable sessions that run batches in order. The rendered checklist lives in [01-prompts.md](../../01-prompts.md).

| File | Contents |
|---|---|
| [batches.json](batches.json) | Each batch's folder, owned targets, support targets, size, focus, prerequisite batches and tiers, including support passes SUP01–SUP05. |
| [packages.json](packages.json) | Packages P001–Pn in default order: label, batches, phase, size, model, status, evidence and note. Package prerequisites are derived from batch prerequisites. |
| [plan.py](plan.py) | Standard-library CLI; runs from any directory. |

Commands (`python3 '00-Guidelines/13-plan/plan.py' <command>`):

- `show Pxxx` or `show <batch>`: compact brief with prerequisites, focus, inventory rows, tests, legacy references and evidence.
- `status`: counts per status and phase, plus the next ready package; `next` prints only its ID.
- `set Pxxx STATUS [--evidence PATH ...] [--note TEXT]`: update a package (evidence is appended) and re-render the checklist.
- `render`: rewrite the checklist between the `plan:begin`/`plan:end` markers in `01-prompts.md`.
- `check`: manifest, schedule, ownership, inventory and checklist-sync checks; `96-Local Testing/03-consistency.py` runs the same `check(root)`.
- `ready [--max K]`: every package whose prerequisites are satisfied, plus a disjoint set of K packages from different folders for parallel worktree sessions.
- `doctor [--fix]`: cross-checks plan, inventory rows, files on disk, testers and evidence, grouped by package with the command that resolves each item; `--fix` re-renders the checklist.

Statuses: planned, in-progress, audit (verified under the previous system; re-audit required), verified. Mark `verified` only with recorded evidence.
