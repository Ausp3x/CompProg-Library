# 12-lazysegmenttree.hpp — evidence

Package P027 (batch DS05), which also owns the support header `00-monoids.hpp`. The 2026-10-17 monoid pass made monoids stored objects in every consumer, replaced `query`/`allQuery`/`fold` with `prod`/`allProd`, regularized the preset names and added the presets, adapters and combinators found by two completeness sweeps (library catalogs and contest problems). Indices are zero-based, ranges half-open, preconditions are `assert`s checked once per operation, and a moved-from tree may only be assigned to or destroyed.

## Contracts

### Monoid contract (`00-monoids.hpp`)

A monoid `M` provides `S`, `op(a, b)` (associative) and `e()` (two-sided identity). An acted monoid adds `F`, `mapping(f, x)`, `composition(f, g)` meaning "`f` after `g`" and `id()`, with `mapping(id(), x) == x`, `mapping(composition(f, g), x) == mapping(f, mapping(g, x))`, `mapping(f, op(x, y)) == op(mapping(f, x), mapping(f, y))` and `mapping(f, e()) == e()` (length or sentinel guard, which lets the dynamic trees of row 13 treat untouched ranges as identities). Dual consumers use only `F`, `composition` and `id`; `SparseTable` and a semigroup `DisjointSparseTable` use only `S` and `op`.

Monoids are objects. Every consumer stores `[[no_unique_address]] M m` (acted and dual trees `A m`), takes it as constructor argument `mon = {}` after its data arguments (only `LCAFold` keeps its optional `root` after it) and calls `m.op`, `m.e()`, `m.mapping`, `m.composition`, `m.id()`. Members may be `static` (all presets are, so `Type<T>::op` works) or `const` members reading runtime state (a modulus, a key array, a comparator flag); lambdas enter through `Monoid` and `ActedMonoid`. `M` must be move-constructible (a deleted move constructor is not supported) and default-constructible only when `mon` is omitted. An empty `M` adds no size (asserted for `LazySegmentTree`, `SegmentTree` and `DualSegmentTree<MonoidAction<...>>`). The consumers are rows 03, 04, 06, 09 (`RollbackDSUAggregate`), 11, 12, 13, 14, 15, 17, 19, 28, 30 and 34 (`SqrtTree`), and Graphs `07-lca.hpp` (`LCAFold`), `11-functionalgraph.hpp` (`FunctionalGraphFold`, `cycleAggregate`, `cycleAggregates`) and `16-centroiddecomposition.hpp` (`ContourFold`, `ContourApply`); function parameters that pass a monoid are also named `mon`.

### Adapters and combinators

