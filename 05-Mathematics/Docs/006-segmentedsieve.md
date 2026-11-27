# 006-segmentedsieve.hpp — Evidence

`006-segmentedsieve.hpp` completes the segmented-sieve inventory row: signed half-open prime intervals, reusable base primes, plain, odd and wheel block modes, streamed and materialized results, interval factorization and the four legacy multiplicative tables. The ordinary and linear sieves stay in `004-sieve_algorithms.hpp`.

## Contracts

### SegmentedSieveMode

`block` is a positive span of consecutive integers, not a count of primes. `Plain` marks every integer from 2. `Odd` (the default) emits 2 separately and stores only odd candidates. `Wheel30` emits 2, 3 and 5 separately, keeps only candidates coprime to 30, and marks `p * k` only for cofactors `k` coprime to 30. Every mode handles the last partial block and any boundary alignment.

| Mode | Candidate/marking behavior | Working candidate storage |
|---|---|---|
| `Plain` | All integers from 2; marking starts at `p * max(p, ceil(first / p))`. | At most `block` bytes. |
| `Odd` | Odd candidates; the start cofactor is forced odd (`k \|= 1`), and the index advances by `p` (value step `2 * p`). | At most `ceil(block / 2)` bytes. |
| `Wheel30` | Wheel cofactors only: the start cofactor moves to the next residue coprime to 30, then follows the wheel gaps. | At most `block` bytes; a wheel traversal, **not packed wheel storage**. |

Wheel residues are `1, 7, 11, 13, 17, 19, 23, 29` modulo 30, with cyclic gaps `6, 4, 2, 4, 2, 4, 6, 2`. For `p >= 7`, a multiple of `p` survives the small-prime exclusions exactly when its cofactor is also coprime to 30, which justifies skipping every other cofactor.

Kernel arithmetic: block ends are formed in 128 bits. Each base prime's start cofactor is `k = max(p, ceil(first / p))`, computed in `lng` with positive quotient and remainder. Odd mode sets `k |= 1`, since `p * k` is odd exactly when `k` is odd; Wheel30 adds `NEXT[k % 30]`, the distance to the next residue coprime to 30. Marking walks block indices directly: index step `p` for Plain and Odd, wheel gaps `STEP[k % 30]` for Wheel30. The offsets `p * k - first` are computed in wrapping `ulng`, exact because the true value is nonnegative and below `2^64`: the starting product is below `max(p * p, first + 7 * p)`; inside the Wheel30 walk `p * k` is recomputed each step and stays below `hi + 6 * p <= 2^63 + 6 * 3037000499 < 2^64`; Plain and Odd indices stay below `n + p`. `size_t` remains only as the `assign`/`operator[]` conversion in base setup and in materializing `is_prime`; loops use `lng`.

### segmented_sieve_detail

`root(n)` returns `floor(sqrt(n))` for `n >= 0`, correcting a floating estimate with exact 128-bit squares. `firstMultiple(l, p)` returns the least multiple of `p` that is `>= l`, as `lll`. `validate` asserts `l <= r`, a positive block and a known mode. `visit` is the shared block kernel. Each helper carries a complexity line.

### SegmentedSieveBase

`SegmentedSieveBase(max_value, block = 32768)` and `reset(max_value, block)` build the primes through `s = floor(sqrt(max_value))`, with `max_value >= 0`, and serve every prime query whose interval satisfies `R - 1 <= max_value`. Every `lng` maximum is supported arithmetically, but near `INT64_MAX` the base primes alone take about **1.2 GB** of storage and substantial setup time. Setup uses a tiny dense seed through `sqrt(s)` and a blocked odd sieve through `s`, never a dense array through `s`. Cost: time `O(s * log(log(s + 3)) + ceil(s / B) * pi(sqrt(s)))`, stored memory `O(pi(s))`, workspace `O(B + sqrt(s))`.

`forEachPrime(L, R, f, block = 32768, mode = Odd)` reports the primes in **half-open `[L, R)`** in ascending order. Endpoints are signed, values below 2 are not prime, and empty intervals are valid. A nonempty interval with values of at least 2 must satisfy `R - 1 <= max_value` (asserted). The callback must not mutate the base. Repeated queries share the immutable base and cache no interval state. Time is `O(n * log(log(R + 3)) + ceil(n / B) * pi(sqrt(R)))` with `n = max(0, R - max(L, 2))`, and workspace is `O(B)`, excluding callbacks. Every block rescans the base primes below its end, so very small blocks lose the usual sieve bound.

