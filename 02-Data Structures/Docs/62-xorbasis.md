# 62-xorbasis.hpp — evidence

Package P032 (batch DS50). New header: an incremental GF(2) basis of 64-bit words kept in fully reduced form (`XorBasis`), its offline range sweep (`XorBasis::rangeXorBasisQuery`) and the online timestamped prefix basis (`PrefixXorBasis<B>`). Contest profile: no ISA code and no Barrett or Montgomery reduction, so no benchmark is required. The header depends only on `01-template.hpp`. Wide GF(2) matrices, nullspaces and orthogonal complements are Core row 12 (`01-Core/Docs/00-notes.md`, Ownership boundaries).

## Contracts

### XorBasis

`b` holds r = `size()` ≤ 64 nonzero words, sorted strictly decreasing and fully reduced: the leading bit (pivot) of each `b[i]` is clear in every other `b[j]`. `m[i]` is the set of successful inserts, numbered 0, 1, … in the order they returned true, whose XOR is `b[i]`. A default object is the zero space. Copies are independent. Callers may read `b` and `m` but must not write them.

- `insert(x)`: returns false, and changes nothing, iff `x` already lies in the span (so `insert(0)` is false); otherwise adds `x` and returns true. O(r). Reduction clears from `x` every pivot it holds; its leading bit q is then a new pivot. Each `b[i]` holding bit q has a pivot above q (a vector holds no bit above its own pivot), so XORing the reduced `x` into it keeps its pivot and keeps every other pivot clear. The masks follow the same XORs.
- `merge(o)`: inserts `o.b` in decreasing order, so the successful ones take the next insert numbers. `a.merge(a)` is a no-op: every vector of `a.b` is already spanned, so each `insert` returns false before changing `b`. O(r · r_o).
- `reduce(x)`: the minimum of `x ^ span`. After reduction no pivot is set. Adding a nonzero span element whose highest pivot is p sets bit p and leaves the bits above p unchanged, because no vector holds a bit above its own pivot. So every other coset value is larger.
- `contains(x)`: `reduce(x) == 0`.
- `represent(x, mask)`: returns true iff `x` is spanned and sets `mask` to the inserts whose XOR is `x` (0 for `x = 0`). When it returns false, `mask` is the partial mask of the reduction and has no meaning. This is the bool-plus-out-parameter form of `03-cpp.md`; every 64-bit mask is a legitimate answer, so no sentinel exists.
- `max(x = 0)`: the maximum of `x ^ span`. Greedy over decreasing pivots: bit p of the running value is fixed once `b` at pivot p has been considered.
- `kth(k)`: the k-th smallest span value, 0-based; asserts `k < 2^r` (any k when r = 64). Bit `r - 1 - i` of `k` selects `b[i]`. The value is increasing in `k`: when two indices first differ at the bit of pivot p, the values agree above p (only vectors whose pivot is p or below differ, and none of them holds a bit above p), and bit p comes from `b` at pivot p alone.
- `rank(x)`: the number of span values below `x`, for any `x`. It is a binary search over the increasing map `k ↦ kth(k)`. Fixing the bits of `k` from the top, `cur ^ b[i]` is the smallest value of the block with that bit set. So the loop finds the largest k with `kth(k) < x` (0 when `x = 0`), and the count is that k plus one. For `x` in the span, `kth(rank(x)) == x`. The count fits in `ulng`: when r = 64, the span is every word and the count equals `x`.
- `intersection(a, b)` (friend): returns a basis of span(a) ∩ span(b), with its own masks (inserts into the result). O(64 · r_b) with r_b = `b.size()`. Each vector y of `b` is reduced against pivot-indexed pairs (value, tag), seeded with `a`'s vectors and tag 0. The invariant: every tag lies in span(b), and every value XOR its tag lies in span(a).
  - A vector that reduces to 0 leaves a tag in both spaces.
  - Otherwise the pair joins the table.
  - Each kept tag is y plus earlier vectors of `b`, so the kept tags are independent.
  - Exactly dim a + dim b − dim(a + b) vectors reduce to 0, which is the dimension of the intersection (Zassenhaus counting).

### XorBasis::rangeXorBasisQuery

`rangeXorBasisQuery(a, qs, f)` (static): asserts `0 <= l <= r <= n` for every query `[l, r)`. It calls `f(i, e)` exactly once per query, in increasing order of r (ties in input order). `e` is a `const vector<ulng> &` holding an echelon basis of span(a[l, r)): distinct leading bits, strictly decreasing, not reduced. The reference is valid only during the call. Time O((n + q) · 64), memory O(n + q).

