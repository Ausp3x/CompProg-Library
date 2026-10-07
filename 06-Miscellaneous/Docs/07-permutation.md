# 07-permutation.hpp — evidence

`07-permutation.hpp` provides lexicographic next/previous and k-th next steps, bijection algebra (inverse, composition, powers, cycles, sign, order), exact Lehmer digits, bounded scalar ranks, and exact multiset count/rank/unrank. It reuses `02-Data Structures/02-fenwick.hpp` for order statistics (P006 dependency). First verified 2026-09-28; re-audited 2026-10-07, when `kthNextPermutation`, `permutationCycles`, `permutationSign`, `permutationOrder` and `permutationOrderMod` were added. They had been inventory gaps (`/reaudit-review` finding 3), while the earlier record wrongly called the header complete. Header contracts moved here, and the `permutation_detail::Multiset` complexity line was added (finding 8).

## Contracts

### Domains

A mapping permutation is a `vector<int>` containing each integer in `[0,n)`
exactly once, with `n <= INT_MAX`. `p[i]` is the image of `i`. This convention
does not implicitly describe moving array values to/from indices; an application
must choose that interpretation explicitly. The empty permutation is valid.
`isPermutation` is a total validity predicate; the algebra and distinct-ranking
operations assert the documented bijection/domain preconditions. Stated auxiliary-space bounds exclude that check's
`n`-bit `vector<bool>`, which exists only while assertions are enabled (so
`kthNextPermutation` uses O(min(n,21)) words under `NDEBUG` and O(n) bits otherwise).

| API | Result and contract | Time / auxiliary space |
|---|---|---|
| `nextPermutation(a, cmp)`, `previousPermutation(a, cmp)` | Generic vector wrappers around the standard algorithms. Return true on a successor/predecessor; false wraps to the first/last order. Empty/singleton inputs return false. | O(n) comparisons/swaps / O(1) |
| `isPermutation(p)` | Whether `p` is a bijection on `[0,n)` and its size fits `int`. | O(n) / O(n) |
| `permutationInverse(p)` | `q[p[i]] == i`. | O(n) / O(n), including checked validation; returned storage O(n) |
| `permutationCompose(p,q)` | `r[i] = p[q[i]]`, so apply `q` first. Requires equal sizes. | O(n) / O(n), including checked validation; returned storage O(n) |
| `permutationPower(p,k)` | `p` composed with itself `k` times; negative powers use its inverse, zero gives identity. Every signed 64-bit exponent is supported, including `LNG_MIN`. | O(n) / O(n), plus O(n) returned storage |
| `kthNextPermutation(p,k)` | `p` a permutation of `[0,n)` (asserted), any `lng` offset including both extremes. Replaces `p` by the permutation of lexicographic rank `(rank(p) + k) mod n!`; returns true iff `0 <= rank(p) + k < n!` (no wrap). For `k = ±1` this matches `std::next_permutation`/`prev_permutation`, including the wrap. `n = 0`: true iff `k = 0`. | O(n + min(n,21)^2) / O(min(n,21)) |
| `permutationCycles(p)` | Disjoint cycles ordered by smallest element, each starting at its smallest element and following `p` (`c[j+1] = p[c[j]]`); fixed points are singleton cycles; empty permutation gives no cycles. | O(n) / O(n), plus O(n) returned |
| `permutationSign(p)` | `+1` or `-1`, `(-1)^(n - cycles)`, equal to `(-1)^(inversions)`. | O(n) / O(n) |
| `permutationOrder(p,out)` | Smallest `t >= 1` with `p^t = id`, the lcm of the cycle lengths. False iff it exceeds `ulng`, leaving `out` unchanged. Empty gives 1. For example, cycles of the first 16 primes (n = 381) overflow. | O(n) / O(n) |
| `permutationOrderMod<T>(p)` | The same lcm evaluated in `T` as a product of maximal prime powers `T(q^e)`. Exact for any `T` that represents it (`lll` for small `n`, a big integer), the residue for `mint`/`ModInt`. `T` needs `T(int)` and `*=`; wrapping types such as `ulng` give the order mod 2^64. | O(n) / O(n) |
| `permutationLehmer(p)` | `d[i]` equals the number of smaller values to the right of `p[i]`. Exact for arbitrary supported `n`. | O(n log(n+1)) / O(n), plus O(n) returned storage |
| `permutationFromLehmer(d)` | Decode digits satisfying `0 <= d[i] < n-i`; invalid digits violate a precondition. | O(n log(n+1)) / O(n), plus O(n) returned storage |
| `permutationRank(p)` | Zero-based lexicographic `ulng` rank, requiring `n <= 20`. Empty rank is zero. | O(n log(n+1)) / O(n) |
| `permutationUnrank(n,rank,out)` | Requires `0 <= n <= 20`. False exactly when `rank >= n!`, preserving `out`. Empty `n=0,rank=0` succeeds. | O(n log(n+1)) / O(n), plus O(n) returned storage |
| `multisetPermutationCount(count,out)` | Nonnegative multiplicities, sum `n <= INT_MAX`; returns exact `n! / product(count[i]!)` if it fits `ulng`, otherwise false with unchanged output. Empty/all-zero multiplicities count as one. | O(n+m), `m=count.size()` / O(1) |
| `multisetPermutationRank(values,out)` | Arbitrary signed `int` labels, including duplicates; zero-based rank among distinct value sequences. False when the total sequence count exceeds `ulng`, even if this individual rank would fit. | O(n log(m+1)) / O(m), `m` distinct labels |
| `multisetPermutationUnrank(values,rank,out)` | Uses the supplied multiset; its input order is irrelevant. False for count overflow or `rank >= total`, preserving `out`. Input and output may alias. | O(n log(m+1)) / O(m), plus O(n) returned storage |