### SegmentedSieve

`SegmentedSieve(L, R, block, mode)` materializes the **half-open** interval `[L, R)` with its own base; the overload `(L, R, const SegmentedSieveBase &, block, mode)` reuses an existing base. It keeps fields `l`, `r`, `is_prime`, `prms` and `bprms`: offset `i` means the integer `l + i`, `is_prime.size() == r - l`, and `prms` is sorted. Any signed endpoints are valid when `r - l` fits `vector::max_size` and memory allows. Empty half-open intervals at either signed extreme stay valid without building huge bases.

A materialized object **copies** only the base primes it can read, those with `p <= floor(sqrt(r - 1))`, which is all that `forEachFactor` (stopping at `p * p >= r`) and `factorize(x)` (`x < r`) use. So the object stays valid after the source base is reset or destroyed, and its memory is `O(n + pi(sqrt(r)))`. For many queries whose results need not persist, stream instead.

`reset(L, R, base, block, mode)` replaces the interval and clears the old optional tables and workspace; vector capacity may stay allocated. Copying creates an independent snapshot, and moving follows vector semantics. Public metadata and vectors are algorithm state and must not be edited outside the APIs. Construction costs base setup plus one streamed query plus `O(n)`. Optional tables add `O(n)` storage, and returned factor lists add `O(n * log(R))`.

### Factorization and Tables

These operations concern **positive integers**, not absolute values of signed integers:

- `forEachFactor(f)` calls `f(lng offset, lng prime, int exponent)` for every distinct prime factor of each positive value greater than one, in increasing prime order per offset. Nonpositive values and 1 emit no factor. The offset is `lng` because intervals may exceed `2^31`. `rem` stores the per-entry residual after small-prime division; final residual primes are reported without overwriting that legacy workspace convention.
- `factorize()` returns sorted factor lists for the whole interval and requires a positive interval (`l >= 1`) or an empty one; 1 has an empty factorization. `factorize(x)` requires `1 <= x` and `l <= x < r` and returns one sorted factorization.
- `divisorCountTable`, `divisorSumTable`, `phiTable` and `mobiusTable` populate only the named table plus the shared residual workspace; repeated calls are valid and replace that table. `tables()` computes all four in one factor traversal.
- For offsets representing `x <= 0`, all four values are **zero sentinels outside the positive domain**, and at `x = 1` all four are one. No claim about the divisors of zero or the factorization of negative integers is implied.

`num_div` and `phi` use `lng`, `mu` uses `vector<int8_t>`, and `sum_div` is deliberately widened from `vector<lng>` to **`vector<lll>`**, since a positive signed-64-bit integer can have a divisor sum above `INT64_MAX`. The recurrence `1 + p + ... + p^e` and its products use signed 128-bit arithmetic throughout. For `x <= INT64_MAX`, `sigma(x) <= x*(x+1)/2 < 2^125`, so all positive partial products fit; `tau(x) <= x` and `phi(x) <= x` fit `lng`.

Correctness: the interval factor engine initializes residuals once, divides the multiples of each base prime (found with `firstMultiple`, checking the distance to the end before adding `p`) by its full prime power, and reports each remaining factor greater than one as prime. A composite residual would have had a factor at most its square root, hence at most `sqrt(r-1)`, already removed; this includes prime squares at interval boundaries. Individual and combined table traversals have the conservative bound `O((r-l)*log(max(2,r)) + pi(sqrt(max(2,r))))`, with `O(r-l)` residual and table storage; returned factor lists add output-sized storage. Single-value factorization stops once its shrinking residual is smaller than `p*p`. These details remove the archive's `L + p - 1`, `R + 1`, signed iteration and divisor-sum overflows.

### Legacy Mapping

| Removed (prune, 2026-10-12) | Replacement |
|---|---|
| `SegmentedSieve::fromClosed(L, R[, base], block, mode)` | `SegmentedSieve(L, R + 1[, base], block, mode)`; the caller checks `R < INT64_MAX` before adding one, since a closed interval ending at `INT64_MAX` has no representable exclusive end |
| `getPrimeFac` (P276 rename, 2026-11-10) | `factorize` |
| `getNumDiv` (P276 rename, 2026-11-10) | `divisorCountTable` |
| `getSumDiv` (P276 rename, 2026-11-10) | `divisorSumTable` |
| `getPhi` (P276 rename, 2026-11-10) | `phiTable` |
| `getMu` (P276 rename, 2026-11-10) | `mobiusTable` |
| `getTables` (P276 rename, 2026-11-10) | `tables` |

