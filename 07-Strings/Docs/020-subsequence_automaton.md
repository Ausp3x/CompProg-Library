# 020-subsequence_automaton.hpp — Evidence

`020-subsequence_automaton.hpp` (batch ST20, package P084) implements the subsequence (sequence) automaton of a text in two storage variants of one template: `SubsequenceAutomaton` (sparse per-symbol position lists) and `SubsequenceAutomatonDense` (next/prev tables). Beyond transitions it answers subsequence membership on ranges, online matching, per-state distinct-subsequence counts, the k-th distinct subsequence, the lexicographically smallest shortest absent subsequence and the number of occurrences of a pattern as a subsequence. Contest profile, GNU C++20, no ISA code.

## Contracts

### BasicSubsequenceAutomaton<DENSE> (SubsequenceAutomaton, SubsequenceAutomatonDense)

`build(s, S = 256)`, also the constructor, takes `n < INT_MAX` symbols: `char` read as unsigned bytes, integers in `[0, S)` (asserted, `S >= 1`). It copies the codes into `text` and computes `same[i]`, the next position after `i` holding `text[i]` (`n` if none). The dense variant stores `nxt` and `prv` tables of `(n + 1) * S` ints (O(n * S) setup and memory); the sparse variant stores the ascending positions of each symbol (O(n + S)). Rebuilding replaces everything; queries never mutate. Fields are public; the other variant's tables or position lists are empty.

States are positions `0..n`: state `i` means `text[0, i)` has been consumed. `next(i, c)` is the smallest `j >= i` with `text[j] = c`, `n` if none; `prev(i, c)` is the largest `j < i` with `text[j] = c`, `-1` if none (`0 <= i <= n`, `0 <= c < S` asserted). Both are O(1) dense and O(log n) sparse, through `rank(i, c)`, a hand-written binary search counting positions of `c` below `i` (`std::lower_bound` would add an O(n) partition check per call under `_GLIBCXX_DEBUG`).

`step(state, c)` is the online form of `isSubsequence`: for `state` in `[0, n]` and `c` in `[0, S)` it returns `next(state, c) + 1`, or `n + 1` (the dead state) when no such position exists. The dead state and query symbols outside `[0, S)` (any integral type, `char` as unsigned) map to `n + 1`. `0 <= state <= n + 1` asserted.

`isSubsequence(t, l = 0, r = -1)` decides whether `t` is a subsequence of `text[l, r)` (exactly `r = -1` means `n`; otherwise `0 <= l <= r <= n` asserted) by greedy leftmost matching from state `l`, accepting when the final state is at most `r`. Foreign symbols make it false; the empty `t` is always a subsequence. O(|t|) dense, O(|t| log n) sparse. Greedy leftmost matching is optimal by the usual exchange argument.

### countDistinctSubsequences<T = lng>()

Returns `n + 1` values: `res[i]` is the number of distinct subsequences of `text[i, n)`, the empty one included, so `res[n] = 1` and `res[0]` counts the whole text. Unlike `19` `countDistinctSubsequences(s)`, which returns only the whole-text count, the per-state counts drive `kthDistinctSubsequence` and lexicographic ranking. Recurrence: `res[i] = 2 * res[i + 1] - res[same[i] + 1]` with `res[n + 1] = 0`; the subsequences of `text[i, n)` are those of `text[i + 1, n)` plus `text[i]` followed by one of them, and the duplicates are exactly `text[i]` followed by a subsequence of `text[same[i] + 1, n)`.

For integral `T` (including `__int128` and unsigned types) the arithmetic saturates at `numeric_limits<T>::max()`. Values below the cap are exact: `res` is nonincreasing in `i`, so a saturated `res[i + 1]` forces a saturated `res[i]`, and otherwise `res[i] = x + (x - y)` with `0 <= x - y <= x` is formed by an overflow-checked addition without a wider type. Any other `T` (a modular integer) runs the recurrence in the ring `T`, which needs `T(0)`, `T(1)`, `+` and `-`. O(n) time and memory.

### kthDistinctSubsequence(k, res)

