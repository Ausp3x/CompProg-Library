# 03-segmenttree.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SegmentTree

`0 <= n <= 2^29`, so doubled indices and search arithmetic stay in `int`. `M` is a monoid (`S`, associative `op`, two-sided identity `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): the tree keeps it as `m` and takes it as constructor argument `mon = {}` after the data, so a preset (`SegmentTree<Max<lng>>`), a stateful struct or a lambda through `Monoid{unit, f}` all work, and an empty monoid adds no size. Operand order is preserved and the empty range returns `e()`. `get` and `allProd` return `vector<S>::const_reference`: a reference owned by the tree, or a `bool` value for `S = bool`. `values()` copies the leaves in O(n). `apply(p, x)` sets point p to `op(a[p], x)`. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(prod(l, r))`; `n` means no failure. `minLeft(r, pred)` returns the smallest `l` in `[0, r]`; 0 means no failure. `fold(l, r, x, act)` returns `act(...act(act(x, a[l]), a[l + 1])..., a[r - 1])` and `foldRight(l, r, x, act)` returns `act(a[l], act(a[l + 1], ...act(a[r - 1], x)...))` for `x` of any type `X`; `act` is a right action (`act(act(x, s), t) == act(x, op(s, t))`, `act(x, e()) == x`) or a left action (`act(s, act(t, x)) == act(op(s, t), x)`, `act(e(), x) == x`), called at most `2 * bit_width(base)` times on canonical nodes. Neither forms the product, so neither is `act(x, prod(l, r))` in another form: for `K`-vectors over `K × K` matrices a query costs `O(K^2 log(n))` instead of `O(K^3 log(n))`; a chain DP uses row-vector transitions with `fold` or column-vector ones with `foldRight`. Predicates are deterministic, true on `e()` (asserted), and stay false as the range grows (truth forms a prefix as the queried interval extends). Callbacks must not mutate the tree. Costs count calls of `op` and copies of `S` as O(1). Size (`SegmentTree(N = 0, mon = {})`), vector and braced-list constructors exist.

`setManySorted(a)` takes pairs `(p, x)` with `0 <= p < n` and `p` nondecreasing (asserted at entry in O(k)), writes each leaf in order, so a repeated `p` keeps its last `x`, and leaves the tree as the same `set` calls in order would; `a` is read only, so it may come from `values()`. An empty `a` is a no-op. `prodIds(l, r)` returns the canonical nodes of `[l, r)` left to right as indices into `tree` (node `i` covers the leaves below it, the root is 1, leaf `p` is `base + p`): the maximal dyadic blocks partitioning the range, at most `2 * bit_width(base)` of them (tested on every range), empty for `l == r`; their `op` product in order is `prod(l, r)`. They stay valid until a constructor or assignment replaces `tree`.

### Correctness and cost

Ordered left/right accumulators preserve noncommutative products. `fold` applies the left-side canonical nodes as the climb finds them (ascending) and the right-side ones, stacked in at most 30 slots because `base <= 2^29`, in reverse; `foldRight` mirrors it, so both visit the nodes in range order and the action laws make the node-wise result equal the element-wise fold. Identity padding makes the root and searches valid for nonpowers of two and empty trees. Build/storage O(n+1), updates/range searches O(log(n+1)), get/allProd O(1). The size bound keeps doubled indices and search arithmetic in signed `int`. `apply` and `values` are one-line compositions of verified operations.

`setManySorted`: the touched leaves are all at depth `log(base)`, and the list of touched nodes stays sorted level by level, so deduplicating adjacent parents gives each touched ancestor once; children are final before their parent is rebuilt because levels go bottom up. Level `j` above the leaves has at most `min(m, base / 2^j)` touched nodes for `m` distinct positions, which sums to O(m * log(2 * n / m)) `op` calls, plus O(k) leaf writes, against `k * log(n)` for `k` calls to `set` (maspypy `set_many_sorted`).

### Legacy mapping

| Renamed (P027 monoid pass, 2026-10-17) | Replacement |
|---|---|
| `query(l, r)`, `allQuery()` | `prod(l, r)`, `allProd()` |
| `(id, f)` constructor arguments | one monoid `mon` |

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`03-segmenttree_tester.cpp` group) | Oracle |
|---|---|---|
| `get`, `set`, `prod`, `allProd`, `maxRight`, `minLeft`, `values`, `apply` | `exhaustive` | Ternary arrays through length 6, every range/search threshold, `values`, `apply` |
| `S = bool` | `booleans` | Every bool array through length 8 for OR and AND trees through `Monoid` lambdas: `get`/`allProd` bound to `const bool &`, `values`, every range, `maxRight`/`minLeft` against scans, `apply` (run under ASan/UBSan); `base` is the least power of two at least `max(n, 1)`; `checkedSize(MAX_SIZE)` and an OR tree of `n = MAX_SIZE` (`2^30` bits) with `set`, `prod`, `maxRight` |
| Random histories | `randomHistories` | 100 arrays of up to 220 points with 250 operations; each `maxRight` and `minLeft` calls `pred` at most `3 * bit_width(base) + 1` times |
| Runtime monoid state | `stateful` | Sums modulo 2, 7, 97 and 10^9+7 through a member (`ModSum{mod}`) and a capturing lambda in `Monoid`; 60 arrays (10 quick, 300 stress) of up to 59 points, 100 random `prod`/`apply` steps each against a modular left fold |
| `fold`, `foldRight` | `folds`, `booleans` | Every range of 80 arrays (10 quick, 250 stress) of up to 39 points, three point-update rounds each: 3×3 max-plus matrices with missing entries on row vectors (`fold`) and column vectors (`foldRight`) against leaf-by-leaf products, at most `2 * bit_width(base)` act calls; Point Set Range Composite (affine maps modulo 97 acting on values) against direct evaluation and composite maps (`foldRight`) by exact coefficients; strings both ways with an `op`-counting monoid that must see no call during either fold; AND/OR trees on `bool`; 6 probes (negative, reversed and past-the-end ranges) |
| Operand order and construction | `noncommutative` | Ordered strings and direct affine evaluation modulo 97, `apply` operand order on strings at every point and on affine maps against `g(old(x))`, direction-sensitive search, nondefault payload, identity/empty/alias/copy/move; braced singleton/list; 15 probes including `apply-end` and `apply-end-full` (`n == base`, so a missing assert reads past `tree`) |
| `prodIds` | `exhaustive`, `noncommutative` | Every range of every enumerated array: the nodes are in `[1, 2 * base)`, cover `[l, r)` contiguously left to right and are maximal (each parent leaves `[l, r)`), which characterizes the canonical decomposition; concatenating `tree[id]` over random string trees equals the direct concatenation; 3 `prod-ids-*` probes |
| `setManySorted` | `setManyExhaustive`, `setManyRandom`, `setManyPayloads` | Every nondecreasing position multiset of length <= 4 on n <= 8 (two values, three starting arrays, sum and string concatenation): `values`, `allProd`, every `prod` and the internal `tree` against a brute-force fold and a twin tree driven by `set`; `op` calls counted and equal to the distinct ancestors (`std::set` of `(base + p) >> j`); 40 random rounds on n <= 4096 (k = n, random k, clustered duplicates) interleaved with `set`/`apply`/`prod`/`maxRight` against a vector; stateful `ModSum`, capturing lambda, `vector<bool>`, affine composition, `Box`; pairs built from `values()`; 5 `set-many-*` probes |

