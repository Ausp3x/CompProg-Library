# 34-slope_trick.hpp — evidence

`34-slope_trick.hpp` (batch MI23, package P075) maintains convex piecewise-linear functions under the slope-trick operations. It has three structures. `SlopeTrick` stores unit breakpoints in two binary heaps. `WeightedSlopeTrick` stores weighted breakpoints in a treap with an exact `+INF` domain, which adds domain restriction, slope clamping and infimal convolution. `DualSlopeTrick` stores the convex conjugate of a finite-domain integer function. Dependencies: the C01 template (P002) and `01-random.hpp` (treap priorities). Contest profile: no ISA-specific code. The code is independently implemented and adapts no external code. No online submission was made.

## Contracts

All three structures represent a convex function `f`. `INF = numeric_limits<T>::max()` is the sentinel: an unbounded end of `argminRange()` or `domain()` is `-INF` or `INF`, and `WeightedSlopeTrick::evaluate` and `DualSlopeTrick::atZero` return `INF` outside the domain. Positions, shifts and query points must stay strictly inside `(-INF/2, INF/2)`, and every queried function value must lie in `(-INF, INF)`, since `INF` means outside the domain. `slide(a, b)` requires `a <= b` (asserted) and sets `f(x) <- min over y in [x - b, x - a] of f(y)`. `shift(d)` sets `f(x) <- f(x - d)`. Copies are independent values. `merge` and `minPlusConvolution` take their operand by value, so pass `std::move(g)` to keep the stated bound; a self-merge copies first and is valid. A moved-from `WeightedSlopeTrick` is the zero function on `(-INF, INF)`. A moved-from `SlopeTrick` or `DualSlopeTrick` is valid but unspecified, so every operation on it is safe. Single-threaded use is the baseline.

### SlopeTrick

`f(x) = mn + sum over l in L of (l - x)^+ + sum over r in R of (x - r)^+`, with `max L <= min R`. `L` is stored in the max-heap `left` and `R` in the min-heap `right`, as raw values with lazy offsets `addl` and `addr`. `T` is any signed integral or floating type; slopes change by one unit per breakpoint. The default function is `0`. Positions and the cumulative offsets `addl`/`addr` must stay in `(-INF/2, INF/2)`, so raw values fit; `T = double` is exact for dyadic inputs and otherwise subject only to the rounding of additions.

- `min()` is `mn`. `argminRange()` is `[max L, min R]`, with `-INF` for empty `L` and `INF` for empty `R`. `size()` is `|L| + |R|`, the slope at `+inf` minus the slope at `-inf`. `evaluate(x)` sums every term without mutation in `O(n)`; partial sums are monotone, so they fit whenever `f(x)` does.
- `addXMinusA(a)` adds `(x - a)^+`. If `a >= max L`, the new term vanishes on the old argmin and `a` joins `R`. Otherwise the minimum rises by `max L - a`, `a` joins `L` and the old `max L` moves to `R`. `addAMinusX(a)` is the mirror image. `addAbs(a)` applies both. `addConst(c)` adds `c` to `mn`.
- `addLinear(a)` adds `a * x` for an integral `a` in `[-|R|, |L|]` (asserted; outside it `f` has no minimum). Each unit moves the top of `L` to `R` and adds its position to `mn`, which is `f + x` at the new argmin; the mirror case handles negative `a`. Cost `O(|a| log n)`.
- `prefixMin()` clears `R`, giving `f(x) <- min over y <= x of f(y)`. `suffixMin()` clears `L`. `shift(d)` adds `d` to both offsets. `slide(a, b)` adds `a` to `addl` and `b` to `addr`: the decreasing part moves by `a` and the increasing part by `b`, which is the window minimum.
- `merge(g)` replaces `f` by `f + g`. It swaps so that `*this` is the larger, then adds each term of the smaller through `addAMinusX`/`addXMinusA`: `O(min(n, m) log(n + m))`, and `O(n log^2 n)` over a merge tree.

### WeightedSlopeTrick

`f(x) = k - wl * x + sum of w * (x - p)^+` over the stored breakpoints `(p, w)`, on `[lo, hi]`, and `+INF` outside. `k` is `lll`. Invariants:
- every stored `p` lies in `[lo, hi]`;
- `wl >= 0` when `lo` is finite;
- `wl <= W` (total stored weight) when `hi` is finite.

