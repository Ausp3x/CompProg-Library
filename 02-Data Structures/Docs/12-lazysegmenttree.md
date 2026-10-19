# 12-lazysegmenttree.hpp — evidence

Package P027 (batch DS05). Replaces the unchanged extraction `SegTree` (`OLD/algorithms.cpp:2042–2213`). The acted monoids it runs on are row 00 ([00-monoids.md](00-monoids.md)). Indices are zero-based, ranges half-open, preconditions are `assert`s checked once per operation, and a moved-from tree may only be assigned to or destroyed.

## Contracts

### LazySegmentTree<A>

`0 <= n <= 2^29`. ACL-style iterative tree with `base = 2^lg >= n`; padding leaves hold `e()` and never receive a non-identity action because an action lands only on nodes fully inside a real range (and their descendants). `get(p)`, `set(p, x)`, `apply(p, f)`, `apply(l, r, f)`, `prod(l, r)` (`e()` for an empty range, operand order preserved), `allProd()` (O(1), const), `values()` (pushes every pending action, O(n)), `maxRight(l, pred)` = largest `r` in `[l, n]` with `pred(prod(l, r))`, `minLeft(r, pred)` = smallest `l` in `[0, r]` with `pred(prod(l, r))`; predicates are deterministic, true on `e()` (asserted) and monotone. `fold(l, r, x, act)` and `foldRight(l, r, x, act)` carry `x` through the range under a right or left action of the monoid part, with the contract of row 03 ([03-segmenttree.md](03-segmenttree.md#segmenttree)), after pushing both boundary paths as `prod` does. Constructors `LazySegmentTree(N = 0, mon = {})` (all `e()`; length-aware presets then have `len = 0`, so build from `leaf` values) and `LazySegmentTree(vector<S>, mon = {})` with an O(n) build. Callbacks must not touch the tree.

### DualSegmentTree<A>, CommutativeDualSegmentTree<A>

Store an action per point; `get(p)` returns the composition of every action applied to `p` (`id()` initially), `apply(l, r, f)`, `values()`; `DualSegmentTree` additionally has `set(p, f)` and pushes pending actions down the two boundary paths before composing, so noncommutative compositions keep their order; `CommutativeDualSegmentTree` uses a bottom-up tree of size `2n` (any `n`) without pushes and requires a commutative composition. Both accept a `vector<F>` of initial actions and a trailing `mon`; `MonoidAction<M>` turns a plain monoid into their action.

### Correctness and cost

The lazy tree is the ACL algorithm: push along both boundary paths before touching canonical nodes (`pushBounds`, shared by `apply`, `prod` and the folds), pull along them afterwards. The searches keep the accumulated product and descend only into a failing node, so each call touches `O(log n)` nodes; identity padding keeps the right-hand descent valid for nonpowers of two.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.cpp`, entry `12-lazysegmenttree_tester.py` (driver `_00_runner.py`; three builds; 27 assertion probes). Row 00's tester runs every preset through this tree as well.

| Operation | Group | Oracle |
|---|---|---|
| LazySegmentTree `set`, `get`, `apply` (point and range), `prod`, `allProd`, `values`, `fold`, `foldRight`, `maxRight`, `minLeft`, vector build, with the stored monoid | `lazy <Preset>` over `RangeAddRangeSum` (commutative), `RangeAssignRangeComposite` (noncommutative assignment), `RangeAffineRangeSumMinMaxIndex` (index-aware leaves) and a stateful struct (affine sums modulo a runtime modulus chosen by the seed) | A plain vector mutated by each action's meaning and brute aggregates, independent of `op`/`mapping`; all `O(n^2)` ranges after every step for `n <= 17`; sizes 0–100 (257 in stress), 300 steps per size (60 quick, 600 stress; four times as many for `n > 17`); searches with monotone threshold families and brute scans; `fold`/`foldRight` for `RangeAddRangeSum` (an `lng` carried through sums) and `RangeAssignRangeComposite` (a composite prepended or appended) against direct folds of the plain vector, each on its own copy and before any `prod` of the step so no earlier push helps it, at most `2 * (lg + 1)` act calls each; `foldOps`: an `op`-counting `RangeAddRangeSum` under pending range adds, every range up to 33 points, against sums with no `op` call |
| DualSegmentTree `apply`, `set`, `get`, `values`, size and vector builds; CommutativeDualSegmentTree `apply`, `get`, `values` | `dual` | Per-point affine pairs modulo P composed in order; per-point sums |
| Empty tree (including empty folds), folds that must push the left or the right boundary, size constructor, copy and move | `edges` | Direct values |
| Preconditions | `invalid` | 27 probes: negative/oversize sizes, `checkedSize`, out-of-range points and ranges, reversed ranges, searches with a false identity predicate, fold and foldRight ranges, dual variants |

## Commands and results

Latest run 2026-10-18, Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `12-lazysegmenttree_tester.py --mode full --seed 20261018` (GCC 16.2.1) | PASS, 3 configurations, 3,318,957 checks per configuration, 27 assertion probes, MEMORY peak 671 MB |
| `CXX=g++-14 12-lazysegmenttree_tester.py --mode full --seed 20261018` (GCC 14.4.1, floor) | PASS, 3,318,957 checks per configuration, MEMORY peak 720 MB |
| `12-lazysegmenttree_tester.py --mode stress --seed 7` | PASS, 9,264,545 checks per configuration, MEMORY peak 888 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1833 MB |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode full --seed 20261018
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode full --seed 20261018
python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| AtCoder Library lazysegtree (https://atcoder.github.io/ac-library/production/document_en/lazysegtree.html) | Iterative algorithm, `max_right`/`min_left` semantics. ACL is CC0. |
| maspypy `lazy_segtree.hpp`, `dual_segtree.hpp` | `get_all` (`values`), dual `set`. |
| OI Wiki segment tree (https://oi-wiki.org/ds/seg/) | Lazy propagation reference. |

Fetch dates and URLs: [00-sources.md](00-sources.md) (2026-10-08 section; the 2026-10-18 section for the folds).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): rollback lazy tree, xor-index range apply, run-based range-assignment tree.

## History

- 2026-10-08: P027 first verification; legacy `SegTree` replaced.
- 2026-10-08: independent review (P027), 1 finding fixed.
- 2026-10-08: P027 re-audit pass (second session), 1 finding fixed; full (seed 20261008) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-17: P027 monoid pass: the trees store their acted monoid; full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 1434/1437/1434 MB.
- 2026-10-18: presets moved to row 00 and their tests to its tester; full (seed 20261018) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 540/558/540 MB.
- 2026-10-18: `fold` and `foldRight` added, boundary pushes shared in `pushBounds`; independent review, 5 findings fixed (two test gaps, three document corrections); full (seed 20261018) on g++ and g++-14, stress (seed 7) and integration passed, MEMORY peak 671/720/888 MB.
