# Support and integration batches

These passes are outside the `01`–`08` numbered algorithm coverage and should follow the implementation they verify or package. Their owner may be the integration owner. Each reads its own folder index and applicable guide; none grants verified status to algorithm entries without algorithm-specific evidence.

| ID | Area | Scope |
|---|---|---|
| SUP01 | `09-Contest Testing/` | Complete or harden stress, quick, interactive, scored and shrinking protocols and their fixtures only when a concrete gap is found; see [contest tools](../08-contest-tools.md). |
| SUP02 | `96-Local Testing/` | Add missing per-header/module suites and representative benchmarks as implementation batches land; keep the runner and standalone/multiple-TU/aggregate/sanitizer checks current; see [testing](../05-testing.md). |
| SUP03 | `97-Online Testing/` | Expand maintained judge solutions and exercise the C++ expander after canonical APIs stabilize; record local run versus genuine acceptance separately; see [online/workspace](../06-online.md). No automatic submissions. |
| SUP04 | `98-Team Notebook/` | Update selection, notes and printable source order after canonical file moves; validate generation/build/page evidence when tools are available; see [notebook](../07-notebook.md). |
| SUP05 | `99-Workspace/template.cpp` | Refresh the explicit generated contest snapshot after template/debug changes, then run `--check`; see [online/workspace](../06-online.md). |

Per-algorithm tests belong in the corresponding implementation batch; SUP02 is maintenance/integration work and is not permission to postpone those tests. Read [the common prompt](../../01-prompts.md) and the relevant support-folder index.
