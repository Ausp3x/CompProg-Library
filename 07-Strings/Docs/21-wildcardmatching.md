# 21-wildcardmatching.hpp — evidence

`21-wildcardmatching.hpp` (batch ST21, package P085) holds single-symbol wildcard matching with wildcards in the text, the pattern or both (Clifford–Clifford with random symbol values, Monte Carlo; bitwise alphabet decomposition, exact), whole-text glob matching with `*` and `?` by greedy leftmost segments, and `GlobAutomaton`, the bit-parallel NFA of a glob for streaming texts. Contest profile, GNU C++20, number-theoretic transforms from `05-Mathematics/13-convolution.hpp` over `mint` (998244353), no ISA code.

## Contracts

Sequences are `string`, `string_view` or `vector<T>` for integral `T` (any type with `value_type`, `size()` and `operator[]`; string literals need a `string_view`); `char` symbols compare as unsigned bytes for ranking and equality, integral symbols by value. Sizes are below `INT_MAX`. The pattern of both matchers has at most `2^22` symbols (asserted at entry), and so does every middle glob segment holding `any` (asserted when that segment is searched). This keeps every block within the `2^23` NTT limit and makes `m * bit_width(S) < 998244353`. Under `NDEBUG` a longer pattern returns no starts, and such a glob segment matches nowhere, instead of overrunning the transform. Results are all match starts ascending with overlaps; the empty pattern matches at `0..n`; a pattern longer than the text has none.

### Matching engine (both matchers)

A start `i` matches when every position `j` has `pattern[j]` or `text[i + j]` equal to the wildcard, or both equal. Both matchers first run the exact scan `wildcard_detail::scan`: starts ascend, each compares until its first mismatch, and the scan may spend `BUDGET = 16` comparisons per examined start plus `16 * m` in total. Typical texts end there in `O(n + m)`. When the budget runs out, the starts from the next unexamined one go to the block convolution: blocks of `bit_ceil(min(n', SPREAD * m))` text symbols (`SPREAD = 8`, `n'` the remaining length, capped at `2^23`), overlapping by `m - 1`, against the transformed reversed pattern rows, so each block yields `len - m + 1` starts from one cyclic transform per text row and one inverse. The worst case is `O(16 * (n + m))` for the scan plus `O(n * log(m + 1) * rows)` for the blocks.

### wildcardMatch(text, pattern, wildcard, seed = CustomHash::rnd)

Clifford–Clifford: symbol `x` gets the value `1 + splitMix64(seed ^ x) mod (p - 1)` and the wildcard 0, and a start matches exactly when `sum_j p_j t_j (p_j - t_j)^2 = sum p^3 t - 2 p^2 t^2 + p t^3` is zero over the integers (three text rows, three pattern rows). Modulo `p = 998244353` the sum, as a polynomial in independent uniform symbol values, has the nonzero coefficient (the count of the pair, below `p`) on `x_a^3 x_b` for every mismatching pair `{a, b}`, so by Schwartz–Zippel over the `p - 1` possible values a non-matching start is reported with probability at most `4 / (p - 1)`. Starts settled by the exact scan are never wrong. Monte Carlo; a fixed `seed` reproduces the run. `T: O(n log(m + 1))`, `M: O(n + m)`.

### wildcardMatchDeterministic(text, pattern, wildcard)

Exact. Pattern symbols are ranked `0..S-1` in sorted order, other text symbols get rank `S`, wildcards are excluded. Fischer–Paterson bitwise alphabet decomposition: with `W` the non-wildcard indicators, `pop` the popcount of a rank and `P_b`, `T_b` its bit `b`, the sum over positions of `popcount(p xor t)` on non-wildcard pairs is `popP * Wt + Wp * popT - 2 sum_b P_b * T_b`, which is at most `m * bit_width(S) < p`, so it is zero modulo `p` exactly when the start matches. `2 + bit_width(S)` rows. `T: O(n log(m + 1) log(S + 1))`, `M: O(n + m log(S + 1))`.

### globMatch(text, pattern, any = '?', star = '*')

Bytes; the whole text must match. `star` matches any run, including the empty one; `any` matches one symbol; there are no escapes, so the two metacharacters cannot be literal. With no `star`, the lengths must agree and every position must fit. Otherwise the pattern splits into `S_0 * S_1 * ... * S_k`. `S_0` must fit the text start and `S_k` the text end, without overlapping. Each middle segment then takes its leftmost occurrence after the previous one, which is optimal by exchange. Empty middle segments come from consecutive stars and match where they are. A segment without `any` is found with `KmpMatcher` (`01-prefixfunction.hpp`). A segment with `any` uses the budgeted scan, with the budget growing with the scanned starts, then the exact blocks with early exit. So a search that advances `d` symbols costs `O((d + |S_i|) log |S_i| log S)` at worst. Total: `O(n + m)` without `any`, `O((n + m) log(m + 1) log(S + 1))` otherwise, `M: O(m)`.

### GlobAutomaton

