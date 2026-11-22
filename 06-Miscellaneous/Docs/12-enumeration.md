# 12-enumeration.hpp — Evidence

`12-enumeration.hpp` (batch MI15, package P015) provides visitor-based traversals of combinations, multicombinations, subsets, mixed-radix products and Gray products; submasks, supermasks, subset masks, fixed-popcount masks and Gray masks; integer partitions and set partitions; and lexicographic combination rank and unrank. It reuses the verified MI02 `BitOps` and the C01 template (P002), and is independent contest-profile code. The row's `missing:` operations are not implemented.

## Contracts

### Traversals (All `forEach*`)

Every traversal takes a visitor returning `true` to continue. A `false` result stops immediately, even at the final output, and the traversal returns `false`; `true` means normal completion. Vector arguments are borrowed immutable state valid only during the callback: copy to retain an output, and do not mutate a radix vector through another alias while traversing it. Exceptions propagate; no global state survives a call. Visitor work and retained copies are additional to the stated bounds. No traversal returns a total output count, which could overflow a fixed-width integer.

| API | Order, domain and boundary semantics |
|---|---|
| `forEachCombination(n,k,visit)` | Nonnegative `int` dimensions. Increasing index vectors in lexicographic order; `k>n` emits nothing, `k=0` one empty vector. Repeated input values stay distinct positions. |
| `forEachMulticombination(n,k,visit)` | Nondecreasing index vectors with replacement, lexicographic; `n=0<k` emits nothing, `k=0` one empty vector. Never forms an overflowing `n+k-1`. |
| `forEachSubset(n,visit)` | Nonnegative `int n`, no machine-word limit. Binary subset order with selected indices stored in **decreasing** order; the empty set comes first, and `n=0` emits it once. |
| `forEachProduct(radices,visit)` | Nonnegative radices, at most `INT_MAX` dimensions; last digit fastest. Any zero radix gives no output; no dimensions gives one empty tuple. Every radix is validated, even after a zero. |
| `forEachGrayProduct(radices,visit)` | Same domain, reflected mixed-radix order. `visit(digits, changed)` reports the changed digit, initially `-1`; consecutive outputs change exactly one digit by one. Not promised cyclic. |
| `forEachSubmask(mask,visit)` | Descending submasks including the mask and zero, by canonical `BitOps::prevSubmask`. |
| `forEachSupermask(mask,n,visit)` | Ascending supermasks of `mask` inside the low `n` bits, from `mask` to `2^n-1`, by canonical `BitOps::nextSupermask`; asserts `0<=n<=width` and `mask<2^n`. |
| `forEachSubsetMask<U>(n,visit)` | All low-`n` masks in numerical order. |
| `forEachCombinationMask<U>(n,k,visit)` | Fixed-popcount low-`n` masks in numerical (colex) order, by canonical `BitOps::nextCombination`, which differs from the lexicographic order of vector combinations. `k>n` emits nothing. |
| `forEachGrayMask<U>(n,visit)` | Binary reflected Gray masks; `visit(mask, changed_bit)` receives `(0,-1)` first. |
| `forEachIntegerPartition(n,[max_part,]visit)` | Partitions of `n>=0` into positive parts at most `max_part` (default `n`; `max_part>=1` when `n>0`, larger values clamped to `n`). Parts are nonincreasing; partitions come in reverse lexicographic order from the greedy `[max_part,...,max_part,rest]` to all ones. `n=0` emits one empty partition. |
| `forEachSetPartition(n,visit)` | Set partitions of `[0,n)` as restricted growth strings `a` (`a[0]=0`, `a[i]<=1+max(a[0..i))`; `a[i]` is the block of element `i`, blocks numbered by first element) in lexicographic order. `n=0` emits one empty string. The block count is `max(a)+1`. |

Mask APIs support the unsigned 8–128-bit words of `BitOps`, with `0<=n<=word width` and nonnegative `k`. They test the last value before stepping, so full-width enumeration needs no unrepresentable `2^width` counter or shift. Early stopping makes prefixes of enormous traversals practical.

### combinationRank, combinationUnrank

`combinationRank(n,a)` returns the zero-based position of the strictly increasing vector `a` (elements in `[0,n)`, `k=a.size()`) in `forEachCombination(n,k)` order; `combinationUnrank(n,k,rank)` is its inverse and returns a new vector. Domain: `0<=k<=n` with `C(n,k)<2^64`, so every rank fits `ulng`. Violations (overflowing `C(n,k)`, unsorted or out-of-range elements, `rank>=C(n,k)`) are assertions.

Correctness: the combinations below `a` that agree on `a[0..i)` and put some `v` in `(a[i-1], a[i])` at position `i` number `sum_v C(n-1-v, k-1-i) = C(n-1-a[i-1], k-i) - C(n-a[i], k-i)` by the hockey-stick identity; summing over `i` gives the rank. Unranking picks at each position the largest `q` whose count does not exceed the remaining rank, by binary search, since the count increases in `q`. Every binomial used, `C(t,b)` with `t<=n`, `b<=k`, `t-b<=n-k`, is at most `C(n,k)`, so none overflows inside the domain. `enumeration_detail::binom` multiplies `C(t-b+j-1,j-1)*(t-b+j)/j` exactly in `ulll` and stops once the value exceeds `2^64-1` (each step at least doubles it), so the entry check costs at most 64 steps. Cost: `O(k * min(k, n-k))` for rank and a `log(n)` factor more for unrank; inside the domain `min(k,n-k)<=33`.

