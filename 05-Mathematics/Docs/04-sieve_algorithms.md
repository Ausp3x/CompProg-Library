# 04-sieve_algorithms.hpp — evidence

`04-sieve_algorithms.hpp` covers full-prefix sieves: Eratosthenes (byte flags, odd-only bits, and an SPF table), the linear sieve, smallest and largest prime-factor tables, positive factor enumeration, and four multiplicative-function tables. Interval sieving belongs to `06-segmentedsieve.hpp`, and general multiplicative transforms to `08-multiplicative_functions.hpp`.

## Contracts

Every struct keeps the inclusive table convention `[0, n]`: the argument is a maximum value, not a table length. `SieveOfErath`, `SieveOfErathSpf` and `LinearSieve` assert `0 <= n < INT_MAX`, so that `n + 1` and loop sentinels stay representable as `int`; `OddBitSieve` asserts `0 <= n < 2^32` and takes `lng` arguments. Prime lists are ascending.

The public vectors and `n` are query data, not inputs. Callers must not modify them and then call methods that rely on the sieve invariant. Copies own independent vectors, and a moved-from object can be restored with `reset`. There is no global cache and no dynamic-modulus state.

`reset(n)` discards old results but may keep vector capacity. Allocated storage is therefore `O(peak_n)`, where `peak_n` is the largest `n` the object has used (just `n` for a fresh object). Construct a fresh object when releasing that capacity matters.

Overflow: index products are checked in `lng` before the `int` product is formed, and Eratosthenes counters are `lng`. Divisors, counts, totients and prime powers are at most `n`. A divisor sum is below `n * (n + 1) / 2 < 2^61`, and the geometric prime-power sum is at most `2 * n`.

### SieveOfErath

`SieveOfErath(n)` and `reset(n)` build `is_prime[0..n]` and `prms` in `O(n * log(log(3 + n)))` time. Zero and one are not prime. `primeCount(x) const` returns pi(x), the number of primes `<= x`, for any `x <= n` (asserted), 0 for every `x < 2` including negatives, by `upper_bound` on `prms` in `O(log(n))`. Ignoring capacity rounding, the state is `n + 1` bytes of flags plus `4 * pi(n)` bytes of primes.