`T` is a signed integral type of at most 64 bits. Requirements:
- weights `w >= 0` (asserted; zero is a no-op);
- `W` and `|wl|` within `INF/2`;
- positions within `INF/2`.

All sums of `w * p` stay below `2^126` in `lll`; returned values are converted to `T`. The default is `0` on `(-INF, INF)`. The treap is keyed by position and keeps subtree weight `sw`, `lll` weight-times-position `sx`, size and a lazy position offset. Priorities come from the global `rng` (`01-random.hpp`, clock-seeded; `rng = Random(seed)` replays). Randomness affects only running time (Las Vegas): every result is deterministic. Removed subtrees go to a free list and are reclaimed lazily, so the pool never exceeds the peak live node count by more than the few nodes one operation allocates before discarding. Balance: the in-order position of every node is independent of the priorities. `insert` puts a new key before its duplicates, `unite` puts the first operand's duplicates before the second's, and splits preserve order. The shape is therefore a random treap, with expected depth `O(log n)`. Ordering duplicates by priority would build a chain.

- Queries:
  - `bounded()` is `0 <= wl <= W`: the minimum exists, which is the precondition of `min`, `argminRange`, `clampSlope` and `minPlusConvolution`. `min()` asserts it and returns `k - (sum of the first wl units of p)`, the value at the `wl`-th unit.
  - `argminRange()` asserts it and returns the positions of units `wl` and `wl + 1`, falling back to `lo`/`hi` when `wl = 0` or `wl = W`.
  - `evaluate(x)` is `k - wl * x + x * W(<x) - S(<x)` from one root-to-leaf descent in `O(log n)`, and `INF` outside `[lo, hi]`. `domain()` is `[lo, hi]`.
- Adds:
  - `addXMinusA(a, w)` inserts `(a, w)`. When `a < lo` the term is linear on the domain, so `k -= w * a` and `wl -= w`; when `a > hi` it vanishes.
  - `addAMinusX(a, w)` uses `w * (a - x)^+ = w * (x - a)^+ + w * (a - x)`, which adds `w * a` to `k` and `w` to `wl`. `addAbs(a, w)` inserts `2w` with the same linear part.
  - `addLinear(a)` subtracts `a` from `wl`; any `a` is allowed, and an unbounded result stays valid for `evaluate`, adds, shifts, `slide` and `merge`.
  - `addConst(c)` adds to `k`.
  - After each change `norm()` restores the invariants. With `wl < 0` and finite `lo`, it rewrites `-wl * x` as `|wl| (x - lo)^+ + |wl| lo` on the domain. With `wl > W` and finite `hi`, it adds weight `wl - W` at `hi`, which is zero on the domain.
- `restrictDomain(l, r)` intersects the domain with `[l, r]`, and `-INF`/`INF` leave a side open. The result must be nonempty (asserted). Breakpoints below `lo` become linear and those above `hi` vanish.
- `prefixMin()` requires `wl >= 0` (asserted). It keeps the first `wl` units, splitting one node if needed, and opens `hi` to `INF`. `suffixMin()` requires `wl <= W`; it sets `k` to the minimum, drops the first `wl` units and opens `lo`.
- `shift(d)` adds `d` to every position and to the finite ends, with `k += wl * d`.
- `slide(a, b)` splits at `clamp(wl, 0, W)` units, adds `a` to the left part and `b` to the right, moves `lo` by `a` and `hi` by `b`, and sets `k += wl * (wl < 0 ? b : a)`. Unbounded states are therefore exact too: an increasing `f` moves by `b`, a decreasing one by `a`.
- `clampSlope(s1, s2)`, with `s1 <= 0 <= s2` and a bounded `f` (both asserted), computes `min over y of f(y) + phi(x - y)` where `phi(t) = s2 * t` for `t >= 0` and `s1 * t` for `t <= 0`; this clamps every slope into `[s1, s2]`. It keeps:
  - the top `min(wl, -s1)` units of the left part and the first `min(W - wl, s2)` units of the right part;
  - at a finite domain end, the slopes that the wall contributes up to `s1` or `s2`, as a node at `lo` or `hi`.

  `k` is rebuilt from the unchanged minimum, and the domain becomes `(-INF, INF)`. `clampSlope(-c, c)` is the transition `f(x) <- min over y of f(y) + c * |x - y|`.
