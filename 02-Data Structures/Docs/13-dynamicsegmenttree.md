# 13-dynamicsegmenttree.hpp — evidence

Package P027 (batch DS06). Replaces the legacy reference `97-Legacy/03-dynsegtree.cpp` (`OLD/algorithms.cpp:2215–2443`, deleted with this verification). Keys are `lng` in a half-open universe `[lo, hi)` with `hi - lo <= 2^62` (so midpoints `a + (b - a) / 2` and lengths never overflow); every node splits its range at that midpoint, giving depth `<= 62`. Preconditions are `assert`s checked once per operation. General persistent trees belong to row 18.

## Contracts

### DynamicSegmentTree<M, PERSISTENT = false>

`M` is a monoid (`S`, `op`, `e`); the row-12 presets qualify. The struct is a node pool shared by any number of trees: a tree is an `int` root handle, `0` is the empty tree, and every mutator returns the (possibly new) root. Unset keys hold `e()`, so the structure is sparse: a `prod` over untouched keys is `e()` and length-aware presets count only set keys. `set(root, i, x)`, `apply(root, i, x)` (`a[i] = op(a[i], x)`), `get(root, i)`, `prod(root, l, r)`, `allProd(root)` (returned by value: the pool may reallocate during the next mutation, so no method hands out references into it), `enumerate(root, visit)` (every key ever set, in increasing order, `visit(key, value)`), `maxRight(root, l, pred)` (largest `r` in `[l, hi]` with `pred(prod(l, r))`), `minLeft(root, r, pred)` (smallest `l` in `[lo, r]`), `reserve(nodes)`, `nodeCount()` (live nodes in the pool). Predicates are deterministic, true on `e()` (asserted) and monotone; callbacks must not mutate the pool.

`meld(a, b)` destructively merges tree `b` into tree `a` and returns the root: `a` and `b` are distinct trees (asserted: `a != b` unless both are empty) that share no nodes, a key present in both becomes `op(a_value, b_value)` (operand order `a` first), nodes of `b` are released to a free list and `b` must not be used again. `split(root, i)` returns `{left, right}` with the keys `< i` in `left` and the keys `>= i` in `right`; `i` in `[lo, hi]`. A half without keys is returned as handle `0` (internal nodes left without children are released), so `root == 0` is exactly "no keys" for every tree produced by `set`, `apply`, `meld` and `split`. Both are `static_assert`ed away when `PERSISTENT`.

