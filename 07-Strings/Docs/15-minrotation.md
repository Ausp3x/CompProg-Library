# 15-minrotation.hpp — evidence

`15-minrotation.hpp` (batch ST07, package P083) finds the lexicographically minimal and maximal rotations of a sequence, all starts of the minimal rotation, the canonical (minimal) rotation itself, the rotation offset between two sequences and rotation equivalence, plus a suffix-array variant for callers that already hold the suffix array of `s + s`. Contest profile, GNU C++20, no ISA code.

## Contracts

All functions take a random-access sequence with `.size()` at most `INT_MAX / 2` (asserted; rotation indices reach `2n`) whose symbols are totally ordered by `<` (`string`, `string_view`, `vector` of integers or other ordered values). `char` symbols compare as unsigned bytes, so byte 255 is the largest symbol and NUL is valid. Equal symbols are those with neither `a < b` nor `b < a`; `rotationOffset` additionally uses `==`, which must agree with that equivalence. No input is retained. The rotation by `k` is `s[k, n) + s[0, k)`.

### minRotation, maxRotation, minRotationIndices

`minRotation(s)` returns the smallest start `k` in `[0, n)` of a lexicographically minimal rotation; `maxRotation(s)` the smallest start of a maximal one. For a periodic string several starts give the same rotation; the smallest is returned (tie convention). `minRotationIndices(s)` returns `{first, gap}`: `first` is `minRotation(s)` and the minimal rotation starts exactly at `first + t * gap` for `t` in `[0, n / gap)`, where `gap` is the smallest period of `s` dividing `n` (`gap = n` when the start is unique). Empty input gives start 0 and `{0, 0}`. O(n) time, O(1) space.

Correctness (two-pointer minimum expression): candidates `i < j` are compared on `k` equal symbols; when `rot(i)[k] > rot(j)[k]`, every start `i + t` (`t <= k`) is strictly worse than `j + t`, so `i` may jump by `k + 1` without discarding any minimal start. Both pointers only move forward, so at most `2n` jumps and `n` matching steps occur. Every index below `max(i, j)` except `min(i, j)` is discarded; therefore, when the loop stops because `k = n`, both pointers are minimal starts with no minimal start between them and their distance is the gap, and when a pointer leaves `[0, n)` the other is the unique minimal start. `maxRotation` flips the comparison.

### canonicalRotation

`canonicalRotation(s)` takes `s` by value and returns it rotated to the minimal rotation (same container type; requires `begin()`/`end()` usable by `std::rotate`, so not `string_view`). O(n).

### rotationOffset, rotationEquivalent

`rotationOffset(a, b)` returns the smallest `k` with `rot(a, k) == b`, or `-1` when `b` is not a rotation of `a` (including a length mismatch); two empty sequences give 0. `rotationEquivalent(a, b)` is `rotationOffset(a, b) >= 0`. O(n) time, O(1) space. Proof: with `x, p` from `minRotationIndices(a)` and `y = minRotation(b)`, `b` is a rotation of `a` iff the two minimal rotations agree; then `rot(a, k) = b` iff `k + y ≡ x (mod p)`, whose smallest non-negative solution is `(x - y) mod p`.

### minRotation(const SuffixArray<T> &doubled)

Takes a `SuffixArray<T>` (from `07-suffixarray.hpp`) built over `s + s` (even length, equal halves; asserted in O(n)) and returns the same index as `minRotation(s)`. Suffix `i < n` of `s + s` is `rot(i)` followed by `s[i, n)`, so the first such suffix in suffix order has a minimal rotation; all minimal starts share that length-`n` prefix and are contiguous in the suffix array with `lcp >= n`, so a forward walk over that block finds the smallest start. O(n) beyond the caller's O(n log n) construction.

## Feature-to-test map

Runner: [`15-minrotation_tester.py`](<../../96-Local Testing/07-Strings/15-minrotation_tester.py>). The oracle materializes every rotation and compares them with `std::vector` ordering; all checks survive `-DNDEBUG`.