### Correctness and Costs of the Traversals

Lexicographic combinations increment the rightmost movable index and restore the smallest increasing suffix. The subset vector is a binary counter: trailing low selected indices are removed and the first absent index appended. Product carries and Gray-direction reversals skip unit radices; for radices at least two, carries and reversals have geometrically decreasing frequencies, so overhead per output is amortized constant after linear setup. Gray masks toggle the trailing-zero bit of the new binary index, equivalent to `i ^ (i >> 1)`. Supermasks step `((sup | ~free) + 1) & free | mask`, a binary counter over the free bits.

Integer partitions use Zoghbi–Stojmenović ZS1. Let `h` be the last part above one. If it is two, it becomes one and a one is appended; otherwise it drops by one to `r`, and the freed amount (one plus the trailing ones) is rewritten as copies of `r` followed by the remainder. A cap `max_part` only changes the start, because the reverse lexicographic successors of the greedy partition are exactly the partitions with parts at most the cap. Each written part above one is later the `h` of a distinct step before it can be rewritten, ones are written at most one per step, and pops never exceed pushes plus `n`, so the cost is `O(n + out)` amortized with or without a cap.

Set partitions increment the rightmost position below its prefix maximum plus one and zero the suffix (Knuth's Algorithm H in lexicographic form), keeping the prefix maxima in a second array. A step changing position `i` costs `O(n-i)` and happens at most `B(i+1)` times; since `B(m-1)/B(m) = O(log(m)/m)`, the sum is `O(B(n)) = O(out)`, so `O(n + out)` amortized.

Costs: vector combinations `O(k)` state and worst-case `O(k+1)` delay; subsets `O(n)` state and amortized constant delay; products `O(d)` state and setup and amortized constant delay; masks constant state and delay; partitions `O(n)` state and amortized constant delay. No recursion or precomputed output list.

## Feature-to-Test Map

`12-enumeration_tester.cpp` uses independent recursive references, never the production steppers: combinations, subsets and products by recursion, reflected Gray lists by mirroring, mask lists by bit recursion, supermasks by subsets of the free bits, partitions by first-part recursion, set partitions by block assignment, and counts by Pascal's triangle, coin-change `p(n)` and the Bell triangle.

| Public feature | Coverage |
|---|---|
| `forEachCombination`, `forEachMulticombination` | Exhaustive small dimensions, positional order, `k>n`, empty identities, `INT_MAX` prefixes and large repeated-index output. |
| `forEachSubset` | Recursive binary-order sets, decreasing-index invariant, empty set, 100,000-dimensional prefixes. |
| `forEachProduct`, `forEachGrayProduct` | Exhaustive radix vectors of length 0–5 with radices 0–3; 1,500 seeded random products; zero and unit radices; 100,000-dimensional sparse products; changed-digit and adjacency checks. |
| `forEachSubmask`, `forEachSupermask`, `forEachSubsetMask`, `forEachCombinationMask`, `forEachGrayMask` | Unsigned 8/16/32/64/128-bit words, exhaustive small words, every mask for supermasks through `n=8`, random sparse submask and dense supermask fixtures at and below full width, boundary combinations, changed-bit checks. |
| `forEachIntegerPartition` | Every `n<=22` (full) under seven caps (1, 2, 3, `n/2+1`, `n`, `n+1`, `INT_MAX`) and the default overload against first-part recursion; counts against `p(n)`; a 1,000,000 prefix. |
| `forEachSetPartition` | Every `n<=10` (full) against block assignment with canonical labels; counts against the Bell triangle. |
| `combinationRank`, `combinationUnrank` | Every combination for `n<=13` (full) against its recursive position both ways; 50,000 random round trips with `n<=67` or `n` up to `INT_MAX`, with a direct Pascal-table sum `sum C(n-1-v, k-1-i)` as independent rank oracle for `n<=67`; closed forms for `k<=2`; consecutive ranks increasing; `C(67,33)`, `C(66,33)`, `C(64,32)`, `k=n-1` boundaries and the last rank `C-1`. |
| Visitor and lifetime behavior | Immutable vector type checks, cancellation at first, intermediate and final output, saved copies, repeated and nested calls, exceptions from 13 traversal calls. |
| Preconditions | 28 checked-build assertion probes, including a negative radix after a zero radix, a supermask bit above `n`, `max_part=0`, `C(68,34)` overflow, unsorted and out-of-range ranks and an oversized unrank rank. |

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), CPython 3.14; optimized, checked and ASan/UBSan builds by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 runs are library-wide (seed 42, beside the other suites, build cache emptied before each cold run); their figures are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS; quick preflight peak 976 MB; optimized 0.1 s compile + 1.7 s run, checked 30.0 s + 3.0 s, ASan-UBSan 43.1 s + 17.0 s; peak 1365 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 949 MB; optimized 0.3 s + 2.5 s, checked 64.0 s + 7.0 s, ASan-UBSan 73.5 s + 30.5 s; peak 1463 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 54.8 s + 0.3 s, checked 34.9 s + 2.2 s; peak 980 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 966 MB; optimized 0.1 s + 4.5 s, checked 15.5 s + 11.6 s, ASan-UBSan 22.6 s + 77.7 s; peak 843 MB |
| 2026-10-12 (P013 prune) | `python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 20260927`, also with `CXX=g++-14` | PASS on both compilers, 3 builds, 28 probes |
| 2026-10-08 | `python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode quick --seed 1` | PASS, 2 builds |
| 2026-10-08 | `python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode stress --seed 3` | PASS, 3 builds, 80,781,781 checks (before the review-added Pascal oracle) |
| 2026-10-08 | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS, 102 headers |
| 2026-10-08 | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

