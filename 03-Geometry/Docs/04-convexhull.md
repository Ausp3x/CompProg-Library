# 04-convexhull.hpp — evidence

Owned by package P007 / GE01 (geometry foundations, together with `01-point.hpp`, `02-line_segment.hpp` and `03-polygon.hpp`); see [00-notes.md](00-notes.md#p007--ge01-package-record) for the package record and the folder numeric policy. Every operation in the row has a test.

## Contracts

### Numeric policy

The GE01 numeric policy in [00-notes.md](00-notes.md#ge01-numeric-policy-p007) applies. The hulls are integral-only because floating rounding can break a sort's strict weak ordering. They accept coordinates wider than `10^9` as long as every intermediate fits `lll`.

### convexHull, convexHullIndices, convexHullGraham

Exact integral hulls, `n <= INT_MAX`. The orientation and squared-distance intermediates must fit `lll`, which `|coordinate| <= 10^9` guarantees. Each function works on a copy and removes duplicates. The output starts at the lexicographic minimum, runs CCW and never repeats its start. `keep_collinear` keeps every distinct boundary input point. For an entirely collinear set, the output is the two sorted endpoints, or every sorted distinct point when collinear points are kept. `convexHullIndices` returns the same cycle as `convexHull`, with each point named by its first occurrence in the input. Graham is the angular-scan alternative. Its integral angle comparison avoids non-transitive floating predicates.

Correctness and cost: monotone chain maintains the lower and upper hulls by removing turns that violate convexity. Each item enters and leaves each stack at most once. `hull_detail::chain` runs this over a vector of points or of indices through an accessor, so `convexHull` and `convexHullIndices` share one implementation. Ordering indices by `(point, index)` and then removing duplicates by point keeps the first occurrence. Graham scan uses an extreme pivot, exact angular order and a final-ray reversal when keeping collinear boundary points. All three hulls take `O(n * log(n))` time and `O(n)` storage including the output.

## Feature-to-test map

`96-Local Testing/03-Geometry/04-convexhull_tester.py` provides quick, full and stress modes, deterministic seeds, oracles that survive `-DNDEBUG`, and assertion-failure subprocesses. Full mode builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and AddressSanitizer/UndefinedBehaviorSanitizer configurations. Quick mode omits the sanitizers and shrinks the exhaustive and random corpora; stress mode extends them.

| Operation | Test | Oracle |
|---|---|---|
| `convexHull`, `convexHullGraham` (both `keep_collinear` policies) | Canonical orientation and start; duplicates; empty and singleton input; all-collinear input and both retention policies; the caller's input unchanged; exhaustive 3×3-grid subsets and seeded adversarial point sets | Independent supporting-edge (Jarvis) oracle; agreement between hull implementations is supplemental |
| `convexHullIndices` | Index ranges, first-duplicate indices, and agreement with the oracle hull on every case, including 20,001-point parabolas | Independent supporting-edge (Jarvis) oracle |

Finite tests supplement the arguments above; they do not prove correctness for all inputs.

## Commands and results

2026-10-07, Linux x86-64, Intel Core i9-11900H, GCC 16.2.1, Python 3.14.7, GNU++20. GCC 14 was unavailable; no GCC 14 run is claimed.

| Command | Result |
|---|---|
| `04-convexhull_tester.py --mode full --seed 20260927` (optimized, checked, ASan/UBSan) | PASS, 1,292,646 checks per configuration |
| `04-convexhull_tester.py --mode stress --seed 42 --configuration optimized` | PASS, 10,264,103 checks |
| `02-integration.py --sanitizers` (102 headers, scalar and AVX2 multi-TU builds, workspace) | PASS |
| Two-translation-unit smoke mixing all four GE01 headers | PASS |
| Warning sweep, every operation for all eight `T`, `-Wall -Wextra -Wconversion` | No warnings |
| `03-consistency.py --braces ...` and `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/03-Geometry/04-convexhull_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/03-Geometry/04-convexhull_tester.py' --mode stress --seed 42 --configuration optimized
python3 '96-Local Testing/01-run.py' --mode quick --filter 03-Geometry --seed 20260927 --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py' --braces 03-Geometry/0[1-4]*.hpp '96-Local Testing/03-Geometry/'0[1-4]-*.cpp
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/03-Geometry/00-foundations_benchmark.py>) and `96-Local Testing/03-Geometry/00-foundations_benchmark.json` compare monotone chain and Graham scan at 32, 2,048 and 32,768 points. The inputs are random coordinates, duplicate-heavy grids, collinear points and parabolic convex-position points, each with both retention policies: 48 measurements. Each measurement uses one warmup and five timed calls and reports the median. The timing includes the input copy and output allocation, and every complete canonical output is checked outside the timed interval. Compiler flags are `-std=gnu++20 -O2 -DNDEBUG`. The record includes the seed, CPU and compiler, input generation settings, code hashes and an observed checksum. Both algorithms use `O(n)` storage; no peak-RSS claim is made.

The measurements support monotone chain as the default, and Graham remains an explicit alternative. Collinear inputs share preprocessing, so their small timing differences are noise-sensitive. Timings are observations on a shared machine, not universal speed claims or pass/fail gates. Medians at 32,768 points without collinear retention, 2026-10-07:

| Distribution | Monotone chain | Graham scan |
|---|---|---|
| Random | 3.68 ms | 8.05 ms |
| Duplicate-heavy grid | 2.95 ms | 4.63 ms |
| Collinear | 2.21 ms | 2.21 ms |
| Convex parabola | 2.61 ms | 4.76 ms |

```bash
python3 '96-Local Testing/03-Geometry/00-foundations_benchmark.py' --seed 20260927
```

## Sources

The implementation was written independently from the algorithms below. Notebook implementations were reviewed as comparison evidence and not copied as correctness guarantees. The completeness sweep sources are in [00-sources.md](00-sources.md#p007-completeness-sweep-fetched-2026-10-07); candidates not adopted are in [00-notes.md](00-notes.md#p007-completeness-decisions-2026-10-07).

- [KACTL team notebook PDF](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/kactl.pdf), saved November 24, 2024 edition: printed pp. 16–18 (hull). The relevant snippets were read for operations, overflow warnings and degeneracy conventions.
- [cp-algorithms: Convex hull construction](https://cp-algorithms.com/geometry/convex-hull.html), inspected September 27, 2026.

## Limits and handoffs

Higher-dimensional hulls belong to P137 / GE12. Chan's output-sensitive hull is a separately scoped variant in row 41.

## History

- 2026-09-27: original P007, full suite (seed 20260927, 1,151,038 checks) and benchmark passed on g++ 16.
- 2026-10-07: re-audit, added `convexHullIndices` and the shared `hull_detail::chain`; 3 findings fixed (complexity comments, braces, comment cap); full suite, stress and benchmark rerun on g++ 16.
