# 28-disjointsparsetable.hpp — Evidence

Package P033 (batch DS13). New header; no legacy source. Static range products for an associative operation without idempotence or commutativity. Each call asserts its preconditions once (`staticRangeProducts` once per query).

## Contracts

### Monoid Contract

`M` provides `S`, an associative `op(a, b)` that need not be commutative, and a two-sided identity `e()`. Only empty ranges and `staticRangeProducts` need `e`: the table and `prod` on nonempty ranges work for a semigroup, since unused cells are copies of the array. `M` follows the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract): kept as `m`, passed last (`DisjointSparseTable(a = {}, mon = {})`, `staticRangeProducts(a, qs, mon = {})`), with static or `const` member functions. Products are taken left to right, `op(a[l], ..., a[r - 1])`. `S` is copyable, and the table copies values.

### DisjointSparseTable<M>

`n < 2^30`. Level 0 is the array. Level `k >= 1` splits the array into blocks of `2^(k + 1)` and stores, around each block midpoint `mid`, the suffix products `a[i..mid)` for `i < mid` and the prefix products `a[mid..i]` for `i >= mid`; there are `h = max(1, bit_width(n - 1))` levels (0 for `n = 0`). A query `[l, r)` with `l < r - 1` has `l` and `r - 1` in one block of level `k = bit_width(l xor (r - 1)) - 1`, on opposite sides of its midpoint, so it is the single `op(level_k[l], level_k[r - 1])`; `l = r - 1` returns `a[l]` without `op`.

- `prod(l, r)`: asserts `0 <= l <= r <= n`. When `l == r` it returns `e()` if `M` has `e` (a compile-time `requires` check) and otherwise asserts `l < r`. O(1), at most one `op` call.
- Build: at most `n` `op` calls per level, `n * (h - 1)` in total; memory `n * h` values in one flat vector.
- Row 04 `SparseTable::associativeProd` answers the same associative query in O(log n) with `n log n` storage; this table answers it with one `op` call.

### DisjointSparseTable<M>::staticRangeProducts(a, qs)

Offline products of `a[l, r)` for every query `(l, r)`, `0 <= l <= r <= n < 2^30`, returned in query order; empty queries give `e()`. Memory O(n + q), with no table.

