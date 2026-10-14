# Decision log

Why each settled decision was made, for a disputed interpretation. The rules live in the guides [00-index.md](00-index.md) lists; current state lives in [packages.json](13-Plan/packages.json), the folder inventories and their `Docs/`; dated records live in [History](History/2026-09-27-migration.md). Later explicit user instructions take precedence. Superseded and obsolete entries keep one dated line each.

## Foundations (2026-09-27)

- **Profiles.** Three profiles resolve the tension between a reusable high-performance numeric foundation and code that stays practical in contests: costly ISA specialization sits in full Core, where reuse justifies it, and dependent algorithms rely on its portable fallback; the mini profile serves print and copy limits without forcing every user onto a restricted interface ([01-principles.md](01-principles.md), Profiles).
- **Scoped completeness.** No folder promises every theoretical idea: inventories are living research roadmaps, and "complete" is scoped to the stated family and domain.
- **Evidence.** The verification rules make confidence inspectable by a later maintainer: a test names the contract it covers and an independent reason its expected result is trustworthy, and a green test is evidence about that coverage, not a correctness argument.
- **Archive.** `OLD` keeps hard-to-reconstruct edge behavior while the active layout keeps one canonical implementation per algorithm. Archiving is neither deletion nor endorsement, so new code is still checked against current contracts and independent references.
- **Qualified standard names.** The template's `using namespace std` is a convenience; qualifying every name outside the closed list in [03-cpp.md](03-cpp.md) keeps files consistent and makes less common standard facilities explicit.
- **Separate tools.** Online testing, the notebook and the workspace keep canonical source editing, reproducible submission generation and temporary contest use apart: generated outputs are disposable derivatives with explicit owners, user workspace files are not, and clean-on-success expansion never leaves a half-updated submission set.
- **Sources.** Research scope is explicit because inventories keep evolving: a ledger records what was consulted and which claim it supports, and a candidate list guides future work without implying that it was reviewed.
- **Excluded notebooks.** "Ateneo/ProgVar notebooks" came up in the first research discussion, but the final reference decision excluded the local `ProgVar.pdf` and `frankenstein.pdf` ([09-sources.md](09-sources.md)).
- **No versioning.** No versioning or semver requirement was agreed.

Superseded:

- 2026-09-27: the C++ baseline named Linux x86-64; since 2026-10-06 the judges, including Windows Codeforces, set it (below).
- 2026-09-27: Barrett/Montgomery outside Core kept a compact alternative where it grew code materially; since 2026-10-09 such a contest header keeps `Compact` twins ([03-cpp.md](03-cpp.md), Headers).
- 2026-09-27: the original 19-entry Core order with separate `modint64`, `dynmodint` and `dynmodint64` headers; replaced the same day (modular-family revision below).
- 2026-09-27: inventories recorded filenames, coverage, domains, tiers, references and omissions per family; since 2026-10-06 rows name operations and contracts live in companion documents.
- 2026-09-27: the workspace template was single-case with a commented multi-case alternative; since 2026-10-06 the user's multi-case template is canonical (below).

## Refinements and superseded proposals

- Extreme optimization, first proposed for every C++ algorithm, was narrowed to full Core; `02`–`07` have no handwritten ISA requirement.
- A proposed ban on namespaces and private code became: no `private` sections, visible state and helpers, small shared namespaces where useful.
- Expander cleanup, first proposed opt-in, was settled as clean by default with `--noclean` to retain.
- Python modules take `_01_name.py` because module names must be importable identifiers; executable scripts keep `01-name.py`.
- The original demand to prove every edge case became the practical evidence standard: correctness arguments, mapped coverage, independent oracles and visible limits.
- An early snippets-only scope was dropped for the organized library with its inventories, tests, online tools, notebook and workspace.

## Obsolete records (2026-09-27)

- Handoff and working preferences: the user authorized autonomous construction of the guides, layout, inventories and tooling, asked for careful verification over speed and simpler agents for simple work, and not to reopen settled choices without concrete conflicting evidence; routing now lives in `CLAUDE.md` and `AGENTS.md`.
- Resource collection: the user added `95-Resources`; it got a short index and a download manifest instead of a guide ([95-Resources/00-index.md](../95-Resources/00-index.md), which records reuse terms).
- Implementation prompts and unchanged extraction: work was split by size and dependency into batches and packages (now [13-Plan](13-Plan/00-index.md)), and the two legacy monoliths were transferred without algorithm changes, improvement being later work, into 26 headers and 14 `97-Legacy` excerpts ([monolith transfer](History/2026-09-27-monolith-transfer.md)).
- Final inventory research and assignment checklist: a deep audit grew the inventories from 274 to 430 targets in 226 packages ([inventory audit](History/2026-09-27-inventory-audit.md)).
- Modular-family layout revision: the user approved cohesive header ownership and four independently copyable modular minis, so the full modular types merged into `05-modint.hpp`, the minis went to `06-modintmini.hpp` and later Core prefixes shifted down two ([Core migration](History/2026-09-27-core-migration.md); current order in [02-structure.md](02-structure.md)).

## 2026-10-06 Claude Code retool

Recorded after a full stock-take of the library (five parallel audits of the inventories plus a tooling survey); the user confirmed each decision.

