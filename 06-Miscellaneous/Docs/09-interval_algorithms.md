# 09-interval_algorithms.hpp — Evidence

`09-interval_algorithms.hpp` (batch MI03, package P014) handles intervals with open or closed ends over continuous or integer points: union, events and overlap, stabbing, scheduling, cover, partition and nesting. It reuses the DS Fenwick tree. Every implemented operation of the row has an independent-oracle test (map below); the row's `missing:` operations are not implemented. Prerequisites P002, P006 and P013 are verified.

## Contracts

Common rules: [00-notes.md](00-notes.md), Basic contracts.

### `IntervalDomain`, `Interval`: `bounds`, `empty`, `contains`

Endpoints are `lng` with `l <= r` (asserted in `bounds`). The default is `[l, r)`; each end is open or closed independently. `Continuous` means subsets of the real line, `Integer` subsets of the integers, and every function defaults to `Continuous`. Points use doubled `lll` coordinates (`3` is `1.5`). `bounds` returns closed doubled bounds on the representative grid, not the real endpoints: continuous `(0, 1)` gives `{1, 1}`. Integer mode uses only even grid points. `empty` and `contains` are O(1); `contains` accepts any `lll`, and odd coordinates are never members in Integer mode. Correctness: every nonempty intersection or difference of sets with integral endpoints contains an integer or half-integer point, so comparing closed grid bounds decides disjointness, containment, equality and coverage exactly in both domains.

### Interval Result Records (`IntervalEvent`, `IntervalOverlap`, `IntervalStabbing`, `IntervalSchedule`)

Plain aggregates. `IntervalEvent{x, phase, id, delta}`: phase 0 removes an open right end, 1 adds a closed left end, 2 removes a closed right end, 3 adds an open left end. After a whole phase-1 group the active set is the set at `x`; after phase 3 it is the set just right of `x`. `IntervalOverlap{count, twice}` with `count = 0` has no witness. `IntervalStabbing{possible, twice}` with `possible = false` has no points. `IntervalSchedule{weight, ids}`: an empty schedule is valid.

### `interval_detail`

`normalize` turns an Integer interval into its closed integer hull and reports emptiness. `nonempty` computes each interval's grid bounds once (one assert per interval), returns them in input order and appends empty ids to the caller's vector; callers sort the cached `Bound` records with their own keys.

### `mergeIntervals` and `intervalUnionMeasure`

`mergeIntervals` returns the sorted maximal components of the union, without empty sets. In Integer mode adjacent integers coalesce and the output is closed. Touching continuous intervals coalesce iff at least one includes the shared point. `intervalUnionMeasure` returns the exact continuous length or integer cardinality in `lll`; the full `lng` integer domain has cardinality `2^64`. Correctness: sorting by left bound and extending the current component gives the exact union.

### `intervalEvents` and `maximumIntervalOverlap`

At most `2 * n` events, sorted by `(x, phase, id)`; empty sets give none. Integer events use the closed hull, and only integer coordinates are meaningful. `maximumIntervalOverlap` returns the maximum coverage and one point attaining it; Integer witnesses are even. Correctness: the phases remove excluded right ends before observing a point and add excluded left ends only after it.

### `intervalStabbingCounts(a, twice, domain)`

Coverage at each doubled query point, in query order; any `lll` query. O((n + q) * log(n + 1)) time.

### `minimumIntervalStabbing`

The fewest increasing doubled points hitting every set. Empty input succeeds with no points; any empty input set makes the task impossible (`possible = false`, no partial witness). Correctness: picking the right bound of the first unhit set in finishing order is optimal by exchange.

### `maximumIntervalSchedule` and `weightedIntervalSchedule(a, weight, domain)`

`maximumIntervalSchedule` returns the largest pairwise disjoint subset by original id: empty sets first in input order (they are disjoint from everything), then nonempty sets in finishing order (doubled right bound, then left bound, then id). Touching sets are compatible iff their intersection in the chosen domain is empty. `weightedIntervalSchedule` requires `weight.size() == a.size()` (asserted), accepts any `lng` weights summed exactly in `lll`, and allows the empty schedule. Positive-weight empty sets come first; zero- or negative-weight empty sets are omitted. On a DP tie the later item in finishing order is skipped. Correctness: earliest-finish greedy is optimal by exchange; the weighted form uses `dp[i + 1] = max(dp[i], w_i + dp[pred_i])`.