The distinct subsequences of the text, in lexicographic order with a proper prefix first (so `k = 0` is the empty sequence), are indexed from 0. For `0 <= k < count` it writes the `k`-th into `res` and returns true; otherwise it clears `res` and returns false. Counts saturate at `INT64_MAX`, so every `k < INT64_MAX` below the true count is answered exactly (`k = INT64_MAX` is reported absent when the true count exceeds it). From state `i` the walk tries symbols in increasing order and descends into `next(i, c) + 1` when `k` falls in that block. O(n + |res| * S) dense, O(n + |res| * S * log n) sparse.

### shortestAbsentSubsequence()

Returns the lexicographically smallest among the shortest sequences over `[0, S)` that are not subsequences of the text (the AtCoder ARC 081 E convention); one exists because `S >= 1`. Let `len[i]` be the shortest absent length from state `i` (`len[n] = 1`, dead state 0). `len` is nonincreasing, so `len[i] = 1 + len[f(i) + 1]`, where `f(i) = max_c next(i, c)` ends the shortest window `[i, f(i)]` holding every symbol, or `len[i] = 1` when some symbol is missing from `text[i, n)`; a two-pointer sweep from the right finds every window in O(n + S). At state `i` with remaining length `L`, the witness takes the smallest `c` absent from `text[i, q - 1)`, `q` being the first state with `len[q] < L`. These ranges are disjoint and the answer has at most `n / S + 1` symbols, so marking and the O(S) minimum searches cost O(n + S) in total, plus one `next` per symbol (O(log n) sparse).

### countSubsequenceOccurrences<T = lng>(t)

The number of index tuples `i_1 < ... < i_m` with `text[i_j] = t[j]` (1 for the empty `t`, 0 when `t` holds a foreign symbol), with the saturating or ring semantics above. The positions of each symbol in `t` are linked in decreasing order and each text symbol updates only the matching `dp` entries, so the cost is O(n + m + S + r), `r` counting pairs `(i, j)` with `text[i] = t[j]` (at most `n * m`). Memory O(m + S).

## Feature-to-Test Map

Runner: [`020-subsequence_automaton_tester.py`](<../../96-Local Testing/07-Strings/020-subsequence_automaton_tester.py>). Both variants run every check. The reference is independent of the header: next and prev by scanning (a backward table above `3 * 10^7` scan steps), counts by summing `1 + sum_c cnt[next(i, c) + 1]` with saturation, the absent word by an O(n * S) DP over all symbols, and occurrences by the O(n * m) DP. Small cases add brute subsequence sets by bitmask.

| Operation | Coverage |
|---|---|
| build, next, prev | Every state and symbol of every case; byte-string construction (`text` codes). |
| step | Every state including the dead state `n + 1`, with symbols `-1`, `0`, `S - 1`, `S`. |
| isSubsequence (range) | Random patterns, some with foreign symbols, on random `[l, r)`; brute: every subsequence against every range of every exhaustive string. |
| countDistinctSubsequences | `lng`, saturated `int` and a wrapping ring (mod 1000003) against the reference; `__int128` and `unsigned __int128` on 200 distinct symbols (exact powers of two up to the cap, then saturated); brute per-state set sizes; long ternary texts beyond `INT64_MAX` (saturation). |
| kthDistinctSubsequence | `k` in {-1, 0, 1, 2, count - 1, count, INT64_MAX - 1, INT64_MAX} and 20 random `k` against the reference walk; brute: every index of the sorted subsequence set. |
| shortestAbsentSubsequence | Every case against the reference DP; brute enumeration on exhaustive cases; a ternary text of 3000 symbols against the reference and of 200000 symbols for absence and prefix presence; a 10^6-symbol alphabet (smallest missing symbol). |
| countSubsequenceOccurrences | `lng`, saturated `int` and ring against the reference DP; brute mask enumeration for every pattern of length at most 4 on exhaustive cases. |

