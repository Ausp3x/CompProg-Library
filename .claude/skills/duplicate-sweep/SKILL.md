---
name: duplicate-sweep
description: Read-only line-by-line sweep for wrappers and second implementations (03-cpp.md, One API per operation) with adversarial refutation. Writes the confirmed per-package list to 00-Guidelines/23-Sweep Ledgers/duplicates.md for /prune.
disable-model-invocation: true
argument-hint: "[folder prefixes, e.g. 02 04] (default: 01 to 08)"
model: claude-opus-5-5
effort: xhigh
---

Scope: the folders whose two-digit prefixes are in `$ARGUMENTS`; empty means `01-Core` through `08-Python`. Sub-agents edit nothing; this session writes only `00-Guidelines/23-Sweep Ledgers/duplicates.md`.

1. **Rule and decisions.** Read `00-Guidelines/03-cpp.md`, One API per operation, and all of `duplicates.md`, whose recorded decisions bind this sweep. Revalidate the user's Keeps, the "Kept by the sweep" entries and the package-section rows in scope against the current code: a row still a wrapper or second implementation stays `remove`, one with a keep reason found becomes `ask`; a kept entry the code contradicts goes to the report.
2. **Files.** List every header and `_NN_*.py` module in scope with its owning package, its status and whether the file exists yet, and every unowned file:
   `python3 -c 'import json,sys,pathlib;P=json.load(open("00-Guidelines/13-Plan/packages.json"))["packages"];B={b["id"]:b for b in json.load(open("00-Guidelines/13-Plan/batches.json"))["batches"]};S=sys.argv[1:] or ["%02d"%i for i in range(1,9)];O={t:p for p in P for b in p["batches"] for t in B[b]["targets"]+B[b].get("support_targets",[])};[print(t,O[t]["id"],O[t]["status"],"file" if pathlib.Path(t).exists() else "row only",sep="\t") for t in O if t[:2] in S];[print(f,"NO OWNER",sep="\t") for d in S for f in map(str,sorted(pathlib.Path().glob(d+"-*/*"))) if f.endswith((".hpp",".py")) and f not in O and f.split("/")[1].lstrip("_")[:2] not in ("98","99")]' $ARGUMENTS`
   A `row only` target has no code yet: check its inventory entries only. Split the rest into chunks of at most 12 files from one folder.
3. **Candidates.** Per chunk, one read-only `general-purpose` agent, at most 8 at a time, given the chunk, the rule text and the recorded decisions. It reads every file line by line (keyword search is not enough), each file's evidence document and the folder's `Docs/00-notes.md`, and tests every public operation and every inventory entry against the rule. Per candidate it returns: file:line, symbol, what the code does, what already provides it, the form or clause, the proposed decision (`remove`, `keep` with its keep reason, or `ask`), the tests that exercise it (file:lines, probe names), its dependents (`rg` outside `OLD/` and `97-Legacy/`), and the owning package with its status. Under 900 words; no edits.
4. **Refute.** For every proposed `remove`, an independent read-only agent (several items each, at most 8 at a time) reads both codes and looks for a keep reason: found, `keep` (or `ask` if uncertain); not found, `remove`. Spot-check proposed `keep` and `ask` items as well.
5. **Write.** Keep "Recorded decisions" and "Conventions" verbatim; each new `keep` joins "Kept by the sweep" (package, item with its header, reason). Replace the package sections in scope with one `## Pxxx — <label> (<status>)` section per package that has non-`keep` rows, in package order, each a table `| Item | Location | Provided by | Decision | Reason | Tests to move | Dependents |`. Decisions: `remove`, `remove (row only)` (pinned extraction or unimplemented row), `ask`. List every `ask` under `## Open questions` with the fact the user must decide.
6. **Report** counts by decision and folder, every `ask`, any recorded decision the code contradicts, and the `/prune` order: a package whose recorded change others rely on first, a package whose removal edits another package's file before that package, Core packages on Fable at max effort. Give the one-file commit command.

Record finished chunks in a temporary `## Sweep progress` section of `duplicates.md` as they finish, resume from it after a compaction (`CLAUDE.md`, Compact instructions), and delete the section when the list is complete.
