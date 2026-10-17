# 19-josephus.hpp — evidence

`19-josephus.hpp` answers Josephus queries:

- the person removed at any step;
- the survivor, and its `k = 2` closed form;
- the step at which any person is removed;
- the whole removal order.

It depends only on `01-template.hpp`. Every query except `josephusOrder` uses `O(1)` memory.

## Contracts

Model, shared by every operation: people `0..n-1` stand in a circle. Counting starts at person 0 with 1, and the person who receives count `k` is removed. Counting resumes at the next living person with 1. Steps are zero-based, and the survivor is removed at step `n - 1`. Domains: `n >= 1` (`n >= 0` for `josephusOrder`), `k >= 1` up to `2^63 - 1`, `0 <= t, p < n`; violations are asserted. Under `-DNDEBUG` a violation still terminates: `k = 0` and steps outside `[0, n)` take no run jumps, and `josephusTime` stops once `n` people would have been removed. A start at person `s` instead of 0 adds `s mod n` to every person index.

### josephusKth, josephus

`josephusKth(n, k, t)` returns the person removed at step `t` in `O(min(t + 1, k log(2 + n / (n - t))))`. `josephus(n, k)` is the survivor, `josephusKth(n, k, n - 1)`, in `O(min(n, k (1 + log(n / k))))` for `k <= n` and `O(n)` otherwise.

Algorithm: with `j(s)` the person removed at step `t - (n - s)` in a circle of `s`:

- `j(n - t) = (k - 1) mod (n - t)`;
- `j(s + 1) = (j(s) + k) mod (s + 1)`. This is Halbeisen–Hungerbühler's recurrence: removing person `k - 1` first and renumbering from `k` shifts every later removal by `k`. At `t = n - 1` it is the classical survivor recurrence `J(i) = (J(i - 1) + k) mod i`.

While `r + i (k - 1) < s` holds for the next `i` steps, those steps add `k` without wrapping. So one division jumps `j = (s - 1 - r) / (k - 1)` steps (all remaining steps when `k = 1`), followed by one reduced step. A comparison skips the division when no step can be jumped (`s - 1 - r < k - 1`), so large `k` costs one remainder per step, as the plain loop does.

After a wrap `r < k`, so once `s >= 2 k` each wrap multiplies `s` by at least `1 + 1 / (2 (k - 1))`. That gives `O(k log(n / max(n - t, k)))` wraps, plus at most `min(t, k)` iterations while `s < 2 k`. Every value stays below `n` except `r + k`, which is computed in `ulng`.

### josephusTime

`josephusTime(n, k, p)` returns the step at which person `p` is removed, the inverse of `josephusKth`, in `O(min(n, k (1 + log(n / k))))` for `k <= n` and `O(n)` otherwise.

Algorithm (count labels, after the renumbering in Concrete Mathematics 3.3): person `p` receives count `c = p + 1` on the first pass. A count `c` that is not a multiple of `k` leaves `n - floor(c / k)` people alive, each counted once before this person again, so the next count is `c + n - floor(c / k)`. The person is removed at the first multiple of `k`, at step `c / k - 1`.

While fewer than `k` people are alive, consecutive counts stay between two multiples of `k`, and one division jumps to the last of them. Each such iteration removes at least one person, so there are at most `min(n, k)` of them. With at least `k` alive, every count passes a multiple of `k`, and the population shrinks geometrically: `O(k log(n / k))` counts.

Labels stay below `k n < 2^126` in `ulll`.

### josephusK2

`josephusK2(n)` returns `2 (n - 2^floor(log2(n)))`, the survivor for `k = 2`, in `O(1)`.

### josephusOrder

`josephusOrder(n, k)` returns every person in removal order, so element `t` is `josephusKth(n, k, t)`; `n = 0` gives an empty vector. A Fenwick tree over `n` ones, built in `O(n)` as `i & -i`, finds the person of rank `(cur + k - 1) mod alive` by binary descent and decrements it: `O(n log(n))` time, `O(n)` memory.

### Legacy mapping

| Removed | Replacement |
|---|---|
| `josephus(n, k)` (`OLD/5-Mathematics/14-miscellaneous.hpp`, recursive, zero-based) | `josephus(n, k)`, identical on every `n <= 300`, `k <= 60` (checked 2026-10-18), iterative with `O(1)` memory |
| The plain `O(n)` survivor loop and `josephusLarge` (user decision 2026-10-18) | `josephus(n, k)`: the same recurrence with run skipping, no slower than the loop (Benchmarks) |
| `josephusZeroBased`, `josephusOneBased` (row only; duplicate sweep, index base) | `josephus(n, k)` and `josephus(n, k) + 1` |

