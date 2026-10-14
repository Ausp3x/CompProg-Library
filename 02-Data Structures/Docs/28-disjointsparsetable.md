# 28-disjointsparsetable.hpp — evidence

Package P033 (batch DS13). New header; no legacy source. Static range products for an associative operation without idempotence or commutativity. Preconditions are `assert`s checked once per call (per query for `staticRangeProducts`).

## Contracts

### Monoid contract

`M` provides `S`, `static S op(const S &, const S &)` (associative, need not be commutative) and `static S e()` (two-sided identity, needed only by `fold` and `staticRangeProducts`; the table and `query` work for a semigroup without `e`, since unused cells are copies of the array). Products are taken left to right: `op(a[l], ..., a[r - 1])`. `S` is copyable; the table copies values.

### DisjointSparseTable<M>

`n < 2^30`. Level 0 is the array; level `k >= 1` splits the array into blocks of `2^(k + 1)` and stores, around each block midpoint `mid`, the suffix products `a[i..mid)` for `i < mid` and the prefix products `a[mid..i]` for `i >= mid`, with `h = max(1, bit_width(n - 1))` levels (0 for `n = 0`). A nonempty query `[l, r)` with `l < r - 1` has `l` and `r - 1` in one block of level `k = bit_width(l xor (r - 1)) - 1` on opposite sides of its midpoint, so it is `op(level_k[l], level_k[r - 1])`: one `op` call; `l = r - 1` returns `a[l]` with none.

- `query(l, r)`: asserts `0 <= l < r <= n`; O(1), at most one `op` call.
- `fold(l, r)`: asserts `0 <= l <= r <= n`; returns `e()` when `l == r`, otherwise `query(l, r)`. Kept by decision beside `query` (`duplicates.md`): it accepts the empty range that `query` rejects.
- Build: at most `n` `op` calls per level, `n * (h - 1)` in total; memory `n * h` values in one flat vector.
- Row 04 `SparseTable::fold` answers the same associative query in O(log n) with `n log n` storage; this table answers it with one `op` call.

### DisjointSparseTable<M>::staticRangeProducts(a, qs)

Offline products of `a[l, r)` for every query `(l, r)`, `0 <= l <= r <= n < 2^30`, returned in query order; empty queries give `e()`. Memory O(n + q), with no table.

- Blocks of `s = bit_ceil(bit_width(n))` elements (a power of two at least `log2(n + 1)`); `pre[i]` and `suf[i]` are the products from the block start to `i` and from `i` to the block end, and `blocks` holds the block products.
- A query inside one block goes to the dyadic routine on `a`; a query over blocks `bl < br` is `op(suf[l], mid, pre[r - 1])`, where `mid` (the product of the blocks strictly between, when there are any) comes from the dyadic routine on `blocks`.
- Dyadic routine: queries are counting-sorted by `l` and grouped by split level `k = bit_width(l xor r) - 1` and then by block `l >> (k + 1)`; for each group the suffix products from the midpoint down to the smallest `l` and the prefix products up to the largest `r` are computed once into a shared buffer, and each query takes one `op` call. A level costs at most one `op` per element, and only levels holding queries are visited.
- Bound: in-block queries have level `< log2(s)`, so the routine on `a` costs at most `n * log2(s) = O(n log(log(n)))` calls; the routine on the `n / s` block products costs `O((n / s) * log(n)) = O(n)`; `pre`, `suf` cost `2n`; each query costs at most 3. Total `O(n log(log(n)) + q)` `op` calls and O(n + q) time beyond them.
- Optimality: Alon and Schieber show that linear preprocessing allows O(α(n)) steps per online semigroup query and that this is best possible, and Chazelle and Rosenberg give an offline `Ω(n + q α(q, n))` lower bound (cited, not inspected), so O(n + q) is out of reach for a general associative operation. The row's former "offline O(n + q)" claim is replaced by this bound; the inverse-Ackermann constructions are research-level and left out ([00-notes.md](00-notes.md#p033-omissions)).

### Violated preconditions

