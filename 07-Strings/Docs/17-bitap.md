# 17-bitap.hpp — evidence

`17-bitap.hpp` (batch ST08, package P084) holds the bit-parallel matchers: pattern masks (plain patterns and character classes), Shift-And and Shift-Or for patterns of at most 64 symbols, multiword Shift-Or with active-word tracking, k-mismatch and k-edit (Wu–Manber) Bitap for any pattern length, and `BitsetText`, a text stored as per-symbol position bitsets that answers occurrence queries under point updates (the Codeforces 914F technique). Contest profile, GNU C++20, plain `ulng` words, no ISA code. Myers' bit-vector edit distance and bit-parallel LCS stay in `18` and `19`.

## Contracts

Symbols are `char` (read as unsigned bytes, `S = 256` default) or integers in `[0, S)`; pattern and text symbols are asserted in range once at entry. Sizes are below `INT_MAX`. Exact searches return all starts ascending with overlaps; the empty pattern gives `0..n`. Every search has two overloads: `f(masks, text, ...)` takes a prebuilt `PatternMasks` (reusable across texts, and the only way to search character classes) and `f(pattern, text, ..., S = 256)` builds the masks first.

### PatternMasks, patternMasks

`PatternMasks{m, sigma, words, bits}` stores `words = ceil(m / 64)` words per symbol, `bits[c * words + i / 64]` bit `i % 64` set when pattern position `i` accepts `c` (Shift-And convention); `masks[c]` points to symbol `c`'s words and `accepts(i, c)` reads one bit. `patternMasks(pattern, S)` sets one symbol per position in `O(m + S * words)`; `patternMasks(classes, S)` accepts at position `i` exactly the symbols listed in `classes[i]` (duplicates harmless, an empty class accepts nothing, a full class is a wildcard), in `O(sum |classes[i]| + S * words)`. Memory `O(S * words)`.

### shiftAnd, shiftOr

`m <= 64` (asserted). Shift-And keeps `D = ((D << 1) | 1) & mask[c]` and reports when bit `m - 1` is set; Shift-Or keeps the complement `D = (D << 1) | ~mask[c]` (precomputed per symbol, `O(S)`) and reports when bit `m - 1` is clear. `O(n)` scan.

### shiftOrMultiword

Any `m`. The Shift-Or state spans `words` words; words above the highest word holding a live prefix (a zero bit) are all ones and unchanged by a step, so each step updates only words `0..top + 1` and then lowers `top` past all-ones words. Bits above `m - 1` in the last word stay one because the complemented masks are one there. Worst case `O(n * words)` (for example a unary text and pattern); when live prefixes rarely exceed `a * 64` symbols the step costs `O(a + 1)`.

### bitapHamming(…, k)

Starts `s` in `[0, n - m]` with at most `k` positions `i` where `pattern[i]` does not accept `text[s + i]`. Rows `R_0..R_k` (Shift-And with error counts): `R'_d = ((R_d << 1 | 1) & mask[c]) | (R_{d-1} << 1 | 1)`, updated from `d = k` down so the previous row is still old. `k >= 0` (asserted); `k > m` is clamped to `m`. `O(n * (k + 1) * words)` time, `O(k * words + out)` memory.

### bitapEdit(…, k)

Wu–Manber: returns every end `e` in `[0, n]` such that some substring `text[s, e)` is within Levenshtein distance `k` of the pattern (a class mismatch costs one substitution). Initial rows have `d` low bits set (deleting the first `d` pattern symbols), so `e = 0` is reported exactly when `m <= k`. Update for `d` ascending: `R'_d = ((R_d << 1 | 1) & mask[c]) | R_{d-1} | (R_{d-1} << 1 | 1) | (R'_{d-1} << 1 | 1)` (match, insertion, substitution, deletion). Same bounds as `bitapHamming`. For large `k`, Myers' bit-vector search (`O(n * ceil(m / w))`, independent of `k`; `18` owns Myers' edit distance) is asymptotically better; this function is the named Wu–Manber algorithm.

### BitsetText