The generic next/previous comparator must impose a strict weak order, and values
must be swappable. Duplicates are comparator-equivalent values, not necessarily
objects with equal payloads. Steps enumerate distinct comparison-class
sequences; they do not preserve the relative order of equivalent objects.
Ranking/algebra APIs use their documented integer orders rather than a custom
comparator. Compress other ordered labels first when using bijection APIs.

The scalar distinct rank bound is intentional: `20! = 2432902008176640000`
fits in `ulng`, whereas `21!` does not. Lehmer digits provide a scalable exact
rank representation without arbitrary-precision dependencies or a twenty-item
limit on algebra/encoding. Lexicographic digit order matches permutation order.
Multiset scalar ranks use the exact multinomial bound rather than a length
bound: a million equal values have only one ordering. `C(67,33)` fits in `ulng`,
while `C(68,34)` does not. Query overflow and out-of-range rank are documented
failure conditions; compute `multisetPermutationCount` first if the caller
needs to distinguish them.

No inputs are mutated except generic next/previous and successful output
parameters. Functions have no persistent cache or global state. Allocation
limits still apply to the theoretical `int` size domain. Arbitrary-precision
scalar multiset ranks are not provided. Partial permutations, constrained
generation and circular equivalence enumeration are separate enumeration
problems rather than implicit variants of these full-permutation APIs.

### permutation_detail

`cycleLengths(p)` returns the distinct cycle lengths in increasing order (O(n) time and space; asserts a permutation). `Multiset` groups a value list into sorted distinct `keys` and their `count`s with an ordered map, then stores the exact multinomial `total`, which is 0 when it exceeds `ulng`. It takes O(n log(m+1)) time and O(m) space for `m` distinct labels. Neither is public API.

## Correctness and optimality

Inverse and composition follow their pointwise definitions. Every bijection
partitions into disjoint cycles. On a cycle of length `m`, raising the mapping
to `k` advances by the normalized remainder `k % m`; taking a signed remainder
avoids negating the minimum signed exponent. Each vertex is visited a constant
number of times. All mapping operations are linear, matching the returned
output size lower bound.

`kthNextPermutation` works on the factorial number system. The Lehmer digits of the last `m = min(n,21)` positions depend only on the relative order of those suffix elements, and they are the low `m` mixed-radix digits (radices 1..m) of the rank. Adding or subtracting `|k|` digit by digit with carries in `{-1,0,1}` yields the new low digits, a carry `c` and a leftover quotient `a`. Since `21! > 2^63 > |k|`, `a = 0` and `|c| <= 1` whenever `n > 21`. If `c = 0` and `a = 0`, the prefix is unchanged. If `c = +1`, the prefix rank must increase by one: sorting the suffix descending gives the maximal suffix, and one `std::next_permutation` on the whole array then increments the prefix rank and leaves an ascending suffix. It returns false exactly when the prefix was the last arrangement, so the result wraps modulo `n!`. `c = -1` is symmetric with ascending sort and `prev_permutation`. When `n <= 21`, any nonzero `c` or `a` is a wrap, and the same whole-array step resets to the first or last order. Finally the suffix is rebuilt from the new digits by selecting the `d[i]`-th smallest remaining suffix value. All three cases leave rank `(rank + k) mod n!`. Cost: O(n) validation and stepping, O(m^2) digit work.

