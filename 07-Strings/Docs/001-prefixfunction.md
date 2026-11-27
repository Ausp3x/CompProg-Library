# 001-prefixfunction.hpp — Evidence

`001-prefixfunction.hpp` (batch ST01, package P016) provides the prefix function, Knuth's strong failure table, streaming KMP with overlapping matches, prefix counts, borders and periods, the dense prefix automaton and exact prefix-function validation. Contest profile, GNU C++20, no ISA code.

## Contracts

Common domain: sequences have random access and equality and length below `INT_MAX`; returned indices and lengths are `int`. `string` and `string_view` include embedded NUL and every byte; integer vectors may use the full range of their element type; positions count bytes, not code points. No separator value is reserved. Generic equality must be a consistent equivalence with constant-time indexing. Static functions need only `.size()` and `operator[]` on the text; the matcher constructor, and so the patterns of `kmpOccurrences` and `prefixOccurrences`, needs an iterable pattern. Wrap raw C arrays and string literals in `string_view`, `string` or a vector before matcher deduction.

### prefixFunction

`pi[i]` is the longest proper border of `s[0..i]`. Empty input returns `[]`. O(n) time and returned storage.

### validPrefixFunction

`validPrefixFunction(pi, s)` decides exact feasibility over an unrestricted integer alphabet in O(n) and returns a witness: on success `s` is a sequence whose prefix function is `pi`; on failure `s` is cleared. `s` may alias `pi`, since the witness is built in a temporary. Empty arrays are valid. No fixed-alphabet, minimum-alphabet or lexicographically minimum guarantee is made (ST25 owns those). `002-z.hpp` uses the witness for its conversions.