Under `NDEBUG` an out-of-range or empty `query` is undefined: `query(l, l)` reads level `bit_width(l xor (l - 1)) - 1`, a meaningless cell, and outside the table for `l = 0`. Every loop is bounded by `n`, the number of levels or `q`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.cpp`, entry `28-disjointsparsetable_tester.py` (driver `_00_runner.py`; three builds; 7 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `query`, `fold`, `staticRangeProducts` | `exhaustive<Affine>`, `exhaustive<Concat>`, `exhaustive<Min>` (every `n <= 70`, every `(l, r)` in shuffled order; 140 in stress) | Left-to-right fold; affine maps mod 998244353 and string concatenation are noncommutative |
| Same, medium sizes | `randomMedium` (n <= 3000, mixed short and long ranges) | Left-to-right fold |
| Same, large sizes and block structure | `largeGroup` for n around powers of two (1, 2, 3, 31–33, 255–257, 4095–4097) and 2^17, 100003 (full), 2^20, 2^20 + 1 (stress); query sets uniform, inside one block, across adjacent blocks, block aligned, the full range, empty and single | Affine maps with a nonzero slope form a group: the product of `[l, r)` is `inverse(pre[l]) * pre[r]`, an O(1) independent oracle |
| Complexity | `largeGroup` | Counted `op` calls: build at most `n * (h - 1)`; `staticRangeProducts` at most `2n + n * log2(s) + blocks * bit_width(blocks) + 3q` on every query set |
| `query` for a semigroup without `e()` | `semigroup` (max over `lng`, every `(l, r)` for `n <= 70`) | Direct maximum; compiling it proves the table never calls `e()` |
| Empty table, no queries, repeated queries, copies | `edges` | Direct values |
| Preconditions | `invalid` | Empty, out-of-range and negative `query`; reversed and out-of-range `fold`; reversed and out-of-range offline queries |

Mutation check (2026-10-14, quick mode, temporary copies): 6 of 6 injected faults detected (wrong level, reversed suffix order, reversed final combine, dyadic right end off by one, prefix products crossing blocks, `fold` returning an element for an empty range).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.py' --mode full --seed 20261014
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/28-disjointsparsetable_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/28' --no-integration
```

Final run 2026-10-14 (after the review fixes): full mode seed 20261014 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 1,784,521 checks per configuration and 7 assertion probes, MEMORY peak 658 MB and 779 MB; stress mode seed 7 (one round) passed all three configurations with 10,011,632 checks per configuration, MEMORY peak 1212 MB. Informal timing (ad-hoc driver, best of 5, `-O2 -DNDEBUG`, GCC 16.2.1; not a benchmark driver): for n = q = 5 * 10^5 affine maps, `staticRangeProducts` took 45 ms against 62 ms for building the table and folding every query. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds; MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (143 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 2758 MB) and `03-consistency.py` reported no errors.

## Sources

| Source | Actual reading and use |
|---|---|
| ei1333 `structure/others/disjoint-sparse-table.hpp`, hitonanode `sparse_table/disjoint_sparse_table.hpp`, maspypy `ds/sparse_table/disjoint_sparse_table.hpp`, tko919 `DataStructure/disjointsparsetable.hpp`, sotanishy `data-structure/disjoint_sparse_table.hpp` | Level layout, `l xor (r - 1)` level selection, empty-range conventions (rejected by ei1333 and hitonanode, identity in maspypy and sotanishy). |
| maspypy `ds/offline_query/static_monoid_products.hpp`, `ds/static_range_product.hpp` | Offline dyadic bucketing by midpoint; block prefix and suffix products around a table of block products. |
| OI Wiki cat tree (`ds/cat-tree`) | The same structure in heap layout. |
| Alon and Schieber, *Optimal Preprocessing for Answering On-Line Product Queries* (arXiv 2406.06321, abstract) | `Θ(n λ(k, n))` preprocessing for k-step semigroup queries, O(α(n)) steps after linear preprocessing; the paper body was not read. Chazelle and Rosenberg's offline lower bound is cited from memory of the literature, not inspected. |

Fetched 2026-10-14 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p033-omissions): `maxRight`/`minLeft`, the blocked online variant with naive in-block queries, the xor-indexed table, the 2D product on a segment tree, Alon–Schieber.
- Row 04 owns the idempotent sparse table, row 34 the sqrt tree, row 54 linear RMQ, row 30 sliding-window aggregation.

## History

- 2026-10-14: P033 first implementation and verification; independent review: the table no longer calls `e()` (semigroups work for `query`), the empty-query note corrected.
