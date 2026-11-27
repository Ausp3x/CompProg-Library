# 014-cyclefinding.hpp — Evidence

`014-cyclefinding.hpp` (batch MI17, package P015) provides budgeted Floyd and Brent cycle detection on one successor orbit and the k-th orbit term built on Brent. Prerequisite: the C01 template (P002); the implementations are independent contest-profile code.

## Contracts

### floydCycle, brentCycle

`floydCycle(start,next,budget,equal)` and `brentCycle(start,next,budget,equal)` analyze one deterministic successor orbit. The budget is required and may be any unsigned 64-bit value, zero and `ULLONG_MAX` included. It limits **every successor invocation**, across cycle detection, entry recovery and cycle-length recovery; equality checks and state copies are not budgeted. Instrumentation side effects are allowed if they change neither successor results nor the equality relation.

Correctness: Floyd's runners compare `x_t` and `x_(2t)`; inside the cycle they meet when the step difference is a multiple of the least period. Resetting one runner to the start and advancing both equally locates the first cycle state, and walking the cycle gives the least period. Brent compares a moving runner with a stationary anchor in blocks of doubling capacity and finds the least period once an anchored block covers a full cycle; two runners separated by that period then recover the entry. Every transition checks `evaluations==budget` before calling the successor, so even `ULLONG_MAX` cannot wrap; tail increments need two successful calls, and period counts never exceed completed transitions. Brent's block capacity saturates at the unsigned maximum, which cannot invalidate any success reachable within the finite budget.

Costs: constant many states and `O(tail+length)` successor and equality calls on success, `O(budget+1)` on failure; state copies and callbacks are extra. A partial successor with an end marker must be adapted to a total transition. Functional-graph decomposition and multi-vertex queries stay with Graphs.

### CycleResult

`CycleResult<T>` holds `found`, `entry`, `tail`, `length` and `evaluations`. On success, `tail` is the first cycle index, `length>0` the least period, `entry` the actual state `f^tail(start)`, and `evaluations<=budget`. On failure, `found=false`, `entry=start`, `tail=length=0` and `evaluations=budget`. Failure means **budget exhaustion**, not proof that the orbit is acyclic; no partial lengths are advertised.

States need copy and move construction and assignment. The successor receives an immutable state and returns another; it must be total and deterministic on the visited domain. Equality defaults to `std::equal_to<T>` but may be any equivalence relation the successor preserves; the cycle is then defined on equivalence classes, while the returned entry stays the concrete state reached from the start. With a custom equality, no default constructor, hash, ordering or built-in equality is needed. Callback exceptions propagate, and all algorithm state is local to a call.

### orbitTerm

`orbitTerm(start,next,k,equal)` returns `f^k(start)` for any `ulng k`, 0 and `ULLONG_MAX` included, under the same state, successor and equality contract. It runs `brentCycle` with budget `k`. On success with `k>tail` it reduces `k` to `tail + (k - tail) mod length` and walks the remaining steps from `start`; when Brent exhausts the budget, it walks `k` steps directly. That makes at most `2k` successor calls and at most the Brent bound plus `tail+length` calls: `O(min(k, mu + lambda))` with constant state. With a custom equivalence the result is **equivalent** to the true `f^k(start)` (the concrete state reached after the reduced exponent), not necessarily identical. No budget parameter is exposed, since the call count is already bounded by `2k`.

## Feature-to-Test Map

| Public feature | Coverage |
|---|---|
| Floyd and Brent results | All 3,413 maps on 1–5 states and all 16,739 starting orbits; 366 structured tails and periods around powers of two; 3,000 seeded random maps through 201 states. |
| Strict whole-operation budget | Every budget from zero through first success plus one on exhaustive small maps; exact call accounting and closed forms; partial detection and recovery; `ULLONG_MAX`; million-step nonrepeating prefixes. |
| Generic state, equality and callback contracts | Strings; states without default constructors or `operator==`; a congruent custom equivalence with the exact concrete entry payload; move-only callbacks; immutable callback arguments; reentrancy; successor and equality exceptions. |
| `orbitTerm` | On every exhaustive, structured and random orbit, against the first-visit path wrapped by the oracle tail and period: every `k` through `2*(tail+period)+2` on exhaustive and small structured fixtures, plus `k` in {0, 1, span-1, span, span+1, 2*span+3, 2^40, 2^64-2, 2^64-1}; call counts at most `2k` and `8*(tail+period)+8`; nonrepeating orbits through 1,000,000 steps; a congruent custom equivalence with a reduced huge exponent. |
| Constant state space | Live-state instrumentation on long orbits, 100,000-state cycles included. |
| Preconditions | Budget zero is valid. Determinism and congruence of the equivalence are semantic preconditions that cannot be checked in general, so there are no assertion probes. |

