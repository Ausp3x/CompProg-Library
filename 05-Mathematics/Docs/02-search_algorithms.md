# 02-search_algorithms.hpp — evidence

`02-search_algorithms.hpp` covers the Basic search row. The four legacy names remain: `binSearch`, `binSearchReal`, `ternSearch` and `ternSearchReal`. The header adds endpoint-independent first/last-true searches, Fibonacci and exponential integer searches, golden-section minimization, and real bracket/result interfaces. Every function is stateless, and every caller predicate or objective must be deterministic.

## Contracts

### binSearch

`binSearch(ok, ng, f)` accepts any two `lng` endpoints, in either order. The caller knows `ok` is feasible and `ng` infeasible, with one monotone transition between them. Neither endpoint is evaluated, so they may stand for hypothetical boundary states. Equal endpoints mean the answer has already converged. The function returns the last feasible value next to the transition and makes at most 64 predicate calls.

Correctness: `std::midpoint` rounds toward `ok`; when the midpoint equals `ok`, the endpoints are adjacent or equal. Every queried midpoint is strictly interior, and updating one endpoint keeps the transition bracketed. This avoids the archived `abs(ok - ng)` overflow even for `INT64_MIN` and `INT64_MAX`.

### firstTrue, lastTrue

Both search half-open `[l, r)` with `l <= r` (asserted). `firstTrue` needs a false-then-true predicate and `lastTrue` a true-then-false one. Both return `{found, position}`, with `{false, r}` for absence, including empty ranges. Neither endpoint is a sentinel. `r = INT64_MAX` stays a valid excluded endpoint; to search through that last value, use `binSearch`.

Correctness: each midpoint satisfies `l <= md < r`, so `md + 1` cannot overflow. `lastTrue` finds the first false position and decrements it only when it exceeds `l`.

### ternSearch, fibSearch

Both take the legacy **closed** interval `[l, r]` with `l <= r` (asserted), including either signed extreme and singletons. The function must strictly decrease, then may stay flat at its minimum, then strictly increase; either strict side may be empty. Flat stretches away from the minimum break this contract. Both return the **leftmost** minimum and need values only to support `<`.

- `ternSearch` makes at most `2 * ceil(log2(n))` evaluations, 128 over the full domain.
- `fibSearch` makes at most `log_phi(n + 1) + 1` evaluations, 92 over the full `lng` domain. That is about 1.44 log2 n against 2 log2 n, which matters when the objective is expensive. A singleton returns without evaluating.

Correctness of `ternSearch`: it compares neighbours at `md` and `md + 1` (safe since `md < r`). A decrease puts the first minimum to the right; otherwise it is at or left of `md`. This halves the interval each step, where the archived ternary reduction kept two thirds.

Correctness of `fibSearch`: it keeps an open window `(a, b)` with `b - a = y`, where `x < y` are consecutive terms of 2, 3, 5, 8, … and the leftmost minimum lies inside. It starts at `a = l - 1` with the least `y >= n + 1` (at least 3) and probes `c = a + y - x < d = a + x`. If `f(c) <= f(d)`, the leftmost minimum is in `(a, d)` (strict: `d` is on the increasing side; equal: both probes are on the flat minimum or on opposite sides of it). Otherwise `c` precedes the leftmost minimum, which is in `(c, b)`. The window shrinks to length `x` and the surviving probe is one of the two new probes, so each step costs one evaluation. Probes beyond `r` read `f(r)`, extending the function with a flat tail; both cases stay valid because the leftmost minimum is at most `r`, and a probe `c <= r < d` with `f(r) < f(c)` cannot have reached the minimum. At `y = 3` the window holds exactly `c` and `d`. With `F_k` the least term of 3, 5, 8, … with `F_k >= n + 1`, the search makes `k - 1` evaluations; minimality and Binet's formula give at most `log_phi(n + 1) + 1`, also checked numerically through `n = 2000` and at `2^64`. Positions are `lll`, so `n = 2^64` is representable.

### expSearch

`expSearch(ok, f)` gallops upward from a known-true `ok`, which is never evaluated. `f` must be true then false on `[ok, INT64_MAX]`. The function returns the last true value, or `INT64_MAX` when `f` never becomes false. It makes at most about `2 * log2(answer - ok + 2) + 2` evaluations and needs no upper bound in advance. For a downward search, negate the argument inside `f`.

Correctness: each round tests `ok + d` with `d = 1, 2, 4, …` and moves `ok` there while true. The step is clamped to `INT64_MAX` once the gap `INT64_MAX - ok`, computed in `ulng`, is at most `d`. The first false probe brackets the transition for `binSearch`. Because the gaps 1, 2, …, 2^63 sum past every distance, `ok` reaches `INT64_MAX` before `d` can wrap.

