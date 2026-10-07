# 14-cyclefinding.hpp — evidence

`14-cyclefinding.hpp` (batch MI17, package P015, status audit) provides budgeted Floyd and Brent cycle detection on one successor orbit. Prerequisite: the C01 template (P002). Implementations are independent contest-profile code. No online submission was made.

## Contracts

### floydCycle, brentCycle

`floydCycle(start,next,budget,equal)` and `brentCycle(start,next,budget,equal)` analyze one deterministic successor orbit. The budget is required and may be any unsigned 64-bit value, including zero and `ULLONG_MAX`. It limits **every successor invocation**, across cycle detection, entry recovery and cycle-length recovery. Equality checks and state copies are not budgeted. Instrumentation side effects are allowed, but they must not change successor results or the equality relation. Correctness: Floyd's runners compare `x_t` and `x_(2t)`; inside the cycle they meet when the step difference is a multiple of the least period. Resetting one runner to the start and advancing both equally locates the first cycle state, and walking the cycle gives the least period. Brent compares a moving runner against a stationary anchor in blocks of doubling capacity, finding the least period once an anchored block covers a full cycle; two runners separated by that period then recover the entry. Every transition checks `evaluations==budget` before calling the successor, so even `ULLONG_MAX` cannot wrap; tail increments need two successful calls and period counts never exceed completed transitions. Brent's block capacity saturates at the unsigned maximum, which cannot invalidate any success reachable within the finite budget. Costs: constant many states and `O(tail+length)` successor/equality calls on success, `O(budget+1)` on failure; state copies and callbacks are extra. A partial successor with an end marker must be adapted to a total transition. Functional-graph decomposition and multi-vertex queries stay with Graphs.

### CycleResult

`CycleResult<T>` contains `found`, `entry`, `tail`, `length` and `evaluations`. On success, `tail` is the first cycle index, `length>0` is its least period, `entry` is the actual state `f^tail(start)`, and `evaluations<=budget`. On failure, `found=false`, `entry=start`, `tail=length=0` and `evaluations=budget`. Failure is **budget exhaustion**, not proof that the orbit is acyclic. No partial lengths are advertised.

States support copy/move construction and assignment. The successor receives an immutable state and returns another state; it must be total and deterministic on the visited domain. Equality defaults to `std::equal_to<T>` but may be any equivalence relation preserved by the successor. In that case the cycle is defined on equivalence classes, while the returned entry remains the concrete state obtained from the start. No default constructor, hash, ordering or built-in equality is needed when the custom equality supplies the relation. Callback exceptions propagate, and all algorithm state is local to a call.

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| Floyd and Brent results | All 3,413 maps on 1–5 states and all 16,739 starting orbits; 366 structured tails/periods around powers of two; 3,000 seeded random maps through 201 states. |
| Strict whole-operation budget | Every budget from zero through first-success-plus-one on exhaustive small maps; exact call accounting and closed forms; partial detection/recovery; `ULLONG_MAX`; million-step nonrepeating prefixes. |
| Generic state/equality/callback contracts | Strings; states without default constructors or `operator==`; congruent custom equivalence and exact concrete entry payload; move-only callbacks; immutable callback arguments; reentrancy and successor/equality exceptions. |
| Constant state space | Live-state instrumentation on long orbits, including 100,000-state cycles. |
| Preconditions | Budget zero is valid. Determinism and equivalence/congruence are semantic preconditions that cannot be generally checked; there are no numeric assertion/death probes for these APIs. |

## Commands and results

