# 14-segtreebeats.hpp — Evidence

Package P028 (batch DS07). The header replaces the unchanged extraction `SegTreeBeats` (`OLD/algorithms.cpp:2858–3183`). Indices are zero-based, ranges half-open, preconditions are `assert`s checked once per operation, and queries are non-const because they push pending tags.

## Contracts

### SegTreeBeats

- Construction: `SegTreeBeats(int N)` (zeros) or `SegTreeBeats(const vector<lng> &)`, `0 <= n <= 2^29`, O(n). Storage is `2 * bit_ceil(n)` nodes of 72 bytes (88 for `HistoricSegTree`).
- Domain: `n * max |a_i| < 2^62` at every moment, so sums, `max - min`, negations and the `lng min` "no second value" sentinel never overflow or collide. `sqrtUpdate` and `modUpdate` require every value in the range to be `>= 0` (asserted with one `minQuery`). `divideUpdate(l, r, q)` needs `q >= 1` (`q = 1` is an O(log n) no-op walk) and rounds toward minus infinity. `chminUpdate` and `chmaxUpdate` assert that `x` is not `lng min`, the sentinel. `modUpdate(l, r, m)` needs `m >= 1`.
- Queries: `sumQuery`, `maxQuery`, `minQuery`; an empty range returns `0`, `lng min`, `lng max`, which no in-domain value reaches.
- Updates: `addUpdate`, `setUpdate`, `chminUpdate` (`a_i = min(a_i, x)`), `chmaxUpdate`, `divideUpdate` (`floor(a_i / q)`), `sqrtUpdate` (`floor(sqrt(a_i))`), `modUpdate` (`a_i mod m`).
- Representation: each node holds `sum`, a pending `add`, and two sides `{largest, second largest or lng min, count of largest}`: `hi` for the values and `lo` for the negated values, so `chmax(x)` is `cap` on `lo` with `-x` and the max and min code is shared. A node whose max equals its min overwrites both children on push, which subsumes a pending assignment: once uniform, a node stays uniform until it is pushed, so no assignment tag is needed. Otherwise push adds the pending `add` and caps the children to the parent's max and min (the beats invariant guarantees each cap is above the child's second value).
- Value-domain rule (`settle`): for `f` = floor division, integer square root or modulo, `f(x) - x` is nonincreasing, so equal shifts at a node's max and min mean every element shifts by the same amount (`add`); for monotone `f` (division and square root) equal images mean the node becomes uniform (`set`). Otherwise the walk descends. Leaves always settle, and the shared walk treats a leaf as finished even if a violated precondition makes its update refuse, so every walk terminates.
- Amortized bounds (V is the value range):
  - add and set: O(log n) worst case.
  - chmin and chmax mixed with add and set: O(log^2 n) amortized (Ji 2016, as summarized by OI Wiki and smijake3); O(log n) amortized without add and set.
  - divide with add, set, chmin, chmax: O((n + q log n) log V) total. Potential: the sum over nodes of `log2(max - min + 1)`. A node that does not settle has spread `s >= 2` (spread 1 always settles), and its spread drops to at most `ceil(s / q) <= (s + 1) / 2`; add and set raise the potential only on the O(log n) partial nodes; chmin, chmax and division never raise any node's spread.
  - sqrt with the same mix: O((n + q log n) log log V) total by the same argument on `log log` of the spread: spread 1 always settles, spread 2 drops to 1 (two squares never lie within distance 2 of each other above 0), and spread `s >= 3` drops to at most `floor(sqrt(s)) + 1 < s`.
  - mod with set, chmin, divide, sqrt: O((n + q) log n log V) total. Potential: the sum over maximal runs of equal adjacent values of `log2(v + 1)`. A visited node that does not settle contains a run with value `>= m` and a run boundary, so it is an ancestor of a boundary of a run whose value at least halves (`v mod m < v / 2` when `m <= v`); each boundary has O(log n) ancestors. set, chmin, divide and sqrt never raise a run's value and create O(1) new runs.
  - mod mixed with add or chmax has no amortized bound: alternating `m, m - 1` values cycle through mod m, add 1, mod m, add m - 1 at O(n) per call. Results stay correct.
- The legacy extraction used inclusive ranges, silently ignored out-of-range calls, needed a `std::span<const T> &` lvalue and kept a separate assignment tag; the contract is now half-open with asserted ranges.

### HistoricSegTree

- `0 <= n <= 2^29`; values, historic values and `time * sum` fit `lng`. `time` starts at 0.
- `addUpdate(l, r, x)`; `tick()` adds every current value to its historic sum, in O(1); `sumQuery`, `maxQuery`, `minQuery`; `historicMaxQuery` and `historicMinQuery` return the largest and smallest value each element has held, including its initial value and every state after an `addUpdate`; `historicSumQuery` is the sum over ticks of the value held at that tick.
- Representation: the pending tags are the net `add`, the largest prefix `up >= 0` and smallest prefix `down <= 0` of the pending adds, and `cadd`. `csum` stores `sum(H_i - time * a_i)`, so an add of `x` at time `T` adds `-T * x` to each `C_i` and a tick changes nothing in the tree. All operations are O(log n) worst case.
- Range assignment and chmin are not supported (see Limits and Handoffs).

