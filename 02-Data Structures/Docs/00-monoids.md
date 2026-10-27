# 00-monoids.hpp — evidence

Package P027 (batch DS05). Replaces the unchanged extractions `MonAlg`/`MonBin`/`MonGcd`/`MonSar`/`MonSet` (`OLD/algorithms.cpp:937–1316`). Presets, adapters and acted presets for the `03-cpp.md` monoid rule (API contracts).

## Contracts

### Monoid contract

`op` is associative and `e()` a two-sided identity. An acted monoid satisfies `mapping(id(), x) == x`, `mapping(composition(f, g), x) == mapping(f, mapping(g, x))`, `mapping(f, op(x, y)) == op(mapping(f, x), mapping(f, y))` and `mapping(f, e()) == e()` (a length or sentinel guard, which lets row 13's dynamic trees treat untouched ranges as identities). Dual consumers use only `F`, `composition` and `id`; `SparseTable` and a semigroup `DisjointSparseTable` use only `S` and `op`. Members are `static` (every preset, so `Type<T>::op` works) or `const` members reading runtime state (a modulus, a key array, a comparator flag). `M` must be move-constructible (a deleted move constructor is not supported), default-constructible only when `mon` is omitted, and adds no size when empty (asserted for `LazySegmentTree`, `SegmentTree` and `DualSegmentTree<MonoidAction<...>>`). Consumers: rows 03, 04, 06, 09 (`RollbackDSUAggregate`), 11, 12, 13, 14, 15, 17, 18, 19, 28, 29 (groups), 30, 34 (`SqrtTree`) and 65 (`ComponentSumDSU`, `DualDSU`); Graphs `07-lca.hpp` (`LCAFold`), `11-functionalgraph.hpp` (`FunctionalGraphFold`, `cycleAggregate`, `cycleAggregates`) and `16-centroiddecomposition.hpp` (`ContourFold`, `ContourApply`).

### Group contract

A group is a monoid with `inv(a)`, a two-sided inverse: `op(a, inv(a)) == op(inv(a), a) == e()`; `op` need not commute. `Sum<T>` (`inv` is `T(-a)`: signed values other than the lowest, unsigned and modular `T` wrap) and `Xor<T>` (`inv(a) = a`) are the group presets. A multiplicative group has no preset, because zero has no inverse; a caller's struct over units supplies one. Consumers: row 29 (`WeightedDSU`, `RollbackWeightedDSU`), which also needs `==` on `S`, and row 65 (`DualDSU`).

### Adapters

| Adapter | Construction | Meaning and domain |
|---|---|---|
| `Monoid<T, Op>` | `Monoid{unit, f}` (CTAD; write `lng(0)`, not `0`, so `T` matches the tree's values) | `op = f`, `e = unit`; `f` associative with identity `unit`; capturing lambdas carry runtime state |
| `ActedMonoid<M, U, Map, Comp>` | `ActedMonoid{m, noop, act, compose}` | extends monoid `m` with `mapping = act(f, x)`, `composition = compose(f, g)` (`f` after `g`), `id = noop` |
| `PairMonoid<A, B>` | `PairMonoid<A, B>{}` or `PairMonoid{a, b}` | `S = pair<A::S, B::S>`, componentwise |
| `ArrayMonoid<M, K>` | `ArrayMonoid<M, K>{}` | `S = array<M::S, K>`, componentwise; `M::S` default-constructible |
| `Opposite<M>` | `Opposite<M>{}` or `Opposite{m}` | `op(a, b) = M::op(b, a)`: products read right to left; keeps an acted `M`'s action; `Opposite<MinIndex<T>>` gives the rightmost minimum |
| `Reversible<M>` | `Reversible<M>{}` | `S {x, y}` holds the product and the reversed product, `reverse(s)` swaps them (the `ImplicitTreap` reverse hook), `leaf(v) = {v, v}`; an acted `M` acts on both halves |
| `MonoidAction<M>` | `MonoidAction<M>{}` or `MonoidAction{m}` | `M`'s values as dual-tree actions: `F = S`, `composition(f, g) = op(g, f)` (older first), `id = e`; covers range add, chmax, affine, clamp and bitwise point-get trees, and lambdas through `MonoidAction{Monoid{...}}` |
| `RangeAssignFlip<M>` | `RangeAssignFlip<M>{}`, `leaf(bit)`, `assign(bit)`, `flip()` | acted monoid over 0/1 sequences: `S {x, y, len}` keeps `M`'s fold of the bits and of their complement; `F {assign, flip}` sets every bit to `assign` (unless `-1`), then flips; `M` needs a static `leaf(bit)`; an assigning mapping costs `O(log(len))` calls of `M` (power by squaring), so a range assignment is `O(log(n)^2)` |

### Plain presets

`leaf` builders exist where a leaf needs derived fields (lengths, indices, counts); function composites take their builders or `{a, b}` directly. Every value bound is part of the domain.

| Preset | `S` | Identity | Domain and notes |
|---|---|---|---|
| `Sum<T>`, `Mul<T>` | `T` | `T(0)`, `T(1)` | ring or modular `T`; `Sum::inv` is negation |
| `Min<T>`, `Max<T>` | `T` | numeric max, lowest | `T` with numeric limits |
| `MinMax<T>` | `{mn, mx}` | `mn > mx` | as `Min`/`Max` |
| `Xor<T>`, `And<T>`, `Or<T>` | `T` | `0`, all ones, `0` | integral `T`; `Xor::inv` is the identity map |
| `Gcd<T>`, `Lcm<T>` | `T` | `0`, `1` | values `>= 0` (gcd) and `>= 1` with every lcm fitting `T` (lcm); `gcd(0, x) = abs(x)` breaks the identity on negative values |
| `AffineComposite<T>` | `{a, b}` (`x -> a x + b`) | `{1, 0}` | `op(f, g)` applies `f` first (Library Checker order); `eval(f, x)`; defaulted `operator==` when `T` has one, so row 18 skips copies under the identity action of the affine presets (user decision 2026-10-27, P030) |
| `AddChminChmax<T>` | `{a, lo, hi}` (`x -> max(min(x + a, hi), lo)`) | `{0, lowest, max}` | composite of add, chmin, chmax and assign (`add`, `chmin`, `chmax`, `assign`, `eval`); values and shifted bounds stay strictly inside (lowest, max), the sentinels |
| `BitwiseComposite<T = ulng>` | `{a, b}` (`x -> (x & a) ^ b`) | `{~0, 0}` | composite of and, or, xor and assign (`andWith`, `orWith`, `xorWith`, `assign`, `eval`) |
| `MinIndex<T, Less>`, `MaxIndex<T>` | `{x, i}` | `i = -1` | leftmost extreme under `Less`; `leaf(i, x)` |
| `MinCount<T, Less>`, `MaxCount<T>` | `{x, cnt}` | `cnt = 0` | extreme and its multiplicity |
| `Max2<T>` | `{x1, c1, x2, c2}` | counts `0` | two largest distinct values with multiplicities (count 0 marks absence); the smallest two are `Max2` on negated values |
| `MaxSubarray<T>` | `{sum, pre, suf, best, len}` | `len = 0` | best nonempty subarray, prefix and suffix; minimum subarray on negated values |
| `MaxGain<T>` | `{mn, mx, best}` | `mn > mx` | `best = max a[j] - a[i]` over `i <= j` (0 for one element); differences fit `T` |
| `RunCount<T>` | `{first, last, runs}` | `runs = 0` | maximal blocks of equal values, with the end values (blocks of ones in a 0/1 sequence are `(runs + first + last) / 2`) |
| `LongestRun` | `{len, ones, pre, suf, best}` | all `0` | 0/1 sequences: ones and the longest prefix, suffix and overall runs of ones |
| `BracketBalance` | `{open, close}` | `{0, 0}` | unmatched `(` and `)`; balanced iff both are 0, and `len - open - close` characters match |
| `MatrixProduct<T, K>` | `array<array<T, K>, K>` | identity matrix | ring `T`; `op(a, b) = a * b` in `O(K^3)` |
| `MaxPlusMatrix<T, K>` | `array<array<T, K>, K>` | `0` diagonal, lowest elsewhere | `(max, +)` product for segment-tree DP; numeric lowest means no entry; finite sums fit `T`; min-plus on negated entries |
| `MajorityVote<T>` | `{x, cnt}` | `cnt = 0` | Boyer–Moore candidate. `op` is not associative: every bracketing returns the strict majority of the range when one exists and an arbitrary value otherwise, so the caller verifies the candidate's count (for example with per-value position lists). The only preset outside the associativity contract, adopted by user decision. |

### Acted presets

Every preset but `RangeLinearRangeSum` has action builders; `F` is listed by meaning. The sum family shares `monoid_detail::SumLen<T>` (`{sum, len}`), the presets marked "on `X`" reuse plain preset `X`'s `S`, `op`, `e` and `leaf`, and composition reuses `AffineComposite`, `AddChminChmax`, `BitwiseComposite` or `MatrixProduct` where `F` is theirs.

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
| `RangeLinearRangeSum<T, K>` | `array<T, K>` (sums of K-vectors) | `K x K` matrix acting as `F * x`, composition `F * G`; the matrix is the action, so there is no builder | commutative ring `T`; covers sums of products under two adds (`K = 4` over `(1, a, b, ab)`), Fibonacci values under exponent add, power sums under add or assignment and weighted sums; mapping `O(K^2)`, composition `O(K^3)` |

### Legacy mapping

| Old | New |
|---|---|
| `RangeSetRangeComposite`, flag `set` in `F` of the assign presets | `RangeAssignRangeComposite`, flag `assign` |
| `RangeAddRangeArgmin` | `RangeAddRangeMinIndex` (reuses `MinIndex`) |
| `RangeAffineRangeMinMaxArg` | `RangeAffineRangeSumMinMaxIndex` |
| `RangeBitwiseRangeAndOrXor` | `RangeAndOrXorRangeAndOrXor` (row 14 keeps `RangeAndOrRangeSumMax`, already regular) |
| `RangeGcdLcm` | `RangeGcdLcmRangeGcdLcm` |
| `RangeAssignMaxSubarray` | `RangeAssignRangeMaxSubarray` |

Earlier legacy names: `MonAlg -> RangeAffineRangeSumMinMaxIndex`, `MonBin -> RangeAndOrXorRangeAndOrXor`, `MonGcd<opt> -> RangeGcdLcmRangeGcdLcm<T, LCM>`, `MonSar -> RangeAssignRangeMaxSubarray`, and the placeholder scaffold `MonSet` (range set with custom states) is the assign presets or `ActedMonoid` with the user's states; the legacy `init(i, x)` / `defR(l, r)` members are `leaf(...)` builders and, for row 13, the user's fill functor. Each consumer's evidence maps its own renames (`query` to `prod`, `fold` to `allProd` or `associativeProd`, `(id, f)` to `mon`).

### Aliases

Catalog names covered by the presets above (sweeps of 2026-10-17): maspypy `minidx`/`maxidx`/`minmincnt`/`minmax`/`affine`/`monoid_reverse`/`add_pair`/`summax`/`summin` (`MinIndex`, `MaxIndex`, `MinCount`, `MinMax`, `AffineComposite`, `Opposite`, `PairMonoid`), `sum_mul`, `min_assign`, `max_assign`, `minmax_add`, `summax_assign`, `summin_add`, `cntsum_add`, `cntsum_affine` (the affine presets), `max_max`/`min_min` (`MonoidAction<Max>`/`<Min>` on a dual tree, `RangeAddChminChmaxRangeMinMax` on a lazy tree), `add_array` (`ArrayMonoid<Sum>`), `from_monoid` (`MonoidAction`); ei1333 `LambdaMonoid`, `LambdaActedMonoid`, `Act` (`Monoid`, `ActedMonoid`, `MonoidAction{Monoid{...}}`); suisen `AffineClamp` (`AffineComposite` then `AddChminChmax`); Nyaan `Update*` presets and KACTL's add/set/max node (assign through the affine presets), `Range_Weighted_Add_Range_Sum` (`RangeAddRangeSum` with weighted `len`); Library Checker `range_set` (assign); `Min2` (`Max2` on negated values); ABC357F's sum of products (`RangeLinearRangeSum<T, 4>`).

### Correctness

- Sums under affine maps use `len`; sums of squares expand `(a x + b)^2`; min/max under affine maps swap on a negative scale; `RangeLinearRangeSum` is exact because a matrix action is linear and commutes with summation.
- `AddChminChmax`: `f` then `g` maps `x` to `max(min(x + f.a + g.a, min(f.hi + g.a, g.hi)), max(min(f.lo + g.a, g.hi), g.lo))` by distributing `min` over `max`; sentinels stay sentinels. The map is nondecreasing, so it commutes with `min` and `max` (`RangeAddChminChmaxRangeMinMax`).
- Bitwise: `x -> (x & a) ^ b` composes to `{f.a & g.a, (f.b & g.a) ^ g.b}`; the and/or folds follow the per-bit case split (`a` bit 0 forces `b`, `a` bit 1 keeps or complements) and xor uses the parity of `len`; per-bit counts become `len`, `0` or `len - cnt`.
- gcd/lcm commute with `x -> lcm(gcd(x, a), b)` because gcd and lcm distribute over each other on the divisibility lattice.
- Flip swaps zeros and ones and replaces inversions by `zero * one - inv`; `RangeAssignFlip` keeps both folds, so flip swaps them and an assignment builds `leaf(bit)^len` by squaring.
- `Max2` keeps the top two distinct values of the multiset union; `RunCount` subtracts one block when the boundary values match; `BracketBalance` matches `min(a.open, b.close)` pairs across the boundary; `MaxGain` adds the cross term `b.mx - a.mn`.
- `MajorityVote`: with `s_y = cnt` if the candidate is `y` and `-cnt` otherwise, every merge keeps `s_y >= count_y - (len - count_y)` for every value `y` (by cases on equal candidates and on the larger count), so a strict majority `y` ends with `s_y >= 1`, which forces it to be the candidate, whatever the bracketing.
- `Opposite` and `Reversible` reverse operand order, so `op` stays associative; `Reversible`'s mapping acts on both halves, which is valid because the action distributes over either operand order.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/00-monoids_tester.cpp`, entry `00-monoids_tester.py` (driver `_00_runner.py`; three builds; no assertion probes, because the presets have no preconditions of their own). The consumers are the harness: `LazySegmentTree`, `SegmentTree` and the dual trees. The runtime modulus of the stateful groups is chosen from `{1000000007, 998244353, 10007, 97}` by the seed.

| Operation | Group | Oracle |
|---|---|---|
| Every acted preset with its `leaf` and action builders; `RangeAssignFlip<LongestRun>`; `Opposite` and `Reversible` over the composite; `ActedMonoid` with plain and capturing lambdas; a stateful struct | `lazy <Preset>`, 29 groups (`RangeAndOrXorRangeSum` at 6 and 64 bits, `RangeGcdLcmRangeGcdLcm` without and with lcm) | A plain vector mutated by each action's meaning (`v += x`, `v = a v + b`, `min`/`max`/assign for the clamp, the and/or/xor/assign case table, gcd/lcm, flips, matrix-vector products modulo 998244353) and brute aggregates (sums, sums of squares, leftmost extremes, counts, Kadane, block counts, run lengths, inversion pairs, composites left to right and right to left, vector sums), independent of `op`/`mapping`; all `O(n^2)` ranges after every step for `n <= 17`; sizes 0–100 (257 in stress), 300 steps per size (60 quick, 600 stress; four times as many for `n > 17`); searches with monotone threshold families and brute scans |
| Plain presets; `Opposite<MinIndex>`; `Monoid` with plain and capturing lambdas; `PairMonoid`; `ArrayMonoid`; `Opposite` and `Reversible` over concatenation | `plain <Preset>`, 33 groups | Brute meaning of the raw values (sums, products modulo P, extremes, bit folds, Euclid gcd and lcm, affine, clamp and bitwise chains evaluated on probe inputs, top two by `map`, Kadane, gain by double loop, blocks, runs, a bracket stack, naive matrix and max-plus products, majority by counting, concatenations both ways); all ranges at every step for `n <= 17`, a random range and the whole range otherwise; 200 steps per size (40 quick, 400 stress); left and right identity of `e()` on every leaf, and left and right inverses wherever the preset has `inv` (`Sum`, `Xor`); `MajorityVote` checked only where a strict majority exists |
| `MonoidAction` over `AddChminChmax`, `BitwiseComposite`, `AffineComposite`, `Max` and a capturing `Monoid` | `MonoidAction on dual trees` | Per-point brute application of each action in order, compared through `eval` on per-point start values |
| Assignment powers, empty-monoid layout, group inverses at the domain edges | `contract` | Composite power 1000 and `RangeAssignFlip` power 700 against direct values; `sizeof` against a mirror struct without the monoid member; `inv` of `Sum<ulng>`, `Sum<uint8_t>` (wrapping), `Sum<mint>`, `Sum<lng>` at `LLONG_MAX` and `Xor<bool>` against direct values |

Every consumer tester also runs a stateful monoid (runtime modulus, key array or comparator flag) next to its static presets, as its own evidence records.

## Commands and results

Latest run 2026-10-23 (P035 `inv` on `Sum` and `Xor`), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, in the runner's optimized, checked and ASan/UBSan builds:

| Command | Result |
|---|---|
| `00-monoids_tester.py --mode full --seed 20261023` (GCC 16.2.1) | PASS, 3 configurations, 13,089,219 checks per configuration, MEMORY peak 1371 MB |
| `CXX=g++-14 00-monoids_tester.py --mode full --seed 20261023` (GCC 14.4.1, floor) | PASS, 13,089,219 checks per configuration, MEMORY peak 1387 MB |
| `00-monoids_tester.py --mode stress --seed 7` | PASS, 31,378,524 checks per configuration, MEMORY peak 1370 MB |
| `02-integration.py` | PASS, 174 standalone and aggregate headers, multi-TU, workspace; MEMORY peak 714 MB |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/00-monoids_tester.py' --mode full --seed 20261023
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/00-monoids_tester.py' --mode full --seed 20261023
python3 '96-Local Testing/02-Data Structures/00-monoids_tester.py' --mode stress --seed 7
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| AtCoder Library lazysegtree (https://atcoder.github.io/ac-library/production/document_en/lazysegtree.html) | `op/e/mapping/composition/id` contract and the flip/inversion example. ACL is CC0. |
| maspypy `alg/monoid`, `alg/acted_monoid`, `alg/acted_set` | Clamp, top-two, bitwise-function and monoid-as-action presets; min-with-count. |
| ei1333 `structure/class` (`monoid.hpp`, `acted-monoid.hpp`, `act.hpp`), hitonanode `segmenttree/trees`, suisen `algebra/monoid`, Nyaan lazy utility, KACTL LazySegmentTree | Adapter shapes, `Range<Action>Range<Fold>` naming, clamp and assign/add presets. |
| Contest problems (AtCoder ABC 223F, 261E, 285F, 322F, 343F, 357F, 360F; practice2 L; CF 380C, 718C, 145E, 2057D; Luogu P1471, P2572, P7453, P6327; SDOI2011 coloring) | Which presets recur; listed per preset above. |

Fetch dates, URLs and failed fetches: [00-sources.md](00-sources.md) (2026-10-08 and 2026-10-17 sections).

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p027-omissions): plain `Assign`, `AddDivide`, `Dummy`/dihedral monoids, public `monoid_pow`, one-problem presets (Fibonacci-progression add, elementary symmetric sums, pairs of acted monoids), the Hotel descent.
- `MajorityVote` is outside the associativity contract by design; `Gcd` takes values `>= 0` only.
- `RangeAssignRangeComposite` and an assigning `RangeAssignFlip` cost `O(log(n)^2)` per assignment; `RangeAndOrXorRangeSum` costs `O(B)` per node operation; `RangeLinearRangeSum` costs `O(K^3)` per composition.

## History

- 2026-10-08: P027 first verification, within row 12; legacy `Mon*` presets replaced.
- 2026-10-08: independent review (P027), 1 finding fixed; re-audit pass, 1 finding fixed in this header; full (seed 20261008) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-17: P027 monoid pass: stored-object contract, regular preset names, 26 plain presets, 8 adapters and 8 new acted presets after catalog and problem sweeps; independent review, 9 findings fixed.
- 2026-10-18: own row 00 (split from row 12), with its own tester and evidence, the monoid rule in `03-cpp.md` and an explicit comment-cap exemption; full (seed 20261018) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 1377/1400/1376 MB.
- 2026-10-23: P035 (the user's yes to editing this row): `inv` on `Sum` and `Xor`, the group contract, inverse checks in `plain` and at the domain edges in `contract` (added after the P035 independent review); full (seed 20261023) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 1371/1387/1370 MB.
- 2026-10-27: P030 (user decision 2026-10-27 to edit this row): defaulted `operator==` on `AffineComposite<T>::S`; full (seed 20261027) on g++ and g++-14 passed for rows 00 and 12, MEMORY peak 1366/1384 MB (row 00) and 678/716 MB (row 12).
