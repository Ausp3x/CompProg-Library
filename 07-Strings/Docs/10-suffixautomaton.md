# 10-suffixautomaton.hpp — evidence

`10-suffixautomaton.hpp` implements the online suffix automaton (minimal DFA of all substrings) of one byte text with map or dense array transitions, online distinct-substring counters, a `build()` pass for endpos counting, occurrence queries, `k`-th substrings, ordered enumeration, matching statistics, longest common substring, rotation occurrences and the shortest absent string.

## Contracts

### BasicSuffixAutomaton, SuffixAutomaton, SuffixAutomatonDense

- `BasicSuffixAutomaton<S, BASE>`: `S = 0` (alias `SuffixAutomaton`) stores transitions in `map<unsigned char, int>` and accepts every byte; `S > 0` (alias `SuffixAutomatonDense<S = 26, BASE = 'a'>`) stores `array<int, S>` over bytes `[BASE, BASE + S)`, with `0` as "no transition" (the root is never a transition target). Bytes are read as `unsigned char`; embedded NUL is valid. `A` is the effective alphabet size (`256` for maps).
- Text length `n < 2^30` (asserted in `extend`), so the at most `2n - 1` states, all lengths and all counts fit `int`; `distinctSubstrings()` is exact `lng` (`<= n(n+1)/2`), `totalSubstringLength()` is exact `lll` (`<= n(n+1)(n+2)/6`).
- `Node`: `len` is the length of the longest string of the state, `link` the suffix link (`-1` at the root), `end` the end position of the first occurrence (`firstpos`, `-1` at the root), `next` the transitions. State `0` is the root; a state is original (created by `extend`, not a clone) exactly when `end + 1 == len` and it is not the root.
- `extend(c)` appends one byte online (`O(log(S+1))` amortized for maps, `O(S)` for dense arrays because a new state zero-initializes its array and a clone copies it) and updates `distinct` and `total` in `O(1)` (each new state contributes the lengths `(len(link), len]`; a clone only splits an existing range). It clears `built`. `step(u, c)` (byte) and `step(u, x)` (code `x = byte - BASE`) return the transition or `-1`; out-of-range bytes and codes give `-1`. A `uint8_t` argument selects the code overload (integral promotion beats conversion to `char`); pass a `char` for bytes.
- `build()` (`O(n)` map, `O(n * S)` dense; `O(n)` stored) sorts states by `len` (counting sort), computes `cnt` (endpos sizes: originals 1, accumulated along suffix links; the root is set to `n + 1`, the occurrences of the empty string), `last_end` (maximum end position in the link subtree), the link-tree children in CSR form, `paths` (number of distinct paths from a state, including the empty one) and `weighted` (sum of `cnt` over nonempty paths). `build(s)` resets the object, reserves `2n + 1` states, extends every byte and calls `build()`. The string constructor calls `build(s)`.
- Queries valid at any time: `size()` (`n`), `findNode`, `isSubstring`, `firstOccurrence`, `minimalLength`, `distinctSubstrings`, `totalSubstringLength`, `lexicographicWalk`, `matchingStatistics`, `longestCommonSubstring`, `suffixLinkTree`. Queries that need `build()` (asserted through `built`, which `extend` clears): `occurrenceCount`, `lastOccurrence`, `occurrences`, `endposSize`, `kthSubstringDistinct`, `kthSubstring`, `cyclicShiftOccurrences`, `shortestAbsentString`. Calling `build()` again after more `extend` calls refreshes them; the fields are read-only to callers.
- Pattern queries cost `O(m log(S+1))` (map) or `O(m)` (dense) for the walk. Absent patterns: `findNode = -1`, `occurrenceCount = 0`, `firstOccurrence = lastOccurrence = -1`, empty `occurrences`. The empty pattern is the root: `occurrenceCount = n + 1`, `firstOccurrence = 0`, `lastOccurrence = n`, `occurrences = 0..n` (boundary positions, unlike `07` `patternRange`, which omits `n`).
- `occurrences(p)` returns all starts in ascending order: the original states in the link subtree of `p`'s state are exactly its end positions; every clone has at least two children, so the subtree has at most `2 out` states. Cost `O(m log(S+1) + out log(out+1))` (final sort), `O(out)` workspace.
- `endposSize(v)` is `|endpos(v)|`; `minimalLength(v) = len(link(v)) + 1` (`0` for the root), so state `v` holds exactly the lengths `[minimalLength(v), len]`.
- `kthSubstringDistinct(k)` and `kthSubstring(k)` (`k` 0-based; with multiplicity orders all `n(n+1)/2` occurrences, equal strings adjacent) return `{start, length}` with `start` the first occurrence; `{-1, 0}` when `k < 0` or `k` is at least the number of substrings. They descend from the root in byte order subtracting `paths` or `weighted` of skipped children: `O(L * S)` dense, `O(L log(S+1) + visited transitions)` map, for answer length `L`. The SA versions are owned by `22`.
- `lexicographicWalk(visit)` calls `visit(string_view word, int state)` for every distinct nonempty substring in lexicographic (byte) order and stops when `visit` returns `false` (the function then returns `false`; a complete walk returns `true`). The view is invalidated after `visit` returns. Cost `O(out * S)` dense, `O(out log(S+1))` map for `out` visited words; `O(n)` stack.
- `matchingStatistics(t)`: `res[j]` is the length of the longest suffix of `t[0, j]` occurring in the text (suffix form; the prefix form follows by a two-pointer pass). `longestCommonSubstring(t)` returns `{start in text, start in t, length}` of a longest common substring: the leftmost occurrence in `t` (smallest end, hence smallest start) and its first occurrence in the text; `{0, 0, 0}` if none. Both cost `O(m log(S+1))` amortized over suffix-link moves. This tie rule differs from `07` `longestCommonSubstring`, which picks the lexicographically smallest.
- `cyclicShiftOccurrences(t)` returns the total number of occurrences in the text of the distinct rotations of `t` (equal rotations of a periodic `t` count once); empty `t` gives `n + 1`. It walks `t + t[0, m - 1)`, keeps a match capped to length `m` by climbing suffix links, collects the states of length-`m` matches and sums `cnt` over the distinct states (a state contains at most one string of each length). `O(m log(S+1) + m log(m+1))`, `O(m)` workspace.
- `shortestAbsentString()` returns the lexicographically smallest among the shortest strings over the automaton alphabet (`[BASE, BASE + S)` dense, all 256 bytes for maps) that are not substrings; it always exists. DP in decreasing `len` order: `d(v) = 1` if some byte is missing at `v`, else `1 + min d(next)`; reconstruction picks the smallest byte achieving the minimum. `O(n * S)` dense, `O(n log(S+1))` map.
- `suffixLinkTree()` returns the children lists of the suffix-link tree, ascending, in `O(n)`.
- `distinctSubstringsOnline(s)` returns `n + 1` counts: `res[i]` is the number of distinct nonempty substrings of `s[0, i)`, using one map automaton (`O(n log(S+1))`).
- Copying, moving and reassigning the struct are ordinary value semantics; `build(s)` resets the object.

