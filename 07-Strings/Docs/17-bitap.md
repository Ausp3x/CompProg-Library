# 17-bitap.hpp — Evidence

`17-bitap.hpp` (batch ST08, package P084) holds the bit-parallel matchers: pattern masks (plain patterns and character classes), Shift-And and Shift-Or for patterns of at most 64 symbols, multiword Shift-Or with active-word tracking, k-mismatch and k-edit (Wu–Manber) Bitap for any pattern length, and `BitsetText`, a text stored as per-symbol position bitsets that answers occurrence queries under point updates (the Codeforces 914F technique). Contest profile, GNU C++20, plain `ulng` words, no ISA code. Myers' bit-vector edit distance and bit-parallel LCS stay in `18` and `19`.

## Contracts

Symbols are `char` (read as unsigned bytes, default `S = 256`) or integers in `[0, S)`; pattern and text symbols are asserted in range once at entry. Sizes are below `INT_MAX`. Exact searches return all starts ascending with overlaps; the empty pattern gives `0..n`. Every search has two overloads: `f(masks, text, ...)` takes a prebuilt `PatternMasks` (reusable across texts, and the only way to search character classes), and `f(pattern, text, ..., S = 256)` builds the masks first.

### PatternMasks, patternMasks

`PatternMasks{m, sigma, words, bits}` stores `words = ceil(m / 64)` words per symbol: bit `i % 64` of `bits[c * words + i / 64]` is set when pattern position `i` accepts `c` (Shift-And convention). `masks[c]` points to symbol `c`'s words and `accepts(i, c)` reads one bit. Memory O(S * words).

- `patternMasks(pattern, S)` sets one symbol per position, in O(m + S * words).
- `patternMasks(classes, S)` accepts at position `i` exactly the symbols listed in `classes[i]`, in O(sum |classes[i]| + S * words). Duplicates are harmless, an empty class accepts nothing and a full class is a wildcard.

### shiftAnd, shiftOr

`m <= 64` (asserted), O(n) scan. Shift-And keeps `D = ((D << 1) | 1) & mask[c]` and reports when bit `m - 1` is set. Shift-Or keeps the complement `D = (D << 1) | ~mask[c]` (precomputed per symbol in O(S)) and reports when bit `m - 1` is clear.

### shiftOrMultiword

Any `m`. The Shift-Or state spans `words` words. Words above the highest one holding a live prefix (a zero bit) are all ones and unchanged by a step, so each step updates only words `0..top + 1` and then lowers `top` past all-ones words. Bits above `m - 1` in the last word stay one because the complemented masks are one there. Worst case O(n * words), for example a unary text and pattern; when live prefixes rarely exceed `a * 64` symbols a step costs O(a + 1).

### bitapHamming(…, k)

Returns the starts `s` in `[0, n - m]` with at most `k` positions `i` where `pattern[i]` does not accept `text[s + i]`. Rows `R_0..R_k` run Shift-And with error counts, `R'_d = ((R_d << 1 | 1) & mask[c]) | (R_{d-1} << 1 | 1)`, updated from `d = k` down so the previous row is still old. `k >= 0` (asserted); `k > m` is clamped to `m`. O(n * (k + 1) * words) time, O(k * words + out) memory.

### bitapEdit(…, k)

Wu–Manber: returns every end `e` in `[0, n]` such that some substring `text[s, e)` is within Levenshtein distance `k` of the pattern; a class mismatch costs one substitution. Initial rows have `d` low bits set (deleting the first `d` pattern symbols), so `e = 0` is reported exactly when `m <= k`. The update for ascending `d` is `R'_d = ((R_d << 1 | 1) & mask[c]) | R_{d-1} | (R_{d-1} << 1 | 1) | (R'_{d-1} << 1 | 1)` (match, insertion, substitution, deletion). Same bounds as `bitapHamming`. For large `k`, Myers' bit-vector search (O(n * ceil(m / w)), independent of `k`) is asymptotically better; it is this row's missing `myersSearch`, while `18` owns Myers' edit distance. This function is the named Wu–Manber algorithm.

