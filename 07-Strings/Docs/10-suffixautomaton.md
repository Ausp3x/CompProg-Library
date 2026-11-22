# 10-suffixautomaton.hpp — Evidence

`10-suffixautomaton.hpp` implements the online suffix automaton (the minimal DFA of all substrings) of one byte text with map or dense array transitions: online distinct-substring counters, a `build()` pass for endpos counting, occurrence queries, `k`-th substrings, ordered enumeration, matching statistics, longest common substring, rotation occurrences and the shortest absent string.

## Contracts

### BasicSuffixAutomaton, SuffixAutomaton, SuffixAutomatonDense

- `BasicSuffixAutomaton<S, BASE>`: `S = 0` (alias `SuffixAutomaton`) stores transitions in `map<unsigned char, int>` and accepts every byte; `S > 0` (alias `SuffixAutomatonDense<S = 26, BASE = 'a'>`) stores `array<int, S>` over bytes `[BASE, BASE + S)`, with `0` meaning no transition (the root is never a target). Bytes are read as `unsigned char`; embedded NUL is valid. `A` is the effective alphabet size (`256` for maps).
- Text length `n < 2^30` (asserted in `extend`), so the at most `2n - 1` states, every length and every count fit `int`. `distinctSubstrings()` is exact `lng` (`<= n(n+1)/2`); `totalSubstringLength()` is exact `lll` (`<= n(n+1)(n+2)/6`).
- `Node`: `len` is the length of the state's longest string, `link` the suffix link (`-1` at the root), `end` the end position of the first occurrence (`firstpos`, `-1` at the root), `next` the transitions. State `0` is the root. A state is original (created by `extend`, not a clone) exactly when `end + 1 == len` and it is not the root.
- `extend(c)` appends one byte online: `O(log(S+1))` amortized for maps, `O(S)` for dense arrays, since a new state zero-initializes its array and a clone copies one. It updates `distinct` and `total` in `O(1)` (a new state contributes the lengths `(len(link), len]`; a clone only splits a range) and clears `built`. `step(u, c)` (byte) and `step(u, x)` (code `x = byte - BASE`) return the transition or `-1`, also for out-of-range bytes and codes. A `uint8_t` argument selects the code overload (integral promotion beats conversion to `char`); pass a `char` for bytes.
- `build()` (`O(n)` map, `O(n * S)` dense; `O(n)` stored) counting-sorts states by `len` and computes `cnt` (endpos sizes: originals 1, accumulated along suffix links, the root set to `n + 1`, the occurrences of the empty string), `last_end` (the largest end position in the link subtree), the link-tree children in CSR form, `paths` (distinct paths from a state, the empty one included) and `weighted` (sum of `cnt` over nonempty paths). `build(s)` resets the object, reserves `2n + 1` states, extends every byte and calls `build()`; the string constructor calls `build(s)`.
- Valid at any time: `size()` (`n`), `findNode`, `isSubstring`, `firstOccurrence`, `minimalLength`, `distinctSubstrings`, `totalSubstringLength`, `lexicographicWalk`, `matchingStatistics`, `longestCommonSubstring`, `suffixLinkTree`. These need `build()` (asserted through `built`, which `extend` clears): `occurrenceCount`, `lastOccurrence`, `occurrences`, `endposSize`, `kthSubstringDistinct`, `kthSubstring`, `cyclicShiftOccurrences`, `shortestAbsentString`. Calling `build()` after more `extend` calls refreshes them; the fields are read-only to callers.
- A pattern walk costs `O(m log(S+1))` (map) or `O(m)` (dense). Absent patterns give `findNode = -1`, `occurrenceCount = 0`, `firstOccurrence = lastOccurrence = -1` and empty `occurrences`. The empty pattern is the root: `occurrenceCount = n + 1`, `firstOccurrence = 0`, `lastOccurrence = n`, `occurrences = 0..n` (boundary positions, unlike `07` `patternRange`, which omits `n`).
- `occurrences(p)` returns every start in ascending order: the original states in the link subtree of `p`'s state are exactly its end positions, and every clone has at least two children, so the subtree has at most `2 out` states. `O(m log(S+1) + out log(out+1))` (final sort), `O(out)` workspace.
- `endposSize(v)` is `|endpos(v)|`. `minimalLength(v) = len(link(v)) + 1` (`0` for the root), so state `v` holds exactly the lengths `[minimalLength(v), len]`.
- `kthSubstringDistinct(k)` and `kthSubstring(k)` (0-based `k`; with multiplicity the order covers all `n(n+1)/2` occurrences, equal strings adjacent) return `{start, length}` with `start` the first occurrence, or `{-1, 0}` when `k < 0` or `k` is at least the number of substrings. They descend from the root in byte order, subtracting `paths` or `weighted` of skipped children: `O(L * S)` dense, `O(L log(S+1) + visited transitions)` map, for answer length `L`. `22` owns the suffix-array versions.
- `lexicographicWalk(visit)` calls `visit(string_view word, int state)` for every distinct nonempty substring in byte order and stops when `visit` returns `false`; it then returns `false`, and a complete walk returns `true`. The view dies when `visit` returns. `O(out * S)` dense, `O(out log(S+1))` map for `out` visited words; `O(n)` stack.
- `matchingStatistics(t)`: `res[j]` is the length of the longest suffix of `t[0, j]` occurring in the text (suffix form; a two-pointer pass gives the prefix form). `longestCommonSubstring(t)` returns `{start in text, start in t, length}` of a longest common substring: the leftmost occurrence in `t` (smallest end, hence smallest start) and its first occurrence in the text, or `{0, 0, 0}` if none. Both cost `O(m log(S+1))` amortized over suffix-link moves. The tie rule differs from `07` `longestCommonSubstring`, which picks the lexicographically smallest.
- `cyclicShiftOccurrences(t)` returns the total occurrences in the text of the distinct rotations of `t` (equal rotations of a periodic `t` count once); empty `t` gives `n + 1`. It walks `t + t[0, m - 1)`, caps the match at length `m` by climbing suffix links, collects the states of length-`m` matches and sums `cnt` over distinct states (a state holds at most one string of each length). `O(m log(S+1) + m log(m+1))`, `O(m)` workspace.
- `shortestAbsentString()` returns the lexicographically smallest of the shortest strings over the automaton alphabet (`[BASE, BASE + S)` dense, all 256 bytes for maps) that are not substrings; one always exists. A DP in decreasing `len` sets `d(v) = 1` if some byte is missing at `v`, else `1 + min d(next)`, and reconstruction picks the smallest byte reaching the minimum. `O(n * S)` dense, `O(n log(S+1))` map.
- `suffixLinkTree()` returns the suffix-link tree's children lists, ascending, in `O(n)`.
- `distinctSubstringsOnline(s)` returns `n + 1` counts, `res[i]` being the distinct nonempty substrings of `s[0, i)`, using one map automaton (`O(n log(S+1))`).
- The struct has ordinary value semantics; `build(s)` resets it.

