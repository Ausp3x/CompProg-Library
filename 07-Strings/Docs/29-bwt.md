# 29-bwt.hpp — evidence

`29-bwt.hpp` (batch ST23, package P084) implements the Burrows–Wheeler transform in its two standard representations (sentinel and rotation matrix with primary index), the inverses, the LF and psi permutations, the construction from a suffix array, cyclic rotation sorting and the Gil–Scott bijective BWT with its inverse. Contest profile, GNU C++20, no ISA code. FM-index queries are `36`; run-length BWT and the r-index are `46`.

## Contracts

Sequences have `n < INT_MAX` symbols; `char` symbols are ordered as unsigned bytes (counting sorts, `O(n)`), any other totally ordered symbol type goes through `compressAlphabet` from `12` (`O(n log n)`). Functions that return a sequence return the input container type `S`, which must be constructible from a size and a value or from an iterator range (`string`, `vector<T>`; not `string_view`). Read-only functions (`sortCyclicRotations`, `psiMapping`, `lfMapping`) accept any random-access sequence. No input is retained.

### sortCyclicRotations(s)

Returns the starts of the `n` rotations `s[i, n) + s[0, i)` in sorted order; equal rotations (periodic `s`) appear by ascending start. With `p` the smallest period of `s` that divides `n` (from the prefix function), rotations `i` and `j` are equal exactly when `i ≡ j (mod p)`; the `p` distinct rotations of the primitive root `u = s[0, p)` sort like the suffixes of `u + u` that start in `u`, read from `sais`, and each start `r` expands to `r, r + p, r + 2p, …`. `O(n)` for bytes, `O(n log n)` otherwise; empty input gives an empty order.

### bwt(s, sentinel), bwtFromSuffixArray(s, sa, sentinel)

The BWT of `s` followed by a sentinel that ranks below every symbol, as `n + 1` symbols: row `r` holds the symbol preceding the `r`-th smallest suffix of `s + $`, and the sentinel value marks the row of suffix 0. The sentinel value is only a marker and need not be the smallest symbol by value, but it must not occur in `s` (asserted, `O(n)`). `bwtFromSuffixArray` takes the sentinel-free suffix array (`n` entries, empty suffix omitted, as produced by `sais` or `SuffixArray::sa`; size asserted): row 0 is `s[n - 1]` (the empty suffix), row `r + 1` is `s[sa[r] - 1]` or the sentinel. `bwt` builds that suffix array with `sais` (`O(n)` for bytes). The empty string gives one sentinel.

### bwtPrimary(s)

The last column of the sorted rotation matrix of `s` (`n` symbols, no sentinel) and the primary index, the smallest row whose rotation equals `s`; empty input gives `({}, 0)`. Ties between equal rotations do not affect the column.

### psiMapping, lfMapping

`psiMapping(L)` is the stable sort of the rows of `L` by symbol: `psi[r]` is the row holding the `r`-th smallest symbol, so row `r` of the first column is `L[psi[r]]`, and `psi` maps a row to the row of its rotation shifted left by one. `lfMapping` is the inverse permutation (`LF[psi[r]] = r`), mapping a row to the row of its rotation shifted right by one (one text position earlier). The `(L, sentinel)` overloads rank the unique sentinel of `L` (asserted: exactly one) below every symbol and are exact for the sentinel BWT. The overloads without a sentinel order rows by symbol only, as in a rotation matrix; for periodic texts the image row's rotation equals the shifted rotation as a string (equal rotations are interchangeable), which is all the inverses need. `O(n)` bytes, `O(n log n)` otherwise.

### inverseBwt(L, sentinel), inverseBwtPrimary(L, primary)

`inverseBwt` returns the `n = |L| - 1` symbols of `s` from a sentinel BWT (exactly one sentinel, asserted) by walking `psi` from the sentinel's row, the row of `s + $`. `inverseBwtPrimary` walks `psi` `n` times from `primary` (`0 <= primary < n`, or `primary = 0` for empty `L`, asserted), emitting `L[psi[r]]` each step. Correctness for periodic texts: the rows ending in `c`, in row order, and the rows starting with `c` are both the sorted rotations that start with `c` after a right shift, so the `k`-th row of each class has the same rotation as a string. Any `L` and `primary` give some sequence; only images of `bwtPrimary` invert to their source.

### bijectiveBwt(s), inverseBijectiveBwt(L)

Gil–Scott bijective BWT: factor `s` into Lyndon words `w_1 >= … >= w_k` (`duval` from `14`), sort all rotations of all factors by omega order (`u < v` when `u^∞ < v^∞`), and output the last symbol of each rotation. Rotations are ranked by prefix doubling over the factor cycles (radix sort, the rank of position `i` at step `h` pairs ranks at `i` and at the factor-cyclic position `h` later); the loop stops once a step leaves the partition unchanged (then every longer prefix agrees), all ranks are distinct, or `h >= n` (`2n` symbols decide by Fine–Wilf). Equal omega words come only from equal factors and have equal last symbols, so ties do not matter. `O(n log n)`.

`inverseBijectiveBwt` decomposes the stable permutation `psi` of `L` into cycles. Scanning rows in increasing order, each new cycle starts at its smallest row, which holds the Lyndon rotation; reading `L[psi[r]]` along the cycle spells that Lyndon word. Cycles are found in increasing order of their Lyndon words (omega order equals lexicographic order on Lyndon words), so emitting them in reverse gives the nonincreasing factorization of `s` (Gessel–Reutenauer; Kufleitner). The transform is a bijection on sequences of each length: `bijectiveBwt(inverseBijectiveBwt(L)) = L` for every `L`. `O(n)` bytes, `O(n log n)` otherwise.

## Feature-to-test map

