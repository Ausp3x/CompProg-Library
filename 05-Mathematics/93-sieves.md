# Ordinary sieves — MA02 verification

`04-sieve_algorithms.hpp` owns full-prefix Eratosthenes and linear sieves,
smallest/largest prime-factor tables, positive factor enumeration and the
four existing multiplicative-function tables. Segmented/block/wheel interval
sieving belongs to `06-segmentedsieve.hpp`; general multiplicative transforms
and additional functions belong to `08-multiplicative_functions.hpp`.

## API and domains

Both classes preserve the existing inclusive table convention `[0,n]`:
constructor arguments are maximum values, not table lengths. Their domain is
`0<=n<INT_MAX`, subject to available memory. This explicit upper bound keeps
`n+1` and loop sentinels representable as `int`; the former `n=INT_MAX` path
overflowed during allocation. All prime lists are ascending. The public
vectors are query data: callers must not modify them or `n` and then call
methods that rely on the sieve invariant.

| API | Result and cost |
|---|---|
| `SieveOfErath(n)` / `.reset(n)` | `is_prime[0..n]` and `prms`; odd composite marking starts at each prime square. `O(n*log(log(3+n)))` time. Zero and one are not prime. |
| `LinearSieve(n)` / `.reset(n)` | Builds `spf[0..n]` and `prms` in `O(n)` time. `spf[0]=spf[1]=-1` where those indices exist; `spf[p]=p` for primes. Optional tables start empty. |
| `.getLpf()` | Builds largest-prime-factor `lpf[0..n]`, with the same `-1` convention at zero/one, in `O(n)` time. |
| `.getNumDiv()` | Builds `num_div[a]=tau(a)`, the positive-divisor count, in `O(n)` time and `O(n)` transient integer storage. |
| `.getSumDiv()` | Builds `sum_div[a]=sigma(a)`, the positive-divisor sum, as signed 64-bit values, in `O(n)` time and `O(n)` transient signed-64 storage. |
| `.getPhi()` / `.getMu()` | Build Euler totient and Möbius tables in `O(n)` time and constant auxiliary space besides the result. `mu` uses signed bytes. |
| `.getPrimeFac(a) const` | For `1<=a<=n`, ascending `(prime,exponent)` pairs; empty for one. `O(log(a))` time and returned storage. |
| `.getAllFac(a) const` | For `1<=a<=n`, every positive divisor once in unspecified order; `{1}` for one. `O(log(a)+tau(a))` time, `O(tau(a))` returned storage. |

All four arithmetic tables define entry zero as a convenience sentinel zero,
not as a statement about divisors of zero; entry one is one. Their builders
may be called repeatedly in any order and recompute only their own table.
`reset(n)` clears every optional table and recomputes the mandatory state.
Copies own independent vectors; moved-from objects can be restored with
`reset`. Existing constructor conversion behavior and public names are
preserved. There is no global cache or dynamic-modulus state.

Ignoring vector capacity rounding, fresh Eratosthenes state uses `n+1` bytes
for flags plus `4*pi(n)` bytes for primes. Fresh linear state uses `4*(n+1)`
bytes for SPF plus the same prime list. LPF, divisor-count and totient tables
each add `4*(n+1)` bytes; divisor sums add `8*(n+1)` and Möbius adds `n+1`.
Thus building every table uses `25*(n+1)+4*pi(n)` logical bytes, plus at most
`8*(n+1)` temporary bytes during `getSumDiv`. Resetting to a smaller `n` can
retain capacity, so allocated storage is `O(peak_n)`, where `peak_n` is the
largest size used by the object. Construct a fresh object when releasing
retained capacity matters.

## Correctness and arithmetic bounds

Every composite has a prime factor at most its square root. Marking the odd
multiples from `p*p` after removing even composites therefore marks every
composite, while no prime is marked; smaller multiples already have a smaller
prime factor. Scanning the remaining flags yields the exact sorted list.

For the linear sieve, every composite `x` has the unique decomposition
`x=p*i` with `p=spf[x]<=spf[i]`. Iterating primes only through `spf[i]`
constructs that decomposition exactly once, sets the correct SPF and accounts
for linear time. Removing SPF repeatedly gives the unique prime factorization.
The largest prime factor obeys `lpf[i]=max(spf[i],lpf[i/spf[i]])`, with the
sentinel at one acting as the empty maximum. Divisor generation multiplies
each old divisor by each power of a new prime, so uniqueness follows from
unique factorization, and it generates exactly the Cartesian product of
allowed exponents.

Write `i=p*j` for `p=spf[i]`. If `p` divides `j`, only that prime's exponent
increases; otherwise a coprime prime factor is added. Divisor counts therefore
replace the factor `e+1` by `e+2` or multiply by two. Divisor sums replace
`1+p+...+p^e` by `1+p+...+p^(e+1)` or multiply by `1+p`. Totients multiply by
`p` in the first case and `p-1` in the second. Möbius is zero in the first case
and negates in the second. The one-valued base at index one proves each table
by induction. These recurrences use exact divisions.