- **Entry points.** `CLAUDE.md` is canonical and `AGENTS.md` adapts it for other agents. Path-scoped guides, skills and subagents load only what a task needs, replacing the long reusable prompts; a post-edit hook runs the validator.
- **Plan manifest.** [13-Plan](13-Plan/00-index.md) replaced nine Markdown batch tables and two JSON schedules, so `01-prompts.md` is rendered from one source and prerequisites are derived, not stored.
- **Re-audit.** P001–P017, verified under the Codex workflow, were reset to `audit` for review under the current rules; retired 2026-10-14 (below).
- **Model split.** Core full types run on Fable and everything else on Opus, recorded per package in `packages.json`.
- **Scope and order.** The full inventory including the research tail is kept; the schedule runs by contest value with research families last.
- **Stable prefixes.** A prefix is assigned once, so a new row never renumbers existing files ([02-structure.md](02-structure.md), Names).
- **Inventory format.** Rows name every public operation so completeness is checkable row by row; contracts and sources moved to companion documents (layout 2026-10-08).
- **Completeness research.** Each folder had a full live sweep of the [09-sources.md](09-sources.md) catalogs ([inventory audit](History/2026-09-27-inventory-audit.md), function-level rewrite).
- **Closing braces.** The attached-brace rule was adopted and verified headers were normalized during re-audit ([03-cpp.md](03-cpp.md), Style).
- **Judges and floor.** The judges set the floor: Codeforces runs GCC 14.2 on Windows (MSYS2), which rules out `long` (32-bit under LLP64) and POSIX-only calls; AtCoder and Library Checker run GCC 15.2 on Linux ([03-cpp.md](03-cpp.md), Toolchain and judges).
- **Workspace template.** The user's hand-edited `99-Workspace/template.cpp` (no doc comments, multi-case `solve(int t)`) is canonical; the generator reproduces it.
- **Enforcement and presets.** Guard hooks protect `OLD/`, `97-Legacy/` and the generated template and stop whole reads of PDFs, JSON ledgers and large files. Presets fix model and effort per package: `core-session` (Fable, max) and `contest-session` (Opus, high, raised to xhigh on 2026-10-12). The `clangd-lsp` plugin with a root `.clangd` gives C++ diagnostics; superpowers, code-review, code-simplifier, commit-commands, hookify, context7 and ralph-loop were judged unnecessary.
- **Comment cap (2026-10-07).** In-code contract blocks were judged to ruin readability, so contracts moved to the evidence documents ([03-cpp.md](03-cpp.md), Comments).
- **Docs layout and floor compiler (2026-10-08).** Companion documents moved into per-folder `Docs/`, replacing `80-notes.md`, `81-sources.md` and per-package `pNNN-*.md` documents, with Title Case folders, lowercase files, JSON maps in `Ledgers/` and dated records in `History/` ([02-structure.md](02-structure.md)). Every package also runs its suites with `g++-14` as the floor check.
- **Type headers keep operations with the type (2026-10-09).** In the P025 Poly review, free functions named `exp`, `log`, `sqrt`, `pow`, `gcd` and `interpolate` collided with `std::` and the Mathematics scalar duplicates once aggregates were combined, and needed forward declarations ([03-cpp.md](03-cpp.md), API contracts). Hidden friends alone were rejected because a friend without a `Poly<T>` parameter is unreachable through ADL and one whose signature ignores `T` is redefined per instantiation; extending the rule beyond `01-Core` and the struct rows of `02-Data Structures` was rejected because it would reopen verified geometry and graph headers whose free functions carry little collision risk. Batch C04 (InfInt) then took C11 (Poly) as a prerequisite to reuse its verified NTT, FFT and Garner kernels.
- **Member order (2026-10-10).** `16-poly.hpp` keeps its 32 static builders inside their algorithm families rather than one construction group, so `nttDoubling` stays beside `convolutionLarge` and `interpolate` beside `evalMulti`; the `03-cpp.md` order stands for small types.
- **One API per operation (2026-10-11).** `EdmondsKarp` in `19-max_flow.hpp` turned out to be a legacy adapter running Dinic, and a sweep found about thirty more wrappers kept only so archived code would compile. The rule ([03-cpp.md](03-cpp.md), One API per operation) removes such forms and supersedes the earlier preference for preserving legacy spellings.
- **Writing (2026-10-11).** The one-rule-one-owner writing rule was adopted ([01-principles.md](01-principles.md), Scope discipline).
- **Cached Montgomery contexts (2026-10-12).** Rebuilding a `Montgomery64` on every `pow` cost about 40% of a 512-exponent power, so each modular context now caches the one for its modulus. Timed on distinct bases, Montgomery loses to native `%` from about `2^63`, so the types use it only below `2^62` ([01-Core/Docs/05-modint.md](../01-Core/Docs/05-modint.md)).
- **Omission sweep (2026-10-14).** Each package researches only its own rows and records what it leaves out; nothing returned those omissions or never-researched families to the plan. `/omission-sweep` does, library-wide (replacing the single-family `/research`), as a read-only workflow (workflows never implement) whose candidates need two agreeing verifiers (else the user decides); `apply` follows `01-principles.md` and reopens verified packages that gain operations. Opus at xhigh: catalog research and rule triage, not Core kernel design.
- **Re-audit retired (2026-10-14).** All 17 Foundations packages passed their re-audit and no package was left in `audit`, so the `audit` status, the `/package` re-audit section and the `/reaudit-review` workflow (64 headers reviewed on 2026-10-06, 168 of 172 findings confirmed and fixed) were removed; a package that needs rework is set `in-progress` with a note. `23-Reaudit Findings` was renamed `23-Sweep Ledgers` (2026-10-15) and holds only the sweep ledgers.
