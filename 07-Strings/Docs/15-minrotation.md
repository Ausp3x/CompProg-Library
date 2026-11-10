# 15-minrotation.hpp — evidence

`15-minrotation.hpp` (batch ST07, package P083) finds the lexicographically minimal and maximal rotations of a sequence, all starts of the minimal rotation, the canonical (minimal) rotation itself, the rotation offset between two sequences and rotation equivalence, plus a suffix-array variant for callers that already hold the suffix array of `s + s`. Contest profile, GNU C++20, no ISA code.

## Contracts

All functions take a random-access sequence with `.size()` at most `INT_MAX / 2` (asserted; rotation indices reach `2n`) whose symbols are totally ordered by `<` (`string`, `string_view`, `vector` of integers or other ordered values). `char` symbols compare as unsigned bytes, so byte 255 is the largest symbol and NUL is valid. Equal symbols are those with neither `a < b` nor `b < a`; `rotationOffset` additionally uses `==`, which must agree with that equivalence. No input is retained. The rotation by `k` is `s[k, n) + s[0, k)`.

### minRotationIndices, maxRotation

`minRotationIndices(s)` returns `{first, gap}`: `first` is the smallest start `k` in `[0, n)` of a lexicographically minimal rotation, and the minimal rotation starts exactly at `first + t * gap` for `t` in `[0, n / gap)`, where `gap` is the smallest period of `s` dividing `n` (`gap = n` when the start is unique). `maxRotation(s)` returns the smallest start of a maximal rotation. For a periodic string several starts give the same rotation; the smallest is returned (tie convention). Empty input gives `{0, 0}` and start 0. O(n) time, O(1) space.

Correctness (two-pointer minimum expression): candidates `i < j` are compared on `k` equal symbols; when `rot(i)[k] > rot(j)[k]`, every start `i + t` (`t <= k`) is strictly worse than `j + t`, so `i` may jump by `k + 1` without discarding any minimal start. Both pointers only move forward, so at most `2n` jumps and `n` matching steps occur. Every index below `max(i, j)` except `min(i, j)` is discarded; therefore, when the loop stops because `k = n`, both pointers are minimal starts with no minimal start between them and their distance is the gap, and when a pointer leaves `[0, n)` the other is the unique minimal start. `maxRotation` flips the comparison.

### canonicalRotation

`canonicalRotation(s)` takes `s` by value and returns it rotated to the minimal rotation (same container type; requires `begin()`/`end()` usable by `std::rotate`, so not `string_view`). O(n).

### rotationOffset, rotationEquivalent

`rotationOffset(a, b)` returns the smallest `k` with `rot(a, k) == b`, or `-1` when `b` is not a rotation of `a` (including a length mismatch); two empty sequences give 0. `rotationEquivalent(a, b)` is `rotationOffset(a, b) >= 0`. O(n) time, O(1) space. Proof: with `x, p` from `minRotationIndices(a)` and `y = minRotationIndices(b).first`, `b` is a rotation of `a` iff the two minimal rotations agree; then `rot(a, k) = b` iff `k + y ≡ x (mod p)`, whose smallest non-negative solution is `(x - y) mod p`.

### minRotation(const SuffixArray<T> &doubled)

Takes a `SuffixArray<T>` (from `07-suffixarray.hpp`) built over `s + s` (even length, equal halves; asserted in O(n)) and returns the same index as `minRotationIndices(s).first`. Suffix `i < n` of `s + s` is `rot(i)` followed by `s[i, n)`, so the first such suffix in suffix order has a minimal rotation; all minimal starts share that length-`n` prefix and are contiguous in the suffix array with `lcp >= n`, so a forward walk over that block finds the smallest start. O(n) beyond the caller's construction (`O(n)` for bytes and dense ranges since `07` builds through SA-IS, otherwise `O(n log n)`).

### Legacy mapping

| Removed (prune, 2026-10-13) | Replacement |
|---|---|
| `minRotation(s)` | `minRotationIndices(s).first` (same algorithm and cost) |

## Feature-to-test map

Runner: [`15-minrotation_tester.py`](<../../96-Local Testing/07-Strings/15-minrotation_tester.py>). The oracle materializes every rotation and compares them with `std::vector` ordering.

| Operation | Coverage |
|---|---|
| minRotationIndices, maxRotation | Exhaustive ternary strings through length 7/9/10 (quick/full/stress), 300/3000/20000 random byte strings (a quarter periodic, a third over 256 bytes) each also as extreme-`lng` and signed-`int` vectors and the 256 bytes descending: `first` and `gap` against the two smallest brute-force minimal starts, `maxRotation` against the smallest brute-force maximal start. Unary and period-3 strings of length 20000/300000/1000000 (`{0, 1}` and `{2, 3}`, maximal starts 0 and 1); `first` of a random binary string of that length against the suffix-array variant. |
| canonicalRotation | Same cases against the brute minimal rotation; large random string versus its rotation. |
| rotationOffset, rotationEquivalent | Every rotation of every case (smallest matching brute offset), a modified string (brute search), a shorter string (`-1`), both argument orders. |
| minRotation (suffix array) | Every case whose symbols fit `int`, against brute force; large random binary string (length up to 10^6) against the linear algorithm. |

