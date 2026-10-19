# 23-dynamicstringhash.hpp — evidence

`23-dynamicstringhash.hpp` (batch ST09, package P085) holds the polynomial hashes of strings under edits. All share the arithmetic, symbol encoding and `Digest` of `StringHash<KIND>` (`03-stringhash.hpp`):

- `HashMonoid`: a length-aware acted monoid (forward hash, reversed hash, `B^length`, with range assignment) for sequence and segment trees.
- `PointUpdateHash`: point updates over two Fenwick trees (`02-Data Structures/02-fenwick.hpp`).
- `SequenceHash`: an `ImplicitTreap` over `HashMonoid` (`02-Data Structures/19-treap.hpp`) for insertions, erasures, splits, joins, reversals and range assignment.
- `DequeHash`: pushes and pops at both ends.

Contest profile, GNU C++20, no ISA code.

## Contracts

`KIND` is as in `StringHash`: 0 is the default double prime `1e9+7`, `1e9+9`; 1 wraps mod `2^64` and has structured collisions, so it is for speed only; 2 is the Mersenne `2^61 - 1`. A symbol `x` is a `uint`, string bytes being read as unsigned. It is encoded as `x + 1` reduced into the field, with `x < 1000000006` for KIND 0 (asserted). A base must satisfy `StringHash`'s rules (asserted):

- KIND 0: each residue in `[257, MOD)`;
- KIND 1: odd and at least 257;
- KIND 2: in `[257, 2^61 - 1)`.

The default is `StringHash<KIND>::defaultBase()`, and `StringHash<KIND>::randomBase(seed)` draws one. Each structure owns its base for its lifetime; there is no static state.

A `Digest` from any of these structures equals the `StringHash<KIND>` digest of the same string under the same base and encoding (`get`: forward; `reverseGet`: the hash of the reversed range, as `StringHash::reverseGet`). Equal digests are Monte Carlo evidence of equal strings with the collision bounds of [03-stringhash.md](03-stringhash.md). `lcp`, `lcs` and `compare` are Monte Carlo through them: binary search on digest equality, then one exact symbol comparison for `compare`. Ranges are half-open and asserted.

### HashMonoid<KIND>

`S = {h, r, p}` holds the forward hash, the hash of the reversal and `B^length`, so concatenation needs no length lookup:

- `op(a, b) = {a.h * b.p + b.h, b.r * a.p + a.r, a.p * b.p}`;
- `e() = {0, 0, 1}`;
- `reverse` swaps `h` and `r`.

