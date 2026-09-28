# Library work: read only the relevant guides

Read `00-Guidelines/01-principles.md` first, then the affected folder's `00-index.md` and the applicable guide below. Do not load all guides, inventories, sources, or the decision log by default.

| Task | Additional guide |
|---|---|
| Add, rename, move, classify, aggregate | `00-Guidelines/02-structure.md` |
| C++ implementation or review | `00-Guidelines/03-cpp.md` |
| Python algorithm implementation or review | `00-Guidelines/04-python.md` |
| Library tests or benchmarks | `00-Guidelines/05-testing.md` |
| Online solutions, expansion, workspace template | `00-Guidelines/06-online.md` |
| Notebook or contest notes | `00-Guidelines/07-notebook.md` |
| In-contest stress, interactive, scored, shrinking tools | `00-Guidelines/08-contest-tools.md` |
| Research, feature completeness, provenance | `00-Guidelines/09-sources.md` |
| Choose an implementation package/batch | `01-prompts.md`, then only the selected table in `00-Guidelines/13-Work Batches/` |
| Recover this planning conversation | `00-Guidelines/10-decisions.md` (on demand only) |

Every implementation must satisfy Optimality, Correctness, Completeness, and Elegance. Existing code is reference, not proof of compliance. Inventory status distinguishes planned, existing-unverified, and verified work. Do not implement the entire backlog merely because it is listed. Keep changes within the requested task; update its inventory, tests, and dependent paths together. Preserve legacy material in `OLD` until superseded features are accounted for. No automatic online submissions.

For a `Pxxx` package, locate only that checklist line at the bottom of `01-prompts.md` (for example with `rg`), read the shared workflow above the checklist, and open its selected batch rows. Do not load the entire checklist, machine maps or research audit by default. Research ledgers and archived source maps are optional lookup evidence, not extra global instructions.