### `intervalCover(a, target, ids, domain)`

The fewest sets whose union contains `target`; the bounds of `target` are asserted. On success it returns `true` and assigns the witness ids to `ids` in increasing start order (empty for an empty target); otherwise it returns `false` and leaves `ids` unchanged. Coverage is exact for open and closed ends: continuous `[a, 0) ∪ (0, b]` misses `0`, while Integer `[a, 0] ∪ [1, b]` covers `[a, b]`. Correctness: with `cur` the first uncovered grid point, every optimal cover holds a set with `l <= cur` reaching `cur`; exchanging it for the one reaching farthest keeps a cover, and `cur` moves to that reach plus one grid step (2 in Integer mode). If no set with `l <= cur` reaches `cur`, no cover exists.

### `intervalPartitionAssignment(a, domain)`

Returns `{machines, machine}` with `machine[i]` in `[0, machines)` and the sets on one machine pairwise disjoint. `machines` is the maximum overlap, `1` when only empty sets exist (all on machine 0), or `0` for no sets. It is minimal and runs in O(n * log(n + 1)) with a min-heap of finishing bounds. Correctness: sets are processed by start, and a new machine opens only when every busy machine finishes at or after the current start, so they and the current set all contain the start point; the count therefore equals the maximum overlap, a lower bound. Reusing the earliest-finishing machine keeps the invariant.

### `removeNestedIntervals(a, domain)` and `nestedIntervalCounts(a, domain)`

Containment is set containment in the chosen domain. `removeNestedIntervals` keeps `i` iff no other set strictly contains it and no earlier id is an equal set; output is in increasing start order, which is also increasing end order. Empty sets lie inside every set and survive only when no nonempty set exists, and then only the first. `nestedIntervalCounts` returns `res[i] = {other sets inside i, other sets containing i}`; equal sets count in both directions, empty sets lie inside every other set, and an empty set contains only the other empty sets. Correctness: after sorting by `(l asc, r desc, id asc)`, an interval lies in an earlier one iff the running maximum right bound reaches its right bound; equal sets meet this through the earlier id. `nestedIntervalCounts` processes blocks of equal sets in that order: containing sets are earlier blocks with right bound `>= r` (Fenwick suffix), contained sets later blocks with right bound `<= r` (reverse pass, Fenwick prefix), plus the other members of its own block.

## Feature-to-Test Map

| Operation | Oracle and cases |
|---|---|
| `Interval`, `bounds`, `empty`, `contains` | All 24 intervals on endpoints `{-1, 0, 1}` with every flag; direct original-endpoint membership at every probe point; `lng` extremes; reversed asserts |
| `mergeIntervals`, `intervalUnionMeasure` | Membership equality at every probe; disjoint, maximal, sorted and idempotent output; independent cell-integration measure; exact `2^64` |
| `intervalEvents`, `maximumIntervalOverlap`, `intervalStabbingCounts` | Ordering, per-interval lifetimes, per-phase coverage against direct counts, witness coverage, far `lll` and odd Integer queries |
| `minimumIntervalStabbing` | Set-cover DP over probe-point masks; infeasible and empty-input cases |
| `maximumIntervalSchedule`, `weightedIntervalSchedule` | All-subset compatibility from probe conflicts; optimum cardinality and weight; witness order; empty-set policy; tie rules; sums above 64 bits |
| `intervalCover` | Minimum subset over probe masks of target points for four targets per case (an open/closed `[-1, 1]` variant, an empty or singleton target, `a[0]`, the hull): feasibility, size, witness coverage and start order, `ids` unchanged on failure; full-width open gap; Integer adjacency; large touching chain |
| `intervalPartitionAssignment` | Machines equal `max(peak, n > 0)` from the probe peak; per-machine disjointness from conflicts; empties on machine 0; empty-only and empty inputs; large disjoint and identical sets |
| `removeNestedIntervals`, `nestedIntervalCounts` | Containment by probe-mask implication for every pair: kept set, start order and both counts; empty sets; full-width nesting; large identical sets `(n-1, n-1)` |