Correctness: the construction is the standard online algorithm (Blumer et al.): states are endpos equivalence classes, `len` the longest member, `link` the class of the longest suffix in another class; extending by `c` walks suffix links adding `c` transitions and splits a class by cloning when the existing `c`-target is not solid. A new state `cur` contributes the new substrings, exactly the suffixes of the new prefix with lengths in `(len(link(cur)), len(cur)]`; cloning partitions an existing length interval, so the distinct count and the total length are maintained exactly. Endpos sizes are the number of original states (one per prefix end) in a link subtree; processing states in decreasing `len` is a valid child-before-parent order because `len(link(v)) < len(v)`, and transitions satisfy `len(target) > len(source)`, so the same order computes path counts. A path from the root spells exactly one distinct substring, so `paths(root) - 1` counts distinct nonempty substrings and the greedy descent by byte order finds the `k`-th one.

## Feature-to-test map

Runnable entry: [10-suffixautomaton_tester.py](<../../96-Local Testing/07-Strings/10-suffixautomaton_tester.py>), with the actual-header [C++ suite](<../../96-Local Testing/07-Strings/10-suffixautomaton_tester.cpp>). All oracles survive `-DNDEBUG`.

| Feature/domain | Independent verification |
|---|---|
| `Node`, states, `minimalLength`, `endposSize` | For every distinct substring (direct enumeration with sorted starts): the state found by `findNode`; equal endpos sets share a state and distinct sets do not; states = endpos classes + root; `len` = longest member, `minimalLength` = shortest, members contiguous, `link` length = `minimalLength - 1`; root values. |
| `extend`, online counters, `build`, `build(s)`, `size` | Per-prefix substring sets after each `extend`; `firstOccurrence` before `build`; `built` flag; online automaton plus `build()` equals `build(s)`; `distinctSubstringsOnline` per prefix. |
| Counting and positions | `occurrenceCount`, `firstOccurrence`, `lastOccurrence`, `occurrences` against the sorted start list of every substring; empty pattern (`n + 1`, `0`, `n`, `0..n`); absent probes (random, text plus a symbol, out-of-alphabet byte). |
| `distinctSubstrings`, `totalSubstringLength` | Set size and summed lengths; large texts: `n(n+1)/2 - sum lcp` and `n(n+1)(n+2)/6 - sum lcp(lcp+1)/2` from the verified `SuffixArray`. |
| `kthSubstringDistinct`, `kthSubstring` | Every `k` against the sorted set and sorted multiset (also `-1` and one past the end); large texts: random `k` against suffix-array rank scanning, map and dense agreement. |
| `lexicographicWalk` | Full order equals the sorted set; reported state equals `findNode`; early stop after 0, 1, 2 visits. |
| `matchingStatistics`, `longestCommonSubstring` | Direct longest-suffix scan per position for random probes; witness with the documented tie rule. |
| `cyclicShiftOccurrences` | Set of rotations with overlapping occurrence counts; empty probe; unary closed form at large sizes. |
| `shortestAbsentString` | Length-ordered lexicographic enumeration over the alphabet (dense `S = 2, 3, 26`, map over all 256 bytes); a text containing all 256 bytes (answer has two bytes); unary large texts. |
| `suffixLinkTree` | Children sorted, each edge is a link with increasing `len`, `V - 1` edges. |
| Alphabets and variants | Dense `S = 2 ('a')`, `S = 3 ('a', 'x')`, `S = 26`; map with NUL, 0x01, 0x7f, 0x80, 0xff. |
| Preconditions | Ten checked subprocesses require assertion `SIGABRT`: byte outside the dense alphabet, and each `build()`-dependent query on an unbuilt or extended-after-build automaton. |
| Large/adversarial | Unary, random binary, random 26-letter and Fibonacci texts of 3,000/200,000/700,000 bytes (quick/full/stress) against `SuffixArray`, map and dense. |

