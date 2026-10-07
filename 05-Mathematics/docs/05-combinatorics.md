# MA02 combinatorics: contracts and verification

`05-combinatorics.hpp` implements the Basic combinatorics scope of P012. The
implementation uses ordinary contest arithmetic and the existing full Core modular
integer types. It adds no Mathematics-specific reduction backend or ISA path.

## Contracts

### PrimeCombinatorics and ModFac

`PrimeCombinatorics<M>` accepts a full Core modular integer type whose modulus is
prime and whose prime flag is set. `ModFac` remains an alias for
`PrimeCombinatorics<mint>`; construction, `n`, `fac`, `inv_fac` and all four legacy
query names are still available. Construction and `reset(n)` require
`0 <= n < min(M::mod(), INT_MAX)`, so every factorial in the table is a unit.
Factorial queries require an index `<= n`. Negative selection counts give 0.
Counts of choices, parts and objects must be nonnegative (asserted).

| API | Meaning and bounds |
|---|---|
| `reset(n)` | Builds factorial, inverse-factorial and derangement tables through `n` in `O(n + log(p))` time. |
| `check()` | Asserts that the current modulus equals the saved one and still has the prime flag. Every query calls it. |
| `factorial(k)`, `inverseFactorial(k)`, `derangement(k)` | `O(1)` queries for `0 <= k <= n`; `D_0 = 1`, `D_1 = 0`. |
| `combiNR(a,b)`, `permuNR(a,b)` | Choose or order `b` distinct objects from `a >= 0`. Negative or excessive selections return 0; otherwise they require `a <= n`. |
| `combiWR(a,b)` | Unordered multisets of size `b`. `b = 0` gives 1 even when `a = 0`, and positive selections from zero choices give 0. Nontrivial queries require `a + b - 1 <= n`. |
| `permuWR(a,b)` | Ordered selections with replacement, `a^b`, for `a >= 0`. Negative `b` returns 0 and `0^0 = 1`. It does not depend on the table size and costs `O(log(2 + b))`. |
| `starsBars(total,parts,positive)` | Ordered parts summing to `total`: nonnegative parts by default, positive parts when `positive` is set. Zero parts give one empty solution exactly when `total = 0`. Uses the matching combination-table bound. |
| `catalan(k)` | Catalan number for `k >= 0` and `2 * k <= n`. |
| `ballot(a,b,strict)` | A/B sequences where every prefix has A >= B, or A > B for every nonempty prefix when `strict`. Both conventions count the empty sequence once. Strict mode removes the forced first A. Weak queries require `a + b <= n` when a solution is possible; strict queries require `a + b - 1 <= n`. |
| `multinomial(parts)` | Arrangements of the given nonnegative multiplicities, with sum at most `n`. The empty list gives 1. Costs `O(parts.size())`. |
| `combiLarge(a,b)` | `C(a, b) mod p` for any `ulng a` and `b <= n` (asserted). `b < 0` or `b > a` gives 0. Costs `O(b)` and uses only `inv_fac[b]`. |

The three tables use about `3 * (n + 1) * sizeof(M)` bytes. `fac` and `inv_fac`
keep their capacity after a smaller reset, so stored memory is `O(peak n)`; `der` is
rebuilt by `derangementTable` (re-audit review: removes the duplicated recurrence) and
holds exactly `n + 1` entries. All fields are
read-only for callers; change them only through the documented APIs.

Every dynamic `setMod` invalidates all earlier table values, even one that sets the
same modulus again. Call `reset` before any further query or direct field read.
`check()` catches a changed modulus by assertion. Core has no generation tag, so it
cannot detect a same-modulus reset or a change away and back. Rebuilding is the
caller's precondition, not a promised runtime check. Copy and move keep the live
table and its modulus requirement. A moved-from table may be discarded or reset
before reuse.

### fibonacciPair, fibonacci, lucas

`fibonacciPair<T>(n)` returns `(F_n, F_(n+1))`; `fibonacci<T>(n)` and `lucas<T>(n)`
return `F_n` and `L_n`, with `L_0 = 2` and `L_1 = 1`. They use iterative fast
doubling: `O(log(2 + n))` ring operations, `O(1)` storage, and the full unsigned
64-bit index range. `T` only has to be a commutative ring (no division), so
composite modular rings and the zero ring work as well as prime fields. Unsigned
builtins of width at least 32 compute in their natural wraparound ring. Narrower
builtin types are rejected by `static_assert`, because integer promotion could
cause signed overflow. Signed builtin types require every intermediate to fit. Use
the exact APIs for checked unsigned 64-bit results.

