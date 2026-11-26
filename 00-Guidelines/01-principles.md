# Principles

Every implementation satisfies all four requirements; Priority orders them by profile.

| Requirement | Meaning |
|---|---|
| Correctness | Right answer for every input in the stated domain, including boundaries, with no undefined behavior. A violated precondition (assert) and a valid input with no answer (sentinel/status) are different cases. |
| Optimality | Each family reaches the best asymptotic bound for its domain, and each operation the best bound and practical constant its algorithm and profile allow. Choose thresholds for real workloads and record them. Compilation time never matters. |
| Completeness | Every public operation of the family is named in the folder inventory and either implemented or marked planned. Python is a documented subset, never a silent one. A wrapper in another context, a named composition and a one-problem algorithm a Completeness-sweep catalog (`09-sources.md`) names are operations; another form in the same context and a second implementation are not (`03-cpp.md`, One API per Operation). |
| Elegance | Direct control flow, short meaningful names, few variables, no duplication or gratuitous abstraction. Compression never hides an invariant or repeats expensive work. |

## Priority

Core full puts Optimality before Completeness: a family keeps only its fastest algorithms. A Core mini mirrors its full type (Profiles). The Contest profile puts Completeness first: beside the fastest algorithm, a family keeps each standard slower one that is

- lighter: fewer dependencies, or at most half the lines with its dependencies counted;
- wider: a larger domain (another ring, any modulus, fewer preconditions);
- faster below a measured crossover; or
- more capable: it offers what the faster one lacks (online updates, the whole table, reconstruction).

