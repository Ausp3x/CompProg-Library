# 09-interval_algorithms.hpp — evidence

`09-interval_algorithms.hpp` (batch MI03, package P014) covers interval domains with open/closed ends over continuous or integer points, union, events and overlap, stabbing, scheduling, cover, partition and nesting. It reuses the DS Fenwick. Every operation in the row has an independent-oracle test (map below). Prerequisites P002, P006 and P013 are verified. No online submission was made.

## Contracts

Common rules: sizes are below `INT_MAX`. Indices are zero-based and ranges half-open. Inputs are borrowed and never mutated unless a contract says so. Ties are deterministic but otherwise arbitrary. Comparators are pure strict weak orders, and equality callbacks are pure equivalences. Projections and predicates are pure and depend on element values, not addresses or positions. Callback exceptions propagate, with no rollback after mutation has begun. Each bound counts a comparison, projection, copy, move or `Count` operation as O(1).

### `IntervalDomain`, `Interval`: `bounds`, `empty`, `contains`

Endpoints are `lng` with `l <= r` (asserted in `bounds`). The default is `[l, r)`, and each end is open or closed independently. `Continuous` means subsets of the real line and `Integer` subsets of the integers. Every function defaults to `Continuous`. Points use doubled `lll` coordinates (`3` is `1.5`). `bounds` returns closed doubled bounds on the representative grid, not the real endpoints: continuous `(0, 1)` gives `{1, 1}`. Integer mode uses only even grid points. `empty` and `contains` are O(1). `contains` accepts any `lll`; odd coordinates are never members in Integer mode. Correctness: every nonempty intersection or difference of sets with integral endpoints contains an integer or half-integer point, so comparing closed grid bounds decides disjointness, containment, equality and coverage exactly in both domains.

### Interval result records (`IntervalEvent`, `IntervalOverlap`, `IntervalStabbing`, `IntervalSchedule`)

These are plain aggregates. `IntervalEvent{x, phase, id, delta}`: phase 0 removes an open right end, 1 adds a closed left end, 2 removes a closed right end and 3 adds an open left end. After a whole phase-1 group, the active set is the set at `x`. After phase 3, it is the set just to the right of `x`. `IntervalOverlap{count, twice}` with `count = 0` has no witness. `IntervalStabbing{possible, twice}`: `possible = false` comes with no points. `IntervalSchedule{weight, ids}`: an empty schedule is a valid result.

### `interval_detail`

`normalize` turns an Integer interval into its closed integer hull and reports emptiness. `nonempty` computes each interval's grid bounds exactly once (one assert per interval). It returns them in input order and appends empty ids to the caller's vector, and callers sort the cached `Bound` records with their own keys.

### `mergeIntervals` and `intervalUnionMeasure`

`mergeIntervals` returns the sorted maximal components of the union, without empty sets. In Integer mode, adjacent integers coalesce and the output is closed. Touching continuous intervals coalesce iff at least one of them includes the shared point. `intervalUnionMeasure` returns the exact continuous length or integer cardinality in `lll`. The full `lng` integer domain has cardinality `2^64`. Correctness: sorting by left bound and extending the current component gives the exact union.

### `intervalEvents` and `maximumIntervalOverlap`

There are at most `2 * n` events, sorted by `(x, phase, id)`. Empty sets give none. Integer events use the closed hull, and only integer coordinates are meaningful. `maximumIntervalOverlap` returns the maximum coverage and one point attaining it; Integer witnesses are even. Correctness: sweep phases remove excluded right ends before observing a point and add excluded left ends only after it.

### `intervalStabbingCounts(a, twice, domain)`

This returns the coverage at each doubled query point, in query order. Any `lll` query is allowed. It runs in O((n + q) * log(n + 1)) time.

### `minimumIntervalStabbing`

This returns the fewest increasing doubled points hitting every set. Empty input succeeds with no points. Any empty input set makes the task impossible (`possible = false`, no partial witness). Correctness: picking the right bound of the first unhit set in finishing order is optimal by exchange.

