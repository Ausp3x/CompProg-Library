# 55-fractional_programming.hpp — evidence

`55-fractional_programming.hpp` (batch MI34, package P072) provides Dinkelbach's method over a caller oracle with exact rational termination, and two direct applications: exactly-k 0/1 selection (Eppstein–Hirschberg after a few Dinkelbach steps) and the ratio knapsack with a denominator floor. Prerequisites: the C01 template (P002) and the MI01 `CustomHash` seed (P013). Contest profile; independent implementation, no legacy source.

## Contracts

### dinkelbach

`dinkelbach(oracle, p, q)` maximizes `N / D` over a finite family of feasible sets with `D > 0`. `oracle(p, q)` returns `{N, D}` of a feasible set maximizing `q * N - p * D` (the parametric objective `N - λD` at `λ = p / q`, scaled by `q > 0`). The start needs `q > 0` and `p / q` at most the optimum: any feasible ratio or a lower bound. With `|p|, |q|, |N|, |D| < 2^63` every comparison `q * N - p * D` is exact in `__int128`. The result is the last oracle result, an optimal pair. Minimize by negating the numerators and the returned numerator. `q > 0` is asserted on entry, and `D > 0` and `q * N - p * D >= 0` for every oracle result: the first catches a start above the optimum, the later ones a nonmaximizing oracle. The loop also stops at 256 calls, the bound below for `|N|, D < 2^63`, asserting that the last value is zero, so even a nonmaximizing oracle under `NDEBUG` costs at most 256 calls.

Correctness: let `F(λ) = max (N - λD)`, strictly decreasing because `D > 0`, with `F(λ*) = 0` at the optimum `λ*`. While `λ = p / q <= λ*`, the optimal set gives `q * N* - p * D* >= 0`, so the oracle value is nonnegative. If it is zero, no set exceeds `p / q`, and the returned argmax satisfies `q * n = p * d`, so `n / d = p / q = λ*`. Otherwise `n / d > p / q`, and the next `λ = n / d` is a feasible ratio, still at most `λ*`. Ratios strictly increase over a finite family, so the loop ends. Every step is integer arithmetic; no tolerance exists.

Iterations: let `A >= |N|` and `B >= D` over the feasible sets, and let the calls run at `λ_0, ..., λ_t`, with a positive value before `λ_t` and zero at `λ_t`. With `S_k` the answer at `λ_k`, `λ_{k+1} = N_k / D_k` gives `F(λ_k) = (λ_{k+1} - λ_k) * D_k`, and the optimality of `S_k` at `λ_k` gives `F(λ_k) - F(λ_{k+1}) >= (λ_{k+1} - λ_k) * D_{k+1}`. So `F(λ_{k+1}) / F(λ_k) + D_{k+1} / D_k <= 1`: `D` never increases, and at every step `F` or `D` at least halves. Over `k = 1 .. t - 2`, `D` halves at most `log2(B)` times. `F` halves at most `log2(F(λ_1) * B)` times, since `F(λ_{t-1}) >= 1 / B` (an integer over `q <= B`) and `F(λ_1) <= A + A * B` (`|λ_1| <= A`). Hence at most `4 + log2(A + 1) + 3 * log2(B)` oracle calls (2 when `A = 0`), about 250 for 62-bit sums: `O(log(A * B + 2) * oracle)` time, `O(1)` extra memory. The tester checks this bound for every run; the observed maximum is in the results below. An oracle result with `D <= 0` ends the loop even under `NDEBUG`.

### FractionalSelection

`{num, den, items}`: `num = sum a`, `den = sum b` over the chosen items, `items` in increasing index order. `den = 0` (with `num = 0` and no items) is the infeasible sentinel, produced by `ratioKnapsack` when `sum b < W`, and by both functions outside their domain under `NDEBUG`.

### zeroOneFractionalSelect

