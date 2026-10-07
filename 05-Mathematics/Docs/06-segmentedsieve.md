# 06-segmentedsieve.hpp — evidence

`06-segmentedsieve.hpp` completes the segmented-sieve inventory row: signed half-open prime intervals, reusable base primes, plain/odd/wheel block modes, streamed and materialized results, interval factorization and the four legacy multiplicative tables. The ordinary sieve and linear sieve remain in `04-sieve_algorithms.hpp`.

## Contracts

### SegmentedSieveMode

`block` is a span of consecutive integers, not a count of primes, and must be positive. `Plain` marks every integer from 2. `Odd` (the default) emits 2 separately and stores only odd candidates. `Wheel30` emits 2, 3 and 5 separately, keeps only candidates coprime to 30, and marks `p * k` only for cofactors `k` coprime to 30. Every mode handles the last partial block and any boundary alignment.

| Mode | Candidate/marking behavior | Working candidate storage |
|---|---|---|
| `Plain` | All integers from 2; marking starts at `p * max(p, ceil(first / p))`. | At most `block` bytes. |
| `Odd` | Odd candidates; the start cofactor is forced odd (`k \|= 1`), and the index advances by `p` (value step `2 * p`). | At most `ceil(block / 2)` bytes. |
| `Wheel30` | Wheel cofactors only: the start cofactor moves to the next residue coprime to 30, then follows the wheel gaps. | At most `block` bytes; a wheel traversal, **not packed wheel storage**. |

Wheel residues are `1, 7, 11, 13, 17, 19, 23, 29` modulo 30, with cyclic gaps `6, 4, 2, 4, 2, 4, 6, 2`. For `p >= 7`, a multiple of `p` survives the small-prime exclusions exactly when its cofactor is also coprime to 30. That justifies skipping every other cofactor.

Kernel arithmetic: block ends are formed in 128 bits. Each base prime's start cofactor is `k = max(p, ceil(first / p))`, computed in `lng` with positive quotient and remainder. Odd mode sets `k |= 1`, since `p * k` is odd exactly when `k` is odd; Wheel30 adds `NEXT[k % 30]`, the distance to the next residue coprime to 30. Marking walks block indices directly: index step `p` for Plain and Odd, wheel gaps `STEP[k % 30]` for Wheel30. The offsets `p * k - first` are computed in wrapping `ulng`, exact because the true value is nonnegative and below `2^64`: the starting product is below `max(p * p, first + 7 * p)`, inside the Wheel30 walk `p * k` is recomputed each step and stays below `hi + 6 * p <= 2^63 + 6 * 3037000499 < 2^64`, and Plain and Odd indices stay below `n + p`. `size_t` remains only as the `assign`/`operator[]` conversion in base setup and in materializing `is_prime`; loops use `lng`.

### segmented_sieve_detail

`root(n)` returns `floor(sqrt(n))` for `n >= 0`. It corrects a floating estimate using exact 128-bit squares. `firstMultiple(l, p)` returns the least multiple of `p` that is `>= l`, as `lll`. `validate` asserts `l <= r`, a positive block and a known mode. `visit` is the shared block kernel. Each helper carries a complexity line.

### SegmentedSieveBase

`SegmentedSieveBase(max_value, block = 32768)` and `reset(max_value, block)` build the primes through `s = floor(sqrt(max_value))`, with `max_value >= 0`. They serve every prime query whose interval satisfies `R - 1 <= max_value`. Every `lng` maximum is supported arithmetically. Near `INT64_MAX`, however, the base primes alone take about **1.2 GB** of storage and substantial setup time. Setup uses a tiny dense seed through `sqrt(s)` and a blocked odd sieve through `s`, never a dense array through `s`. Cost: time `O(s * log(log(s + 3)) + ceil(s / B) * pi(sqrt(s)))`, stored memory `O(pi(s))`, workspace `O(B + sqrt(s))`.

