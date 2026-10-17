# 34-sqrttree.hpp — evidence

Package P034 (batch DS30). New header; no legacy source. Range products with O(1) queries and O(sqrt(n)) point updates for an associative operation. Preconditions are `assert`s checked once per call.

## Contracts

### Monoid contract

`M` provides `S`, `op(a, b)` (associative, need not be commutative) and `e()` (two-sided identity), under the stored-object contract of [12-lazysegmenttree.md](12-lazysegmenttree.md#monoid-contract-00-monoidshpp): `SqrtTree(a = {}, mon = {})` keeps it as `m`, and the block-count field is `nb`. `e()` is returned for empty queries and fills unused storage; it never enters a nonempty product. Products run left to right: `op(a[l], ..., a[r - 1])`. `S` is copyable. Like row 28's `DisjointSparseTable::fold`, the folder convention (empty range gives the identity) needs `e()`.

### SqrtTree<M>

`n < 2^30`, `lg = bit_width(n - 1)` (0 for `n <= 1`). Layer sizes are `lay[0] = lg` and `lay[i + 1] = ceil(lay[i] / 2)` while `lay[i] > 3`. Layer `i` cuts aligned segments of `2^lay[i]` positions into blocks of `2^ceil(lay[i] / 2)` and stores, per position, the product from its block start (`pre`) and to its block end (`suf`). Layers `i >= 1` also store, per segment, `btw[a][b]`, the product of blocks `a..b` (at most `2^lay[i]` cells per segment).

Layer 0 would need a Θ(n) block table. Instead its `m = ceil(n / 2^ceil(lg / 2))` block products (the index) sit after the array and form a tree at base `n` that starts from layer 1 (`m <= 2^lay[1]`), so a point set rebuilds O(sqrt(n)) cells.

- `prod(l, r)` (named `query` until the P027 monoid pass of 2026-10-17): asserts `0 <= l <= r <= n`; returns `e()` on `l == r`.
  - Otherwise let `d = bit_width(l xor (r - 1))`. If `d <= cut`, the closed range lies in one block of the last layer, at most `2^cut <= 8` positions, and is folded directly. `cut` is the last layer's block exponent, or `lg` when there is no layer.
  - Otherwise the deepest layer with `lay[i] >= d` holds `l` and `r - 1` in one segment but different blocks, and the answer is `suf[l]`, then the blocks strictly between, then `pre[r - 1]`. The middle comes from `btw` on layers `i >= 1`. On layer 0 it comes from one index query, which lands on a layer `>= 1` or on a direct fold of at most 8 index values.
  - At most 9 `op` calls per product.
- `set(p, x)`: asserts `0 <= p < n`. On every layer it rebuilds the block containing `p`, and the segment's `btw` on layers `>= 1`. On layer 0 it also updates `p`'s index value and that value's path in the index tree. Cost O(2^ceil(lg / 2)) = O(sqrt(n)) `op` calls. `x` may alias a stored value.
- Build: per layer at most `2(n + m)` prefix and suffix calls plus `(n + m) / 2 + 2^lay[i]` block-table calls. There are O(log(log(n))) layers, so build is O(n log(log(n))). Memory: `2 * layers * (n + m) + (layers - 1) * (2^lg + 2^ceil(lg / 2))` values of `S`.
- Threshold: layering stops at `lay <= 3`. The two smallest layers of the textbook structure (segments of 4 and 8) are replaced by a direct fold of at most 8 values, which also removes its `l == r` and `l + 1 == r` special cases. For `n = 2^20` the stored values drop from 15 to 9 per element (5 layers to 3) and build time drops by half; query and set times are unchanged within noise (see Benchmarks).
- Tradeoffs: row 03 `SegmentTree` answers in O(log(n)) with O(log(n)) updates and 2n values; row 28 `DisjointSparseTable` answers in O(1) with n log(n) values and no updates; row 04 needs idempotence. The sqrt tree gives O(1) queries with O(sqrt(n)) point updates in O(n log(log(n))) memory.
- Copies are independent snapshots.

### Violated preconditions

Under `NDEBUG` a reversed or out-of-range `prod`, or an out-of-range `set`, reads or writes outside the arrays and is undefined. Every loop is bounded by `n`, and index recursion shrinks its range by a block factor on each call.

### Legacy mapping

No legacy code. The row's planned `update (point)` is named `set`, as in rows 03 and 06. cp-algorithms `query`, maspypy `prod` and sotanishy `fold` are `prod`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/34-sqrttree_tester.cpp`, entry `34-sqrttree_tester.py` (driver `_00_runner.py`; three builds; 5 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| `prod` (including `e()` on empty), build | `exhaustive<Affine>`, `<Min>`, `<Or>`: every `n <= 70` (140 in stress, 24 in quick); `<Concat>` to half that; every `(l, r)` | Left-to-right fold. Affine maps mod 998244353 and string concatenation are noncommutative; `Or` stores `bool`. |
| `set` | Same groups: random point sets, every `(l, r)` rechecked after each set | Left-to-right fold over the updated array |
| `prod`, `set`, medium sizes | `randomMedium` (n <= 5000; short, block-aligned power-of-two and long ranges interleaved with sets) | Left-to-right fold |
| Layer and block boundaries | `large<Affine>` and `large<Byte>` (affine maps mod 16 in one byte) for n in 1, 2, 3, 8, 9, 16, 17, 31–33, 64, 65, 255–257, 4095–4097; `large<Affine>` for 65535–65537 and 100003 (full), 2^20, 2^20 + 1 and 2^22 − 3 (stress); `large<Byte>` for 2^24 + 1, the four-layer configuration (stress) | Reference bottom-up segment tree (noncommutative), itself checked against the left fold on every `(l, r)` for `n <= 40` |
| Complexity | `large` | Counted `op` calls: `prod` at most 9; set at most `sum(4 * 2^bs + 2 * 2^lay)` over layers, at most `16 * 2^ceil(lg / 2) + 64`; build at most `sum(3(n + nb) + 2^lay)` |
| Copies, aliasing, repeated sets, empty tree | `edges` | Direct values |
| Runtime monoid state | `edges` | Sums modulo a member modulus (7 and 10^9+7) on 300 points: every seventh left end against every right end, before and after point sets |
| Preconditions | `invalid` | Reversed, out-of-range and negative `prod` (probes `prod-reversed`, `prod-range`, `prod-negative`); out-of-range and negative `set` |

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `34-sqrttree_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 1,517,687 checks per configuration, 5 assertion probes, MEMORY peak 724 MB |
| `CXX=g++-14 34-sqrttree_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 1,517,687 checks per configuration, MEMORY peak 684 MB |
| `34-sqrttree_tester.py --mode stress --seed 7` | PASS, 11,185,398 checks per configuration, MEMORY peak 1479 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/34-sqrttree_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/34-sqrttree_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/34-sqrttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

Driver `96-Local Testing/02-Data Structures/34-sqrttree_benchmark.py` (C++ `34-sqrttree_benchmark.cpp`). It rewrites the layering threshold in a temporary copy of the header: `lay > 1` is the textbook structure; `lay > 3` is the library. Conditions: Intel Core i9-11900H, Linux x86-64, GCC 16.2.1 `-std=gnu++20 -O2 -DNDEBUG`, seed 20261017, 5 runs, median seconds. Checksums agree across variants. Run 2026-10-17 with no other suite running.

Each workload is a build, q uniform random ranges, q ranges of length 1–16, and q/200 random point sets. Values per element count the array, `pre`, `suf` and `btw`.

| Workload | Threshold | Layers | Values per element | Build | Random | Short | Sets |
|---|---|---|---|---|---|---|---|
| min, n = 2^20, q = 4 * 10^6 | lay > 1 | 5 | 15.01 | 0.0487 | 0.1434 | 0.1314 | 0.0665 |
| | lay > 2 | 4 | 12.01 | 0.0369 | 0.1520 | 0.1203 | 0.0642 |
| | **lay > 3** | 3 | 9.01 | 0.0251 | 0.1250 | 0.1100 | 0.0609 |
| | lay > 4 | 3 | 9.01 | 0.0246 | 0.1451 | 0.1211 | 0.0628 |
| min, n = 1000003, q = 4 * 10^6 | lay > 1 | 5 | 15.21 | 0.0413 | 0.1431 | 0.1394 | 0.0685 |
| | lay > 2 | 4 | 12.16 | 0.0263 | 0.1178 | 0.1252 | 0.0615 |
| | **lay > 3** | 3 | 9.11 | 0.0200 | 0.1113 | 0.1218 | 0.0618 |
| | lay > 4 | 3 | 9.11 | 0.0227 | 0.1365 | 0.1208 | 0.0620 |
| affine mod 998244353, n = 2^18, q = 2 * 10^6 | lay > 1 | 5 | 15.03 | 0.0321 | 0.1134 | 0.0846 | 0.0723 |
| | lay > 2 | 4 | 12.02 | 0.0225 | 0.0937 | 0.0729 | 0.0671 |
| | **lay > 3** | 3 | 9.02 | 0.0180 | 0.1245 | 0.0899 | 0.0811 |
| | lay > 4 | 3 | 9.02 | 0.0222 | 0.1079 | 0.0787 | 0.0754 |

For n = 2^18 the `lay > 3` and `lay > 4` builds have identical layers and code paths. Their 15% spread therefore measures run-to-run noise, and `prod` and `set` differences between thresholds stay within it. Memory and build time decide: `lay > 3` keeps the fewest values (tied with `lay > 4`, which allows direct folds of up to 16 elements) and builds 1.8–2.1 times faster than the textbook layering.

## Sources

| Source | Actual reading and use |
|---|---|
| cp-algorithms *Sqrt Tree*, OI Wiki *Sqrt Tree* (translated from it) | Layer recursion, `between` tables, the index for O(sqrt(n)) updates, layer choice by the highest differing bit; range assignment (left out). |
| maspypy `ds/sqrt_tree.hpp`, sotanishy `data-structure/sqrt_tree.hpp`, AtCoder Info sqrt-tree lecture | Identity versus semigroup conventions; maspypy's three fixed layers with a short direct scan motivated the threshold. |

Fetched 2026-10-17 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-17-p034-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p034-omissions): range assignment, generator build, the fixed three-layer layout.
- Row 28 owns static O(1) products with n log(n) memory and the offline `staticRangeProducts`; row 12 owns range actions.

## History

- 2026-10-17: P034 first implementation and verification; independent review: no valid-input defect; added the four-layer configuration to stress, the threshold benchmark driver and corrected test-size statements.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `query` renamed `prod`, block-count field `nb`, stateful-monoid test added); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 724/684/1479 MB.
