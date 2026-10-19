# 03-segmenttree.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SegmentTree

`0 <= n <= 2^29`, so doubled indices and search arithmetic stay in `int`. `M` is a monoid (`S`, associative `op`, two-sided identity `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): the tree keeps it as `m` and takes it as constructor argument `mon = {}` after the data, so a preset (`SegmentTree<Max<lng>>`), a stateful struct or a lambda through `Monoid{unit, f}` all work, and an empty monoid adds no size. Operand order is preserved and the empty range returns `e()`. `get` and `allProd` return `vector<S>::const_reference`: a reference owned by the tree, or a `bool` value for `S = bool`. `values()` copies the leaves in O(n). `apply(p, x)` sets point p to `op(a[p], x)`. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(prod(l, r))`; `n` means no failure. `minLeft(r, pred)` returns the smallest `l` in `[0, r]`; 0 means no failure. `fold(l, r, x, act)` returns `act(...act(act(x, a[l]), a[l + 1])..., a[r - 1])` and `foldRight(l, r, x, act)` returns `act(a[l], act(a[l + 1], ...act(a[r - 1], x)...))` for `x` of any type `X`; `act` is a right action (`act(act(x, s), t) == act(x, op(s, t))`, `act(x, e()) == x`) or a left action (`act(s, act(t, x)) == act(op(s, t), x)`, `act(e(), x) == x`), called at most `2 * bit_width(base)` times on canonical nodes. Neither forms the product, so neither is `act(x, prod(l, r))` in another form: for `K`-vectors over `K × K` matrices a query costs `O(K^2 log(n))` instead of `O(K^3 log(n))`; a chain DP uses row-vector transitions with `fold` or column-vector ones with `foldRight`. Predicates are deterministic, true on `e()` (asserted), and stay false as the range grows (truth forms a prefix as the queried interval extends). Callbacks must not mutate the tree. Costs count calls of `op` and copies of `S` as O(1). Size (`SegmentTree(N = 0, mon = {})`), vector and braced-list constructors exist.

### Correctness and cost

Ordered left/right accumulators preserve noncommutative products. `fold` applies the left-side canonical nodes as the climb finds them (ascending) and the right-side ones, stacked in at most 30 slots because `base <= 2^29`, in reverse; `foldRight` mirrors it, so both visit the nodes in range order and the action laws make the node-wise result equal the element-wise fold. Identity padding makes the root and searches valid for nonpowers of two and empty trees. Build/storage O(n+1), updates/range searches O(log(n+1)), get/allProd O(1). The size bound keeps doubled indices and search arithmetic in signed `int`. `apply` and `values` are one-line compositions of verified operations.

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
| `S = bool` | `booleans` | Every bool array through length 8 for OR and AND trees through `Monoid` lambdas: `get`/`allProd` bound to `const bool &`, `values`, every range, `maxRight`/`minLeft` against scans, `apply` (run under ASan/UBSan) |
| Random histories | `randomHistories` | 100 arrays of up to 220 points with 250 operations |
| Runtime monoid state | `stateful` | Sums modulo 2, 7, 97 and 10^9+7 through a member (`ModSum{mod}`) and a capturing lambda in `Monoid`; 60 arrays (10 quick, 300 stress) of up to 59 points, 100 random `prod`/`apply` steps each against a modular left fold |
| `fold`, `foldRight` | `folds`, `booleans` | Every range of 80 arrays (10 quick, 250 stress) of up to 39 points, three point-update rounds each: 3×3 max-plus matrices with missing entries on row vectors (`fold`) and column vectors (`foldRight`) against leaf-by-leaf products, at most `2 * bit_width(base)` act calls; Point Set Range Composite (affine maps modulo 97 acting on values) against direct evaluation and composite maps (`foldRight`) by exact coefficients; strings both ways with an `op`-counting monoid that must see no call during either fold; AND/OR trees on `bool`; 6 probes (negative, reversed and past-the-end ranges) |
| Operand order and construction | `noncommutative` | Ordered strings and direct affine evaluation modulo 97, `apply` operand order on strings at every point and on affine maps against `g(old(x))`, direction-sensitive search, nondefault payload, identity/empty/alias/copy/move; braced singleton/list; 14 probes including `apply-end` |

Quick: length 4, bool length 5. Stress: length 8, bool length 10.

## Commands and results

Latest run 2026-10-18, Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `03-segmenttree_tester.py --mode full --seed 20261018` (GCC 16.2.1) | PASS, 3 configurations, 1,267,715 checks per configuration, 20 assertion probes, MEMORY peak 514 MB |
| `CXX=g++-14 03-segmenttree_tester.py --mode full --seed 20261018` (GCC 14.4.1, floor) | PASS, 1,267,715 checks per configuration, MEMORY peak 531 MB |
| `03-segmenttree_tester.py --mode stress --seed 7` | PASS, 6,455,458 checks per configuration, MEMORY peak 578 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1833 MB |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20261018
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20261018
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SegmentTree reference. |
| [ecnerwala cp-book `seg_tree.hpp`](https://raw.githubusercontent.com/ecnerwala/cp-book/master/src/ds/seg_tree.hpp) | `for_each_l_to_r`/`for_each_r_to_l`, canonical-node iteration in range order: the only library form of `fold`/`foldRight` found. |
| [Library Checker Point Set Range Composite](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/data_structure/point_set_range_composite/task.md) | `f_{r-1}(...f_l(x))`, the `fold` semantics; test target. |
| [betrue12, matrices on a segment tree](https://betrue12.hateblo.jp/entry/2020/03/30/220421) | `O(K^3 log(n))` matrix-product queries applied to an initial vector, which `fold` replaces. |

References inspected on 2026-09-27; the fold sources were fetched on 2026-10-18 ([00-sources.md](00-sources.md), 2026-10-18 section).

## Limits and handoffs

Lazy and dual segment trees are row 12, dynamic ones row 13 and persistent ones row 18. The presets and adapters (`Sum`, `Min`, `Monoid`, and so on) live in row 00, `00-monoids.hpp`, included by `98-basic.hpp`; this header does not include it. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 13 findings fixed, `values` and `apply` added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed (here `apply` operand order, restated contract comments).
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract and `prod`/`allProd` names, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 473/487/522 MB.
- 2026-10-18: `fold` and `foldRight` added; independent review, 5 findings fixed (two test gaps, three document corrections); full (seed 20261018) on g++ and g++-14, stress (seed 7) and integration passed, MEMORY peak 514/531/578 MB.
