---
name: restyle
description: Mechanical restyle of a verified package to the current 03-cpp.md comment cap and closing-brace rule. Moves contract comments into the evidence document; no behavior, API or test changes.
disable-model-invocation: true
argument-hint: "Pxxx"
---

## Package brief

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-Plan/plan.py" show $ARGUMENTS`

## Current violations

!`cd "${CLAUDE_PROJECT_DIR}" && python3 "00-Guidelines/13-Plan/plan.py" show $ARGUMENTS | grep -oE '`[^`]+\.hpp`' | tr -d '`' | sort -u | xargs -d '\n' python3 "96-Local Testing/03-consistency.py" --braces 2>/dev/null || true`

## Procedure

This is a text-moving pass. Behavior, public names, domains, tests and benchmarks do not change. Run on any model.

1. For each target header, read it once. Above each struct and free function keep only the complexity line and the domain/no-answer line `03-cpp.md`, Comments, allows; keep an in-body comment only if it is one line naming a non-obvious trick; delete restated contracts, history and cross-references.
2. Move every removed sentence that states a contract (domain, exactness, mutation, setup or reset, aliasing, staleness or invalidation, sentinel policy, precondition) into the package's evidence document under `## Contracts`, one `###` subsection per struct or free function, as compact prose; drop sentences that only repeat the code. Do not create a new evidence document if the brief links one. Apply the latest-run rule (`02-structure.md`, Names).
3. Fix any closing-brace violations listed above in the same files, touching nothing else.
4. Verify: `python3 '96-Local Testing/03-consistency.py' --braces <each header>` must print no violations; run the package's tester in `full` mode (locally and with `CXX=g++-14`) and `python3 '96-Local Testing/02-integration.py'`; then `python3 '96-Local Testing/03-consistency.py'` must report no errors. If the Core template or debug header changed, run `python3 '97-Online Testing/03-workspace.py'` then `--check`.
5. `python3 '00-Guidelines/13-Plan/plan.py' set Pxxx verified --note ""` and report: files touched, lines of comments removed per header, where the contracts went, and the test commands with their results. End with `git add -A && git commit -m "Pxxx: restyle"` for the user to confirm.