With `PERSISTENT = true`, `set`/`apply` copy the `<= 63` nodes on the update path and leave every earlier root valid (path copying); roots are immutable snapshots and the pool never recycles, so memory is `O(q * log(n))` nodes for `q` updates (the header's `M` term states both modes).

### DynamicLazySegmentTree<A, D>

`A` is an acted monoid, `D` a callable `S fill(lng l, lng r)` giving the product of an untouched range; it must be consistent with `op` (`fill(l, r) == op(fill(l, m), fill(m, r))`) so that nodes materialised later agree with their parents. One tree, created eagerly for `[lo, hi)` (an empty universe has no nodes). `set(i, x)`, `get(i)`, `apply(l, r, f)`, `prod(l, r)` (`e()` for an empty range), `allProd()`, `maxRight(l, pred)`, `minLeft(r, pred)`, `reserve`, `nodeCount`. Descents push pending actions and create both children of a node they pass through, so queries allocate too: memory is `O(q * log(n))` nodes for `q` operations of any kind.

### DynamicDualSegmentTree<A>

Uses `F`, `composition`, `id`. `apply(l, r, f)` composes `f` after every action on `[l, r)`; `get(i)` returns the composed action at `i` (`id()` initially) by composing root-to-leaf without allocating. Pushes before descending keep ancestors newer than descendants, so noncommutative compositions are ordered correctly.

## Correctness and cost

Ownership: the plain tree never aliases nodes between roots (every node belongs to one tree) unless `PERSISTENT`, in which case nodes are immutable after creation and sharing is safe. Meld: a recursion step with two nonnull nodes frees exactly one node, so the total meld work over the lifetime of the pool is bounded by the number of nodes ever allocated (`O((sets + splits) * log n)`); a step with one null side is `O(1)`. Split allocates at most one node per level (`O(log n)`), preserves every leaf, and trims childless internal nodes so empty halves are handle 0. Leaf combination in meld is `op(a, b)`, matching `apply`; untouched leaves on one side are inherited verbatim because the other side is `e()` and `op(x, e()) = x`. Searches stop at the first failing leaf, visiting `O(log n)` nodes. Node recycling keeps `nodeCount()` equal to the live nodes. The lazy tree is the recursive lazy algorithm with the identity-fill invariant; the dual tree is the recursive form of row 12's `DualSegmentTree`.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/13-dynamicsegmenttree_tester.cpp`, entry `13-dynamicsegmenttree_tester.py` (driver `_00_runner.py`; three builds; 23 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| DynamicSegmentTree `set`, `apply`, `get`, `prod`, `allProd`, `enumerate`, `maxRight`, `minLeft`, `meld`, `split`, `reserve`, `nodeCount` | `plain<Sum>`, `plain<Composite>` | Three roots with `map<key, tuple>` oracles folded by an independent tuple `op` (sum/len; modular affine composite, noncommutative, which pins the meld operand order); universes `[0,0)`, `[0,1)`, `[0,2)`, `[-7,9)`, `[0,64)`, `[-3,100)`, `[-2^61, 2^61)`, `[-2^61+5, 2^61)`, `[INT64_MIN, INT64_MIN + 2^62)`, `[INT64_MAX - 2^62, INT64_MAX)`; all `O(n^2)` ranges of all three roots re-read after each step on small universes; meld/split between roots with map merge/partition; `root == 0` iff the oracle map is empty; one root's `allProd` stored into another root (the aliasing case, run under ASan; taken only while the stored total is small, since repeating it through melds doubles the sums until they overflow and the length predicate stops being monotone); `nodeCount` equals pool size minus free list and stays within the allocation bound |
| `PERSISTENT` path copying | `persistent` | Every version keeps its own map snapshot; random old versions re-queried (`prod`, `allProd`, `get`, `enumerate`) |
| DynamicLazySegmentTree `set`, `get`, `apply`, `prod`, `allProd`, `maxRight`, `minLeft`, `fill`, `reserve`, `nodeCount` | `lazySmall` (affine min/max/argmin with `fill` carrying leftmost indices and constants 0, 3, −2), `lazyLarge` (sum on `[-2^40, 2^40 + k)`) | Dense vector with element-wise affine maps and brute min/max/argmin; compressed-segment oracle on the 2^41 universe with searches computed arithmetically inside the failing segment |
| DynamicLazySegmentTree with `RangeSetRangeComposite` on full-width universes | `lazyCompositeWide` | `[0, 2^62)` and `[INT64_MIN, INT64_MIN + 2^62)`: the first step assigns the whole universe (a root node of length 2^62, the overflow reproducer), then random range assigns and `prod`/`allProd` against a piecewise map of assigned affine maps powered by an independent tuple square-and-multiply |
| DynamicDualSegmentTree `apply`, `get`, `reserve`, `nodeCount` | `dual` | Per-segment affine composition mod 998244353 on all universes including 2^62 and the `INT64` extremes |
| Preconditions | `invalid` | 23 probes: reversed or too-wide universes (the width check uses unsigned arithmetic so a 2^63 span cannot overflow past it), keys and ranges outside `[lo, hi)`, reversed ranges, searches with a false identity predicate, split key beyond `hi`, `meld(a, a)`, lazy and dual variants |

Quick: 3 universes, 120 steps. Full: 10 universes, 900 steps, three builds. Stress: 1400 steps (sized so the `-O0` debug-iterator build with per-root verification stays inside the runner's 180 s limit). Mutation check (2026-10-08, quick mode, temporary copies): 9 of 9 injected faults detected (meld operand order, split boundary, lazy push to one child, dual composition order, right-first search descent, persistence without copying, split leaving the right child shared, `minLeft` leaf result, `get` stopping one level early).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/13-dynamicsegmenttree_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/13-dynamicsegmenttree_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/1' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-08 (re-audit pass, after its fixes): full mode seed 20261008 passed on both compilers, 13,214,123 checks per configuration and 23 assertion probes; stress mode seed 7 passed all three configurations with 20,610,112 checks per configuration (the optimized binary also passed seeds 1 and 2 at the earlier 2500-step size, 36.9 M checks each). All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, with `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, on GCC 16.2.1 and again on the floor compiler GCC 14.4.1 (`CXX=g++-14`). `02-integration.py --sanitizers` passed 108 standalone/aggregate headers (including this one alone, in `99-all.hpp` and in the two-translation-unit build), the workspace build and the sanitizer self-tests; `03-consistency.py` reports no errors and checks the closing-brace rule and comment cap on this header and its tester. Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

## Sources

| Source | Actual reading and use |
|---|---|
| maspypy `dynamic_segtree.hpp`, `dynamic_segtree_sparse.hpp`, `dynamic_lazy_segtree.hpp`, `dynamic_dual_segtree.hpp` | Root-handle API over a shared pool, `default_prod` (fill) functor, signed universe bounds, persistence flag, `enumerate`. |
| OI Wiki segment tree merge/split (https://oi-wiki.org/ds/seg-merge-split/) | Destructive meld with amortised `O(total nodes)` cost, split by key, node recycling when meld and split are mixed. |
| Nyaan dynamic segment tree, tko919 dynamic / dynamic lazy segtree, suisen sparse (lazy) segment tree | Operation catalog; `reserve`, `nodeCount`. |
| Library Checker `point_set_range_composite_large_array`, `range_affine_range_sum_large_array` | Large-universe targets for the plain and lazy trees. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): one-node-per-key sparse variant, range `clear`, split by count, meld on the lazy tree, `getAll` for the dual tree, vector build (that is row 12).
- Queries on the lazy tree allocate; a read-mostly workload on a huge universe should prefer the plain tree or offline compression.
- Persistent split/meld and `copyRange` stay with row 18.

## History

- 2026-10-08: P027 first verification; legacy `DynSegTree` reference deleted.
- 2026-10-08: independent review (P027) — fixed `allProd` returning a reference into the reallocating pool (finding 1, heap-use-after-free when storing one tree's total into another; now by value, aliasing case added to the tester), `meld(a, a)` corrupting the pool (finding 2, asserted), non-normalised empty split halves (finding 5, trimmed to handle 0), untested `reserve` on the lazy and dual trees and the `INT64` extreme universes (finding 8), and the undefined `n` in the complexity comments (finding 9).
- 2026-10-08: P027 re-audit pass (second session): the reviewer found signed overflow in `RangeSetRangeComposite::mapping` (fixed in `00-monoids.hpp`, row 12) reachable through a 2^62-long lazy node; added `lazyCompositeWide`, which failed under UBSan before the fix (seed 5) and passes after; the `M` comment now states the `PERSISTENT` bound.