### LazySegmentTreeBeats<A> and the BeatsMonoid Contract

- A BeatsMonoid is an acted monoid in the sense of [00-monoids.md](00-monoids.md#monoid-contract) (`S`, `F`, `op`, `e`, `mapping`, `composition` = new after old, `id`) plus `fail(const S &)` (static or a `const` member), stored as `m` and passed last (`LazySegmentTreeBeats(N = 0, mon = {})`, `LazySegmentTreeBeats(vector<S>, mon = {})`). `mapping(f, x)` may return a value with `fail` true when it cannot summarize a node of two or more elements; the tree then composes `f` into the node's tag, pushes it to the children and recomputes the node. Requirements: `mapping` never fails on a single element or on `e()`, `mapping(id(), x)` never fails, and the failed value is discarded.
- `0 <= n <= 2^29`; `get`, `set`, `apply(l, r, f)`, `prod`, `allProd` in the ACL iterative layout. Each operation costs O(log n) plus O(1) per failed node; the total number of failures is the preset's amortized bound. The searches `maxRight` and `minLeft` are planned (listed under `missing:` in the row).

### RangeAndOrRangeSumMax<T>

- `S {sum, mx, band, bor, len, bad}`; `F {a, b}` maps `x` to `(x & a) | b`; builders `andWith(x) = {x, 0}`, `orWith(x) = {~0, x}`, `assign(x) = {0, x}`; `composition(f, g) = {g.a & f.a, (g.b & f.a) | f.b}`; `leaf(x)`.
- `mapping` succeeds when every touched bit (`~a | b`) is uniform in the node (set in `band` or clear in `bor`); then every element changes by the same amount and `sum` moves by `len` times it. `T` is unsigned (sum modulo 2^w) or signed with nonnegative values and or-operands.
- Amortized: O((n + q log n) w) failures in total for w-bit words. A failed node ends with at least one more uniform bit, updates make touched bits uniform on canonical nodes, and only the O(log n) boundary nodes can lose up to w uniform bits per update.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/14-segtreebeats_tester.cpp`, entry `14-segtreebeats_tester.py` (driver `_00_contest_runner.py`, three builds, 24 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| SegTreeBeats `addUpdate`, `chminUpdate`, `chmaxUpdate`, `setUpdate`, `sumQuery`, `maxQuery`, `minQuery`; vector build | `beatsRun` families add/chmin/chmax/set and chmin/chmax | A plain vector updated element by element; three random range queries per step and a full point scan every 64 steps |
| `divideUpdate`, `sqrtUpdate` with add and set; wide values with negative floors | `beatsRun` addSetDivideSqrt, addDivideWide | An independent floor `(x - ((x % q) + q) % q) / q` and integer square root by binary search on 128-bit squares |
| `modUpdate` with set, chmin, divide, sqrt; every operation mixed | `beatsRun` setModChminDivideSqrt, allMixed, allMixedNonnegative | The same vector; sqrt and mod are only issued when the oracle's range minimum is `>= 0` |
| Size build, empty ranges and sentinels, values near `2^59` with n = 8, division by one, two-valued chmin/chmax regressions, copy independence | `edges` | Direct values |
| HistoricSegTree `addUpdate`, `tick`, every query | `historicRun` | Per-element current, maximum, minimum and summed history |
| LazySegmentTreeBeats `get`, `set`, `apply`, `prod`, `allProd`; `RangeAndOrRangeSumMax` with `andWith`, `orWith`, `assign`, `leaf` for `ulng` (64 bits), `uint` (5 bits) and nonnegative `lng` (7 bits) | `andOrRun` | A vector of words; sum modulo 2^w and max |
| BeatsMonoid failure path on a second preset | `chminGenericRun` | A tester-local chmin/sum/max monoid failing at the second maximum, against a vector |
| Runtime monoid state | `statefulGenericRun` | Affine sums modulo a member modulus (7, 998244353 or 10^9+7) in a never-failing stateful BeatsMonoid, against a vector |
| Tree without failures | `affineGenericRun` | `RangeAffineRangeSum` with `fail` false, against a vector |
| Preconditions | `invalid` | 24 probes: negative and oversize sizes, reversed and out-of-range ranges for every update and query (including `divideUpdate` with `q = 1`), `q = 0`, `m = 0`, chmin and chmax with the `lng min` sentinel, sqrt and mod on negative values, historic and generic variants |

Full mode runs sizes 0–100 (stress adds 257 and 1000) with 1500 steps per family and size. A hand mutation check (2026-10-10, quick mode, temporary copies) detected 9 of 11 injected faults (no uniform overwrite on push, missing second-minimum update in `cap`, monotone flag on mod, truncating division, historic peak tag, historic-sum sign, ignored `fail`, and/or uniformity test, second-maximum merge). The two survivors are equivalent: accepting `x == v2` as a tag leaves a node whose max equals its second max, which no later tag can touch before a recompute; and testing `<` instead of `!=` on the shifts is the same test, because `f(x) - x` is nonincreasing.

## Commands and Results

Machine: Linux x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.7; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 894 MB | 0.1 + 1.9 | 23.1 + 3.0 | 24.2 + 17.3 | 1096 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 940 MB | 0.3 + 2.4 | 27.9 + 3.5 | 31.7 + 15.6 | 1046 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 27.0 + 0.1 | 28.9 + 0.7 | | 904 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 910 MB | 0.1 + 4.2 | 10.5 + 7.2 | 10.4 + 35.3 | 577 MB |

Package runs of 2026-10-17 (P027 monoid pass):

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/14-segtreebeats_tester.py' --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 1,947,073 checks per configuration, 24 assertion probes, MEMORY peak 542 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/14-segtreebeats_tester.py' --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 1,947,073 checks per configuration, MEMORY peak 566 MB |
| `python3 '96-Local Testing/02-Data Structures/14-segtreebeats_tester.py' --mode stress --seed 7` | PASS, 9,632,205 checks per configuration, MEMORY peak 541 MB |
| `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS: 156 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace, sanitizer self-tests; MEMORY peak 1834 MB |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

Driver `14-segtreebeats_benchmark.py` with `14-segtreebeats_benchmark.cpp`: n = q = 200000, seed 20261010, median of 5 runs without a separate warm-up, GNU++20 `-O2 -DNDEBUG`, GCC 16.2, Intel Core i9-11900H. Each workload checksums its answers, and the checksum must repeat across runs. These are the final runs after the review fixes; other worktree sessions were running and no CPU was pinned, so the medians carry noise.

| Workload | Median ms |
|---|---|
| random chmin/chmax/add/set/sum (Library Checker `range_chmin_chmax_add_range_sum` shape, values up to 1e9) | 408.7 |
| whole-range divide by 2 then add 1 on alternating 1, 2 (defeats `max == min` pruning) | 10.2 |
| whole-range sqrt then add `k^2 - k` on alternating `k^2 - 1`, `k^2` (k = 10^6) | 10.3 |
| random divide/sqrt/mod/set/sum, values up to 10^12 | 600.7 |
| HistoricSegTree random add plus tick, historic max and sum queries | 473.1 |
| LazySegmentTreeBeats and/or with sum and max (`uint`) | 555.4 |

The two adversarial rows run in O(log n) per call, which confirms the shift rule; a `max == min`-only prune would touch all n leaves per call.

## Sources

| Source | Actual reading and use |
|---|---|
| OI Wiki segment tree beats (https://oi-wiki.org/ds/seg-beats/) | Chmin/chmax/add tags, historic max with add, bounds. |
| smijake3, "Segment Tree Beats" (https://smijake3.hatenablog.com/entry/2019/04/28/021457) | O(n log n + q log^2 n) with add and assign. |
| maspypy `segtree_beats`, `beats_summinmax_chminchmax`; ei1333 `segment-tree-beats`, `beats-monoid`; Nyaan `segment-tree-beats(-abstract)`; suisen `segment_tree_beats`; hitonanode `acl_beats`, `acl_range-bitwiseandor-range-max` | Generic beats contract (`fail` on mapping, push and recompute) and the and/or preset idea. |
| UOJ 164, UOJ 169, yukicoder 880, AtCoder abc256_h; mzhang historic segment tree notes; tjkendev historic gist | Historic-value definitions and the left-out variants. |

Fetched 2026-10-10 by the completeness sweep ([00-sources.md](00-sources.md)). Ji's 2016 paper, LOJ 6029 and Codeforces blog 57319 were not read (known from search summaries only). The implementation was written from the contract.

## Limits and Handoffs

- Row 14 is partial: `DynamicLazySegmentTreeBeats`, `RangeChminChmaxAddAssignRangeSumMinMax`, `RangeChminAddRangeSumMaxHistoricMax`, `RangeGcdAssignRangeMaxSum`, `RangeConvergingMapRangeSumMax`, `RangeChminChmaxAddRangeChangeCount`, `TwoArrayChminAddRangeMaxSum`, `LazySegmentTreeBeats::maxRight` and `LazySegmentTreeBeats::minLeft` are listed under `missing:` in the inventory row.
- Left out, with reasons in [00-notes.md](00-notes.md#p028-omissions): historic values under chmax (UOJ 169), the historic-max sum, historic sums of squares, and `LazySegmentTreeBeats::values`.
- Kinetic segment trees belong to row 45, which may build on `LazySegmentTreeBeats`.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): 区间最值操作 & 区间历史最值, 吉司机线段树, 吉老师线段树, 势能线段树, 区间取最值, 区间历史和, 区间开平方, 区间取模.

## History

- 2026-10-10: P028 first verification; legacy `SegTreeBeats` replaced (half-open ranges, shared max/min sides, no assignment tag, value-domain updates, historic and generic trees added).
- 2026-10-10: independent review (P028): no wrong answers in the domain; fixed a walk that never ended for chmin/chmax with the `lng min` sentinel (entry assert plus leaf guard), the skipped range assert for `q = 1`, and the node size in this document.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `prod`/`allProd` names, stateful-monoid tests); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 542/566/541 MB.
- 2026-11-07: P263 added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
