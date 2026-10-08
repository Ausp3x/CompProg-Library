# 12-lazysegmenttree.hpp — evidence

Package P027 (batch DS05), which also owns the support header `00-monoids.hpp`. Replaces the unchanged extractions `SegTree` (`OLD/algorithms.cpp:2042–2213`) and `MonAlg`/`MonBin`/`MonGcd`/`MonSar` (`OLD/algorithms.cpp:937–1258`). Indices are zero-based, ranges half-open, preconditions are `assert`s checked once per operation, and a moved-from tree may only be assigned to or destroyed.

## Contracts

### Acted monoid contract (`00-monoids.hpp`)

A preset `A` provides `S`, `F`, `static S op(const S &, const S &)` (associative), `static S e()` (two-sided identity), `static S mapping(const F &, const S &)`, `static F composition(const F &f, const F &g)` meaning "`f` after `g`", and `static F id()`. Required laws: `mapping(id(), x) == x`, `mapping(composition(f, g), x) == mapping(f, mapping(g, x))`, `mapping(f, op(x, y)) == op(mapping(f, x), mapping(f, y))`. Every preset also maps `e()` to `e()` (length or sentinel guard), which lets the same preset drive the dynamic trees of row 13 where untouched ranges are identities. Presets that need per-element data expose `leaf(...)` builders; `RangeArithmeticAddRangeSum::progression(l, a, d)` encodes "add `a + d * (i - l)` on `[l, r)`" as an index-absolute action. Monoid-only consumers (row 13 `DynamicSegmentTree`) use just `S`, `op`, `e`; dual consumers use just `F`, `composition`, `id`.

| Preset | `S` | `F` | Domain and notes |
|---|---|---|---|
| `RangeAddRangeSum<T>` | `{sum, len}` | `T` | `T` commutative ring; `len` is a `T` so the dynamic tree can hold 2^62 points |
| `RangeAddRangeMin<T>`, `RangeAddRangeMax<T>` | `T` | `T` | identity is `numeric max` / `lowest`, which is also the untouched-value sentinel; stored values stay strictly inside |
| `RangeAddRangeArgmin<T>` | `{x, i}` | `T` | leftmost minimum, `i = -1` marks the identity; `leaf(i, x)` |
| `RangeAddRangeMinCount<T>` | `{x, cnt}` | `T` | minimum and its multiplicity, `cnt = 0` marks the identity (rectangle-union sweeps, "count zeros under range add") |
| `RangeAffineRangeSum<T>` | `{sum, len}` | `{a, b}` | `x -> a * x + b`; composition `{f.a g.a, f.a g.b + f.b}` |
| `RangeAssignRangeSum<T>` | `{sum, len}` | `{set, x}` | assign when `set`; composition keeps the newer assignment |
| `RangeSetRangeComposite<T>` | `{a, b, len}` | `{set, a, b}` | `op(p, q)` applies `p` first (Library Checker order); `mapping` raises the assigned map to the power `len` by squaring, so a range assign costs `O(log^2 n)` in a tree; `leaf(a, b)` |
| `RangeArithmeticAddRangeSum<T>` | `{sum, len, idx}` | `{b, d}` | adds `b + d * i` at absolute index `i`; `idx` is the sum of indices; `leaf(i, x)`, `progression` |
| `RangeAffineRangeMinMaxArg<T>` | `{sum, mx, mn, len, lo, mxi, mni}` | `{a, b}` | affine maps of any sign: `a < 0` swaps min/max and their indices, `a == 0` assigns and points both argument indices at `lo`, the leftmost index of the node; ties resolve leftmost; `len = 0` marks the identity; `leaf(i, x)` |
| `RangeBitwiseRangeAndOrXor<T = ulng>` | `{band, bor, bxor, len}` | `{a, b}` | `x -> (x & a) ^ b` encodes and (`andWith`), or (`orWith`), xor (`xorWith`) and assign (`assign`); the aggregate maps are exact bitwise identities; `len` parity handles xor |
| `RangeGcdLcm<T, LCM = false>` | `{g, l, len}` | `{a, b}` | values `>= 0`, `x -> lcm(gcd(x, a), b)` encodes gcd-with (`gcdWith`), lcm-with (`lcmWith`) and assign; exact because divisibility is a distributive lattice with 0 on top; `l` is maintained only when `LCM`; every lcm of applied operands must fit `T` |
| `RangeAssignMaxSubarray<T>` | `{sum, pre, suf, best, len}` | `{set, x}` | best nonempty subarray sum with max prefix/suffix; `len = 0` marks the identity; min-subarray is the same preset on negated values |