Correctness: the construction is the standard online algorithm (Blumer et al.). States are endpos classes, `len` the longest member, `link` the class of the longest suffix in another class; extending by `c` walks suffix links adding `c` transitions and clones a class when the existing `c`-target is not solid. A new state `cur` adds exactly the suffixes of the new prefix with lengths in `(len(link(cur)), len(cur)]`, and cloning partitions an existing length interval, so the distinct count and total length stay exact. An endpos size is the number of original states (one per prefix end) in a link subtree. Decreasing `len` is a child-before-parent order, since `len(link(v)) < len(v)`, and transitions raise `len`, so the same order computes path counts. Each root path spells one distinct substring, so `paths(root) - 1` counts distinct nonempty substrings and the greedy byte-order descent finds the `k`-th.

## Feature-to-Test Map

Runner: [10-suffixautomaton_tester.py](<../../96-Local Testing/07-Strings/10-suffixautomaton_tester.py>), with the [C++ suite](<../../96-Local Testing/07-Strings/10-suffixautomaton_tester.cpp>) on the real header.

| Feature/domain | Independent verification |
|---|---|
| `Node`, states, `findNode`, `isSubstring`, `step`, `minimalLength`, `endposSize` | For every distinct substring (direct enumeration with sorted starts): the state `findNode` returns; equal endpos sets share a state and distinct sets do not; states are the endpos classes plus the root; `len` is the longest member and `minimalLength` the shortest, members are contiguous and the `link` length is `minimalLength - 1`; root values; `isSubstring` on every substring and absent probe; `step` (code overload) from the root for every code and out-of-range codes against `findNode`. |
| `extend`, online counters, `build`, `build(s)`, `size` | Per-prefix substring sets after each `extend`; `firstOccurrence` before `build`; the `built` flag; an online automaton plus `build()` equals `build(s)`; `distinctSubstringsOnline` per prefix. |
| Counting and positions | `occurrenceCount`, `firstOccurrence`, `lastOccurrence`, `occurrences` against the sorted start list of every substring; the empty pattern (`n + 1`, `0`, `n`, `0..n`); absent probes (random, text plus a symbol, out-of-alphabet byte). |
| `distinctSubstrings`, `totalSubstringLength` | Set size and summed lengths; on large texts `n(n+1)/2 - sum lcp` and `n(n+1)(n+2)/6 - sum lcp(lcp+1)/2` from the verified `SuffixArray`. |
| `kthSubstringDistinct`, `kthSubstring` | Every `k` against the sorted set and multiset, plus `-1` and one past the end; on large texts random `k` against suffix-array rank scanning, with map and dense agreeing. |
| `lexicographicWalk` | The full order equals the sorted set; the reported state equals `findNode`; early stop after 0, 1 and 2 visits. |
| `matchingStatistics`, `longestCommonSubstring` | Direct longest-suffix scan per position for random probes; the witness follows the documented tie rule. |
| `cyclicShiftOccurrences` | Set of rotations with overlapping occurrence counts; empty probe; unary closed form at large sizes. |
| `shortestAbsentString` | Length-ordered lexicographic enumeration over the alphabet (dense `S = 2, 3, 26`, map over all 256 bytes); a text containing all 256 bytes (two-byte answer); large unary texts. |
| `suffixLinkTree` | Children sorted, each edge a link with increasing `len`, `V - 1` edges. |
| Alphabets and variants | Dense `S = 2 ('a')`, `S = 3 ('a', 'x')`, `S = 26`; map with NUL, 0x01, 0x7f, 0x80, 0xff. |
| Preconditions | Ten checked subprocesses require an assertion `SIGABRT`: a byte outside the dense alphabet, and each `build()`-dependent query on an unbuilt or extended-after-build automaton. |
| Large and adversarial | Unary, random binary, random 26-letter and Fibonacci texts of 3,000/200,000/700,000 bytes (quick/full/stress) against `SuffixArray`, map and dense. |

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.1 + 3.7 s, checked 34.6 + 13.4 s, ASan-UBSan 48.3 + 71.3 s. Peak 1383 MB, pre-flight quick 960 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.3 + 6.0 s, checked 46.9 + 15.9 s, ASan-UBSan 65.9 + 65.7 s. Peak 1373 MB, pre-flight quick 990 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 65.6 + 0.6 s, checked 40.3 + 7.4 s. Peak 961 MB. |
| `python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode stress --seed 42` (three suites at a time) | PASS. Optimized 0.1 + 8.2 s, checked 13.9 + 20.3 s, ASan-UBSan 19.1 + 118.5 s. Peak 1195 MB, pre-flight quick 962 MB. |