Cycles visit each vertex once with a `seen` array. Each cycle of length `L` contributes `L - 1` transpositions, so the sign is `(-1)^(n - cycles)`. The order of a product of disjoint cycles is the lcm of their lengths. The checked version multiplies `res / gcd(res, L) * L` in `ulll` and stops once the value exceeds `ulng`; lcm is monotone, so the first overflow is final. The modular version factors each distinct length by trial division. The distinct lengths sum to at most `n`, so there are at most `sqrt(2n)` of them and the total trial work is O(n^(3/4)). It keeps the maximal prime power per prime and multiplies those in `T`, which equals the lcm reduced in `T`.

A Lehmer digit counts available elements less than the selected value. Initially
the Fenwick tree contains one per label. Prefix counts produce the digit and
removing the selected label maintains this invariant. Decoding selects digit
`d[i]+1` by Fenwick order statistic and removes that element. Thus the operations
are inverse bijections between permutations and valid mixed-radix digit
vectors. Scalar rank evaluates these digits in bases `n,n-1,...,1`; scalar
unrank divides in reverse order. A nonzero leftover quotient is exactly an
out-of-range rank, including the empty case. Every scalar intermediate fits
under the `n <= 20` contract. Fenwick setup is linear and each selection/update
is logarithmic; there is no quadratic erase-from-vector bottleneck.

For multiplicities `c`, the number of arrangements is
`W = n! / product(c[i]!)`. Build this as successive binomial factors. During
each factor's recurrence, the current exact product multiplied by its next
numerator and divided by its next denominator remains an integer. Intermediate
products are below `2^95` because the previously accepted count is at most
`2^64-1` and the numerator at most `INT_MAX`; unsigned 128-bit arithmetic is
sufficient. The count grows monotonically, so exceeding `ulng` can be rejected
immediately. Zero multiplicities and a single all-equal group require no inner
recurrence work. The implementation validates every count before any early
overflow return.

With `r` positions left, all arrangements beginning with a label of remaining
multiplicity `c` form a contiguous block of exactly `W*c/r` arrangements. These
block sizes are integers. Ranking adds the combined blocks for smaller labels,
then restricts to the chosen block. For unranking rank `k`, select the first
cumulative multiplicity `s` for which `W*s/r > k`; its order-statistic target is
`floor(k*r/W)+1`. Subtract the earlier blocks and recurse. All products again
fit unsigned 128 bits; accepted ranks and counts stay within `ulng`. Fenwick
queries count remaining multiplicities without scanning every smaller label.
An ordered frequency map groups labels directly from the const input in
O(n log(m+1)) time and O(m) space for `m` distinct values. The subsequent queries
have the same time bound and O(m) auxiliary space, including linear all-equal
and one-rare-value cases. Input values are neither copied nor sorted during
grouping; the O(n) returned sequence is constructed only for successful unrank.

The implementation uses standard lexicographic steps and the shared Fenwick
engine, without empirically tuned thresholds or a claimed constant-factor
speedup. Large tests exercise the linear and logarithmic paths; their runtime
is not presented as a portable benchmark.

## Re-audit findings (P013, 2026-10-07)

| Header | Gap or finding | Resolution |
|---|---|---|
| `07` | `kthNextPermutation`, `permutationCycles`, `permutationSign`, `permutationOrder` missing; evidence claimed completeness (finding 3) | Implemented, plus `permutationOrderMod`; enumeration, property, repeated-step and Python factorial-base/`lcm` oracles |
| `07` | Finding 8: `permutation_detail::Multiset` lacks a complexity line | Added `// T: O(n * log(m + 1)), M: O(m), m distinct labels; ...` |
| all | Contracts in multi-line header comments over the 8% cap | Moved to `## Contracts` sections; headers pass the cap |
| all | `; }` closing braces in headers, testers, benchmark (finding 7) | Normalized; `03-consistency.py --braces` reports none |