The sweep keeps one basis indexed by pivot, each entry with a position. Inserting `a[r - 1]` at position `r - 1` walks the pivots downward. At a pivot it holds, if the stored position is older, the incoming vector and its position swap with the stored ones before XORing. This keeps at each pivot the vector whose position is latest among those that could stand there. The invariant: the stored vectors with position ≥ l span a[l, r).

- Two invariants hold after every insertion (review argument, 2026-11-03): (I1) each stored `v[i]` lies in span(a[p[i], r)); (I2) each a[j], j < r, lies in span{v[i] : p[i] ≥ j}. A swap keeps both, because the carried vector's position is never below that of the vector it combines with: the XOR of two vectors with positions s ≤ t lies in span(a[s, r)) and keeps position s. For a query, (I1) puts {v[i] : p[i] ≥ l} inside span(a[l, r)) and (I2) puts every a[j], j ≥ l, inside its span; distinct pivots make it independent. (The stored vectors need not equal the reverse-order greedy basis; only the positions agree.)
- The tester checks this invariant against an independent critical-position sweep: position i stays iff a[i] lies outside span(a[i + 1, r)).

Range minimum, k-th and rank are composed by inserting `e` into an `XorBasis`, at O(64^2) per query.

### PrefixXorBasis

`PrefixXorBasis<B>(a = {})`, `1 <= B <= 64` (`static_assert`); every value must be below 2^B (asserted). It stores the timestamped pivot table of every prefix: `v[r][i]` and `p[r][i]`, with -1 for an empty pivot. It answers range queries online in O(B), using O(n · B) memory: 12 · B bytes per prefix, 768 bytes at B = 64, so 154 MB at n = 2 · 10^5. A smaller B cuts both memory and query time when the values are narrow.

- `push(x)`: appends `x` as position `n()`. It copies the last prefix table, then runs the same timestamped insertion as the sweep. O(B) amortized.
- `n()`: the number of values pushed.
- `rangeMax(l, r, x = 0)`: the maximum of `x ^ span(a[l, r))`. The start value `x` may use any bit. Asserts `0 <= l <= r <= n()`.
- `rangeContains(l, r, x)`: whether `x` lies in span(a[l, r)); a word with a bit at or above B is never spanned. Asserts the range.

### Violated preconditions

Under `NDEBUG`:

- `kth` with `k >= 2^r` ignores the high bits of `k`.
- A bad range in `PrefixXorBasis` reads outside its tables. In `rangeXorBasisQuery`, an `r` above n writes outside the counting array, as with any unchecked index.
- A value at or above 2^B in `PrefixXorBasis` may be stored with its high bits (pivots below B only), so later results are meaningless; every loop stays bounded.
- Every loop is bounded by r, B or 64, so each operation terminates within its normal bound.

### Aliases and catalog names

No legacy source. Graphs row 28's private `cycle_basis_detail::insert` and `reduce` were the same reduced basis and are replaced by `XorBasis` (migrated here).