Run 2026-09-28: Linux x86-64, i9-11900H, GCC 16.2.1 (20260810), CPython 3.14.7. Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie` with leak checking enabled.

```bash
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_tester.py' --mode full --seed 20260928 --configuration optimized    # PASS
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_tester.py' --mode full --seed 20260928 --configuration checked      # PASS
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan   # PASS
python3 '96-Local Testing/01-run.py' --mode quick --seed 20260928 --filter 14-cyclefinding --no-integration                  # PASS, also when run from /tmp
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_benchmark.py'                                                    # PASS, 80 comparisons
python3 '96-Local Testing/02-integration.py'                                                                                 # PASS, 93 headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'                                                                                 # no errors
```

Per configuration: 20,105 orbits, 436,497 algorithm runs and 69,782,885 non-removable checks. A visited-state table is the independent entry/tail/period oracle. Exact invocation counts match closed forms: Floyd `3*t+2*mu+lambda`, where `t` is the least positive multiple of `lambda` at least `mu`; Brent `P-1+2*lambda+2*mu`, where `P` is the least power of two greater than `mu` and at least `lambda`. Quick/full/stress exhaust maps through 3/5/6 states, use power-boundary exponents 8/14/17, add 100/3,000/20,000 random maps and live-state periods 1,000/100,000/500,000. Stress mode exists but was not run. No `CXX=g++-14` floor run is recorded.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/06-Miscellaneous/14-cyclefinding_benchmark.py>) and `96-Local Testing/06-Miscellaneous/14-cyclefinding_benchmark.json` (2026-09-28, GCC 16.2.1 GNU++20 `-O2 -DNDEBUG`, i9-11900H) passed 80 workload/method comparisons.

Twenty explicit tail/period shapes include self-loops, pure cycles, long tails and lengths around powers of two, through 65,539 states. Each runs both algorithms with either table lookup or a synthetic successor performing 16 rounds of integer mixing. One warmup and five rotating-order repetitions include complete detection and recovery. Every result, independent closed-form call count, exact/one-short budget, and retained expensive-work checksum is verified; setup and verification occur outside timing.

Brent/Floyd median time ratios ranged 0.485–0.980 for instrumented table lookups and 0.490–1.004 for the expensive successor. Both methods pay an observed volatile call-counter cost, which is material for cheap successors. Integer states, contiguous tables, warmed caches and synthetic successor cost limit these shared-host observations; the slight near-boundary regression is retained. No universal winner or automatic algorithm selector is claimed.

## Sources

Actually inspected on 2026-09-28; code is independently implemented, with no external code adaptation:

- R. P. Brent, [*An improved Monte Carlo factorization algorithm*](https://maths-people.anu.edu.au/~brent/pd/rpb051i.pdf), BIT 20 (1980), 176–184, [author publication page](https://maths-people.anu.edu.au/~brent/pub/pub051.html): §§1–3 pp.176–179 for Floyd comparison, doubling and worst-case analysis; §4 pp.179–180 for historical average-case assumptions only. The paper calls the older algorithm “attributed to Floyd”; this is not a claim to have read a Floyd-authored paper. Brent's historical average improvement concerns function-evaluation counts under random-function assumptions, not a universal runtime guarantee.
- [CP-algorithms tortoise and hare](https://cp-algorithms.com/others/tortoise_and_hare.html): full detection/entry proof. Its linked-list null handling is a different domain from this header's explicitly total successor.
- [Cycle detection overview](https://en.wikipedia.org/wiki/Cycle_detection): Floyd, Brent and time-space sections/citations, as secondary comparison evidence.
- Gabriel Nivasch, [*Cycle detection and the stack algorithm*](https://www.gabrielnivasch.org/fun/cycle-detection), author exposition updated November 2004/migrated August 2021: problem/model, stack proof, partitioning and comparison; linked IPL 90(3), 2004, 135–140, DOI 10.1016/j.ipl.2004.01.016. Its ordering and random-rank assumptions differ from these equality-only APIs; logarithmic expected space is not a worst-case constant-state guarantee.

## Limits and handoffs

- Open `/reaudit-review` findings for P015 ([p015.md](<../../00-Guidelines/23-Reaudit Findings/p015.md>)), not yet resolved:
  - 8: complexity comments use `mu` and `lambda` without defining them and separate `M` with `;` instead of `,`.
  - 9: a multi-line `if` block in `brentCycle` closes with `; }` instead of `;}`.
- No general orbit-cycle implementation exists in the legacy index or `OLD`. Archived Pollard–rho routines use a related technique and stay with Mathematics, unmodified.
- Full-history hashing, ordered stack/multistack, Gosper and distinguished-point time-memory tradeoffs are future research alternatives.
- Determinism and equivalence/congruence are semantic preconditions that cannot be checked in general.
- Stress mode and other compiler/interpreter versions (including the GCC 14 floor) were not run.