### RealSearchResult

The result holds the sorted closed bracket `[l, r]`, the recommended point `x`, the number of completed reductions in `iterations`, and `converged`. `converged` is true exactly when

```
r - l <= abs_tol + rel_tol * max(abs(l), abs(r))
```

or no binary64 value lies strictly inside `[l, r]`. False means the iteration cap was reached or the interpolation stagnated. The width and point guarantees assume reliable comparisons of `f`; this is not interval arithmetic.

### search_detail::validate, search_detail::done

`validate` asserts finite endpoints, `itr >= 0`, and finite nonnegative tolerances. `done` is the `converged` predicate above, evaluated in `long double` so that opposite-sign endpoints and tolerance products cannot overflow.

### binSearchRealBracket

Finite true and false endpoints come in either order and are never evaluated; equal endpoints are already converged. `x` is the true endpoint. In exact arithmetic the width after `k` reductions is `initial_width / 2^k`. Each reduction makes one predicate evaluation, at most `itr` in total.

### ternSearchRealBracket, goldenSearchRealBracket

Both take finite `l <= r` and the same strict-side, optional-flat-minimum shape as `ternSearch`. Comparisons must preserve that shape, so NaN values are not allowed. The returned bracket intersects the set of minimizers, and `x` is its rounded midpoint. It is not promised to be the leftmost point of a continuous plateau.

- Ternary search shrinks the width by `2/3` per reduction at two evaluations each, at most `2 * itr`.
- Golden search shrinks it by `(sqrt(5) - 1) / 2` per reduction and caches one value. It makes zero evaluations when the bracket is already done or `itr = 0`, and at most `itr + 1` otherwise.

Correctness of the real searches: `std::midpoint` and `std::lerp` avoid overflowing `r - l` in binary64. Every successful reduction keeps ordered interior probes, and golden search checks that order before reusing cached values. Ternary and golden interpolation may stall before the endpoints become adjacent; the result is then the remaining bracket with `converged == false`, unless the tolerance was met anyway. Reaching a positive absolute width `epsilon < W` in exact arithmetic takes `ceil(log(W / epsilon) / log(1 / q))` reductions, `q` being 1/2, 2/3 or `(sqrt(5) - 1) / 2`. Rounding limits the useful precision, so the actual bracket and status are reported rather than a fixed iteration count. A purely relative tolerance near zero may be unattainable within the cap.

### binSearchReal, ternSearchReal, goldenSearchReal

These are the legacy point interfaces: they return `.x` of the matching bracket function and share its domain. The default iteration counts are 100 for binary, 200 for ternary (legacy) and 100 for golden. To maximize, minimize a reversed objective whose evaluation stays valid, such as a negated real objective. Negating `INT64_MIN` is not valid.

## Feature-to-test map

Entry: [`02-search_algorithms_tester.py`](<../../96-Local Testing/05-Mathematics/02-search_algorithms_tester.py>), driving the C++ suite, which includes the header directly and keeps its checks under `-DNDEBUG`. Quick mode exhausts arrays through length 6 and runs 100 random cases; full exhausts through length 8 with 3000 random cases; stress reaches length 9 with 30000. The exhaustive shape filter accepts 282 arrays in quick mode and 452 in full.

| Feature | Independent verification |
|---|---|
| `binSearch`, `firstTrue`, `lastTrue`: both orientations, absence, empty ranges | Every small endpoint and transition triple, a signed-extreme endpoint matrix, and seeded arbitrary 64-bit intervals. Checks against analytic transition positions, endpoint-query guards and the 64-call limit. |
| `expSearch` | In every `boundaries()` triple (exhaustive small, signed-extreme matrix, random full-width), the result must equal `p - 1`, with `ok` never queried and at most `2 * bit_width(p - l) + 2` calls. An always-true predicate from every edge value must return `INT64_MAX` within 65 calls. |
| `ternSearch` | Every four-valued array through length 8, filtered to exactly the declared shape, compared with a linear scan at offsets near both signed extremes. Random variable-slope arrays. Full-domain asymmetric V and flat-minimum functions with analytic answers, within the 128-call bound. |
| `fibSearch` | The same exhaustive and random arrays against the linear scan, with an evaluation bound `fibBound(n)` computed independently. The full-domain plateau matrix within 93 calls. Random full-width plateaus. Singletons make no call. Death probe `fib-order`. |
| `binSearchRealBracket`, `binSearchReal` | Analytic thresholds in both orientations: the final feasible endpoint, enclosure of the threshold, wrapper parity, default arguments and one call per reduction. |
| `ternSearchRealBracket`, `goldenSearchRealBracket` and wrappers | Analytic V functions, minimum plateaus, endpoint minima and constant functions. Checks that the bracket meets the minimizer set, wrapper parity, default arguments, and two (ternary) or one cached (golden) evaluation per reduction. |
| `RealSearchResult`, precision and finite domain | Zero, low and high iteration limits; absolute and relative tolerances; opposite `DBL_MAX`; subnormal intervals; signed zeros; equal and adjacent endpoints; interpolation stagnation. `converged` is checked against an independent stopping predicate, and widths against exact contraction plus a rounding allowance. |
| Preconditions | 16 checked-build death probes: reversed ranges (including `fib-order`), negative iteration budgets, infinite or NaN endpoints, negative, infinite or NaN tolerances. |