The former legacy names map as `MonAlg -> RangeAffineRangeMinMaxArg` (gains the `lo` field that fixes argmin/argmax after an assignment), `MonBin -> RangeBitwiseRangeAndOrXor`, `MonGcd<opt> -> RangeGcdLcm<T, LCM>`, `MonSar -> RangeAssignMaxSubarray` (the min-subarray fields are dropped; negate the values instead). The legacy `init(i, x)` / `defR(l, r)` members are `leaf(...)` builders and, for row 13, the user's fill functor.

### LazySegmentTree<A>

`0 <= n <= 2^29`. ACL-style iterative tree with `base = 2^lg >= n`; padding leaves hold `e()` and never receive a non-identity action because an action lands only on nodes fully inside a real range (and their descendants). `get(p)`, `set(p, x)`, `apply(p, f)`, `apply(l, r, f)`, `prod(l, r)` (returns `e()` for an empty range, operand order preserved), `allProd()` (O(1), const), `values()` (pushes every pending action, O(n)), `maxRight(l, pred)` = largest `r` in `[l, n]` with `pred(prod(l, r))`, `minLeft(r, pred)` = smallest `l` in `[0, r]` with `pred(prod(l, r))`; predicates are deterministic, true on `e()` (asserted) and monotone (true on a prefix of growing ranges). Constructors: size (all `e()` — note that length-aware presets then have `len = 0`; build from `leaf` values instead) and `vector<S>` with an O(n) build. Callbacks must not touch the tree.

### DualSegmentTree<A>, CommutativeDualSegmentTree<A>

Store an action per point; `get(p)` returns the composition of every action applied to `p` (`id()` initially), `apply(l, r, f)`, `values()`; `DualSegmentTree` additionally has `set(p, f)` and pushes pending actions down the two boundary paths before composing, so noncommutative compositions keep their order; `CommutativeDualSegmentTree` uses a bottom-up tree of size `2n` (any `n`) without pushes and requires a commutative composition. Both accept a `vector<F>` of initial actions.

## Correctness and cost