Correctness: a positive entry forces `s[i] = s[pi[i]-1]`; a zero entry receives the fresh symbol `i`. These endpoint equalities generate every equality a feasible array requires (induct along the border's positions), and splitting otherwise unconstrained classes cannot destroy a required border. Recomputing the prefix function therefore accepts exactly the feasible arrays.

### kmpNext

`kmpNext(p)` and `kmpNext(p, pi)` (with `pi = prefixFunction(p)`) return `m + 1` entries: `next[0] = -1`; for `0 < i < m`, `next[i]` is the longest border `b` of `p[0, i)` with `p[b] != p[i]`, or `-1` when none exists; `next[m] = pi[m-1]`. O(m) time and returned storage. The recurrence `next[i] = p[i] == p[b] ? next[b] : b` with `b = pi[i-1]` is correct because a border whose next symbol equals `p[i]` fails exactly when `p[i]` fails, and `next[b]` already skips those.

### KmpMatcher

The matcher owns a copy of the pattern, its `pi` and `next`. `state` is the length of the longest pattern prefix that ends the consumed stream, the accepting state `m` included; `matched()` tests `state == m`; `step(c)` consumes one symbol and returns whether a match ends there; `reset()` clears `state` and `processed` and keeps the pattern. Setup is O(m). A step is O(1) amortized and O(log(m)) worst case, because consecutive strong-failure jumps shrink the state at least by Fibonacci ratios (Knuth–Morris–Pratt 1977). From the accepting state the next step first falls back to `next[m] = pi[m-1]`, which preserves overlapping matches. An empty pattern matches the initial boundary (a new matcher already has `matched() == true`) and every step. `processed` is an exact `lng` count, at most `INT64_MAX` steps between resets (asserted). The stream stores no text and can exceed `INT_MAX` symbols.

`KmpMatcher<>::same(a, b)` compares symbols. When the two types differ and both are integral (character types, `bool` and the 128-bit types included), it compares the signs first and then the values as `unsigned __int128`, so values compare exactly and an unsigned `UINT_MAX` or `~ulll(0)` never equals a signed `-1`; otherwise it uses `a == b`. Deduction uses the sequence's `value_type`.

`pushBack(c)` appends `c` to the pattern (pattern length below `INT_MAX`, asserted) and extends `pi` and `next` by one entry each: `pi[m]` by the usual border walk from `pi[m - 1]`, `next[m]` recomputed from `pi[m - 1]` now that `p[m]` exists, and `next[m + 1] = pi[m]`. So `pi` and `next` equal `prefixFunction` and `kmpNext` of the grown pattern. A sequence of pushes costs O(1) amortized each (the walk shrinks `j`, and each push raises it by at most 1), O(m) worst for one push. It then calls `reset()` (user decision 2026-11-13): the matcher keeps no text, so a stream state could not stay the longest pattern prefix ending the stream across several appends (with pattern `a`, stream `aa`, two appends of `a`, then `a`, the match of `aaa` would be missed). A default-constructed matcher grown by pushes equals the constructed one. `c` is taken by value, so it may be an element of the matcher's own `pattern`, `pi` or `next`, which the push may reallocate.

Copies are independent; reassign a moved-from matcher before use. Public fields are implementation state and must not be edited independently.

### kmpOccurrences

All overlapping start positions in ascending order; an empty pattern gives `0..n`. O(n + m) time, O(m + out) space. The text is accessed by index only.

### prefixOccurrences

`prefixOccurrences(pi)` returns exact `lng` counts of every prefix length of the original string inside itself, with `res[0] = n + 1`; `pi` must be valid. `prefixOccurrences(pattern, text)` counts every pattern prefix inside the text, `res[0] = n + 1`, in O(n + m) time and O(m) space; the text is accessed by index only. Correctness: each prefix is a node whose parent is its longest proper border, and propagating end counts from long to short adds each occurrence exactly once.

### prefixBorders, prefixPeriod, prefixPeriods

These require a valid `pi` and do not revalidate it; call `validPrefixFunction` first for untrusted arrays. `prefixBorders(pi, len = -1, include_full = false)` lists the positive borders of prefix length `len` (`-1` means n) ascending, proper unless `include_full`; O(out). `prefixPeriod(pi, whole = false)` is the smallest positive period in O(1); `whole = true` returns the smallest period that divides n (the primitive root length); empty gives 0. `prefixPeriods(pi)` lists every positive period ascending, n included; empty gives `[]`.

On an invalid `pi`, `prefixBorders` and `prefixPeriods` still stop. Each step takes `min(k - 1, pi[k - 1])`, so `k` strictly decreases and stays in `[1, len]` (`[1, n)`), every read `pi[k - 1]` lies in `[0, len)` (`[0, n - 1)`), and the output is at most `len` (`n`) meaningless entries. On a valid `pi` the minimum is `pi[k - 1]` and the results are unchanged.

### prefixAutomaton

Encoded pattern symbols lie in `[0, sigma)`, `sigma >= 0` (asserted once at entry). Rows are `0..m`, columns `0..sigma-1`; `aut[q][c]` is the longest pattern prefix that is a suffix after reading `c` in state `q`, and row `m` handles overlaps. O((m + 1) * sigma) time and returned storage; an empty pattern with `sigma = 0` yields one empty row. Encode bytes through `unsigned char` to use `0..255`. Each row reuses the already built row of its border.

### Legacy Mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `validPrefixFunction(pi)` | `vector<int> s; validPrefixFunction(pi, s)` (same status and cost; `s` is the witness) |

## Feature-to-Test Map

Runner: [`001-prefixfunction_tester.py`](<../../96-Local Testing/07-Strings/001-prefixfunction_tester.py>) with shared oracles in `001-prefix_z_test_support.hpp`. The oracles are direct substring and suffix scans and restricted-growth alphabet partitions, independent of failure chains.

| Operation | Coverage |
|---|---|
| prefixFunction | Exhaustive binary strings, random 6-symbol strings, the full byte alphabet, signed 64-bit extremes, index-only sequences, million-symbol unary and near-unary strings. |
| validPrefixFunction | Every range-valid array through length 5/8/9 against the partition-derived feasible set, with the witness's recomputed prefix function on every feasible array and a cleared witness on every infeasible one; witness recomputation on every tested string, separate and aliased (`validPrefixFunction(pi, pi)`); invalid extremes clear separate and aliased witnesses. |
| kmpNext (both overloads), KmpMatcher::next | Direct strong-border scan on every exhaustive and random string. |
| KmpMatcher: step, matched, reset, state, processed, same, pushBack | All bounded pattern and text pairs: state against the naive longest suffix after every symbol, match flags, processed count, copy and move mid-stream, reset and replay; a Fibonacci-word stream (987-symbol pattern) and 1000-symbol unary overlaps; `same` on uint/int, ulng/lng, char/int, int/lng, bool/int, ulll/int, ulll/lll (`2^127` versus `-2^127`), ulll/lng, lll/int and double pairs. `pushBack`: on every exhaustive and random string, matchers grown from the empty pattern and from the first half (after two steps) equal the string's `pattern`, the naive `pi` and the direct strong-border `next`, with state and `processed` reset; on every bounded pair, a matcher grown by pushes, stepped over half the text and pushed once more matches the naive occurrences of the grown pattern in the rest; 10000/200000/1000000-symbol near-unary patterns (`7^(n-1) 9`, whose last push walks the whole border chain) and 2584/75025/317811-symbol Fibonacci patterns grown by pushes against `prefixFunction` and `kmpNext`; patterns grown from 1 to 40 symbols by pushing elements of the matcher's own `pattern`, `pi` and `next` after `shrink_to_fit` (aliasing under reallocation) against the naive `pi`. |
| kmpOccurrences, prefixOccurrences (pattern in text) | Naive occurrences and counts on all bounded pairs and random pairs; a mixed-signedness differential (ulng pattern, lng text with values 0, ±1, `UINT_MAX`, `2^32`, `LLONG_MIN`) against an exact-value id oracle; signed/unsigned regressions (`uint`, `ulng`, `char32_t`, `ulll` against `int -1`); an index-only text type. |
| prefixOccurrences (from pi) | Naive self-counts on every tested string. |
| prefixBorders, prefixPeriod, prefixPeriods | Every prefix length, proper and full borders, divisor-restricted roots, empty, singleton and periodic inputs. |
| prefixAutomaton | Every state and symbol transition against a direct suffix scan, absent symbols and the zero alphabet included; three range probes. |

Checked builds run thirteen assertion probes:

- sequences of size `INT_MAX` and `INT_MAX + 1` (`size-huge` is a fake sequence of the latter);
- `kmpOccurrences` texts of both sizes (`occurrences-text-large`, `occurrences-text-huge`) and the same two texts for `prefixOccurrences` (`counts-text-large`, `counts-text-huge`);
- a negative and a too-large border length;
- a negative alphabet;
- a symbol below range, at `sigma` and well above it (`symbol-far`);
- matcher step-count overflow.

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.1 + 0.2 s, checked 24.0 + 4.2 s, ASan-UBSan 21.9 + 5.4 s. Peak 1049 MB, pre-flight quick 862 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.2 + 0.3 s, checked 24.0 + 3.8 s, ASan-UBSan 22.4 + 4.1 s. Peak 1042 MB, pre-flight quick 931 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 30.3 + 0.0 s, checked 23.5 + 1.8 s. Peak 854 MB. |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | PASS. Optimized 0.1 + 0.7 s, checked 9.8 + 9.9 s, ASan-UBSan 8.4 + 11.3 s. Peak 817 MB, pre-flight quick 869 MB. |

Latest package run 2026-11-13 (ST01 `/finish`; experiment `none`) on GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14 and Linux 7.2 x86-64. Other sessions' jobs shared the machine, so times are indicative.

```bash
python3 '96-Local Testing/07-Strings/001-prefixfunction_tester.py' --mode full --seed 20261113
CXX=g++-14 python3 '96-Local Testing/07-Strings/001-prefixfunction_tester.py' --mode full --seed 20261113
python3 '96-Local Testing/07-Strings/001-prefixfunction_tester.py' --mode stress --seed 20261113
python3 '96-Local Testing/05-extraction.py' both --filter '07-Strings/001-prefixfunction'
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '07-Strings/001-prefixfunction.hpp' '96-Local Testing/07-Strings/001-prefixfunction_tester.cpp'
```

- Full on g++: PASS in all three configurations with the 13 assertion probes, 624,135 checks per configuration. Runs: optimized 0.1 s, checked 1.3 s, ASan-UBSan 1.5 s. Peak 1083 MB.
- Full on g++-14: PASS. Runs: optimized 0.1 s, checked 1.9 s, ASan-UBSan 2.0 s. Peak 1050 MB, pre-flight quick 940 MB.
- Stress: PASS, 4,027,405 checks per configuration. Runs: optimized 0.9 s, checked 11.2 s, ASan-UBSan 12.9 s. Peak 711 MB.
- Extraction `ops`: 15 operations in 14 extractions on g++ and g++-14, 0 failures. Extraction `testers`: the tester passes quick on its extracted build (10 names).
- `02-integration.py`: PASS, 190 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace.
- `03-consistency.py`: no errors. `--braces`: no violations.

## Mutation Testing

Record: mewt 4.0.0 on blob `a5b4f6e275f523ced68c67218459ef777e8b2417` (2026-11-13, seed 1, whole file, uniform sample of 626): 500 mutants, 462 caught (7 by timeout), 38 survived, 0 untested.

```bash
python3 '96-Local Testing/04-mutation.py' run 07-Strings/001-prefixfunction.hpp
```

Stage seconds (the header's first campaign, ST01 `/finish`): quick 698, full on the optimized build 15, full on the checked and ASan-UBSan builds 344; the survivor retest 756, 23 and 537.

| Mutant | Line | Why it survives |
|---|---|---|
| `CR 93`, `COS 376` (line 18); `CR 105`, `COS 416`/`417` (line 50); `CR 111`, `AOS 236`–`238`, `COS 421`/`422` (line 57); `COS 496`/`497` (line 97); `CR 149`, `COS 521`/`522` (line 116); `COS 551`/`552` (line 126); `COS 556` (line 134); `COS 571`/`572` (line 144) | 18–144 | Untestable (user decision 2026-11-13, size asserts beyond the memory caps, as for Z and Fenwick): only a `vector<int>` of about `INT_MAX` entries (8 GiB), or a pattern the matcher copies into one, tells them apart. |
| `COS 381`, `COS 491` (`i < n` → `!=`), `COS 501` (`i > 0` → `!=`), `COS 601` (`c < sigma` → `!=`) | 21, 91, 100, 150 | Equivalent: the index steps by 1 from its start to the bound. |
| `CR 124` (empty-pattern early return removed) | 76 | Equivalent: with `m = 0` the step sets `state = next[0] = -1`, skips the loop and returns `++state == 0`, which is true with `state = 0`, as the early return does. |
| `COS 474`, `COS 484` (`== m` → `>= m`) | 77, 79 | Equivalent: `state <= m` always (a prefix length, or `next` of one). |
| `AS 208`, `AS 210` (`min` operands swapped) | 120, 138 | Equivalent: `min` commutes. |
| `AOS 291`–`293`, `AOS 307`–`309` (`k - 1` → `k + 1`, `k * 1`, `k / 1` inside the `min`), `COS 541` (`0 < k` → `!=`), `COS 567` (`k < n` → `<=`) | 120, 138 | Untestable (user decision 2026-11-13, the invalid-`pi` guards): on a valid `pi`, `pi[k - 1] < k` makes the minimum `pi[k - 1]` and `k` never leaves `[0, len]` (`[0, n)`), so they differ only on an invalid `pi`, which no test may pass; the guards keep the loops bounded there (Contracts). |

## Benchmarks

The algorithms are linear, with no backend choice or dispatch threshold; the contest profile requires no benchmark and no speed claim is made. Large adversaries check scalability.

## Sources

Code is independently implemented from the algorithmic invariants.

- [cp-algorithms, Prefix function. Knuth–Morris–Pratt algorithm](https://cp-algorithms.com/string/prefix-function.html): failure-chain proof, streaming storage, both prefix-count applications, periods and the O(m * sigma) automaton.
- [Lecroq, Knuth-Morris-Pratt algorithm](https://www-igm.univ-eiffel.fr/~lecroq/string/node8.html) (fetched 2026-10-08): the strong failure table `kmpNext` and the logarithmic per-symbol delay bound.
- ST01 re-sweep 2026-11-12 and its `/finish` follow-up 2026-11-13 in [00-sources.md](00-sources.md) (ST01 re-sweep): the KMP/MP files of suisen, emthrm, Nyaan and ei1333 read; [emthrm `MorrisPratt::add`](https://raw.githubusercontent.com/emthrm/cp-library/master/include/emthrm/string/morris-pratt.hpp) is the source of `pushBack` (adopted by user decision 2026-11-13; emthrm is not a `09-sources.md` catalog); catalog aliases under Aliases.
- Catalog sweep 2026-10-08 (cp-algorithms, OI Wiki, KACTL, maspypy `kmp`/`periods`, suisen `morris_pratt`, hitonanode `mp_algorithm`, Nyaan, ei1333, ACL, Library Checker) in [00-sources.md](00-sources.md).
- Targeted searches of `OLD/algorithms.cpp`, `OLD/[1] algorithms.cpp` and Team Notebook `algs.cpp`/`algsbetter.cpp` found no additional prefix or KMP feature.

## Limits and Handoffs

- Validity of `pi` for the border, period and count helpers is a caller precondition and is not rechecked.
- Invalid `pi` (user decision 2026-11-13, recorded as limits): every operation terminates within its normal bound, but `prefixOccurrences(pi)` writes out of range for an entry outside `[0, n]`, negative ones included; `kmpNext(p, pi)` reads `p` out of range; `prefixPeriod` overflows in `n - pi.back()` (for example `pi = {INT_MIN}`, either mode) and with `whole = true` divides by zero when `pi.back() = n`. Only `prefixBorders` and `prefixPeriods` are memory-safe on invalid input (Contracts).
- Online append-only Z, minimum-alphabet reconstruction, substring period indexing and runs stay with their own rows (`30`, `31`, `27`, `26`). Distinct-substring counting belongs to the suffix structures; the quadratic prefix-function reduction is not duplicated.

## Aliases

Search aliases (P263, grab --find): 前缀函数与 KMP 算法, KMP算法, 字符串匹配, 最长公共前后缀, 最小循环节, 字符串周期, KMP自动机, 失败函数.
Catalog aliases (ST01 sweep 2026-11-12): Knuth-Morris-Pratt, Morris-Pratt, failure function, partial match table, border array, MP algorithm; emthrm `MorrisPratt::period(idx)` is `idx - pi[idx - 1]` (user decision 2026-11-13: an alias, no length parameter on `prefixPeriod`), suisen `all_periods` is `prefixPeriods`, emthrm `MorrisPratt::add` is `KmpMatcher::pushBack`.

## History

- 2026-09-28: verified under the previous system (P016).
- 2026-10-08: re-audit baseline, full suite seed 1 passed in 3 configurations before any change.
- 2026-10-08: re-audit, `kmpNext` added, the matcher switched to strong failure links, the witness overload of `validPrefixFunction` replaced `prefix_detail::witness`, 3 findings and 1 review defect fixed, contracts moved out of the header; quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: prune, removed `validPrefixFunction(pi)` (Legacy Mapping), review found 1 defect, fixed; full suite passed on g++ and g++-14.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
- 2026-11-12: ST01 completeness re-sweep found nothing missing (catalog aliases added); header unchanged; full passed on g++ and g++-14.
- 2026-11-13: ST01 `/finish`: `KmpMatcher::pushBack` added (emthrm `MorrisPratt::add`, user decision), `prefixBorders` and `prefixPeriods` bounded on invalid `pi`, invalid-`pi` limits recorded; review found a dangling-reference defect in `pushBack` (fixed with a by-value symbol and an aliasing test); full passed on g++ and g++-14, stress, extraction and integration passed; first mutation campaign.