A visited-state table is the independent entry, tail and period oracle. Exact invocation counts match closed forms: Floyd `3*t+2*mu+lambda`, where `t` is the least positive multiple of `lambda` at least `mu`; Brent `P-1+2*lambda+2*mu`, where `P` is the least power of two greater than `mu` and at least `lambda`. Quick, full and stress exhaust maps through 3, 5 or 6 states, use power-boundary exponents 8, 14 or 17, add 100, 3,000 or 20,000 random maps and use live-state periods of 1,000, 100,000 or 500,000. Each full configuration runs 20,105 orbits, 436,224 algorithm runs and 70,311,058 non-removable checks.

## Commands and Results

Conditions: Intel Core i9-11900H, Linux x86-64, GNU++20, CPython 3.14, GCC 16.2.1 (`g++`) and GCC 14.4.1 (`CXX=g++-14`); optimized, checked and ASan/UBSan builds made by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>). The 2026-11-10 library-wide runs used seed 42, ran beside the other suites and emptied the build cache before each cold run; their times are this tester's.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 732 MB; optimized 0.1 s compile + 2.7 s run, checked 13.6 s + 3.8 s, ASan-UBSan 15.8 s + 91.1 s; peak 892 MB |
| 2026-11-10 (P274) | `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | PASS; quick preflight peak 792 MB; optimized 0.2 s + 3.7 s, checked 15.5 s + 4.8 s, ASan-UBSan 21.4 s + 106.8 s; peak 903 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode quick` | PASS; optimized 25.2 s + 0.2 s, checked 16.5 s + 1.7 s; peak 729 MB |
| 2026-11-10 (P274) | `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS; quick preflight peak 741 MB; optimized 0.1 s + 11.1 s, checked 5.2 s + 15.4 s, ASan-UBSan 6.5 s + 305.3 s; peak 682 MB |
| 2026-10-08 (P015) | `python3 '96-Local Testing/06-Miscellaneous/014-cyclefinding_tester.py' --mode quick --seed 1` | PASS, 2 configurations |
| 2026-10-08 (P015) | `python3 '96-Local Testing/06-Miscellaneous/014-cyclefinding_tester.py' --mode full --seed 1` | PASS, 3 configurations |
| 2026-10-08 (P015) | `CXX=g++-14 python3 '96-Local Testing/06-Miscellaneous/014-cyclefinding_tester.py' --mode full --seed 2` | PASS, 3 configurations |
| 2026-10-08 (P015) | `python3 '96-Local Testing/06-Miscellaneous/014-cyclefinding_tester.py' --mode stress --seed 3` | PASS, 3 configurations, 618,532,217 checks |
| 2026-10-08 (P015) | `python3 '96-Local Testing/06-Miscellaneous/014-cyclefinding_benchmark.py'` | PASS, 80 comparisons |
| 2026-10-08 (P015) | `python3 '96-Local Testing/02-integration.py' --sanitizers` | PASS, 102 standalone and aggregate headers, scalar and AVX2 multi-TU, workspace, sanitizers |
| 2026-10-08 (P015) | `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Benchmarks

The [benchmark driver](<../../96-Local Testing/06-Miscellaneous/014-cyclefinding_benchmark.py>) and its local, git-ignored `014-cyclefinding_benchmark.json` (2026-10-08, GCC 16.2.1, GNU++20, `-O2 -DNDEBUG`, i9-11900H) passed 80 workload and method comparisons.