`forEachPrime(L, R, f, block = 32768, mode = Odd)` reports the primes in **half-open `[L, R)`** in ascending order. Endpoints are signed. Values below 2 are not prime, and empty intervals are valid. A nonempty interval with values of at least 2 must satisfy `R - 1 <= max_value` (asserted). The callback must not mutate the base. Repeated queries share the immutable base and cache no interval state. Time is `O(n * log(log(R + 3)) + ceil(n / B) * pi(sqrt(R)))` with `n = max(0, R - max(L, 2))`, and workspace is `O(B)`, excluding callbacks. Every block rescans the base primes below its end, so very small blocks do not keep the usual sieve bound.

### SegmentedSieve

`SegmentedSieve(L, R, block, mode)` materializes the **half-open** interval `[L, R)` with its own base. The overload `(L, R, const SegmentedSieveBase &, block, mode)` reuses an existing base. Fields `l`, `r`, `is_prime`, `prms` and `bprms` are kept. Offset `i` means the integer `l + i`, `is_prime.size() == r - l`, and `prms` is sorted. Any signed endpoints are valid when `r - l` fits `vector::max_size` and memory allows.

A materialized object **copies** only the base primes it can read: those with `p <= floor(sqrt(r - 1))`. That is all that `forEachFactor` (which stops at `p * p >= r`) and `getPrimeFac(x)` (`x < r`) use. So the object stays valid after the source base is reset or destroyed, and its memory is `O(n + pi(sqrt(r)))`. For many queries whose results need not persist, use streaming instead.

The archived constructor used **closed `[L, R]`**. Migrate such calls to `SegmentedSieve::fromClosed(L, R, ...)`, which exists with and without a reusable base. It checks `L <= R` and `R < INT64_MAX` **before** forming `R + 1`. So a closed right endpoint at `INT64_MAX` is rejected, because the excluded endpoint is not representable. No implicit signed overflow or silent wraparound occurs. Empty half-open intervals at either signed extreme stay valid without building huge bases.

`reset(L, R, base, block, mode)` replaces the interval and clears the old optional tables and workspace. Logical contents are reset, but vector capacity may stay allocated. Copying creates an independent snapshot, and moving follows vector semantics. Public metadata and vectors are algorithm state and must not be edited outside the APIs. Construction costs base setup plus one streamed query plus `O(n)`. Optional tables add `O(n)` storage, and returned factor lists add `O(n * log(R))`.

### Factorization and tables

The following number-theory operations concern **positive integers**, not absolute values of signed integers:

- `forEachFactor(f)` calls `f(lng offset, lng prime, int exponent)` for every distinct prime factor of each positive value greater than one, in increasing prime order for each offset. Nonpositive values and 1 emit no factor. The offset is `lng` because intervals may exceed `2^31`. `rem` stores the per-entry residual after small-prime division; final residual primes are reported without overwriting that legacy workspace convention.
- `getPrimeFac()` returns sorted factor lists for the whole interval; it requires a positive interval (`l >= 1`) or an empty interval. One has an empty factorization. `getPrimeFac(x)` requires `1 <= x` and `l <= x < r` and returns one sorted factorization.
- `getNumDiv`, `getSumDiv`, `getPhi`, and `getMu` populate only the named table plus the shared residual workspace. Repeated calls are valid and replace that table. `getTables()` computes all four in one factor traversal when all are needed.
- For table offsets representing `x <= 0`, all four values are **zero sentinels outside the positive domain**. At `x = 1`, all four are one. No mathematical claim about the divisors of zero or the factorization of negative integers is implied.

`num_div` and `phi` use `lng`, `mu` uses `vector<int8_t>`, and `sum_div` is deliberately widened from `vector<lng>` to **`vector<lll>`**. A positive signed-64-bit integer can have a divisor sum exceeding `INT64_MAX`. The recurrence `1 + p + ... + p^e` and products use signed 128-bit arithmetic throughout. For `x <= INT64_MAX`, `sigma(x) <= x*(x+1)/2 < 2^125`, so all intermediate positive partial products fit; `tau(x) <= x` and `phi(x) <= x` fit `lng`.