Aliases: cp-algorithms `josephus`; OI Wiki 约瑟夫问题 (linear and `O(k log n)` forms); CSES Josephus Queries (`josephusKth` with `k = 2`, one-based); Library Checker has no Josephus problem.

## Feature-to-test map

Entry: [`19-josephus_tester.py`](<../../96-Local Testing/05-Mathematics/19-josephus_tester.py>).

| Feature | Independent evidence |
|---|---|
| Every operation | A direct vector simulation for every `n <= 60`, `k <= n + 10` and `k` in {2^40, 2^63 - 2, 2^63 - 1}. Every step `t` for `josephusKth`, every person `p` for `josephusTime`, the full order, and `josephusK2` at `k = 2`. Seeded simulations up to `n = 3000` with small and full-width `k`. |
| `josephusOrder` at scale | Up to `n = 200000`: a permutation whose last element is the survivor, agreeing with `josephusKth` and `josephusTime` on 300 sampled steps, which is a different algorithm. |
| Huge `n` (up to 2^63 - 1, `k` up to 5000) | Mutual inverses `josephusTime(josephusKth(t)) = t` and `josephusKth(josephusTime(p)) = p`, the survivor removed at step `n - 1`, and `josephusK2 = josephus(n, 2)`. |
| Python oracle | A list simulation for `n <= 2000` with any `k`. For `n` up to `2^63 - 1` and `k <= 300`: the survivor by the Concrete Mathematics D-sequence closed form (`J = k n + 1 - D`), the unaccelerated forward recurrence for `t < 3000`, and the unaccelerated count-label walk. All three are validated by simulation first. |
| Preconditions | Seven checked-build death probes: `n = 0`, `k = 0`, `josephusK2(0)`, `t = n`, `p < 0`, `josephusOrder` with `k = 0` and `n < 0`. A separate `-DNDEBUG` probe (2026-10-18, `-O0` and `-O2`) confirmed that these and the out-of-range `k`, `t` and `p` cases terminate. |

## Commands and results

Run 2026-10-18; conditions as in [18-floorsum.md](18-floorsum.md).

| Command | Result |
|---|---|
| `19-josephus_tester.py --mode full --seed 1` | PASS, 1,381,967 C++ checks and 600 Python rows per configuration, 7 assertion probes, `MEMORY peak=354MB` |
| same with `CXX=g++-14` | PASS, same counts, `MEMORY peak=387MB` |
| `19-josephus_tester.py --mode stress --seed 7` | PASS, 3,160,504 C++ checks and 4000 Python rows per configuration, 7 assertion probes, `MEMORY peak=356MB` |
| `19-josephus_benchmark.py 9` | PASS; table below |

## Benchmarks

`19-josephus_benchmark.py`, GCC 16.2.1, `-O2` without `-march`, Intel Core i9-11900H, one warmup and the median of 9 repetitions. The survivor for `n = 10^7` is compared against the plain loop it replaced, and the outputs are checked equal.

| `k` | Plain loop | `josephus` |
|---|---|---|
| 3 | 40.4 ms | 0.00 ms |
| 1000 | 42.0 ms | 0.09 ms |
| 10^6 | 40.3 ms | 21.8 ms |
| 5 · 10^6 | 39.7 ms | 44.3 ms |
| 10^7 + 1 | 41.5 ms | 41.7 ms |
| 10^18 | 41.2 ms | 41.2 ms |

`josephus` matches the loop when every step wraps and is at most about 10% slower around `k = n / 2`, where half the steps wrap. Everywhere else it is faster, by orders of magnitude for small `k`.

## Sources

Fetched 2026-10-18 by `@researcher` ([00-sources.md](00-sources.md), P060 sweep). Implemented from the recurrences, with no external code copied.

- cp-algorithms and OI Wiki Josephus problem: the `O(n)` recurrence and the `O(k log(n))` survivor.
- Halbeisen and Hungerbühler (1997), Theorem 1: the forward recurrence for the `i`-th removal.
- Wikipedia Josephus problem, Shahjalal Shohag's code library and CSES Josephus Queries: run skipping for the `m`-th removal and its bounds.
- Concrete Mathematics 3.3 (count renumbering and the D-sequence) is cited by these pages. It was not consulted directly; both formulas were re-derived here and validated against simulation.

## Limits and handoffs

- Variable steps per round, `k = 3` closed forms and the Halbeisen–Hungerbühler matrix method are left out ([00-notes.md](00-notes.md), P060 research decisions).
- `OLD/5-Mathematics/14-miscellaneous.hpp` stays in `OLD`; there was no `97-Legacy` excerpt.

## History

- 2026-10-18: P060 initial implementation; `@reviewer` found 8 issues, all fixed, and `josephusLarge` was merged into `josephus` by user decision; full suite passed on g++ and g++-14, stress seed 7 passed.