| Catalog name | Here |
|---|---|
| maspypy `get_max(x)`, `get_min(x)`, `lower_bound`, `solve`, `kth` | `max(x)`, `reduce(x)`, `rank`, `represent`, `kth` |
| maspypy `range_xor_basis_query` | `XorBasis::rangeXorBasisQuery` |
| ei1333 `normalize`, `==` | the stored basis is always reduced; compare `b` |
| suisen `+`, `&`, `dim` | `merge`, `intersection`, `size` |
| hitonanode `f2_intersection` | `intersection` |
| PyRival `max_xor` (with indices) | `represent(max(), mask)` |
| OI Wiki minimum nonzero value | `kth(1)` (a nonempty subset of the n inserted values reaches 0 iff `size() < n`) |
| `countRepresentable` | `1ull << size()` (r < 64) |
| k-th with multiplicities over n inserted values | `kth(k >> (n - size()))` when `n - size() < 64` (each value `2^(n - size())` times), 0 otherwise |
| Prefix-basis online append | `PrefixXorBasis::push` |

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/62-xorbasis_tester.cpp`, entry `62-xorbasis_tester.py` (driver `_00_runner.py`; optimized, checked and ASan/UBSan builds; 17 assertion probes).

The tester's own oracles never call the header:

- `Ech`: an echelon table by leading bit, giving minimum, maximum, membership and the RREF.
- `countBelow`: a rank count over the reduced rows.
- `closure`: brute-force span enumeration for widths up to 8.
- A critical-position sweep for ranges.
- Group `oracles` checks the first two against `closure`, and the sweep against slice elimination on every (l, r) of arrays up to 48 long.

| Operation | Groups | Oracle |
|---|---|---|
| `insert`, `size`, `merge` (other, self, empty, into empty), invariants of `b` and `m` | `histories`, `exhaustive`, `edges` | After every mutation: `b` equals the oracle RREF (so it is sorted and reduced), and each `m[i]` XORs the recorded inserts, with merge inserts replayed in `o.b` order, to `b[i]` |
| `reduce`, `contains`, `max()`, `max(x)`, `rank`, `represent` | `histories`, `exhaustive` | `Ech` and `countBelow`; brute spans for widths up to 8, every x for widths up to 5; `represent`'s mask XOR-checked, 0 for x = 0 |
| `kth` | `histories`, `edges` | The result is spanned and has oracle rank k; `rank(kth(k)) == k`; equals the sorted span for widths up to 8; at full rank, `kth(k) == k` |
| `intersection` | `intersection`, `exhaustive` | Brute span equality for widths up to 8, including all 5329 pairs of bases from sequences of length at most 2. Larger widths: dimension dim A + dim B − dim(A + B), every vector in both spaces, symmetry, self and empty cases |
| `rangeXorBasisQuery` | `rangeSmall`, `rangeLarge`, `edges` | Each callback exactly once; `e` echelon. Every (l, r) on small arrays (shuffled, duplicated) against slice elimination; large runs against the critical-position sweep; n = 0, no queries, 100 duplicate queries, the swap example |
| `PrefixXorBasis<B>` build, `push`, `n`, `rangeMax` (with and without x), `rangeContains` (including x ≥ 2^B) | `prefixSuite`, B ∈ {1, 3, 20, 63, 64} | Built from a vector and by `push` with interleaved queries; slice elimination (small), critical-position sweep (large) |
| Preconditions | probes | `kth` on an empty basis, past `2^r` and at r = 63; `rangeXorBasisQuery` with l > r, r > n, l < 0 and on an empty array; `rangeMax` and `rangeContains` with each bad range; `push` and build of a value ≥ 2^B for B = 1, 3, 63 |

Value profiles: uniform, sparse, top bit set (bit 63 included), low-dimensional, zeros and duplicates, staircase, periodic, mixed, full rank 64.

The checked build divides the full and stress sizes by 20 and ASan by 4.

| Mode | History steps per (width, profile) | Small rounds | `rangeXorBasisQuery` n = q | `PrefixXorBasis<B>` n, B ∈ {63, 64} | `PrefixXorBasis<B ≤ 20>` n |
|---|---|---|---|---|---|
| quick | 150 | 60 | 3000 | 3000 | 3000 |
| full | 2500 | 600 | 2 · 10^5 | 2 · 10^5 | 2 · 10^5 |
| stress | 12000 | 3000 | 5 · 10^5 | 2 · 10^5 | 5 · 10^5 |

Mutation check (2026-11-03, quick mode, seed 3) caught all 7 mutants:

- the `p[i] < pos` swap dropped;
- `rank` returning k instead of k + 1;
- `kth` reading the bits of `k` in reverse;
- `m[i] ^= mx` dropped;
- `>=` changed to `>` in the range filter;
- `intersection` inserting y instead of its tag;
- `rangeContains` without its early `x >> B` return (rechecked 2026-11-03 after the O(B) rewrite: caught by `contains 2^B`).

## Benchmarks

Not required by the Contest profile; recorded because `insert` and `represent` replaced Graphs row 28's mask-free basis. Measured 2026-11-03 with `g++` 16.0.1 `-O2`, the same machine as the commands below, median of 5 repetitions, `mt19937_64` seed 1, under `_00_memory_cap.py`. The driver is a `/finish` scratch probe, not kept: 5 · 10^6 random 64-bit inserts with a fresh basis every 1000; `XorWalk` on n = 2 · 10^5, m = 10^6 random 63-bit weights; `CutSpaceHash::disconnects` on 2 · 10^5 sets of 40 distinct edges.

| Workload | Old `cycle_basis_detail` (no masks) | `XorBasis`, branchy selects (first version) | `XorBasis`, branchless selects |
|---|---|---|---|
| 5 · 10^6 inserts | 208 ms | 1010 ms | 246 ms |
| `XorWalk` build | 97 ms | 226 ms | 103 ms |
| `disconnects`, 2 · 10^5 sets of 40 | 424 ms | 1190 ms | 528 ms |

The branchy column is the `/finish` review's measurement (same workloads, its own run). The branch `if ((x ^ b[i]) < x)` is unpredictable on random words; the selects use an all-ones mask `-ulng(cond)`. The remaining 6–25% is the mask word per vector that `represent` costs (user decision 2026-11-02, accepted with that cost).

## Commands and results

Latest run 2026-11-03, Linux x86-64 (WSL2), `g++` 16.0.1, `g++-14` 14.3.0, CPython 3.14:

| Command | Result | Memory peak |
|---|---|---|
| `62-xorbasis_tester.py --mode quick` | PASS, 2 configurations, 3,414,496 checks (optimized), 17 probes | 546 MB |
| `62-xorbasis_tester.py --mode full --seed 1` | PASS, 3 configurations; 119,315,420 / 6,131,650 / 29,732,237 checks (optimized / checked / ASan) | 782 MB |
| `CXX=g++-14 62-xorbasis_tester.py --mode full --seed 1` | PASS, the same check counts | 769 MB |
| `62-xorbasis_tester.py --mode stress --seed 7` | PASS; 354,784,578 / 17,688,576 / 89,231,368 checks | 1022 MB |
| `02-integration.py` | PASS, 186 standalone and aggregate headers, scalar and AVX2 multi-TU, workspace | 1898 MB |
| `04-Graphs/28-cycle_basis_tester.py` quick, full (seed 1) on g++ and g++-14, stress (seed 7), after the migration and the branchless selects | PASS ([28-cycle_basis.md](../../04-Graphs/Docs/28-cycle_basis.md#commands-and-results)) | 943 MB |

```bash
python3 '96-Local Testing/02-Data Structures/62-xorbasis_tester.py' --mode quick
python3 '96-Local Testing/02-Data Structures/62-xorbasis_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/62-xorbasis_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-Data Structures/62-xorbasis_tester.py' --mode stress --seed 7
python3 '96-Local Testing/04-Graphs/28-cycle_basis_tester.py' --mode full --seed 1
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