Correctness: every composite has a prime factor at most its square root. Even composites are cleared first. The odd primes up to `r = floor(sqrt(n))` (`int(std::sqrt(double(n)))`, exact below `2^52` since the square root is correctly rounded) come from the table's own prefix `[0, r]`, sieved first in place by the odd primes up to `sqrt(r)` in `O(sqrt(n) * log(log(n)))`; for `r < 3` there are none. Then the odd multiples of each such prime `p` are struck from `p * p`, one 128 KB segment of the table at a time: each prime keeps its next multiple across segments, and a segment `[lo, hi)` visits only primes with `p * p < hi`, which ascend with `p`. Each composite is struck exactly as in the unsegmented sieve and no prime ever is, since smaller multiples already have a smaller prime factor. Each prime's step `2p <= 2 * 46341` is below the segment size, so every active prime pays at least its one strike per segment, and the work stays `O(n * log(log(3 + n)))` (cycle counts against the unsegmented sieve under [Benchmarks](#benchmarks)).

### OddBitSieve

`OddBitSieve(n)` and `reset(n)` build bit `k` of `bits` for the odd number `2k + 1 <= n` (set when prime; bits past `n` in the last word stay set and are never read, since `primeCount` masks its last word and `block_count` entries cover only whole earlier words), and `block_count[b]`, the prime bits in words `[0, 8b)`, for `0 <= n < 2^32` (asserted), in `O(n * log(log(3 + n)))` time (user decision 2026-11-14, beside the byte table). The base primes up to `sqrt(n) < 2^16` come from `SieveOfErath(int(std::sqrt(double(n))))`; each one strikes its odd multiples from `p * p` in segments of `2^18` bits (32 KB) as above, its step `p < 2^18` again below the segment size. `isPrime(x)` asserts `0 <= x <= n` and reads one bit (2 is the one even prime) in `O(1)`. `primeCount(x)` asserts `x <= n` and returns pi(x), 0 for every `x < 2` including negatives, in `O(1)`: one block count plus at most 8 popcounts. The state is `8 * ceil(ceil(n / 2) / 64)` bytes of words plus 4 bytes per 8 words, about `n / 16 + n / 256` bytes; there is no prime list. At `n = 2^32 - 1` that is 272 MB, so a 256 MB judge fits `n` up to about `3.8e9`, where the byte table with its prime list stops near `2.2e8`. Copies own their vectors; `reset` may keep capacity.

### SieveOfErathSpf

`SieveOfErathSpf(n)` and `reset(n)` build `spf[0..n]` and `prms` with `LinearSieve`'s conventions (`spf[0] = spf[1] = -1`, `spf[p] = p`) by Eratosthenes, in `O(n * log(log(3 + n)))` time and `4 * (n + 1)` bytes plus the prime list (user decision 2026-11-14). Even entries get 2 first; each odd prime `p <= sqrt(n)`, from `SieveOfErath`, then writes itself into the still unset odd multiples from `p * p`, in increasing order per 128 KB segment, so the first write to a composite is its smallest prime factor; the odd entries left unset are the primes. A step `2p` can exceed the segment of `2^15` entries, so a segment may visit an active prime without writing; those visits number at most `(n / 2^15) * pi(sqrt(n)) <= 0.15 * n`, within the bound. It has no factorization or multiplicative tables: those stay with `LinearSieve`, which is linear in the worst case, while this table builds in fewer cycles from `n = 1000` to `5e4` and from `1e7` up, and at most 6.3% more between (see [Benchmarks](#benchmarks)).

### LinearSieve

`LinearSieve(n)` and `reset(n)` build `spf[0..n]` and `prms` in `O(n)` time. `spf[0] = spf[1] = -1` where those indices exist, and `spf[p] = p` for a prime `p`. The optional tables `lpf`, `num_div`, `sum_div`, `phi` and `mu` stay empty until their `get...()` call, and `reset` clears them. Each builder takes `O(n)` time, may be called repeatedly in any order, and recomputes only its own table.

- `lpfTable()` builds the largest prime factor, with `lpf[0] = lpf[1] = -1`.
- `divisorCountTable()` builds `num_div[a] = tau(a)`, the number of divisors, using `O(n)` temporary integers for the smallest-prime exponent.
- `divisorSumTable()` builds `sum_div[a] = sigma(a)` as `lng`, using `O(n)` temporary `lng` values for `1 + p + ... + p^e`.
- `phiTable()` and `mobiusTable()` build Euler's totient and the Möbius function. `mu` is `int8_t`.
- In all four arithmetic tables, entry 0 is a convenience 0 (it says nothing about divisors of zero) and entry 1 is 1.
- `primeCount(x) const` has the `SieveOfErath` contract on the same `prms`.
- `factorize(a) const` asserts `1 <= a <= n` and returns ascending `(prime, exponent)` pairs, empty for 1, in `O(log(a))`.
- `divisors(a) const` has the same domain and returns every positive divisor once, in unspecified order, with `{1}` for 1. It takes `O(log(a) + out)` time and `O(out)` returned storage.

Fresh linear state uses `4 * (n + 1)` bytes of SPF plus the prime list. LPF, divisor counts and totients each add `4 * (n + 1)` bytes, divisor sums `8 * (n + 1)`, and Möbius `n + 1`. Building every table therefore takes `25 * (n + 1) + 4 * pi(n)` logical bytes, plus up to `8 * (n + 1)` temporary bytes during `divisorSumTable`.

Correctness: every composite `x` has a unique decomposition `x = p * i` with `p = spf[x] <= spf[i]`; iterating primes only up to `spf[i]` builds each once, which sets the correct SPF in linear time, and removing the SPF repeatedly yields the factorization. `lpf[i] = max(spf[i], lpf[i / spf[i]])`, with the sentinel at 1 as the empty maximum. `divisors` multiplies each existing divisor by each power of a new prime, so unique factorization gives every divisor exactly once. For the multiplicative tables write `i = p * j` with `p = spf[i]`. If `p` divides `j`, only that exponent grows: the divisor count replaces `e + 1` by `e + 2`, the divisor sum replaces `1 + ... + p^e` by `1 + ... + p^(e + 1)`, the totient multiplies by `p`, and Möbius becomes 0. Otherwise a new coprime prime appears: the count doubles, the sum multiplies by `1 + p`, the totient by `p - 1`, and Möbius changes sign. Induction from entry 1 proves each table, and the recurrences use exact divisions.

### Legacy mapping

- P276 rename, 2026-11-10: `getLpf` → `lpfTable`; `getNumDiv` → `divisorCountTable`; `getSumDiv` → `divisorSumTable`; `getPhi` → `phiTable`; `getMu` → `mobiusTable`; `getPrimeFac` → `factorize`; `getAllFac` → `divisors`.

## Feature-to-test map

Entry: [`04-sieve_algorithms_tester.py`](<../../96-Local Testing/05-Mathematics/04-sieve_algorithms_tester.py>). Its exact integer oracles do not share the SPF recurrence.

| Feature | Oracle and edge classes |
|---|---|
| `SieveOfErath` (`is_prime`, `prms`), `LinearSieve` (`spf`, `prms`, `lpfTable`) | Trial division for every integer through 50,000 and every prefix through 128. The known prime count at one million, and every flag of the byte table against `LinearSieve`'s `spf[x] == x` there. Random larger trial factorizations. Zero, one, prime endpoints and prime squares. |
| Small tables and segment edges (`SieveOfErath`, `OddBitSieve`, `SieveOfErathSpf`) | For every `n` from 0 through 128 (32 in quick) and `n` in {32767, 32768, 32769, 131071, 131072, 131073, 262145, 524287, 524288, 524289, 524290} (quick: 131073), every value against trial division: byte and bit flags, the bit sieve's count at every `x` from -2, the byte count every 499th value and at the end, the SPF entry against the first trial divisor, and both prime lists. |
| `OddBitSieve` (`isPrime`, `primeCount`) | Every prefix 0..128 and the exhaustive table, every flag against `LinearSieve` at one million with 200 random counts against the prime list, and the known pi at `10^7`, `10^9` and `2^32 - 1` (quick/full/stress: 664,579, 50,847,534 and 203,280,221) with a 3000-value trial-division window below that top, counts included. |
| `SieveOfErathSpf` (`spf`, `prms`) | The small tables and segment edges above, and equality with `LinearSieve`'s table and list at one million (5 million in stress). |
| `primeCount` (both structs) | A running trial-division count at every `x` from -2 through `n` for every prefix 0..128 and the exhaustive table (50,000 in full); the known pi at the large bound and one below it; 200 random points against a linear scan of `prms`. Probes for `x = n + 1`. |
| `factorize`, `divisors` | Trial factorization, and a separate square-root divisor enumeration sorted to check set equality and duplicates. Covers prime powers and squarefree products. |
| `divisorCountTable`, `divisorSumTable` | An independent divisor-multiple enumeration. |
| `phiTable`, `mobiusTable` | Inversion of `sum(phi(d)) = n` and `sum(mu(d)) = [n = 1]` over divisors. Totients also by gcd counting through 300, Möbius also by trial-factor exponents. |
| `reset`, state and lifetime | Randomized builder order, repeated calls, lazy tables, copy and copy assignment, move construction, reset after a move, grow and shrink resets, for all four structs. |
| Preconditions | 28 checked-build probes: negative and upper-bound constructor and reset arguments on all four structs (`2^32` and `2^32 + 1` for `OddBitSieve`), out-of-range factor queries, `primeCount` above `n` on the three structs that have it, and `isPrime` at -1 and `n + 1`. |

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 748 MB; optimized 0.3 s compile + 0.5 s run, checked 12.3 s compile + 1.5 s run, ASan-UBSan 12.2 s compile + 2.4 s run; MEMORY peak 885 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 792 MB; optimized 0.2 s compile + 0.5 s run, checked 20.4 s compile + 2.6 s run, ASan-UBSan 17.5 s compile + 3.9 s run; MEMORY peak 902 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 19.1 s compile + 0.0 s run, checked 18.8 s compile + 0.4 s run; MEMORY peak 745 MB. `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` PASS: pre-flight quick peak 751 MB; optimized 0.1 s compile + 0.8 s run, checked 5.0 s compile + 3.0 s run, ASan-UBSan 5.4 s compile + 6.2 s run; MEMORY peak 731 MB. Dated rows below stay those commands' latest results.

P012 MA02 run, 2026-11-14 (experiment `none`): GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`), GNU++20, CPython 3.14, Linux x86-64, Intel Core i9-11900H. Every suite runs the shared runner's optimized `-O2 -DNDEBUG`, checked and ASan/UBSan configurations.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/05-Mathematics/04-sieve_algorithms_tester.py' --mode full --seed 1` | PASS, 3 configurations, 10,199,724 C++ checks each, 28 assertion probes; quick pre-flight peak 48 MB (cached builds; 819 MB with compiles, as in the g++-14 run), checked full run 26.7 s; `MEMORY peak=901MB` |
| same with `CXX=g++-14` | PASS, same counts; pre-flight quick peak 819 MB, `MEMORY peak=986MB` |
| `... --mode stress --seed 1` | PASS, 16,231,224 checks (exhaustive trial table at 250,000, linear comparison at 5,000,000, the bit sieve at `2^32 - 1`); checked build 252 s of its 720 s limit, beside a mutation campaign; pre-flight quick peak 49 MB (cached builds), `MEMORY peak=1048MB` |
| `python3 '96-Local Testing/05-extraction.py' ops --filter 05-Mathematics/04-sieve_algorithms.hpp` | PASS, 21 operations in 15 extractions on g++ and g++-14 |
| `python3 '96-Local Testing/05-extraction.py' testers --filter 04-sieve_algorithms_tester` | PASS, 4 names |
| `python3 '96-Local Testing/02-integration.py'` | PASS, 190 standalone and aggregate headers, scalar and AVX2 multi-TU builds, workspace; `MEMORY peak=2718MB` |

## Benchmarks

Conditions (2026-11-14, P012 MA02 `/finish`): Intel Core i9-11900H (48 KB L1d, 1.25 MB L2 per core), GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, Linux x86-64, otherwise idle. No Barrett/Montgomery or ISA-specific code is used.

Cycle and instruction counts (`perf stat -r 3`, user space, one binary built with `-falign-functions=64 -falign-loops=32`, construction repeated so each run takes about 0.3 to 2 s, the prime count summed so the work stays) compare the segmented `SieveOfErath` with the previous unsegmented header, which struck all odd multiples across the whole table:

| n | repetitions | previous cycles | segmented cycles | change | previous instructions | segmented instructions |
|---|---|---|---|---|---|---|
| 1,000 | 200,000 | 607M | 643M | +6.0% | 2.57G | 2.76G |
| 10,000 | 30,000 | 808M | 727M | −10% | 3.66G | 3.37G |
| 100,000 | 3,000 | 1,919M | 1,783M | −7.1% | 3.77G | 3.32G |
| 500,000 | 600 | 2,140M | 2,000M | −6.5% | 3.88G | 3.37G |
| 2,000,000 | 150 | 2,480M | 2,032M | −18% | 3.97G | 3.42G |
| 100,000,000 | 3 | 6,586M | 1,961M | −70% | 4.23G | 3.63G |

At `n = 1000` the base-prime vectors cost about 180 cycles per construction, under 0.2 µs. A recursive base sieve (`SieveOfErath(r)`) was measured first and cost 823M cycles there (+36%); sieving the table's own prefix replaced it. Wall clock at `1e9`, two runs each: 1.43 and 1.57 s segmented against 6.35 and 6.51 s before.

The same harness compares `SieveOfErathSpf` with `LinearSieve`'s constructor (`perf stat -r 5`), both building `spf` and `prms`:

| n | repetitions | `LinearSieve` cycles | `SieveOfErathSpf` cycles | change |
|---|---|---|---|---|
| 32 | 3,000,000 | 1,348M | 2,209M | +64% |
| 1,000 | 300,000 | 1,875M | 1,218M | −35% |
| 50,000 | 6,000 | 2,802M | 2,162M | −23% |
| 2,000,000 | 150 | 2,922M | 2,975M | +1.8% |
| 5,000,000 | 60 | 3,362M | 3,573M | +6.3% |
| 10,000,000 | 30 | 4,181M | 3,840M | −8.2% |
| 100,000,000 | 3 | 5,274M | 4,005M | −24% |

`SieveOfErathSpf` executes about half the instructions at every size from 50,000 (3.5–3.9G against 7.7–7.8G); between 2e6 and 5e6 its scattered 4-byte writes leave it at most 6.3% behind the linear sieve. Its segment of `2^15` ints (128 KB) stays: `2^16`, `2^17` and `2^18` ints took 3–4%, 6–7% and 8–9% fewer cycles at 2e6 and 5e6 on this machine, and 1–3% at 1e8, but the larger two fill or exceed the 512 KB L2 of the judges' Zen 3 cores, and such cache tuning stays judge-unverified (`05-testing.md`, Benchmarks).

The [benchmark driver](<../../96-Local Testing/05-Mathematics/00-foundations_benchmark.py>) with its [C++ workload](<../../96-Local Testing/05-Mathematics/00-foundations_benchmark.cpp>) writes `96-Local Testing/05-Mathematics/00-foundations_benchmark.json`: one warmup and five samples per workload, deterministic inputs, prime lists checked against an independent packed-bit sieve outside the timed region, the constructor timed with its allocation. Medians in ms:

| n | packed-bit reference | `SieveOfErath` | `OddBitSieve` | `SieveOfErathSpf` | `LinearSieve` |
|---|---|---|---|---|---|
| 50,000 | 0.124 | 0.063 | 0.025 | 0.079 | 0.114 |
| 2,000,000 | 5.46 | 3.36 | 1.29 | 4.57 | 4.86 |
| 5,000,000 | 14.7 | 7.09 | 3.48 | 14.4 | 13.9 |
| 10,000,000 | 26.5 | 12.9 | 6.35 | 28.1 | 32.8 |
| 100,000,000 | 320 | 142 | 70.5 | 301 | 408 |

`OddBitSieve` builds its table and block counts in 38–50% of the time of `SieveOfErath` at every size from 50,000, in a sixteenth of the memory, without a prime list; at `1e9` it took 0.75 s in both wall-clock runs above. The run also records the streaming and materialized segmented-sieve workloads of `06-segmentedsieve.hpp`, whose evidence owns them.

## Mutation testing

Record: mewt 4.0.0 on blob `6e691b59372d7c7d3c82639d5c3d0c4d39253e27` (2026-11-14, seed 1, 1 lines changed since blob `e079d169563f`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

The changed line is the `SieveOfErathSpf` comment, whose speed claim the benchmarks corrected; it yields no mutants. The survivors below come from the campaign before it, which keeps their line numbers: mewt 4.0.0 on blob `e079d169563f8152cd507af55f9d833831a07c1e` (2026-11-14, seed 1, 74 lines changed since blob `abfb64b65c0e`): 528 mutants, 465 caught (35 by timeout), 63 survived, 0 untested. That campaign covered the 74 lines changed since blob `abfb64b65c0e` by the MA02 `/finish` rewrite: the segmented byte sieve with its in-place prefix, `OddBitSieve` and `SieveOfErathSpf`. Stage seconds (both runs together, as the run reports them): quick 1351, full on the optimized build 92, full on the checked and ASan-UBSan builds 1516. The first run left 64 survivors. The small-prefix variant oracle (`variantTables` for every `n` through 128, each value against trial division) closed `AOS 289` (line 43, `(n + 1) / 2` → `(n + 1) - 2`, which leaves `OddBitSieve(1)` without a word); the rerun with the same workdir retested the survivors with it. Of the 63 it left, the final review closed 4 more with storage checks in `variantTables` (the word count, the set bits past `n` and the block prefix counts the contract states): `AOS 288` and `AOS 290` (line 43, `(n + 1) / 2` → `(n + 1) + 2` or `* 2`) and `AOS 336` and `AOS 338` (line 57, `block_count` sized `(words + 7) + 9` or `(words + 7) * 8 + 1`). Each was rebuilt by hand and fails the tester's full mode at `n = 0` (`bit sieve storage sizes`); the campaign workdir was already deleted, so the record above still counts them. Of the 59 left, 12 change only the running time (runtime only, `05-testing.md`, Mutation testing, user decision 2026-11-14) and 47 are equivalent.

The runtime-only group shares one argument. Each of these mutants leaves odd composites up to `r = floor(sqrt(n))` unmarked in the prefix, or admits them as base values, and a composite base value `c` only restrikes multiples of its own prime factors, so every table is identical. The striking work grows from the sum of `n / (2p)` over odd primes `p <= r` to at most the same sum over every odd `c <= r`, about `(n / 8) * ln(n)`, so `O(n * log(n))` against the stated `O(n * log(log(n)))`. With every odd value in the base, the worst member, the strike counts are 1.75, 2.07 and 2.28 times the original's at `n = 10^6`, `10^8` and `2^31 - 2`, and the total loop work 1.22, 1.35 and 1.44 times (counted exactly, with the table fill, even clearing and prime collection). The cycle counts (`perf stat`, `-O2`) are 1.29 and 1.53 times at `10^6` and `10^8`. All are below the 3-times timeout, and no fixed timing gate is allowed.

| Mutant | Line | Why it survives |
|---|---|---|
| `CR 98` (prefix loop removed), `IF 178` (`is_prime[p]` → `false`), `BL 246` (strike writes `true`), `AOS 256`, `AOS 257`, `AOS 259` (strikes start at `2p` or 0, so only even entries), `COS 676` (`p * p == root`), `COS 681` (`k == root`), `COS 684` (`k > root`) | 17 | runtime only: the prefix stays unsieved except at most `root` itself, so every odd value up to `r` enters the base |
| `AAOS 565`, `p += 2` → `p *= 2` | 17 | runtime only: only 3 sieves the prefix (6, 12, … are even), so odd composites prime to 3 enter the base |
| `AAOS 569`, `k += 2 * p` → `k *= 2 * p` | 17 | runtime only: each prime strikes only its square (the next value is even), so other odd composites enter the base |
| `IT 200`, base filter `is_prime[p]` → `true` | 19 | runtime only: every odd value up to `r` enters the base |
| `IT 199`, prefix filter `is_prime[p]` → `true` | 17 | equivalent: composites also sieve the prefix, marking only what their factors mark; the extra work is `O(sqrt(n) * log(n))`, inside the bound |
| `AOS 252`, `p * p <= root` → `p + p <= root` | 17 | equivalent: the outer loop runs on to `r / 2`, but for `p * p > r` the inner loop marks nothing; `O(sqrt(n))` extra steps |
| `COS 678`, `p * p <= root` → `<`; `COS 683`, `k <= root` → `<` | 17 | equivalent: they miss only the value `root` itself, so at most one composite `c = root` enters the base and adds `n / (2c) <= sqrt(n)` strikes |
| `COS 693`, `COS 823`, `lo <= n` → `<` | 22, 86 | equivalent: the skipped last segment is `[n, n + 1)` only when `n` is a multiple of the segment size, so even, already cleared or set to 2 |
| `COS 743`, `lo < odd` → `<=` | 51 | equivalent: the extra segment `[odd, odd)` is empty |
| `AS 222`, `AS 226`, `AS 228`, `AS 230`, `min` operands swapped | 23, 52, 60, 87 | equivalent: symmetric |
| `AOS 276` to `AOS 279`, `AOS 323`, `AOS 327` to `AOS 331`, `AOS 412` to `AOS 415`, the active-prime test `p * p < hi` (or its bit form `(p * p - 1) / 2 < hi`) replaced by `p + p`, `p - p`, `p / p`, `p % p` or a remainder | 24, 53, 88 | equivalent: a weaker test visits primes whose next multiple is at or past `hi`, whose inner loop then marks nothing; at most one visit per base prime per segment, `O(n)` in all |
| `COS 702`, `COS 752`, `COS 832`, `< hi` → `!= hi`; `COS 703`, `COS 753`, `COS 833`, `< hi` → `<= hi` | 24, 53, 88 | equivalent: the test ascends with `p`, and a prime whose first multiple equals `hi` marks nothing in that segment, so stopping at it or visiting it changes nothing |
| `COS 697`, `COS 747`, `COS 767`, `COS 797`, `COS 827`, `<` → `!=` in a loop | 24, 53, 60, 69, 88 | equivalent: the index steps by 1 from a start at most the bound |
| `AOS 325`, `AOS 326`, `(p * p - 1) / 2` → `(p * p * 1) / 2` or `(p * p / 1) / 2`; `AOS 309`, `AOS 310`, the same in the next-index setup | 53, 48 | equivalent: `p * p` is odd, so `p * p / 2 == (p * p - 1) / 2` |
| `COS 737`, `COS 817`, `p > 2` → `!= 2`; `IT 206`, `COS 820`, `p > 2` → `true` or `>= 2` | 48, 83 | equivalent: the list holds primes, so `!= 2` is `> 2`; in `SieveOfErathSpf` a base 2 strikes only even entries, all already 2, `n / 4` extra steps |
| `COS 807`, `COS 343`, `COS 388`, `N < INT_MAX` → `!=` | 79, 11, 104 | equivalent: `N` is an `int`, so no value exceeds `INT_MAX` |
| `COS 843`, `COS 858`, `COS 399`, `spf[...] == -1` → `<=` | 90, 93, 108 | equivalent: `spf` holds -1 or a prime |

The survivors below are those of the whole-file campaign on blob `681917571ef2e0ca4b76f0e9d20063fd5b85d173` (2026-11-14, seed 1, uniform sample of 519): 500 mutants, 482 caught (10 by timeout), 18 survived, 0 untested; stage seconds quick 700, full on the optimized build 23, full on the checked and ASan-UBSan builds 319. Their lines are unchanged since and are numbered as in the current header; `COS 343`, `COS 388` and `COS 399` are in the table above, and the rewrite removed the code of `AOS 153` and `IF 102`. `LC 143` is runtime only under the rule above, and `COS 493` follows the segment tree's `COS 411` (user decision 2026-11-13).

| Mutant | Line | Why it survives |
|---|---|---|
| `LC 143`, `if (p == spf[i]) { break; }` → `continue;` | 112 | runtime only: iteration `i` also writes `spf[i * p] = p` for primes `p > spf[i]`, but the later iteration `i * p / spf[i]` (at most `n`, with no smaller factor than `spf[i]`) overwrites it with `spf[i]`; the last writer of each cell is `x / spf(x)`, which runs before `x` is read, so `spf` and `prms` are identical; the work grows from O(n) to Σ n/p = O(n log log n): the inner loop runs 2.30, 2.45 and 2.57 times as often at `10^6`, `10^7` and `10^8` (counted), and the build takes 1.94 and 2.92 times the cycles at `10^6` and `10^8` (`perf stat`, `-O2`); the tester's largest `LinearSieve` is `10^6` in full mode and `5·10^6` in stress, and the whole stage stays under its 3-times timeout |
| `COS 410`, `COS 411`, `p == spf[i]` → `>` / `>=` | 112 | equivalent: primes ascend and `spf[i]` is already in `prms`, so `>=` first holds at `p == spf[i]`; `>` breaks one prime later after one wrong write, which the same later overwrite corrects, O(n) extra in all |
| `AS 134`, `max` operands swapped | 116 | equivalent: symmetric |
| `COS 429`, `COS 444`, `COS 459`, `COS 474`, `j % p == 0` → `<=` | 122, 131, 140, 146 | equivalent: `j >= 1` and `p >= 2`, so the remainder is nonnegative |
| `COS 493`, `while (a > 1)` → `a != 1` | 154 | untestable (as the segment tree's `COS 411`, user decision 2026-11-13): equal for every `a >= 1`, and `a` stays at least 1; they differ only for `a <= 0` with the assert compiled out, where the mutant pushes `(-1, 1)` forever at `a = 0` (or reads `spf[a]` out of bounds below it), an input `05-testing.md` (Memory safety) keeps from the `NDEBUG` build |
| `COS 498`, `COS 501`, `a > 1` → `!=` / `>=` in the inner loop | 156 | equivalent: a division by its own prime factor leaves `a >= 1`, `spf[1] = -1` never equals `p`, and with the assert compiled out an `a <= 0` never enters the outer loop |
| `COS 504`, `spf[a] == p` → `<=` | 156 | equivalent: `p` was the smallest prime factor, so the quotient's is at least `p` |
| `COS 513`, `i < len` → `!=` | 167 | equivalent: `i` steps by 1 from 0 and `len >= 1` |

## Sources

Inspected on 2026-09-27; written independently with no code copied.

- [cp-algorithms, Sieve of Eratosthenes](https://cp-algorithms.com/algebra/sieve-of-eratosthenes.html): marking from prime squares, odd-only work, and memory tradeoffs.
- [cp-algorithms, Linear Sieve](https://cp-algorithms.com/algebra/prime-sieve-linear.html): the unique `spf[x] * i` construction and linear work.
- [OI Wiki, 筛法](https://oi-wiki.org/math/number-theory/sieve/): SPF-based recurrences for totient, Möbius, divisor count and divisor sum.
- MA02, 2026-11-14 (`@researcher`): hitonanode `bs_sieve.hpp` `prime_counting`, ei1333 `prime-count.hpp`, suisen `linear_sieve.hpp` `prime_num` and Library Checker `enumerate_primes`, for the `primeCount` convention (primes `<= x`, asserted bound, 0 below 2); URLs in [00-sources.md](00-sources.md).
- MA02 `/finish`, 2026-11-14: cp-algorithms (bit and segmented sieves), maspypy `lpf_table.hpp` and `prime_table.hpp`, suisen `sieve_of_eratosthenes.hpp`, Nyaan `prime-enumerate.hpp` and PyRival `sieve.py`, for `OddBitSieve`, `SieveOfErathSpf` and the segments; URLs in [00-sources.md](00-sources.md).
- [KACTL, `Eratosthenes.h`](https://github.com/kth-competitive-programming/kactl/blob/main/content/number-theory/Eratosthenes.h), CC0: independent comparison of odd marking and prime-list construction.

## Limits and handoffs

- `OLD/5-Mathematics/04-sieves.hpp` is unchanged, and every ordinary-sieve legacy operation is kept.
- Bit-packed storage is `OddBitSieve`, beside the byte `SieveOfErath`, whose public `is_prime` field stays (user decision 2026-11-14); wheel storage is left out ([00-notes.md](00-notes.md), P012 MA02). Sublinear prime counting is row 31.
- The 2026-10-07 completeness sweep left out three candidates (reasons in [00-notes.md](00-notes.md)); its fourth, prime counting, returned as `primeCount` (P275, implemented in MA02): factoring values above `n` is owned by `07` `factorize`; the completely multiplicative `i^K` table is handed to `08` `multiplicativeTable`; an lcm of a list is caller-side work on top of `factorize`.

## Aliases

Renamed aliases (P276, 2026-11-10): `getLpf`, `getNumDiv`, `getSumDiv`, `getPhi`, `getMu`, `getPrimeFac`, `getAllFac`.

Search aliases (P263, grab --find): sieve of Eratosthenes, linear sieve; bit sieve, bitset sieve, odd-only sieve, segmented sieve of Eratosthenes, `lpf_table` (maspypy), `Sieve` (suisen), smallest prime factor table; Library Checker `enumerate_primes`; prime counting, primePi, `prime_counting` (hitonanode), `prime_num` (suisen), 素数计数函数; 欧拉函数, 筛法, 莫比乌斯反演, 线性筛, 埃氏筛, 欧拉筛, 素数筛, 质数筛, 埃拉托斯特尼筛法, 最小质因子, 莫比乌斯函数.

## History

- 2026-09-27: initial P012 evidence; full mode with seed `20260927` passed on all configurations.
- 2026-10-07: P012 re-audit baseline, full suite passed on all six headers before any change.
- 2026-10-07: P012 re-audit, 2 findings and 3 brace-style issues fixed; after `@reviewer`, full, stress (seed 1) and integration passed.
- 2026-10-12: P012 prune rerun, header unchanged; full suite passed on g++ and g++-14.
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases (`grab --find`): OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-10: P276 rename (`LinearSieve` `get…` builders → `…Table`, `getPrimeFac`/`getAllFac` → `factorize`/`divisors`); full on g++ and g++-14, stress and extraction passed.
- 2026-11-14: P012 MA02 added `SieveOfErath::primeCount` and `LinearSieve::primeCount` with tests and 2 probes; full on g++ and g++-14, stress, extraction and integration passed; first mutation campaign recorded (15 equivalent, 2 runtime-only, 1 untestable survivors); independent review: no correctness defect, the `IF 102` work ratio corrected, a blank line before `primeCount` (changed-lines campaign, no mutants).
- 2026-11-14: P012 MA02 `/finish` (user decisions): added `OddBitSieve` and `SieveOfErathSpf`; `SieveOfErath` strikes per cache segment after sieving its own prefix (an optimality defect the `/finish` benchmark found; A/B cycle counts above, up to 70% fewer); 28 probes, small-table variant oracle added for mutant `AOS 289`; full on g++ and g++-14, stress, extraction and integration passed; changed-lines mutation campaign (12 runtime-only and 47 equivalent survivors; the final review's storage checks closed 4 more); the `SieveOfErathSpf` speed claim corrected to its measured ranges; final review: `OddBitSieve` storage checks, the `LC 143` ratio measured and the `SieveOfErathSpf` segment-visit bound stated; full on g++ and g++-14, stress and extraction passed again.
