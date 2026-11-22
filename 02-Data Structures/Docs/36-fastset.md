# 36-fastset.hpp — Evidence

Package P035 (batch DS32). New header. Both structures store plain `vector<ulng>` words and use `std::countr_zero`/`std::countl_zero`; neither depends on Core `18-bitset.hpp`, whose `findNext`/`findPrev` scan words linearly. `DecrementalSet` uses row 01 `DSU`. Without ISA code or thresholds the Contest profile requires no benchmark. Each call asserts its preconditions once.

## Contracts

### FastSet

Keys `[0, n)`, `0 <= n <= INT_MAX`. Bit `x & 63` of word `x >> 6` on level 0 holds key `x`, and bit `i` of level `h + 1` is set exactly when word `i` of level `h` is nonzero. The top level is one word; when `n = 0` level 0 has no words and every query returns its sentinel. There are `max(1, ceil(log64(n)))` levels and about `n / 63` words in all.

- `FastSet(n)`: empty. `FastSet(n, f)`: contains `x` exactly when `f(x)` is true, calling `f` once per `x` in increasing order; O(n), against O(n log64(n)) for `n` inserts.
- `insert(x)`, `erase(x)`: return whether the set changed; assert `0 <= x < n`. O(log64(n)), stopping at the first level whose word was already nonzero (insert) or stays nonzero (erase).
- `contains(x)` asserts `0 <= x < n`; `size()` is the number of keys, not the universe `n`; `empty()`.
- `next(x)` returns the smallest key `>= x`, or `n` when there is none; `prev(x)` the largest key `<= x`, or `-1`. Both take any `int` (`x < 0` acts as 0 for `next`, `x >= n` as `n - 1` for `prev`), so `prev(k - 1)` is the strict predecessor without a guard. O(log64(n)): climb while the rest of the word is empty, then descend by one bit scan per level.
- `min()` is `next(0)` (`n` when empty); `max()` is `prev(n - 1)` (`-1` when empty).
- `enumerate(l, r, f)`: calls `f(x)` for every key in `[l, r)` in increasing order; asserts `0 <= l <= r <= n`. As a `next` loop it lets `f` erase the key it visits (or any smaller key), and visits a key that `f` inserts beyond the current one. A step costs O(1) while the next key is in the same level-0 word and O(log64(n)) otherwise.
- Copies are independent.

### DecrementalSet

Keys `[0, n)`, `0 <= n <= INT_MAX`, all present at construction and only erased afterwards. `w` holds one bit per key, the last word masked to `n` bits. `d` is a row 01 `DSU` over the `ceil(n / 64)` words whose sets are the maximal runs of empty words, and `lo[r]` and `hi[r]` hold the first and last word of root `r`'s run.

- `erase(x)`: returns whether `x` was present; asserts `0 <= x < n`. When its word becomes empty, it unites the word with the empty runs on either side and stores the run bounds at the new root.
- `next(x)`, `prev(x)`: as in `FastSet`, with the same sentinels and argument domain. One in-word scan; if it fails and the adjacent word is empty, one DSU find gives the far end of the run, whose neighbour is nonempty or out of range.
- `contains(x)`, `enumerate(l, r, f)`: as in `FastSet`. Erasing while enumerating is the cp-algorithms "painting subarrays" pattern.
- Complexity: O(alpha(n / 64)) amortized per call (union by size with path compression in `DSU`); memory `ceil(n / 64)` words plus four `int` per word. `next` and `prev` are not `const`, because finds compress paths.
- The tail mask is not needed for correctness, since unmasked bits at positions `>= n` would act as a sentinel returning `n` (the equivalent mutant of the hand mutation check). It is kept so the words hold exactly the set.

### Violated Preconditions

Under `NDEBUG`, out-of-range `insert`, `erase` and `contains` read or write beyond the last word; `enumerate` with `r > n` stops at `n`; a negative size either builds at most one word per level, on which every query returns its sentinel (`next` gives `n`, `prev` gives -1), or makes `vector` throw `length_error` (n = -64 and n <= -128). Loops are bounded by the level count or the range.

### Legacy Mapping