- `merge(g)` replaces `f` by `f + g` on the intersection of the domains, which must be nonempty (asserted). The smaller operand is read in order in `O(m)` and rebuilt in the larger pool with fresh priorities by a stack-based Cartesian-tree build in `O(m)`. Fresh priorities matter: a copy that kept its priorities would chain on self-merges or merges with shifted copies. The two treaps are then united by the Blelloch–Reid-Miller split-and-recurse union, in expected `O(m log(n / m + 1))`, which totals `O(n log n)` over a merge tree. Finally `merge` clips to the new domain and adds `k` and `wl`.
- `minPlusConvolution(g)` computes `h(z) = min over x of f(x) + g(z - x)` for bounded `f` and `g` (asserted). Proof: `h* = f* + g*` and slopes are integers, so the `r`-th unit of `h` counted outward from the argmin lies at the sum of the `r`-th units of `f` and `g`. A finite domain end acts as infinitely many units at that end, and an infinite end with no units stops the pairing.
  - The structure keeps the larger operand. For each run of the smaller operand's units, in rank order, it adds the run's position to the matching rank range of the larger, which preserves order because both sequences are sorted. Excess ranks pair with a wall or are dropped.
  - The minimum is `min f + min g`, and the domain is the Minkowski sum.
  - Cost: expected `O(m log n)` for an `m`-node smaller operand.

### DualSlopeTrick

`f` is an integer convex function, finite exactly on an integer interval `domain()`, and `+INF` elsewhere. Initially `f(0) = 0` and `f(x) = +INF` for `x != 0`. Requirements:
- `T` is signed integral, and weights `w >= 0` (asserted).
- Slopes of `f` and the products `|a| * |c|`, where `c` is the anchor below, must stay within `INF/2`.

The structure keeps an anchor `c = clamp(0, domain())` and a `SlopeTrick<T> g` equal to the convex conjugate of `f(x + c)`. The breakpoints of `g` are the slopes `f(i + 1) - f(i)` of `f`, one unit per integer step. Hence `domain() = [c - |g.L|, c + |g.R|]`, and `f(c) = -min g`.

- `atZero()` is `-g.mn` when `c = 0` and `INF` otherwise, in `O(1)`.
- `addConst(b)` is `g -= b`.
- `addLinear(a)` is `g(p) <- g(p - a) - a * c`, in `O(1)`.
- `shift(d)` moves the domain by `d`, re-anchors at `c' = clamp(0, new domain)` and adds `-e * p` to `g` with `e = c' - d - c`. The precondition of `g.addLinear` follows from `c, c' - d in domain`. Cost `O(min(|d|, n) log n)`.
- `addXMinusA(a, w)` adds `w * (x - a)^+`. It is linear when `a <= min domain` and a no-op when `a >= max domain`. Otherwise it convolves `g` with the conjugate `a' * p` on `[0, w]` (`a' = a - c`) via `addLinear(-a')`, `slide(0, w)`, `addLinear(a')`, in `O(min(|a|, n) log n)`. The cost does not depend on `w`. `addAMinusX(a, w)` adds `w * (a - x)^+ = w * (x - a)^+ - w * x + w * a` and costs the same.
- `slide(a, b)` is `shift(a)`, then `b - a` unit relus at `p = 0` in `g` (the support function of `[0, b - a]`), then a re-anchor. Cost `O((min(|a|, n) + b - a) log n)`; the domain widens by `b - a`.

