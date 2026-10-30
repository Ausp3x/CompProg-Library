# 22-convexhulltrick.hpp — evidence

Package P032 (batch DS11). New header: a deque hull for slopes arriving at either extreme and the KACTL line container for arbitrary slopes, each with one envelope chosen by `MAX`. No ISA code, reduction or dispatch threshold, so no benchmark is required (Contest profile). Preconditions are `assert`s checked once per call.

## Contracts

### Common domain

Both structures take a value type `T`: `lng` (the default) or a floating type (`static_assert`).

- `T = lng`, exact: lines `m * x + b` with `|m|, |b| < 2^62` (asserted), so slope and intercept differences fit `lng` and products of two differences fit `lll`; `|m * x|` and `|m * x + b|` must stay below `2^63 - 1` at every queried `x` for every stored line, so the negated forms below also fit.
- Floating `T` (user decision 2026-10-28), approximate: `|m|` and `|b|` are 0 or in `[2^-E, 2^E]` with `E = max_exponent / 4` (256 for `double`; asserted by `cht_detail::inDomain`), so differences of slopes and intercepts stay normal and their products in the hull test neither overflow nor underflow; `m * x + b` must be finite and strictly inside the `NONE` limits at every queried `x` for every stored line. The hull test and the breakpoints are rounded, so a line optimal only on an interval about as wide as the rounding error may be dropped; the result is the computed value of a stored line within `8 eps (1 + max(abs(m x) + abs(b)))` of the true optimum (tested; the worst error seen was below `1 eps`).
- `NONE` is `std::numeric_limits<T>::max()` (min) or `lowest()` (max), returned only when no line is stored; a value never equals it. Both structures keep the lower envelope internally and negate lines and results for the other orientation (`MonotoneCHT` negates when `MAX`, `LineContainer` when not `MAX`), so `d` and `s` hold the negated lines in that case.

### MonotoneCHT<MAX = false, T = lng>

- `addLine(m, b)`: the slope must be at least or at most every stored slope (asserted against the hull's extreme slopes; lines that are not on the hull are dominated, so only the hull's extremes matter). Each call gets the next line id (0, 1, …), also when the line is discarded. Equal slopes keep the better intercept (the earlier line on a tie). Amortized O(1): each line is pushed and popped at most once.
- Hull: `d` holds the internal lower hull with strictly decreasing slopes from front to back; a middle line `g` between `f` and `h` is removed when `(h.b - f.b)(f.m - g.m) <= (g.b - f.b)(f.m - h.m)`, in `lll` for `lng` (exact; a line optimal at one point only is removed, its neighbours tie there). Consecutive breakpoints are then strictly increasing, so for any `x` the values along the hull strictly decrease, tie at most once, then strictly increase.
- Neighbouring lines are compared through differences, `(f.m - g.m) x <= g.b - f.b` (`lll` for `lng`, so exact; for floating `T` close slopes and intercepts subtract exactly, so the comparison errs only at the rounding of the difference, not of `m x + b`, and lines whose values coincide in `T` are still ordered).
- `query(x)`: binary search for the first hull line not worse than its successor; returns `{value, line id}`, or `{NONE, -1}`. O(log(n)). Const.
- `queryMonotone(x)`: moves the pointer `ptr` right while the next line is strictly better and left while the previous line is not worse, then returns as `query`. Correct for any `x` (the strict-then-tie-then-strict shape makes a local optimum global, and the two moves cannot alternate); O(1 + distance moved). For `T = lng` and `x` monotone in either direction across calls, the pointer moves O(n + q) in total: insertions shift it by one per popped line and clamp it into the hull. For floating `T` this amortized bound is not proved (rounding can break unimodality), only the per-call bound.
- Ties (`T = lng`): when two hull lines are optimal at `x`, both queries return the front-most one, so `query` and `queryMonotone` always give the same `{value, line id}` (user decision 2026-10-28). For floating `T` the rounded neighbour comparisons need not be monotone along the hull, so the two queries may return different lines, each within the approximate contract.

### LineContainer<MAX = false, T = lng>

