# MA02 combinatorics: contracts and verification

`05-combinatorics.hpp` implements the Basic combinatorics scope of P012. The
implementation uses ordinary contest arithmetic and the existing full Core modular
integer types. It adds no Mathematics-specific reduction backend or ISA path.

## Features and domains

`PrimeCombinatorics<M>` accepts a full Core modular integer type with a prime
modulus and its prime flag enabled. `ModFac` remains an alias for
`PrimeCombinatorics<mint>`: construction, `n`, `fac`, `inv_fac`, and all four legacy
query names remain available. Construction/reset requires
`0 <= n < min(M::mod(), INT_MAX)`.

| API | Meaning and bounds |
|---|---|
| `reset(n)` | Build factorial, inverse-factorial, and derangement tables through `n`; `O(n + log(p))` time. |
| `factorial(k)`, `inverseFactorial(k)`, `derangement(k)` | `O(1)` queries with `0 <= k <= n`; `D_0=1`, `D_1=0`. |
| `combiNR(a,b)`, `permuNR(a,b)` | Choose/order `b` distinct objects from `a >= 0`; negative or excessive selections return zero, otherwise require `a <= n`. |
| `combiWR(a,b)` | Choose an unordered multiset of size `b`; `b=0` gives one even for `a=0`; positive selections from zero choices give zero. Nontrivial queries require `a+b-1 <= n`. |
| `permuWR(a,b)` | Ordered selections, `a^b`; `a >= 0`, negative `b` returns zero, `0^0=1`. Independent of table size; `O(log(2+b))` time. |
| `starsBars(total,parts,positive)` | Ordered nonnegative parts by default, positive parts when requested. Zero parts have one empty solution exactly for total zero. Uses the appropriate combination-table bound. |
| `catalan(k)` | Dyck paths/standard Catalan number, `k >= 0`, `2*k <= n`. |
| `ballot(a,b,strict)` | A/B sequences with A never behind B, or strictly ahead at every nonempty prefix. Both conventions count the empty sequence once. Weak queries require `a+b <= n` when a solution is possible; strict queries remove their first A and require `a+b-1 <= n`. |
| `multinomial(parts)` | Distinguishable arrangements of the supplied nonnegative multiplicities, sum at most `n`; empty list gives one. `O(parts.size())` time. |

Three tables use approximately `3 * (n+1) * sizeof(M)` bytes. Vectors retain
capacity after a smaller reset: stored memory is `O(peak n)`. Queries, fields, and
Core modulus contexts are read-only except through their documented APIs.

Every dynamic `setMod` invalidates all prior table values, including a call that
sets the same modulus again. Rebuild with `reset` before further queries or raw
field reads. Query methods assert that the current modulus matches their saved
modulus and still has the prime flag. Core deliberately has no generation tag;
same-modulus resets and changes away and back cannot be detected. This is a
caller precondition, not a promised runtime diagnostic. Copy/move preserve the
live table and its modulus requirement; a moved-from table may be discarded or
reset before reuse.

`fibonacciPair<T>(n)`, `fibonacci<T>(n)`, and `lucas<T>(n)` use iterative fast doubling
in `O(log(2+n))` ring operations and `O(1)` storage, with the full unsigned 64-bit
index domain. They work in composite modular rings and the zero ring as well as
prime fields. `L_0=2`, `L_1=1`. Unsigned builtins of width at least 32 compute in
their natural wraparound ring; narrower builtin types are rejected because C++
integer promotion can introduce signed overflow. Other builtin integer types
require every intermediate to fit; named exact APIs provide checked unsigned
64-bit results.

All exact APIs accept nonnegative, full-width unsigned 64-bit counts. They
return `true` and write the mathematical unsigned 64-bit result when it fits;
`false` means actual output overflow and preserves the output argument. Zero and
impossible counts are successful results. Inputs may alias the output because
scalar inputs are copied before evaluation.