Tester runs 2026-10-09 (GCC 16.2.1 and the floor GCC 14.4.1), the latest with check counts:

```bash
python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode quick --seed 20261009  # PASS, 2 configurations, 952 cases and 550,940 checks each, 10 assertion probes
python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, 8,404 cases and 5,958,564 checks each, MEMORY peak=839MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, same counts, MEMORY peak=750MB
python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode stress --seed 20261010  # PASS, 3 configurations, 28,726 cases and 23,443,846 checks each, MEMORY peak=1101MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 114 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=2737MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261009 --no-integration  # PASS, 12 suites
python3 '96-Local Testing/03-consistency.py'  # only the then-existing out-of-scope error on 16-poly_tester.py quick discovery (P231)
```

2026-10-19 P017: full on g++ (before the final code-range cutoff) and quick on the final code passed (counts not recorded; see [07-suffixarray.md](07-suffixarray.md#commands-and-results)).

## Benchmarks

No benchmark: the online construction is the direct linear algorithm with no dispatch threshold; map or dense transitions are an explicit caller choice with the stated bounds.

## Sources

| Source | Use |
|---|---|
| [cp-algorithms, “Suffix Automaton”](https://cp-algorithms.com/string/suffix-automaton.html) | Construction, endpos/`firstpos`, counting, `k`-th substring, all occurrences through the inverse link tree, shortest absent string, two-string LCS walk. |
| [OI Wiki, 后缀自动机](https://oi-wiki.org/string/sam/) | Same applications; linear-size bounds (`2n - 1` states, `3n - 4` transitions). |
| [maspypy `suffix_automaton.hpp`](https://maspypy.github.io/library/string/suffix_automaton.hpp) | API survey only: `next` (matching step), `max_pos` (`lastOccurrence`), `len_range`, `find_node`, `pos` (left out, see notes). |
| [Nyaan `suffix-automaton.hpp`](https://nyaannyaan.github.io/library/string/suffix-automaton.hpp) | API survey only: `find`, `next`, `chd`. |

The code was written independently from the algorithm; no source code was copied. `OLD` holds no suffix-automaton implementation (archive map and text search).

## Limits and Handoffs

- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.
- Byte alphabets only; integer alphabets would need a `map<int, int>` instantiation this row does not promise. Generalized (multi-string) automata are row `28`; rollback, persistence and sliding windows are row `44`.
- The map variant costs about 48 bytes per transition plus the node; the dense variant `4S` bytes per state.

## Aliases

Search aliases (P263, grab --find): SAM (suffix automaton, 后缀自动机); 本质不同子串, 第k小子串, 最长公共子串, 后缀链接树, 子串出现次数, 最短未出现子串.

## History

- 2026-10-09: implemented and verified (P081); independent review found 1 defect and 1 naming issue, fixed.
- 2026-10-19: P017 dependent rerun after `07` moved to SA-IS (no change here): full on g++ before the final code-range cutoff and quick on the final code passed (recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results)).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added Chinese search aliases: OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
