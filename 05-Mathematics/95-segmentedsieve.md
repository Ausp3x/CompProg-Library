# MA02 segmented sieves — contract and evidence

`06-segmentedsieve.hpp` completes the segmented-sieve inventory row: signed half-open prime intervals, reusable base primes, plain/odd/wheel block modes, streamed and materialized results, interval factorization and the four legacy multiplicative tables. The ordinary sieve and linear sieve remain in `04-sieve_algorithms.hpp`.

## APIs and explicit migration

`SegmentedSieveBase(max_value, block = 32768)` builds primes through `floor(sqrt(max_value))`, where `max_value >= 0`. Its `forEachPrime(L, R, callback, block = 32768, mode = SegmentedSieveMode::Odd)` reports ascending primes in **half-open `[L, R)`**. Endpoints use `lng`; negative values, zero and one are simply not prime. Empty intervals are valid. A nonempty interval containing values at least two must satisfy `R - 1 <= max_value`. Callbacks must not mutate the base. Repeated queries share immutable base data and keep no interval cache.

`SegmentedSieve(L, R, block, mode)` materializes the same half-open interval; the overload `(L, R, const SegmentedSieveBase&, block, mode)` reuses existing setup. Fields `l`, `r`, `is_prime`, `prms`, and `bprms` remain available. Offset `i` means integer `l + i`, `is_prime.size() == r - l`, and `prms` is sorted. Materialized objects **copy** the base vector, so they remain valid after the source base is reset or destroyed. For many queries whose results need not persist, use the streaming interface to avoid these copies.

The archived constructor used **closed `[L, R]`**. Migrate such calls to `SegmentedSieve::fromClosed(L, R, ...)`, available with and without a reusable base. It checks `L <= R` and `R < INT64_MAX` **before** forming `R + 1`; a closed right endpoint at `INT64_MAX` is explicitly rejected because that excluded endpoint is not representable by this signed half-open API. No implicit signed overflow or silent wraparound is allowed. Empty half-open intervals at either signed extreme remain valid without building enormous bases. There were no other active call sites requiring conversion when the header was updated; archived code remains unchanged.

`reset(L, R, base, block, mode)` replaces the interval and clears old optional tables/workspace. `base.reset(max_value, block)` rebuilds its prime list. The types use ordinary vectors: logical contents are reset, while high-water capacities can remain allocated. Copying creates an independent snapshot; moving follows standard vector semantics. Public metadata and vectors are algorithm state and must not be edited behind the APIs.

## Prime modes and costs

Every block represents at most `block` consecutive integers, and `block` must be positive. Each mode handles the last partial block and arbitrary boundary alignment.

| Mode | Candidate/marking behavior | Working candidate storage |
|---|---|---|
| `Plain` | All integers from 2; ordinary marking from `max(p*p, ceil(L/p)*p)`. | At most `block` bytes. |
| `Odd` (default) | Emits 2 separately, stores odd candidates and crosses odd multiples using stride `2*p`. | At most `ceil(block/2)` bytes. |
| `Wheel30` | Emits 2, 3, 5 separately; keeps candidates coprime to 30, and marks `p*k` only for wheel cofactors `k` coprime to 30. | At most `block` bytes; this is a wheel traversal, **not packed wheel storage**. |

Wheel residues are `1, 7, 11, 13, 17, 19, 23, 29` modulo 30, with cyclic gaps `6, 4, 2, 4, 2, 4, 6, 2`. For `p >= 7`, a candidate multiple of `p` survives the small-prime exclusions exactly when its cofactor is also coprime to 30. This justifies skipping all other cofactors. The plain mode provides the compact conventional alternative and can outperform the other modes when per-prime initialization dominates.

Let `s = floor(sqrt(max_value))`, `B = block`, and `n` be the positive part of an interval's length. Base setup uses a tiny dense seed through `sqrt(s)` and a blocked odd sieve through `s`: time `O(s*log(log(s+3)) + ceil(s/B)*pi(sqrt(s)))`, stored memory `O(pi(s))`, workspace `O(B + sqrt(s))`. It does not allocate a dense array through `s`. Streaming takes `O(n*log(log(max(3,R))) + ceil(n/B)*pi(sqrt(max(2,R))))` time and `O(B)` workspace, excluding callbacks/output. The per-block scan of the base primes is material: arbitrarily small blocks do not preserve the familiar sieve bound without that term.

Materialization adds `O(r-l)` storage and initialization plus the copied bases and returned primes. Full signed endpoint arithmetic is supported, subject to vector size and available-memory limits. Near `INT64_MAX`, storing every base prime alone can require about **1.2 GB** and substantial setup time; streaming bounds candidate storage but does not eliminate the base-prime requirement. Narrow isolated queries at such heights may need the separately owned primality/factorization family instead. Tests exercise ordinary high intervals through approximately `10^12` and directly verify endpoint arithmetic through `INT64_MAX`; they do not claim a full sieve through the maximum signed endpoint was run.