| Operation | Coverage |
|---|---|
| minRotation, maxRotation | Exhaustive ternary strings through length 7/9/10 (quick/full/stress), 300/3000/20000 random byte strings (a quarter periodic, a third over 256 bytes) each also as extreme-`lng` and signed-`int` vectors, the 256 bytes descending, unary and period-3 strings of length 20000/300000/1000000. |
| minRotationIndices | Same cases: `first` and `gap` against the two smallest brute-force minimal starts; unary `{0, 1}`, period 3 `{2, 3}`. |
| canonicalRotation | Same cases against the brute minimal rotation; large random string versus its rotation. |
| rotationOffset, rotationEquivalent | Every rotation of every case (smallest matching brute offset), a modified string (brute search), a shorter string (`-1`), both argument orders. |
| minRotation (suffix array) | Every case whose symbols fit `int`, against brute force; large random binary string (length up to 10^6) against the linear algorithm. |

Checked builds run two precondition probes: an odd-length suffix array and one whose halves differ.

## Commands and results

Run 2026-10-10 on the final code with the configurations listed in [13-palindromictree.md](13-palindromictree.md#commands-and-results).

```bash
python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode quick --seed 1  # PASS, 2 configurations, 2 assertion probes
python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode full --seed 1  # PASS, 3 configurations, 1,246,221 checks each, MEMORY peak 566 MB
CXX=g++-14 python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode full --seed 2  # PASS, 3 configurations, 1,239,770 checks each, MEMORY peak 546 MB
python3 '96-Local Testing/07-Strings/15-minrotation_tester.py' --mode stress --seed 3  # PASS, 3 configurations, 5,557,197 checks (optimized, ASan/UBSan), 1,243,992 (checked), MEMORY peak 742 MB
python3 '96-Local Testing/02-integration.py' --sanitizers  # PASS, 115 standalone/aggregate headers, scalar/AVX2 multi-TU, workspace, sanitizer self-tests
python3 '96-Local Testing/01-run.py' --mode quick --filter 07-Strings --seed 1 --no-integration  # PASS, 13 suites
```

The consistency validator result is recorded in [13-palindromictree.md](13-palindromictree.md#commands-and-results).

## Benchmarks

Linear algorithms with one backend each; no speed claim is made and no benchmark is required.

## Sources

Code is independently implemented from the invariants above.

- [KACTL `MinRotation.h`](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/strings/MinRotation.h) (fetched 2026-10-10): two-pointer minimal rotation returning an index.
- [OI Wiki, 最小表示法](https://oi-wiki.org/string/minimal-string/) (fetched 2026-10-10): the `i, j, k` minimum expression with `min(i, j)` as the answer.
- [cp-algorithms, Lyndon factorization](https://cp-algorithms.com/string/lyndon_factorization.html) (fetched 2026-10-10): smallest cyclic shift by Duval on `s + s` (returns the string); the same operation as `minRotation`.
- [maspypy `minimum_cyclic_shift.hpp`](https://maspypy.github.io/library/string/minimum_cyclic_shift.hpp) (fetched 2026-10-10): index form.
- [cp-algorithms, suffix array](https://cp-algorithms.com/string/suffix-array.html) (fetched 2026-10-10): cyclic-shift sorting, basis of the suffix-array variant.
- Catalog sweep 2026-10-10 in [00-sources.md](00-sources.md).

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): `minCyclicShift` (merged: every catalog treats it as `minRotation`), minimal rotation of every prefix, a separate rotation-period function (`minRotationIndices().second`, or `27` `primitiveRoot`), cyclic comparison beyond `rotationEquivalent`.
- The row previously named Booth's algorithm; the two-pointer minimum expression has the same O(n) bound and O(1) space and is what KACTL and OI Wiki use.
- Sorting all cyclic shifts belongs to `29` `sortCyclicRotations`; cyclic-shift occurrence counting to `10`.
- Exact GCC 14.2 and the Windows build were not run.

## History

- 2026-10-10: implemented and verified (P083); independent review finding fixed: domain narrowed to n <= INT_MAX / 2 because rotation indices reach 2n.
