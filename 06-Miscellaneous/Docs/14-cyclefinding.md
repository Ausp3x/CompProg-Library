# 14-cyclefinding.hpp — evidence

`14-cyclefinding.hpp` (batch MI17) provides budgeted Floyd and Brent cycle detection on one successor orbit. Package P015 (order MI15, MI16, MI17). The verified C01 template prerequisite is supplied by P002. Implementations are independent contest-profile code. Archived originals remain unchanged. No online submissions are part of this work.

## Contracts

### floydCycle, brentCycle

`floydCycle(start,next,budget,equal)` and `brentCycle(start,next,budget,equal)` analyze one deterministic successor orbit. The budget is required and may be any unsigned 64-bit value, including zero and `ULLONG_MAX`. It limits **every successor invocation**, across cycle detection, entry recovery and cycle-length recovery. Equality checks and state copies are not budgeted. Instrumentation side effects are allowed, but they must not change successor results or the equality relation.

### CycleResult

`CycleResult<T>` contains `found`, `entry`, `tail`, `length` and `evaluations`. On success, `tail` is the first cycle index, `length>0` is its least period, `entry` is the actual state `f^tail(start)`, and `evaluations<=budget`. On failure, `found=false`, `entry=start`, `tail=length=0` and `evaluations=budget`. Failure is **budget exhaustion**, not proof that the orbit is acyclic. No partial lengths are advertised.

States support copy/move construction and assignment. The successor receives an immutable state and returns another state; it must be total and deterministic on the visited domain. Equality defaults to `std::equal_to<T>` but may be any equivalence relation preserved by the successor. In that case the cycle is defined on equivalence classes, while the returned entry remains the concrete state obtained from the start. No default constructor, hash, ordering or built-in equality is needed when the custom equality supplies the relation. Callback exceptions propagate, and all algorithm state is local to a call.

## Correctness and costs

Floyd's runners compare `x_t` and `x_(2t)` after one and two transitions per iteration. Once in the eventual cycle, they meet when the step difference is a multiple of the least period. Resetting one runner to the original start and advancing both equally locates the first cycle state; walking around that cycle finds the least period. Brent compares a moving runner against a stationary anchor in blocks whose capacity doubles, discovering the least period once an anchored block covers a complete cycle. Two runners separated by that period then recover the entry.

Every transition checks `evaluations==budget` before invoking the successor and incrementing the count. Thus even `ULLONG_MAX` cannot wrap. Tail increments require two successful calls, and period counts never exceed already completed transitions. Brent's block capacity saturates at the unsigned maximum instead of doubling past it. Saturation cannot invalidate any success reachable within the finite call budget; a block that large cannot be exhausted after its preceding blocks without already exhausting that budget. Counter safety follows from these invariants rather than an infeasible test orbit of length `2^64`.

Both algorithms use constant many states and `O(tail+length)` successor/equality invocations on success, or `O(budget+1)` work on failure. State-copy/assignment and callback costs are additional; expensive state transitions can make call counts more informative than asymptotic notation alone. A partial successor with an end marker must be adapted explicitly to a total state transition if this API is desired. Finite functional-graph decomposition and multi-vertex queries remain owned by Graphs.

## Feature-to-test map

| Public feature | Coverage |
|---|---|
| Floyd and Brent results | All 3,413 maps on 1–5 states and all 16,739 starting orbits; 366 structured tails/periods around powers of two; 3,000 seeded random maps through 201 states. |
| Strict whole-operation budget | Every budget from zero through first-success-plus-one on exhaustive small maps; exact call accounting and closed forms; partial detection/recovery; `ULLONG_MAX`; million-step nonrepeating prefixes. |
| Generic state/equality/callback contracts | Strings; states without default constructors or `operator==`; congruent custom equivalence and exact concrete entry payload; move-only callbacks; immutable callback arguments; reentrancy and successor/equality exceptions. |
| Constant state space | Live-state instrumentation on long orbits, including 100,000-state cycles. |
| Preconditions | Budget zero is valid. Determinism and equivalence/congruence are semantic preconditions that cannot be generally checked; there are no numeric assertion/death probes for these APIs. |

## Commands and results

The mirrored `14-cyclefinding_tester.py` passed full optimized NDEBUG, checked and ASan/UBSan builds: 20,105 orbits, 436,497 algorithm runs and 69,782,885 non-removable checks per configuration. A visited-state table supplies the independent entry/tail/period oracle. Exact invocation counts also match independently derived formulas: Floyd uses `3*t+2*mu+lambda`, where `t` is the least positive multiple of `lambda` at least `mu`; Brent uses `P-1+2*lambda+2*mu`, where `P` is the least power of two greater than `mu` and at least `lambda`.

Quick/full/stress modes exhaust maps through 3/5/6 states, use power-boundary exponents 8/14/17, add 100/3,000/20,000 random maps, and exercise live-state periods 1,000/100,000/500,000. Full/stress include ASan/UBSan. Final commands:

```bash
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_tester.py' --mode full --seed 20260928 --configuration optimized
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_tester.py' --mode full --seed 20260928 --configuration checked
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_tester.py' --mode full --seed 20260928 --configuration ASan-UBSan
```

The final sanitizer run passed with leak checking outside the sandbox; its process tracer blocked the initial LeakSanitizer run. Stress mode is available but was not executed.

Final environment: Linux x86-64, Intel Core i9-11900H, GCC 16.2.1 (20260810), CPython 3.14.7. Full configurations use GNU++20 optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`. Leak checking remained enabled. The initial sanitizer failures came from LeakSanitizer's documented inability to run beneath the sandbox process tracer; approved runs outside that tracer passed. No algorithm failures remained.

Package-wide P015 integration and consistency commands passed:

```bash
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Integration compiled all 93 present standalone/aggregate headers, linked combined scalar and available AVX2 aggregates across two translation units, and checked the standalone Workspace in LOCAL/non-LOCAL modes. Repository consistency reported no errors. Every saved benchmark source hash matched the final source bytes.

The shared runner discovered and passed this suite from `/tmp`, confirming that tests resolve paths independently of the caller's directory:

```bash
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --seed 20260928 --filter 14-cyclefinding --no-integration
```

## Benchmarks

The [benchmark driver](<../../96-Local Testing/06-Miscellaneous/14-cyclefinding_benchmark.py>) and `96-Local Testing/06-Miscellaneous/14-cyclefinding_benchmark.json` passed 80 workload/method comparisons, using the same GCC 16/i9-11900H GNU++20 `-O2 -DNDEBUG` environment. Command:

```bash
python3 '96-Local Testing/06-Miscellaneous/14-cyclefinding_benchmark.py'
```

Twenty explicit tail/period shapes include self-loops, pure cycles, long tails and lengths around powers of two, through 65,539 states. Each runs both algorithms with either table lookup or a synthetic successor performing 16 rounds of integer mixing. One warmup and five rotating-order repetitions include complete detection and recovery. Every result, independent closed-form call count, exact/one-short budget, and retained expensive-work checksum is verified; setup and verification occur outside timing.

Brent/Floyd median time ratios ranged 0.485–0.980 for instrumented table lookups and 0.490–1.004 for the expensive successor. Both methods pay an observed volatile call-counter cost, which is material for cheap successors. Integer states, contiguous tables, warmed caches and synthetic successor cost limit these shared-host observations; the slight near-boundary regression is retained. No universal winner or automatic algorithm selector is claimed.

## Sources

Actually inspected on 2026-09-28; code is independently implemented, with no external code adaptation:

- R. P. Brent, [*An improved Monte Carlo factorization algorithm*](https://maths-people.anu.edu.au/~brent/pd/rpb051i.pdf), BIT 20 (1980), 176–184, [author publication page](https://maths-people.anu.edu.au/~brent/pub/pub051.html): §§1–3 pp.176–179 for Floyd comparison, doubling and worst-case analysis; §4 pp.179–180 for historical average-case assumptions only. The paper calls the older algorithm “attributed to Floyd”; this is not a claim to have read a Floyd-authored paper. Brent's historical average improvement concerns function-evaluation counts under random-function assumptions, not a universal runtime guarantee.
- [CP-algorithms tortoise and hare](https://cp-algorithms.com/others/tortoise_and_hare.html): full detection/entry proof. Its linked-list null handling is a different domain from this header's explicitly total successor.
- [Cycle detection overview](https://en.wikipedia.org/wiki/Cycle_detection): Floyd, Brent and time-space sections/citations, as secondary comparison evidence.
- Gabriel Nivasch, [*Cycle detection and the stack algorithm*](https://www.gabrielnivasch.org/fun/cycle-detection), author exposition updated November 2004/migrated August 2021: problem/model, stack proof, partitioning and comparison; linked IPL 90(3), 2004, 135–140, DOI 10.1016/j.ipl.2004.01.016. Its ordering and random-rank assumptions differ from these equality-only APIs; logarithmic expected space is not a worst-case constant-state guarantee.

## Limits and handoffs

No general orbit-cycle implementation was found in the relevant local legacy index/targeted OLD search. Archived Pollard–rho factorization routines use a related technique but remain owned by Mathematics; they were not migrated or modified. Full-history hashing, ordered stack/multistack, Gosper and distinguished-point time-memory tradeoffs are explicitly future research alternatives, not implied by completion of the requested Floyd/Brent APIs.

Graphs retains whole-functional-graph queries, and alternative cycle time-memory tradeoffs remain research; they are not claimed implemented by P015. Extended stress mode and execution on other compiler/interpreter versions were not performed. No external online acceptance is claimed, and no online submission was made.