## Factorization and tables

The following number-theory operations concern **positive integers**, not absolute values of signed integers:

- `forEachFactor(callback)` visits `(offset, prime, exponent)` for every distinct prime factor of each positive value greater than one, in increasing prime order for each offset. Nonpositive values and one emit no factor. `rem` stores the per-entry residual after small-prime division; final residual primes are reported without overwriting that legacy workspace convention.
- `getPrimeFac()` returns sorted factor lists for the whole interval; it requires a positive interval (`l >= 1`) or an empty interval. One has an empty factorization. `getPrimeFac(x)` requires `1 <= x` and `l <= x < r` and returns one sorted factorization.
- `getNumDiv`, `getSumDiv`, `getPhi`, and `getMu` populate only the named table plus the shared residual workspace. Repeated calls are valid and replace that table. `getTables()` computes all four in one factor traversal when all are needed.
- For table offsets representing `x <= 0`, all four values are **zero sentinels outside the positive domain**. At `x = 1`, all four are one. No mathematical claim about the divisors of zero or the factorization of negative integers is implied.

`num_div` and `phi` use `lng`, `mu` uses signed bytes, and `sum_div` is deliberately widened from `vector<lng>` to **`vector<lll>`**. A positive signed-64-bit integer can have a divisor sum exceeding `INT64_MAX`. The recurrence `1 + p + ... + p^e` and products use signed 128-bit arithmetic throughout. For `x <= INT64_MAX`, `sigma(x) <= x*(x+1)/2 < 2^125`, so all intermediate positive partial products fit; `tau(x) <= x` and `phi(x) <= x` fit `lng`.

The interval factor engine initializes residuals once, divides multiples of each base prime by its full prime power, and reports each remaining factor greater than one as prime. A composite residual would have had a factor at most its square root, hence at most `sqrt(r-1)`, already removed. This proves completeness, including prime squares at interval boundaries. Individual and combined table traversals have a conservative bound `O((r-l)*log(max(2,r)) + pi(sqrt(max(2,r))))`, with `O(r-l)` residual/table storage. Returned factor lists add output-sized storage. Single-value factorization stops as its shrinking residual becomes smaller than `p*p`.

All interval-start calculations use positive quotient/remainder arithmetic and a widened final product. A computed first multiple may exceed `INT64_MAX` but is compared with the endpoint before narrowing. Block ends are formed in 128 bits. Marking loops check the distance to the end before adding a stride. A corrected floating square-root estimate is validated using exact 128-bit squares. These details remove the archive's `L + p - 1`, `R + 1`, signed iteration, and divisor-sum overflows.

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

Quick uses endpoints `[-4,24]`, three block sizes and 50 random intervals. Full uses the bounds above. Stress extends exhaustive endpoints to `[-8,96]` and uses 3000 random intervals. High and boundary regressions run in every mode.

Final verification, GCC 16.2.1, GNU++20, seed `20260927`:

```
python3 '96-Local Testing/05-Mathematics/06-segmentedsieve_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/05-Mathematics/06-segmentedsieve_tester.py' --mode full --seed 20260927 --configuration ASan-UBSan
```

Optimized `-O2 -DNDEBUG` and checked `-O0 -g -D_GLIBCXX_DEBUG` each passed **2,392,184 checks**; checked passed all **16 assertion probes**. The first sandbox sanitizer launch hit LeakSanitizer's ptrace incompatibility. The approved unrestricted ASan/UBSan rerun passed the same **2,392,184 checks** with leak detection enabled. Quick had also passed both configurations. Stress mode is available but not part of this completion claim. Package integration separately owns standalone/aggregate/multiple-translation-unit compilation and consistency checks.

## Performance evidence

The reproducible driver and machine/compiler/seed/flags/warmup/repetition record are [90-foundations_benchmark.py](<../96-Local Testing/05-Mathematics/90-foundations_benchmark.py>) and [90-foundations_benchmark.json](<../96-Local Testing/05-Mathematics/90-foundations_benchmark.json>). It verifies variant outputs outside timed regions, measures base construction separately, and uses repeated interval workloads of lengths 32, 50000 and 2000000 at low and high endpoints.

The final recorded high interval near `10^12`, length 2000000, had medians around 39.16/56.50/85.62 ms for Plain/Odd/Wheel30 with the default block span 32768, versus 21.58/13.80/21.23 ms with span 262144. This is the expected per-block/base-scan tradeoff, not a uniform mode ranking. Low intervals favor the odd mode; small blocks at high endpoints can be much slower. The default remains the compact conventional odd implementation, with both alternative modes and block size explicitly selectable. These are host measurements, not universal timing gates; no Barrett/Montgomery or ISA-specific specialization was introduced.

No owned segmented-sieve feature remains incomplete within these explicit domains.
