# 19-lcs.hpp — evidence

`19-lcs.hpp` (batch ST34, package P083) covers longest common subsequences and the subsequence counts listed with them: bit-parallel length, Hunt–Szymanski for sparse matches, witnesses in linear memory (Hirschberg over bit-parallel or scalar rows), Myers' linear-space shortest edit script, the shortest common supersequence, three-sequence LCS, the longest palindromic subsequence, and distinct, palindromic-distinct and palindromic-multiset subsequence counts. Contest profile, GNU C++20, plain 64-bit word operations (no ISA code).

## Contracts

Inputs are random-access sequences with `.size()` below `INT_MAX` (`string`, `string_view`, `vector`); two-sequence functions require the same value type (static assertion). `char` symbols are treated as unsigned bytes. "Ordered" means `std::totally_ordered` value type whose `==` agrees with the order; "equality only" means `==` alone. No input is retained or modified. Witnesses are ascending index pairs `(i, j)` with `a[i] == b[j]`, strictly increasing in both coordinates; which maximum common subsequence is returned is deterministic but unspecified.

### bitsetLcs, lcs

`bitsetLcs(a, b)` (ordered symbols) returns the LCS length with the Allison–Dix / Hyyrö recurrence `V = (V + (V & M[c])) | (V & ~M[c])` over `ceil(m / 64)` words per symbol of `a`, where `M[c]` marks the positions of `c` in `b` and the LCS of the processed prefix with `b[0, j)` is the number of zero bits below `j`. Symbols are compressed by sorting (bytes map directly to 256 ids); a symbol occurring at least `ceil(m / 64)` times in `b` gets a cached dense mask, a rarer one is written into a scratch row and cleared after use, so each row costs O(ceil(m / 64)) and the masks take O(m) words in total. Time O(n * ceil(m / w) + (n + m) log(n + m)) (no log term for bytes), memory O(n + m + S).

`lcs(a, b)` dispatches: ordered symbols use `bitsetLcs`; equality-only symbols use the scalar DP in O(n * m) time and O(m) memory, optimal in the equality-comparison model (Aho–Hirschberg–Ullman).

### lcsWitness, hirschbergLcs