Exactly `k` of `n` items maximizing `sum a / sum b` (POJ 2976 keeps `n - k'` tests). Domain: `1 <= k <= n`, `b_i > 0`, `sum |a_i| < 2^62`, `sum b_i < 2^62`, all asserted together (sizes, `k`, signs and both sums) after one `__int128` pass over the common length; outside the domain under `NDEBUG` the call returns the empty `den = 0` result, because the rounds below need consistent sizes and positive `b`. Inside it every bound, crossing and set ratio has `lng` parts below `2^62` and every product, key and test value is below `2^125`. `seed` (default `CustomHash::rnd`) draws the pivots: the ratio is the optimum for every seed and only the time depends on it (Las Vegas); when several sets are optimal, the witness may differ between seeds.

Method. `F(t) = max` over `k`-sets of `sum (a - t * b)` decreases strictly and is zero exactly at the optimum `t*`. A test at `t = p / q` takes the `need` largest keys `q * a - p * b` among the live items by `std::nth_element` and adds the items already taken: `F(t) > 0` raises `lo` to that set's ratio (above `t` and at most `t*`, a Dinkelbach step), `F(t) < 0` lowers `hi` to `t`, and `F(t) = 0` returns that set. `lo` starts at the average of all items, a lower bound because `F` there is at least the average of `sum (a - t * b)` over all `k`-sets, which is zero; `hi` starts at `+inf`. Up to 8 tests at `lo` come first: plain Dinkelbach, which settles every non-adversarial benchmark input below with as many selections as the old engine needed. Then come the rounds of Eppstein and Hirschberg's randomized algorithm (their Figure 1): a pivot `i` is drawn from the live items by `splitMix64(seed++)`, and each live `j` is classified at `t*` against `i` (their Lemma 5). When the crossing of the lines `a_j - t * b_j` and `a_i - t * b_i` lies at or below `lo` or at or above `hi`, the slopes give the side (`X`: at least item `i` at `t*`, `Y`: at most); equal `b` compares `a`; coincident lines form `E`, which holds `i`; the rest `Z` is unknown. While `32 * |Z| > |live|`, a test at the median crossing of `Z` settles at least half of `Z`. Then, as an optimal set is any `need` largest values at `t*` with ties broken either way (a set holding a `Y` item can swap it for an unused `X` or `E` item without loss): if `|X| + |E| >= need`, all of `Y` leaves with all members of `E` but `max(0, need - |X|)`; otherwise, if `|Y| + |E| >= drop = |live| - need`, all of `X` is taken with all members of `E` but `max(0, drop - |Y|)`. The loop ends when `need` is 0 or equals the live count.