No legacy source. Catalog names: maspypy `FastSet` `insert`/`erase`/`next`/`prev`/`enumerate`, `FastSet(n, f)` and `build` → the predicate constructor, `fill_one` → `FastSet(n, [](int) { return true; })`, `any(l, r)` → `FastSet::anyInRange` (missing; meanwhile `next(l) < r`), `enumerate(l, r, f, erase = true)` → `enumerate` with `erase` in `f`, `size()` (the universe) → `n`; hitonanode `fast_set` `next(i, default)`/`prev(i, default)` and `clear` → the sentinels and a new set; Nyaan `w-ary-tree` `find_next`/`find_prev`/`contain` → `next`/`prev`/`contains`; noshi91 `w_ary_tree_set` strict `pred(k)` → `prev(k - 1)`; tko919 `FastSet` → `FastSet`; maspypy `Decremental_FastSet` → `DecrementalSet`. Library Checker `predecessor_problem` returns `-1` for no successor: map `next`'s `n` to `-1`.

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/36-fastset_tester.cpp`, entry `36-fastset_tester.py` (driver `_00_contest_runner.py`; three builds; 11 assertion probes). Sizes 0, 1, 2, 63, 64, 65, 127, 128, 129, 200, 4095, 4096, 4097, plus 262143, 262144 and 262145 outside quick mode (every level boundary up to three levels).

| Operation | Group | Oracle |
|---|---|---|
| Predicate build, `insert`, `erase` (return values), `contains`, `next`, `prev`, `min`, `max`, `size`, `empty`, `enumerate` (with and without erasing in `f`) | `fastHistories`: densities 0, 1, 50, 99 and 100 %, 3000 steps (300 quick; sizes above 300 take 20000 in stress) | Brute force: `std::set` (`lower_bound`, `upper_bound`, `count`). `next`/`prev` probe `INT_MIN`, -64, -2, -1, 0, 1, `n - 2` … `n + 1`, `n + 64`, `INT_MAX`, word and level boundaries and random keys, plus every `x` in `[-1, n]` for `n <= 300`; `enumerate` over a random window of at most 4096 keys |
| `DecrementalSet` `erase` (return value), `contains`, `next`, `prev`, `enumerate` (with erasure) | `decrementalHistories`: erase orders random, ascending, descending and whole words in random word order (runs merge from both sides); every key for `n <= 300`, 3000 erasures otherwise (300 quick, 20000 stress) | The same brute force |
| Scale | `large`: n = 10^6 with 2 * 10^6 operations and n = 2^18 with 10^6 (full); n = 2^22 with 8 * 10^6 (stress) | `std::set` (`lower_bound`, `upper_bound`) |
| Empty sets, sentinels at the extremes, duplicate insert, empty ranges, copies, predicate call order, `enumerate` whose `f` erases a smaller key and inserts one ahead | `edges` | Direct values |
| Preconditions | `invalid` | Out-of-range `insert`, negative `erase`, out-of-range `contains`, reversed and overlong `enumerate`, negative size; `DecrementalSet` out-of-range `erase`, negative `contains`, reversed and overlong `enumerate`, negative size |

A hand mutation check on 2026-10-23 (quick mode, seed 3) caught faults in the `next` descent, the erase level propagation and the right-run bound; the dropped tail mask is the equivalent mutant noted under `DecrementalSet`.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 743 MB; optimized 0.1 + 22.3 s, checked 10.6 + 32.4 s, ASan-UBSan 12.3 + 136.6 s; MEMORY peak 888 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 810 MB; optimized 0.2 + 36.5 s, checked 20.5 + 62.2 s, ASan-UBSan 19.3 + 180.9 s; MEMORY peak 965 MB |
| `01-run.py --mode quick` | Optimized 21.1 + 4.1 s, checked 10.9 + 21.5 s; MEMORY peak 727 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 48 MB; optimized 0.1 + 29.9 s, checked 6.0 + 41.9 s, ASan-UBSan 7.0 + 126.5 s; MEMORY peak 884 MB |

Latest per-header run 2026-10-23 on the same machine with CPython 3.14.7, in the optimized, checked and ASan/UBSan builds:

| Command (2026-10-23) | Result |
|---|---|
| `36-fastset_tester.py --mode full --seed 20261023` (GCC 16.2.1) | PASS, 3 configurations, 52,109,369 checks per configuration, 11 assertion probes, MEMORY peak 676 MB |
| `CXX=g++-14 36-fastset_tester.py --mode full --seed 20261023` (GCC 14.4.1, floor) | PASS, 52,109,369 checks per configuration, MEMORY peak 780 MB |
| `36-fastset_tester.py --mode stress --seed 7` | PASS, 70,322,996 checks per configuration, MEMORY peak 922 MB |
| `36-fastset_tester.py --mode quick` (2026-10-24, after the `enumerate` bound fix) | PASS, 2 configurations, 5,919,788 checks, 11 assertion probes; an ad-hoc `-DNDEBUG` probe of `enumerate(0, n + 1)` on both structures terminates |
| `02-integration.py` | PASS, 174 standalone and aggregate headers, multi-TU, workspace; MEMORY peak 714 MB |
| `03-consistency.py`, `plan.py check` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/36-fastset_tester.py' --mode full --seed 20261023
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/36-fastset_tester.py' --mode full --seed 20261023
python3 '96-Local Testing/02-Data Structures/36-fastset_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '00-Guidelines/13-Plan/plan.py' check
```

## Sources

| Source | Actual reading and use |
|---|---|
| maspypy `ds/fastset.hpp`, `ds/decremental_fastset.hpp`; hitonanode `fast_set.hpp`; tko919 `fastset.hpp`; Nyaan `w-ary-tree.hpp`; noshi91 `w_ary_tree_set.cpp` | Level layout, linear build from a predicate, sentinel choices, enumeration with erasure, the word-level decremental set. |
| cp-algorithms *Disjoint Set Union* (painting subarrays) | Next-alive pointers through a DSU with run bounds kept at the root. |

Fetched 2026-10-23 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-23-p035-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Row 36 is partial: `FastSet::anyInRange` and `FastSet::erase` (range) are missing, returned by P275.
- Left out with reasons in [00-notes.md](00-notes.md#p035-omissions): `clear`/`reset`, `fillOne`, `toString`, counting and k-th.
- Planned row 42 (van Emde Boas) lists `WAryTreeSet` as another name for this row's `FastSet` (`duplicates.md`, P114).

## Aliases

Search aliases (P263, grab --find): 64-ary bitset tree, word-level van Emde Boas set; Library Checker `predecessor_problem`; 压位Trie, 64叉树, 前驱后继, 区间染色.

## History

- 2026-10-23: P035 first implementation; the review's 7 findings were fixed across the package (here the negative-size `NDEBUG` note, a reversed `DecrementalSet::enumerate` probe, predicate-order and insert-ahead tests); full (seed 20261023) on g++ and g++-14 and stress (seed 7) passed.
- 2026-10-24: `/finish` review, 4 findings fixed (here `enumerate` loops to `min(r, n)`, so `r > n` under `NDEBUG` terminates, identically on the asserted domain); quick passed.
- 2026-11-06: P253 `/finish` (user decision 2026-11-06): the inventory's `build` became `ctor` (the predicate constructor), so `96-Local Testing/05-extraction.py ops` resolves it; no code changed.
- 2026-11-07: P263 added the search aliases (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