### `maximumIntervalSchedule` and `weightedIntervalSchedule(a, weight, domain)`

`maximumIntervalSchedule` returns the largest pairwise disjoint subset, by original id. Empty sets are disjoint from everything and come first in input order, followed by nonempty sets in finishing order (doubled right bound, then left bound, then id). Touching sets are compatible iff their intersection in the chosen domain is empty. `weightedIntervalSchedule` requires `weight.size() == a.size()` (asserted) and accepts any `lng` weights, summed exactly in `lll`. The empty schedule is allowed. Positive-weight empty sets come first, and zero- or negative-weight empty sets are omitted. On a DP tie, the later item in finishing order is skipped. Correctness: earliest-finish greedy is optimal by exchange; weighted scheduling uses `dp[i + 1] = max(dp[i], w_i + dp[pred_i])`.

### `intervalCover(a, target, ids, domain)`

This finds the fewest sets whose union contains `target`. The bounds of `target` are asserted. On success it returns `true` and assigns the witness ids to `ids`, ordered by increasing start, with an empty witness for an empty target. When no cover exists it returns `false` and leaves `ids` unchanged. Coverage is exact for open and closed ends. For example, continuous `[a, 0) ∪ (0, b]` misses `0`, while Integer `[a, 0] ∪ [1, b]` covers `[a, b]`. Correctness: with the first uncovered grid point `cur`, any optimal cover contains a set with `l <= cur` reaching `cur`; exchanging it for the one reaching farthest keeps a cover, and `cur` moves to that reach plus one grid step (2 in Integer mode). If no set with `l <= cur` reaches `cur`, no cover exists.

### `intervalPartitionAssignment(a, domain)`

This returns `{machines, machine}`, where `machine[i]` is in `[0, machines)` and sets on one machine are pairwise disjoint. `machines` equals the maximum overlap, or `1` when only empty sets exist (they all use machine 0), or `0` for no sets. It is minimal and runs in O(n * log(n + 1)) time using a min-heap of finishing bounds. The planned `39-scheduling.hpp` must forward here. Correctness: sets are processed by start. A new machine opens only when every busy machine's finish is at or after the current start, so all of them and the current set contain the start point; the machine count equals the maximum overlap, a lower bound. Reusing the earliest-finishing machine keeps the invariant.

### `removeNestedIntervals(a, domain)` and `nestedIntervalCounts(a, domain)`

Containment is set containment in the chosen domain. `removeNestedIntervals` keeps `i` iff no other set strictly contains it and no earlier id is an equal set. Output is in increasing start order, which is also increasing end order. Empty sets lie inside every set. They survive only when no nonempty set exists, and then only the first one. `nestedIntervalCounts` returns `res[i] = {number of other sets inside i, number of other sets containing i}`. Equal sets count in both directions. Empty sets are inside every other set, and an empty set contains only the other empty sets. Correctness: after sorting by `(l asc, r desc, id asc)`, an interval is contained in an earlier one iff the running maximum right bound reaches its right bound; equal sets meet this through the earlier id. `nestedIntervalCounts` processes blocks of equal sets in this order: containing sets are earlier blocks with right bound `>= r` (Fenwick suffix), contained sets are later blocks with right bound `<= r` (reverse pass, Fenwick prefix), plus the other members of its own block.

## Feature-to-test map

Every oracle is independent of the implementation and uses non-removable checks. Tests run in the optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (the last only in full and stress). Assertion probes run as SIGABRT subprocesses in the checked build.