| Adapter | Construction | Meaning and domain |
|---|---|---|
| `Monoid<T, Op>` | `Monoid{unit, f}` (CTAD; write `lng(0)`, not `0`, so `T` matches the tree's values) | `op = f`, `e = unit`; `f` associative with identity `unit`; capturing lambdas carry runtime state |
| `ActedMonoid<M, U, Map, Comp>` | `ActedMonoid{m, noop, act, compose}` | extends monoid `m` with `mapping = act(f, x)`, `composition = compose(f, g)` (`f` after `g`), `id = noop` |
| `PairMonoid<A, B>` | `PairMonoid<A, B>{}` or `PairMonoid{a, b}` | `S = pair<A::S, B::S>`, componentwise (sum and max together, and so on) |
| `ArrayMonoid<M, K>` | `ArrayMonoid<M, K>{}` | `S = array<M::S, K>`, componentwise; `M::S` default-constructible |
| `Opposite<M>` | `Opposite<M>{}` or `Opposite{m}` | `op(a, b) = M::op(b, a)`: products read right to left; keeps an acted `M`'s action; `Opposite<MinIndex<T>>` gives the rightmost minimum |
| `Reversible<M>` | `Reversible<M>{}` | `S {x, y}` holds the product and the reversed product, `reverse(s)` swaps them (the `ImplicitTreap` reverse hook), `leaf(v) = {v, v}`; an acted `M` acts on both halves |
| `MonoidAction<M>` | `MonoidAction<M>{}` or `MonoidAction{m}` | `M`'s values as dual-tree actions: `F = S`, `composition(f, g) = op(g, f)` (older first), `id = e`; covers range add, chmax, affine, clamp and bitwise point-get trees and lambdas via `MonoidAction{Monoid{...}}` |
| `RangeAssignFlip<M>` | `RangeAssignFlip<M>{}`, `leaf(bit)`, `assign(bit)`, `flip()` | acted monoid over 0/1 sequences: `S {x, y, len}` keeps `M`'s fold of the bits and of their complement; `F {assign, flip}` sets every bit to `assign` (unless `-1`), then flips; `M` needs a static `leaf(bit)`; an assigning mapping costs `O(log(len))` calls of `M` (power by squaring), so a range assignment is `O(log(n)^2)` |

### Plain presets

`leaf` builders exist where a leaf needs derived fields (lengths, indices, counts); function composites take their builders or `{a, b}` directly. Every value bound is part of the domain.

| Preset | `S` | Identity | Domain and notes |
|---|---|---|---|
| `Sum<T>`, `Mul<T>` | `T` | `T(0)`, `T(1)` | ring or modular `T` |
| `Min<T>`, `Max<T>` | `T` | numeric max, lowest | `T` with numeric limits |
| `MinMax<T>` | `{mn, mx}` | `mn > mx` | as `Min`/`Max` |
| `Xor<T>`, `And<T>`, `Or<T>` | `T` | `0`, all ones, `0` | integral `T` |
| `Gcd<T>`, `Lcm<T>` | `T` | `0`, `1` | values `>= 0` (gcd) and `>= 1` with every lcm fitting `T` (lcm); `gcd(0, x) = abs(x)` breaks the identity on negative values |
| `AffineComposite<T>` | `{a, b}` (`x -> a x + b`) | `{1, 0}` | `op(f, g)` applies `f` first (Library Checker order); `eval(f, x)` |
| `AddChminChmax<T>` | `{a, lo, hi}` (`x -> max(min(x + a, hi), lo)`) | `{0, lowest, max}` | composite of add, chmin, chmax and assign (`add`, `chmin`, `chmax`, `assign`, `eval`); values and shifted bounds stay strictly inside (lowest, max), the sentinels |
| `BitwiseComposite<T = ulng>` | `{a, b}` (`x -> (x & a) ^ b`) | `{~0, 0}` | composite of and, or, xor and assign (`andWith`, `orWith`, `xorWith`, `assign`, `eval`) |
| `MinIndex<T, Less>`, `MaxIndex<T>` | `{x, i}` | `i = -1` | leftmost extreme under `Less`; `leaf(i, x)` |
| `MinCount<T, Less>`, `MaxCount<T>` | `{x, cnt}` | `cnt = 0` | extreme and its multiplicity |
| `Max2<T>` | `{x1, c1, x2, c2}` | counts `0` | two largest distinct values with multiplicities (count 0 marks absence); smallest two are `Max2` on negated values |
| `MaxSubarray<T>` | `{sum, pre, suf, best, len}` | `len = 0` | best nonempty subarray, prefix and suffix; minimum subarray on negated values |
| `MaxGain<T>` | `{mn, mx, best}` | `mn > mx` | `best = max a[j] - a[i]` over `i <= j` (0 for one element); differences fit `T` |
| `RunCount<T>` | `{first, last, runs}` | `runs = 0` | maximal blocks of equal values, with the end values (blocks of ones in a 0/1 sequence are `(runs + first + last) / 2`) |
| `LongestRun` | `{len, ones, pre, suf, best}` | all `0` | 0/1 sequences: ones and the longest prefix, suffix and overall runs of ones |
| `BracketBalance` | `{open, close}` | `{0, 0}` | unmatched `(` and `)`; balanced iff both are 0, and `len - open - close` characters match |
| `MatrixProduct<T, K>` | `array<array<T, K>, K>` | identity matrix | ring `T`; `op(a, b) = a * b` in `O(K^3)` |
| `MaxPlusMatrix<T, K>` | `array<array<T, K>, K>` | `0` diagonal, lowest elsewhere | `(max, +)` product for segment-tree DP; numeric lowest means no entry; finite sums fit `T`; min-plus on negated entries |
| `MajorityVote<T>` | `{x, cnt}` | `cnt = 0` | Boyer–Moore candidate. `op` is not associative; every bracketing returns the strict majority of the range when one exists and an arbitrary value otherwise, so the caller verifies the candidate's count (for example with per-value position lists). The only preset outside the associativity contract, adopted by user decision. |

### Acted presets

Every preset has its action builders; `F` is listed by meaning. The sum family shares `monoid_detail::SumLen<T>` (`{sum, len}`), and the presets marked "on `X`" reuse plain preset `X`'s `S`, `op`, `e` and `leaf`; composition reuses `AffineComposite`, `AddChminChmax`, `BitwiseComposite` or `MatrixProduct` where `F` is theirs.

| Preset | `S` | `F` and builders | Domain and notes |
|---|---|---|---|
| `RangeAddRangeSum<T>` | `SumLen` | add (`add`) | commutative ring `T`; `len` is a `T` so the dynamic tree can hold 2^62 points; a leaf may carry a weight as `len` (weighted range add) |
| `RangeAddRangeMin<T>`, `RangeAddRangeMax<T>` | on `Min`, `Max` | add (`add`) | values stay strictly inside the identity sentinel |
| `RangeAddRangeMinIndex<T, Less>`, `RangeAddRangeMaxIndex<T>` | on `MinIndex`, `MaxIndex` | add (`add`) | leftmost extreme |
| `RangeAddRangeMinCount<T, Less>`, `RangeAddRangeMaxCount<T>` | on `MinCount`, `MaxCount` | add (`add`) | extreme and multiplicity (rectangle-union sweeps, count of zeros) |
| `RangeAffineRangeSum<T>` | `SumLen` | `x -> a x + b` (`affine`) | add, multiply and assign are affine maps |
| `RangeAffineRangeSumSquares<T>` | `{sum, sq, len}` | `x -> a x + b` (`affine`) | `sq' = a^2 sq + 2ab sum + b^2 len`; variance is `sq / len - (sum / len)^2` |
| `RangeAffineRangeSumMinMaxIndex<T>` | `{sum, mx, mn, len, lo, mxi, mni}` | `x -> a x + b` (`affine`) | any sign of `a`: negative swaps min/max and their indices, zero assigns and points both indices at `lo`; ties leftmost; `len = 0` marks the identity |
| `RangeAssignRangeSum<T>` | `SumLen` | `{assign, x}` (`assign`) | newer assignment wins |
| `RangeAssignRangeComposite<T>` | `{a, b, len}` | `{assign, a, b}` (`assign`) | `op(p, q)` applies `p` first; `mapping` powers the assigned map by squaring only while bits of `len` remain, so a range assignment costs `O(log(n)^2)`; `leaf(a, b)` |
| `RangeAssignRangeMaxSubarray<T>` | on `MaxSubarray` | `{assign, x}` (`assign`) | |
| `RangeAssignRangeRunCount<T>` | on `RunCount` | `{assign, x}` (`assign`) | range painting with block counts (SDOI2011 coloring) |
| `RangeArithmeticAddRangeSum<T>` | `{sum, len, idx}` | `{b, d}` adds `b + d i` (`progression(l, a, d)` adds `a + d (i - l)`) | `leaf(i, x)` |
| `RangeAndOrXorRangeAndOrXor<T = ulng>` | `{band, bor, bxor, len}` | `BitwiseComposite` (`andWith`, `orWith`, `xorWith`, `assign`) | exact per-bit case split; `len` parity handles xor |
| `RangeAndOrXorRangeSum<T = ulng, B>` | `{cnt[B], len}` | `BitwiseComposite` (same builders) | values in `[0, 2^B)`; per-bit counts make and/or/xor/assign exact on the sum, `O(B)` per call; `sum(s)` is the range sum modulo `2^w` for unsigned `T`; a node holds `B + 1` counters (520 bytes at the default `B = 64`, so a lazy tree over 2^18 points needs about 270 MB: pick `B` from the value range) |
| `RangeGcdLcmRangeGcdLcm<T, LCM = false>` | `{g, l, len}` | `x -> lcm(gcd(x, a), b)` (`gcdWith`, `lcmWith`, `assign`) | values `>= 0`; `l` is kept only when `LCM`, and every lcm of applied operands must fit `T` |
| `RangeAddChminChmaxRangeMinMax<T>` | on `MinMax` | `AddChminChmax` (`add`, `chmin`, `chmax`, `assign`) | the clamp map is monotone, so it maps min to min and max to max exactly (range sum under chmin needs beats, row 14) |
| `RangeFlipRangeInversion` | `{zero, one, inv}` (`lng`) | flip (`flip`) | 0/1 sequences; inversions count a one before a zero; `leaf(bit)` (AtCoder practice2 L) |
| `RangeLinearRangeSum<T, K>` | `array<T, K>` (sums of K-vectors) | `K x K` matrix acting as `F * x`, composition `F * G`; the action is the matrix itself, so it needs no builder | commutative ring `T`; covers sums of products under two adds (`K = 4` over `(1, a, b, ab)`), Fibonacci values under exponent add, power sums under add or assignment and weighted sums; mapping `O(K^2)`, composition `O(K^3)` |

### Renamed and merged operations

| Old | New |
|---|---|
| `RangeSetRangeComposite`, flag `set` in `F` of the assign presets | `RangeAssignRangeComposite`, flag `assign` |
| `RangeAddRangeArgmin` | `RangeAddRangeMinIndex` (reuses `MinIndex`) |
| `RangeAffineRangeMinMaxArg` | `RangeAffineRangeSumMinMaxIndex` |
| `RangeBitwiseRangeAndOrXor` | `RangeAndOrXorRangeAndOrXor` (row 14 keeps `RangeAndOrRangeSumMax`, already regular) |
| `RangeGcdLcm` | `RangeGcdLcmRangeGcdLcm` |
| `RangeAssignMaxSubarray` | `RangeAssignRangeMaxSubarray` |
| static `A::op` calls, `(id, f)` constructor arguments | stored `m`, constructor argument `mon` (all consumers) |
| `SegmentTree::query`, `allQuery` | `prod`, `allProd` |
| `SparseTable::query`, `fold`; `SparseTable2D::query` | `prod` (idempotent, O(1)), `associativeProd` (O(log n)); `prod` |
| `DisjointSparseTable::query`, `fold` | `prod` (calls `e()` only when `M` has it, so a semigroup keeps nonempty ranges) |
| `SWAGQueue`/`SWAGDeque`/`SWAGStack::fold` | `allProd` |
| `SqrtDecomp::opeUpdate`, `query` | `apply`, `prod` |
| `SuffixArray::Minimum` | `Min<int>` |

Earlier legacy names: `MonAlg -> RangeAffineRangeSumMinMaxIndex`, `MonBin -> RangeAndOrXorRangeAndOrXor`, `MonGcd<opt> -> RangeGcdLcmRangeGcdLcm<T, LCM>`, `MonSar -> RangeAssignRangeMaxSubarray`; the legacy `init(i, x)` / `defR(l, r)` members are `leaf(...)` builders and, for row 13, the user's fill functor.

### Aliases

Catalog names covered by the presets above (sweeps of 2026-10-17): maspypy `minidx`/`maxidx`/`minmincnt`/`minmax`/`affine`/`monoid_reverse`/`add_pair`/`summax`/`summin` (`MinIndex`, `MaxIndex`, `MinCount`, `MinMax`, `AffineComposite`, `Opposite`, `PairMonoid`), `sum_mul`, `min_assign`, `max_assign`, `minmax_add`, `summax_assign`, `summin_add`, `cntsum_add`, `cntsum_affine` (the affine presets), `max_max`/`min_min` (`MonoidAction<Max>`/`<Min>` on a dual tree, `RangeAddChminChmaxRangeMinMax` on a lazy tree), `add_array` (`ArrayMonoid<Sum>`), `from_monoid` (`MonoidAction`); ei1333 `LambdaMonoid`, `LambdaActedMonoid`, `Act` (`Monoid`, `ActedMonoid`, `MonoidAction{Monoid{...}}`); suisen `AffineClamp` (`AffineComposite` then `AddChminChmax`); Nyaan `Update*` presets and KACTL's add/set/max node (assign through the affine presets), `Range_Weighted_Add_Range_Sum` (`RangeAddRangeSum` with weighted `len`); Library Checker `range_set` (assign); `Min2` (`Max2` on negated values); ABC357F's sum of products (`RangeLinearRangeSum<T, 4>`).

### LazySegmentTree<A>

`0 <= n <= 2^29`. ACL-style iterative tree with `base = 2^lg >= n`; padding leaves hold `e()` and never receive a non-identity action because an action lands only on nodes fully inside a real range (and their descendants). `get(p)`, `set(p, x)`, `apply(p, f)`, `apply(l, r, f)`, `prod(l, r)` (`e()` for an empty range, operand order preserved), `allProd()` (O(1), const), `values()` (pushes every pending action, O(n)), `maxRight(l, pred)` = largest `r` in `[l, n]` with `pred(prod(l, r))`, `minLeft(r, pred)` = smallest `l` in `[0, r]` with `pred(prod(l, r))`; predicates are deterministic, true on `e()` (asserted) and monotone. Constructors `LazySegmentTree(N = 0, mon = {})` (all `e()`; length-aware presets then have `len = 0`, so build from `leaf` values) and `LazySegmentTree(vector<S>, mon = {})` with an O(n) build. Callbacks must not touch the tree.

### DualSegmentTree<A>, CommutativeDualSegmentTree<A>

Store an action per point; `get(p)` returns the composition of every action applied to `p` (`id()` initially), `apply(l, r, f)`, `values()`; `DualSegmentTree` additionally has `set(p, f)` and pushes pending actions down the two boundary paths before composing, so noncommutative compositions keep their order; `CommutativeDualSegmentTree` uses a bottom-up tree of size `2n` (any `n`) without pushes and requires a commutative composition. Both accept a `vector<F>` of initial actions and a trailing `mon`.

### Correctness and cost

The lazy tree is the ACL algorithm (push along both boundary paths before touching canonical nodes, pull along them afterwards); searches keep the accumulated product and descend only into a failing node, so each call touches `O(log n)` nodes. Preset algebra:

- Sums under affine maps use `len`; sums of squares expand `(a x + b)^2`; min/max under affine maps swap on a negative scale; `RangeLinearRangeSum` is exact because a matrix action is linear and commutes with summation.
- `AddChminChmax`: `f` then `g` maps `x` to `max(min(x + f.a + g.a, min(f.hi + g.a, g.hi)), max(min(f.lo + g.a, g.hi), g.lo))` by distributing `min` over `max`; sentinels stay sentinels. The map is nondecreasing, so it commutes with `min` and `max` (`RangeAddChminChmaxRangeMinMax`).
- Bitwise: `x -> (x & a) ^ b` composes to `{f.a & g.a, (f.b & g.a) ^ g.b}`; the and/or folds follow the per-bit case split (`a` bit 0 forces `b`, `a` bit 1 keeps or complements) and xor uses the parity of `len`; per-bit counts become `len`, `0` or `len - cnt`.
- gcd/lcm commute with `x -> lcm(gcd(x, a), b)` because gcd and lcm distribute over each other on the divisibility lattice.
- Flip swaps zeros and ones and replaces inversions by `zero * one - inv`; `RangeAssignFlip` keeps both folds, so flip swaps them and an assignment builds `leaf(bit)^len` by squaring.
- `Max2` keeps the top two distinct values of the multiset union; `RunCount` subtracts one block when the boundary values match; `BracketBalance` matches `min(a.open, b.close)` pairs across the boundary; `MaxGain` adds the cross term `b.mx - a.mn`.
- `MajorityVote`: with `s_y = cnt` if the candidate is `y` and `-cnt` otherwise, every merge keeps `s_y >= count_y - (len - count_y)` for every value `y` (by cases on equal candidates and on the larger count), so a strict majority `y` ends with `s_y >= 1`, which forces it to be the candidate, whatever the bracketing.
- `Opposite` and `Reversible` reverse operand order, so `op` stays associative; `Reversible`'s mapping acts on both halves, which is valid because the action distributes over either operand order.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.cpp`, entry `12-lazysegmenttree_tester.py` (driver `_00_runner.py`; three builds; 21 assertion probes). The runtime modulus for the stateful groups is chosen from `{1000000007, 998244353, 10007, 97}` by the seed.

| Operation | Group | Oracle |
|---|---|---|
| LazySegmentTree `set`, `get`, `apply` (point and range), `prod`, `allProd`, `values`, `maxRight`, `minLeft`, vector build, with every acted preset, its `leaf` and action builders | `lazy <Preset>` for 29 groups: the 21 acted presets (`RangeAndOrXorRangeSum` at 6 and 64 bits, `RangeGcdLcmRangeGcdLcm` without and with lcm), `RangeAssignFlip<LongestRun>`, `Opposite` and `Reversible` over the composite, `ActedMonoid` lambdas, a stateful struct and capturing lambdas | A plain vector mutated by each action's meaning (`v += x`, `v = a v + b`, `min`/`max`/assign for the clamp, the and/or/xor/assign case table, gcd/lcm, flips, matrix-vector products modulo 998244353) and brute aggregates (sums, sums of squares, leftmost extremes, counts, Kadane, block counts, run lengths, inversion pairs, composites left to right and right to left, vector sums), independent of `op`/`mapping`; all `O(n^2)` ranges after every step for `n <= 17`; sizes 0–100 (257 in stress), 300 steps per size (60 quick, 600 stress; four times as many for n > 17); searches with monotone threshold families and brute scans |
| Plain presets and adapters through `SegmentTree<M>` | `plain <Preset>` for 33 groups (the 26 plain presets, `Opposite<MinIndex>`, `Monoid` with a plain and a capturing lambda, `PairMonoid`, `ArrayMonoid`, `Opposite` and `Reversible` over concatenation) | Brute meaning of the raw values (sums, products modulo P, extremes, bit folds, Euclid gcd and lcm, affine and clamp and bitwise chains evaluated on probe inputs, top two by `map`, Kadane, gain by double loop, blocks, runs, a bracket stack, naive matrix and max-plus products, majority by counting, concatenations forward and backward); all ranges at every step for `n <= 17`, a random range and the whole range otherwise; 200 steps per size (40 quick, 400 stress); left and right identity of `e()` on every leaf checked; `MajorityVote` checked only where a strict majority exists |
| `MonoidAction` with `AddChminChmax`, `BitwiseComposite`, `AffineComposite` (dual), `Max` (commutative dual) and a capturing `Monoid` | `MonoidAction on dual trees` | Per-point brute application of each action in order, compared through `eval` on per-point start values |
| DualSegmentTree `apply`, `set`, `get`, `values`, size and vector builds; CommutativeDualSegmentTree `apply`, `get`, `values` | `dual` | Per-point affine pairs modulo P composed in order; per-point sums |
| Empty tree, size constructor, copy and move, composite power 1000, `RangeAssignFlip` assignment power 700, empty-monoid layout | `edges` | Direct values; `sizeof` against a mirror struct without the monoid member |
| Preconditions | `invalid` | 21 probes: negative/oversize sizes, `checkedSize`, out-of-range points and ranges, reversed ranges, searches with a false identity predicate, dual variants |

The consumer testers cover the contract in their own structures: every one has a stateful-monoid group (runtime modulus, key array or comparator flag) next to its static presets, listed in its own evidence document.

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `12-lazysegmenttree_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 13,713,267 checks per configuration, 21 assertion probes, MEMORY peak 1434 MB |
| `CXX=g++-14 12-lazysegmenttree_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 13,713,267 checks per configuration, MEMORY peak 1437 MB |
| `12-lazysegmenttree_tester.py --mode stress --seed 7` | PASS, 35,040,109 checks per configuration, MEMORY peak 1434 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/02-Data Structures/12-lazysegmenttree_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| AtCoder Library lazysegtree (https://atcoder.github.io/ac-library/production/document_en/lazysegtree.html) | `op/e/mapping/composition/id` contract, iterative algorithm, `max_right`/`min_left` semantics, the flip/inversion example. ACL is CC0. |
| maspypy `alg/monoid`, `alg/acted_monoid`, `alg/acted_set`, `lazy_segtree.hpp`, `dual_segtree.hpp` | Clamp, top-two, bitwise-function and monoid-as-action presets; `values`, dual `set`, min-with-count. |
| ei1333 `structure/class` (`monoid.hpp`, `acted-monoid.hpp`, `act.hpp`), hitonanode `segmenttree/trees`, suisen `algebra/monoid`, Nyaan lazy utility, KACTL LazySegmentTree | Adapter shapes, `Range<Action>Range<Fold>` naming, clamp and assign/add presets. |
| Contest problems (AtCoder ABC 223F, 261E, 285F, 322F, 343F, 357F, 360F; practice2 L; CF 380C, 718C, 145E, 2057D; Luogu P1471, P2572, P7453, P6327; SDOI2011 coloring) | Which presets recur; listed per preset above. |
| OI Wiki segment tree (https://oi-wiki.org/ds/seg/) | Lazy propagation reference. |

Fetch dates, URLs and failed fetches: [00-sources.md](00-sources.md) (2026-10-08 and 2026-10-17 sections).

## Limits and handoffs

- Row 18 (`PerSegTree`) still consumes the legacy `Mon` contract (`idS/idF/defR/init/ope/map/cmp`); its package adopts the stored-object contract above, using `leaf` builders and a fill functor in place of `init`/`defR`.
- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): plain `Assign`, `AddDivide`, `Dummy`/dihedral monoids, public `monoid_pow`, one-problem presets (Fibonacci-progression add, elementary symmetric sums, pairs of acted monoids), the Hotel descent, rollback and xor-index lazy trees, run-based assignment trees.
- `MajorityVote` is outside the associativity contract by design (above); `Gcd` takes values `>= 0` only.
- `RangeAssignRangeComposite` and an assigning `RangeAssignFlip` cost `O(log(n)^2)` per assignment; `RangeAndOrXorRangeSum` costs `O(B)` per node operation; `RangeLinearRangeSum` costs `O(K^3)` per composition.
- `00-monoids.hpp` carries one required complexity line per struct (53 of 618 non-blank lines, 8.6%); the 8% comment cap applies to row headers, and every comment here is the mandatory complexity and domain line.
- `HashMonoid` (Strings row 23) stays a static preset: its base lives in `SequenceHash` because only `leaf` and `assignment` need it.

## History

- 2026-10-08: P027 first verification; legacy `SegTree` and `Mon*` presets replaced.
- 2026-10-08: independent review (P027), 1 finding fixed.
- 2026-10-08: P027 re-audit pass (second session), 1 finding fixed; full (seed 20261008) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-17: P027 monoid pass: stored-object contract in every consumer, `prod`/`allProd` naming, regular preset names, 26 plain presets, 8 adapters and 8 new acted presets after catalog and problem sweeps; independent review: 9 findings fixed (a `SWAGQueue::push` aliasing defect, the P034 consumers on `master` converted, contract wording, a direct include, a complexity line, implicit SWAG constructors, documentation); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 1434/1437/1434 MB.
