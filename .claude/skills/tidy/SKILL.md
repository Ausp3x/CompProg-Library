---
name: tidy
description: Documentation and configuration hygiene. Read-only audits per scope unit, then fixes backed by a rule (stale or contradicted claims, dead references, restated rules, superseded run narrative, contract violations); judgement calls go to the user. Code and tests are out of scope.
disable-model-invocation: true
argument-hint: "[.claude | 00-Guidelines | folder prefixes | evidence | tools] (default: all)"
model: claude-opus-5-5
effort: high
---

Scope units: `.claude` with `CLAUDE.md`, `AGENTS.md`, the root `00-index.md`, `01-prompts.md` above the plan block and the auto-memory; `00-Guidelines` with its ledgers; each library folder (`00-index.md`, `Docs/00-notes.md`, `Docs/00-sources.md`, `97-Legacy/00-index.md`); `evidence`, the `Docs/<NN-name>.md` documents, one unit per folder; `tools`, the indexes and documents of `09`, `95`–`98` and `OLD/00-index.md`.

1. **Audit.** One read-only `general-purpose` agent per unit, at most 8 at a time. It checks the unit against `01-principles.md` (each rule stated once, in its owning file), `02-structure.md` (inventory contract, companion documents, latest-run rule) and the repository (paths, rows, statuses, files on disk), and returns file:line, class, evidence and the minimal fix, confirmed findings apart from judgement calls. Omission lists in `Docs/00-notes.md` and dated source sections are intended (`/package` step 3, `09-sources.md`).
2. **Ask** the judgement calls with AskUserQuestion.
3. **Fix** the confirmed findings and the answered calls, one editing agent per unit on disjoint files. Inventory operations are never removed (that is `/prune` or the user); vague ones go to `/omission-sweep`. A contract, decision rationale or omission reason moves to its owning file instead of disappearing, and a deleted run narrative must already have its `## History` line (`02-structure.md`, Names). `OLD/` and `97-Legacy/` follow `01-principles.md`, Scope discipline; editing `OLD/00-index.md` also updates its record in `00-Guidelines/Ledgers/archive-map.json`.
4. **Verify.** An independent read-only agent reviews `git diff` for lost contracts, decisions or reasons and for new contradictions; `plan.py check` and `96-Local Testing/03-consistency.py` print no errors.
5. **Report** per unit the fixes and words removed, the open items, and `git add <every touched path> && git commit -m "Tidy <scope>"`.