### BitsetText

`BitsetText(s, S = 256)` stores the symbols and one bitset of `ceil(n / 64)` words per symbol, O(n + S * n / 64) setup and memory.

- `get(i)` and `set(i, c)` (`0 <= i < n`, `0 <= c < S`, asserted) are O(1).
- `count(pattern, l = 0, r = -1)` and `occurrences(pattern, l, r)` consider the starts `i` in `[l, r - m]`, that is the occurrences inside `[l, r)`; exactly `r = -1` means `n`, otherwise `0 <= l <= r <= n` (asserted). The candidate words covering those starts are intersected with `bits[pattern[j]] >> j` for every `j`, so a query costs O(m * ((r - l) / 64 + 1)). Pattern symbols outside `[0, S)` match nothing; the empty pattern occurs at every start in `[l, r]`. `count` returns `int`, `occurrences` the starts ascending.
- `scan(pattern, l, r, f)` exposes the raw candidate words: in `f(w, x)`, bit `b` of `x` is start `64 * w + b`.
- Queries do not mutate; `set` changes only later answers.

## Feature-to-Test Map

Runner: [`17-bitap_tester.py`](<../../96-Local Testing/07-Strings/17-bitap_tester.py>). Oracles: brute window scans (exact and Hamming), Sellers' semi-global edit DP (checked against brute Levenshtein over every substring on small cases), and brute substring scans for `BitsetText`.

| Operation | Coverage |
|---|---|
| patternMasks (both), PatternMasks::accepts | Every case, every position and symbol against the class lists. |
| shiftAnd, shiftOr | Every case with `m <= 64`, masks and pattern overloads; byte strings; large binary texts at `m` = 1, 63, 64. |
| shiftOrMultiword | Every case (up to 200 symbols, four words); a large binary text at `m` = 1, 63, 64, 65, 130, 1000; a unary 300-symbol pattern over a unary text. |
| bitapHamming, bitapEdit | Every case for `k` in {0, 1, 2, 3, m/2, m-1, m, m+2} (deduplicated), both overloads; brute Levenshtein on exhaustive cases; a large binary text (20000/100000/500000 symbols) at `m <= 130`. |
| Character classes | Exhaustive ternary class patterns (each position any of the 8 subsets, empty and wildcard included) and random class patterns. |
| BitsetText (get, set, count, occurrences; `scan` only through `count` and `occurrences`) | 40/500/5000 random texts (quick/full/stress) up to 400 symbols, alphabets 1..3 and 256, 30 interleaved `set`/`get`/range queries each, patterns up to 150 symbols, default ranges, foreign symbols (`-1`, `S`), byte-string construction; a 20000/200000-symbol binary text with updates. |

Exhaustive cases: binary patterns through length 3/4/5 against binary texts through 5/7/8, and ternary class patterns through length 2 against texts through 4/4/5. Random cases: 100/1500/6000, alphabets 1..4 and 256, half with `m < 70` and half up to 200, a third periodic. Checked builds run nine precondition probes: Shift-And and Shift-Or with `m = 65`; text, pattern and class symbols outside the alphabet; negative `k`; `BitsetText::set` out of range; `l > r`; `r = -5`.

## Commands and Results

Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan-UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>), on an Intel Core i9-11900H with GNU++20. The checked build runs stress at full sizes.

Library-wide runs 2026-11-10 (P274; seed 42, beside the other suites, build cache emptied before each cold run), this suite's results:

| Command | Result |
|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1) | PASS. Compile + run: optimized 0.3 + 2.5 s, checked 16.6 + 47.2 s, ASan-UBSan 19.0 + 40.4 s. Peak 918 MB, pre-flight quick 739 MB. |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1) | PASS. Optimized 0.2 + 3.6 s, checked 20.1 + 88.4 s, ASan-UBSan 20.1 + 70.5 s. Peak 997 MB, pre-flight quick 806 MB. |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS. Optimized 21.2 + 0.7 s, checked 14.5 + 33.9 s. Peak 744 MB. |
| `python3 '96-Local Testing/07-Strings/17-bitap_tester.py' --mode stress --seed 42` (three suites at a time) | PASS. Optimized 0.1 + 3.7 s, checked 5.8 + 19.2 s, ASan-UBSan 6.1 + 63.5 s. Peak 895 MB, pre-flight quick 48 MB. |