## Sources

| Source | Actual reading and use |
|---|---|
| OI Wiki, linear basis | The greedy and Gaussian bases, maximum, minimum and k-th, the timestamped prefix basis and its swap rule, intersection by tracking |
| maspypy `linalg/xor/basis.hpp`, `ds/offline_query/range_xor_basis_query.hpp` | `get_max(x)`, `solve` (the mask form of `represent`), `lower_bound` (`rank`); the offline callback with the raw range basis |
| ei1333 `binary-basis`, suisen `xor_base`, hitonanode `f2_linear_space`, PyRival `max_xor` | Catalog names and the operation set (aliases above) |

The completeness sweep fetched these on 2026-11-02 ([00-sources.md](00-sources.md#fetched-2026-11-02-p032-ds50-sweep)). The implementation was written from the contracts above; no catalog code was copied.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p032-omissions) (user decision 2026-11-02): `countRepresentable`, `reducedEchelon`, `min`, the online maximum-weight basis, `undo`, the orthogonal complement and a segment tree of bases.
- `intersection`'s result masks refer to inserts into the result object, so they say nothing about `a` or `b`; the tester checks only their shape.
- `PrefixXorBasis<64>` memory: built from a vector (reserved), n = 3 · 10^5 needs about 231 MB; built by `push` alone, vector doubling peaked at 334 MB at n = 262,145 (review measurement), so reserve `v` and `p` first or pass the vector. Beyond that, use a smaller B or the offline sweep.
- The `NDEBUG` claims (Violated preconditions) rest on the loop bounds; no tester group runs violating inputs in optimized builds (`05-testing.md`, Memory safety).
- Python `_22_bitset.py` keeps its own `XorBasis` (`02-Data Structures/Docs/00-notes.md`, Ownership boundaries); its `witness` corresponds to `represent`.

## History

- 2026-11-03: P032 DS50 first implementation; duplicates ask row and sweep candidates decided by the user (2026-11-02); Graphs row 28 migrated onto `XorBasis`; 7/7 mutants caught; independent review, 6 findings and 6 nits fixed (sweep correctness argument, `NDEBUG` wide values, `push` memory, `rangeContains` O(B), copy-free `merge`, integration result); quick, full (seed 1) on g++ and g++-14, stress (seed 7) and `02-integration.py` passed after the fixes.
- 2026-11-03: `/finish` audit, 6 defects fixed (branchless `insert` and `represent` with benchmarks, row 63 wording, stale mutant entry, decision and fetch dates, Library Checker target, nits); quick, full (seed 1) on g++ and g++-14, stress (seed 7), `28-cycle_basis` suites and `02-integration.py` passed.