All index products are checked in signed 64 bits before the `int` product is
used. Eratosthenes progression counters also use signed 64 bits to avoid the
last increment overflowing near `INT_MAX`. Divisors, divisor counts, totients,
prime powers and prime factors are at most `n`, so their signed-32 products
fit. A divisor sum is at most `n*(n+1)/2<2^61`; the geometric prime-power sum
is at most `2*n`, and its recurrence has no larger intermediate than its
result. Signed 64 bits are therefore sufficient for every sum and product in
`getSumDiv`. Möbius values remain in `{-1,0,1}`.

## Sources and legacy accounting

Inspected 2026-09-27. The implementation preserves and audits the existing
local algorithms; new code was independently written from the arguments,
without copying external source code.

- [cp-algorithms, Sieve of Eratosthenes](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html), page update 2026-09-18: marking from prime squares, odd-only work, byte/bit storage and segmented-memory tradeoffs. Timing claims there are external observations, not measurements of this repository.
- [cp-algorithms, Linear Sieve](https://cp-algorithms.com/algebra/prime-sieve-linear.html), page update 2023-11-25: unique `spf[x]*i` construction, linear work and factor-table memory. The page cites Pritchard's 1987 paper; this task inspected the page and proof, not the original paper.
- [OI Wiki, 筛法](https://oi-wiki.org/math/number-theory/sieve/), page update 2026-01-27: SPF-based totient, Möbius, divisor-count and divisor-sum recurrences, and the general multiplicative-function extension. Its Eratosthenes exposition credits the e-maxx/cp-algorithms lineage.
- [KACTL, `Eratosthenes.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/Eratosthenes.h), Håkan Terelius, source dated 2009-08-26, CC0: independent comparison of odd marking from prime squares and prime-list construction. Its global fixed-size bitset and exclusive limit differ from this header's existing inclusive per-object interface.

`OLD/5-Mathematics/04-sieves.hpp` was inspected and remains unchanged. Every
ordinary-sieve legacy operation is retained: Eratosthenes flags, linear SPF
and prime lists, factor/divisor getters, and all four multiplicative tables.
New work adds LPF, Eratosthenes prime lists, const queries and reset semantics;
it also removes endpoint-overflow risks. The archived segmented portion is
accounted for separately by the interval-sieve owner.

## Verification

`96-Local Testing/05-Mathematics/04-sieve_algorithms_tester.py` runs from any
working directory and reports modes, seed, configuration, operation/input,
expected/actual predicate values and subprocess diagnostics. Exact integer
oracles are independent of the production SPF recurrence.

| Feature | Oracle and edge classes |
|---|---|
| Eratosthenes/linear flags, SPF, LPF, sorted lists | Trial division for every integer through 50,000 and every prefix through 128; known prime count at one million; random larger trial factorizations. Zero, one, prime endpoints and prime squares included. |
| Prime powers and all divisors | Trial factorization and separate square-root divisor enumeration, sorted for set equality and duplicate detection; const calls, prime powers and squarefree products. |
| Divisor count and sum | Enumerate each divisor/multiple pair independently, plus explicit divisor lists. |
| Totient and Möbius | Independently invert `sum(phi(d) for divisors d of n)=n` and `sum(mu(d) for divisors d of n)=[n=1]`; totient also checked by direct gcd counting through 300, Möbius by trial-factor exponents. |
| State/lifetime | Randomized builder order, repeated calls, lazy tables, copy and copy assignment, move construction, reset of moved-from objects, grow/shrink resets and independent copy reset. |
| Invalid domains | Four constructor and four reset probes for negative/`INT_MAX` endpoints; negative, zero and `n+1` queries for both factor methods. |

The full command is:

```text
python3 '96-Local Testing/05-Mathematics/04-sieve_algorithms_tester.py' --mode full --seed 20260927
```

Quick/full/stress cover their documented domains in the test entry. Full
checks every prefix `0..128`, all integers through 50,000 and 1,500 seeded
random factor queries at one million, with prime-power and squarefree saved
cases. The fresh 2026-09-27 run passed 545,142 checks per configuration with
GNU C++20 and GCC 16.2.1 (20260810): `-O2 -DNDEBUG`,
`-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS` with
AddressSanitizer/UndefinedBehaviorSanitizer, no recovery, frame pointers and
non-PIE. The checked configuration passed all 14 precondition probes. The
sandbox's LeakSanitizer ptrace restriction required a final full rerun outside
the sandbox; that rerun passed completely. Its log is
`/tmp/p012-sieves-final.log`. Stress is available but was not claimed as run.

Performance comparisons use constructors, allocation and prime-list output;
linear sieve additionally computes SPF, so its output workload is larger.
Reproducible source and measurements are in
[the foundations benchmark](<../96-Local Testing/05-Mathematics/90-foundations_benchmark.py>)
and its companion `.cpp`/`.json`. Package-level standalone/aggregate/MTU and
consistency checks are recorded in [P012 evidence](96-p012.md). No online
submission was made.