`GlobAutomaton(pattern, any, star)`: bytes. The `k` non-star symbols become `PatternMasks` positions (`17-bitap.hpp`). The masks have `k / 64 + 1` words so the accepting bit `k` fits; `any` positions are ORed into all 256 masks in `O(256 * words)`. Bit `i` of `state` means the text read so far matches the pattern up to its `i`-th non-star symbol. A star before position `i` sets loop bit `i`. One step is `state = ((state & mask[c]) << 1) | (state & loops)`. A star needs no epsilon move, because the boundary state itself is the state before and after it. `reset()` restores the initial state (bit 0); `step(c)` consumes one byte and returns `accepted()`, which is true when the text read so far matches the whole pattern. `S: O(m + 256 * k / w)`, `U: O(k / w + 1)`, `Q: O(1)`, `M: O(256 * k / w)`. Streaming several texts needs `reset()` between them; searching for a match ending anywhere is a pattern with a leading star.

### Aliases

Library Checker `wildcard_pattern_matching` (0/1 per start): set entries from the returned starts. maspypy `wildcard_pattern_matching` (`vc<bool>`), Nyaan/ei1333 `WildcardPatternMatching` (`vector<int>`), cp-algorithms FFT matching with pattern wildcards: `wildcardMatch` or `wildcardMatchDeterministic`. Glob DP ("variable-length star DP", LeetCode 44): `globMatch` for one text, `GlobAutomaton` for per-prefix answers.

## Feature-to-test map

Runner: [`21-wildcardmatching_tester.py`](<../../96-Local Testing/07-Strings/21-wildcardmatching_tester.py>). Oracles: brute window scans at every start, and the glob DP over (text prefix, pattern prefix), itself checked against exponential backtracking on tiny inputs. Every check survives `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| wildcardMatch, wildcardMatchDeterministic | Exhaustive texts over `{a,b,?}` against patterns over `{a,b,?}` (quick 5/3, full 7/4, stress 8/5 symbols). Random cases with cheap texts, unary texts and late unary tails; at least a tenth (asserted) hand over to the blocks. Budget neighbours `m` = 14..19; single and multiple blocks (`n` = `m - 1`, `m`, `m + 1`, `8m`, `8m + 1`, `30m`). All-wildcard texts and patterns; `lng` symbols with `LLONG_MIN` as the wildcard; high bytes with NUL as the wildcard; `int` symbols with 0; fresh seeds per case. Large texts (30000/300000/1500000) with `m` = 41, 1000, `min(30000, n / 4)`, and a periodic text with an analytic oracle (even starts). |
| globMatch | Exhaustive binary texts (quick 4, full 6, stress 7) against patterns over `{a,b,?,*}` (4/5/6). Random globs with KMP, naive and convolution segments; consecutive, leading and trailing stars; custom metacharacters `#`/`%`; high bytes; multi-block searches in 3000..6000-symbol texts; unary texts whose segments are asserted to hand over to the blocks. A planted 6-segment glob and an absent symbol on the large text. |
| GlobAutomaton (reset, step, accepted) | Every prefix of every glob case against the DP row, `accepted()` before any step, `reset()` and reuse. Automata of 63, 64, 65, 127, 128 and 129 non-star symbols. A planted 6x2000 pattern on the large text. |
| Preconditions | Checked builds abort on a 2^22 + 1 pattern (both matchers) and a 2^22 + 1 glob segment holding `any`. |
| Wide alphabets on the block path | 10/60/60 `int` cases with up to 2000 distinct pattern symbols (13 rows) behind a zero prefix, and 60-byte-alphabet glob segments, each asserted to hand over to the blocks. |

## Commands and results

Run 2026-10-16 on the final code. Every run builds optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`, precondition probes) binaries, and full/stress add ASan/UBSan (`-O1`); all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress first pass the quick pre-flight under the 2048 MB cap. The checked build runs stress at full-mode sizes.

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 23 suites (this one: 2 configurations, 3 assertion probes)
python3 '96-Local Testing/07-Strings/21-wildcardmatching_tester.py' --mode full --seed 1  # PASS, 3 configurations, 2,936,728 checks each (652 block handovers), 3 assertion probes, MEMORY peak 558 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/21-wildcardmatching_tester.py' --mode full --seed 2  # PASS, 3 configurations, 2,933,618 checks each, MEMORY peak 572 MB
python3 '96-Local Testing/07-Strings/21-wildcardmatching_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 22,122,502 checks (optimized, ASan/UBSan; 2864 handovers), 2,947,679 (checked), MEMORY peak 1028 MB
python3 '96-Local Testing/07-Strings/21-wildcardmatching_benchmark.py'  # medians in Benchmarks, MEMORY peak 369 MB
python3 '96-Local Testing/02-integration.py'  # PASS, 146 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace
python3 '96-Local Testing/03-consistency.py'  # PASS, errors: []
```

## Benchmarks

Driver [`21-wildcardmatching_benchmark.py`](<../../96-Local Testing/07-Strings/21-wildcardmatching_benchmark.py>) (raw JSONL beside it, git-ignored). Conditions: Intel i9-11900H, GCC 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, seed 20261016, one warmup and five timed repetitions, medians in ms; the reference implementation is the same deterministic path with another scan rate or block length.