Any other slower algorithm, in any profile, stays out unless the maintainer, or a contributor in a pull request the maintainer lands, records an exception under Recorded Decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md`. Every kept algorithm is implemented as efficiently as its own bound allows.

## Profiles

| Profile | Scope | Required behavior |
|---|---|---|
| Core full | Non-mini headers in `01-Core` | Extreme optimization: AVX2/FMA/BMI2 or other specialized kernels behind compile-time guards, scalar fallback with identical results, measured crossover thresholds, Barrett/Montgomery wherever beneficial. |
| Core mini | `*mini.hpp` in `01-Core` | Scalar counterpart of its full type: every operation and the whole domain except the backend internals its row names (`02-structure.md`, Inventory Contract), with the same complexity and identical results, in optimized scalar code without handwritten SIMD or ISA-specific code. Includes only `01-template.hpp` and other minis. |
| Contest | `02`–`07` and `08-Python` | Completeness first (Priority), each operation an efficient direct implementation of its algorithm. No handwritten SIMD, assembly or ISA-specific code; ordinary compiler optimization only. Core types are template parameters defaulting to minis or dependency-free code (`03-cpp.md`, Headers); a full type the caller passes works through its scalar fallback without special flags. |

A Barrett or Montgomery reduction written outside Core needs a recorded end-to-end benchmark showing at least 20% lower runtime on a representative workload and no meaningful common-case regression. A smaller gain needs a written reason such as a judge limit. Core's own reducers reach contest code only as template arguments the caller passes (`03-cpp.md`, Headers).

A Core full header leaves no optimization undecided (user decisions 2026-11-17): before its package is verified, every candidate anyone names for one of its operations, at any stage, is adopted, rejected or handed to its owning package on recorded evidence (`05-testing.md`, Benchmarks); `Not tried`, `unmeasured` and `deferred` are not verdicts. `01-template.hpp` and `02-debug.hpp` are exempt from this and from the sweep: they are boilerplate, not algorithms, held to correctness and ordinary efficiency.

Single-threaded contest execution is the baseline. Static caches and dynamic-modulus state are allowed when their lifetime and invalidation rules are documented.

## Evidence

- A status claim is backed by evidence: verified means every operation in the inventory row has a test with an independent oracle and the recorded commands pass. Existing code, old tests or one accepted judge submission prove nothing alone. A package with a missing operation is never `verified`: its row is `partial` and the package `in-progress` with a note, unless the operation joined the row after the package was verified and is an open follow-up on it (`plan.py follow-up`; user decision 2026-11-26).
- Record domains, omissions, provenance, test commands with results, and benchmark conditions. Never state that a source was read, a test was run or a submission was accepted when it was not.
- Finite tests prove nothing about untested inputs; pair them with a correctness argument.
- A source the work needs and cannot open (a paper, PDF, page or data file whose download fails, is blocked or sits behind a paywall) is never skipped silently or replaced by memory (user decision 2026-11-18). When it happens, ask the user through AskUserQuestion, with its title and URL, to place it in the main checkout's `95-Resources/00-Inbox/`, named by its absolute path and logged there (`95-Resources/00-index.md`, Storage), and continue meanwhile with work that does not depend on it; a subagent reports it and the session that started it asks. Every source file the work reads is filed when first cited (`95-Resources/00-index.md`, Filing; user decision 2026-11-27), and evidence citing it records its filed path, title, URL, SHA-256 and the pages read. If the user declines, the evidence says `not fetched` beside each claim that cites the source, and no claim rests on it alone.

## Scope Discipline

- Do the assigned package only. Update its inventory rows, tests, aggregates, evidence, plan entry and dependent paths in the same change. A finding in another package's files becomes a follow-up on that package (`plan.py follow-up`); only a wrong answer or undefined behavior is fixed here, and recorded in that package's evidence (user decision 2026-11-26). The call-site migrations `/prune` and `/package` step 4 record are made here.
- Conservative default (user decision 2026-11-26): every gap the package can close within its scope and on its model is fixed when found, without asking, a shortfall against its brief included (user decision 2026-11-10). Add rather than omit: an operation no rule excludes and no other row owns is implemented unless it needs another model, package or source, and then becomes a follow-up on its owner, named in the report; one left out gets a one-line reason in the folder's `Docs/00-notes.md`. An optimization is decided by `05-testing.md` (Benchmarks), taking the faster correct form. A decisions list, follow-up note or passing exit code never stands in for a fix.
- Nothing is dropped or decided silently (user decision 2026-11-23). Leaving out an operation or candidate, narrowing a domain, changing a contract, weakening or skipping a check, departing from the brief, and any other choice that changes what the work delivers and that no rule or recorded user decision settles, is the user's: ask it through AskUserQuestion, once, after the work that does not depend on the answer (user decision 2026-11-26). When the work cannot wait for the answer, take the option that keeps the most and the fastest correct result and is easiest to undo, and say so. Every final report closes with two lists, each `none` when empty (user decision 2026-11-27): **Decided Alone**, each decision made without the user with its alternatives, then **Not Done**, everything left out, deferred, skipped, weakened or unverified with its reason; items a sweep ledger (`00-Guidelines/23-Sweep Ledgers/`) lists in full appear as counts with its path. Brevity rules shorten wording, never these lists. A report's headings and bold labels, these two included, take the Title Case of `02-structure.md` (Names) (user decision 2026-11-27).
- `OLD` is never deleted. A `97-Legacy` excerpt is deleted once the row it informs is verified; until then it stays untouched.
- Claude never submits to an online judge; the maintainer submits, by hand or with `97-Online Testing/04-submit.py` (`06-online.md`; user decision 2026-11-15).
- Guideline, skill and document edits state each rule once, in its owning file, as briefly as unambiguity allows; other files link it.
- Rule freeze (user decision 2026-11-26): guidelines, skills, agents, hooks and `CLAUDE.md` change only in a process batch or by a user decision, recorded at once in its owning file (`CLAUDE.md`, Start Here), about every ten packages: `/reflect`, a review of `00-Guidelines/23-Sweep Ledgers/process.md` and `/tidy` on the units changed since its last run. A process idea found meanwhile is added to `process.md`. A proposed rule names the text it replaces, or why none can go (user decision 2026-11-27). New rules apply forward: verified work reopens only for a wrong answer or undefined behavior, as a follow-up.
