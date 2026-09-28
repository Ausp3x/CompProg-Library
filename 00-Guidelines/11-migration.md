# Migration and verification status — 2026-09-27

This is an implementation-status record, separate from the agreed design in [10-decisions.md](10-decisions.md). The later [monolith transfer](14-monolith-transfer.md) updates the active-header count and records unchanged excerpts; the initial-pass checks below remain historical evidence for that pass. Inventories are future-work roadmaps, not declarations that the ultra library is already complete.

## What this pass establishes

- Replaced the active one-off prompts.txt instructions with a small AGENTS.md routing entry and focused Markdown guides. The original prompt is retained in OLD.
- Created all approved numbered categories and concise feature inventories for 01–08, with Basic/Advanced/Esoteric sections outside Core and the fixed Core order. Empty categories have honest include-only aggregates; Python aggregates currently export nothing.
- Preserved every original library/material file in OLD. Original notebook code/PDFs, monoliths, old online outputs and raw test binaries remain archival, not active dependencies. No legacy data is intentionally deleted.
- Migrated existing Core headers to fixed numbers; added the old scalar Montgomery header at Core04 as existing-unverified. Renamed the mini big-integer alias to `iintmini` so full and mini coexist.
- Migrated coherent ordinary math headers, split segmented sieve mechanically from the two basic sieve structs, and placed FastConv's OR/AND/XOR/GCD/LCM transforms in Mathematics. FastConv is not fast I/O. Mixed legacy number-theory headers and older Poly/Moly implementations remain in OLD for future coherent extraction/consolidation; their feature requirements remain inventoried.
- Updated includes, mirrored six existing per-header Python suites and supporting C++ checks, and migrated 20 Yosupo sources. Online problem acceptance has not been re-established or claimed.
- Added full/batch-quick contest runners, generator/checker/interactor/validator/scorer templates and an optional problem-specific shrinker. The complete runner covers batch, interactive and scored tasks; no parallel cases.
- Added recursive oj-bundle expansion with protected manifest ownership, clean-by-default/--noclean semantics, source/output separation and failure preservation. Python bundling is explicitly not implemented.
- Added the configurable cyan notebook browser/CLI, ordered canonical-source selection, Inter/JetBrains Mono settings, notes, LuaLaTeX generation and page-budget reporting.
- Generated the standalone single-case Workspace template explicitly from current template/debug headers. No ordinary build/test regenerates it.

## Small integration fixes made while moving code

These are structural/build changes, not a claim to have performed the requested future maximal algorithm audits:

- Distinct full/mini aliases and corresponding test/judge uses.
- Direct LinearSieve dependency added to FastConv.
- Inline linkage added to four ordinary free math functions and four AVX2 Matrix namespace functions for multiple-translation-unit safety.
- Dynamic-modint test's mixed `unsigned long`/`unsigned long long` initializer list given an explicit element type for GNU/Linux fixed-width aliases.
- Test paths updated; big-integer differential seeds made configurable, and unavailable AVX2 execution explicitly skipped.

## Verification record

The migration was prepared and checked in an isolated staging copy before application. Compiler used here: GCC16; Python used here: CPython3.14. Python sources additionally pass parsing under Python3.10 grammar. The declared GCC14/Python3.10 floors and PyPy compatibility still need execution on those exact runtimes; passing on a newer version is not proof of those compatibility targets.

- Template/debug/modint/dynmodint migrated optimized/checked suites pass.
- Full InfInt passes scalar and AVX2 self-tests and more than 20,000 independent Python differential cases per configuration; Mini passes optimized/debug-iterator self-tests and about 15,000 differential cases. Both original seeds and seed42 were exercised.
- All current standalone/aggregate headers compile. Combined scalar and AVX2 multiple-translation-unit linkage is checked by 96's integration runner.
- Real oj-bundle 5.6.0 expanded all 20 sources successfully using a temporary isolated dependency environment. The permanent dependency is documented in the online index; it is not silently installed globally.
- Tool fixture checks cover batch pass/mismatch/crash/timeouts, reference/checker/scorer errors, mixed languages/space-containing paths, failure replay, interactive pipes/rejection/idle termination/transcripts, score gaps and shrink validity/signatures.
- Expander fixture checks cover all-source bundling failures, output preservation, clean/noclean, ownership hashes, unsafe paths/symlinks and source preservation.
- Notebook config/path/ordering/discovery/TeX fixtures pass. A live loopback HTTP GET/POST test verified ordered selections, persistence and one-column generation. JavaScript syntax was checked.
- Workspace generation/check and GNU++20 LOCAL/non-LOCAL compilation pass.

Final checks: all **31** current headers/aggregates passed standalone compilation; scalar and AVX2 combined headers linked across two translation units. ASan/UBSan self-tests passed outside the sandbox (LeakSanitizer cannot run under the sandbox process tracer). All **40** online source/expanded translation units passed GNU++20 syntax compilation. Three persistent tool regression entries are included under 96/00-Tools.

## Remaining work and explicit limits

- Most inventoried algorithms are not yet implemented. Existing code retains legacy API/style/complexity details pending per-header audits. Do not interpret the reorganization as maximal optimization, complete semantics review, or research verification.
- Complete feature suites are still missing for Matrix, Montgomery, ordinary Mathematics and miscellaneous utilities. Available-suite modes describe actual coverage in 96/00-index.md; they do not cover nonexistent suites.
- No performance superiority benchmarks or Barrett/Montgomery >=20% claims were established by this organizational pass.
- LuaLaTeX and Inter are unavailable in this environment. The notebook selector and generated TeX were checked, but actual PDF rendering, glyph coverage (including Unicode comments), and page count against real PDFs remain unverified. The build command reports missing prerequisites and does not claim a PDF was built.
- PyPy and GCC14 were unavailable for exact-baseline execution. No online submissions were made.
- The current session's original Workspace directory may retain protected .git/.agents/.codex scaffolding. Contest content belongs in 99-Workspace; the session scaffolding is not a second contest workspace and was not edited.

See [path map](12-path-map.json) for active header paths and migrated online sources. OLD retains the original byte-for-byte sources for comparisons. The source list contains research candidates explicitly distinguished from references actually inspected.

Later inventory audit: [16-inventory-audit.md](16-inventory-audit.md) records the expanded scope/current counts; [18-path-updates.json](18-path-updates.json) resolves later prefix changes. Counts and placement descriptions above are historical snapshots. The live monolith map uses current target paths while preserving original source/body hashes.

The later [Core modular-family migration](22-core-migration.md) is authoritative for the current Core order, package ownership and migration verification. Original pass counts and unfinished items above describe that earlier snapshot; current per-family inventories and evidence track subsequently completed work.