## Commands and results

Latest P012 re-audit runs (2026-10-07), recorded once for all six headers `01-mod_arithmetic.hpp` … `06-segmentedsieve.hpp`. GCC 16.2.1, GNU++20, Python 3.14, Linux x86-64 (i9-11900H). Every C++ build uses the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (quick runs the first two).

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 05-Mathematics/0 --no-integration                     # PASS, 6 suites, 2 configurations
python3 '96-Local Testing/01-run.py' --mode full --filter 05-Mathematics/0 --seed 1 --no-integration              # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/01-run.py' --mode stress --filter 05-Mathematics/0 --seed 1 --rounds 1 --no-integration # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/02-integration.py'                                                                     # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                     # no errors
```

After the `@reviewer` pass, full (all six headers), stress (combinatorics) and integration were rerun and passed with these counts:

| Mode | Count |
|---|---|
| Full (seed 1), per configuration | 2,376,661 C++ checks |
| Stress (seed 1) | 22,083,875 |
| Assertion probes | 16 |

The `@reviewer` pass independently probed `fibSearch` on 151,280 shapes.

## Benchmarks

No timing benchmark. `fibSearch` makes at most `log_phi(n + 1) + 1` evaluations, against `2 * ceil(log2(n))` for `ternSearch`; the evaluation-count bounds are the cost evidence. No Barrett/Montgomery or ISA-specific code is used.

## Sources

Inspected on 2026-09-27, plus the 2026-10-07 completeness sweep:

| Reference | Inspected claim and use |
|---|---|
| [cp-algorithms, Binary Search](https://cp-algorithms.com/num_methods/binary_search.html) | Monotone-predicate invariant, absent transitions, continuous bisection and search with powers of two. Independently implemented, with overflow-free midpoint termination, explicit absence and `expSearch` galloping. |
| [cp-algorithms, Ternary Search](https://cp-algorithms.com/num_methods/ternary_search.html) | Strict-side unimodality, equality elimination, integer stopping issues, exact contraction, golden-ratio derivation and evaluation reuse. The discrete API uses neighbouring-point binary minimization instead. |
| [KACTL, GoldenSectionSearch.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/numerical/GoldenSectionSearch.h), Ulf Lundstrom, 2009-04-17, CC0 | Cached-value golden recurrence, and the warning that imprecise ratios can break the order of the interior points. The local code adds ordering and stagnation checks, finite domains and a result status. No code copied. |
| [maspypy, fibonacci_search.hpp](https://github.com/maspypy/library/blob/main/other/fibonacci_search.hpp) | Integer golden-section search reusing one evaluation per step. Adopted as `fibSearch`, rewritten for the closed `lng` domain with `lll` positions, the `f(r)` tail and the leftmost-minimum contract. |
| [maspypy, exp_search.hpp](https://github.com/maspypy/library/blob/main/other/exp_search.hpp) | Galloping from a known-true point without an upper bound. Adopted as `expSearch`, with `ulng` gap arithmetic for the full domain. |
| [cppreference, std::midpoint](https://en.cppreference.com/w/cpp/numeric/midpoint.html) | No overflow, and integer rounding toward the first argument. |

## Limits and handoffs

- `OLD/5-Mathematics/02-search.hpp` and `OLD/Team Notebook/src/math/search.hpp` are unchanged. Their known-true endpoint result, closed integer minimization, real point interfaces and iteration arguments remain available.
- Root finding beyond predicate bisection belongs to `17-numerical_methods.hpp`, and batched parallel binary search to Miscellaneous `20-parallelbinarysearch.hpp`.
- The researcher's log-scale real bisection candidate was not adopted; the reason is in [00-notes.md](00-notes.md).

## History

- 2026-09-27: initial P012 evidence; full mode with seed `20260927` passed before `fibSearch` and `expSearch` existed.
- 2026-10-07: P012 re-audit start, full suite passed on all six headers before any change; re-audit then added `fibSearch` and `expSearch` and fixed findings 7 and 10.