The completeness sweep added `permutationOrderMod`. The independent `@reviewer` pass (2026-10-08) found no correctness defects: an independent `lll` rank/unrank probe agreed with `kthNextPermutation` on 200,000 cases (n up to 33, extreme and random offsets, |k| >= n!), and exhaustive or random probes confirmed the order and sign behaviour. It confirmed that `kthNextPermutation`'s memory bound ignored the checked-build validation vector; the bound is now stated in the contracts above.

## Feature-to-test map

`96-Local Testing/06-Miscellaneous/07-permutation_tester.py` runs from any working
directory and uses non-removable checks in every configuration.

| Feature | Independent evidence |
|---|---|
| Lex order, wrappers, wrap and duplicates | Recursive choice-by-label enumeration, independent of the standard permutation algorithms; strings, descending comparator, equal comparison keys with unequal records, empty/singleton/all-equal cases. |
| Inverse/composition and validation | Pointwise inverse and composition oracles, identity, invalid labels/duplicates, equal-size contract. |
| Powers | Independent binary exponentiation of mappings, all small exponents, random full-width exponents, signed min/max, empty/fixed-point/multicycle and long single-cycle inputs. |
| Lehmer and scalar ranks | Brute inversion counts, every permutation through n=8 in full, independent recursive enumeration ranks, Python exact factorial arithmetic through n=20, rank n! and maximum `ulng` rejection, empty rank0. |
| Multiset count/rank/unrank | Independent recursive enumeration for all three-symbol counts 0..3 in full; Python factorial-based counts and candidate-block enumeration; negative/extreme integer labels, zero counts, alias-safe output, preserved output on failure. |
| Width and long-duplicate boundaries | Exact `C(67,33)` and overflowing `C(68,34)`, overflowing `21!`, repeated-value n>20 fitting totals, `INT_MAX` multiplicity count fixtures, 100,000-element full inputs with all-equal and one-rare values. |
| `kthNextPermutation` | For every permutation through n=8 (full), offsets 0, ±1, `-rank`, `-rank-1`, `n!-rank-1`, `n!-rank`, `-2n!-1`, `3n!+2`, `LNG_MIN`, `LNG_MAX` against the recursively enumerated list at `(rank+k) mod n!` with the wrap flag from `lll` arithmetic; random n<70 with offsets in [-1000,1000] against repeated `std::next/prev_permutation` (result and any-wrap flag), random full-width offsets inverted by `-k`; n=25 wrap through the prefix in both directions with extreme offsets; Python exact factorial-base oracle on n in {0,1,2,5,20,21,22,40,<60} with random and extreme offsets (300 full / 3,000 stress cases) |
| `permutationCycles`, `permutationSign` | Property oracle on every enumerated and random permutation: cycles partition `[0,n)`, follow `p`, start at their minimum, ordered by start; sign equals brute inversion parity; empty permutation; Python cycle walk for sign and cycle count |
| `permutationOrder`, `permutationOrderMod` | Smallest `t` with `p^t = id` by repeated composition for every permutation through n=8; `permutationPower(p, order) = id` and `OrderMod<ulng>` agreement on random n<70; product of the first 15 primes fits exactly, first 16 primes overflow with `out` preserved, and `OrderMod<mint>` matches that product times 53; `OrderMod<lll>`/`<mint>` agreement on enumerated permutations; Python `math.lcm` (exact and mod 998244353) on random conjugated cycle structures with up to 19 distinct lengths (n up to several hundred), on both sides of 2^64 |
| Preconditions | 20 checked-build assertion probes: non-permutation inputs to kthNext, cycles, sign, order and orderMod, plus malformed inverse/compose/power/Lehmer/rank inputs, invalid digit bounds, scalar rank size, negative/oversized unrank size, negative and excessive multiplicity sums. |

Quick exhausts distinct permutations through n=6, three-symbol multiplicities 0..2, 100 random algebra cases and size-1000 long inputs. Full raises these to n=8, counts 0..3, 3000 cases and size 100,000, with 1,202 Python exact-arithmetic queries. Stress uses n=9, counts 0..4, 30,000 cases, size 500,000 and 9,102 Python queries. Failures include seed, operation, input, expected and actual values.

## Commands and results

GCC 16.2.1, GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H, 2026-10-07:

