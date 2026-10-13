# 19-treap.hpp — evidence

Package P029 (batch DS10). New header; the legacy reference `OLD/Team Notebook/src/ds/impltreap.cpp` informed `ImplicitTreap` (no `97-Legacy` excerpt existed). Indices are zero-based, ranges `[l, r)` half-open, `kth` 0-based; preconditions are `assert`s checked once per operation, with the recursive helpers (`join`, `cut`) left unchecked.

## Contracts

Both structures keep every node in one pool `t`; node 0 is the empty tree (size 0), and a tree is the id of its root. Operations that can change a root take `int &v`; `split`, `merge`, `unite` and `build` take and return roots, and the trees passed in stop existing as separate trees. Nodes are never reused, so memory is O(nodes created); copying the struct copies the pool, giving an independent snapshot. Priorities are the high 32 bits of `CustomHash::splitMix64(seed++)`. `seed` defaults to `CustomHash::rnd` (clock ticks mixed with an address, the library's seeding policy), and the same seed with the same operation sequence reproduces the pool exactly. Randomization is Las Vegas: results never depend on the seed, and expected depth is O(log n) for every input order; equal 32-bit priorities only perturb balance. Recursion depth is the tree height, expected O(log n).

### Treap<T>

- `T` is totally ordered by `<` (a strict weak order) and default-constructible (the sentinel's key). A tree is a multiset: the search-tree order is non-strict on both sides, `insert` always adds a copy and `erase(v, x)` removes one copy, returning whether one existed.
- `build(a)` asserts `a` is sorted (O(n) check) and builds in O(n) with a Cartesian stack over the priorities. `size(v)` O(1); `values(v)` in-order, O(n).
- `split(v, x)` returns the keys `< x` and the keys `>= x`. `merge(a, b)` asserts that no key of `b` is below a key of `a`, walking the right spine of `a` and the left spine of `b` (expected O(log n), the same as the join), then joins. `unite(a, b)` is the multiset union of two arbitrary trees: the root with the higher priority splits the other tree by its key, and the halves are united recursively, expected O(m log(n / m + 1)) for sizes `m <= n`.
- Arguments may reference the pool (`insert(s, kth(r, 0))`): `insert` splits before it allocates the new node. Under `NDEBUG` an out-of-range `kth` stops at the empty node and returns its key.
- `insert`, `erase` and the read-only queries are expected O(log n): `find(v, x)` returns a node holding `x` or 0; `kth(v, k)` asserts `0 <= k < size(v)`; `rank` counts keys `< x` and `upperRank` keys `<= x`; `lowerBound` and `upperBound` return the node of the first key `>= x` or `> x`, or 0. A returned node id stays valid, with `t[id].key` readable, until that copy is erased.

### ImplicitTreap<A>

- `A` is an acted monoid as in `00-monoids.hpp` (`S`, `F`, `op`, `e`, `mapping`, `composition(f, g)` applying `f` after `g`, `id`). `all` applies an action to a node's own value and product at once and keeps it in `lz` for the children; the `act` flag makes equality on `F` unnecessary.
- Reversal: `flip` swaps a node's children at once and toggles `rev` (its children's subtrees still to flip). It replaces the product by `A::reverse(sum)` when `A` declares `static S reverse(const S &)` (the product of the reversed sequence), so a noncommutative `op` stays exact if `S` carries both directions. Without the hook, `op` must be commutative for products after a reversal to be exact. A one-element `S` must equal its own reverse. Pending reversals and actions commute because actions act elementwise.
- `build(a)` O(n); `size(v)` O(1); `values(v)` pushes every tag, O(n); `at(v, i)` pushes along its path and returns the value (under `NDEBUG` an out-of-range `i` stops at the empty node and returns `e()`). A value referencing the pool may be inserted, because a new node copies it before the pool grows.
- `split(v, k)` (asserts `0 <= k <= size(v)`) returns the first `k` elements and the rest; `merge(a, b)` concatenates. `insert(v, i, x)` (`0 <= i <= size`), `erase(v, i)`, `set(v, i, x)`, `reverse(v, l, r)`, `apply(v, l, r, f)` and `prod(v, l, r)` (`e()` on an empty range) cut out the range, act on it and merge it back, expected O(log n) each. `prod`, `maxRight` and `minLeft` therefore also take `int &v`.
- `maxRight(v, l, pred)` returns the largest `r` with `pred(prod(l, r))`, and `minLeft(v, r, pred)` the smallest `l` with `pred(prod(l, r))`. Both assert `pred(e())`; the predicate must stay false once it fails as the range grows. Each descends one cut part, expected O(log n).

### Legacy mapping

| Legacy (`impltreap.cpp`) | New |
|---|---|
| `ImplicitTreap<Mon>` (`ope`, `idS`, `idF`, `map`, `cmp`, `init(i, x)`) | `ImplicitTreap<A>` with the acted-monoid contract; index-aware leaves come from the caller (`leaf` builders) |
| `ImplicitTreap(span a)` (n merges, O(n log n)) | `build(a)`, O(n) |
| `insert(k, x)`, `erase(k)` (out-of-range ignored) | `insert(v, k, x)`, `erase(v, k)` (asserted) |
| `reverse(ql, qr)`, `update(ql, qr, f)`, `query(ql, qr)` (inclusive; invalid ranges ignored or `idS`) | `reverse(v, ql, qr + 1)`, `apply(v, ql, qr + 1, f)`, `prod(v, ql, qr + 1)` (asserted) |
| `S::rev()` hook on products and values | `A::reverse(s)` on products |
| `split(i, k, l, r)` out-parameters, `merge` | `split(v, k)` returning a pair, `merge(a, b)` |
| static clock-seeded `mt19937`, `reserve(500000)` | `seed` parameter (default `CustomHash::rnd`), no fixed reserve |

The legacy tree compared lazy tags with `!=` (equality on `F` required) and silently ignored invalid ranges.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/19-treap_tester.cpp`, entry `19-treap_tester.py` (driver `_00_runner.py`; three builds; 18 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| Treap `insert`, `erase` (present and absent), `find`, `kth`, `rank`, `upperRank`, `lowerBound`, `upperBound`, `size`, `values`, `split`, `merge`, `unite`, `build`; four trees in one pool; `lng` keys from `[-30, 30]` and the full range, `string` keys | `keyRandom` | `std::multiset` per tree: bounds, `std::distance` ranks, `std::next` order statistics, split halves; merges of halves taken from different trees |
| Empty trees, `INT64_MIN`/`INT64_MAX` keys, duplicates, merge of built trees, copy independence, seed reproducibility and seed sensitivity, height under sorted inserts and small-to-large unions (`4 * bit_width(n) + 20`), keys passed by reference into a growing pool | `keyEdges` | Direct values; heights by an explicit traversal; `std::multiset` for the aliasing loop |
| ImplicitTreap `insert`, `erase`, `at`, `set`, `reverse`, `apply`, `prod`, `split`/`merge` (rotation and moving a prefix to another sequence), `build`, `values`, `size`, `maxRight`, `minLeft`; three sequences in one pool | `implicitRandom` with a reversible affine composite under range assignment (noncommutative, `A::reverse`), reversible strings under case actions, affine maps on sums (commutative, no hook), range add with range max | `vector` of values: `std::reverse`, elementwise `mapping`, left-to-right fold, `std::rotate`, linear predicate scans |
| Seed reproducibility and seed sensitivity, values passed by reference into a growing pool, empty sequences, nested reversals with actions, empty-range updates, copy independence, searches at the ends | `implicitEdges` | Direct values; pools compared node by node |
| Preconditions | `invalid` | `kth` ranks and empty tree, unordered `merge`, unsorted `build`, position and range bounds of every sequence operation, a predicate false on `e()` |

Mutation check (2026-10-13, quick mode, temporary copies): 10 of 10 injected faults detected. They covered equal keys sent left by `split`, a missing `pull` in `erase`, swapped `unite` halves, a strict `upperRank`, a skipped reverse hook, a `push` that flips one child, the composition order, `maxRight` skipping a left subtree, `minLeft` keeping its boundary and a strict `lowerBound`.

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/19-treap_tester.py' --mode quick --seed 1
python3 '96-Local Testing/02-Data Structures/19-treap_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/19-treap_tester.py' --mode full --seed 1
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/19-' --no-integration
python3 '96-Local Testing/03-consistency.py' --braces '02-Data Structures/19-treap.hpp' '96-Local Testing/02-Data Structures/19-treap_tester.cpp'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/02-integration.py' --sanitizers
```

Latest run 2026-10-13 (after the review fixes): quick mode passed as the pre-flight of every run (peak about 525 MB); full mode seed 1 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 910,093 checks per configuration and 18 assertion probes, MEMORY peak 670 MB and 685 MB; stress (`--seed 7`, one round) passed with 5,470,793 checks per configuration, peak 671 MB. `--braces` reported no violations and `03-consistency.py` no errors; `python3 '96-Local Testing/02-integration.py' --sanitizers` passed on 2026-10-14 (137 standalone/aggregate headers, scalar and AVX2 multi-TU, workspace, sanitizer self-tests; peak 2757 MB). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14.7, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds; MEMORY peaks are dominated by compilation. GCC 14.2 exactly, the Windows build and PyPy were not run.

## Sources

| Source | Actual reading and use |
|---|---|
| cp-algorithms treap; KACTL `Treap.h`; OI Wiki treap (FHQ) | Split/merge, linear build, split-based union bound, in-order dumps. |
| Nyaan `treap`, `lazy-reversible-rbst`; suisen reversible implicit treaps; maspypy `rbst_acted_monoid` | Reversal through a toggle of a two-direction product, `maxRight`/`minLeft` on cut parts, acted-monoid interface. |
| Library Checker `ordered_set`, `range_reverse_range_sum`, `dynamic_sequence_range_affine_range_sum` | Query shapes. |

Fetched 2026-10-13 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-13-p029-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p029-omissions): `prev`/`next` keys, `count`, `intersect`, split by count, key-range products, `rotate` and end pushes.
- Persistent treaps belong to row 58, splay trees and RBSTs to row 20; ordered maps with order statistics without split/merge are row 07.
- Bounds are expected; a caller who fixes the seed and lets an adversary see it loses the expected-depth guarantee.

## History

- 2026-10-13: P029 first verification. The independent review led to three fixes, each with a regression test: `insert` with a key referencing the growing pool, `kth`/`at` loops that were unbounded under `NDEBUG`, and the missing `ImplicitTreap` seed test.