Runner: [`29-bwt_tester.py`](<../../96-Local Testing/07-Strings/29-bwt_tester.py>). Oracles materialize rotations and sort them stably; the sentinel is a key below every symbol; brute Lyndon factors compare all rotations; omega order compares the first `|u| + |v|` symbols of the repetitions. All checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| sortCyclicRotations | Every case against stably sorted rotations; a random text of the large size against `SuffixArray` of `s + s`. |
| bwt, bwtFromSuffixArray | Every case against the brute sentinel matrix (the brute suffix order supplies `sa`), with sentinels NUL, `0xFF` (largest byte, still ranked lowest), `INT64_MIN` and `1000` (largest `int` value used); large texts against `SuffixArray`. |
| inverseBwt, inverseBwtPrimary | Round trip on every case and on random, period-5, Fibonacci and unary texts of 30000/200000/500000 symbols (quick/full/stress). |
| bwtPrimary | Last column and smallest primary row against the brute matrix; `order[primary] = 0` on large texts. |
| psiMapping, lfMapping (both forms) | Against a brute stable sort; LF as the inverse of psi; LF maps each row to the rotation one position earlier (exactly for the sentinel form, as strings for rotations). |
| bijectiveBwt, inverseBijectiveBwt | Against brute Lyndon factors with omega-sorted conjugates; round trip; `bijectiveBwt(inverseBijectiveBwt(s)) = s` on every case (bijectivity on exhaustive lengths); permutation and round trip on large texts. |

Exhaustive strings over alphabets 1..3 through length 7/9/10 (ternary two shorter) for quick/full/stress, then 300/3000/6000 random strings up to 40 symbols (a third over 255 bytes, a quarter periodic), each as bytes, shifted bytes, extreme `lng` and signed `int`. Checked builds run five precondition probes: a sentinel inside the text, no sentinel, two sentinels, a primary index out of range and a suffix array of the wrong size.

## Commands and results

Run 2026-10-12, after the tester gained its own `07-suffixarray.hpp` include; the stress and integration lines are from 2026-10-11 (the header is unchanged since). Every run builds optimized (`-O2 -DNDEBUG`) and checked (`-O0 -g -D_GLIBCXX_DEBUG`, precondition probes) binaries, and full and stress add ASan/UBSan (`-O1`); all with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full and stress first pass the quick pre-flight under the 2048 MB cap.

```bash
python3 '96-Local Testing/07-Strings/29-bwt_tester.py' --mode quick --seed 1  # PASS, 2 configurations, 133,013 checks, 5 assertion probes, MEMORY peak 516 MB
python3 '96-Local Testing/07-Strings/29-bwt_tester.py' --mode full --seed 1  # PASS, 3 configurations, 1,281,729 checks each, MEMORY peak 601 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/29-bwt_tester.py' --mode full --seed 1  # PASS, 3 configurations, 1,281,729 checks each, MEMORY peak 602 MB
python3 '96-Local Testing/07-Strings/29-bwt_tester.py' --mode stress --seed 3  # 2026-10-11: PASS, 3 configurations, 2,747,479 checks (optimized, ASan/UBSan), 1,291,565 (checked), MEMORY peak 780 MB
python3 '96-Local Testing/02-integration.py'  # 2026-10-11: PASS, 129 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace
python3 '96-Local Testing/02-integration.py' --sanitizers  # 2026-10-11: FAIL outside this package: the Core 16-poly_tester.cpp self-test is linked alone and misses its second translation unit (otherUnitConvolution); all earlier stages passed
python3 '96-Local Testing/03-consistency.py'  # PASS, errors: []
```

## Benchmarks

No dispatch threshold and no Barrett/Montgomery; every function has a single backend, so no benchmark is required. `sortCyclicRotations` and `bwt` reuse the measured `sais` threshold from [12-sais.md](12-sais.md).

## Sources

Code is independently implemented from the cited descriptions.

- [Wikipedia, Burrows–Wheeler transform](https://en.wikipedia.org/wiki/Burrows%E2%80%93Wheeler_transform) (fetched 2026-10-10): sentinel and primary-index forms, LF inversion, bijective variant with its example (`SIX.MIXED.PIXIES.SIFT.SIXTY.PIXIE.DUST.BOXES`, reproduced by the smoke test).
- [Princeton COS226 Burrows–Wheeler specification](https://www.cs.princeton.edu/courses/archive/fall20/cos226/assignments/burrows/specification.php) (fetched 2026-10-10): primary index (`first`), linear inverse with the `next` array (psi).
- [Rosalind BA9I/BA9J/BA9K](https://rosalind.info/problems/list-view/?location=bioinformatics-textbook-track) (fetched 2026-10-10): BWT, inverse BWT and LF mapping exercises.
- [Kufleitner, arXiv 0908.0239](https://arxiv.org/abs/0908.0239) (abstract, fetched 2026-10-10): bijective BWT correctness via the standard permutation.
- SPOJ BWHEELER (inverse from a primary row; statement returned 403, search snippet only).
- Catalog sweep in [00-sources.md](00-sources.md#p084-sweeps-2026-10-10).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): extended BWT of a string collection and its inverse, the sort transform, the run count `r`, dynamic BWT updates, move-to-front.
- `36` builds FM-index backward search on `bwt` and `lfMapping`; `46` counts runs of `bwt` or `bwtPrimary`.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-11: implemented and verified (P084), no review findings; full (seed 1), `CXX=g++-14` full (seed 2), stress and integration passed; sanitizer integration failed outside this package.
- 2026-10-12: the tester includes `07-suffixarray.hpp` itself, since `14-lyndon.hpp` no longer does; quick, full and `CXX=g++-14` full passed.