Quick: length 4, bool length 5, `setManySorted` n <= 5 with k <= 3. Stress: length 8, bool length 10, `setManySorted` n <= 10 with k <= 5 and n <= 16384.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 813 MB; optimized 0.3 s compile + 2.3 s run, checked 21.9 s compile + 2.7 s run, ASan-UBSan 23.1 s compile + 21.5 s run; MEMORY peak 1025 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 872 MB; optimized 0.3 s compile + 2.4 s run, checked 24.8 s compile + 2.7 s run, ASan-UBSan 29.4 s compile + 15.3 s run; MEMORY peak 1017 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 28.4 s compile + 0.3 s run, checked 19.7 s compile + 1.9 s run; MEMORY peak 806 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 817 MB; optimized 0.1 s compile + 3.5 s run, checked 8.7 s compile + 4.7 s run, ASan-UBSan 9.2 s compile + 38.9 s run; MEMORY peak 574 MB. Dated rows below stay those commands' latest results.

Latest run 2026-11-12 (DS01 `/finish`; experiment `none`), Linux 7.2 x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.8, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `03-segmenttree_tester.py --mode full --seed 20261112` (GCC 16.2.1) | PASS, 3 configurations, 29 assertion probes, MEMORY peak 1300 MB, 5,465,498 checks per configuration |
| `CXX=g++-14 03-segmenttree_tester.py --mode full --seed 20261112` (GCC 14.4.1, floor) | PASS, 3 configurations, 29 assertion probes, MEMORY peak 1278 MB |
| `03-segmenttree_tester.py --mode stress --seed 7` | PASS, 3 configurations, 77,356,102 checks per configuration, MEMORY peak 1006 MB |
| `05-extraction.py ops --filter '02-Data Structures/03-segmenttree.hpp'`; `05-extraction.py testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run on the extracted build passes |
| `01-run.py --mode quick` (after this run's header edits) | PASS, 187 suites; NOTE: `00-Tools/15-harness_tester.py` (outside this package) failed beside other suites and passed alone |
| `02-integration.py` | PASS: 190 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, MEMORY peak 2722 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20261112
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20261112
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/03-segmenttree.hpp'
python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'
python3 '96-Local Testing/01-run.py' --mode quick
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| [maspypy segtree](https://raw.githubusercontent.com/maspypy/library/main/ds/segtree/segtree.hpp) | Fetched 2026-11-11: `set_many_sorted` and `prod_ids`, the sources of `setManySorted` and `prodIds`. |
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SegmentTree reference. |
| [ecnerwala cp-book `seg_tree.hpp`](https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/ds/seg_tree.hpp) | `for_each_l_to_r`/`for_each_r_to_l`, canonical-node iteration in range order: the only library form of `fold`/`foldRight` found. |
| [Library Checker Point Set Range Composite](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/data_structure/point_set_range_composite/task.md) | `f_{r-1}(...f_l(x))`, the `fold` semantics; test target. |
| [betrue12, matrices on a segment tree](https://betrue12.hateblo.jp/entry/2020/03/30/220421) | `O(K^3 log(n))` matrix-product queries applied to an initial vector, which `fold` replaces. |

References inspected on 2026-09-27; the fold sources were fetched on 2026-10-18 ([00-sources.md](00-sources.md), 2026-10-18 section).

## Mutation testing

Record: mewt 4.0.0 on blob `282638776f6bbf8ec24482b7a45b0262ee9bd5ee` (2026-11-12, seed 1, 1 lines changed since blob `396624cfa6a3`): 22 mutants, 21 caught (3 by timeout), 1 survived, 0 untested.

That campaign (DS01 `/finish`: `set` stops at `(p >>= 1) > 0`, so `p < -base` under `NDEBUG` ends after its out-of-bounds write instead of looping at -1) took quick 170 s, full on the optimized build 15 s, full on the other builds 105 s. Its survivor is untestable:

| Mutant | Line | Why it survives |
|---|---|---|
| `COS 411` `(p >>= 1) > 0` → `!= 0` | 34 | untestable (user decision 2026-11-13, confirming Claude's call of 2026-11-12): both stop for every `p >= -base`, so for every valid `p`; they differ only for `p < -base` with the assert compiled out, where the mutant restores the endless loop that `03-cpp.md` forbids, an input `05-testing.md` (Memory safety) keeps from the `NDEBUG` build |

Earlier campaign: mewt 4.0.0 on blob `396624cfa6a3a9b49b5f9c5ae497f5409adf669e` (2026-11-12, seed 1, whole file, uniform sample of 619): 500 mutants, 481 caught (23 by timeout), 19 survived, 0 untested. Line numbers below are that blob's.

Stage seconds (DS01, first campaign of this header, three runs together): quick 1064, full on the optimized build 146, full on the checked and ASan-UBSan builds 1651. The first run left 28 survivors and two test rounds closed 9: probe `set-many-past` (`COS 426`, line 38, `back < n` → `!=`); probe `apply-end-full` (`CR 70`, `LOS 352`, line 35: on a tree with `n == base`, `apply` without its own assert reads `tree[2 * base]` before `set` asserts, which the checked build's bounds check turns into a libstdc++ abort the runner does not count); the `pred` call bound in `randomHistories` (`WF 144`, `BOS 341` on line 89 and `COS 572` on line 105, which stop the climb and make a search O(n)); `checkedSize(MAX_SIZE)` and the `MAX_SIZE` OR tree (`COS 372`, `COS 382`, lines 15 and 17); the `base` check (`COS 387`, line 18, which doubles `base` for powers of two). The 19 left are equivalent:

| Mutant | Line | Why it is equivalent |
|---|---|---|
| `MR 604`, `605`, `607`, `609`, `611`, `612`, `614` `std::move(...)` removed | 16, 20, 69, 79, 94, 96, 112 | a copy instead of a move, same values |
| `COS 434` `adjacent_find(...) == end` → `>=` | 39 | a random-access iterator into `a` is at most `a.end()`, so `>=` holds exactly when `==` does |
| `CR 75` `q.reserve(a.size())` removed | 40 | capacity only; `push_back` grows `q` and no pointer into it is held |
| `COS 441` `q[0] > 1` → `!=` | 42 | node indices are at least 1 |
| `COS 446` `q[k - 1] != u >> 1` → `<` | 45 | `q` is sorted, so the parents arrive nondecreasing and `q[k - 1] <= u >> 1`; `<` and `!=` agree |
| `COS 466`, `486`, `506`, `526` `l < r` → `!=` | 52, 59, 68, 77 | `l <= r` holds on entry and is kept by each step (`l` odd moves right only below `r`, and halving keeps the order), so the loops end at `l == r` either way |
| `COS 544` `l == n` → `>=` | 86 | `l <= n` is asserted |
| `AOS 256` `l - base` → `l % base` | 95 | the descent ends at a leaf, `base <= l < 2 * base` |
| `COS 567` `r == 0` → `<=` | 101 | `r >= 0` is asserted |
| `COS 571` `r > 1` → `!=` | 105 | `r` is a node index, at least 1 |

## Limits and handoffs

Lazy and dual segment trees are row 12, dynamic ones row 13 and persistent ones row 18. The presets and adapters (`Sum`, `Min`, `Monoid`, and so on) live in row 00, `00-monoids.hpp`, included by `98-basic.hpp`; this header does not include it. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`.

