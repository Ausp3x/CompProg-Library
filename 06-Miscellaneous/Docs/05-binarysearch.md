# 05-binarysearch.hpp — evidence

`05-binarysearch.hpp` (batch MI02, package P013) is an include facade for the canonical Mathematics search API (13 names); it exposes `05-Mathematics/02-search_algorithms.hpp` without copying its engine. Dependencies: P002 (template) and P012 (search engine), both verified.

## Contracts

### Binary search facade

The inventory explicitly assigns Mathematics as the canonical owner of shared search. Consequently the Miscellaneous target is an intentional direct-include facade, despite the default preference against redundant alias-only wrappers. Its direct dependency is `../05-Mathematics/02-search_algorithms.hpp`, whose complete API/proofs are in [02-search_algorithms.md](../../05-Mathematics/Docs/02-search_algorithms.md).

- `firstTrue` and `lastTrue` search `[l,r)` with their respective monotone predicate direction and return `{found,position}`. Absence is `{false,r}`, including an empty range. All representable signed 64-bit endpoints are accepted, but an excluded endpoint beyond `INT64_MAX` is not representable.
- `binSearch(ok,ng,f)` preserves known true/false endpoints in either order, never evaluates those endpoints, and uses overflow-safe `std::midpoint`. Equal endpoints are already converged. It supports a known-true maximum signed endpoint without adding one to it.
- `binSearchRealBracket` accepts finite binary64 endpoints and nonnegative finite tolerances/iteration caps. It reports the remaining sorted bracket, feasible endpoint, iteration count and actual convergence. Width tolerance or adjacent representable endpoints establishes convergence; merely exhausting an iteration cap does not. Predicate evaluations must preserve the claimed monotonicity.
- The canonical header also exposes `ternSearch`, `fibSearch` (Fibonacci-section leftmost minimum), `expSearch` (upward galloping, unbounded), the real ternary/golden bracket methods and point wrappers. The facade introduces no alternate versions, names or stopping rules; all thirteen names in the inventory row resolve to the canonical definitions.

Integer searches take O(log(n+1)) predicate work with O(1) memory and at most 64 bisection predicate evaluations over the full signed domain. Real bisection takes O(iteration cap) work and O(1) memory. Objective/predicate cost multiplies those counts.

## Feature-to-test map

| Feature | Verification |
|---|---|
| Actual search facade availability | Compile-time uses of first/last-true, integer bracket and real bracket **before** including shared canonical test source; an empty/broken facade fails to compile |
| Search behavior and edge cases | Reused canonical exhaustive monotone arrays, brute-force minima, full signed endpoints, real extremes/subnormals/adjacency, analytic roots/minima, iteration/tolerance/call counts, every exported API, 15 assertion probes |

Search modes reuse the established canonical suite: quick checks minimum arrays through length 6 and 100 random cases, full through length 8 and 3,000 random cases, stress through length 9 and 30,000. Full performs 2,375,977 checks per configuration (stress 22,081,188). Output from the shared C++ suite intentionally names `02-search_algorithms`; the Python entry names the tested `05-binarysearch` facade. This reuse avoids copying both engine and test logic. The package integration separately checks Basic/All and cross-translation-unit inclusion.

## Commands and results

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. The binarysearch full run passed all 3 configurations and 15 assertion probes.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

## Sources

Inspected during the 2026-09-27/28 session; code written locally, with no external implementation copied:

- [cp-algorithms, *Binary Search*](https://cp-algorithms.com/num_methods/binary_search.html): read the monotone-predicate invariant, midpoint-overflow discussion, absent transitions and continuous-search section. Compared these with the existing canonical implementation and its stronger finite-width/status contracts.
- Local [Mathematics search header](../../05-Mathematics/02-search_algorithms.hpp), [verification note](../../05-Mathematics/Docs/02-search_algorithms.md) and existing search tester: read API, proof, test coverage and dependency before reusing them. Their cited historical sources/acceptance are not new claims made by the facade.
- Completeness sweep 2026-10-07 (`@researcher`): [hitonanode bisect](https://hitonanode.github.io/cplib-cpp/other_algorithms/bisect.hpp) (full sweep list in [04-compression.md](04-compression.md)). Exact IEEE-754 bisection was not adopted: Mathematics `02` owns real search.

## Limits and handoffs

Mathematics `02-search_algorithms.hpp` is the canonical owner; its evidence is [02-search_algorithms.md](../../05-Mathematics/Docs/02-search_algorithms.md). No canonical Mathematics implementation was changed by this facade. No benchmark: the facade adds no code. No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-28: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, row now names `fibSearch`/`expSearch` (always exported), finding 4 already fixed upstream by P012, contracts moved out of code comments; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects.
