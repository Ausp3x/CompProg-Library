# MA01 search algorithms — contracts and verification

`02-search_algorithms.hpp` is verified for the Basic inventory row. The four legacy names remain: `binSearch`, `binSearchReal`, `ternSearch`, and `ternSearchReal`. The header adds endpoint-independent first/last-true searches, golden-section minimization, and real bracket/result interfaces. All functions are stateless and all caller predicates/objectives must be deterministic.

## Integer domains and correctness

`binSearch(ok, ng, f)` accepts the full signed 64-bit endpoint domain in either order. The caller knows that `ok` is feasible and `ng` infeasible, with one monotone transition between them. Neither endpoint is evaluated, so they may represent hypothetical boundary states. Equal endpoints mean an already converged answer. Each queried midpoint is strictly interior. `std::midpoint` rounds toward `ok`; when it equals `ok`, the endpoints are adjacent or equal. Updating one endpoint preserves the transition bracket. This avoids the archived `abs(ok - ng)` overflow even for `INT64_MIN` and `INT64_MAX`. There are at most 64 predicate calls.

`firstTrue(l, r, f)` searches half-open `[l, r)` with a false-then-true predicate. `lastTrue` uses a true-then-false predicate. Both return `{found, position}`; absence returns `{false, r}`, including empty ranges. The presence flag distinguishes absence from every valid position. Each midpoint satisfies `l <= md < r`, so advancing `md + 1` cannot overflow. `lastTrue` searches the first false boundary and decrements it only when it exceeds `l`. The maximum signed endpoint remains a valid excluded endpoint; these half-open APIs do not pretend to represent an excluded endpoint beyond `INT64_MAX`. Use the explicit endpoint bracket interface when searching through that last representable value.

`ternSearch(l, r, f)` retains its legacy **closed** interval `[l, r]`, including either signed extreme and singleton intervals. It returns the **leftmost minimum**. The contract is strictly decreasing values, then an optional interval of equal minimum values, then strictly increasing values; either strict side may be empty. Arbitrary flat stretches away from the minimum do not satisfy this contract. The implementation compares neighboring values at `md` and `md + 1`: a decrease puts the first minimum to the right, otherwise it is at or left of `md`. The midpoint satisfies `md < r`, making `md + 1` safe. This binary reduction shrinks the interval by a factor of two instead of the archived ternary reduction’s factor of three-halves; both have logarithmic complexity. It takes at most 128 objective evaluations over the full signed domain. Values need only a reliable `<` comparison; no objective-value arithmetic is performed.

## Real contracts and precision

The real domain is finite IEEE binary64 endpoints, including subnormals, signed zero and opposite-sign values near `DBL_MAX`. Iteration limits and finite absolute/relative tolerances are nonnegative. Bracket endpoints are sorted in the returned `RealSearchResult {l, r, x, iterations, converged}`.

- `binSearchRealBracket(ok, ng, f, itr, abs_tol, rel_tol)` keeps known true/false endpoints in either order and returns the feasible endpoint as `x`. It does not evaluate supplied endpoints. Coincident endpoints are an already converged answer.
- `ternSearchRealBracket(l, r, f, itr, abs_tol, rel_tol)` and `goldenSearchRealBracket` require `l <= r` and the same strict-side/optional-minimum-plateau shape as the integer minimizer. Their bracket intersects the set of minimizers, and `x` is its rounded midpoint. They do not promise the leftmost point of a continuous minimum plateau.
- The three point wrappers return `.x`, with the legacy iteration defaults of 100 for binary and 200 for ternary; golden defaults to 100. To maximize, minimize a reversed objective whose evaluation remains valid, such as a negated real objective. Negating a minimum signed integer is not valid arithmetic.

`converged` is true exactly when

```
r - l <= abs_tol + rel_tol * max(abs(l), abs(r))
```

or no binary64 value lies strictly inside `[l, r]`. The width and tolerance expression use the platform's wider `long double`, so neither opposite-sign endpoints nor tolerance products overflow. Reaching the iteration cap does not imply convergence. Ternary/golden interpolation may stall before adjacency; this returns the remaining bracket with `converged == false` unless the requested tolerance was achieved. Zero iterations perform no objective evaluations. `iterations` counts completed interval reductions, which may be smaller than the cap.

For initial mathematical width `W`, exact-arithmetic widths after `k` reductions are bounded by `W * q^k`, with `q = 1/2` (binary), `2/3` (ternary), and `(sqrt(5) - 1)/2` (golden). Thus a positive absolute width target `epsilon < W` needs `ceil(log(W / epsilon) / log(1 / q))` reductions in exact arithmetic. A midpoint has distance at most half that width from some bracketed minimizer, before rounding; a feasible binary endpoint has distance at most the whole width from the transition. Compute huge-width formulas in a wider type or through differences of logarithms. Floating-point rounding limits useful precision, so the implementation reports the **actual final bracket and stopping status** instead of claiming a fixed number of iterations always reaches an absolute tolerance. Pure relative tolerance near zero may be unattainable within the cap.