Tester runs 2026-10-11 on the final code, the latest with check counts:

```bash
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 20 suites (this one: 2 configurations, 9 assertion probes)
python3 '96-Local Testing/07-Strings/17-bitap_tester.py' --mode full --seed 1  # PASS, 3 configurations, 8,058,159 checks each, MEMORY peak 780 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/17-bitap_tester.py' --mode full --seed 2  # PASS, 3 configurations, 7,526,189 checks each, MEMORY peak 721 MB
python3 '96-Local Testing/07-Strings/17-bitap_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 31,339,657 checks (optimized, ASan/UBSan), 7,578,493 (checked), MEMORY peak 845 MB
python3 '96-Local Testing/02-integration.py'  # PASS, 129 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/02-integration.py' --sanitizers  # FAIL outside this package: the Core 16-poly_tester.cpp self-test is linked alone and misses its second translation unit (otherUnitConvolution); all earlier stages passed
python3 '96-Local Testing/03-consistency.py'  # PASS, errors: []
```

## Benchmarks

Each function is one algorithm, with no dispatch threshold and no Barrett or Montgomery reduction; the single-word and multiword functions keep separate names (the row's contract), so no benchmark is required.

## Sources

Code is independently implemented from the cited descriptions.

- [Lecroq, Shift-Or](https://www-igm.univ-eiffel.fr/~lecroq/string/node6.html) (fetched 2026-10-10): Shift-Or with complemented masks, `m <= w`.
- [Wikipedia, Bitap algorithm](https://en.wikipedia.org/wiki/Bitap_algorithm) (fetched 2026-10-10): exact, k-mismatch and Wu–Manber k-error recurrences; character classes credited to Baeza-Yates–Gonnet.
- [G. Navarro, "Pattern Matching", J. Applied Statistics 2004](https://users.dcc.uchile.cl/~gnavarro/ps/jstat04.pdf) (pages 1–6 and 8–13 read 2026-10-10): classes, multiword simulation, error recurrences.
- Codeforces 914F, statement read 2026-11-22 ([ledger](00-sources.md#fetched-2026-11-22-source-check)): point updates `1 i c` and queries `2 l r y` counting the occurrences of `y` inside `s[l, r]` (1-based), `|s|, q ≤ 10^5`, total `|y| ≤ 10^5`. That is `BitsetText::set` and `count(y, l - 1, r)`. The statement names no technique; the bitset AND comes from search results, and the cost rests on the contract's own argument.
- Catalog sweep in [00-sources.md](00-sources.md#p084-sweeps-2026-10-10).

## Limits and Handoffs

- Probe passes predate the 2026-11-12 runner fix; the package's next run rechecks them.
- Not adopted (reasons in [00-notes.md](00-notes.md)): multi-pattern Shift-And, optional or repeatable symbols and bounded gaps.
- Glushkov regex simulation is row `24` (`Nfa: fromRegex (Glushkov)`) and BNDM row `49` (`bndm`), both planned.
- `BitsetText` memory is `S * n / 8` bytes, which for byte texts (`S = 256`) is `32 n` bytes.
- `BitsetText::scan` has no direct test; it is exercised only through `count` and `occurrences` (open item).

## Aliases

Search aliases (P263, grab --find): 位并行匹配, 近似匹配, 模糊匹配, 近似字符串匹配, 汉明距离匹配.

## History

- 2026-10-11: implemented and verified (P084), independent review found 1 defect, fixed.
- 2026-11-07: P263 added Chinese search aliases (`grab --find`: common operation names, Simplified plus Taiwan or Hong Kong variants); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` (`-Wall -Wextra -Wconversion -Werror`, 10 s quick bound), inputs and checks unchanged; the library-wide runs above passed.