### binomialTable and derangementTable

`binomialTable<T>(n)` returns the Pascal rows `C(i, 0..i)` for `0 <= i <= n`.
`derangementTable<T>(n)` returns `D_0..D_n` by
`D_i = (i - 1) * (D_(i-1) + D_(i-2))`. Both assert `n >= 0` and use only addition
and multiplication, so they work in any ring `T`, including composite moduli
(`DynModInt64` with the prime flag off), modulus 1 and wrapping `ulng`. Signed
builtin `T` needs every intermediate to fit. Costs are `O(n^2)` time and memory for
the binomial table and `O(n)` for derangements. They were added because Library
Checker `montmort_number_mod` allows any modulus up to `10^9`, while
`PrimeCombinatorics` needs a prime.

### Exact APIs

Every exact API accepts all `ulng` inputs. It returns `true` and writes the exact
count when the count fits in `ulng`. `false` means the count exceeds `UINT64_MAX`,
and `out` is left unchanged. Impossible selections are successful zero results.
Empty selections and arrangements count once, including `0^0`. Inputs may alias
`out`, because scalar inputs are copied before evaluation. Every function finishes
within a constant number of loop steps (at most 64) before it completes or detects
overflow.

| Exact API | Coverage and cost |
|---|---|
| `combiExact(a,b,out)` | Binomial, 0 for an impossible selection. At most 34 product/division steps before completion or overflow. |
| `permuExact(a,b,out)`, `factorialExact(n,out)` | Falling product and factorial with checked multiplication. At most 21 steps; factorial fits through 20. |
| `combiRepExact(a,b,out)`, `permuRepExact(a,b,out)` | Unordered and ordered selections with replacement, with the same empty-selection rules as the modular APIs. Uses a checked index sum or binary exponentiation (at most 64 steps). |
| `starsBarsExact(total,parts,out,positive)` | Nonnegative or positive parts, including zero parts. |
| `ballotExact(a,b,out,strict)`, `catalanExact(n,out)` | Same prefix and empty conventions as `PrimeCombinatorics::ballot`. The direct ballot recurrence avoids overflowing a binomial when the final ballot count still fits. At most 37 recurrence steps, each with gcd cancellation, costing `O(min(b, 37) * log(2 + a + b))`. Catalan fits through 36. |
| `combinatorics_detail::ratioProduct` | Positive `r * a * b / (c * d)` with an integral result. Cancels each denominator against the numerators before multiplying; false above `UINT64_MAX`. `O(log(max(r, a, b, c, d)))`. Finding 10 of the re-audit: it now has its complexity line. |
| `multinomialExact(parts,out)` | Full-width multiplicities, `O(k)` combination calls (each at most 34 steps) for `k` parts; the empty list gives 1. |
| `derangementExact(n,out)` | Exact through `D_20`; larger indices report overflow. |
| `fibonacciPairExact(n,out)` | Both values fit through index 92; larger indices report overflow. |
| `fibonacciExact(n,out)`, `lucasExact(n,out)` | Exact Fibonacci through 93 and Lucas through 92; larger indices report overflow. |

Negative population or part counts, invalid table bounds, a composite factorial
modulus and stale tables are precondition errors. Negative selection counts in the
four preserved modular APIs are legitimate zero results. Exact APIs take counts as
unsigned parameters, so they cannot be negative.

## Correctness and overflow arguments

Factorials below a prime modulus are units. Inverting `n!` once and walking
backward gives every inverse factorial; factorial ratios then count ordered and
unordered selections. Pascal's identity and elementary enumeration independently
check those identities. Stars-and-bars places separators between or around
objects. The reflection argument gives weak ballot counts
`C(a+b,b) - C(a+b,b-1)`; removing the forced first A gives strict counts. Catalan
is the case `a=b`. Derangements use
`D_n = (n-1) * (D_(n-1) + D_(n-2))`, independently tested by inclusion-exclusion
and direct permutation enumeration.

Exact binomial recurrence carries a fitting unsigned 64-bit partial value and
multiplies it by a factor at most `UINT64_MAX`; its unsigned 128-bit intermediate
therefore cannot overflow. After each exact division, exceeding the result limit
is conclusive because subsequent coefficients increase. Symmetry ensures that
the smallest overflowing central case occurs by step 34. Exact falling products
are nondecreasing through their used factors and overflow by the 21st step when
that many nontrivial factors are requested. Checked exponentiation avoids an
unused final square, so a fitting power is never rejected due to unused work.