Correctness: the interval factor engine initializes residuals once, divides multiples of each base prime (found with `firstMultiple`, checking the distance to the end before adding `p`) by its full prime power, and reports each remaining factor greater than one as prime. A composite residual would have had a factor at most its square root, hence at most `sqrt(r-1)`, already removed; this includes prime squares at interval boundaries. Individual and combined table traversals have a conservative bound `O((r-l)*log(max(2,r)) + pi(sqrt(max(2,r))))`, with `O(r-l)` residual/table storage. Returned factor lists add output-sized storage. Single-value factorization stops as its shrinking residual becomes smaller than `p*p`. These details remove the archive's `L + p - 1`, `R + 1`, signed iteration, and divisor-sum overflows.

## Feature-to-test map

Runnable entry: [`06-segmentedsieve_tester.py`](<../../96-Local Testing/05-Mathematics/06-segmentedsieve_tester.py>). It includes the actual production header, uses non-removable checks, and runs from any working directory through the shared runner. Quick uses endpoints `[-4,24]`, three block sizes and 50 random intervals. Full uses the bounds below. Stress extends exhaustive endpoints to `[-8,96]` and uses 3000 random intervals. High and boundary regressions run in every mode.

| Feature | Verification |
|---|---|
| Base generation and reuse | Exhaust base limits through 4096; independently enumerate trial/Miller–Rabin primes through each square-root boundary. Repeated queries, base reset/shrink, materialized snapshot lifetime (exactly the needed base prefix `p * p <= r - 1` after the source base is reset), copy and move. |
| All three modes and arbitrary block boundaries | Exhaust all half-open endpoint pairs in `[-6,64]` with block spans `1,2,7,30,31,64`; compare each flag, stored list and streamed list with independent primality. |
| Signed/empty/singleton and closed migration | Negative-only intervals near `INT64_MIN`, huge negative streaming ranges, empty `INT64_MAX`, zero/one, closed prime singleton, explicit maximum closed-endpoint death tests and materialized width overflow checks. |
| Factor lists and multiplicative tables | Independent trial factorization and direct divisor-pair count/sum; individual/combined parity and repeated calls. Full positive interval factor output, callback output (`lng` offset), one and nonpositive table sentinels. |
| High endpoints and arithmetic | Short intervals near `10^9`, `10^12`, and a large prime square; independent deterministic 64-bit Miller–Rabin, validated first against trial division for every integer through 10000. Independent divisor/factor checks around `10^12`. Exact square inequalities and first-multiple invariants at `INT64_MAX`. |
| Broad/adversarial inputs | 500 seeded intervals in the full suite; known prime counts `pi(10^5)=9592`, `pi(10^6)=78498`; tiny blocks and all wheel alignment residues. |
| Preconditions | 16 assertion subprocess probes for negative base limits, invalid blocks/modes/order, insufficient bases, closed maximum endpoints, oversized materialization and invalid factorization domains. |

## Commands and results

