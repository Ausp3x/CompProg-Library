# 03-segmenttree.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SegmentTree

`0 <= n <= 2^29`, so doubled indices and search arithmetic stay in `int`. `M` is a monoid (`S`, associative `op`, two-sided identity `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): the tree keeps it as `m` and takes it as constructor argument `mon = {}` after the data, so a preset (`SegmentTree<Max<lng>>`), a stateful struct or a lambda through `Monoid{unit, f}` all work, and an empty monoid adds no size. Operand order is preserved and the empty range returns `e()`. `get` and `allProd` return `vector<S>::const_reference`: a reference owned by the tree, or a `bool` value for `S = bool`. `values()` copies the leaves in O(n). `apply(p, x)` sets point p to `op(a[p], x)`. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(prod(l, r))`; `n` means no failure. `minLeft(r, pred)` returns the smallest `l` in `[0, r]`; 0 means no failure. Predicates are deterministic, true on `e()` (asserted), and stay false as the range grows (truth forms a prefix as the queried interval extends). Callbacks must not mutate the tree. Costs count calls of `op` and copies of `S` as O(1). Size (`SegmentTree(N = 0, mon = {})`), vector and braced-list constructors exist. Renamed on 2026-10-17: `query` is `prod`, `allQuery` is `allProd`, and the `(id, f)` arguments are one monoid.

### Correctness and cost

Ordered left/right accumulators preserve noncommutative products. Identity padding makes the root and searches valid for nonpowers of two and empty trees. Build/storage O(n+1), updates/range searches O(log(n+1)), get/allProd O(1). The size bound keeps doubled indices and search arithmetic in signed `int`. `apply` and `values` are one-line compositions of verified operations. Associativity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`03-segmenttree_tester.cpp` group) | Oracle |
|---|---|---|
| `get`, `set`, `prod`, `allProd`, `maxRight`, `minLeft`, `values`, `apply` | `exhaustive` | Ternary arrays through length 6, every range/search threshold, `values`, `apply` |
| `S = bool` | `booleans` | Every bool array through length 8 for OR and AND trees through `Monoid` lambdas: `get`/`allProd` bound to `const bool &`, `values`, every range, `maxRight`/`minLeft` against scans, `apply` (run under ASan/UBSan) |
| Random histories | `randomHistories` | 100 arrays of up to 220 points with 250 operations |
| Runtime monoid state | `stateful` | Sums modulo 2, 7, 97 and 10^9+7 through a member (`ModSum{mod}`) and a capturing lambda in `Monoid`; 60 arrays (10 quick, 300 stress) of up to 59 points, 100 random `prod`/`apply` steps each against a modular left fold |
| Operand order and construction | `noncommutative` | Ordered strings and direct affine evaluation modulo 97, `apply` operand order on strings at every point and on affine maps against `g(old(x))`, direction-sensitive search, nondefault payload, identity/empty/alias/copy/move; braced singleton/list; 14 probes including `apply-end` |

Quick: length 4, bool length 5. Stress: length 8, bool length 10. Per configuration: 730,507 checks (full, 2026-10-07), 4,087,658 (stress seed 9, 2026-10-07).

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `03-segmenttree_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 668,319 checks per configuration, 14 assertion probes, MEMORY peak 473 MB |
| `CXX=g++-14 03-segmenttree_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 668,319 checks per configuration, MEMORY peak 487 MB |
| `03-segmenttree_tester.py --mode stress --seed 7` | PASS, 4,237,831 checks per configuration, MEMORY peak 522 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SegmentTree reference. |

References inspected on 2026-09-27.

## Limits and handoffs

Lazy and dual segment trees are row 12, dynamic ones row 13 and persistent ones row 18. The presets and adapters (`Sum`, `Min`, `Monoid`, and so on) live in row 00, `00-monoids.hpp`, included by `98-basic.hpp`; this header does not include it. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, 13 findings fixed, `values` and `apply` added, full and stress passed on g++; independent review: confirmed findings 2–24 fixed, accepted the contract-based resolution of finding 1, raised five findings, all fixed (here `apply` operand order, restated contract comments).
- 2026-10-13: P006 prune rerun, header and tests unchanged; full suite (seed 20261013) passed on g++ and g++-14.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-10-17: P027 monoid pass (stored-object monoid contract and `prod`/`allProd` names, stateful-monoid tests added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 473/487/522 MB.
