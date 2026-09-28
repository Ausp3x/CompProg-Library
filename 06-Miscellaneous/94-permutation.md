# Permutations — MI02 / P013

`07-permutation.hpp` provides lexicographic next/previous steps, bijection
algebra, exact Lehmer digits, bounded scalar ranks, and exact multiset
count/rank/unrank. It directly reuses `02-Data Structures/02-fenwick.hpp` for
order statistics; DS01 / P006 is therefore an actual dependency of MI02 in
addition to the Core template prerequisite.

## Domains and APIs

A mapping permutation is a `vector<int>` containing each integer in `[0,n)`
exactly once, with `n <= INT_MAX`. `p[i]` is the image of `i`. This convention
does not implicitly describe moving array values to/from indices; an application
must choose that interpretation explicitly. The empty permutation is valid.
`isPermutation` is a total validity predicate; the algebra and distinct-ranking
operations assert the documented bijection/domain preconditions.

| API | Result and contract | Time / auxiliary space |
|---|---|---|
| `nextPermutation(a, cmp)`, `previousPermutation(a, cmp)` | Generic vector wrappers around the standard algorithms. Return true on a successor/predecessor; false wraps to the first/last order. Empty/singleton inputs return false. | O(n) comparisons/swaps / O(1) |
| `isPermutation(p)` | Whether `p` is a bijection on `[0,n)` and its size fits `int`. | O(n) / O(n) |
| `permutationInverse(p)` | `q[p[i]] == i`. | O(n) / O(n), including checked validation; returned storage O(n) |
| `permutationCompose(p,q)` | `r[i] = p[q[i]]`, so apply `q` first. Requires equal sizes. | O(n) / O(n), including checked validation; returned storage O(n) |
| `permutationPower(p,k)` | `p` composed with itself `k` times; negative powers use its inverse, zero gives identity. Every signed 64-bit exponent is supported, including `LNG_MIN`. | O(n) / O(n), plus O(n) returned storage |
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

## Correctness and optimality

Inverse and composition follow their pointwise definitions. Every bijection
partitions into disjoint cycles. On a cycle of length `m`, raising the mapping
to `k` advances by the normalized remainder `k % m`; taking a signed remainder
avoids negating the minimum signed exponent. Each vertex is visited a constant
number of times. All mapping operations are linear, matching the returned
output size lower bound.

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

## Sources inspected

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
- [Permutation](https://en.wikipedia.org/wiki/Permutation#Permutations_of_multisets),
  “Permutations of multisets” and cycle/order sections: repeated-value semantics,
  multinomial count and cycle decomposition. Partial/circular variants were
  identified as separate enumeration scopes, not silently claimed here.

The independent reviewer checked saved KACTL `IntPerm.h` and found its integer
numbering explicitly not order-preserving, with narrower integer/bitmask bounds;
it was not adopted as a lexicographic rank algorithm or correctness oracle.

## Feature-to-test map and verification

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
| Preconditions | 15 checked-build assertion probes: malformed inverse/compose/power/Lehmer/rank inputs, invalid digit bounds, scalar rank size, negative/oversized unrank size, negative and excessive multiplicity sums. |

Quick exhausts distinct permutations through n=6, three-symbol multiplicities
0..2, 100 random algebra cases and size-1000 long inputs. Full raises these to
n=8, counts0..3, 3000 cases and size100000, with 602 Python exact-arithmetic
queries. Stress uses n=9, counts0..4, 30000 algebra cases, size500000 and 3102
Python queries. The exhaustive and random seeds are printed, and failures
include configuration, operation/input, expected/actual values and command.

On GCC 16.2.1 / GNU++20, full seed `20260927` passes optimized `-O2 -DNDEBUG`,
checked `-O0 -g -D_GLIBCXX_DEBUG`, and ASan/UBSan. Sanitizers run outside the
sandbox because its ptrace environment prevents LeakSanitizer from running.
Quick optimized/checked and stress optimized seed `20260928` also passed before
the final ordered-frequency grouping change; full verification was rerun after
that change, including 100,000-element duplicate/rare regressions. Package
integration records the standalone-header, aggregates and multiple-translation
unit checks separately.

```sh
python3 '96-Local Testing/06-Miscellaneous/07-permutation_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/06-Miscellaneous/07-permutation_tester.py' --mode stress --seed 20260928 --configuration optimized
```

All owned MI02 permutation features are complete within these explicit domains.
No online judge submissions were made.