| Exact API | Coverage and cost |
|---|---|
| `combiExact(a,b,out)` | Binomial with impossible selection equal to zero; at most 34 product/division steps before completion or overflow. |
| `permuExact(a,b,out)`, `factorialExact(n,out)` | Falling product/factorial with checked multiplication; at most 21 steps, factorial fits through 20. |
| `combiRepExact(a,b,out)`, `permuRepExact(a,b,out)` | Unordered/ordered selections with replacement; the same empty-selection semantics as the modular APIs; checked index sum or binary exponentiation. |
| `starsBarsExact(total,parts,out,positive)` | Nonnegative/positive parts, including zero parts. |
| `ballotExact(a,b,out,strict)`, `catalanExact(n,out)` | Full-width input domain and actual-result overflow detection, including fitting answers whose corresponding binomial exceeds 64 bits. At most 37 ballot recurrence steps, each with gcd cancellation. Catalan fits through 36. |
| `multinomialExact(parts,out)` | Full-width multiplicities; `O(k * 34)` arithmetic steps for `k` parts until overflow. |
| `derangementExact(n,out)` | Exact through 20; larger indices report overflow. |
| `fibonacciPairExact(n,out)` | Both consecutive values fit through index 92; larger indices report overflow. |
| `fibonacciExact(n,out)`, `lucasExact(n,out)` | Exact Fibonacci through 93 and Lucas through 92; larger indices report overflow. |

Negative combinatorial population/part counts, invalid table bounds, a composite
factorial modulus, and stale tables are precondition errors. Negative selection
counts in the four preserved modular APIs are legitimate zero results. Exact
APIs express nonnegative counts with unsigned parameters.

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
it does not pretend that a zero factorial has an inverse. Lucas-theorem,
prime-power, and composite-modulus binomial algorithms belong to the separately
planned Advanced `12-combinatorics_advanced.hpp`, batch **MA26**. General
rising/falling factorial families, Stirling/Bell/partition numbers, bounded
partitions, q-binomial coefficients, and combinatorial transforms remain with
that owner. Fibonacci's companion **Lucas sequence** here is distinct from
**Lucas's binomial theorem**. No advanced package is claimed complete here.

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

## Verification evidence

Runner: `96-Local Testing/05-Mathematics/05-combinatorics_tester.py`, directly
runnable from any working directory, using the shared mathematics driver.

| Feature | Independent coverage |
|---|---|
| Factorials/inverses/derangements | Python exact factorials and inverse; Pascal table; derangement inclusion-exclusion plus permutation enumeration through 8. |
| Four counting operations | Exact Python combinations/permutations/powers; empty/impossible selections; zero choices; unsigned index-sum overflow; every binomial threshold around rows 60–80. |
| Stars-and-bars/ballot/Catalan | Explicit small composition/path enumeration; independent exact closed forms; empty conventions; strict/weak branches; Catalan 36/37 boundary and overflowing-binomial regression. |
| Multinomial | Independent Python factorial ratios, empty/zero groups, full-width groups, sum/product overflow. |
| Fibonacci/Lucas | Independent modular matrix powers, arbitrary 64-bit indices, static/dynamic and prime/composite/one moduli, unsigned32/64 rings, exact 92/93/94 boundaries; safe signed32 regression and narrow-type compile rejection. |
| Type/state/preconditions | Static/dynamic 32/64-bit Core types, modulus 2 and a prime near `2^64`, copy/move/reset, same-modulus rebuild, changed-modulus assertions, preserved aliased outputs, 27 assertion probes. |

Quick mode uses 300 seeded random iterations plus all fixed small/exhaustive and
boundary cases: 28,447 Python protocol cases per configuration. Full uses 5,000
random iterations: 70,747 protocol cases per configuration and ASan/UBSan. Stress
uses 40,000 random iterations; it is an explicitly larger workload and is not
claimed run by this record. A compile-fail fixture checks rejection of narrow
unsigned Fibonacci types in every invocation, separate from assertion probes.

Final command (2026-09-27):

```text
python3 '96-Local Testing/05-Mathematics/05-combinatorics_tester.py' --mode full --seed 20260927
```

**PASS** for all three configurations: GNU++20 `-O2 -DNDEBUG`,
`-O0 -g -D_GLIBCXX_DEBUG`, and
`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined
-fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`.
Each ran 70,747 independent Python protocol cases plus the C++ fixtures;
the checked build passed all 27 invalid-input assertion probes. Narrow-type
compilation rejection passed. The final run used GCC 16.2.1 (20260810),
CPython 3.14.7, Linux x86-64, Intel Core i9-11900H, and seed 20260927.
Log: `/tmp/p012-combinatorics-final.log` (ephemeral local artifact).

The initial sandboxed full run passed optimized/checked but LeakSanitizer could
not initialize under ptrace. A permitted run outside the sandbox completed all
configurations with leak detection enabled; no sanitizer option was disabled.
Package-wide standalone-header, aggregate, multi-translation-unit and existing
Core consumer checks are recorded by the integration owner in `96-p012.md`.

No additional combinatorics performance claim is made. The standard factorial
table and doubling algorithms have their stated preprocessing/query bounds;
the third table's cost is explicit. Performance measurements in the package
record concern sieve alternatives, not these counting algorithms.
