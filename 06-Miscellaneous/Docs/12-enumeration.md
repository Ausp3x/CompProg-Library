# 12-enumeration.hpp — evidence

`12-enumeration.hpp` (batch MI15, package P015) provides visitor-based traversals of combinations, multicombinations, subsets, mixed-radix products and Gray products; submasks, supermasks, subset masks, fixed-popcount masks and Gray masks; integer partitions and set partitions; and lexicographic combination rank/unrank. It reuses verified MI02 `BitOps` and the C01 template (P002). Implementations are independent contest-profile code. No online submission was made.

## Contracts

### Traversals (all `forEach*`)

Every traversal takes a visitor returning `true` to continue. A `false` result stops immediately, including at the final output, and makes the traversal return `false`; `true` means normal completion. Vector arguments are borrowed immutable state valid only during the callback: copy to retain an output, and do not mutate a radix vector through another alias while traversing it. Exceptions propagate; no global state survives a call. Visitor work and retained copies are additional to the stated bounds. No traversal returns a total output count, which could overflow a fixed-width integer.

| API | Order, domain and boundary semantics |
|---|---|
| `forEachCombination(n,k,visit)` | Nonnegative `int` dimensions. Increasing index vectors in lexicographic order; `k>n` emits nothing, `k=0` emits one empty vector. Repeated input values remain distinct positions. |
| `forEachMulticombination(n,k,visit)` | Nondecreasing lexicographic index vectors with replacement; `n=0<k` emits nothing, and `k=0` emits one empty vector. Avoids forming an overflowing `n+k-1`. |
| `forEachSubset(n,visit)` | Nonnegative `int n`, no machine-word limit. Binary subset order with selected indices stored in **decreasing** order. The empty set comes first; `n=0` emits it once. |
| `forEachProduct(radices,visit)` | Nonnegative radices, at most `INT_MAX` dimensions; last digit fastest. Any zero radix gives no output; no dimensions gives one empty tuple. Every radix is validated, even after a zero. |
| `forEachGrayProduct(radices,visit)` | Same domain, reflected mixed-radix order. `visit(digits, changed)` reports the changed digit, initially `-1`; consecutive outputs change exactly one digit by one. The traversal is not promised cyclic. |
| `forEachSubmask(mask,visit)` | Descending submasks including the mask and zero, using canonical `BitOps::prevSubmask`. |
| `forEachSupermask(mask,n,visit)` | Ascending supermasks of `mask` inside the low `n` bits, from `mask` to `2^n-1`, using canonical `BitOps::nextSupermask`; asserts `0<=n<=width` and `mask<2^n`. |
| `forEachSubsetMask<U>(n,visit)` | All low-`n` masks in numerical order. |
| `forEachCombinationMask<U>(n,k,visit)` | Fixed-popcount low-`n` masks in numerical (colex) order, using canonical `BitOps::nextCombination`; this differs from vector-combination lexicographic order. `k>n` emits nothing. |
| `forEachGrayMask<U>(n,visit)` | Binary reflected Gray masks; `visit(mask, changed_bit)` receives `(0,-1)` first. |
| `forEachIntegerPartition(n,[max_part,]visit)` | Partitions of `n>=0` into positive parts at most `max_part` (default `n`; `max_part>=1` when `n>0`, any larger value is clamped to `n`). Parts are nonincreasing, partitions appear in reverse lexicographic order starting with the greedy `[max_part,...,max_part,rest]` and ending with all ones. `n=0` emits one empty partition. |
| `forEachSetPartition(n,visit)` | Set partitions of `[0,n)` as restricted growth strings `a` (`a[0]=0`, `a[i]<=1+max(a[0..i))`; `a[i]` is the block of element `i`, blocks numbered by first element) in lexicographic order. `n=0` emits one empty string. The block count is `max(a)+1`. |

Mask APIs support the unsigned 8–128-bit word domains of `BitOps`, with `0<=n<=word width` and nonnegative `k`. They test the last value before stepping, so full-width enumeration needs no unrepresentable `2^width` counter or shift. Early stopping makes prefixes of otherwise enormous traversals practical.

### combinationRank, combinationUnrank

`combinationRank(n,a)` returns the zero-based position of the strictly increasing vector `a` (elements in `[0,n)`, `k=a.size()`) in `forEachCombination(n,k)` order. `combinationUnrank(n,k,rank)` is its inverse. The domain is `0<=k<=n` with `C(n,k)<2^64`, so every rank fits `ulng`; violations (overflowing `C(n,k)`, unsorted or out-of-range elements, `rank>=C(n,k)`) are assertions. `combinationUnrank` returns a new vector.