`hirschbergLcs(a, b)` (equality only) returns a witness in O(n * m) time and O(n + m) memory: split `a` at its midpoint, compute the forward LCS row of the left half and the backward row of the right half against the current `b` range, cut `b` where their sum is maximal, recurse; rows are freed before recursing and a zero best sum stops the branch. `lcsWitness(a, b)` uses the same recursion with bit-parallel rows for ordered symbols, O(n * ceil(m / w) + (n + m) log(n + m)) time (each recursion level costs at most the previous one's half plus the O(m + n) row overhead) and O(n + m + S) memory, and falls back to `hirschbergLcs` for equality-only symbols.

### huntSzymanski

`huntSzymanski(a, b)` (ordered) returns the LCS length in O((n + m) log(n + m) + r log(min(n, m) + 1)) time, `r` the number of matching pairs, O(n + m + S) memory: for each `a[i]`, the positions of `a[i]` in `b` are visited in decreasing order and each replaces the first threshold not below it, where `tails[k]` is the smallest end in `b` of a common subsequence of length `k + 1`. Faster than `bitsetLcs` when `r` is small (large alphabets); worse on dense matches.

### DiffOp, myersDiff

`myersDiff(a, b)` (equality only, `n, m < INT_MAX / 2` asserted) returns a shortest edit script with insertions and deletions: a sequence of `DiffOp{op, i, j}` where `op` is `'='` (keep `a[i] == b[j]`), `'-'` (delete `a[i]`) or `'+'` (insert `b[j]`), and `i`, `j` are the cursors in `a` and `b` before the operation. The number of `'-'` and `'+'` operations is `D = n + m - 2 * LCS`. Linear-space Myers: find the middle snake of a `D`-path by simultaneous forward and backward furthest-reaching searches on diagonals, recurse on the two sides; subproblems with `D <= 1` or an empty side are emitted directly. O((n + m) * D) time, O(n + m + D) memory plus the output; recursion depth O(log D).

### shortestCommonSupersequence, lcs3, longestPalindromicSubsequence

`shortestCommonSupersequence(a, b)` returns a shortest sequence containing both as subsequences (`string` for `char` input, `vector` otherwise), length `n + m - LCS`, by merging along `lcsWitness`; cost of `lcsWitness` plus O(n + m). `lcs3(a, b, c)` (equality only, `(m + 1) * (k + 1) < INT_MAX` asserted) returns the three-way LCS length in O(n * m * k) time and O(m * k) memory. `longestPalindromicSubsequence(s)` is `lcs(s, reverse(s))`, so O(n * ceil(n / w) + n log n) for ordered symbols.

### countDistinctSubsequences, countPalindromicSubsequences

The result type `T` is any commutative ring with `T(0)`, `T(1)`, `+`, `-`, `+=` (a modular integer type, `ulng` for arithmetic mod 2^64, or `lng` while the exact value fits). Ordered symbols are required (compression).

`countDistinctSubsequences<T>(s)` counts distinct subsequences including the empty one: `total' = 2 * total - last[c]`, where `last[c]` is the total before the previous occurrence of `c`. O(n log n + S) time (O(n + S) for bytes), O(n + S) memory.

`countPalindromicSubsequences<T>(s, distinct)` counts nonempty palindromic subsequences: as distinct strings when `distinct`, else as index sets. Multiset: `C(i, j) = C(i + 1, j) + C(i, j - 1) - C(i + 1, j - 1) + [s_i = s_j](C(i + 1, j - 1) + 1)`, two rows, O(n^2) time and O(n) memory. Distinct: with `l` the next occurrence of `s_i` after `i` and `r` the previous occurrence of `s_j` before `j`, `D(i, j) = D(i + 1, j) + D(i, j - 1) - D(i + 1, j - 1)` when `s_i != s_j`, else `2 D(i + 1, j - 1) + 2` (no inner occurrence), `+ 1` (one) or `- D(l + 1, r - 1)` (two or more). Row `i` needs rows `i + 1` and `l + 1`; a row is kept only until its single consumer (the previous occurrence of the symbol before it) runs, so at most one pending row per symbol exists: O(n^2) time, O(n * min(n, S)) memory. Empty input gives 0.

## Feature-to-test map

Runner: [`19-lcs_tester.py`](<../../96-Local Testing/07-Strings/19-lcs_tester.py>). The oracle is an independent full-table DP; counts use subsequence enumeration; all checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| lcs, bitsetLcs, huntSzymanski | All binary pairs through length 5/6/7 (quick/full/stress); 300/2000/6000 random pairs of bytes (a fifth over 256 values, second lengths at the word boundaries 63..65, 127..129, 191..193), sparse extreme-`lng` alphabets (scratch masks), string symbols; medium strings of length 300/1200/2500 over 2/4/26/256 letters; equality-only symbols reach the scalar `lcs`; large `n = 5000/40000/60000` over 500 symbols (`bitsetLcs` against `huntSzymanski`) and a binary prefix against the table. |
| lcsWitness, hirschbergLcs | Every pair: size equals the table, matched symbols, strictly increasing pairs; equality-only symbols reach the scalar recursion; large sparse witness. |
| DiffOp, myersDiff | Every pair replayed (cursors, keeps match, consumption of both sequences, edit count `n + m - 2 LCS`); large similar binary strings with 40 edits. |
| shortestCommonSupersequence | Every pair: length `n + m - LCS` and both inputs are subsequences. |
| lcs3 | Random triples of lengths up to 10/11/11 against enumeration of the first sequence's subsequences. |
| longestPalindromicSubsequence | Enumeration (lengths up to 12), interval DP on medium strings, unary closed form at length up to 6 * 10^4. |
| countDistinctSubsequences, countPalindromicSubsequences | Enumeration with `lng` and mod 998244353 (lengths up to 12, binary exhaustively through 5/6/7, extreme `int` symbols); the distinct palindromic count against an independent per-letter recurrence `F = sum_c (1 + [two occurrences](1 + F(inner)))` on medium strings; unary closed form. |

The checked (`-O0`, `_GLIBCXX_DEBUG`) build divides the random count and the medium and large sizes by 4, because the debug library validates every binary-search range in linear time; the optimized and sanitizer builds run the full sizes. The checked build runs one precondition probe (`lcs3` with a table above `INT_MAX`); the remaining preconditions are sizes near `INT_MAX`.

## Commands and results

Run 2026-10-10 on the final code with the configurations listed in [13-palindromictree.md](13-palindromictree.md#commands-and-results); the checked build uses the size divisor described above.

```bash
python3 '96-Local Testing/07-Strings/19-lcs_tester.py' --mode quick --seed 1  # PASS, 2 configurations, 1 assertion probe
python3 '96-Local Testing/07-Strings/19-lcs_tester.py' --mode full --seed 1  # PASS, 3 configurations, 3,534,893 checks (optimized, ASan/UBSan), 1,301,083 (checked), MEMORY peak 581 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/19-lcs_tester.py' --mode full --seed 2  # PASS, 3 configurations, 3,559,647 checks (optimized, ASan/UBSan), 1,337,545 (checked), MEMORY peak 597 MB
python3 '96-Local Testing/07-Strings/19-lcs_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 11,226,793 checks (optimized, ASan/UBSan), 1,321,025 (checked), MEMORY peak 642 MB
python3 '96-Local Testing/07-Strings/19-lcs_benchmark.py'  # PASS, outputs cross-checked, MEMORY peak 331 MB (table below)
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 115 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 13 suites
```

The consistency validator result is recorded in [13-palindromictree.md](13-palindromictree.md#commands-and-results).

## Benchmarks

Driver: [`19-lcs_benchmark.py`](<../../96-Local Testing/07-Strings/19-lcs_benchmark.py>) (`19-lcs_benchmark.cpp`), run 2026-10-10 on an 11th Gen Intel Core i9-11900H, g++ 16.2.1, `-std=gnu++20 -O2 -DNDEBUG`, seed 20261010, one warmup and five repetitions, median reported; every workload checks that `bitsetLcs`, `huntSzymanski`, `lcsWitness` (and the scalar paths where run) agree. Peak memory 331 MB (driver plus compiler).

| Workload | n = m | bitsetLcs | huntSzymanski | lcsWitness | scalar DP row | hirschbergLcs |
|---|---|---|---|---|---|---|
| DNA, 4 letters | 20000 | 9.11 ms | 4393 ms | 28.8 ms | 1821 ms | 3052 ms |
| bytes, 256 letters | 50000 | 77.8 ms | 678 ms | 170 ms | — | — |
| sparse, 10^5 letters | 100000 | 180 ms | 45.9 ms | 256 ms | — | — |

| Workload | Size | Operation | Median |
|---|---|---|---|
| random binary with 10 edits | 10^6 | myersDiff | 12.9 ms |
| random binary with 1000 edits | 10^6 | myersDiff | 24.0 ms |
| 4 letters | 2000 / 5000 | countPalindromicSubsequences distinct | 24.3 / 165 ms |
| 4 letters | 2000 / 5000 | countPalindromicSubsequences multiset | 42.8 / 249 ms |

Reading: the bit-parallel row is about 200 times faster than the scalar DP on dense matches and is the `lcs` default; Hunt–Szymanski wins only when matches are rare (3.9 times faster on the sparse workload, 480 times slower on DNA), so it stays a separate named variant; the witness costs 1.4 to 3.2 times the length computation. The dense-mask threshold (`ceil(m / 64)` occurrences) is an asymptotic device that keeps each row O(m / w) and total mask storage O(m) words, not a tuned crossover.

## Sources

Code is independently implemented from the recurrences above.

- [maspypy `longest_common_subsequence.hpp`](https://maspypy.github.io/library/string/longest_common_subsequence.hpp) and [`count_subsequence.hpp`](https://maspypy.github.io/library/string/count_subsequence.hpp) (fetched 2026-10-10): length and index-pair witness; distinct subsequences counting the empty one.
- [suisen `number_of_subsequences.hpp`](https://suisen-cp.github.io/cp-library-cpp/library/dp/number_of_subsequences.hpp) (fetched 2026-10-10): distinct subsequences (excluding the empty one there).
- [PyRival `lcs.py`](https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/strings/lcs.py) (fetched 2026-10-10): LCS and longest palindromic subsequence.
- [AtCoder DP F](https://atcoder.jp/contests/dp/tasks/dp_f) and [CSES problem set](https://cses.fi/problemset/) (fetched 2026-10-10): LCS string output.
- [OI Wiki, dynamic programming basics](https://oi-wiki.org/dp/basic/) (fetched 2026-10-10): LCS DP.
- Allison–Dix (1986) and Hyyrö (2004) bit-parallel LCS, Hunt–Szymanski (1977), Hirschberg (1975), Myers (1986, "An O(ND) difference algorithm and its variations", middle snake) are named from standard knowledge; no copy of these papers was read in this package.
- Catalog sweep 2026-10-10 in [00-sources.md](00-sources.md).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): longest common increasing subsequence, an exposed full LCS table, counting LCS strings, the lexicographically smallest LCS, range distinct-subsequence counting.
- Edit distance and alignment are `18`; subsequence automata and pattern-as-subsequence counting `20`; prefix–substring (semi-local) LCS `48`; longest common substring `07`/`28`.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-10: implemented and verified (P083); independent review findings fixed: carry computation split so no operand order is relied on, DiffOp complexity line.