| Operation | Oracle and cases |
|---|---|
| `Interval`, `bounds`, `empty`, `contains` | All 24 intervals on endpoints `{-1, 0, 1}` with every flag; direct original-endpoint membership at every probe point; `lng` extremes; reversed asserts |
| `mergeIntervals`, `intervalUnionMeasure` | Membership equality at every probe, disjoint, maximal, sorted and idempotent output; independent cell-integration measure; exact `2^64` |
| `intervalEvents`, `maximumIntervalOverlap`, `intervalStabbingCounts` | Ordering, per-interval lifetimes, per-phase coverage against direct counts, witness coverage, far `lll` and odd Integer queries |
| `minimumIntervalStabbing` | Set-cover DP over probe-point masks; infeasible and empty-input cases |
| `maximumIntervalSchedule`, `weightedIntervalSchedule` | All-subset compatibility from probe conflicts; optimum cardinality and weight; witness order; empty-set policy; tie rules; sums above 64 bits |
| `intervalCover` | Minimum subset over probe masks of target points, for four targets per case (an open/closed `[-1, 1]` variant, an empty or singleton target, `a[0]`, the hull); feasibility, size, witness coverage and start order, `ids` unchanged on failure; full-width open gap; Integer adjacency; large touching chain |
| `intervalPartitionAssignment` | Machines equal `max(peak, n > 0)` from the probe peak; pairwise disjointness per machine from conflicts; empties on machine 0; empty-only and empty inputs; large disjoint and identical sets |
| `removeNestedIntervals`, `nestedIntervalCounts` | Containment by probe-mask implication for every pair: kept set, start order and both counts; empty sets; full-width nesting; large identical sets `(n-1, n-1)` |

Mutation probes (2026-10-08, quick mode): 9 of 9 changed headers were detected (cover step, start test, machine reuse, empty machine count, nested tie, block counts, Fenwick bound, empty counts).

## Commands and results

GCC 16.2.1 20260810, Python 3.14.7, Linux x86-64, i9-11900H, 2026-10-08. The commands, floor run and integration are shared with the other P014 suites and recorded in [08-sequence_algorithms.md](08-sequence_algorithms.md).

- Full: `python3 '96-Local Testing/06-Miscellaneous/09-interval_algorithms_tester.py' --mode full --seed 20261008`: PASS (table below); with `CXX=g++-14` (GCC 14.4.1): PASS in all three configurations.
- Stress, one round: `python3 '96-Local Testing/06-Miscellaneous/09-interval_algorithms_tester.py' --mode stress --seed 7`: PASS.

| Suite | Full, seed 20261008 (optimized, checked, ASan/UBSan) | Stress, seed 7, one round (same three) | Probes |
|---|---|---|---|
| `09-interval_algorithms` | PASS: 12410 cases per configuration (6402 exhaustive), large n = 100000 | PASS in 123 s: 64858 cases (28850 exhaustive), 15000 random, n = 500000 | 13 |

## Benchmarks

No benchmark: no tuned threshold or specialized backend. The P014 sorting/selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

The code was written independently from the recurrences and proofs in the package; no external code was copied. Sources read in the 2026-09-28 audit: cp-algorithms length of segments union and Fenwick tree; Kleinberg–Tardos weighted interval scheduling slides 7–18; Wikipedia, Interval scheduling. The 2026-10-08 `@researcher` sweep is recorded in [00-sources.md](00-sources.md); its agents fetched the CSES problem set and KACTL `various` pages. The cover and partition greedies are proved here rather than taken from those pages.

## Limits and handoffs

- Finite corpora support the proofs above; they are not proofs themselves. Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's responsibility and are not asserted.
- Checked builds cap the large interval fixtures (and the sequence fixtures) at 2000, because debug-STL partition checks make repeated `lower_bound` calls linear.
- `39-scheduling.hpp` must forward `intervalPartitionMachines` to `intervalPartitionAssignment`. Geometry `13`'s `segmentUnionLength` should reuse `intervalUnionMeasure` or record why not.
- GCC 14.2 exactly and Codeforces' Windows MSYS2 build were not run; the code uses no Windows-sensitive types or POSIX calls.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): k-machine schedules; interval jump queries; circular arc cover; ConstantIntervals.

## History

- 2026-09-28: first implementation and audit (MI03, P014); full suite passed.
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before changes; re-audit then added `intervalCover`, `intervalPartitionAssignment`, `removeNestedIntervals` and `nestedIntervalCounts` (`/reaudit-review` findings 1, 3, 6–12, 15, 17 fixed).