None: the direct successors have no dispatch, cutoff or speed claim; the bounds follow from the carry and successor arguments above.

## Sources

The implementation was written independently from these algorithm and proof references, without copying external source:

- [CP-algorithms combinations](https://cp-algorithms.com/combinatorics/generating_combinations.html) (2026-09-28): lexicographic successor; its fixed-weight Gray recursion is a separate order not implemented here.
- [CP-algorithms submasks](https://cp-algorithms.com/algebra/all-submasks.html) and [Gray code](https://cp-algorithms.com/algebra/gray-code.html) (2026-09-28): descending submask successor, reflected formula and adjacency.
- [Python itertools](https://docs.python.org/3/library/itertools.html) (2026-09-28): `combinations`/`product` positional identity and empty-product semantics.
- Jörg Arndt, [*Matters Computational*](https://www.jjj.de/fxt/fxtbook.pdf), chapter 9 pp.217–223 (2026-09-28): odometer carry analysis and constant-amortized reflected traversal. FXT's GPL code was not adapted.
- Torsten Mütze, [*Combinatorial Gray codes—an updated survey*](https://arxiv.org/html/2202.01280v4), §§2.5, 3.19, 4.1, 4.2 (2026-09-28): amortized versus worst-case delay, ranking and unranking, revolving-door and cool-lex orders.
- A. Zoghbi and I. Stojmenović, *Fast algorithms for generating integer partitions*, Int. J. Comput. Math. 70 (1998) 319–332: the ZS1 reverse lexicographic successor, cited from the standard statement; the paper was not read for this run, and the arguments above are self-contained.
- D. E. Knuth, TAOCP vol. 4A, read 2026-11-22 in pre-fascicles 3A and 3B ([ledger](00-sources.md#fetched-2026-11-22-source-check)). §7.2.1.5 Algorithm H (3B pp. 26–27) visits restricted growth strings in lexicographic order: it increases the rightmost `a_j` below `1 + max(a_1, …, a_{j-1})` and zeroes the suffix, which is what `forEachSetPartition` does. Knuth leaves the cost analysis to exercise 47, which was not read, so the amortized bound rests on the argument above. §7.2.1.3 Theorem L (3A p. 6, eqs. (19)–(20)) ranks a combination `c_t > … > c_1` as `C(c_t, t) + … + C(c_1, 1)` in the lexicographic order of these decreasing sequences. That is not the order of `combinationRank`. The two correspond by `c = n - 1 - a` with rank `C(n, k) - 1 - r`, so Theorem L is background, and the rank formula rests on the hockey-stick argument above.
- Catalog sweep 2026-10-08 ([00-sources.md](00-sources.md)): maspypy `enumerate/partition`, `bits`, `multiset`, `xor_range`; suisen `subset_iterator`; Nyaan `enumerate-set`; hitonanode `enumerate_partitions`.

## Limits and Handoffs

- Not adopted in the 2026-10-08 sweep: [00-notes.md](00-notes.md), Basic contracts (Enumeration).
- No enumeration implementation exists in the legacy index or `OLD`. Permutation ranking stays in MI02; constrained search keeps its own owner.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Search aliases (P263, grab --find): binary set operations (OI Wiki: 二进制集合操作), bitmask subsets, submask enumeration; 格雷码, 枚举, 组合枚举, 子集枚举, 超集枚举, 格雷码枚举, 整数划分枚举, 集合划分枚举, 可重组合枚举, 穷举.

## History

- 2026-09-28: nine traversals verified (P015, previous system).
- 2026-10-06: completeness sweep added partitions and rank/unrank to the row.
- 2026-10-08: re-audit added four missing operations and `forEachSupermask`, restyled the comment cap and closing braces, 4 findings fixed; full suites and g++-14 full passed.
- 2026-10-12: P013 prune migrated `forEachGrayMask` to `std::countr_zero`; full suite (seed 20260927, 28 probes) passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases, then their Chinese names (`grab --find`); no code or test change.
- 2026-11-10: P274 (SUP25) moved builds to the shared runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; every library-wide run passed.
