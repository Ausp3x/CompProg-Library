# 013-dynamicsegmenttree.hpp — Evidence

Package P027 (batch DS06). The header replaces the legacy reference `DynSegTree` (`OLD/algorithms.cpp:2215–2443`). Keys are `lng` in a half-open universe `[lo, hi)` with `hi - lo <= 2^62`, so midpoints `a + (b - a) / 2` and lengths never overflow; every node splits its range at that midpoint, giving depth `<= 62`. Preconditions are `assert`s checked once per operation. General persistent trees belong to row 18.

## Contracts

### DynamicSegmentTree<M, PERSISTENT = false>

`M` is a monoid (`S`, `op`, `e`) under the stored-object contract of [00-monoids.md](00-monoids.md#monoid-contract), kept as `m` and passed last (`DynamicSegmentTree(lo, hi, mon = {})`); the row 00 presets, stateful structs and `Monoid` lambdas qualify. The struct is a node pool shared by any number of trees: a tree is an `int` root handle, `0` is the empty tree, and every mutator returns the (possibly new) root. Unset keys hold `e()`, so the structure is sparse: a `prod` over untouched keys is `e()` and length-aware presets count only set keys.

- `set(root, i, x)`, `apply(root, i, x)` (`a[i] = op(a[i], x)`), `get(root, i)`, `prod(root, l, r)` and `allProd(root)`. Values are returned by value: the pool may reallocate during the next mutation, so no method hands out references into it.
- `enumerate(root, visit)` calls `visit(key, value)` for every key ever set, in increasing order.
- `maxRight(root, l, pred)` is the largest `r` in `[l, hi]` with `pred(prod(l, r))`, and `minLeft(root, r, pred)` the smallest `l` in `[lo, r]`. Predicates are deterministic, true on `e()` (asserted) and monotone; callbacks must not mutate the pool.
- `reserve(nodes)`; `nodeCount()` is the number of live nodes in the pool.
- `meld(a, b)` destructively merges tree `b` into tree `a` and returns the root. `a` and `b` are distinct trees (asserted: `a != b` unless both are empty) that share no nodes. A key present in both becomes `op(a_value, b_value)` (operand order `a` first); the nodes of `b` are released to a free list, and `b` must not be used again.
- `split(root, i)` returns `{left, right}` with the keys `< i` in `left` and the keys `>= i` in `right`, for `i` in `[lo, hi]`. A half without keys is returned as handle `0` (internal nodes left without children are released), so `root == 0` means exactly "no keys" for every tree produced by `set`, `apply`, `meld` and `split`.
- `meld` and `split` are `static_assert`ed away when `PERSISTENT`. With `PERSISTENT = true`, `set` and `apply` copy the `<= 63` nodes on the update path and leave every earlier root valid (path copying); roots are immutable snapshots and the pool never recycles, so memory is `O(q * log(n))` nodes for `q` updates (the header's `M` term states both modes).

### DynamicLazySegmentTree<A, D>

`A` is an acted monoid, stored as `m` and passed last (`DynamicLazySegmentTree(lo, hi, fill, mon = {})`). `D` is a callable `S fill(lng l, lng r)` giving the product of an untouched range; it must be consistent with `op` (`fill(l, r) == op(fill(l, m), fill(m, r))`) so that nodes materialised later agree with their parents. There is one tree, created eagerly for `[lo, hi)` (an empty universe has no nodes). Operations: `set(i, x)`, `get(i)`, `apply(l, r, f)`, `prod(l, r)` (`e()` for an empty range), `allProd()`, `maxRight(l, pred)`, `minLeft(r, pred)`, `reserve`, `nodeCount`. Descents push pending actions and create both children of a node they pass through, so queries allocate too: memory is `O(q * log(n))` nodes for `q` operations of any kind.

### DynamicDualSegmentTree<A>

It uses `F`, `composition` and `id` of the stored `m` (`DynamicDualSegmentTree(lo, hi, mon = {})`; `MonoidAction<M>` turns a plain monoid into such an action). `apply(l, r, f)` composes `f` after every action on `[l, r)`; `get(i)` returns the composed action at `i` (`id()` initially) by composing root to leaf without allocating. Pushes before descending keep ancestors newer than descendants, so noncommutative compositions are ordered correctly.

### Correctness and Cost

Ownership: the plain tree never aliases nodes between roots (every node belongs to one tree) unless `PERSISTENT`, in which case nodes are immutable after creation and sharing is safe. Meld: a recursion step with two nonnull nodes frees exactly one node, so the total meld work over the lifetime of the pool is bounded by the number of nodes ever allocated (`O((sets + splits) * log n)`); a step with one null side is `O(1)`. Split allocates at most one node per level (`O(log n)`), preserves every leaf, and trims childless internal nodes so empty halves are handle 0. Meld combines leaves with `op(a, b)`, matching `apply`; untouched leaves on one side are inherited verbatim because the other side is `e()` and `op(x, e()) = x`. Searches stop at the first failing leaf, visiting `O(log n)` nodes. Node recycling keeps `nodeCount()` equal to the live nodes. The lazy tree is the recursive lazy algorithm with the identity-fill invariant; the dual tree is the recursive form of row 12's `DualSegmentTree`.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/013-dynamicsegmenttree_tester.cpp`, entry `013-dynamicsegmenttree_tester.py` (driver `_00_contest_runner.py`, three builds, 23 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| DynamicSegmentTree `set`, `apply`, `get`, `prod`, `allProd`, `enumerate`, `maxRight`, `minLeft`, `meld`, `split`, `reserve`, `nodeCount` | `plain<SumSpec>`, `plain<Composite>` | Three roots with `map<key, tuple>` oracles folded by an independent tuple `op` (sum and length; a modular affine composite, noncommutative, which pins the meld operand order). Universes `[0,0)`, `[0,1)`, `[0,2)`, `[-7,9)`, `[0,64)`, `[-3,100)`, `[-2^61, 2^61)`, `[-2^61+5, 2^61)`, `[INT64_MIN, INT64_MIN + 2^62)`, `[INT64_MAX - 2^62, INT64_MAX)`. All `O(n^2)` ranges of all three roots re-read after each step on small universes; meld and split between roots against map merge and partition; `root == 0` iff the oracle map is empty. One root's `allProd` is stored into another root (the aliasing case, run under ASan; taken only while the stored total is small, since repeating it through melds doubles the sums until they overflow and the length predicate stops being monotone). `nodeCount` equals the pool size minus the free list and stays within the allocation bound |
| `PERSISTENT` path copying | `persistent` | Every version keeps its own map snapshot; random old versions re-queried (`prod`, `allProd`, `get`, `enumerate`) |
| DynamicLazySegmentTree `set`, `get`, `apply`, `prod`, `allProd`, `maxRight`, `minLeft`, `fill`, `reserve`, `nodeCount` | `lazySmall` (affine min/max/argmin with `fill` carrying leftmost indices and constants 0, 3, −2), `lazyLarge` (sum on `[-2^40, 2^40 + k)`) | A dense vector with element-wise affine maps and brute min/max/argmin; a compressed-segment oracle on the 2^41 universe with searches computed arithmetically inside the failing segment |
| DynamicLazySegmentTree with `RangeAssignRangeComposite` on full-width universes | `lazyCompositeWide` | `[0, 2^62)` and `[INT64_MIN, INT64_MIN + 2^62)`: the first step assigns the whole universe (a root node of length 2^62, the overflow reproducer), then random range assigns and `prod`/`allProd` against a piecewise map of assigned affine maps powered by an independent tuple square-and-multiply |
| DynamicDualSegmentTree `apply`, `get`, `reserve`, `nodeCount` | `dual` | Per-segment affine composition mod 998244353 on all universes, including 2^62 and the `INT64` extremes |
| Runtime monoid state in all three trees | `stateful` | Sums modulo a member modulus (`ModSum`), affine sums through a stateful acted struct with a `fill` functor, and range add through `MonoidAction{Monoid}` with a capturing lambda, on `[-40, 41)` for moduli 2, 97, 998244353 and 10^9+7 (4 rounds quick, 24 full, 80 stress): `prod`, `get`, `apply` against arrays |
| Preconditions | `invalid` | 23 probes: reversed or too-wide universes (the width check uses unsigned arithmetic so a 2^63 span cannot overflow past it), keys and ranges outside `[lo, hi)`, reversed ranges, searches with a false identity predicate, a split key beyond `hi`, `meld(a, a)`, lazy and dual variants |

Quick mode runs 3 universes and 120 steps; full runs 10 universes and 900 steps in three builds; stress runs 1400 steps, sized so the `-O0` debug-iterator build with per-root verification stays inside the runner's 180 s limit. A hand mutation check (2026-10-08, quick mode, temporary copies) detected 9 of 9 injected faults: meld operand order, split boundary, lazy push to one child, dual composition order, right-first search descent, persistence without copying, split leaving the right child shared, the `minLeft` leaf result, and `get` stopping one level early.

## Commands and Results

Machine: Linux x86-64, Intel Core i9-11900H, GNU++20, CPython 3.14.7; the runner's optimized, checked and ASan/UBSan builds.

Library-wide runs of 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), all passed. Times are compile + run seconds for this suite.

| Command | Pre-flight quick peak | Optimized | Checked | ASan-UBSan | MEMORY peak |
|---|---|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | 940 MB | 0.1 + 12.4 | 20.7 + 37.8 | 31.2 + 160.0 | 1129 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | 958 MB | 0.3 + 19.2 | 35.2 + 43.4 | 47.5 + 100.2 | 1116 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | | 34.1 + 0.2 | 28.9 + 2.1 | | 931 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | 949 MB | 0.1 + 8.2 | 10.2 + 31.9 | 14.6 + 97.3 | 616 MB |

Package runs of 2026-10-17 (P027 monoid pass):

| Command | Result |
|---|---|
| `python3 '96-Local Testing/02-Data Structures/013-dynamicsegmenttree_tester.py' --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 13,299,055 checks per configuration, 23 assertion probes, MEMORY peak 593 MB |
| `CXX=g++-14 python3 '96-Local Testing/02-Data Structures/013-dynamicsegmenttree_tester.py' --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 13,299,055 checks per configuration, MEMORY peak 609 MB |
| `python3 '96-Local Testing/02-Data Structures/013-dynamicsegmenttree_tester.py' --mode stress --seed 7` | PASS, 21,058,112 checks per configuration, MEMORY peak 593 MB |
| `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS: 156 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace, sanitizer self-tests; MEMORY peak 1834 MB |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Sources

| Source | Actual reading and use |
|---|---|
| maspypy `dynamic_segtree.hpp`, `dynamic_segtree_sparse.hpp`, `dynamic_lazy_segtree.hpp`, `dynamic_dual_segtree.hpp` | Root-handle API over a shared pool, `default_prod` (fill) functor, signed universe bounds, persistence flag, `enumerate`. |
| OI Wiki segment tree merge/split (https://oi-wiki.org/ds/seg-merge-split/) | Destructive meld with amortised `O(total nodes)` cost, split by key, node recycling when meld and split are mixed. |
| Nyaan dynamic segment tree, tko919 dynamic / dynamic lazy segtree, suisen sparse (lazy) segment tree | Operation catalog; `reserve`, `nodeCount`. |
| Library Checker `point_set_range_composite_large_array`, `range_affine_range_sum_large_array` | Large-universe targets for the plain and lazy trees. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and Handoffs

- Row 13 is partial: `DynamicLazySegmentTree::enumerate`, `DynamicLazySegmentTree::meld` and `DynamicDualSegmentTree::values` are listed under `missing:` in the inventory row.
- Left out, with reasons in [00-notes.md](00-notes.md#p027-omissions): the one-node-per-key sparse variant, range `clear`, split by count, and the vector build (that is row 12).
- Queries on the lazy tree allocate; a read-mostly workload on a huge universe should prefer the plain tree or offline compression.
- Persistent split and meld and `copyRange` stay with row 18.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): segment tree merge and split (OI Wiki: 线段树合并 & 分裂), dynamic segment tree; 线段树基础, 动态开点线段树, 动态开点, 线段树分裂, 权值线段树, 值域线段树.

## History

- 2026-10-08: P027 first verification; legacy `DynSegTree` reference deleted.
- 2026-10-08: independent review (P027), 5 findings fixed.
- 2026-10-08: P027 re-audit (second session), 1 finding fixed (in `00-monoids.hpp`, row 12) and the `M` comment corrected; full (seed 20261008) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-17: P027 monoid pass (stored-object monoid contract, `prod`/`allProd` names, stateful-monoid tests); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 593/609/593 MB.
- 2026-11-07: P263 added the search aliases, including OI Wiki's page titles and common Chinese names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