Twenty explicit tail and period shapes, through 65,539 states, include self-loops, pure cycles, long tails and lengths around powers of two. Each runs both algorithms with either a table lookup or a synthetic successor performing 16 rounds of integer mixing. One warmup and five rotating-order repetitions time complete detection and recovery. Every result, the independent closed-form call count, the exact and one-short budgets and a retained expensive-work checksum are verified outside the timed region, as is setup.

Brent/Floyd median time ratios ranged 0.476–0.977 for instrumented table lookups and 0.492–1.017 for the expensive successor. Both methods pay the cost of a volatile call counter, which is material for cheap successors. These are shared-host observations limited to integer states, contiguous tables, warm caches and a synthetic successor cost; the slight regression near the boundary is kept. No universal winner or automatic algorithm selector is claimed.

## Sources

Inspected on 2026-09-28 (catalog sweep of 2026-10-08 in [00-sources.md](00-sources.md)); the code is implemented independently, adapting no external code:

- R. P. Brent, [*An improved Monte Carlo factorization algorithm*](https://maths-people.anu.edu.au/~brent/pd/rpb051i.pdf), BIT 20 (1980), 176–184, [author publication page](https://maths-people.anu.edu.au/~brent/pub/pub051.html): §§1–3, pp. 176–179, for the Floyd comparison, doubling and worst-case analysis; §4, pp. 179–180, for the historical average-case assumptions only. The paper calls the older algorithm "attributed to Floyd"; no Floyd-authored paper was read. Brent's average improvement concerns function-evaluation counts under random-function assumptions, not a universal runtime guarantee.
- [CP-algorithms tortoise and hare](https://cp-algorithms.com/others/tortoise_and_hare.html): the full detection and entry proof. Its linked-list null handling is a different domain from this header's explicitly total successor.
- [Cycle detection overview](https://en.wikipedia.org/wiki/Cycle_detection): the Floyd, Brent and time-space sections and citations, as secondary comparison evidence.
- Gabriel Nivasch, [*Cycle detection and the stack algorithm*](https://www.gabrielnivasch.org/fun/cycle-detection), author exposition updated November 2004, migrated August 2021: problem and model, stack proof, partitioning and comparison; linked paper IPL 90(3), 2004, 135–140, DOI 10.1016/j.ipl.2004.01.016. Its ordering and random-rank assumptions differ from these equality-only APIs, and logarithmic expected space is not a worst-case constant-state guarantee.

## Limits and Handoffs

- Neither the legacy index nor `OLD` holds a general orbit-cycle implementation. The archived Pollard–rho routines use a related technique and stay with Mathematics, unmodified.
- Not adopted in the 2026-10-08 sweep: hash-map orbit recording (needs hashing or ordering on states and `O(mu+lambda)` memory, while Brent is within a small constant of the minimal call count at constant memory); Gosper's and Nivasch's stack and multistack algorithms (find the period only, need an order, and give expected rather than worst-case space); Sedgewick–Szymanski–Yao and distinguished points (cryptanalytic time-memory trade-offs); orbit monoid folds (Graphs `FunctionalGraphFold` covers array graphs, and implicit orbits compose `CycleResult` with a fold).
- Graphs `011-functionalgraph.hpp` `functionalOrbit` stays beside these as a different domain, walks that stop ([duplicates.md](<../../00-Guidelines/23-Sweep Ledgers/duplicates.md>), P011).
- The budgeted `step` and `failed` lambdas are repeated in `floydCycle` and `brentCycle` on purpose: a shared detail helper would need its own complexity line, which would push the header over the 8% comment cap.

## Aliases

Search aliases (P263, grab --find): 判圈算法, Floyd判圈, 弗洛伊德判圈, 龟兔赛跑算法, 布伦特判圈, 循环节.

## History

- 2026-09-28: Floyd and Brent verified (P015, previous system).
- 2026-10-08: re-audit added `orbitTerm`, restyled for the comment cap and closing braces, inlined the budget helper and fixed 2 findings; full suites, g++-14 full and the benchmark passed.
- 2026-11-07: P263 added common Chinese names of the row's operations to the search aliases (Simplified plus Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 (SUP25) moved the builds to the shared runner `_00_contest_runner.py` and its ccache `Builder`, with warnings `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; inputs and checks unchanged; the library-wide runs above passed.