Every operation here is also available on `WeightedSlopeTrick` in expected `O(log n)`, so the dual heaps are the deterministic heap-only choice for small arguments and slopes up to `INF/2`. They are 12x faster on the choose-half benchmark (below). hitonanode documents its `shift` as `O(log n)`, but its loop runs `|d|` times. This header keeps an exact finite domain instead of the `INF/2` sentinel slopes.

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| Oracle | Exact `lll` values on the integers of `[-128, 128]` with `+INF` outside the domain, extended linearly. A driver-side breakpoint bound keeps every breakpoint and finite domain end in `[-124, 124]`, which makes each brute operation exact. Validated by hand fixtures (values, both extrapolation directions) and a convexity and extent check after every operation. |
| `SlopeTrick`: all 14 operations | Random sequences on four maps `p -> s*p + t`: identity; `(2^40, 2^59)`; `(2^20, -2^59)`; `(3, -2^61)`. Plus `int` `(3, 2^28)` and `double` `(0.5, 0.25)`. Each operation is followed by checks of `size`, `min`, `argminRange` (with `-INF`/`INF`) and `evaluate`. Both `merge` forms (moved, copied lvalue) and self-merge are covered. `addLinear` is drawn from `[-min(3, \|R\|), min(3, \|L\|)]`. Fixtures cover the empty function and positions near `2^62`. |
| `WeightedSlopeTrick`: all 18 operations | Random sequences over every operation, with weights 0–6, on four maps: identity; `(2^20, 2^40, u = 2^10)`; `(2^30, -2^59)`; `u = 2^40`. Plus `int` `(3, 2^20, u = 4)`. Each operation is followed by checks of `domain`, `evaluate` (`INF` outside) and, when the oracle says bounded, `min` and `argminRange` and the `bounded()` state. Unbounded states from `addLinear` are covered. Operands for `merge`/`minPlusConvolution` are random short sequences, moved or copied, with either side larger. `restrictDomain` has open and closed ends. |
| `DualSlopeTrick`: all 8 operations | Random sequences on three maps `t, u`: identity; `(2^40, 2^20)`; `(-2^59, 1)`. Domains far from 0 are included. Each operation is followed by checks of `domain`, `atZero` and `f(x)` by shift-then-`atZero` on a copy, at the domain ends ±2 and at dense or sampled interior points. Fixtures cover `INF` at zero, slide widening and a `2^50` weight. |
| Independent end-to-end DPs | Monotone fit (non-decreasing via `prefixMin`, non-increasing via `suffixMin`), unit and weighted, against an `O(n V)` table at every `x`. Bounded-step tracking (`slide`) and linear move cost (`clampSlope`) against `O(n V D)` and `O(n V^2)` tables. APIO 2016 Fireworks (`restrictDomain`, `minPlusConvolution`, `merge`) against a tree DP. Choose exactly K (`DualSlopeTrick` and `WeightedSlopeTrick`) against a sorting closed form with costs up to `5 * 10^14`. |
| Heavy weights, complexity guards | Ping-pong: `n` unit points plus alternating weight-`10^6` breakpoints at `±10^6`, the pattern that makes heaps of pairs quadratic, checked against a weighted-median oracle. Treap depth is at most `6 * bit_width(n) + 16` after ping-pong, Fireworks and the cross-check; this guard catches a duplicate-key ordering bug at depth 1008 for 4000 nodes. |
| Balance regressions | Treap depth is checked in four cases: 16000 single-node merges at one key, 16000-leaf equal-edge Fireworks stars, 12 self-merges (262144 nodes) and 12 shifted-copy merges. The value checks are `5n`, `2n`, `2^12 f` and the binomial sum `sum C(12, j) f(x - j)`. Moved-from objects are reused after `merge` and `minPlusConvolution`. |
| Supporting only | Unit and weighted structures agree on 2·10^5 (full) and 10^6 (stress) random operations. Node reuse holds: the pool stays within peak live nodes + 8. Copies stay independent. |
| Preconditions | 20 checked-build assertion probes: reversed `slide` (all three), out-of-range `SlopeTrick::addLinear`, negative weights in every weighted add (five), empty domain, disjoint `merge`, `min`/`argminRange`/`prefixMin`/`suffixMin` on unbounded functions, `clampSlope` with `s1 > 0`, with `s2 < 0` and on an unbounded function, `minPlusConvolution` with either operand unbounded. |

Mutation check (2026-10-13): 27 hand mutations of the header ran against quick mode. 26 failed the suite; the survivor (clipping nodes at exactly `hi`) is an equivalent mutant, because those nodes are zero on the domain and `norm()` restores the weight. Three more mutations restored the defects that review found. The first sends duplicates right in `insert`, the second orders `unite` duplicates by priority, and the third keeps the operand's priorities in `merge`. The depth guards caught each one: depths 1008 for 4000 nodes, 4000 for 4000, and 1539 for 16384.

## Commands and results

Run 2026-10-13: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14.7. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` (with the probes), and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined` with leak checking (full and stress).