The lazy tree is the ACL algorithm (push along both boundary paths before touching canonical nodes, pull along them afterwards). The search descents keep the accumulated product in `acc` and only descend into a node after it fails, so each call touches `O(log n)` nodes; identity padding keeps the right-hand descent valid for nonpowers of two. Each preset is proven exact as an algebra: sums under affine maps use `len`; min/max under affine maps swap on negative scale; bitwise and/or under `(x & a) ^ b` follow from the per-bit case split (`a` bit 0 forces `b`; `a` bit 1 keeps or complements) and xor uses the parity of `len`; gcd/lcm commute with the action because gcd and lcm distribute over each other. `RangeSetRangeComposite::mapping` is `O(log len)`, giving `O(log^2 n)` per range assignment; an `O(log n)` amortized alternative is `IntervalMap` (row 24) with a point-update segment tree of powered maps, recorded under limits.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.cpp`, entry `12-lazysegmenttree_tester.py` (driver `_00_runner.py`; three builds; 21 assertion probes).

| Operation | Group | Oracle |
|---|---|---|
| LazySegmentTree `set`, `get`, `apply` (point), `apply` (range), `prod`, `allProd`, `values`, `maxRight`, `minLeft`, vector build; every preset including `leaf`, `progression`, `andWith`/`orWith`/`xorWith`/`assign`, `gcdWith`/`lcmWith`/`assign` | `preset<Spec>` for 14 specs | A plain vector mutated element by element by the action's *meaning* (e.g. `v = a * v + b`, `v = (v & a) ^ b`, `v = lcm(gcd(v, a), b)`, progression `v += a + d * (i - l)`) and a brute aggregate (`Kadane`, leftmost argmin, bit folds, modular composite) independent of `op`/`mapping`; after every step all `O(n^2)` ranges are re-read for `n <= 17`; sizes 0–100 (257 in stress), 300 steps per size (60 quick, 800 stress); searches use monotone threshold families and brute scans |
| DualSegmentTree `apply`, `set`, `get`, `values`, size and vector builds | `dual` | Per-point affine pairs mod 998244353 composed in order |
| CommutativeDualSegmentTree `apply`, `get`, `values` | `dual` | Per-point sums |
| Empty tree, size constructor, copy/move independence, composite power 1000 | `edges` | Direct values |
| Preconditions | `invalid` | 21 probes: negative/oversize sizes, `checkedSize`, out-of-range points and ranges, reversed ranges, searches with a false identity predicate, dual variants |

Mutation check (2026-10-08, quick mode, temporary copies): 10 of 12 injected faults detected (reversed affine composition, rightmost argmin tie, dual push order, assignment argmin index, xor parity, missing pull after range apply, commutative-dual `values` skipping a node, lcm field, max-subarray suffix, min-count merge); the two survivors are equivalent (powers of one affine map commute; a no-op).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/1' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

Final run 2026-10-08 (after the review fixes): full mode seed 20261008 passed on both compilers, 4,113,802 checks per configuration and 21 assertion probes; stress mode seed 7 passed all three configurations with 16,727,836 checks per configuration. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, with `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, plus the assertion probes) and ASan/UBSan (leak checking, `halt_on_error`) configurations, on GCC 16.2.1 and again on the floor compiler GCC 14.4.1 (`CXX=g++-14`). `02-integration.py --sanitizers` passed 104 standalone/aggregate headers (including this one alone, in `99-all.hpp` and in the two-translation-unit build), the workspace build and the sanitizer self-tests; `03-consistency.py` reports no errors and checks the closing-brace rule and comment cap on this header and its tester. Exact GCC 14.2 and the Windows build stay unrun; PyPy is not involved.

## Sources

| Source | Actual reading and use |
|---|---|
| AtCoder Library lazysegtree (https://atcoder.github.io/ac-library/production/document_en/lazysegtree.html) | `op/e/mapping/composition/id` contract, iterative algorithm, `max_right`/`min_left` semantics. ACL is CC0. |
| maspypy `lazy_segtree.hpp`, `dual_segtree.hpp`, `alg/acted_monoid/minmincnt_add.hpp` | `get_all` (`values`), dual `set`, min-with-count preset. |
| Library Checker `range_affine_range_sum`, `range_set_range_composite`, AtCoder practice2 L | Preset selection and the composite operand order. |
| Nyaan lazy-segment-tree-utility, KACTL LazySegmentTree | Catalog of assign/add/min/max presets (expressible with the affine min/max preset). |
| OI Wiki segment tree (https://oi-wiki.org/ds/seg/) | Lazy propagation reference. |

References inspected 2026-10-08 by the completeness sweep ([00-sources.md](00-sources.md)).

## Limits and handoffs

- Rows 15 (`SegTree2D`) and 18 (`PerSegTree`) still consume the legacy `Mon` contract (`idS/idF/defR/init/ope/map/cmp`); their packages adopt the acted-monoid contract above, using `leaf` builders and a fill functor in place of `init`/`defR`.
- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): rollback lazy tree, xor-index range apply, run-based range-assignment tree, power-sum assignment preset, flip/inversion preset, chmin/chmax-only presets.
- `RangeSetRangeComposite` costs `O(log^2 n)` per assignment (power by squaring inside `mapping`).

## History

- 2026-10-08: P027 first verification; legacy `SegTree` and `Mon*` presets replaced.
- 2026-10-08: independent review (P027) — every preset's algebra, the ACL push/pull order, padding and both dual trees were confirmed; only a complexity-comment form and tester `std::get` qualification were corrected (finding 9).
