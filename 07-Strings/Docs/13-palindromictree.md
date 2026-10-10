# 13-palindromictree.hpp — evidence

`13-palindromictree.hpp` (batch ST07, package P083) is the eertree (palindromic tree) of Rubinchik and Shur: append-only construction with suffix, series (difference) and parent links, a joint tree over several strings, per-position longest palindromic suffixes, substring palindromic-suffix queries, occurrence counts, and the series-link partition DPs (generic fold, minimum count with witness, even-palindrome partitions). Contest profile, GNU C++20, no ISA code.

## Contracts

### BasicPalindromicTree<S, BASE>, PalindromicTree, PalindromicTreeDense

Symbols are `int`. `S = 0` (`PalindromicTree`) stores transitions in `map<int, int>` and accepts any `int`; `S > 0` (`PalindromicTreeDense<S = 26, BASE = 'a'>`) stores `array<int, S>` and accepts symbols in `[BASE, BASE + S)` (asserted on `append`). String input (`build(string_view)`, the `string_view` constructor) appends bytes as unsigned values 0..255, so NUL and high bytes are valid with `S = 0`. The total stored length `n` (all strings of a joint tree) is at most `INT_MAX - 3` (asserted on `append`); there are at most `n + 2` nodes. Under `NDEBUG` an out-of-range dense symbol is still appended but creates no transition, so results are meaningless but memory stays in bounds.

Nodes: `0` is the imaginary root with `len = -1`, `1` the empty palindrome with `len = 0`; both have `link = 0`, `series = 0`, `parent = 0`, `pos = -1`, `diff = 0`. Every other node `v` is one distinct nonempty palindrome with fields `len`; `link` (longest proper palindromic suffix, node `1` for length 1); `diff = len - nodes[link].len`; `series` (first node on the link chain whose `diff` differs from `diff[v]`, using `diff = 0` for node `1`, so the walk `v, series[v], ...` stops at `len <= 0` after O(log n) steps); `parent` (the node obtained by deleting both end symbols, node `0` for length 1); `pos` (end position of the first occurrence, in joint-storage coordinates); `next` (children by symbol). Node ids are creation order, so `link[v] < v` and `parent[v] < v`.

Members: `s` (all appended symbols), `suffix[i]` (node of the longest palindromic suffix ending at position `i` within its string), `last` (current longest suffix palindrome), `start` (first position of the current string). They are read-only for callers.

Construction and mutation: `clear()` resets to the two roots (map storage is released; vector capacity kept); `build(t)` clears then appends every symbol (`string_view` or `vector<int>`); `append(c)` adds one symbol to the current string and returns the new `last`, amortized O(1) link walks plus one transition lookup (O(log(S + 2)) map, O(1) dense); `newString()` starts a new string in the same tree (joint eertree): later palindromes never cross the boundary, existing nodes are shared, and `start` moves to the current size. Construction time O(n log(S + 2)) map, O(n) dense (Rubinchik–Shur amortization: each link step of either walk moves the start of the candidate suffix palindrome right, and that start never moves left between appends). Memory O(n) map, O(n * S) dense.

Queries (no mutation): `size()` = number of nodes including roots; `distinctPalindromes()` = `size() - 2` (distinct nonempty palindromic substrings of all strings); `step(v, c)` = child of `v` by `c` (`c X c` for node `X`, `c` for node 0) or `-1`, including out-of-range symbols in the dense tree; `longestSuffixPalindrome(r)` for `r` in `[0, n]` (asserted) is `suffix[r - 1]`, node `1` for `r = 0`; `palindrome(v)` for `v >= 2` (asserted) is the half-open interval of the first occurrence; `substringSuffixPalindrome(l, r)` for `0 <= l <= r <= n` (asserted; `l` at or after the start of the string containing `r - 1`) is the length of the longest palindromic suffix of `s[l, r)`, O(log n): the palindromic suffixes of `s[l, r)` are those of the prefix ending at `r` that fit, and each series group is an arithmetic progression of lengths `len(series) + diff, ..., len`, so the largest fitting member is computed in O(1) per group.