`BitsetText(s, S = 256)` stores the symbols and one bitset of `ceil(n / 64)` words per symbol (`O(n + S * n / 64)` setup and memory). `get(i)` and `set(i, c)` (`0 <= i < n`, `0 <= c < S`, asserted) are `O(1)`. `count(pattern, l = 0, r = -1)` and `occurrences(pattern, l, r)` consider starts `i` in `[l, r - m]`, that is occurrences lying inside `[l, r)` (exactly `r = -1` means `n`; otherwise `0 <= l <= r <= n` asserted): the candidate words covering those starts are intersected with `bits[pattern[j]] >> j` for every `j`, so a query costs `O(m * ((r - l) / 64 + 1))`. Pattern symbols outside `[0, S)` match nothing; the empty pattern occurs at every start in `[l, r]`. `count` returns `int`, `occurrences` the starts ascending. `scan(pattern, l, r, f)` exposes the raw candidate words (`f(w, x)`, bit `b` of `x` is start `64 * w + b`). Queries do not mutate; `set` changes later answers only.

## Feature-to-test map

Runner: [`17-bitap_tester.py`](<../../96-Local Testing/07-Strings/17-bitap_tester.py>). Oracles: brute window scans (exact and Hamming), Sellers' semi-global edit DP checked against brute Levenshtein over every substring on small cases, and brute substring scans for `BitsetText`. All checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| patternMasks (both), PatternMasks::accepts | Every case, every position and symbol against the class lists. |
| shiftAnd, shiftOr | Every case with `m <= 64`, masks and pattern overloads; byte strings; large binary texts at `m` = 1, 63, 64. |
| shiftOrMultiword | Every case (up to 200 symbols, four words); large binary text at `m` = 1, 63, 64, 65, 130, 1000; unary 300-symbol pattern over a unary text. |
| bitapHamming, bitapEdit | Every case for `k` in {0, 1, 2, 3, m/2, m-1, m, m+2} (deduplicated), both overloads; brute Levenshtein on exhaustive cases; large binary text (50000/100000/500000 symbols) at `m <= 130`. |
| Character classes | Exhaustive ternary class patterns (each position any of the 8 subsets, including empty and wildcard) and random class patterns. |
| BitsetText | 100/500/5000 random texts (quick/full/stress) up to 400 symbols, alphabets 1..3 and 256, 30 interleaved `set`/`get`/range queries each, patterns up to 150 symbols, default ranges, foreign symbols (`-1`, `S`), byte-string construction; a 20000/200000-symbol binary text with updates. |

Exhaustive: binary patterns through length 3/4/5 against binary texts through 5/7/8; ternary class patterns through length 2 against texts through 4/4/5. Random: 300/1500/6000 cases, alphabets 1..4 and 256, half with `m < 70` and half up to 200, a third periodic. Checked builds run nine precondition probes (Shift-And/Shift-Or with `m = 65`, text, pattern and class symbols outside the alphabet, negative `k`, `BitsetText::set` out of range, `l > r`, `r = -5`).

## Commands and results

Run 2026-10-11 on the final code. Every run builds optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`, precondition probes; stress runs at full sizes) binaries, and full/stress add ASan/UBSan (`-O1`); all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress first pass the quick pre-flight under the 2048 MB cap.

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

No dispatch threshold and no Barrett/Montgomery: each function is one algorithm. The single-word functions and the multiword one keep separate names (the row's contract), so no benchmark is required.

## Sources

Code is independently implemented from the cited descriptions.

- [Lecroq, Shift-Or](https://www-igm.univ-eiffel.fr/~lecroq/string/node6.html) (fetched 2026-10-10): Shift-Or with complemented masks, `m <= w`.
- [Wikipedia, Bitap algorithm](https://en.wikipedia.org/wiki/Bitap_algorithm) (fetched 2026-10-10): exact, k-mismatch and Wu–Manber k-error recurrences; character classes credited to Baeza-Yates–Gonnet.
- [G. Navarro, "Pattern Matching", J. Applied Statistics 2004](https://users.dcc.uchile.cl/~gnavarro/ps/jstat04.pdf) (pages 1–6 and 8–13 read 2026-10-10): classes, multiword simulation, error recurrences.
- Codeforces 914F (bitset per character, occurrences by AND of shifted bitsets, point updates): statement not fetched (403); technique from search results only.
- Catalog sweep in [00-sources.md](00-sources.md#p084-sweeps-2026-10-10).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): multi-pattern Shift-And, optional/repeatable symbols and bounded gaps, Glushkov regex simulation (`24`), BNDM (`49`).
- `BitsetText` memory is `S * n / 8` bytes; for byte texts (`S = 256`) that is `32 n` bytes.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-11: implemented and verified (P084), independent review found 1 defect, fixed.