```bash
python3 '96-Local Testing/06-Miscellaneous/34-slope_trick_tester.py' --mode quick --seed 1             # PASS, 2 configurations, 20 probes, 156,345 checks, MEMORY peak 489 MB
python3 '96-Local Testing/06-Miscellaneous/34-slope_trick_tester.py' --mode full --seed 1              # PASS, 3 configurations, 20 probes, 3,293,364 checks, MEMORY peak 605 MB
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/34-slope_trick_tester.py' --mode full --seed 2   # PASS, 3 configurations, 20 probes, 3,313,146 checks, MEMORY peak 641 MB
python3 '96-Local Testing/06-Miscellaneous/34-slope_trick_tester.py' --mode stress --seed 3            # PASS, 3 configurations, 7,548,614 checks, MEMORY peak 708 MB
python3 '96-Local Testing/06-Miscellaneous/34-slope_trick_benchmark.py'                                # PASS, 21 rows, answers agree, MEMORY peak 339 MB
python3 '96-Local Testing/02-integration.py' --sanitizers                                               # PASS, 136 standalone/aggregate headers, multi-TU, workspace, sanitizers, MEMORY peak 2737 MB
python3 '96-Local Testing/03-consistency.py'                                                            # no errors
```

The memory peaks are dominated by compiling the tester; the test binary alone peaks near 60 MB (full mode). The stress random section ran 3760 sequences with 241,577 operations, and 15 sequences stopped early at the window bound. Full mode ran 121,049 operations with no early stops.

Modes:
- **quick:** optimized and checked builds. Per map: 40 unit, 40 weighted and 30 dual sequences of up to 16 operations (20 for the `int`/`double` maps). Classics: 10 rounds each. Balance: 4000 merges and 8 doublings. Ping-pong 2000, cross-check 20000.
- **full:** adds ASan/UBSan. 8x the sequences, of up to 48 operations. Classics: 80 rounds. Balance: 16000 merges and 12 doublings. Ping-pong 50000, cross-check 200000.
- **stress:** full-mode counts with up to 96 operations, ping-pong 200000, cross-check 10^6.

The `_GLIBCXX_DEBUG` build verifies the whole heap on every push and pop. Stress volume therefore comes from more seeds, not more rounds, to stay under the runner's 180 s limit.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/06-Miscellaneous/34-slope_trick_benchmark.py>) wrote its local, git-ignored `34-slope_trick_benchmark.json` on 2026-10-13: GCC 16.2.1, GNU++20 `-O2 -DNDEBUG`, i9-11900H, shared host. Each row is one warmup plus five repetitions in rotating order. Inputs are seeded uniform draws. Every repetition of every implementation returned the same answer. Medians:

| Workload | n | Implementation | Median ms |
|---|---|---|---|
| monotone fit (`prefixMin`, `addAbs`) | 200000 | SlopeTrick / WeightedSlopeTrick | 14.8 / 102 |
| monotone fit | 1000000 | SlopeTrick / WeightedSlopeTrick | 86.6 / 1040 |
| bounded step (`slide(-1000, 1000)`, `addAbs`) | 200000 | SlopeTrick / WeightedSlopeTrick | 20.8 / 249 |
| bounded step | 1000000 | SlopeTrick / WeightedSlopeTrick | 130 / 2041 |
| weighted fit, weights 1–8 | 200000 | repeated unit `addAbs` / one weighted `addAbs` | 72.8 / 141 |
| ping-pong (weight `n` at `±10^6`) | 2500 / 5000 / 10000 | heaps of pairs (benchmark-only) | 245 / 1076 / 4373 |
| ping-pong | 2500 / 5000 / 10000 / 200000 | WeightedSlopeTrick | 0.94 / 2.41 / 5.31 / 310 |
| tree merge (`prefixMin`, `addAbs`, merge into parent) | 200000 | SlopeTrick small-to-large / treap union | 29.6 / 180 |
| choose half of `n` items | 200000 | DualSlopeTrick / WeightedSlopeTrick | 18.8 / 262 |

Conclusions:
- On unit workloads the binary heaps are 6–16x faster than the treap, so both structures stay.
- With small total weights (1–8), repeating unit operations is 1.9x faster. The treap wins once weights grow, and it is the only bounded choice for weights such as `10^9`.
- Heaps of `(position, count)` pairs grow quadratically on ping-pong (4x time per doubling of `n`), which is why the weighted variant is a treap.
- On this shallow random tree, small-to-large heaps beat treap union in practice, despite the `log` factor.
- No timing gate is used.

