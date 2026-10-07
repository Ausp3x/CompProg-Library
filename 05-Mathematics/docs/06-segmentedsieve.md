# MA02 segmented sieves — contract and evidence

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

### segmented_sieve_detail

`root(n)` returns `floor(sqrt(n))` for `n >= 0`. It corrects a floating estimate using exact 128-bit squares. `firstMultiple(l, p)` returns the least multiple of `p` that is `>= l`, as `lll`. `validate` asserts `l <= r`, a positive block and a known mode. `visit` is the shared block kernel. Each helper carries a complexity line (re-audit finding 12).

### SegmentedSieveBase

`SegmentedSieveBase(max_value, block = 32768)` and `reset(max_value, block)` build the primes through `s = floor(sqrt(max_value))`, with `max_value >= 0`. They serve every prime query whose interval satisfies `R - 1 <= max_value`. Every `lng` maximum is supported arithmetically. Near `INT64_MAX`, however, the base primes alone take about **1.2 GB** of storage and substantial setup time. Setup uses a tiny dense seed through `sqrt(s)` and a blocked odd sieve through `s`, never a dense array through `s`. Cost: time `O(s * log(log(s + 3)) + ceil(s / B) * pi(sqrt(s)))`, stored memory `O(pi(s))`, workspace `O(B + sqrt(s))`.

`forEachPrime(L, R, f, block = 32768, mode = Odd)` reports the primes in **half-open `[L, R)`** in ascending order. Endpoints are signed. Values below 2 are not prime, and empty intervals are valid. A nonempty interval with values of at least 2 must satisfy `R - 1 <= max_value` (asserted). The callback must not mutate the base. Repeated queries share the immutable base and cache no interval state. Time is `O(n * log(log(R + 3)) + ceil(n / B) * pi(sqrt(R)))` with `n = max(0, R - max(L, 2))`, and workspace is `O(B)`, excluding callbacks. Every block rescans the base primes below its end, so very small blocks do not keep the usual sieve bound.

### SegmentedSieve

`SegmentedSieve(L, R, block, mode)` materializes the **half-open** interval `[L, R)` with its own base. The overload `(L, R, const SegmentedSieveBase &, block, mode)` reuses an existing base. Fields `l`, `r`, `is_prime`, `prms` and `bprms` are kept. Offset `i` means the integer `l + i`, `is_prime.size() == r - l`, and `prms` is sorted. Any signed endpoints are valid when `r - l` fits `vector::max_size` and memory allows.

A materialized object **copies** only the base primes it can read: those with `p <= floor(sqrt(r - 1))`. That is all that `forEachFactor` (which stops at `p * p >= r`) and `getPrimeFac(x)` (`x < r`) use. So the object stays valid after the source base is reset or destroyed, and its memory is `O(n + pi(sqrt(r)))` as the complexity line states. This fixes re-audit finding 3: the earlier full copy of the base vector made 100 small objects built from a `10^16` base store 46 MB each. For many queries whose results need not persist, use streaming instead.

The archived constructor used **closed `[L, R]`**. Migrate such calls to `SegmentedSieve::fromClosed(L, R, ...)`, which exists with and without a reusable base. It checks `L <= R` and `R < INT64_MAX` **before** forming `R + 1`. So a closed right endpoint at `INT64_MAX` is rejected, because the excluded endpoint is not representable. No implicit signed overflow or silent wraparound occurs. Empty half-open intervals at either signed extreme stay valid without building huge bases. No active call sites needed conversion; archived code is unchanged.

`reset(L, R, base, block, mode)` replaces the interval and clears the old optional tables and workspace. Logical contents are reset, but vector capacity may stay allocated. Copying creates an independent snapshot, and moving follows vector semantics. Public metadata and vectors are algorithm state and must not be edited outside the APIs. Construction costs base setup plus one streamed query plus `O(n)`. Optional tables add `O(n)` storage, and returned factor lists add `O(n * log(R))`.

### Factorization and tables

The following number-theory operations concern **positive integers**, not absolute values of signed integers:

- `forEachFactor(f)` calls `f(lng offset, lng prime, int exponent)` for every distinct prime factor of each positive value greater than one, in increasing prime order for each offset. Nonpositive values and 1 emit no factor. The offset is `lng` (it was `size_t` before re-audit finding 12) because intervals may exceed `2^31`. `rem` stores the per-entry residual after small-prime division; final residual primes are reported without overwriting that legacy workspace convention.
- `getPrimeFac()` returns sorted factor lists for the whole interval; it requires a positive interval (`l >= 1`) or an empty interval. One has an empty factorization. `getPrimeFac(x)` requires `1 <= x` and `l <= x < r` and returns one sorted factorization.
- `getNumDiv`, `getSumDiv`, `getPhi`, and `getMu` populate only the named table plus the shared residual workspace. Repeated calls are valid and replace that table. `getTables()` computes all four in one factor traversal when all are needed.
- For table offsets representing `x <= 0`, all four values are **zero sentinels outside the positive domain**. At `x = 1`, all four are one. No mathematical claim about the divisors of zero or the factorization of negative integers is implied.