`std::midpoint` and `std::lerp` avoid overflowing `r - l` in binary64. Every successful real reduction retains ordered interior probes; golden search checks this order before reusing cached values. Its first reduction costs two evaluations, and each later reduction costs one; ternary costs two per reduction and binary one. Objective comparisons must reflect the stated unimodal/monotone problem. Rounded objective values, noisy evaluations and NaNs can violate that assumption; the returned interval is not an interval-arithmetic certificate for an independently defined exact function. Root-finding methods beyond predicate bisection belong to `17-numerical_methods.hpp`; certified ball arithmetic has its own family. Batched parallel binary search remains canonical in Miscellaneous `20-parallelbinarysearch.hpp`.

## Source review and legacy accounting

Inspected on 2026-09-27:

| Reference | Inspected claim and use |
|---|---|
| [cp-algorithms, Binary Search](https://cp-algorithms.com/num_methods/binary_search.html) | Arbitrary monotone-predicate invariant, absent transitions, continuous bisection, and distinction from parallel binary search. Independently implemented with overflow-free midpoint termination and explicit absence. |
| [cp-algorithms, Ternary Search](https://cp-algorithms.com/num_methods/ternary_search.html) | Strict-side unimodality, equality elimination, integer stopping issues, exact contraction, golden-ratio derivation and evaluation reuse. Independently implemented; this header uses neighboring-point binary minimization for the discrete API. |
| [KACTL, GoldenSectionSearch.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/numerical/GoldenSectionSearch.h), Ulf Lundstrom, 2009-04-17, CC0 | Independently compared cached-value golden-section recurrence and the warning that imprecise ratios can break interior-point ordering. The local implementation adds robust interpolation, explicit ordering/stagnation checks, finite domains and result status. No source code copied. |
| [cppreference, std::midpoint](https://en.cppreference.com/w/cpp/numeric/midpoint.html) | No-overflow guarantee and integer rounding toward the first endpoint; supports full signed-domain termination and finite-double bracketing. |

`OLD/5-Mathematics/02-search.hpp` and `OLD/Team Notebook/src/math/search.hpp` were inspected and preserved. Their known-true endpoint result, closed integer minimization, real point interfaces and iteration arguments remain available. Their unguarded signed differences and real interpolation differences were replaced. No online submissions or acceptance claims were made.

## Feature-to-test map

The runnable entry is `96-Local Testing/05-Mathematics/02-search_algorithms_tester.py`; the C++ suite includes the actual header directly and uses non-removable checks under `-DNDEBUG`.

| Feature | Independent verification |
|---|---|
| Integer monotone searches; both bracket orientations; absence and emptiness | Exhaust all small endpoint/transition triples, a signed-extreme endpoint matrix, then seeded arbitrary 64-bit intervals; analytic transition positions, endpoint-query guards, and 64-call limit. |
| Discrete leftmost minimization | Exhaust all four-valued arrays through length 8 and filter exactly the declared shape; compare with linear scan at offsets near both signed extremes. Random variable-slope arrays use another linear scan. Full-domain asymmetric V/flat-minimum functions have analytic answers; 128-call bound. |
| Real binary endpoints and wrappers | Analytic thresholds in both orientations; final feasible endpoint and threshold enclosure, wrapper parity, default arguments and one-call-per-reduction checks. |
| Real ternary/golden minima and wrappers | Analytic V functions and minimum plateaus, endpoint minima, constant functions; bracket/minimizer intersection, point-wrapper parity, default arguments and two/one cached evaluation counts. |
| Real precision, failure-to-converge and finite domain | Zero/low/high iteration limits; absolute and relative tolerances; opposite `DBL_MAX`, subnormal intervals, signed zeros, equal/adjacent endpoints and interpolation stagnation; independent stopping predicate and exact contraction plus rounding allowance. |
| Preconditions | 15 checked-build subprocess death tests: reversed ranges, negative iteration budgets, infinite/NaN endpoints, negative/infinite/NaN tolerances. |

Quick exhausts arrays through length 6 and runs 100 random cases. Full exhausts through length 8 and runs 3000 random cases. Stress extends those bounds to length 9 and 30000 cases. The exhaustive minimum-shape acceptance counts are 282 in quick and 452 in full; all rejected shapes remain outside the documented contract.

Verification completed with GCC 16.2.1, GNU++20, seed `20260927`:

```
python3 '96-Local Testing/05-Mathematics/02-search_algorithms_tester.py' --mode full --seed 20260927
```

Optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan builds each passed **1,944,079 checks**; checked passed all **15 assertion probes**. The initial sandbox sanitizer run encountered LeakSanitizer's documented ptrace incompatibility; the approved unrestricted rerun passed all three configurations with leak detection enabled. Quick also passed both configurations. Evaluation-count bounds provide direct cost evidence; no machine-specific speedup claim or timing threshold is used. Package integration owns standalone/aggregate/multiple-translation-unit compilation and consistency checks.

No search-row feature remains incomplete. Stress mode is supplied but was not needed for the full verification claim.