Correctness: the combinations below `a` that agree on `a[0..i)` and put some `v` in `(a[i-1], a[i])` at position `i` number `sum_v C(n-1-v, k-1-i) = C(n-1-a[i-1], k-i) - C(n-a[i], k-i)` by the hockey-stick identity; summing over `i` gives the rank. Unranking picks at each position the largest `q` whose such count does not exceed the remaining rank, found by binary search because the count is increasing in `q`. Every binomial used, `C(t,b)` with `t<=n`, `b<=k`, `t-b<=n-k`, is at most `C(n,k)`, so none overflows inside the domain. `enumeration_detail::binom` multiplies `C(t-b+j-1,j-1)*(t-b+j)/j` exactly in `ulll` and stops once the value exceeds `2^64-1` (each step at least doubles it), so the entry check costs at most 64 steps. Cost: `O(k * min(k, n-k))` for rank, a `log(n)` factor more for unrank; inside the domain `min(k,n-k)<=33`.

### Correctness and costs of the traversals

Lexicographic combinations increment the rightmost movable index and restore the smallest increasing suffix. The subset vector acts as a binary counter: trailing low selected indices are removed and the first absent index is appended. Product carries and Gray-direction reversals skip unit radices; for radices at least two, carries/reversals have geometrically decreasing frequencies, giving amortized constant overhead per output after linear setup. Gray masks toggle the trailing-zero bit of the new binary index, equivalent to `i ^ (i >> 1)`. Supermasks step `((sup | ~free) + 1) & free | mask`, a binary counter over the free bits.

Integer partitions use Zoghbi–Stojmenović ZS1: let `h` be the last part above one. If it is two, it becomes one and a one is appended; otherwise it drops by one to `r`, and the freed amount (one plus the trailing ones) is rewritten as copies of `r` followed by the remainder. A cap `max_part` only changes the starting point, because the reverse lexicographic successors of the greedy partition are exactly the partitions with parts at most the cap. Each written part above one is later the `h` of a distinct step before it can be rewritten, ones are written at most one per step, and pops never exceed pushes plus `n`, so the cost is `O(n + out)` amortized with or without a cap.