`num_div` and `phi` use `lng`, `mu` uses `vector<int8_t>`, and `sum_div` is deliberately widened from `vector<lng>` to **`vector<lll>`**. A positive signed-64-bit integer can have a divisor sum exceeding `INT64_MAX`. The recurrence `1 + p + ... + p^e` and products use signed 128-bit arithmetic throughout. For `x <= INT64_MAX`, `sigma(x) <= x*(x+1)/2 < 2^125`, so all intermediate positive partial products fit; `tau(x) <= x` and `phi(x) <= x` fit `lng`.

The interval factor engine initializes residuals once, divides multiples of each base prime by its full prime power, and reports each remaining factor greater than one as prime. A composite residual would have had a factor at most its square root, hence at most `sqrt(r-1)`, already removed. This proves completeness, including prime squares at interval boundaries. Individual and combined table traversals have a conservative bound `O((r-l)*log(max(2,r)) + pi(sqrt(max(2,r))))`, with `O(r-l)` residual/table storage. Returned factor lists add output-sized storage. Single-value factorization stops as its shrinking residual becomes smaller than `p*p`.

Block ends are formed in 128 bits. In the prime kernel, each base prime's start cofactor is `k = max(p, ceil(first / p))`, computed in `lng` with positive quotient and remainder. Odd mode sets `k |= 1`, since `p * k` is odd exactly when `k` is odd. Wheel30 adds `NEXT[k % 30]`, the distance to the next residue coprime to 30. Marking then walks block indices directly: index step `p` for Plain and Odd, wheel gaps `STEP[k % 30]` for Wheel30. The offsets `p * k - first` are computed in wrapping `ulng`. This is exact because the true value is nonnegative and below `2^64`. The starting product is below `max(p * p, first + 7 * p)`. Inside the Wheel30 walk `p * k` is recomputed each step and stays below `hi + 6 * p <= 2^63 + 6 * 3037000499 < 2^64`. Plain and Odd indices stay below `n + p`. Re-audit finding 2 replaced the earlier branchy start (`if (odd && !(start & 1))`, plus a `while` loop over wheel residues) and a per-element division by the stride. The interval factor engine uses `firstMultiple` and checks the distance to the end before adding `p`. A corrected floating square-root estimate is validated using exact 128-bit squares. These details remove the archive's `L + p - 1`, `R + 1`, signed iteration, and divisor-sum overflows.

## Sources and legacy accounting

Inspected 2026-09-27:

| Reference | Inspected material and use |
|---|---|
| [cp-algorithms, Sieve of Eratosthenes](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html) | Square-root bases, blocked prime intervals, odd candidates, byte/bit storage tradeoffs and per-block division overhead. Implemented independently with reusable bases and explicit signed/wide arithmetic. |
| [OI Wiki, 筛法](https://oi-wiki.org/math/number-theory/sieve/) | Odd and segmented sieves and multiplicative formulas for phi, mu, divisor count and divisor sum. Compared domains and formulas with the independent divisor/factorization oracles. |
| [primesieve, EratSmall.cpp](https://github.com/kimwalisch/primesieve/blob/master/src/EratSmall.cpp), Kim Walisch, 2026 BSD source | Inspected the modulo-30 small-prime kernel and its explanation of cache blocks, initialization overhead and skipping multiples of 2, 3 and 5. The local compact cofactor-wheel traversal is independently implemented; none of the specialized unrolled/packed implementation was copied. |

The `SegmentedSieve` excerpt in `OLD/5-Mathematics/04-sieves.hpp` and the mechanical-split statement in `00-Guidelines/11-migration.md` were reviewed. Prime flags/lists, exposed base primes, residual workspace and all four table methods are accounted for. The archive remains preserved. The endpoint and divisor-sum type migrations are explicit above. No online submissions or acceptance claims were made.

## Feature-to-test map and results

Runnable entry: `96-Local Testing/05-Mathematics/06-segmentedsieve_tester.py`. It includes the actual production header, uses non-removable checks, and runs from any working directory through the shared runner.

| Feature | Verification |
|---|---|
| Base generation and reuse | Exhaust base limits through 4096; independently enumerate trial/Miller–Rabin primes through each square-root boundary. Repeated queries, base reset/shrink, materialized snapshot lifetime, copy and move. |
| All three modes and arbitrary block boundaries | Exhaust all half-open endpoint pairs in `[-6,64]` with block spans `1,2,7,30,31,64`; compare each flag, stored list and streamed list with independent primality. |
| Signed/empty/singleton and closed migration | Negative-only intervals near `INT64_MIN`, huge negative streaming ranges, empty `INT64_MAX`, zero/one, closed prime singleton, explicit maximum closed-endpoint death tests and materialized width overflow checks. |
| Factor lists and multiplicative tables | Independent trial factorization and direct divisor-pair count/sum; individual/combined parity and repeated calls. Full positive interval factor output, callback output, one and nonpositive table sentinels. |
| High endpoints and arithmetic | Short intervals near `10^9`, `10^12`, and a large prime square; independent deterministic 64-bit Miller–Rabin, validated first against trial division for every integer through 10000. Independent divisor/factor checks around `10^12`. Exact square inequalities and first-multiple invariants at `INT64_MAX`. |
| Broad/adversarial inputs | 500 seeded intervals in the full suite; known prime counts `pi(10^5)=9592`, `pi(10^6)=78498`; tiny blocks and all wheel alignment residues. |
| Preconditions | 16 assertion subprocess probes for negative base limits, invalid blocks/modes/order, insufficient bases, closed maximum endpoints, oversized materialization and invalid factorization domains. |

Quick uses endpoints `[-4,24]`, three block sizes and 50 random intervals. Full uses the bounds above. Stress extends exhaustive endpoints to `[-8,96]` and uses 3000 random intervals. High and boundary regressions run in every mode. The snapshot check now expects exactly the needed base prefix (`p * p <= r - 1`) after the source base is reset. Callbacks in the tester take a `lng` offset.

## Re-audit findings resolved (P012, 2026-10-07)

- Finding 2 (Odd/Wheel30 3–6 times slower than Plain at high endpoints): fixed with the branch-free start offsets described above. The earlier text here blamed the gap on an expected per-block tradeoff; that was wrong. The cause was branch-mispredicting start adjustments, measured below.
- Finding 3 (the materialized sieve copied every base prime): fixed with the prefix copy.
- Finding 11 (spaces before closing braces): fixed; the header passes `03-consistency.py --braces`.
- Finding 12 (`size_t` loops and callback index, missing helper complexity lines, `signed char`): loops and the callback offset are `lng`, the detail helpers have a complexity line, and `mu` is `vector<int8_t>`. `size_t` remains only as the `assign`/`operator[]` conversion in base setup and in materializing `is_prime`.

A completeness sweep (2026-10-07) found no operation missing from this header. cp-algorithms range sieve, KACTL FastEratosthenes, maspypy `factor_interval`/`prime_table` and Library Checker `enumerate_primes` are covered by `prms`, `forEachPrime` and `forEachFactor`.

## Performance evidence

A/B on the same machine (Intel Core i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`). [90-foundations_benchmark.cpp](<../../96-Local Testing/05-Mathematics/90-foundations_benchmark.cpp>) was built against the HEAD header (old) and the re-audited header (new), and the two binaries ran interleaved three times. The table gives the median of the three per-run medians (each of 5 samples after one warmup), in ms; block 32768 unless noted. Streaming excludes base setup.

| Workload | Plain old/new | Odd old/new | Wheel30 old/new |
|---|---|---|---|
| `[0, 2e6)` | 13.93/4.11 | 5.58/1.88 | 7.99/7.74 |
| `[1e12, 1e12+2e6)` | 42.79/27.73 | 63.97/21.39 | 95.98/30.87 |
| same, block 1024 | 534.04/461.13 | 1643.99/454.92 | 2628.72/496.58 |
| same, block 262144 | 19.89/10.22 | 14.09/6.78 | 20.00/13.60 |
| `[1e16-1e5, 1e16)` | 87.25/78.05 | 246.92/80.11 | 396.49/84.05 |

Materialize workload (100 `SegmentedSieve` objects `[100+q, 200+q)` sharing a base for `10^16 - 1`): old 1087.12 ms, new 0.02 ms.

An intermediate version divided 128-bit offsets by a runtime stride. It regressed Plain at high endpoints (85 to 155 ms at `10^16`), so it was replaced by the shift and `ulng` form above, which is no slower than the old code in any row. The committed record [90-foundations_benchmark.json](<../../96-Local Testing/05-Mathematics/90-foundations_benchmark.json>) now includes the `10^16` stream and materialize workloads. Its single run gave 66.86/66.80/83.53 ms at `10^16` and 0.03 ms for materialize. Odd remains the default. Wheel30 is slower than Odd at low endpoints because of its per-step table walk. These are host measurements, not timing gates. No Barrett/Montgomery or ISA-specific code was introduced.

## Verification

Commands, configurations and results for the P012 re-audit are recorded in [96-p012.md](p012.md). The suite builds optimized, checked and ASan/UBSan configurations and runs 16 assertion probes in the checked build.