Host variance: the shared host varied by up to 2x during the session. The same treap binary took 1047–1349 ms on the 10^6 monotone fit across repeated runs, and an A/B against the header without the move operations showed no difference. Ratios between implementations stayed stable.

## Sources

Fetched on 2026-10-13 by `@researcher` (ledger in [00-sources.md](00-sources.md)). They were read for operation sets, semantics and stated complexities; the implementation, the proofs above and every test are independent.

Adopted from the 2026-10-13 sweep: `addLinear` (all three), `size`, `restrictDomain`/`domain`, `clampSlope`, `minPlusConvolution`, non-destructive `O(log n)` `evaluate`, weighted adds, treap-union `merge`.

- hitonanode `slope_trick` and `dual_slope_trick`: the unit API and the conjugate representation. Its loops make `shift` cost `O(|d| log n)`, not the documented `O(log n)`.
- maspypy `slope_trick_0`, `slope_trick_1`, `slope_trick_super` and articles (1) and (3): `add_linear`, `restrict_domain`, `convolve`/`convolve_segment`, and the conjugate trick.
- ei1333 `slope-trick` and `generalized-slope-trick`; Nyaan `slope-trick-weighted`: weighted adds and BST sums for non-destructive evaluation.
- OI Wiki Slope Trick and USACO Guide: infimal convolution, mergeable heaps, Fireworks.
- Treap union: G. E. Blelloch and M. Reid-Miller, *Fast set operations using treaps*, SPAA 1998. The expected `O(m log(n / m + 1))` bound is the published result; the paper itself was not read for this package.

Aliases in the fetched libraries:

| This header | Other libraries |
|---|---|
| `addConst` | `add_all` (EI), `add_const`, `shift_y` (NYA) |
| `addXMinusA` | `add_relu` (HIT), `add_x_minus_a`, `add_xma` (NYA), `addx_a` (TKO), `add_right` (SUI) |
| `addAMinusX` | `add_irelu`, `add_a_minus_x`, `add_amx`, `adda_x`, `add_left` (SUI) |
| `prefixMin` / `suffixMin` | `clear_right` / `clear_left`, `chmin_right` / `chmin_left`, `cumulative_min_left` / `cumulative_min_right` (SUI), `clear_inc` (MAS) |
| `shift` | `translate` (HIT, SUI), `shift(a)` (EI, MAS), `shift_x` (NYA) |
| `slide` | `shift(a, b)` (EI), `sliding_window_minimum` (MAS, SUI), `move_right_curve(w)` = `slide(0, w)` (HIT) |
| `min`, `argminRange` | `get_min`, `query` (EI), `argmin_range` (SUI) |
| `evaluate` | `get`, `get_destructive`, `eval` |
| `merge` | `merge_destructive` (HIT), `add(f, g)` (MAS) |
| `minPlusConvolution` | `convolve` (MAS super) |
| `restrictDomain`, `domain` | `restrict_domain`, `domain` (MAS) |
| `DualSlopeTrick::atZero`, `addXMinusA(a, w)`, `slide` | `get_at_zero`, `add_linear_or_zero(w, a)`, `slide_min` (HIT) |

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md), P075 record): meldable-heap merge, domain, clamp and convolution operations on the unit heaps, `fromPoints`, `convolveSegment` (a three-call composition), `sumAll`, breakpoint iteration, `slopeAt`, `clear`, floating-point weighted variant, `DualSlopeTrick::evaluate`.
- `WeightedSlopeTrick` needs signed integral `T`. `SlopeTrick<double>` covers real positions.
- `DualSlopeTrick::slide` costs `b - a` heap insertions. Use `WeightedSlopeTrick` for wide windows.
- 08-Python `_30_dp_optimization.py` plans a SlopeTrick reduction (`addLeft`/`addRight`). Its owner should adopt these names (`addAMinusX`/`addXMinusA`).
- Mathematics `27` (planned) bridges to these structures; array min-plus convolution stays with Mathematics `53`.
- No legacy excerpt or `OLD` implementation exists for this row.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-13: implemented and verified (P075), `@reviewer` found 5 defects, fixed before the recorded runs.