Set partitions increment the rightmost position that is below its prefix maximum plus one and zero the suffix (Knuth's Algorithm H in lexicographic form), with the prefix maxima kept in a second array. A step that changes position `i` costs `O(n-i)` and happens at most `B(i+1)` times; because `B(m-1)/B(m) = O(log(m)/m)`, the sum is `O(B(n)) = O(out)`, giving `O(n + out)` amortized.

Costs: vector combinations use `O(k)` state and worst-case `O(k+1)` delay; subsets `O(n)` state and amortized constant delay; products `O(d)` state/setup and amortized constant delay; masks constant state and delay; partitions `O(n)` state and amortized constant delay. No recursion or precomputed output list is used.

## Feature-to-test map

`12-enumeration_tester.cpp` uses independent recursive references: combinations, subsets and products by recursion, reflected Gray lists by mirroring, mask lists by bit recursion, supermasks by subsets of the free bits, partitions by first-part recursion, set partitions by block assignment, and counts by Pascal's triangle, coin-change `p(n)` and the Bell triangle. Production steppers are never oracles. Non-removable checks run in optimized NDEBUG, checked and sanitizer builds.

| Public feature | Coverage |
|---|---|
| Combinations and multicombinations | Exhaustive small dimensions, positional order, `k>n`, empty identities, `INT_MAX` prefixes and large repeated-index output. |
| Arbitrary-dimensional subsets | Recursive binary-order sets, decreasing-index invariant, empty set, 100,000-dimensional prefixes. |
| Product and Gray product | Exhaustive radix vectors of length 0–5 with radices 0–3; 1,500 seeded random products; zero/unit radices; 100,000-dimensional sparse products; changed-digit and adjacency checks. |
| Five mask traversals | Unsigned 8/16/32/64/128-bit words, exhaustive small words, every mask for supermasks through `n=8`, random sparse submask and dense supermask fixtures at and below full width, boundary combinations, changed-bit checks. |
| Integer partitions | Every `n<=22` (full) under seven caps (1, 2, 3, `n/2+1`, `n`, `n+1`, `INT_MAX`) and the default overload against first-part recursion; counts against `p(n)`; a 1,000,000 prefix. |
| Set partitions | Every `n<=10` (full) against block assignment with canonical labels, counts against the Bell triangle. |
| Rank and unrank | Every combination for `n<=13` (full) against its recursive position both ways; 50,000 random round trips (with a direct Pascal-table sum `sum C(n-1-v, k-1-i)` as an independent rank oracle for `n<=67`) with `n<=67` or `n` up to `INT_MAX`, closed forms for `k<=2`, consecutive ranks increasing; `C(67,33)`, `C(66,33)`, `C(64,32)`, `k=n-1` boundaries and the last rank `C-1`. |
| Visitor/lifetime behavior | Immutable vector type checks, cancellation at first/intermediate/final output, saved copies, repeated and nested calls, exceptions from 13 traversal calls. |
| Preconditions | 28 checked-build assertion probes, including a negative radix after a zero radix, a supermask bit above `n`, `max_part=0`, `C(68,34)` overflow, unsorted and out-of-range ranks and an oversized unrank rank. |

## Commands and results

Run 2026-10-08: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined` with leak checking.

```bash
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode quick --seed 1                 # PASS, 2 configurations
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 1                  # PASS, 3 configurations, 28 probes, 11,145,684 checks
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode full --seed 2       # PASS, 3 configurations
python3 '96-Local Testing/06-Miscellaneous/12-enumeration_tester.py' --mode stress --seed 3                # PASS, 3 configurations, 80,781,781 checks (before the review-added Pascal oracle)
python3 '96-Local Testing/02-integration.py' --sanitizers                                                   # PASS, 102 headers (one package run)
python3 '96-Local Testing/03-consistency.py'                                                                # no errors
```

## Benchmarks

None: the direct successors have no dispatch, cutoff or speed claim; bounds follow from the carry/successor arguments above.

## Sources

Algorithm/proof references; the implementation was written independently without copying external source:

- [CP-algorithms combinations](https://cp-algorithms.com/combinatorics/generating_combinations.html) (2026-09-28): lexicographic successor; its fixed-weight Gray recursion is a separate order not implemented here.
- [CP-algorithms submasks](https://cp-algorithms.com/algebra/all-submasks.html) and [Gray code](https://cp-algorithms.com/algebra/gray-code.html) (2026-09-28): descending submask successor, reflected formula and adjacency.
- [Python itertools](https://docs.python.org/3/library/itertools.html) (2026-09-28): `combinations`/`product` positional identity and empty-product semantics.
- Jörg Arndt, [*Matters Computational*](https://www.jjj.de/fxt/fxtbook.pdf), chapter 9 pp.217–223 (2026-09-28): odometer carry analysis and constant-amortized reflected traversal. FXT's GPL code was not adapted.
- Torsten Mütze, [*Combinatorial Gray codes—an updated survey*](https://arxiv.org/html/2202.01280v4), §§2.5, 3.19, 4.1, 4.2 (2026-09-28): amortized versus worst-case delay, ranking/unranking, revolving-door and cool-lex orders.
- A. Zoghbi and I. Stojmenović, *Fast algorithms for generating integer partitions*, Int. J. Comput. Math. 70 (1998) 319–332: the ZS1 reverse lexicographic successor. Cited from the standard statement; the paper was not read for this run, and the correctness and amortized arguments above are self-contained.
- D. E. Knuth, TAOCP Vol. 4A §7.2.1.5 Algorithm H (restricted growth strings) and §7.2.1.3 (combinatorial number system): cited from memory of the standard statements, not re-read for this run; the correctness and cost arguments above are self-contained.
- Catalog sweep 2026-10-08 (see [00-sources.md](00-sources.md)): maspypy `enumerate/partition`, `bits`, `multiset`, `xor_range`; suisen `subset_iterator`; Nyaan `enumerate-set`; hitonanode `enumerate_partitions`.

## Limits and handoffs

- Not adopted, with reasons recorded in [00-notes.md](00-notes.md): set-bit iteration (the idiom `x &= x - 1` with `BitOps::trailingZeros`), submasks of fixed popcount (reduce to `forEachCombinationMask` over the mask's bits), revolving-door/cool-lex combination orders, weak/strict compositions (bijection with `forEachMulticombination` by bar counts), xor-range block decomposition (a BitOps-family operation, outside this package), labeled/unlabeled tree enumeration (Graphs Prüfer codes plus `forEachProduct`), all (mask, submask) pairs (nested `forEachSubmask`; `32-subset_dp` owns the engine), and a part-count cap for integer partitions.
- No enumeration implementation exists in the legacy index or `OLD`. Permutation ranking stays in MI02; constrained search keeps its own owner.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-09-28: nine traversals verified (P015, previous system); the 2026-10-06 sweep added partitions and rank/unrank to the row. 2026-10-08 re-audit: four missing operations and `forEachSupermask` implemented, comment cap and closing braces restyled, `/reaudit-review` findings 1–4 fixed. Tester and benchmark sources brace-normalized after review (whitespace only); full suites, g++-14 full and benchmarks rerun, PASS.