## Aliases

Search aliases (P263, grab --find): segtree, seg (OI Wiki: 线段树); Library Checker `point_set_range_composite`; 线段树基础, RMQ, zkw线段树, 非递归线段树, 单点修改区间查询, 线段树上二分, 区间最值查询, 区间求和.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 13 findings fixed, `values` and `apply` added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed (here `apply` operand order, restated contract comments).
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract and `prod`/`allProd` names, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 473/487/522 MB.
- 2026-10-18: `fold` and `foldRight` added; independent review, 5 findings fixed (two test gaps, three document corrections); full (seed 20261018) on g++ and g++-14, stress (seed 7) and integration passed, MEMORY peak 514/531/578 MB.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: P006 DS01: `setManySorted` and `prodIds` added (sweep) with their test groups; mutation closed 9 test gaps (probes `set-many-past`, `apply-end-full`, the `pred` call bound, the `MAX_SIZE` and `base` checks); full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (19 equivalent survivors).
- 2026-11-12: DS01 `/finish`: `set` bounded for a negative index under `NDEBUG`, the `prodIds` bound corrected to `2 * bit_width(base)` and tested on every range, a stale omission clause removed; full on g++ and g++-14, stress, extraction and integration passed; changed-line mutation campaign recorded (1 equivalent survivor).