Time: their Lemma 6 shows that a round removes at least `49/256` of the live items in expectation (the pivot's rank is uniform and lies at least `|live| / 32` from `1`, `need` and `|live|` with probability `7/8`); a round costs `O(|live|)` plus at most 5 tests, so with linear-time selection the total is `O(n)` expected. GNU `std::nth_element` (introselect) is linear on average and `O(m * log(m))` in the worst case, which gives the comment's `O(n * log(n + 1))` expected; the 8 leading tests stay within it. With `Z` empty a round always removes an item (one of the two cases applies, and `i` lies in `E`), which every round guarantees once fewer than 32 items are live. Memory `O(n)`.

### ratioKnapsack

Maximum `sum a / sum b` over subsets with `sum b >= W` (USACO 2018 Open Gold "Talent Show": weights `b`, talents `a`). Domain: `b_i >= 1` (an `lng` by user decision 2026-10-24, like `zeroOneFractionalSelect`; the oracle caps every sum at `W`), `1 <= W < INT_MAX` (the parameter `need`), `sum |a_i| < 2^62`, `sum b_i < 2^62`, all asserted together; outside the domain under `NDEBUG` it returns the `den = 0` sentinel; `a` may be negative. Returns the `den = 0` sentinel when `sum b < W`. The oracle is a 0/1 knapsack maximizing `t * a - s * b` over capped weights `min(W, c + b_i)`, state `W` absorbing every sum at least `W`. The source index `c` runs downward and every move goes to a strictly larger index or keeps `W` at `W` (processed first, from its pre-item value), so each item is used at most once. `from[i][c]` records the source state when item `i` improved state `c`, and walking back from `W` reconstructs the argmax set. Partial sums stay below `2^126` in absolute value, above the `-2^126` sentinel. The start is all items. Cost `O(n * (W + 1))` per iteration, `O(n * (W + 1))` ints for the parent table.

### Legacy mapping

| Removed (user decision 2026-10-23) | Replacement |
|---|---|
| `fractionalBinarySearch` | Mathematics `binSearchRealBracket` over `oracle(t) >= 0` for real bisection; Mathematics `20` `fractionBinarySearch` for exact rational search without a witness |

## Feature-to-test map

| Public feature | Independent coverage |
|---|---|
| `dinkelbach` | Random families of 1–40 `(N, D)` pairs with a brute-force argmax oracle, started from the worst, best and a random member and from the infeasible lower bound `-INT64_MAX / 1`; families with `abs(N)` and `D` within 1 of `INT64_MAX` or `D <= 3` reach the `2^63` operand bound; the result must equal the family maximum by exact cross-multiplication and be a family member, within `4 + log2(A + 1) + 3 * log2(B)` oracle calls. Minimization by negated numerators. Also the engine of `ratioKnapsack`. |
| `zeroOneFractionalSelect`, `FractionalSelection` | Two seeds per case outside the exhaustive section. Every size-`k` subset with exact ratio comparison: exhaustive `a ∈ {-2,0,1,3}`, `b ∈ {1,2,3}`, `n <= 3/4/5` (quick/full/stress), every `k`; seeded `n <= 12` with values to `1e12`, ties, zeros, negatives, values near `2^62`, the two POJ 2976 sample cases, both sums at exactly `2^62 - 1`. These finish within the Dinkelbach tests, so the Eppstein–Hirschberg rounds run on tangent families (tangents to `(T - t)^2` at distances `2^(j+1)`, on which Dinkelbach halves the gap once per test): `n <= 14` against every subset for every `k`, with fillers, noise, always-chosen items and lowered copies; up to 98 and 1204 items, with lowered copies (equal `b`), noise reusing tangent weights, always-chosen items and shifts to negative values, by the certificate below and equal ratios from both seeds; 150/9000/45000 rounds. Large `n = 2000/200000/1000000` over random, duplicate (5 values, `b <= 3`), concurrent (`a = 7b - 5`) and tangent families, `k ∈ {n/3, 1, n - 1}`, by the zero-optimum certificate (the `k` largest keys `a * den - num * b` sum to zero) and witness sums. Mutation check 2026-10-24 (33 mutants of this function, each replacing one expression: the `below` comparison, the initial `lo` and `hi`, both bound updates, the test's key, order, base sums and zero case, the crossing, every classification branch, both case conditions, both spare counts, the drop and take branches, the sum updates and the final sort; tester built `-O2 -DNDEBUG`, seed 1, quick then full then stress): 29 killed, 26 in quick mode, 2 in full and 1 in stress; the 4 survivors (`up + same > need`, `down + same > drop`, no twins taken in the second case, `drop + 1`) only weaken a case condition or move fewer members of `E`, which changes progress, never the result. |
| `ratioKnapsack` | Every subset with `sum b >= W`: exhaustive as above with every `W <= 3n + 1`, including infeasible `W` (`den = 0`), seeded rounds (some with weights within 2 of `INT_MAX`, some up to `2^40` above it), hand fixtures with weights from `INT_MAX - 1` to `2^61 - 1` against `W` from 2 to 1000, the Talent Show sample (`32 / 30`, items `{0, 1}`). Large `n = 60/250`, `W = 300/1000` with an independent forward capped-knapsack certificate `dp[W] = 0`. |
| Witnesses | Indices in range and strictly increasing, sums equal to `num` and `den`, exactly `k` items (selection) and `den >= W` (knapsack). |
| Preconditions | 16 probes: start and oracle denominators, a start above the optimum, a rising nonmaximizing oracle (the 256-call cap), both sum bounds of each application, `k = 0`, `k > n`, size mismatches, a nonpositive `b`, `W = 0`, `W = INT_MAX`. |

## Commands and results

Run 2026-10-24: Linux x86-64, i9-11900H, GCC 16.2.1 and GCC 14.4.1, CPython 3.14. Configurations: optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_runner.py`](<../../96-Local Testing/06-Miscellaneous/_00_runner.py>); MEMORY peaks include the compiler.

```bash
python3 '96-Local Testing/06-Miscellaneous/55-fractional_programming_tester.py' --mode quick --seed 1               # PASS, 2 configurations, 16 probes, peak 431 MB
python3 '96-Local Testing/06-Miscellaneous/55-fractional_programming_tester.py' --mode full --seed 1                # PASS, 3 configurations, peak 786 MB
CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/55-fractional_programming_tester.py' --mode full --seed 2     # PASS, 3 configurations, peak 792 MB
python3 '96-Local Testing/06-Miscellaneous/55-fractional_programming_tester.py' --mode stress --seed 3              # PASS, 3 configurations, peak 769 MB
python3 '96-Local Testing/06-Miscellaneous/55-fractional_programming_tester.py' --mode full --seed 4 --configuration ASan-UBSan    # PASS, 1 configuration, peak 763 MB
python3 '96-Local Testing/02-integration.py'    # PASS, 174 standalone/aggregate headers, multi-TU, workspace, peak 704 MB
python3 '96-Local Testing/03-consistency.py'    # no errors
python3 '96-Local Testing/06-Miscellaneous/55-fractional_programming_benchmark.py'    # PASS, 90 workload/method rows, peak 335 MB (Benchmarks)
```

Per configuration, full ran 397,364 cases / 5.06 million checks and stress 5,726,836 cases / 52.5 million checks (the debug-container and ASan builds run stress as full; alone, ASan stress took 86 s of the runner's 180 s per-binary limit); the most oracle calls any `dinkelbach` run needed was 7.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/06-Miscellaneous/55-fractional_programming_benchmark.py>) and its local, git-ignored `55-fractional_programming_benchmark.json` (2026-10-24, GCC 16.2.1 GNU++20 `-O2 -DNDEBUG`, i9-11900H, one warmup and 5 timed repetitions in alternating order, peak 335 MB) compare `zeroOneFractionalSelect` with the engine it replaced (Dinkelbach from the first `k` items over an `std::nth_element` oracle with index tie-breaks, kept in the driver) on 45 workloads: `n ∈ {10^3, 10^5, 10^6}`, `k ∈ {1, n/2, n - 1}` and five families (random: `a < 10^6`, `b <= 1000`; wide: `|a| < 2^40`, `b <= 2^20`; duplicates: `a <= 4`, `b <= 3`; concurrent: `a = 7b - 5`; tangent: 28 tangents `b = 2^(j+2)`, `a = b * (2^30 - 2^j)` after `n - 28` fillers `(0, 2^30)`). Every result has `k` sorted distinct items with matching sums and the same ratio from both engines and every seed; the first per workload also passes the zero-optimum certificate.

Medians at `n = 10^6`, ms, this function / replaced engine (its oracle calls):

| Family | `k = 1` | `k = n/2` | `k = n - 1` |
|---|---|---|---|
| random | 33.1 / 21.9 (2) | 118.1 / 107.8 (5) | 71.3 / 87.6 (2) |
| wide | 43.3 / 29.3 (3) | 124.6 / 109.5 (5) | 91.9 / 85.0 (2) |
| duplicates | 30.0 / 32.2 (2) | 71.4 / 55.5 (2) | 88.6 / 91.8 (2) |
| concurrent | 31.8 / 24.0 (2) | 71.0 / 62.7 (2) | 84.0 / 77.7 (2) |
| tangent | 45.1 / 94.8 (29) | 21.6 / 88.5 (2) | 26.8 / 116.1 (2) |

Over all three sizes the time ratio is 0.54–1.79 on the four non-adversarial families, where both run the same number of selections (an instrumented count matched the replaced engine's 2–5 calls), and 0.22–0.53 on the tangent family: for `k = 1` Dinkelbach needs 29 calls there and this function 11 tests and 4 rounds; for the other `k` the replaced engine's index tie-break slows its selections over the identical fillers. A tuning run the same day compared 0, 4, 6, 8, 10 and 16 leading Dinkelbach tests: with 0 (plain Eppstein–Hirschberg) the non-adversarial families took 0.7–4.3 times the replaced engine, 4 fell into the rounds on the 5-call workloads (up to 3.3 times), 16 lost ground on tangent `k = 1` (up to 0.85), and 6, 8 and 10 were within noise of each other.

## Sources

Fetched 2026-10-23 by `@researcher` ([00-sources.md](00-sources.md), P072 sweep); the implementation is independently written from the mathematics:

- [OI Wiki 分数规划](https://oi-wiki.org/misc/frac-programming/): Dinkelbach iteration, the binary-search alternative, exactly-k selection and the 0/1 ratio knapsack.
- [USACO 2018 Open Gold "Talent Show" solution](https://usaco.org/current/data/sol_talent_gold_open18.html): binary search over the ratio with the capped knapsack; this header runs Dinkelbach over the same oracle. The sample answer 1066 is checked as `floor(1000 * 32 / 30)`.
- D. Eppstein and D. S. Hirschberg, "Choosing subsets with maximum weighted average", J. Algorithms 24 (1997), [PDF](https://ics.uci.edu/~eppstein/pubs/EppHir-Algs-97.pdf), pages 1–12 read 2026-10-24: testing `F` in linear time (Lemma 1), classification by crossings (Lemmas 4–5), the randomized algorithm (Figure 1) and its expected linear time (Lemma 6). `zeroOneFractionalSelect` implements it (user decision 2026-10-24) and adds the leading Dinkelbach tests and the Dinkelbach update of `lo`, measured under Benchmarks.
- POJ 2976 "Dropping tests": the statement was not read (poj.org unreachable). The two sample cases are hand-computed fixtures (`5 / 6` for 83, `16 / 16` for 100), not judge evidence.

## Limits and handoffs

- Graph ratio problems belong to Graphs and call `dinkelbach` with a graph oracle ([00-notes.md](00-notes.md), Ownership boundaries): the optimum-ratio cycle is row `56`'s planned `minimumRatioCycle`, and the minimum-ratio spanning tree is planned in P152's note (user decision 2026-10-24).
- Under `NDEBUG`, any violated asserted precondition may read or write out of bounds; `03-cpp.md` requires only termination without a hang or unbounded allocation (user decisions 2026-10-23 and 2026-10-24).
- Not adopted in the 2026-10-23 sweep: see [00-notes.md](00-notes.md), P072 package record.

## History

- 2026-10-23: implemented, independent review (unbounded loop under `NDEBUG`, the iteration bound, complexity symbols, a probe; 3 findings fixed) and verified (P072).
- 2026-10-24: `/finish` audit, five review rounds: `zeroOneFractionalSelect` rebuilt on Eppstein–Hirschberg after 8 Dinkelbach tests (user decision 2026-10-24) with tangent families, a benchmark and a mutation check; `dinkelbach` 256-call cap, `ratioKnapsack` `lng` weights (user decision), the `NDEBUG` domain gate, 5 probes, brute force to `n = 14`, ASan stress run as full; full suite passed on g++ and g++-14.