Scan budget (n = 2,000,000; rate 0 hands over at once, "naive" never does):

| Input | m | rate 0 | 4 | 8 | 16 | 32 | 64 | naive | wildcardMatch |
|---|---|---|---|---|---|---|---|---|---|
| unary text, pattern `a^(m-1) b` | 32 | 135 | 157 | 154 | 157 | 88 | 91 | 85 | 112 |
| | 256 | 184 | 182 | 184 | 188 | 167 | 179 | 367 | 141 |
| | 4096 | 240 | 241 | 247 | 244 | 243 | 243 | 5905 | 189 |
| periodic `ab`, one late mismatch | 32 | 149 | 146 | 145 | 33 | 32 | 31 | 33 | 29 |
| | 256 | 184 | 185 | 197 | 207 | 204 | 197 | 116 | 164 |
| | 4096 | 246 | 248 | 239 | 245 | 240 | 241 | 1345 | 178 |
| random over 4 symbols | 32 | 183 | 11 | 11 | 11 | 11 | 11 | 10 | 10 |
| | 4096 | 252 | 9 | 9 | 10 | 10 | 10 | 10 | 11 |
| sparse hits every 997 | 256 | 163 | 3 | 3 | 3 | 3 | 3 | 3 | 6 |
| | 4096 | 221 | 223 | 7 | 7 | 7 | 7 | 7 | 10 |

A rate of 16 keeps typical and sparse inputs in the scan (10–60 times faster than the blocks), and adversarial inputs pay at most about 16 comparisons per symbol, at most about 16% above the blocks alone (157 against 135 ms), before handing over. Larger rates help only patterns of 16..63 symbols on unary-like texts and raise that worst case.

Block spread (n = 4,000,000, random over 4 symbols, every fifth pattern symbol a wildcard; block length `bit_ceil(c * m)`):

| m | c = 2 | 4 | 8 | 16 | 32 | one block |
|---|---|---|---|---|---|---|
| 64 | 541 | 381 | 383 | 385 | 399 | 3534 |
| 1024 | 834 | 678 | 594 | 574 | 663 | 3707 |
| 32768 | 1154 | 853 | 817 | 909 | 978 | 3482 |

`SPREAD = 8` is best or within 4% of best at every `m`, and one whole-text block is 4–9 times slower.

Headline (n = 4,000,000): random binary/26-letter texts with every seventh pattern symbol a wildcard settle in the scan (11–35 ms for `m` = 1000 and 100000, both matchers). Unary adversarial `m = 4096`: `wildcardMatch` 189 ms, deterministic 244 ms. `globMatch` with six planted 2000-symbol segments: 24 ms with `?`, 18 ms without (KMP). `GlobAutomaton` over the 4M-symbol text: 12 ms (k = 64), 71 ms (k = 1024).

## Sources

Code is independently implemented from the cited descriptions; catalog sweep in [00-sources.md](00-sources.md#p085-sweeps-2026-10-16).

- [P. Clifford, R. Clifford, "Simple deterministic wildcard matching", preprint](https://ics.uci.edu/~goodrich/teach/cs262P/notes/StringMatchingWithFFT.pdf) (pages 1–3 read 2026-10-16): the three-convolution sum, block splitting for `O(n log m)`, and the Fischer–Paterson and Kalai references.
- [Library Checker `wildcard_pattern_matching` task](https://github.com/yosupo06/library-checker-problems/blob/master/string/wildcard_pattern_matching/task.md) (fetched 2026-10-16): both-sided wildcards, 0/1 per start.
- [Nyaan](https://nyaannyaan.github.io/library/string/wildcard-pattern-matching.hpp), [ei1333](https://ei1333.github.io/library/string/wildcard-pattern-matching.hpp), [maspypy](https://maspypy.github.io/library/string/wildcard_pattern_matching.hpp) (fetched 2026-10-16 by the researcher): three-prime exact, one caller modulus, and a random shift with three middle products.
- [Wikipedia, Matching wildcards](https://en.wikipedia.org/wiki/Matching_wildcards) and [R. Cox, "Glob matching can be simple and fast too"](https://research.swtch.com/glob) (fetched 2026-10-16): the glob DP and the backtracking hazard. The greedy leftmost-segment bound with block searches is derived here; no fetched source states it.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): Kalai's one-convolution randomized matcher, the single-backtrack iterative glob, dynamic wildcard matching and wildcard indexing.
- `wildcardMatch` defaults to the per-process `CustomHash::rnd` seed; pass a fixed seed for reproducible runs. Its Monte Carlo bound needs independent uniform symbol values, which the seeded splitmix64 hashes model.
- Tests include `06-Miscellaneous/02-customhash.hpp` first under a `-Wshadow` suppression: its constructor parameter `seed` shadows the member (owned by P013). This header itself builds with `-Wshadow`.

## History

- 2026-10-16: implemented and verified (P085), independent review found 1 defect and 5 rule findings, fixed; full suite passed on g++ and g++-14.