Counts: `occurrenceCounts(l = 0, r = -1)` (exactly `r = -1` means `n`; otherwise `0 <= l <= r <= n` asserted) returns, per node, the number of occurrences ending at positions in `[l, r)` (per string when `[l, r)` is one string of a joint tree; the two root entries carry no meaning), O(V + r - l) by propagating counts from `suffix` along links in reverse creation order. `occurrencesAt()` returns, per position, the number of palindromic substrings ending there (the link depth of `suffix[i]`), O(V + n); their sum is the number of palindromic substrings counted with multiplicity.

Partition DPs (on the current string `s[start, n)`, local prefix lengths `0..m`; for a single-string tree that is the whole string). `palindromicFactorization(zero, one, plus, piece, even = false)` returns `res` with `res[0] = one` and `res[i] = plus` over every palindromic suffix of the prefix of length `i` of `piece(res[i - len])`, `zero` when empty; `plus` must be associative and commutative with identity `zero` and `piece` must distribute over `plus` (`piece(plus(a, b)) = plus(piece(a), piece(b))`); with `even` the odd entries are forced to `zero`, which restricts pieces to even lengths (an odd piece ending at an even position starts at an odd one). O(m log m) calls of `plus` and `piece` (series-link trick: `series[v]` accumulates the progression of `v`, reusing the value stored for `link[v]` at position `i - diff[v]`, valid because `link[v]` heads its group there). `piece(zero)` may be evaluated in `even` mode, so a minimum fold must saturate. `palindromicLength()` = minimum number of palindromes partitioning each prefix (`int`). `evenPalindromePartition<T>()` = number of partitions of each prefix into even-length palindromes in the ring `T` (exact `lng` only while counts fit; use a modular or `ulng` type otherwise). `minPalindromePartition()` returns the cuts `0 = c[0] < ... < c[k] = m` (local coordinates) of a minimum partition; ties choose the longest last piece recursively (smallest previous cut). All O(m log m) time, O(V + m) space.

## Feature-to-test map

Runner: [`13-palindromictree_tester.py`](<../../96-Local Testing/07-Strings/13-palindromictree_tester.py>). The oracle enumerates substrings and tests palindromes directly; partitions use an O(n^2) table DP; all checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| Node fields len, link, diff, series, parent, pos | Every node of every case against the brute palindrome set (first occurrence, longest proper palindromic suffix, stripped parent, brute suffix-length chain for `series`), exhaustive ternary strings through length 6/8/9 (quick/full/stress), 100/800/5000 random strings (forced palindromes, `INT_MIN`/`INT_MAX` symbols), a byte string with NUL and high bytes; Fibonacci words of length 50000/500000/1000000 checked for parent edges and link invariants. |
| clear, build, append, newString, size, distinctPalindromes | Single tree, joint tree (second string per case), `build` after another string (reset), distinct count against the brute set and closed forms (unary `n`, at most `n` for Fibonacci). |
| step | Every node and every symbol present plus an absent symbol, against the brute palindrome set. |
| longestSuffixPalindrome, suffix, substringSuffixPalindrome | Every `r` and every `[l, r)` within a string, joint trees included. |
| palindrome | First-occurrence interval of every node. |
| occurrenceCounts, occurrencesAt | Whole storage, each string of a joint tree and an arbitrary range, against brute occurrence scans; unary closed forms `n - len + 1` and `i + 1`; total palindromic substrings on medium strings. |
| palindromicFactorization | Count of all partitions (`ulng`, mod 2^64) and saturating even minimum against the table DP. |
| palindromicLength, minPalindromePartition, evenPalindromePartition | Table DP on every case and on medium random binary and Thue–Morse strings of length 400/2000/4000 (`ulng` counts); exact witness cuts with the longest-last-piece convention; unary even partitions `2^(i/2 - 1)` mod 10^9 + 7 through a minimal modular type at length up to 10^6; dense tree compared field for field with the map tree. |