Checked builds run two precondition probes: an odd-length suffix array and one whose halves differ.

## Commands and results

Latest library-wide runs 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, build cache emptied before each cold run), each PASS unless stated: `python3 '96-Local Testing/01-run.py' --mode full` (GCC 16.2.1): pre-flight quick peak 803 MB; optimized 0.3 s compile + 1.8 s run, checked 24.8 s compile + 7.8 s run, ASan-UBSan 24.6 s compile + 28.0 s run; MEMORY peak 1001 MB. `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` (GCC 14.4.1): pre-flight quick peak 869 MB; optimized 0.3 s compile + 1.3 s run, checked 34.3 s compile + 9.6 s run, ASan-UBSan 26.8 s compile + 20.2 s run; MEMORY peak 1038 MB. `python3 '96-Local Testing/01-run.py' --mode quick`: optimized 20.2 s compile + 0.2 s run, checked 21.4 s compile + 4.8 s run; MEMORY peak 801 MB. `python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode stress --seed 42` (three suites at a time) PASS: pre-flight quick peak 808 MB; optimized 0.0 s compile + 2.0 s run, checked 7.1 s compile + 2.6 s run, ASan-UBSan 6.3 s compile + 40.2 s run; MEMORY peak 790 MB. Dated rows below stay those commands' latest results.

P083 prune run, 2026-10-13: GCC 16.2.1 20260810, GCC 14.4.1 20260915 (`CXX=g++-14`), Python 3.14.7, Linux x86-64, i9-11900H. Configurations: optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan, as built by [`_00_contest_runner.py`](<../../96-Local Testing/_00_contest_runner.py>).

```bash
python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode full --seed 20261013
CXX=g++-14 python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode full --seed 20261013
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 20261013 --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/03-consistency.py' --braces '07-Strings/15-minrotation.hpp' '96-Local Testing/07-Strings/15-minrotation_tester.cpp'
```

Full, seed 20261013: PASS in all three configurations with both assertion probes, 1,209,268 checks per configuration; peak 575 MB on g++, 555 MB on g++-14 (rerun after rebasing onto master, whose `SuffixArray` now builds `lcp` with `lcpArray`). The quick 07-Strings run passed all 20 suites. `02-integration.py` passed 135 standalone and aggregate headers, the scalar and available AVX2 multiple-translation-unit builds and the workspace. `03-consistency.py` reports no errors; `--braces` reports no closing-brace, comment-cap or free-function violations.

## Benchmarks

Linear algorithms with one backend each; no speed claim is made and no benchmark is required.

## Sources

Code is independently implemented from the invariants above.

- [KACTL `MinRotation.h`](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/MinRotation.h) (fetched 2026-10-10): two-pointer minimal rotation returning an index.
- [OI Wiki, 最小表示法](https://oi-wiki.org/string/minimal-string/) (fetched 2026-10-10): the `i, j, k` minimum expression with `min(i, j)` as the answer.
- [cp-algorithms, Lyndon factorization](https://cp-algorithms.com/string/lyndon_factorization.html) (fetched 2026-10-10): smallest cyclic shift by Duval on `s + s` (returns the string); the same operation as `canonicalRotation`.
- [maspypy `minimum_cyclic_shift.hpp`](https://maspypy.github.io/library/string/minimum_cyclic_shift.hpp) (fetched 2026-10-10): index form.
- [cp-algorithms, suffix array](https://cp-algorithms.com/string/suffix-array.html) (fetched 2026-10-10): cyclic-shift sorting, basis of the suffix-array variant.
- Catalog sweep 2026-10-10 in [00-sources.md](00-sources.md); adopted: `minRotationIndices`, `rotationOffset`.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): `minCyclicShift` (merged: every catalog treats it as `minRotationIndices().first`), minimal rotation of every prefix, a separate rotation-period function (`minRotationIndices().second`, or `27` `wordRoot`), cyclic comparison beyond `rotationEquivalent`.
- Booth's algorithm is not used: the two-pointer minimum expression has the same O(n) bound and O(1) space and is what KACTL and OI Wiki use.
- Sorting all cyclic shifts belongs to `29` `sortCyclicRotations`; cyclic-shift occurrence counting to `10`.

## Aliases

Search aliases (P263, grab --find): minimum representation (最小表示法, OI Wiki: minimal string), lexicographically minimal string rotation, 最小循环表示, 循环同构, 最小循环移位, 最大表示法.

## History

- 2026-10-10: implemented and verified (P083), independent review found 1 defect, fixed; quick, full and stress passed on g++, full on g++-14.
- 2026-10-13: prune, removed `minRotation(s)` (cases moved to `minRotationIndices(s).first`); full suite passed on g++ and g++-14.
- 2026-10-19: P017, the suffix-array variant's construction cost restated for SA-IS (no API change); full on g++ before the final code-range cutoff and quick on the final code passed (recorded in [07-suffixarray.md](07-suffixarray.md#commands-and-results)).
- 2026-11-07: P263 added the search aliases above (`grab --find`); no code or test change.
- 2026-11-07: P263 follow-up added common Chinese names of the row's operations to the search aliases (`grab --find`; Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, the 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
