# Principles

Every implementation satisfies all four requirements. None is optional and none trades against another.

| Requirement | Meaning |
|---|---|
| Correctness | Right answer for every input in the stated domain, including boundaries, with no undefined behavior. A violated precondition (assert) and a valid input with no answer (sentinel/status) are different cases. |
| Optimality | Best asymptotic bound for the domain and the best practical constant that the profile allows. Choose thresholds for real workloads and record them. Compilation time never matters. |
| Completeness | Every public operation of the family is named in the folder inventory and either implemented or marked planned. Mini and Python are documented reductions, never silent ones. |
| Elegance | Direct control flow, short meaningful names, few variables, no duplication or gratuitous abstraction. Compression never hides an invariant or repeats expensive work. |

## Profiles

| Profile | Scope | Required behavior |
|---|---|---|
| Core full | Non-mini headers in `01-Core` | Extreme optimization: AVX2/FMA/BMI2 or other specialized kernels behind compile-time guards, scalar fallback with identical results, measured crossover thresholds, Barrett/Montgomery wherever beneficial. |
| Core mini | `*mini.hpp` in `01-Core` | Fewest source lines: drop rare operations first, then narrow and document the domain, and only then accept a slower algorithm. Identical results to the full type on the shared domain. Independently copyable: no dependency on a sibling, full type, detail namespace or reduction header. |
| Contest | `02`–`07` and `08-Python` | Optimal complexity and an efficient direct implementation. No handwritten SIMD, assembly or ISA-specific code; ordinary compiler optimization only. May depend on Core full types, whose scalar fallback must work without special flags. |

Barrett or Montgomery reduction outside Core needs a recorded end-to-end benchmark showing at least 20% lower runtime on a representative workload and no meaningful common-case regression. A smaller gain needs a written reason such as a judge limit. Every contest header that uses Core Barrett or Montgomery also keeps dependency-free `Compact` twins of the functions that use it, so a contest copy never needs the Core reduction headers (rule in `03-cpp.md`, Headers).

Single-threaded contest execution is the baseline. Static caches and dynamic-modulus state are allowed when their lifetime and invalidation rules are documented.

## Evidence

- A status claim is backed by evidence: verified means every operation in the inventory row has a test with an independent oracle and the recorded commands pass. Existing code, old tests or one accepted judge submission prove nothing on their own.
- Record domains, omissions, provenance, test commands with results, and benchmark conditions. Never state that a source was read, a test was run or a submission was accepted when it was not.
- Finite tests prove nothing about untested inputs. Pair them with a correctness argument.

## Scope discipline

- Do the assigned package only. Update its inventory rows, tests, aggregates, evidence and dependent paths in the same change.
- `OLD` is never deleted. A `97-Legacy` excerpt is deleted once the row it informs is verified; until then it stays untouched.
- Never submit to an online judge automatically.