```sh
python3 '96-Local Testing/06-Miscellaneous/07-permutation_tester.py' --mode full --seed 20260927                              # PASS, 3 configurations, 20 probes
python3 '96-Local Testing/06-Miscellaneous/07-permutation_tester.py' --mode stress --seed 20260928 --configuration optimized   # PASS
```

Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan with leak detection. No benchmark is required; no threshold or speedup is claimed. No online submission was made.

Batch MI02, package P013; dependencies P002 (template) and P006 (Fenwick), both verified.

P013 package runs, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H, 2026-10-07. Baseline before the re-audit changes: all seven P013 suites (`01`–`07`) passed full mode with seed 20260927. After the changes:

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

`02-integration.py` also builds every header alone and the Basic/All aggregates.

## Sources

Inspected 2026-09-28 (Asia/Manila). Algorithms were independently implemented;
next/previous directly call the standard library and Fenwick is a shared local
dependency. No external implementation was copied.

- [cppreference, `std::next_permutation`](https://en.cppreference.com/w/cpp/algorithm/next_permutation.html):
  lexicographic successor/wrap behavior, comparator requirements, duplicate
  example and linear worst-case work. The independent reviewer also inspected
  installed libstdc++16 `stl_algo.h` and the saved Competitive Programmer's
  Handbook §5.2, pp.49–50, for standard enumeration semantics.
- [Lehmer code](https://en.wikipedia.org/wiki/Lehmer_code), sections “The code”
  and “Encoding and decoding”: inversion digits, radix bounds, exact
  lexicographic rank interpretation and selection among remaining labels.
  The Fenwick implementation follows these invariants rather than the article's
  quadratic illustrative encoding procedure.
- [NIST DLMF §26.4(i), equation 26.4.2](https://dlmf.nist.gov/26.4#E2),
  version 1.2.8 released 2026-09-15: multinomial factorial formula and its product
  of binomial coefficients, including empty and one-group conventions. The
  cited Comtet/Abramowitz–Stegun texts were not separately reviewed.
- Completeness sweep 2026-10-07 (`@researcher`): [maspypy kth_next_permutation](https://raw.githubusercontent.com/maspypy/library/main/seq/kth_next_permutation.hpp) and [factorial_digit_system](https://raw.githubusercontent.com/maspypy/library/main/seq/factorial_digit_system.hpp) (suffix factorial digits with carries; maspypy covers distinct values only and gives a short count on overflow, while this header wraps modulo `n!` like `std::next_permutation`), [suisen permutation](https://suisen-cp.github.io/cp-library-cpp/library/util/permutation.hpp), [maspypy cycle_decomposition](https://raw.githubusercontent.com/maspypy/library/main/seq/cycle_decomposition.hpp) (dynamic; Data Structures scope), [KACTL IntPerm.h](https://github.com/kth-competitive-programming/kactl/blob/main/content/combinatorial/IntPerm.h). The checked plus modular order contract follows the researcher's recommendation; no fetched source states one.
- [Permutation](https://en.wikipedia.org/wiki/Permutation#Permutations_of_multisets),
  “Permutations of multisets” and cycle/order sections: repeated-value semantics,
  multinomial count and cycle decomposition. Partial/circular variants were
  identified as separate enumeration scopes, not silently claimed here.

The independent reviewer checked saved KACTL `IntPerm.h` and found its integer
numbering explicitly not order-preserving, with narrower integer/bitmask bounds;
it was not adopted as a lexicographic rank algorithm or correctness oracle.

## Limits and handoffs

Omitted candidates: `permute`/`inversePermute` on value arrays (one loop; `permutationCompose` covers integer labels), `argsort`/index sort (`11-sorting_selection.hpp` and `04-compression.hpp` `stableRanks`), `permutationFromCycles` and `cycleType` (direct one-pass transforms of `permutationCycles`), linear non-lexicographic ranking (KACTL `IntPerm`, suisen `PermutationHash`; the lexicographic rank covers the need), dynamic cycle decomposition under swaps (Data Structures), k-th roots of permutations and permutation groups (Mathematics `64`), inversion count (`08-sequence_algorithms.hpp`).

- Scalar distinct ranks need n ≤ 20; Lehmer digits and `kthNextPermutation` scale to any n; multiset ranks need the multinomial to fit `ulng`; `permutationOrder` reports overflow, `permutationOrderMod` reduces into the caller's type.
- No online submission was made and no judge acceptance is claimed.