With-replacement counts can require a mathematical index `a+b-1` above the
unsigned 64-bit limit. In this case both complementary binomial selections are
nonzero and the binomial is at least that index, so reporting overflow is exact.
Similarly, if the running multinomial total overflows, it combines at least two
positive groups and its count is at least the new total. No fitting answer is
rejected merely because an intermediate index is large.

For exact ballot counts fix `d=a-b >= 0` and build
`B_i = (d+1)/(d+i+1) * C(d+2*i,i)`. Then
`B_i / B_(i-1) = (d+2*i)*(d+2*i-1)/(i*(d+i+1))`.
The implementation cancels both denominator factors against the previous value
and the two numerator factors before multiplying. All cancellation operands fit
unsigned 128 bits even for full-width inputs. Integrality guarantees complete
cancellation; the remaining products are tested against the result limit before
multiplication. `B_i` is nondecreasing in `i` and is at least Catalan `C_i`, so an
overflow cannot later disappear and at most 37 steps are needed. This includes
Catalan 36, whose intermediate central binomial exceeds 64 bits but final count
`11959798385860453492` fits. Strict ballot uses the equivalent weak instance
`(a-1,b)` after handling empty/impossible cases.

`combiLarge(a, b)` computes `inv_fac[b] * a(a-1)...(a-b+1)` with each factor
reduced mod `p`. The falling product divided by `b!` is the exact integer
`C(a, b)`. Since `b <= n < p`, `b!` is a unit mod `p`, so reducing that identity
mod `p` is exact for every `a`, including `a >= p`. This agrees with Lucas's
theorem, which gives `C(a mod p, b)` for `b < p`. Pascal's rule
`C(i, j) = C(i-1, j-1) + C(i-1, j)` and the derangement recurrence are polynomial
identities over the integers, so they hold in every quotient ring.

Fast doubling maintains `(F_k,F_(k+1))` and applies
`F_(2k)=F_k*(2*F_(k+1)-F_k)` and
`F_(2k+1)=F_k^2+F_(k+1)^2`. Lucas follows from
`L_k=2*F_(k+1)-F_k`. These are polynomial identities and need no inverses.
Exact wrappers use unsigned 128-bit intermediates within their explicitly
checked fitting-index range; independent matrix exponentiation verifies the
full-width modular index domain.

## Migration and ownership limits

Inspected and preserved unchanged:

- `OLD/5-Mathematics/12-combinatorics.hpp` and the `ModFac` block in
  `OLD/[1] algorithms.cpp`.
- `97-Legacy/02-modfac.cpp`, the verbatim disabled block from that monolith.

The legacy factorial, inverse-factorial, and four counting operations are all
accounted for. `ModFac` and its exposed `n`, `fac`, and `inv_fac` remain available.
The old `combiWR(0,0)=0` behavior is corrected to one empty multiset. Construction
now explicitly rejects invalid prime-factorial domains; wide intermediate index
checks avoid signed overflow before indexing. Legacy method-based modular
inverse/power syntax remains only in its preserved archive.

Factorial inversion at `n >= p` is invalid. This header rejects such table sizes;
it does not pretend that a zero factorial has an inverse. Lucas-theorem (small
prime, large index), prime-power and CRT composite-modulus binomial algorithms belong to the separately
planned Advanced `12-combinatorics_advanced.hpp`, batch **MA26**. General
rising/falling factorial families, Stirling/Bell/partition numbers, bounded
partitions, q-binomial coefficients, and combinatorial transforms remain with
that owner. Fibonacci's companion **Lucas sequence** here is distinct from
**Lucas's binomial theorem**. No advanced package is claimed complete here.

The 2026-10-07 completeness sweep proposed five operations. `combiLarge`,
`binomialTable` and `derangementTable` were adopted. `combiNegative` and
`combiInverse` were not: `C(-n, k) = (-1)^k * combiWR(n, k)` and
`1 / C(a, b) = inv_fac[a] * fac[b] * fac[a-b]` are one-line identities over the
existing API and tables. The reasons are also recorded in [80-notes.md](notes.md).

## References actually inspected

All pages below were retrieved and their cited sections read on 2026-09-27.
Code is independently implemented from the mathematical identities, with no
external code copied.