- `addLine(m, b)`: any slope; amortized O(log(n)). `s` is a multiset ordered by slope; `p` is the last `x` where the line is optimal (floor division for `lng`, plain division for floating `T`), `INF` for the last line (`LLONG_MAX`, or infinity), and a line whose `p` reaches its successor's is erased (KACTL `LineContainer`).
- `query(x)`: the line with the first `p >= max(x, -INF + 1)`; returns the value or `NONE`. O(log(n)). A worse parallel line inserted first is never erased and keeps `p = -INF` (KACTL keeps it too); every integer breakpoint is at least `LLONG_MIN + 2 = -INF + 1`, so the search skips it and finds the first undominated line, optimal on all `x <= p` including `LLONG_MIN`. For floating `T`, `-INF + 1` is minus infinity and the key is `x`.

### Violated preconditions

Under `NDEBUG` an out-of-bound slope or intercept can overflow the differences (meaningless breakpoints), and a `MonotoneCHT` slope strictly inside the hull's range is pushed at the front, breaking the hull; every loop still pops at most the stored lines and `queryMonotone` moves within the deque, so all calls terminate.

### Legacy mapping

No legacy source (`OLD/[1] algorithms.cpp:661-691`, an unfinished complex-number monotone hull, is accounted for in [21-lichao.md](21-lichao.md#legacy-mapping)). Planned names: `queryWitness`, `query (monotone pointer)`, `query (binary search)` → `query` and `queryMonotone`, both returning `{value, line id}` (user decision 2026-10-28: `query` was the value field of the witness form). Catalog aliases: ei1333 `ConvexHullTrickAddMonotone` `add`, `query`, `query_monotone_inc`, `query_monotone_dec` → `addLine`, `query`, `queryMonotone` (non-destructive, either direction); maspypy `CHT_monotone`, suisen `MonotonicCHT` → `MonotoneCHT`; KACTL, Nyaan `LineContainer`, maspypy `CHT`, suisen `ConvexHullTrick` → `LineContainer`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/22-convexhulltrick_tester.cpp`, entry `22-convexhulltrick_tester.py` (driver `_00_runner.py`; three builds; 15 assertion probes). Every group runs for both orientations; for `lng`, every `queryMonotone` is preceded by a const `query` at the same `x` and must return the identical `{value, line id}`, and the returned line must be the front-most optimal one (steepest optimal internal slope, then the earliest identical line).

| Operation | Group | Oracle |
|---|---|---|
| `MonotoneCHT` `addLine` (front and back, equal slopes, identical and discarded lines), `query` (through `std::as_const`), `queryMonotone` (increasing, decreasing, constant, random and alternating x, interleaved with insertions at both ends) | `edges`, `crossings` (at, before and after integer and fractional breakpoints, negative ones), `concurrent`, `boundary` (`m * x` or `m * x + b` at `±(2^63 - 2)`, both insertion orders), `extremes` (slopes and intercepts up to `±(2^62 - 1)` with x in {-1, 0, 1}; Library Checker bounds), `exhaustive` (every insertion sequence over a small box, 278k sequences in full, each queried at every x of a window), `random` (5000 runs of up to 12 lines plus 2 * 1250 runs of up to 200 in full), `scale` (2 * 10^5 lines and queries, 4 patterns; 3 * 10^5 in stress) | `__int128` brute force over all added lines; the returned id attains the value; no earlier identical line; a tester-local Li Chao tree at scale (spot-checked against the brute force); total pointer moves at most 3(n + q) + 3 |
| `LineContainer` `addLine` (random, ascending, descending, few distinct and repeated slopes, alternating ends), `query` (including `LLONG_MIN` on a min and a max container) | The same groups | The same brute force; the Li Chao check at scale |
| Tie rule (`lng`) | `ties` (y = x and y = -x at 0, concurrent triples, ties at breakpoints, pointer approaching from both sides) | Expected front-most line id |
| Floating `T` (`double`, `long double`): `addLine`, `query`, `queryMonotone` for both structures | `floating edges`, `floating random` (40 runs of up to 30 lines quick, 400 of up to 80 full; a quarter with integer inputs; x up to 10^6, 1000 or 1), `floating scale` (3000 quick, 10^5 full) | Brute force in `long double` (for `double`) or `_Float128` (for `long double`) within `8 eps (1 + max(abs(m x) + abs(b)))`, eight times the worst error seen; each returned id's line evaluates in `T` exactly to the returned value; a `_Float128` Li Chao oracle at scale |
| `NONE`, `{NONE, -1}` when empty | `edges`, `floating edges` | Literal values (`max()`, `lowest()` for floating `T`) |
| Preconditions | `invalid` | `MonotoneCHT` slope and intercept at `±2^62`, slope strictly inside the hull range (min and max), floating slope `1e200` and intercept `1e-200` outside `[2^-256, 2^256]`; `LineContainer` slope and intercept at `±2^62`, `long double` slope `1e1300` |

Quick runs a box of 2 at depth 3/2, 300 random runs and 3000 lines at scale.

## Commands and results

Latest run 2026-10-28, Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `22-convexhulltrick_tester.py --mode quick` | PASS, 2 configurations, 1,855,313 checks per configuration, 15 assertion probes, MEMORY peak 579 MB |
| `22-convexhulltrick_tester.py --mode full --seed 20261028` (GCC 16.2.1) | PASS, 3 configurations, 36,223,679 checks per configuration, 15 assertion probes, MEMORY peak 729 MB |
| `CXX=g++-14 22-convexhulltrick_tester.py --mode full --seed 20261028` (GCC 14.4.1, floor) | PASS, 3 configurations, 36,223,679 checks per configuration, 15 assertion probes, MEMORY peak 731 MB |
| `22-convexhulltrick_tester.py --mode stress --seed 7` | PASS, 3 configurations, 39,701,198 checks per configuration, 15 assertion probes, MEMORY peak 724 MB; seeds 1 to 5 optimized configuration also PASS |
| `02-integration.py` | PASS, 178 standalone and aggregate headers, multi-TU, workspace; MEMORY peak 1592 MB (cap 8192 MB) |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/22-convexhulltrick_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/22-convexhulltrick_tester.py' --mode full --seed 20261028
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/22-convexhulltrick_tester.py' --mode full --seed 20261028
python3 '96-Local Testing/02-Data Structures/22-convexhulltrick_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| KACTL `LineContainer.h` | Multiset with mutable breakpoints, floor-division intersections, the erase cascade. |
| ei1333 `convex-hull-trick-add-monotone`; maspypy `cht_monotone`; suisen `monotonic_convex_hull_trick`; cp-algorithms *Convex hull trick*; OI Wiki *斜率优化* | Deque insertion at either end, monotone queries in both directions, exact cross-multiplied removal test. |

Fetched 2026-10-28 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-28-p032-ds11-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p032-omissions): rollback CHT, `empty`/`size`/`clear`, hull merging, `CHT_XY`, envelope pieces.
- `LineContainer::query` returns the value only (user decision 2026-10-28).
- Geometry row 41 `lowerEnvelope` against this row is an open question of P139 (`duplicates.md`).
- On aarch64 `long double` is already binary128, so the `_Float128` brute force for `long double` lines has no extra precision there; the `8 eps (1 + mag)` tolerance was calibrated on x87 and passed the aarch64 quick run.

## History

- 2026-10-28: P032 DS11 first implementation; the tester found the `LLONG_MIN` query defect and the independent review the narrow value domain of the negated side (both fixed, `boundary` group added) and two style findings; full (seed 20261028) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-28: `/finish` DS11: floating `T` with an asserted magnitude domain, the front-most tie rule for `lng` (user decisions), tolerance tightened to `8 eps (1 + mag)`; floating stress seed 7 failed when two hull lines with slopes `3e-10` apart evaluated to the same `double`, fixed by comparing neighbours through slope and intercept differences (`notWorse`); quick, full on g++ and g++-14 and stress (seeds 1 to 5 and 7) passed.
- 2026-10-30: SUP18 arm64 portability (user decision 2026-10-30, fixed in P250): the floating oracle type `__float128` became `_Float128`, which aarch64 g++ also provides; x86-64 output unchanged; quick passed on x86-64 and on aarch64 under qemu-user, full passed on g++ and g++-14.
