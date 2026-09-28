# Shared principles

## Four requirements

- **Correctness:** correct for every input in the stated domain, including boundaries; no undefined behavior. Preconditions and legitimate no-answer outcomes are different. Correctness is unconditional.
- **Optimality:** best appropriate asymptotic and practical implementation, avoiding general bottlenecks and needless work. Select algorithms/thresholds for actual domains and workloads; a universal fastest implementation is not a meaningful claim. Compilation time is not a constraint. State material memory and preprocessing costs.
- **Completeness:** a complete, explicit feature inventory for each problem family, including relevant advanced and research-level variants. Inventory omissions and unfinished entries remain visible. Mini/Python scopes are explicitly reduced; do not silently weaken shared semantics.
- **Elegance:** clean compressed algorithm code, direct control flow, short meaningful names, few necessary variables, no redundancy or gratuitous abstractions. This remains a requirement in every profile, not an optional priority to discard. Compression must not hide invariants, duplicate expensive computation, or weaken another requirement.

## Profiles

| Profile | Required behavior |
|---|---|
| Core full: non-mini algorithms in 01-Core | Extreme practical optimization, including applicable SIMD/AVX2/FMA/BMI or other specialized techniques, compile-time selection, scalar fallback, measured thresholds |
| Core mini | Minimize printed lines: omit rare operations, then restrict documented domains, before choosing slower implementations; identical semantics on the shared domain; no fixed line cap; independently copyable from full and sibling implementations |
| Contest: 02–07 and Python | Optimal complexity and direct efficient implementation with more brevity leeway; no handwritten SIMD/assembly/ISA-specific requirements; ordinary compiler optimization is allowed |

Contest algorithms may depend on accelerated full Core types, whose fallback must work without special flags. Keep specialization implemented in Core. Large tables or similarly bulky optimizations require a compact alternative in the same header/module. Single-threaded contest semantics are the baseline; static caches and dynamic-modulus state are allowed with documented validity/lifetime rules.

Barrett/Montgomery use outside Core requires at least **20% lower end-to-end runtime** on representative relevant workloads, no meaningful common-case regression, and recorded evidence. A smaller gain needs a specific documented justification such as meeting a judge limit. A tiny isolated arithmetic benchmark is insufficient. Keep a simpler alternative when code growth is substantial. Inside Core, use them wherever applicable and beneficial.

## Evidence and scope

Record supported domains, missing features, source provenance, test commands/results, and benchmark conditions honestly. Existing source, old test success, or one accepted judge problem does not establish maximality or complete correctness. Finite tests cannot prove all inputs; combine justified algorithms with systematic verification. Do not claim an online acceptance or external source review that did not happen.

The current inventory is a living roadmap, not an instruction to build everything in every task. Update the relevant entry and verification evidence when changing an implementation. Preserve legacy files in OLD; remove them only after their intended features have been migrated and accounted for.