- Blocks have `s = bit_ceil(bit_width(n))` elements, a power of two at least `log2(n + 1)`. `pre[i]` and `suf[i]` are the products from the block start to `i` and from `i` to the block end, and `blocks` holds the block products.
- A query inside one block goes to the dyadic routine on `a`. A query over blocks `bl < br` is `op(suf[l], mid, pre[r - 1])`, where `mid`, the product of the blocks strictly between when there are any, comes from the dyadic routine on `blocks`.
- Dyadic routine: queries are counting-sorted by `l` and grouped by split level `k = bit_width(l xor r) - 1`, then by block `l >> (k + 1)`. For each group the suffix products from the midpoint down to the smallest `l` and the prefix products up to the largest `r` are computed once into a shared buffer, and each query takes one `op` call. A level costs at most one `op` per element, and only levels holding queries are visited.
- Bound: in-block queries have level `< log2(s)`, so the routine on `a` costs at most `n * log2(s) = O(n log(log(n)))` calls; the routine on the `n / s` block products costs `O((n / s) * log(n)) = O(n)`; `pre` and `suf` cost `2n`; each query costs at most 3. The total is `O(n log(log(n)) + q)` `op` calls and O(n + q) other time.
- Optimality, in the model where only `op` combines values: Alon and Schieber's two-step construction (§2.1: split at the largest power of two, products found by bit operations on the endpoints) is `DisjointSparseTable`, and their §2.2 proves Θ(n log n) preprocessing necessary for queries answered by one product of two stored values, so the table's `n * h` cells are optimal for one `op` per query. With linear preprocessing an online query needs Θ(α(n)) steps (§2.1, §2.2). Offline, Chazelle and Rosenberg prove that for any n and q some q queries need `Ω(n + q α(q, n))` semigroup operations, even over a commutative semigroup (abstract, §1, §2), so O(n + q) `op` calls are out of reach for a general associative operation. The inverse-Ackermann constructions are research-level and left out ([00-notes.md](00-notes.md#p033-omissions)).

### Violated Preconditions

Under `NDEBUG` an out-of-range `prod`, or an empty one on a semigroup, is undefined: `prod(l, l)` without `e` reads level `bit_width(l xor (l - 1)) - 1`, a meaningless cell, and outside the table for `l = 0`. Every loop is bounded by `n`, the number of levels or `q`.

### Legacy Mapping

| Renamed (P027 monoid pass, 2026-10-17) | Replacement |
|---|---|
| `query(l, r)` (nonempty) | `prod(l, r)` on a semigroup |
| `fold(l, r)` (empty allowed) | `prod(l, r)` with `e()` |

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.cpp`, entry `28-disjointsparsetable_tester.py` (driver `_00_contest_runner.py`; three builds; 6 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `prod` (empty and nonempty ranges), `staticRangeProducts` | `exhaustive<Affine>`, `exhaustive<Concat>`, `exhaustive<Min>` (every `n <= 70`, every `(l, r)` in shuffled order; 140 in stress) | Left-to-right fold; affine maps mod 998244353 and string concatenation are noncommutative |
| Same, medium sizes | `randomMedium` (n <= 3000, mixed short and long ranges) | Left-to-right fold |
| Same, large sizes and block structure | `largeGroup` for n around powers of two (1, 2, 3, 31–33, 255–257, 4095–4097) and 2^17, 100003 (full), 2^20, 2^20 + 1 (stress); query sets uniform, inside one block, across adjacent blocks, block aligned, the full range, empty and single | Affine maps with a nonzero slope form a group: the product of `[l, r)` is `inverse(pre[l]) * pre[r]`, an O(1) independent oracle |
| Complexity | `largeGroup` | Counted `op` calls: build at most `n * (h - 1)`; `staticRangeProducts` at most `2n + n * log2(s) + blocks * bit_width(blocks) + 3q` on every query set |
| `prod` for a semigroup without `e()` | `semigroup` (max over `lng`, every nonempty `(l, r)` for `n <= 70`) | Direct maximum; compiling it proves the table and nonempty `prod` never call `e()` |
| Empty table, no queries, repeated queries, copies; runtime monoid state | `edges` | Direct values; sums modulo a member modulus (5 and 10^9+7) over every range through `prod` and `staticRangeProducts(a, qs, mon)` |
| Preconditions | `invalid` | Empty `prod` on a semigroup, out-of-range, negative and reversed `prod`; reversed and out-of-range offline queries (6 probes) |

A hand mutation check on 2026-10-14 (quick mode, temporary copies) detected 6 of 6 injected faults: a wrong level, a reversed suffix order, a reversed final combine, the dyadic right end off by one, prefix products crossing blocks, and the empty-range path returning an element.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 834 MB; optimized 0.1 + 3.7 s, checked 22.9 + 4.5 s, ASan-UBSan 22.8 + 41.9 s; MEMORY peak 1039 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 893 MB; optimized 0.3 + 4.9 s, checked 34.7 + 5.6 s, ASan-UBSan 40.0 + 33.5 s; MEMORY peak 1044 MB |
| `01-run.py --mode quick` | Optimized 26.8 + 0.5 s, checked 24.7 + 1.9 s; MEMORY peak 831 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 838 MB; optimized 0.1 + 8.8 s, checked 9.7 + 10.7 s, ASan-UBSan 9.4 + 82.5 s; MEMORY peak 1273 MB |

Latest per-header run 2026-10-17 (P027 monoid pass) on the same machine with CPython 3.14.7, in the optimized, checked and ASan/UBSan builds:

| Command (2026-10-17) | Result |
|---|---|
| `28-disjointsparsetable_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 1,657,545 checks per configuration, 6 assertion probes, MEMORY peak 693 MB |
| `CXX=g++-14 28-disjointsparsetable_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 1,657,545 checks per configuration, MEMORY peak 653 MB |
| `28-disjointsparsetable_tester.py --mode stress --seed 7` | PASS, 9,017,706 checks per configuration, MEMORY peak 1250 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| ei1333 `structure/others/disjoint-sparse-table.hpp`, hitonanode `sparse_table/disjoint_sparse_table.hpp`, maspypy `ds/sparse_table/disjoint_sparse_table.hpp`, tko919 `DataStructure/disjointsparsetable.hpp`, sotanishy `data-structure/disjoint_sparse_table.hpp` | Level layout, `l xor (r - 1)` level selection, empty-range conventions (rejected by ei1333 and hitonanode, identity in maspypy and sotanishy). |
| maspypy `ds/offline_query/static_monoid_products.hpp`, `ds/static_range_product.hpp` | Offline dyadic bucketing by midpoint; block prefix and suffix products around a table of block products. |
| OI Wiki cat tree (`ds/cat-tree`) | The same structure in heap layout. |
| Alon and Schieber, *Optimal Preprocessing for Answering On-Line Product Queries* (TR 71/87, arXiv 2406.06321), pp. 1–7; Chazelle and Rosenberg, *The Complexity of Computing Partial Sums Off-Line* (IJCGA 1991), pp. 33–35; both read on 2026-11-22 ([00-sources.md](00-sources.md#fetched-2026-11-22-source-check)) | The Optimality bullet above: `Θ(n λ(k, n))` preprocessing for k-step queries, the two-step case being this table, Θ(α(n)) steps after linear preprocessing, and the offline lower bound. |

Fetched 2026-10-14 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Row 28 is partial: `DisjointSparseTable::maxRight`, `DisjointSparseTable::minLeft` (returned by P275) and `staticRangeMinArpa` are missing. The xor-indexed table is planned Esoteric row 72 (`XorDisjointSparseTable`).
- Left out with reasons in [00-notes.md](00-notes.md#p033-omissions): the blocked variant with naive in-block queries, the 2D associative product on a segment tree, Alon–Schieber.
- Row 04 owns the idempotent sparse table, row 34 the sqrt tree, row 54 linear RMQ, row 30 sliding-window aggregation.

## Aliases

Search aliases (P263, grab --find): cat tree (猫树); ST 表, 猫树分治, 不相交稀疏表, 静态区间查询.

## History

- 2026-10-14: P033 first implementation and verification; after the review the table no longer calls `e()`, so semigroups work, and the empty-query note was corrected.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `query` and `fold` merged into `prod`, stateful-monoid tests); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed.
- 2026-11-07: P263 added the search aliases (`grab --find`); `cat tree`, OI Wiki's name for this structure in heap layout, is kept as a synonym (user decision 2026-11-07); no code or test change.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