Checked builds run ten precondition probes: `palindrome` on a root and past the end, `longestSuffixPalindrome` below and above range, `substringSuffixPalindrome` reversed and past the end, `occurrenceCounts` past the end and reversed, dense `append` below and above the alphabet.

## Commands and results

Run 2026-10-10 on the final code. Configurations: optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`, assertion probes; stress runs at full-mode sizes because the debug library is slow) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`), all with `-std=gnu++20 -Wall -Wextra -Wshadow -Wconversion -Werror`; local `g++` is GCC 16.2.1, the floor run uses `g++-14` (GCC 14.4.1).

```bash
python3 '96-Local Testing/07-Strings/13-palindromictree_tester.py' --mode quick --seed 1  # PASS, 2 configurations, 10 assertion probes
python3 '96-Local Testing/07-Strings/13-palindromictree_tester.py' --mode full --seed 1  # PASS, 3 configurations, 8,706,979 checks each, MEMORY peak 854 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/13-palindromictree_tester.py' --mode full --seed 2  # PASS, 3 configurations, 8,693,538 checks each, MEMORY peak 773 MB
python3 '96-Local Testing/07-Strings/13-palindromictree_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 28,632,370 checks (optimized, ASan/UBSan), 8,741,368 (checked), MEMORY peak 881 MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 115 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 2693 MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 13 suites
python3 '96-Local Testing/03-consistency.py'  # no new errors (see below)
```

Every pre-flight quick run passed under the 2048 MB cap. `03-consistency.py` cannot run inside this worktree because gitignored `OLD` binaries and a `.jsonl` log are absent; on a temporary copy of the worktree with the main checkout's `OLD` archive and `18-bitset_benchmark.jsonl` it reports no errors. After rebasing onto `master` (which added `10`, `12`, `18` to this folder) the four suites passed quick again, `02-integration.py` passed (124 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace) and the validator reported no errors.

## Benchmarks

Linear construction with no backend choice beyond the documented map/dense storage; no speed claim is made and no benchmark is required.

## Sources

Code is independently implemented from the invariants above.

- [OI Wiki, 回文树](https://oi-wiki.org/string/pam/) (fetched 2026-10-10): construction, series links, minimum palindromic partition.
- [maspypy `palindromic_tree.hpp`](https://maspypy.github.io/library/string/palindromic_tree.hpp) and [`palindrome_decomposition_dp.hpp`](https://maspypy.github.io/library/string/palindrome_decomposition_dp.hpp) (fetched 2026-10-10): first-occurrence position, substring longest palindromic suffix, generic decomposition DP over series links.
- [Library Checker `eertree`](https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/string/eertree/task.md) (fetched 2026-10-10): parent and suffix link per node, node of the longest suffix palindrome per prefix.
- [ei1333](https://ei1333.github.io/library/string/palindromic-tree.hpp), [hitonanode](https://hitonanode.github.io/cplib-cpp/string/palindromic_tree.hpp) (joint trees, yukicoder 263), [tko919](https://tko919.github.io/library/String/palindromictree.hpp) (fetched 2026-10-10).
- [Rubinchik and Shur, EERTREE (arXiv 1506.04862)](https://arxiv.org/abs/1506.04862) (pages 1–2 read): joint eertrees and the linear construction bound.
- Catalog sweep 2026-10-10 in [00-sources.md](00-sources.md).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): a separate `seriesLink` method (the `series` field), a raw per-node count field (recomputed by `occurrenceCounts`), k-factorization, per-symbol DP callbacks, rich-string enumeration, persistence (`45`), listing the end positions of one palindrome.
- `39` lists `palindromicLength (per prefix)`; the series-link implementation lives here and `39` should reuse it.
- Double-ended and rollback eertrees are `45`; range palindrome queries and palindromic characteristics are `39`.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-10: implemented and verified (P083); independent review found no wrong answers, its fixes (NDEBUG-bounded dense append and occurrenceCounts default, documented length bound) are in place.
