# 07-permutation.hpp — evidence

`07-permutation.hpp` provides lexicographic next/previous and k-th next steps, bijection algebra (inverse, composition, powers, cycles, sign, order), exact Lehmer digits, bounded scalar ranks, and exact multiset count/rank/unrank. It reuses `02-Data Structures/02-fenwick.hpp` for order statistics (P006 dependency). Batch MI02, package P013; dependencies P002 (template) and P006 (Fenwick), both verified.

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

Correctness:

- Algebra: inverse and composition follow their pointwise definitions. Every bijection partitions into disjoint cycles; on a cycle of length `m`, power `k` advances by `k % m` (a signed remainder, so the minimum exponent is never negated). Each vertex is visited a constant number of times, so mapping operations are linear, matching the output lower bound.
- `kthNextPermutation`: the Lehmer digits of the last `m = min(n,21)` positions depend only on that suffix's relative order and are the low mixed-radix digits (radices 1..m) of the rank. Adding `k` digit by digit with carries in `{-1,0,1}` gives new low digits, a carry `c` and a leftover quotient `a`; since `21! > 2^63 > |k|`, `a = 0` and `|c| <= 1` when `n > 21`. For `c = +1`, sorting the suffix descending and calling `std::next_permutation` on the whole array increments the prefix rank and leaves an ascending suffix, returning false exactly on wrap; `c = -1` is symmetric. For `n <= 21` any nonzero `c` or `a` is a wrap and the same step resets to the first or last order. The suffix is then rebuilt by selecting the `d[i]`-th smallest remaining value, giving rank `(rank + k) mod n!`.
- Cycles, sign, order: a `seen` array visits each vertex once; a cycle of length `L` contributes `L - 1` transpositions; the order of disjoint cycles is the lcm of their lengths. The checked version computes `res / gcd(res, L) * L` in `ulll` and stops at the first value above `ulng` (lcm is monotone). The modular version factors each distinct length by trial division (at most `sqrt(2n)` distinct lengths summing to `n`, so O(n^(3/4)) trial work), keeps the maximal prime power per prime and multiplies those in `T`.
- Lehmer and scalar ranks: a Fenwick tree holding one per available label gives each digit as a prefix count; decoding selects the `(d[i]+1)`-th remaining label. These are inverse bijections between permutations and valid mixed-radix digit vectors, whose order matches lexicographic order. Scalar rank evaluates the digits in bases `n,...,1` and unrank divides in reverse; a nonzero leftover quotient is exactly an out-of-range rank. Every intermediate fits under `n <= 20`; no quadratic erase-from-vector step occurs.
- Multiset count: `W = n! / product(c[i]!)` is built as successive binomial factors whose running product stays integral; intermediates are below `2^95` (count at most `2^64-1`, numerator at most `INT_MAX`), so `ulll` suffices, and monotone growth allows immediate rejection above `ulng`. All counts are validated before any early overflow return.
- Multiset rank/unrank: with `r` positions left, arrangements beginning with a label of remaining multiplicity `c` form a contiguous block of exactly `W*c/r`. Ranking adds the blocks of smaller labels; unranking `k` selects the first cumulative multiplicity `s` with `W*s/r > k` (order-statistic target `floor(k*r/W)+1`) and recurses. Products fit `ulll`. Fenwick queries count remaining multiplicities, and an ordered frequency map groups labels from the const input in O(n log(m+1)) without copying or sorting it.

The implementation uses standard lexicographic steps and the shared Fenwick engine, with no tuned threshold or claimed speedup, so no benchmark is required.

### permutation_detail

`cycleLengths(p)` returns the distinct cycle lengths in increasing order (O(n) time and space; asserts a permutation). `Multiset` groups a value list into sorted distinct `keys` and their `count`s with an ordered map, then stores the exact multinomial `total`, which is 0 when it exceeds `ulng`. It takes O(n log(m+1)) time and O(m) space for `m` distinct labels. Neither is public API.

## Feature-to-test map

[`07-permutation_tester.py`](<../../96-Local Testing/06-Miscellaneous/07-permutation_tester.py>) runs from any working
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

P013 package run, 2026-10-07, GCC 16.2.1, GNU++20, CPython 3.14.7, Linux x86-64, Intel Core i9-11900H. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, ASan/UBSan `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all` with leak detection. The permutation full run passed all 3 configurations and 20 assertion probes.

```sh
python3 '96-Local Testing/01-run.py' --mode quick --filter 06-Miscellaneous --no-integration                            # PASS (all 14 suites)
for s in 01-random 02-customhash 03-fastio 04-compression 05-binarysearch 06-bit_operations 07-permutation; do
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode full --seed 20260927                                # PASS x7, 3 configurations each
  python3 "96-Local Testing/06-Miscellaneous/${s}_tester.py" --mode stress --seed 20260928 --configuration optimized   # PASS x7
done
python3 '96-Local Testing/06-Miscellaneous/03-fastio_benchmark.py'                                                       # PASS, record rewritten
python3 '96-Local Testing/02-integration.py'                                                                             # PASS (every header alone, Basic/All aggregates)
python3 '96-Local Testing/03-consistency.py'                                                                             # no errors
```

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
- Saved KACTL `IntPerm.h` (checked by the independent reviewer): its integer numbering is explicitly not order-preserving, with narrower bounds; not adopted as a rank algorithm or oracle.

## Limits and handoffs

Omitted candidates: `permute`/`inversePermute` on value arrays (one loop; `permutationCompose` covers integer labels), `argsort`/index sort (`11-sorting_selection.hpp` and `04-compression.hpp` `stableRanks`), `permutationFromCycles` and `cycleType` (direct one-pass transforms of `permutationCycles`), linear non-lexicographic ranking (KACTL `IntPerm`, suisen `PermutationHash`; the lexicographic rank covers the need), dynamic cycle decomposition under swaps (Data Structures), k-th roots of permutations and permutation groups (Mathematics `64`), inversion count (`08-sequence_algorithms.hpp`).

- Scalar distinct ranks need n ≤ 20; Lehmer digits and `kthNextPermutation` scale to any n; multiset ranks need the multinomial to fit `ulng`; `permutationOrder` reports overflow, `permutationOrderMod` reduces into the caller's type.
- No online submission was made and no judge acceptance is claimed.

## History

- 2026-09-28: P013 first verification, full and stress suites passed.
- 2026-10-07: P013 re-audit, finding 3 (missing `kthNextPermutation`, cycles, sign, order) and finding 8 (`Multiset` complexity line) fixed, `permutationOrderMod` added, contracts moved out of code comments; full, stress and integration passed; `@reviewer` (2026-10-08) found no defects, and its `kthNextPermutation` memory-bound note is now in the contracts.
