---
name: restyle
description: Mechanical restyle of a verified package to the current 03-cpp.md comment cap and closing-brace rule. Moves contract comments into the evidence document; no behavior, API or test changes.
disable-model-invocation: true
argument-hint: "Pxxx"
---

## Package brief

!`python3 "${CLAUDE_PROJECT_DIR}/00-Guidelines/13-plan/plan.py" show $ARGUMENTS`

## Current violations

!`cd "${CLAUDE_PROJECT_DIR}" && python3 "00-Guidelines/13-plan/plan.py" show $ARGUMENTS | grep -oE '`[^`]+\.hpp`' | tr -d '`' | sort -u | xargs -d '\n' python3 "96-Local Testing/03-consistency.py" --braces 2>/dev/null || true`

## Procedure

This is a text-moving pass. Behavior, public names, domains, tests and benchmarks do not change. Run on any model.

1. For each target header, read it once. For every struct and free function keep exactly the two lines `03-cpp.md` allows: the complexity line and one line with the domain and the no-answer representation. Keep an in-body comment only if it is one line and names a trick that is not evident from the code; delete restated contracts, history and cross-references.
2. Move every removed sentence that states a contract (domain, exactness, mutation, setup or reset, aliasing, staleness or invalidation, sentinel policy, precondition) into the package's evidence document under a `## Contracts` heading, one `###` subsection per struct or free function, as compact prose. Drop sentences that only repeated what the code shows. Do not create a new evidence document if one is linked in the brief.
3. Fix any closing-brace violations listed above in the same files, touching nothing else.
4. Verify: `python3 '96-Local Testing/03-consistency.py' --braces <each header>` must print no violations; run the package's tester in `full` mode and `python3 '96-Local Testing/02-integration.py'`; then `python3 '96-Local Testing/03-consistency.py'` must report no errors. If the Core template or debug header changed, run `python3 '97-Online Testing/03-workspace.py'` then `--check`.
5. `python3 '00-Guidelines/13-plan/plan.py' set Pxxx verified --note ""` and report: files touched, lines of comments removed per header, where the contracts went, and the test commands with their results.
