# Principles

Every implementation satisfies all four requirements, and none is optional; Priority orders them by profile.

| Requirement | Meaning |
|---|---|
| Correctness | Right answer for every input in the stated domain, including boundaries, with no undefined behavior. A violated precondition (assert) and a valid input with no answer (sentinel/status) are different cases. |
| Optimality | Each family reaches the best asymptotic bound for its domain, and each operation the best bound and practical constant its algorithm and profile allow. Choose thresholds for real workloads and record them. Compilation time never matters. |
| Completeness | Every public operation of the family is named in the folder inventory and either implemented or marked planned. Python is a documented subset, never a silent one. A wrapper in another context, a named composition and a one-problem algorithm a Completeness-sweep catalog (`09-sources.md`) names are operations; another form in the same context and a second implementation are not (`03-cpp.md`, One API per operation). |
| Elegance | Direct control flow, short meaningful names, few variables, no duplication or gratuitous abstraction. Compression never hides an invariant or repeats expensive work. |

## Priority

Core full puts Optimality before Completeness: a family keeps only its fastest algorithms. A Core mini mirrors its full type (Profiles). The Contest profile puts Completeness first: beside the fastest algorithm, a family keeps each standard slower one that is

- lighter: fewer dependencies, or at most half the lines with its dependencies counted;
- wider: a larger domain (another ring, any modulus, fewer preconditions);
- faster below a measured crossover; or
- more capable: it offers what the faster one lacks (online updates, the whole table, reconstruction).

Any other slower algorithm, in any profile, stays out unless the maintainer, or a contributor in a pull request the maintainer lands, records an exception under Recorded decisions in `00-Guidelines/23-Sweep Ledgers/omissions.md`. Every kept algorithm is implemented as efficiently as its own bound allows.

## Profiles

| Profile | Scope | Required behavior |
|---|---|---|
| Core full | Non-mini headers in `01-Core` | Extreme optimization: AVX2/FMA/BMI2 or other specialized kernels behind compile-time guards, scalar fallback with identical results, measured crossover thresholds, Barrett/Montgomery wherever beneficial. |
| Core mini | `*mini.hpp` in `01-Core` | Scalar counterpart of its full type: every operation and the whole domain except the backend internals its row names (`02-structure.md`, Inventory contract), with the same complexity and identical results, in optimized scalar code without handwritten SIMD or ISA-specific code. Includes only `01-template.hpp` and other minis. |
| Contest | `02`–`07` and `08-Python` | Completeness first (Priority), each operation an efficient direct implementation of its algorithm. No handwritten SIMD, assembly or ISA-specific code; ordinary compiler optimization only. Core types are template parameters defaulting to minis or dependency-free code (`03-cpp.md`, Headers); a full type the caller passes works through its scalar fallback without special flags. |

A Barrett or Montgomery reduction written outside Core needs a recorded end-to-end benchmark showing at least 20% lower runtime on a representative workload and no meaningful common-case regression. A smaller gain needs a written reason such as a judge limit. Core's own reducers reach contest code only as template arguments the caller passes (`03-cpp.md`, Headers).

Single-threaded contest execution is the baseline. Static caches and dynamic-modulus state are allowed when their lifetime and invalidation rules are documented.

## Evidence

- A status claim is backed by evidence: verified means every operation in the inventory row has a test with an independent oracle and the recorded commands pass. Existing code, old tests or one accepted judge submission prove nothing on their own. A package with a missing operation is never `verified`: its row is `partial` and the package `in-progress` with a note.
- Record domains, omissions, provenance, test commands with results, and benchmark conditions. Never state that a source was read, a test was run or a submission was accepted when it was not.
- Finite tests prove nothing about untested inputs. Pair them with a correctness argument.

## Scope discipline

- Do the assigned package only. Update its inventory rows, tests, aggregates, evidence, plan entry and dependent paths in the same change. A defect `/finish` finds in another package's files is fixed only with the user's yes and recorded in that package's evidence; the recorded call-site migrations of `/prune` and `/package` step 4 need none.
- `OLD` is never deleted. A `97-Legacy` excerpt is deleted once the row it informs is verified; until then it stays untouched.
- Never submit to an online judge.
- Guideline, skill and document edits state each rule once, in its owning file, as briefly as unambiguity allows; other files link it.
