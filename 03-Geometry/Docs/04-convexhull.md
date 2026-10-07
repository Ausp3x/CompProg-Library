# 04-convexhull.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `01-point.hpp`, `02-line_segment.hpp` and `03-polygon.hpp`); see [00-notes.md](00-notes.md#p007--ge01-package-record) for the package record and the folder numeric policy. The 2026-10-07 re-audit added `convexHullIndices` from the completeness sweep. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies. The hulls are integral-only because floating rounding can break a sort's strict weak ordering. They accept coordinates wider than `10^9` as long as every intermediate fits `lll`.

### convexHull, convexHullIndices, convexHullGraham

Exact integral hulls, `n <= INT_MAX`. The orientation and squared-distance intermediates must fit `lll`, which `|coordinate| <= 10^9` guarantees. Each function works on a copy and removes duplicates. The output starts at the lexicographic minimum, runs CCW and never repeats its start. `keep_collinear` keeps every distinct boundary input point. For an entirely collinear set, the output is the two sorted endpoints, or every sorted distinct point when collinear points are kept. `convexHullIndices` returns the same cycle as `convexHull`, with each point named by its first occurrence in the input. Graham is the angular-scan alternative. Its integral angle comparison avoids non-transitive floating predicates.

Correctness and cost: monotone chain maintains the lower and upper hulls by removing turns that violate convexity. Each item enters and leaves each stack at most once. `hull_detail::chain` runs this over a vector of points or of indices through an accessor, so `convexHull` and `convexHullIndices` share one implementation. Ordering indices by `(point, index)` and then removing duplicates by point keeps the first occurrence. Graham scan uses an extreme pivot, exact angular order and a final-ray reversal when keeping collinear boundary points. All three hulls take `O(n * log(n))` time and `O(n)` storage including the output.

## Feature-to-test map

`96-Local Testing/03-Geometry/04-convexhull_tester.py` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses. Full mode builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them. The entry resolves paths from its own location.

| Operation | Test | Oracle |
|---|---|---|
| `convexHull`, `convexHullGraham` (both `keep_collinear` policies) | Canonical orientation and start; duplicates; empty and singleton input; all-collinear input and both retention policies; the caller's input unchanged; exhaustive 3×3-grid subsets and seeded adversarial point sets | Independent supporting-edge (Jarvis) oracle; agreement between hull implementations is supplemental |
| `convexHullIndices` | Index ranges, first-duplicate indices, and agreement with the oracle hull on every case, including 20,001-point parabolas | Independent supporting-edge (Jarvis) oracle |

Finite tests supplement the arguments above; they do not prove correctness for all inputs.

## Commands and results

### 2026-09-27 (original P007)

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (`20260810`), Python 3.14.7, GNU++20. GCC 14 was unavailable. Full mode with seed `20260927` passed every configuration with 1,151,038 checks. The 2026-10-07 baseline rerun before the re-audit edits reproduced exactly this check count.

### 2026-10-07 re-audit

Environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 was unavailable, so these are GCC 16 runs; no GCC 14 run is claimed. No online submission was made.

| Entry | Full, seed `20260927` (optimized, checked, ASan/UBSan) | Stress, seed `42`, optimized |
|---|---|---|
| `04-convexhull_tester.py` | PASS, 1,292,646 checks per configuration | PASS, 10,264,103 checks |

The stress round ran before the review fixes, which did not change behavior in this header; the full run was repeated after them. Also run (package-wide):

- The sanitized integration (102 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace) passed.
- A two-translation-unit smoke that mixes all four GE01 headers passed.
- A warning sweep instantiated every operation for `int8_t`, `int16_t`, `int`, `lng`, `lll`, `float`, `double` and `long double` under `-Wall -Wextra -Wconversion`. The header produced no warnings.
- The brace and comment-cap check and repository consistency report no errors.

```bash
python3 '96-Local Testing/03-Geometry/04-convexhull_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/04-convexhull_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

### Re-audit findings and their disposition (2026-10-07)

The re-audit compared every operation in the row against the code and tests before making changes.

| # | Finding | Disposition |
|---|---|---|
| 3, 4 | Evidence claimed no gaps while rows were partial | Superseded: every operation is implemented and mapped above; status and handoff text rewritten |
| 7 | Free functions lack complexity comments | Every function sits under a complexity line, one per group of functions with the same bound (the restyle convention). Contracts moved to `## Contracts` |
| 10 | `; }` closings | Header, C++ tester and the benchmark normalized; `03-consistency.py --braces` is clean |

Other changes: the multi-line contract comment blocks moved out of the header into `## Contracts`, and the header meets the comment cap. The monotone chain was factored into `hull_detail::chain`, which `convexHull` and `convexHullIndices` share. `convexHull`'s behavior is unchanged, and its tester and benchmark outputs match. No public name or documented behavior changed.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/03-Geometry/00-foundations_benchmark.py>) and `96-Local Testing/03-Geometry/00-foundations_benchmark.json` compare monotone chain and Graham scan at 32, 2,048 and 32,768 points. The inputs are random coordinates, duplicate-heavy grids, collinear points and parabolic convex-position points, each with both retention policies: 48 measurements. Each measurement uses one warmup and five timed calls and reports the median. The timing includes the input copy and output allocation, and every complete canonical output is checked outside the timed interval. Compiler flags are `-std=gnu++20 -O2 -DNDEBUG`. The record includes the seed, CPU and compiler, input generation settings, code hashes and an observed checksum. Both algorithms use `O(n)` storage; no peak-RSS claim is made.

The measurements support monotone chain as the default, and Graham remains an explicit alternative. Collinear inputs share preprocessing, so their small timing differences are noise-sensitive. Timings are observations on a shared machine, not universal speed claims or pass/fail gates. The table below gives medians at 32,768 points without collinear retention.

| Distribution | Monotone chain 2026-10-07 | Graham scan 2026-10-07 | Monotone chain 2026-09-27 | Graham scan 2026-09-27 |
|---|---|---|---|---|
| Random | 3.68 ms | 8.05 ms | 3.52 ms | 7.77 ms |
| Duplicate-heavy grid | 2.95 ms | 4.63 ms | 2.83 ms | 4.51 ms |
| Collinear | 2.21 ms | 2.21 ms | 2.15 ms | 2.10 ms |
| Convex parabola | 2.61 ms | 4.76 ms | 2.43 ms | 4.44 ms |

The 2026-10-07 rerun measured the shared `hull_detail::chain` refactor. Both algorithms moved by a similar 3–7%, including Graham, whose code did not change, and the monotone/Graham ratio held: 0.457 against 0.453 on random, 0.548 against 0.548 on parabola. The shift is attributed to machine state, not to the refactor. The record's hashes match the current point, hull and benchmark sources.

```bash
python3 '96-Local Testing/03-Geometry/00-foundations_benchmark.py' --seed 20260927
```

## Sources

The implementation was written independently from the algorithms below. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The 2026-10-07 completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (hull). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [cp-algorithms: Convex hull construction](https://cp-algorithms.com/geometry/convex-hull.html), inspected September 27, 2026.

## Limits and handoffs

No failing reproducer and no unfinished owned operation. Higher-dimensional hulls belong to P137 / GE12. Chan's output-sensitive hull is a separately scoped variant in row 41.