## Feature-to-Test Map

Entry: [`006-segmentedsieve_tester.py`](<../../96-Local Testing/05-Mathematics/006-segmentedsieve_tester.py>). Quick mode uses endpoints `[-4,24]`, three block sizes and 50 random intervals; full uses the bounds below; stress extends exhaustive endpoints to `[-8,96]` with 3000 random intervals. High and boundary regressions run in every mode.

| Feature | Verification |
|---|---|
| `SegmentedSieveBase` (`reset`, `bprms`, `forEachPrime`): base generation and reuse | Every base limit through 4096, against independently enumerated trial/Miller–Rabin primes through each square-root boundary; repeated queries, base reset and shrink, materialized snapshot lifetime (exactly the needed base prefix `p * p <= r - 1` after the source base is reset), copy and move. |
| `SegmentedSieveMode` (`Plain`, `Odd`, `Wheel30`), `SegmentedSieve` (`is_prime`, `prms`): all three modes and arbitrary block boundaries | Every half-open endpoint pair in `[-6,64]` with block spans `1,2,7,30,31,64`; each flag, stored list and streamed list against independent primality. |
| Signed, empty and singleton intervals | Negative-only intervals near `INT64_MIN`, huge negative streaming ranges, empty `INT64_MAX`, zero and one, the prime singleton `[97, 98)`, `[0, 98)` on a shared base equal to its own-base twin and including `r - 1 = 97`, and the materialized width overflow probe. |
| `forEachFactor`, `factorize`, `divisorCountTable`, `divisorSumTable`, `phiTable`, `mobiusTable`, `tables` | Independent trial factorization and direct divisor-pair count and sum; individual and combined parity and repeated calls; full positive-interval factor output, callback output (`lng` offset), and the one and nonpositive table sentinels. |
| High endpoints and arithmetic | Short intervals near `10^9`, `10^12` and a large prime square, against an independent deterministic 64-bit Miller–Rabin first validated by trial division for every integer through 10000; independent divisor and factor checks around `10^12`; exact square inequalities and first-multiple invariants at `INT64_MAX`. |
| Broad and adversarial inputs | 500 seeded intervals in the full suite; known prime counts `pi(10^5)=9592`, `pi(10^6)=78498`; tiny blocks and all wheel alignment residues. |
| Preconditions | 13 assertion subprocess probes: negative base limits, invalid blocks, modes or order, insufficient bases, oversized materialization and invalid factorization domains. |

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20; GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`); the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations, with counts per configuration and equal on both compilers.

| Date, package | Command | Result |
|---|---|---|
| 2026-10-12 P012 prune (CPython 3.14.7) | `006-segmentedsieve_tester.py --mode full --seed 1` | PASS, 2,390,564 C++ checks, 13 assertion probes, `MEMORY peak=463MB` |
| 2026-10-12 P012 prune | same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=478MB` |
| 2026-10-12 P012 prune | `02-integration.py`, `03-consistency.py` | PASS |
| 2026-11-10 P274 (seed 42, beside the other suites, build cache emptied before each cold run) | `01-run.py --mode full` (GCC 16.2.1) | PASS; pre-flight quick peak 763 MB; optimized 0.2 s compile + 1.3 s run, checked 16.6 s + 2.7 s, ASan-UBSan 18.6 s + 15.2 s; MEMORY peak 940 MB |
| 2026-11-10 P274 | `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS; pre-flight quick peak 832 MB; optimized 0.4 s + 1.3 s, checked 21.8 s + 3.8 s, ASan-UBSan 27.3 s + 12.8 s; MEMORY peak 958 MB |
| 2026-11-10 P274 | `01-run.py --mode quick` | PASS; optimized 24.9 s + 0.6 s, checked 16.4 s + 7.3 s; MEMORY peak 760 MB |
| 2026-11-10 P274 | `01-run.py --mode stress --rounds 1` | PASS; pre-flight quick peak 767 MB; optimized 0.1 s + 1.5 s, checked 6.2 s + 3.2 s, ASan-UBSan 7.6 s + 16.2 s; MEMORY peak 524 MB |
| 2026-11-10 P276 | full on g++ and g++-14, stress and extraction | passed (counts not recorded) |

## Benchmarks

Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, [00-foundations_benchmark.cpp](<../../96-Local Testing/05-Mathematics/00-foundations_benchmark.cpp>) (driver and record shared with [004-sieve_algorithms.md](004-sieve_algorithms.md)). Median of three interleaved runs, each the median of 5 samples after one warmup, in ms; block 32768 unless noted; streaming excludes base setup.

| Workload | Plain | Odd | Wheel30 |
|---|---|---|---|
| `[0, 2e6)` | 4.11 | 1.88 | 7.74 |
| `[1e12, 1e12+2e6)` | 27.73 | 21.39 | 30.87 |
| same, block 1024 | 461.13 | 454.92 | 496.58 |
| same, block 262144 | 10.22 | 6.78 | 13.60 |
| `[1e16-1e5, 1e16)` | 78.05 | 80.11 | 84.05 |

The materialize workload (100 `SegmentedSieve` objects `[100+q, 200+q)` sharing a base for `10^16 - 1`) takes 0.02 ms. Odd stays the default: Wheel30 is slower than Odd at low endpoints because of its per-step table walk. No Barrett/Montgomery or ISA-specific code is used.

## Sources

Inspected 2026-09-27:

| Reference | Inspected material and use |
|---|---|
| [cp-algorithms, Sieve of Eratosthenes](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html) | Square-root bases, blocked prime intervals, odd candidates, byte/bit storage tradeoffs and per-block division overhead. Implemented independently with reusable bases and explicit signed and wide arithmetic. |
| [OI Wiki, 筛法](https://oi-wiki.org/math/number-theory/sieve/) | Odd and segmented sieves and multiplicative formulas for phi, mu, divisor count and divisor sum; domains and formulas compared with the independent divisor and factorization oracles. |
| [primesieve, EratSmall.cpp](https://github.com/kimwalisch/primesieve/blob/master/src/EratSmall.cpp), Kim Walisch, 2026 BSD source | The modulo-30 small-prime kernel and its explanation of cache blocks, initialization overhead and skipping multiples of 2, 3 and 5. The compact cofactor-wheel traversal here is independent; none of the unrolled or packed implementation was copied. |

## Limits and Handoffs

- The constructor always means half-open `[L, R)`; old closed intervals map as in Legacy Mapping. Divisor sums use `lll`. There were no active dependent call sites to migrate.
- Enormous segmented queries still need square-root base-prime storage; the contracts above state that cost and the tested high-endpoint limits.
- The `SegmentedSieve` excerpt in `OLD/5-Mathematics/04-sieves.hpp` and the mechanical-split statement in `00-Guidelines/History/2026-09-27-migration.md` were reviewed. Prime flags and lists, exposed base primes, residual workspace and all four table methods are accounted for; the archive is preserved.
- The 2026-10-07 completeness sweep found no missing operation: cp-algorithms range sieve, KACTL FastEratosthenes, maspypy `factor_interval`/`prime_table` and Library Checker `enumerate_primes` are covered by `prms`, `forEachPrime` and `forEachFactor`.
- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.

## Aliases

Renamed aliases (P276, 2026-11-10): `getPrimeFac`, `getNumDiv`, `getSumDiv`, `getPhi`, `getMu`, `getTables`.

Search aliases (P263, grab --find): 筛法, 区间筛, 分段筛, 区间素数.

## History

- 2026-09-27: initial P012 evidence.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit fixed 5 findings and 1 reviewer evidence finding; after `@reviewer` (which probed `[INT64_MAX - 3000, INT64_MAX)` against Miller–Rabin), full, stress (seed 1) and integration passed.
- 2026-10-12: prune removed both `fromClosed` overloads (cases moved to the half-open constructors); full suite passed on g++ and g++-14.
- 2026-11-07: P263 added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 moved the builds to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; library-wide runs passed. For `-Werror` without demotions the header gained `[[maybe_unused]]` on names only an `assert` uses and split a chained assignment in `tables` in two, which removed its `-Wconversion` warning; no logic change and no mutation campaign in P274 (user decision 2026-11-10).
- 2026-11-10: P276 renamed `get…` to `factorize`, `…Table` and `tables`; full on g++ and g++-14, stress and extraction passed.