Hand mutation probes (2026-10-08, quick mode) detected 9 of 9 changed headers: cover step, start test, machine reuse, empty machine count, nested tie, block counts, Fenwick bound, empty counts.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14.7; optimized, checked and ASan/UBSan builds. The 2026-11-10 runs are library-wide (seed 42, beside the other suites, build cache emptied before each cold run); their figures are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS; quick preflight peak 939 MB; optimized 0.3 s compile + 1.8 s run, checked 31.1 s + 11.4 s, ASan-UBSan 36.3 s + 35.0 s; peak 1179 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 958 MB; optimized 0.3 s + 1.9 s, checked 46.8 s + 17.2 s, ASan-UBSan 47.1 s + 32.5 s; peak 1170 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 32.3 s + 0.2 s, checked 35.3 s + 5.9 s; peak 943 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 949 MB; optimized 0.1 s + 2.6 s, checked 11.0 s + 23.4 s, ASan-UBSan 13.6 s + 58.6 s; peak 674 MB |
| 2026-10-08 | `python3 '96-Local Testing/06-Miscellaneous/09-interval_algorithms_tester.py' --mode full --seed 20261008` | PASS in all three builds on g++ and g++-14: 12410 cases per build (6402 exhaustive), large n = 100000, 13 assertion probes |
| 2026-10-08 | `python3 '96-Local Testing/06-Miscellaneous/09-interval_algorithms_tester.py' --mode stress --seed 7` | PASS in 123 s, one round, three builds: 64858 cases (28850 exhaustive), 15000 random, n = 500000 |
| 2026-10-08 | `python3 '96-Local Testing/02-integration.py'` | PASS: 102 standalone and aggregate headers, multi-TU builds, workspace |
| 2026-10-08 | `python3 '96-Local Testing/03-consistency.py'` with `--braces` on the four P014 headers and testers | PASS |

## Benchmarks

No benchmark: no tuned threshold or specialized backend. The P014 sorting/selection benchmark is in [11-sorting_selection.md](11-sorting_selection.md).

## Sources

The code was written independently from the recurrences and proofs here; no external code was copied. Read in the 2026-09-28 audit: cp-algorithms, length of segments union and Fenwick tree; Kleinberg–Tardos weighted interval scheduling slides 7–18; Wikipedia, Interval scheduling. The 2026-10-08 `@researcher` sweep is in [00-sources.md](00-sources.md); it fetched the CSES problem set and the KACTL `various` pages. The cover and partition greedies are proved here, not taken from those pages.

## Limits and Handoffs

- Vector sizes beyond `INT_MAX` are reviewed, not allocated.
- Comparator, callback and window-validity obligations are the caller's and are not asserted.
- Checked builds cap the large interval fixtures (and the sequence fixtures) at 2000, because debug-STL partition checks make repeated `lower_bound` calls linear.
- `39-scheduling.hpp`'s planned `intervalPartitionMachines` left its row as the projected `.first` of `intervalPartitionAssignment` (P079, applied by P254 2026-11-08). Geometry `13`'s `segmentUnionLength` stays beside `intervalUnionMeasure` by decision (2026-10-18, P037, [duplicates.md](<../../00-Guidelines/23-Sweep Ledgers/duplicates.md>)).
- The code uses no `long` or POSIX-only calls.
- Omitted candidates, with reasons in [00-notes.md](00-notes.md) ("P014 omissions"): k-machine schedules; interval jump queries; circular arc cover; ConstantIntervals.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 区间合并, 区间选点, 区间调度, 区间覆盖, 区间分组, 最大不相交区间数, 带权区间调度, 最大区间重叠.

## History

- 2026-09-28: first implementation and audit (MI03, P014); full suite passed.
- 2026-10-08: re-audit baseline, full suite seed 1 passed in three builds before changes.
- 2026-10-08: re-audit added `intervalCover`, `intervalPartitionAssignment`, `removeNestedIntervals` and `nestedIntervalCounts`, 11 findings fixed; full seed 20261008 passed on g++ and g++-14, stress seed 7, integration and consistency passed.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; every library-wide run passed.
