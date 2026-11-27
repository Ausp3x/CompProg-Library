# 003-segmenttree.hpp — Evidence

Package P006 (DS01, then DS02). The contracts common to rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SegmentTree

`0 <= n <= 2^29`, so doubled indices and search arithmetic stay in `int`. `M` is a monoid (`S`, associative `op`, two-sided identity `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): the tree keeps it as `m` and takes it as constructor argument `mon = {}` after the data, so a preset (`SegmentTree<Max<lng>>`), a stateful struct or a lambda through `Monoid{unit, f}` all work, and an empty monoid adds no size. Operand order is preserved and the empty range returns `e()`. Size (`SegmentTree(N = 0, mon = {})`), vector and braced-list constructors exist. Costs count calls of `op` and copies of `S` as O(1).

- `get` and `allProd` return `vector<S>::const_reference`: a reference owned by the tree, or a `bool` value for `S = bool`. `values()` copies the leaves in O(n). `apply(p, x)` sets point `p` to `op(a[p], x)`.
- `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(prod(l, r))`; `n` means no failure. `minLeft(r, pred)` returns the smallest `l` in `[0, r]`; 0 means no failure. Predicates are deterministic, true on `e()` (asserted), and stay false as the range grows.
- `fold(l, r, x, act)` returns `act(...act(act(x, a[l]), a[l + 1])..., a[r - 1])` and `foldRight(l, r, x, act)` returns `act(a[l], act(a[l + 1], ...act(a[r - 1], x)...))` for `x` of any type `X`. `act` is a right action (`act(act(x, s), t) == act(x, op(s, t))`, `act(x, e()) == x`) or a left action (`act(s, act(t, x)) == act(op(s, t), x)`, `act(e(), x) == x`), called at most `2 * bit_width(base)` times on canonical nodes. Neither forms the product, so neither is `act(x, prod(l, r))` in another form: for `K`-vectors over `K × K` matrices a query costs `O(K^2 log(n))` instead of `O(K^3 log(n))`. A chain DP uses row-vector transitions with `fold` or column-vector ones with `foldRight`.
- `setManySorted(a)` takes pairs `(p, x)` with `0 <= p < n` and `p` nondecreasing (asserted at entry in O(k)) and writes each leaf in order, so a repeated `p` keeps its last `x`; the tree ends as the same `set` calls in order would leave it. `a` is only read, so it may come from `values()`. An empty `a` is a no-op.
- `prodIds(l, r)` returns the canonical nodes of `[l, r)` left to right as indices into `tree` (node `i` covers the leaves below it, the root is 1, leaf `p` is `base + p`): the maximal dyadic blocks partitioning the range, at most `2 * bit_width(base)` of them (tested on every range), empty for `l == r`. Their `op` product in order is `prod(l, r)`. They stay valid until a constructor or assignment replaces `tree`.
- Callbacks must not mutate the tree.

### Correctness and Cost

Ordered left and right accumulators preserve noncommutative products. `fold` applies the left-side canonical nodes as the climb finds them (ascending) and the right-side ones, stacked in at most 30 slots because `base <= 2^29`, in reverse; `foldRight` mirrors it. Both visit the nodes in range order, and the action laws make the node-wise result equal the element-wise fold. Identity padding makes the root and the searches valid for nonpowers of two and empty trees. Build and storage are O(n+1), updates and range searches O(log(n+1)), `get` and `allProd` O(1). `apply` and `values` are one-line compositions of verified operations.

`setManySorted`: the touched leaves are all at depth `log(base)` and the list of touched nodes stays sorted level by level, so deduplicating adjacent parents gives each touched ancestor once; children are final before their parent is rebuilt because levels go bottom up. Level `j` above the leaves has at most `min(m, base / 2^j)` touched nodes for `m` distinct positions, which sums to O(m * log(2 * n / m)) `op` calls plus O(k) leaf writes, against `k * log(n)` for `k` calls to `set` (maspypy `set_many_sorted`).

### Legacy Mapping

| Renamed (P027 monoid pass, 2026-10-17) | Replacement |
|---|---|
| `query(l, r)`, `allQuery()` | `prod(l, r)`, `allProd()` |
| `(id, f)` constructor arguments | one monoid `mon` |

## Feature-to-Test Map

The Python entry and the C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; the runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`003-segmenttree_tester.cpp` group) | Oracle |
|---|---|---|
| `get`, `set`, `prod`, `allProd`, `maxRight`, `minLeft`, `values`, `apply` | `exhaustive` | Ternary arrays through length 6, every range and search threshold, `values`, `apply` |
| `S = bool` | `booleans` | Every bool array through length 8 for OR and AND trees through `Monoid` lambdas: `get`/`allProd` bound to `const bool &`, `values`, every range, `maxRight`/`minLeft` against scans, `apply` (run under ASan/UBSan); `base` is the least power of two at least `max(n, 1)`; `checkedSize(MAX_SIZE)` and an OR tree of `n = MAX_SIZE` (`2^30` bits) with `set`, `prod`, `maxRight` |
| Random histories | `randomHistories` | 100 arrays of up to 220 points with 250 operations; each `maxRight` and `minLeft` calls `pred` at most `3 * bit_width(base) + 1` times |
| Runtime monoid state | `stateful` | Sums modulo 2, 7, 97 and 10^9+7 through a member (`ModSum{mod}`) and a capturing lambda in `Monoid`; 60 arrays (10 quick, 300 stress) of up to 59 points, 100 random `prod`/`apply` steps each against a modular left fold |
| `fold`, `foldRight` | `folds`, `booleans` | Every range of 80 arrays (10 quick, 250 stress) of up to 39 points, three point-update rounds each: 3×3 max-plus matrices with missing entries on row vectors (`fold`) and column vectors (`foldRight`) against leaf-by-leaf products, at most `2 * bit_width(base)` act calls; Point Set Range Composite (affine maps modulo 97 acting on values) against direct evaluation, and composite maps (`foldRight`) by exact coefficients; strings both ways with an `op`-counting monoid that must see no call during either fold; AND/OR trees on `bool`; 6 probes (negative, reversed and past-the-end ranges) |
| Operand order and construction | `noncommutative` | Ordered strings and direct affine evaluation modulo 97; `apply` operand order on strings at every point and on affine maps against `g(old(x))`; direction-sensitive search; nondefault payload; identity, empty, alias, copy and move; braced singleton and list; 15 probes including `apply-end` and `apply-end-full` (`n == base`, so a missing assert reads past `tree`) |
| `prodIds` | `exhaustive`, `noncommutative` | Every range of every enumerated array: the nodes lie in `[1, 2 * base)`, cover `[l, r)` contiguously left to right and are maximal (each parent leaves `[l, r)`), which characterizes the canonical decomposition; concatenating `tree[id]` over random string trees equals the direct concatenation; 3 `prod-ids-*` probes |
| `setManySorted` | `setManyExhaustive`, `setManyRandom`, `setManyPayloads` | Every nondecreasing position multiset of length <= 4 on n <= 8 (two values, three starting arrays, sum and string concatenation): `values`, `allProd`, every `prod` and the internal `tree` against a brute-force fold and a twin tree driven by `set`; `op` calls counted and equal to the distinct ancestors (`std::set` of `(base + p) >> j`); 40 random rounds on n <= 4096 (k = n, random k, clustered duplicates) interleaved with `set`/`apply`/`prod`/`maxRight` against a vector; stateful `ModSum`, capturing lambda, `vector<bool>`, affine composition, `Box`; pairs built from `values()`; 5 `set-many-*` probes |

Quick mode uses length 4, bool length 5, and `setManySorted` on n <= 5 with k <= 3. Stress uses length 8, bool length 10, and `setManySorted` on n <= 10 with k <= 5 and on n <= 16384.

## Commands and Results

Machine: Linux 7.2 x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.8; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 813 MB | 0.3 + 2.3 | 21.9 + 2.7 | 23.1 + 21.5 | 1025 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 872 MB | 0.3 + 2.4 | 24.8 + 2.7 | 29.4 + 15.3 | 1017 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 28.4 + 0.3 | 19.7 + 1.9 | | 806 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 817 MB | 0.1 + 3.5 | 8.7 + 4.7 | 9.2 + 38.9 | 574 MB |

Package runs of 2026-11-12 (DS01 `/finish`; experiment `none`):

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/003-segmenttree_tester.py' --mode full --seed 20261112` (GCC 16.2.1) | PASS, 3 configurations, 29 assertion probes, 5,465,498 checks per configuration, MEMORY peak 1300 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/003-segmenttree_tester.py' --mode full --seed 20261112` (GCC 14.4.1, floor) | PASS, 3 configurations, 29 assertion probes, MEMORY peak 1278 MB |
| `python3 '96-Local Testing/02-Data Structures/003-segmenttree_tester.py' --mode stress --seed 7` | PASS, 3 configurations, 77,356,102 checks per configuration, MEMORY peak 1006 MB |
| `python3 '96-Local Testing/05-extraction.py' ops --filter '02-Data Structures/003-segmenttree.hpp'`; `python3 '96-Local Testing/05-extraction.py' testers --mode full --filter '02-Data Structures/0'` | PASS on g++ and g++-14; the tester's quick run passes on the extracted build |
| `python3 '96-Local Testing/01-run.py' --mode quick` (after the header edits) | PASS, 187 suites; `00-Tools/15-harness_tester.py` (outside this package) failed beside other suites and passed alone |
| `python3 '96-Local Testing/02-integration.py'` | PASS: 190 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace; MEMORY peak 2722 MB |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Sources

| Source | Actual reading and use |
|---|---|
| [maspypy segtree](https://raw.githubusercontent.com/maspypy/library/main/ds/segtree/segtree.hpp) | Fetched 2026-11-11: `set_many_sorted` and `prod_ids`, the sources of `setManySorted` and `prodIds`. |
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick and segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SegmentTree reference. |
| [ecnerwala cp-book `seg_tree.hpp`](https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/ds/seg_tree.hpp) | `for_each_l_to_r`/`for_each_r_to_l`, canonical-node iteration in range order: the only library form of `fold`/`foldRight` found. |
| [Library Checker Point Set Range Composite](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/data_structure/point_set_range_composite/task.md) | `f_{r-1}(...f_l(x))`, the `fold` semantics; test target. |
| [betrue12, matrices on a segment tree](https://betrue12.hateblo.jp/entry/2020/03/30/220421) | `O(K^3 log(n))` matrix-product queries applied to an initial vector, which `fold` replaces. |

References inspected on 2026-09-27; the fold sources were fetched on 2026-10-18 ([00-sources.md](00-sources.md), 2026-10-18 section).

## Mutation Testing

Record: mewt 4.0.0 on blob `282638776f6bbf8ec24482b7a45b0262ee9bd5ee` (2026-11-12, seed 1, 1 lines changed since blob `396624cfa6a3`): 22 mutants, 21 caught (3 by timeout), 1 survived, 0 untested.

Stage seconds: quick 170, full on the optimized build 15, full on the other builds 105. Earlier campaign: the whole-file campaign of 2026-11-12 (DS01; uniform sample of 619, 500 mutants, 481 caught, 19 survived after test rounds closed 9). Its 19 survivors are equivalent and sit on lines the later record did not cover; the line numbers are unchanged in the current header.

| Mutant | Line | Why it survives |
|---|---|---|
| `MR 604`, `605`, `607`, `609`, `611`, `612`, `614` `std::move(...)` removed | 16, 20, 69, 79, 94, 96, 112 | equivalent: a copy instead of a move, same values |
| `COS 411` `(p >>= 1) > 0` → `!= 0` (current record) | 34 | untestable (user decision 2026-11-13, confirming Claude's call of 2026-11-12): both stop for every `p >= -base`, so for every valid `p`. They differ only for `p < -base` with the assert compiled out, where the mutant restores the endless loop that `03-cpp.md` forbids, an input `05-testing.md` (Memory Safety) keeps from the `NDEBUG` build |
| `COS 434` `adjacent_find(...) == end` → `>=` | 39 | equivalent: a random-access iterator into `a` is at most `a.end()`, so `>=` holds exactly when `==` does |
| `CR 75` `q.reserve(a.size())` removed | 40 | equivalent: capacity only; `push_back` grows `q` and no pointer into it is held |
| `COS 441` `q[0] > 1` → `!=` | 42 | equivalent: node indices are at least 1 |
| `COS 446` `q[k - 1] != u >> 1` → `<` | 45 | equivalent: `q` is sorted, so the parents arrive nondecreasing and `q[k - 1] <= u >> 1`; `<` and `!=` agree |
| `COS 466`, `486`, `506`, `526` `l < r` → `!=` | 52, 59, 68, 77 | equivalent: `l <= r` holds on entry and each step keeps it (`l` odd moves right only below `r`, and halving keeps the order), so the loops end at `l == r` either way |
| `COS 544` `l == n` → `>=` | 86 | equivalent: `l <= n` is asserted |
| `AOS 256` `l - base` → `l % base` | 95 | equivalent: the descent ends at a leaf, `base <= l < 2 * base` |
| `COS 567` `r == 0` → `<=` | 101 | equivalent: `r >= 0` is asserted |
| `COS 571` `r > 1` → `!=` | 105 | equivalent: `r` is a node index, at least 1 |

## Limits and Handoffs

Lazy and dual segment trees are row 12, dynamic ones row 13 and persistent ones row 18. The presets and adapters (`Sum`, `Min`, `Monoid` and the rest) live in row 00, `00-monoids.hpp`, which `98-basic.hpp` includes and this header does not. `reset`/`build` stay out, with the reason in [00-notes.md](00-notes.md#p006-re-audit-omissions).

## Aliases

Search aliases (P263, grab --find): segtree, seg (OI Wiki: 线段树); Library Checker `point_set_range_composite`; 线段树基础, RMQ, zkw线段树, 非递归线段树, 单点修改区间查询, 线段树上二分, 区间最值查询, 区间求和.

## History

- 2026-09-27: original P006 verification; full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 13 findings fixed, `values` and `apply` added, full and stress passed on g++; the independent review confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1 and raised five findings, all fixed (here `apply` operand order, restated contract comments).
- 2026-10-13: P006 prune rerun, header and tests unchanged; full (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full (seed 20261014) passed on g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `prod`/`allProd` names, stateful-monoid tests); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 473/487/522 MB.
- 2026-10-18: `fold` and `foldRight` added; independent review, 5 findings fixed (two test gaps, three document corrections); full (seed 20261018) on g++ and g++-14, stress (seed 7) and integration passed, MEMORY peak 514/531/578 MB.
- 2026-11-07: P263 added the search aliases, including OI Wiki's page titles and common Chinese names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
- 2026-11-12: P006 DS01: `setManySorted` and `prodIds` added (sweep) with their test groups; the first mutation campaign closed 9 test gaps (probes `set-many-past` and `apply-end-full`, the `pred` call bound, the `MAX_SIZE` and `base` checks) and was recorded with 19 equivalent survivors; full on g++ and g++-14, stress, extraction and integration passed.
- 2026-11-12: DS01 `/finish`: `set` bounded for a negative index under `NDEBUG` (it now ends after its out-of-bounds write instead of looping at -1), the `prodIds` bound corrected to `2 * bit_width(base)` and tested on every range, a stale omission clause removed; full on g++ and g++-14, stress, extraction and integration passed; changed-line mutation campaign recorded (1 untestable survivor).
