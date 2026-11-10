---
paths:
  - "08-Python/**/*.py"
---

# Python

- Baseline: Python 3.10 syntax and standard library; must run on CPython 3.10+ and PyPy 3.10+. PyPy is the primary performance target: avoid per-element Python-object churn, prefer lists of ints over tuples, pack pairs into one int when it is hot, use iterative traversal or the generator-trampoline pattern instead of deep recursion.
- Algorithm modules import only the standard library. Python `int` and `fractions.Fraction` replace custom numeric types.
- `08-Python` is a deliberate subset of `01`–`07`: practical algorithms a PyPy contestant uses, plus PyPy performance idioms. The inventory lists inclusions; `Docs/00-notes.md` lists omissions. A ported operation follows its C++ algorithm, and the slower alternatives its C++ row keeps for it, as fast as PyPy allows.
- Keep C++ semantics where they transfer; use idiomatic Python where they do not, and state the difference in the docstring (floor division, unbounded ints, mutability, recursion limits).
- Modules are importable and copy-pasteable. Relative imports inside packages; no `sys.path` edits in modules.
- Style: four spaces; classes `CamelCase`; operations keep the library's `camelCase` names for cross-language consistency: an operation, type or member with a C++ counterpart takes its exact C++ name (acronyms included: `RollbackDSU`, `LCA`, `findSet`) and contract, and one C++ operation stays one Python operation (user decision 2026-11-10); a module has no evidence document, so its row may carry a search alias as a qualifier (`longestIncreasingSubsequence (LIS)`, `combiNR (binomial)`); fields and locals `snake_case`; constants `ALL_CAPS`; dunder protocols as required. Helpers and state live in the class, public by convention; no name mangling or property boilerplate. Stateless algorithms are functions. Light annotations where they clarify a contract.
- Contracts: explicit valid-input domain; `None` or a documented sentinel for a valid no-answer; `assert` for programmer preconditions only, since `-O` removes it. Exact and approximate variants are distinct functions. Seeds and cache lifetimes are controllable.
- `03-cpp.md`, One API per operation, applies; standard-library functions count as existing operations (`bisect` with `key=`).
- Complexity comments use the C++ notation with `#`: `# T: O(n * log(n)), M: O(n)`, with big-integer and output costs counted.