Inputs: exhaustive strings over alphabets 1..3 through length 6/7/8 (ternary one shorter) for quick/full/stress, then 200/1000/2000 random strings up to 400 symbols over alphabets 1..5 and 256 (dense only while `(n + 1) * S <= 2 * 10^6`), and a ternary text of 20000/200000 symbols. Checked builds run six precondition probes: a build symbol outside `[0, S)`, `next` past `n`, `prev` with `c = S`, `l > r`, `r = -5`, and a step from state `n + 2`.

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`; it runs stress at full sizes) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.3 + 3.1 s, checked 16.4 + 8.5 s, ASan-UBSan 18.2 + 63.8 s. Peak 969 MB, pre-flight quick 781 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.2 + 3.9 s, checked 22.9 + 11.6 s, ASan-UBSan 25.2 + 68.4 s. Peak 1047 MB, pre-flight quick 847 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 26.9 + 0.7 s, checked 21.8 + 15.1 s. Peak 782 MB. |
| `python3 '96-Local Testing/07-Strings/020-subsequence_automaton_tester.py' --mode stress --seed 42` (three suites at a time) | PASS. Optimized 0.1 + 3.3 s, checked 6.6 + 3.1 s, ASan-UBSan 6.7 + 61.8 s. Peak 681 MB, pre-flight quick 785 MB. |

Tester runs 2026-10-11 on the final code, the latest with check counts:

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 20 suites (this one: 2 configurations, 6 assertion probes)
python3 '96-Local Testing/07-Strings/020-subsequence_automaton_tester.py' --mode full --seed 1  # PASS, 3 configurations, 23,430,539 checks each, MEMORY peak 642 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/020-subsequence_automaton_tester.py' --mode full --seed 2  # PASS, 3 configurations, 19,839,839 checks each, MEMORY peak 669 MB
python3 '96-Local Testing/07-Strings/020-subsequence_automaton_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 54,944,793 checks (optimized, ASan/UBSan), 23,734,911 (checked), MEMORY peak 660 MB
python3 '96-Local Testing/02-integration.py'  # PASS, 129 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/02-integration.py' --sanitizers  # FAIL outside this package: the Core 16-poly_tester.cpp self-test is linked alone and misses its second translation unit (otherUnitConvolution); all earlier stages passed
python3 '96-Local Testing/03-consistency.py'  # PASS, errors: []
```

## Benchmarks

The dense and sparse variants are explicit storage choices with different bounds (folder convention: sparse default, dense opt-in), not a dispatched threshold; no benchmark is required.

## Sources

Code is independently implemented from the cited descriptions.

- [OI Wiki, 序列自动机](https://oi-wiki.org/string/seq-automaton/) (fetched 2026-10-10): next table, greedy membership, HEOI2015 product-automaton applications.
- [maspypy `count_subsequence.hpp`](https://raw.githubusercontent.com/maspypy/library/main/string/count_subsequence.hpp) and [`is_subsequence.hpp`](https://raw.githubusercontent.com/maspypy/library/main/string/is_subsequence.hpp) (fetched 2026-10-10): distinct count including the empty sequence.
- [Nyaan `number-of-subsequences.hpp`](https://nyaannyaan.github.io/library/string/number-of-subsequences.hpp) and [suisen `number_of_subsequences.hpp`](https://suisen-cp.github.io/cp-library-cpp/library/dp/number_of_subsequences.hpp) (fetched 2026-10-10): nonempty count convention.
- [AtCoder ARC 081 E](https://atcoder.jp/contests/arc081/tasks/arc081_c) (fetched 2026-10-10): lexicographically smallest shortest absent subsequence.
- [luogu article v381e762](https://www.luogu.com.cn/article/v381e762) and [cnblogs BlankAo](https://www.cnblogs.com/BlankAo/p/15826647.html) (fetched 2026-10-10): position lists with binary search for large alphabets.
- Catalog sweep in [00-sources.md](00-sources.md#p084-sweeps-2026-10-10).

## Limits and Handoffs

- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.
- Not adopted (reasons in [00-notes.md](00-notes.md)): lexicographic rank of a subsequence, a persistent-segment-tree transition table.
- The whole-text distinct count is `countDistinctSubsequences()[0]`; subtract one for the nonempty convention of Nyaan, suisen and Library Checker `number_of_subsequences`.

## Aliases

Search aliases (P263, grab --find): 序列自动机, 子序列自动机, 判断子序列, 本质不同子序列, 最短非子序列, 第k小子序列, 子序列出现次数.

## History

- 2026-10-11: implemented and verified (P084), independent review found 3 issues, fixed.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