Latest P012 re-audit runs (2026-10-07), recorded once for all six headers `01-mod_arithmetic.hpp` … `06-segmentedsieve.hpp`. GCC 16.2.1, GNU++20, Python 3.14, Linux x86-64 (i9-11900H). Every C++ build uses the shared runner's optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` and ASan/UBSan configurations (quick runs the first two).

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 05-Mathematics/0 --no-integration                     # PASS, 6 suites, 2 configurations
python3 '96-Local Testing/01-run.py' --mode full --filter 05-Mathematics/0 --seed 1 --no-integration              # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/01-run.py' --mode stress --filter 05-Mathematics/0 --seed 1 --rounds 1 --no-integration # PASS, 6 suites, 3 configurations
python3 '96-Local Testing/02-integration.py'                                                                     # PASS, 102 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                     # no errors
```

After the `@reviewer` pass, full (all six headers), stress (combinatorics) and integration were rerun and passed with these counts:

| Mode | Count |
|---|---|
| Full (seed 1), per configuration | 2,390,564 C++ checks |
| Stress (seed 1) | 7,768,677 |
| Assertion probes | 16 |

The `@reviewer` pass independently probed the segmented sieve at `[INT64_MAX - 3000, INT64_MAX)` against Miller–Rabin. No online submissions or acceptance claims were made.

## Benchmarks

Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, [00-foundations_benchmark.cpp](<../../96-Local Testing/05-Mathematics/00-foundations_benchmark.cpp>) (driver and record shared with [04-sieve_algorithms.md](04-sieve_algorithms.md)). Median of three interleaved runs, each the median of 5 samples after one warmup, in ms; block 32768 unless noted; streaming excludes base setup.

| Workload | Plain | Odd | Wheel30 |
|---|---|---|---|
| `[0, 2e6)` | 4.11 | 1.88 | 7.74 |
| `[1e12, 1e12+2e6)` | 27.73 | 21.39 | 30.87 |
| same, block 1024 | 461.13 | 454.92 | 496.58 |
| same, block 262144 | 10.22 | 6.78 | 13.60 |
| `[1e16-1e5, 1e16)` | 78.05 | 80.11 | 84.05 |

Materialize workload (100 `SegmentedSieve` objects `[100+q, 200+q)` sharing a base for `10^16 - 1`): 0.02 ms. The committed record `96-Local Testing/05-Mathematics/00-foundations_benchmark.json` (single run) gave 66.86/66.80/83.53 ms at `10^16` and 0.03 ms for materialize. Odd remains the default; Wheel30 is slower than Odd at low endpoints because of its per-step table walk. These are host measurements, not timing gates. No Barrett/Montgomery or ISA-specific code was introduced.

## Sources

Inspected 2026-09-27:

| Reference | Inspected material and use |
|---|---|
| [cp-algorithms, Sieve of Eratosthenes](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html) | Square-root bases, blocked prime intervals, odd candidates, byte/bit storage tradeoffs and per-block division overhead. Implemented independently with reusable bases and explicit signed/wide arithmetic. |
| [OI Wiki, 筛法](https://oi-wiki.org/math/number-theory/sieve/) | Odd and segmented sieves and multiplicative formulas for phi, mu, divisor count and divisor sum. Compared domains and formulas with the independent divisor/factorization oracles. |
| [primesieve, EratSmall.cpp](https://github.com/kimwalisch/primesieve/blob/master/src/EratSmall.cpp), Kim Walisch, 2026 BSD source | Inspected the modulo-30 small-prime kernel and its explanation of cache blocks, initialization overhead and skipping multiples of 2, 3 and 5. The local compact cofactor-wheel traversal is independently implemented; none of the specialized unrolled/packed implementation was copied. |

## Limits and handoffs

- The constructor consistently means half-open `[L, R)`; `SegmentedSieve::fromClosed` serves old closed intervals and rejects an unrepresentable exclusive maximum endpoint. Divisor sums use `lll`. There were no active dependent call sites to migrate.
- Enormous segmented queries still require square-root base-prime storage; the contracts above report that cost and the tested high-endpoint limits.
- The `SegmentedSieve` excerpt in `OLD/5-Mathematics/04-sieves.hpp` and the mechanical-split statement in `00-Guidelines/History/2026-09-27-migration.md` were reviewed. Prime flags/lists, exposed base primes, residual workspace and all four table methods are accounted for; the archive remains preserved.
- The 2026-10-07 completeness sweep found no missing operation: cp-algorithms range sieve, KACTL FastEratosthenes, maspypy `factor_interval`/`prime_table` and Library Checker `enumerate_primes` are covered by `prms`, `forEachPrime` and `forEachFactor`.

## History

- 2026-09-27: initial P012 evidence.
- 2026-10-07: P012 re-audit start, full suite passed on all six headers before any change; re-audit fixed findings 2 (branch-free Odd/Wheel30 start offsets, up to 5x faster at high endpoints), 3 (prefix-only base copy; materialize 1087 ms to 0.02 ms), 10–12 (complexity lines, braces, `lng` offsets, `int8_t` Möbius), and the reviewer's Wheel30 offset-bound evidence error.