- [cp-algorithms: Binomial coefficients](https://cp-algorithms.com/combinatorics/binomial-coefficients.html),
  factorial/inverse-factorial method for a prime `p > n`, Pascal recurrence,
  and separation of prime-power/arbitrary-modulus/large-index methods.
- [OI Wiki: 排列组合](https://oi-wiki.org/math/combinatorics/combination/),
  selection with repetition, multinomial counting, and Pascal/binomial identities;
  independent Chinese-language comparison of counting conventions.
- 2026-10-07 sweep, for the added operations:
  [Nyaan binomial.hpp](https://nyaannyaan.github.io/library/modulo/binomial.hpp)
  (large-index `C` with a small lower index),
  [Nyaan binomial-table.hpp](https://nyaannyaan.github.io/library/modulo/binomial-table.hpp)
  (Pascal table without inverses),
  [ei1333 montmort.hpp](https://ei1333.github.io/library/math/combinatorics/montmort.hpp)
  (derangement table for any modulus), and the cp-algorithms binomial page above
  (Pascal triangle for arbitrary moduli). These were read for the operation lists
  only; the code was written independently.
- [cp-algorithms: Stars and bars](https://cp-algorithms.com/combinatorics/stars_and_bars.html),
  nonnegative and positive ordered parts and their shifted binomial indices.
- [OI Wiki: 卡特兰数](https://oi-wiki.org/math/combinatorics/catalan/),
  path interpretation, recurrence, and reflection/closed forms; displayed page
  update 2026-09-06.
- [Bertrand's ballot theorem](https://en.wikipedia.org/wiki/Bertrand%27s_ballot_theorem),
  strict-leading formula and the ties-allowed variant; mathematical identities
  compared with explicit short path enumeration.
- [OI Wiki: 错位排列](https://oi-wiki.org/math/combinatorics/derangement/),
  two-term derangement recurrence and inclusion-exclusion.
- [cp-algorithms: Fibonacci numbers](https://cp-algorithms.com/algebra/fibonacci-numbers.html),
  matrix representation and the two fast-doubling identities. Lucas's companion
  identity is verified from its initial values and recurrence.

## Feature-to-test map

Runner: [`05-combinatorics_tester.py`](<../../96-Local Testing/05-Mathematics/05-combinatorics_tester.py>) driving
[`05-combinatorics_tester.cpp`](<../../96-Local Testing/05-Mathematics/05-combinatorics_tester.cpp>). Oracles: exact
Python integers, enumeration, inclusion-exclusion and matrix powers.

| Feature | Independent coverage |
|---|---|
| Factorials, inverses, derangements | Python exact factorials and inverses; Pascal table; derangements by inclusion-exclusion plus permutation enumeration through 8. |
| Four counting operations | Exact Python combinations, permutations and powers; empty and impossible selections; zero choices; unsigned index-sum overflow; every binomial threshold around rows 60–80. |
| Stars-and-bars, ballot, Catalan | Explicit small composition and path enumeration; independent exact closed forms; empty conventions; strict and weak branches; the Catalan 36/37 boundary and the overflowing-binomial regression. |
| Multinomial | Python factorial ratios, empty and zero groups, full-width groups, sum and product overflow. |
| `combiLarge` | Op `ml` against `math.comb(a, b) % p` for `a` in {0, 1, 2, n, p-1, p, p+1, 2p+3, 2^63, 2^64-1, random words} and `b` in {-1, 0, 1, 2, n/2, n, random}, restricted to `b <= n`, for every prime setup (2, 3, 7, 97, 998244353 and a prime near `2^64`). Probe `large-bound`. |
| `derangementTable`, `binomialTable` | Ops `rd` (n = 30) and `rb` (n = 12) over `DynModInt64` ring moduli 1, 2, 8, 1000, `2^63` and `2^64-1`, and op `ud` (wrapping `ulng`, n = 40), against Python exact values reduced mod the ring. Probes `table-negative` and `derangement-table-negative`. |
| Fibonacci, Lucas | Modular matrix powers, arbitrary 64-bit indices, static and dynamic prime, composite and modulus-one rings, unsigned 32/64-bit rings, exact 92/93/94 boundaries; a safe signed 32-bit regression and narrow-type compile rejection. |
| Types, state, preconditions | Static and dynamic 32/64-bit Core types, modulus 2 and a prime near `2^64`, copy/move/reset, same-modulus rebuild, changed-modulus assertions, preserved aliased outputs, and 30 assertion probes. |

A compile-fail fixture checks, on every run, that narrow unsigned Fibonacci types
are rejected. This is separate from the assertion probes.

## Verification

Commands, configurations and results for the P012 re-audit are recorded in
[96-p012.md](p012.md). The standard factorial-table and doubling algorithms have
their stated bounds; no combinatorics performance claim is made.