## Commands and results

Run 2026-10-09 on an 11th Gen Intel Core i9-11900H with GCC 16.2.1 (`g++`) and the floor compiler GCC 14.4.1 (`CXX=g++-14`). Configurations (shared string runner): optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG` (with the assertion probes), and in full/stress `-O1 -g -fsanitize=address,undefined`; all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress runs passed the quick pre-flight under the 2048 MB cap; `MEMORY peak` is the run's capped peak (4096 MB cap, 0 OOM kills).

```bash
python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode quick --seed 20261009  # PASS, 2 configurations, 952 cases and 550,940 checks each, 10 assertion probes
python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, 8,404 cases and 5,958,564 checks each, MEMORY peak=839MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode full --seed 20261009  # PASS, 3 configurations, same counts, MEMORY peak=750MB
python3 '96-Local Testing/07-Strings/10-suffixautomaton_tester.py' --mode stress --seed 20261010  # PASS, 3 configurations, 28,726 cases and 23,443,846 checks each, MEMORY peak=1101MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 114 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak=2737MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261009 --no-integration  # PASS, 12 suites
python3 '96-Local Testing/03-consistency.py'  # only the pre-existing, out-of-scope error 'Renamed/new verified suites missing from quick discovery' (16-poly_tester.py, introduced by P231)
```

## Benchmarks

No benchmark: the online construction is the direct linear algorithm with no dispatch threshold; map versus dense transitions is an explicit caller choice with the stated bounds.

## Sources

| Source | Use |
|---|---|
| [cp-algorithms, “Suffix Automaton”](https://cp-algorithms.com/string/suffix-automaton.html) | Construction, endpos/`firstpos`, counting, `k`-th substring, all occurrences through the inverse link tree, shortest absent string, two-string LCS walk. |
| [OI Wiki, 后缀自动机](https://oi-wiki.org/string/sam/) | Same applications; linear-size bounds (`2n - 1` states, `3n - 4` transitions). |
| [maspypy `suffix_automaton.hpp`](https://maspypy.github.io/library/string/suffix_automaton.hpp) | API survey only: `next` (matching step), `max_pos` (`lastOccurrence`), `len_range`, `find_node`, `pos` (left out, see notes). |
| [Nyaan `suffix-automaton.hpp`](https://nyaannyaan.github.io/library/string/suffix-automaton.hpp) | API survey only: `find`, `next`, `chd`. |

The code was written independently from the algorithm; no source code was copied. No legacy suffix-automaton implementation exists in `OLD` (archive map and text search).

## Limits and handoffs

- Byte alphabets only; integer alphabets would need a `map<int, int>` instantiation that this row does not promise. Generalized (multi-string) automata are `28`; rollback, persistence and sliding windows are `44`.
- The map variant costs about 48 bytes per transition plus the node; the dense variant `4S` bytes per state.

## History

- 2026-10-09: implemented and verified (P081). Independent review: map `step(u, x)` now range-checks codes (regression test over every code and out-of-range values); state-count locals renamed.
