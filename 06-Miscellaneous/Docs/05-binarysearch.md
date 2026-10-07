# 05-binarysearch.hpp — evidence

`05-binarysearch.hpp` (batch MI02, package P013) is an include facade for the canonical Mathematics search API (13 names); it exposes `05-Mathematics/02-search_algorithms.hpp` without copying its engine. First verified 2026-09-28 for P013. Re-audited 2026-10-07: the facade row now names `fibSearch` and `expSearch`, which the facade always exported. Dependencies: P002 (template) and P012 (search engine), both verified.

## Contracts

### Binary search facade

The inventory explicitly assigns Mathematics as the canonical owner of shared search. Consequently the Miscellaneous target is an intentional direct-include facade, despite the default preference against redundant alias-only wrappers. Its direct dependency is `../05-Mathematics/02-search_algorithms.hpp`, whose complete API/proofs are in [02-search_algorithms.md](../../05-Mathematics/Docs/02-search_algorithms.md).

- `firstTrue` and `lastTrue` search `[l,r)` with their respective monotone predicate direction and return `{found,position}`. Absence is `{false,r}`, including an empty range. All representable signed 64-bit endpoints are accepted, but an excluded endpoint beyond `INT64_MAX` is not representable.
- `binSearch(ok,ng,f)` preserves known true/false endpoints in either order, never evaluates those endpoints, and uses overflow-safe `std::midpoint`. Equal endpoints are already converged. It supports a known-true maximum signed endpoint without adding one to it.
- `binSearchRealBracket` accepts finite binary64 endpoints and nonnegative finite tolerances/iteration caps. It reports the remaining sorted bracket, feasible endpoint, iteration count and actual convergence. Width tolerance or adjacent representable endpoints establishes convergence; merely exhausting an iteration cap does not. Predicate evaluations must preserve the claimed monotonicity.
- The canonical header also exposes `ternSearch`, `fibSearch` (Fibonacci-section leftmost minimum), `expSearch` (upward galloping, unbounded), the real ternary/golden bracket methods and point wrappers. The facade introduces no alternate versions, names or stopping rules; all thirteen names in the inventory row resolve to the canonical definitions.

Integer searches take O(log(n+1)) predicate work with O(1) memory and at most 64 bisection predicate evaluations over the full signed domain. Real bisection takes O(iteration cap) work and O(1) memory. Objective/predicate cost multiplies those counts. No canonical Mathematics implementation was changed by this task.

## Re-audit findings (P013, 2026-10-07)

| Header | Gap or finding | Resolution |
|---|---|---|
| `05` | Finding 4: `RealSearchResult` lacks a complexity line | Already fixed by the P012 re-audit: `05-Mathematics/02-search_algorithms.hpp` carries `// T: O(1), M: O(1); ...` above the struct; the facade needs no change |
| all | Contracts in multi-line header comments over the 8% cap | Moved to `## Contracts` sections; headers pass the cap |
| all | `; }` closing braces in headers, testers, benchmark (finding 7) | Normalized; `03-consistency.py --braces` reports none |

The completeness sweep made the facade row list `fibSearch`/`expSearch`. The independent `@reviewer` pass (2026-10-08) reported a comment-cap hit on `05-binarysearch_tester.cpp`; that is outside the rule, which covers only headers.

## Feature-to-test map

| Feature | Verification |
|---|---|
| Actual search facade availability | Compile-time uses of first/last-true, integer bracket and real bracket **before** including shared canonical test source; an empty/broken facade fails to compile |
| Search behavior and edge cases | Reused canonical exhaustive monotone arrays, brute-force minima, full signed endpoints, real extremes/subnormals/adjacency, analytic roots/minima, iteration/tolerance/call counts, every exported API, 15 assertion probes |

Search modes reuse the established canonical suite: quick checks minimum arrays through length 6 and 100 random cases, full through length 8 and 3,000 random cases, stress through length 9 and 30,000. Full performs 2,375,977 checks per configuration after the P012 re-audit enlarged the canonical suite (stress 22,081,188). Output from the shared C++ suite intentionally names `02-search_algorithms`; the Python entry names the tested `05-binarysearch` facade. This reuse avoids copying both engine and test logic. The package integration separately checks Basic/All and cross-translation-unit inclusion.

## Commands and results

GCC 16.2.1, GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H, 2026-10-07:

```sh
python3 '96-Local Testing/06-Miscellaneous/05-binarysearch_tester.py' --mode full --seed 20260927                              # PASS, 3 configurations, 15 probes
python3 '96-Local Testing/06-Miscellaneous/05-binarysearch_tester.py' --mode stress --seed 20260928 --configuration optimized  # PASS
```

Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan with leak detection. No online submission was made.

P013 package runs, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H, 2026-10-07. Baseline before the re-audit changes: all seven P013 suites (`01`–`07`) passed full mode with seed 20260927. After the changes:

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

`02-integration.py` also builds every header alone and the Basic/All aggregates.

## Benchmarks

No benchmark is required: there is no tuned threshold or reduction backend; the facade adds no code.

## Sources

Inspected during the 2026-09-27/28 session; code written locally, with no external implementation copied:

- [cp-algorithms, *Binary Search*](https://cp-algorithms.com/num_methods/binary_search.html): read the monotone-predicate invariant, midpoint-overflow discussion, absent transitions and continuous-search section. Compared these with the existing canonical implementation and its stronger finite-width/status contracts.
- Local [Mathematics search header](../../05-Mathematics/02-search_algorithms.hpp), [verification note](../../05-Mathematics/Docs/02-search_algorithms.md) and existing search tester: read API, proof, test coverage and dependency before reusing them. Their cited historical sources/acceptance are not new claims made by the facade.
- Completeness sweep 2026-10-07 (`@researcher`): [hitonanode bisect](https://hitonanode.github.io/cplib-cpp/other_algorithms/bisect.hpp) (full sweep list in [04-compression.md](04-compression.md)). Exact IEEE-754 bisection was not adopted: Mathematics `02` owns real search.

## Limits and handoffs

Mathematics `02-search_algorithms.hpp` is the canonical owner; its evidence is [02-search_algorithms.md](../../05-Mathematics/Docs/02-search_algorithms.md). No canonical Mathematics implementation was changed by this facade. No online submission was made and no judge acceptance is claimed.