It is a static preset under the stored-object monoid contract ([00-monoids.md](<../../02-Data Structures/Docs/00-monoids.md#monoid-contract>)): `ImplicitTreap` and `LazySegmentTree` store it as an empty member `m`, and `ImplicitTreap` calls `m.reverse`. The base stays in `SequenceHash`, because only `leaf` and `assignment` need it.

`F = {k, assign}` is a range assignment. `mapping` gives every symbol of the range the code `c`: `h = r = k (p - 1)` with `k = c / (B - 1)`, the geometric sum `c (B^L - 1) / (B - 1)`. `id()` has `assign = false` and maps every value to itself; `composition(f, g)` returns `f` when `f` assigns, else `g`, the newer action winning. Assignments need `B - 1` invertible, so not KIND 1 (a `static_assert` in `SequenceHash::assignment`). The interface (`S`, `F`, `op`, `e`, `mapping`, `composition`, `id`, `reverse`) plugs into `ImplicitTreap` and `LazySegmentTree` (`12-lazysegmenttree.hpp`), the latter for fixed-length range assignment (tested). `O(1)` per operation.

### PointUpdateHash<KIND>

`PointUpdateHash(vector<uint> s, base)` or `(string_view, base)`; `O(n)` setup with one modular inverse of the base. Fenwick tree `forward` stores `code_i B^(n-1-i)` and `backward` stores `code_i B^i`. Normalisation uses `O(n)` precomputed powers and inverse powers:

- `get(l, r)`: the forward range sum times `B^-(n-r)`;
- `reverseGet(l, r)`: the backward range sum times `B^-l`.

Operations:

- `set(i, x)`: `O(log n)`; it adds the code difference to both trees.
- `get`, `reverseGet`: `O(log n)`.
- `lcp(l, r, a, b)` and `lcs(l, r, a, b)`: the longest common prefix or suffix of `s[l, r)` and `s[a, b)`, `O(log^2 n)`.
- `compare(l, r, a, b)`: -1/0/1, a proper prefix being smaller, `O(log^2 n)`.
- `size()`, and the field `symbols`, which holds the current symbols; `common(..., suffix)` and `checkRange` are the shared helpers of these queries.

`M: O(n)`. Palindrome queries under updates are `get(l, r) == reverseGet(l, r)`.

### SequenceHash<KIND>

`SequenceHash(base, seed)` is an `ImplicitTreap<HashMonoid<KIND>>` with one base. Trees are root ids in one pool (0 is the empty tree), as in the treap's contract, and the treap's operations act on leaves and assignments built here:

- `insert(v, i, leaf(x))`, `erase(v, i)`, `set(v, i, leaf(x))`;
- `split(v, k)`, and `merge(a, b)` (join), which concatenates two sequences of the same pool;
- `reverse(v, l, r)`, and `apply(v, l, r, assignment(x))` (range assignment);
- `at`, `size`, `values`, `build(leaves(s))`.

All are `O(log n)` expected, `build` `O(n)`. The structure adds:

- `leaf(x)`: the monoid value of one symbol.
- `leaves(s)`: the batch form for a string or a symbol vector.
- `assignment(x)`: the action giving every symbol in a range the value `x`; not for KIND 1.
- `get(v, l, r)` and `reverseGet(v, l, r)`: from `prod`, `O(log n)` expected. They are kept by the user's decision 2026-10-16: the typed `Digest` (length, base) compares soundly with other digests, and `prod`'s raw value does not.
- `lcp`, `lcs`, `compare(v, l, r, a, b)` on two ranges of one sequence: `O(log^2 n)` expected. `compare` reads the two symbols at the first difference with `at`.

Queries split and re-merge the tree. The shape depends only on the priorities, so it is unchanged; root ids may change and are passed by reference.

### DequeHash<KIND>

`DequeHash(base)` keeps `prefix`, a deque of `size() + 1` field values with `prefix[i] - prefix[j] B^(i-j)` equal to the hash of positions `[j, i)` counted from the current front, and a power table grown to the largest size reached.

- `pushBack(x)` appends `prefix.back() B + code`.
- `pushFront(x)` prepends `(prefix.front() - code) B^-1`, the base inverse being computed once at construction.
- `popBack()` and `popFront()` drop an end value; both assert a nonempty deque.
- `get(l, r)` is the usual prefix difference.

All operations are `O(1)` amortized, and memory is `O(max size)`.

### Legacy mapping

- `mutableLcp` (row) → `SequenceHash::lcp` (user decision 2026-10-16; point-update LCP is `PointUpdateHash::lcp`).
- `SequenceHash: insert, erase, split, join, reverse` (row) → the inherited `ImplicitTreap` operations with `leaf` values; `join` is `merge`.

### Aliases

maspypy `Monoid_Rolling_Hash` and suisen's `DynamicRollingHash` node are `HashMonoid`; maspypy `combine` is `HashMonoid::op` (or `StringHash::concat`); Nyaan `RollingHashOnSegmentTree::same` is digest equality; maspypy `deque_rolling_hash` is `DequeHash`; suisen push/pop at the ends of a dynamic sequence are `insert`/`erase` at `0` and `size`.

## Feature-to-test map

Runner: [`23-dynamicstringhash_tester.py`](<../../96-Local Testing/07-Strings/23-dynamicstringhash_tester.py>). Oracle: an independent polynomial hash of a mirrored `vector`/`deque` (per-KIND arithmetic written in the tester), brute prefix/suffix scans and brute lexicographic comparison; `StringHash<KIND>` agreement is supporting evidence. All three KINDs; default and random bases.

| Operation | Coverage |
|---|---|
| PointUpdateHash (construction, set, get, reverseGet, lcp, lcs, compare, size) | 30/200/800 rounds of 60 steps (half point sets) on 0..119 symbols, alphabets 1..3 and the full symbol range. Equal-length range pairs a third of the time. `StringHash` agreement after edits. A high-byte `string_view`. Periodic texts of 20000/100000/300000 symbols with 2000/20000/20000 long-prefix queries and point edits. |
| SequenceHash (leaf, leaves, assignment, get, reverseGet, lcp, lcs, compare, with insert, erase, set, split, merge, reverse, apply, at, size, values, build) | 20/120/500 rounds of 80 random edits (split checked on both parts, then merge as a rotation; assignment for KIND 0 and 2) with queries after every edit; a high-byte string; large periodic texts against `PointUpdateHash`. |
| DequeHash (pushFront, pushBack, popFront, popBack, get, size) | 20/150/600 rounds of 300 random end operations with a random range digest after each; 20000/100000/300000 front pushes. |
| HashMonoid (e, op, reverse, mapping, composition, id) | 100/1000/4000 rounds: identity, associativity, forward/reversed/power fields against the brute hash, reversal, assignment mapping and composition, `mapping(id, x) = x`, `composition` with `id` on either side, `mapping(f, e) = e`; `LazySegmentTree<HashMonoid>` with 400 mixed assignments and products (it applies `id`). |
| Preconditions | Checked builds abort on ten probes: point index, range, symbol and lcp ranges, a base below 257, an even KIND 1 base, popping an empty deque, a deque range, a sequence lcp range, a sequence symbol. |

## Commands and results

Latest run 2026-10-17 (P027 monoid pass), Linux x86-64 (Intel Core i9-11900H), GNU++20, CPython 3.14.7, the optimized, checked (`-D_GLIBCXX_DEBUG`) and ASan/UBSan configurations built by [`_00_runner.py`](<../../96-Local Testing/07-Strings/_00_runner.py>):

| Command | Result |
|---|---|
| `23-dynamicstringhash_tester.py --mode full --seed 20261017` (GCC 16.2.1) | PASS, 3 configurations, 1,468,101 checks per configuration, 10 assertion probes, MEMORY peak 551 MB |
| `CXX=g++-14 23-dynamicstringhash_tester.py --mode full --seed 20261017` (GCC 14.4.1, floor) | PASS, 1,468,101 checks per configuration, MEMORY peak 558 MB |
| `23-dynamicstringhash_tester.py --mode stress --seed 7` | PASS, 5,712,181 checks per configuration, MEMORY peak 554 MB |
| `02-integration.py --sanitizers` | PASS: 156 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, sanitizer self-tests, MEMORY peak 1834 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/07-Strings/23-dynamicstringhash_tester.py' --mode full --seed 20261017
CXX=g++-14 python3 '96-Local Testing/07-Strings/23-dynamicstringhash_tester.py' --mode full --seed 20261017
python3 '96-Local Testing/07-Strings/23-dynamicstringhash_tester.py' --mode stress --seed 7
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

No dispatch threshold and no Barrett/Montgomery: each structure is one algorithm over `StringHash` arithmetic, so no benchmark is required.

## Sources

Code is independently implemented from the cited descriptions; catalog sweep in [00-sources.md](00-sources.md#p085-sweeps-2026-10-16).

- [maspypy `alg/monoid/rolling_hash.hpp`](https://raw.githubusercontent.com/maspypy/library/main/alg/monoid/rolling_hash.hpp) and [`string/deque_rolling_hash.hpp`](https://raw.githubusercontent.com/maspypy/library/main/string/deque_rolling_hash.hpp) (fetched 2026-10-16 by the researcher): the `(B^len, hash)` monoid and the inverse-base front push.
- [suisen `DynamicRollingHash`](https://suisen-cp.github.io/cp-library-cpp/library/string/dynamic_rolling_hash.hpp) and [Nyaan `rolling-hash-on-segment-tree`](https://nyaannyaan.github.io/library/string/rolling-hash-on-segment-tree.hpp) (fetched 2026-10-16): point replace and equality on dynamic sequences.
- BZOJ 1014 / Luogu P4036 (insert, replace, LCQ), CF 580E (range assignment), CSES Palindrome Queries (point update, palindrome check): the target problems, from write-ups only. Statements were not fetched.

## Limits and handoffs

- Not adopted (reasons in [00-notes.md](00-notes.md)): `reverseGet`, `lcp` and `compare` on `DequeHash` (double-ended palindrome structures are `45`), multi-base hashing as a separate operation, period checks.
- `lcp`/`lcs`/`compare` are `O(log^2 n)`; an `O(log n)` dynamic LCE is the Optimal Dynamic Strings row `47`.
- Tests include `06-Miscellaneous/02-customhash.hpp` and `02-Data Structures/19-treap.hpp` first under a `-Wshadow` suppression. Their constructor parameters `seed` shadow members (owned by P013 and P029). This header builds with `-Wshadow`.

## History

- 2026-10-16: implemented and verified (P085), independent review found 4 rule and test-gap findings, fixed; full suite passed on g++ and g++-14.
- 2026-10-17: P027 monoid pass (`ImplicitTreap` stores `HashMonoid` as an empty object; no API change); full (seed 20261017) on g++ and g++-14 and stress (seed 7) passed, MEMORY peak 551/558/554 MB.
