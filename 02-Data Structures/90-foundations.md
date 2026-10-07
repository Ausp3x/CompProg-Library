# P006 foundations — DS01 and DS02

P006 owns headers `01`–`08`, completed in DS01 then DS02 order. C01/P002 supplies the verified template, integer aliases and PBDS imports. This record describes the verified Basic contest profile; advanced structures retain their separate inventory owners.

## Contracts

### Common

Canonical indices are zero-based and ranges are half-open. Empty construction is supported. Invalid indices, sizes, shapes and required predicate identities are preconditions checked by assertions; they remain preconditions under `NDEBUG`. A valid absent result has a documented sentinel. Public storage is exposed for contest use but must not be mutated behind an algorithm's invariants.

Generic arithmetic must satisfy the stated algebra and keep every stored value and intermediate representable, or use explicitly modular arithmetic. `lng` is a convenient sum type, not an overflow check. Complexity counts constant-cost element operations/comparisons/copies; strings and large integers add their own costs. Allocation failure is ordinary C++ allocation failure. Structures are single-threaded and own their storage; copying produces an independent snapshot.

A moved-from structure may only be assigned to or destroyed. Its size fields keep their old values while its storage is empty, so every other call is a precondition violation. Assignment restores every invariant. Structures that have both a size constructor and a vector constructor (`Fenwick`, `SegmentTree`, `PrefixSum`, `DifferenceArray`, `SqrtDecomp`, `SqrtRangeSum`) also take an `std::initializer_list<T>`. A braced list therefore always means values, as with `std::vector`: `PrefixSum<lng> p({7})` and `PrefixSum<lng> p{7}` hold the single value 7, and `PrefixSum<lng> p(7)` holds seven zeros. Elements of `T = bool` are read by value through `vector<bool>::const_reference` wherever a reference to storage is returned.

### DSU

Vertices `[0, n)` with `0 <= n <= INT_MAX`. Public `n`, `ncon`, `par` and `siz` must not be edited directly. Representatives are arbitrary and can change after a union. `findSet`, `getSize`, `isSameSet` and `groups` compress paths, so they are non-const. `uniteSets` returns whether two distinct components merged. `count` is the number of components. `groups` returns the components ordered by smallest vertex, each with members ascending, in O(n) time, workspace and output. `makeSet` appends the singleton vertex `n` and returns its id; it requires `n < INT_MAX`. Union by size gives O(log(n)) worst-case and inverse-Ackermann amortized find and unite.

### Fenwick

`0 <= n <= INT_MAX`, zero-based `add`/`get`/`set`, `prefixSum(r)` over `[0, r)`, `sum(l, r)` over `[l, r)`. T is an additive commutative group with identity `T(0)`, `+=`, `+`, `-` and `-=`. Every intermediate prefix and range sum must fit T, unless T is explicitly modular. Public `n` and `v` must not be edited directly. `get(i)` is `sum(i, i + 1)` and `set(i, x)` is `add(i, x - get(i))`, both O(log(n)). `values()` returns the point values in O(n) by undoing the linear build. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(sum(l, r))`, and `minLeft(r, pred)` the smallest `l` in `[0, r]` with `pred(sum(l, r))`. Both assert `pred(T(0))`; pred must be deterministic and stay false once the range has grown past a failure. They need no ordering on T (tested with `lng` and wrapping `uint`). `lowerBound(x)` is the first `i` with `prefixSum(i + 1) >= x` and `upperBound(x)` the first `i` with `prefixSum(i + 1) > x`. Both return `n` when absent; `lowerBound` returns 0 for `x <= 0` and `upperBound` for `x < 0`. They require ordered T, nonnegative point values and exact sums; individual updates may be negative if this still holds. One-based adapters: `add1(i)` with `i` in `[1, n]`, `prefixSum1(r)` over inclusive `[1, r]`, `sum1(l, r)` over inclusive `[l, r]`, allowing empty `l == r + 1` without computing `r + 1`. `lowerBound1` returns a value in `[1, n]`, or 0 when absent. Costs count T operations as O(1).

### SegmentTree

`0 <= n <= 2^29`, so doubled indices and search arithmetic stay in `int`. f is associative with two-sided identity `id`, and operand order is preserved. The empty range returns `id`. `get` and `allQuery` return `vector<T>::const_reference`: a reference owned by the tree, or a `bool` value for `T = bool`. `values()` copies the leaves in O(n). `apply(p, x)` sets point p to `f(a[p], x)`. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(query(l, r))`; `n` means no failure. `minLeft(r, pred)` returns the smallest `l` in `[0, r]`; 0 means no failure. Predicates are deterministic, true on `id` (asserted), and stay false as the range grows. Callbacks must not mutate the tree. Costs count calls and copies of T and f as O(1).

### SparseTable

Immutable sequence, `0 <= n <= INT_MAX`; empty construction is valid. f is associative; no identity and no default-constructible T are needed. `query(l, r)` is O(1) on a nonempty `[l, r)` and also requires idempotence; noncommutative idempotent operations (rectangular bands) are valid because the two overlapping blocks are combined in order. `fold(l, r)` is O(log(n + 1)), needs only associativity, and combines disjoint blocks left to right. `queryFast(l, r)` and `querySlow(l, r)` are the inclusive `[l, r]` forms of `query` and `fold`. `operator<<` prints the legacy diagnostic format with inclusive block bounds in O(n * log(n + 1)). Public `v` holds only valid blocks: level `i` has `n - 2^i + 1` entries.

### SparseTable2D

Rectangular `n x m` grid with `n * m <= INT_MAX`; rows of equal length are asserted once at build, before the empty-grid return. Grids with zero rows or zero columns are valid and admit no query. f must be associative, commutative and idempotent (min, max, gcd, and, or). A 2D rectangle has no canonical operand order, and the four overlapping corner blocks repeat cells. `query(x1, y1, x2, y2)` folds the nonempty half-open rectangle `[x1, x2) x [y1, y2)` in O(1) and asserts its bounds once. No identity or default-constructible T is needed. Level `(i, j)` stores the `2^i x 2^j` block folds for every valid top-left corner, row-major with width `m - 2^j + 1`. Total storage is at most `n * m * bit_width(n) * bit_width(m)` values. The input grid is copied.

### PrefixSum and PrefixSum2D

Zero-based half-open ranges and rectangles `[x1, x2) x [y1, y2)` in (row, column) order. Each dimension is in `[0, INT_MAX - 1]`. T is an additive commutative group with zero `T(0)`. Every stored prefix and intermediate must fit T, or T is modular. Input vectors are copied, so `rebuild` may take references to the object's own storage. An empty outer vector means 0 x 0; the dimension constructor also represents 0 x m and n x 0. Rows must have equal length (asserted). `rebuild` uses O(storage) workspace.

### DifferenceArray and DifferenceArray2D

Offline range or rectangle additions over the same domains; `add` on an empty range or rectangle is a no-op. `values()` reconstructs the array in O(n) or O(n * m) without changing the state. `clear` resets to zeros; `rebuild` replaces the initial values. Unused far-border subtractions are skipped, so a whole-array update by `LLONG_MIN` never computes its unrepresentable negation. Intermediate sums and differences must still fit T. The 2D update delta is named `w` so that it does not clash with the coordinates `x1` and `x2`.

### SqrtDecomp

f is associative with two-sided identity `id` and keeps left-to-right operand order. `0 <= n <= INT_MAX`, zero-based points, half-open ranges, and an empty query returns `id`. `B = 0` picks block width `m = max(1, floor(sqrt(n)))`; a positive `B` is the width. Constructors are `explicit`. `rebuild(a, B)` replaces every point and picks a fresh width in O(n). `get` is O(1) (by value for `T = bool`), `values` copies in O(n), and `set`/`setUpdate`/`opeUpdate` rebuild one block in O(m). `opeUpdate(i, x)` sets `v[i] = f(v[i], x)`. `query` is O(m + n / m). `pull(bi)` recomputes block `bi` from `v`. It is an internal helper without a bounds check and is not part of the inventory row. Public state must not be edited directly. Costs count T and f operations as O(1).

### SqrtRangeSum

T is a commutative ring with `T(0)`, `T(1)`, `T(length)`, `+`, `*` and `==`. Every intermediate, including pending tags, must fit T unless T is modular. Points, ranges and block widths match SqrtDecomp. `affine(l, r, a, b)` maps each x to `a * x + b`. New tags compose after pending ones: `(a, b)` after `(c, d)` is `(a * c, a * d + b)`. Assignment uses `a = 0` and needs no inverse. `add`, `assign`, `multiply` and `set` are affine special cases. `get` is O(1). `values` is O(n) and leaves tags intact. `rebuild` clears pending tags. `apply`, `push` and `pull` are unchecked internal block helpers: `pull` requires the block's tags to be pushed already. They are not part of the row.

### SortedVector and CoordinateCompression

Immutable sorted sequence. The comparator is a stable strict weak order, `n <= INT_MAX`. SortedVector keeps duplicates in unspecified order inside an equivalence class. `rank(x)` counts elements before x and `upperRank(x)` counts those not after x. `count` is their difference. `index(x)` is the first equivalent rank, or -1 if absent; no `operator==` is needed. `findByOrder(k)` returns an iterator, or `end()` for any out-of-range k. `begin`/`end` iterate the sorted values; iterators and ranks last until `rebuild`. CoordinateCompression deduplicates by comparator equivalence into ranks `[0, size())`. The representative of a class is unspecified, and distances are not preserved. `encode` maps each key to its rank, or -1 if unknown, in O(k * log(n + 1)). `decode(i)` returns the representative of rank i and asserts the range. `size`/`empty` are O(1).

### OrderedSet and OrderedMap

GNU PBDS order-statistics trees with unique keys: `OrderedSet<T, C>` (`null_type` mapped) and `OrderedMap<K, V, C>`. The native `insert`/`erase`/`find`/`lower_bound`/`order_of_key`/`find_by_order`/`join`/`split`/`operator[]` APIs are kept; `find_by_order(k)` for `k >= size()` returns `end()`. Never use `less_equal` as C. With stateful comparators, use `_GLIBCXX_ASSERTIONS` for checked builds: GNU PBDS's `_GLIBCXX_DEBUG` equality checker default-constructs a separate comparator.

### OrderedMultiSet

GNU PBDS tree over `(key, insertion ID)` pairs, so equivalent keys stay in insertion order. IDs in `[1, LLONG_MAX)` are never reused, even after `clear` or `rebuild`. At most `LLONG_MAX - 1` lifetime insertions and `INT_MAX` live elements (asserted). `insert` returns its token `(key, ID)`, and `erase(token)` removes exactly that occurrence; an erased token returns false. `eraseOne(x)` erases the earliest surviving equivalent occurrence, or returns false. `lowerBound`/`upperBound` give the first element `>= x` / `> x`; `prev(x)` gives the last element `<= x`. All three return `end()` when absent. Ranks and `findByOrder` follow SortedVector (negative or out-of-range k gives `end()`). Copies keep existing tokens, and later histories are local to each copy. Assignment replaces the destination history and invalidates its old tokens. Tokens from unrelated or replaced histories must not be passed to `erase`. `clear` is O(n) and `rebuild` is O(n + k * log(k + 1)); both invalidate iterators and tokens but keep the ID history. Public state must not be edited directly. C is a stable strict weak order.

### Monotone stack functions

All sequence and matrix dimensions fit `int`. `less` is a pure strict weak order; "smaller" means earlier in that order. `strict = true` treats comparator-equivalent elements as not smaller; `strict = false` accepts them. A missing previous index is -1 and a missing next index is n. `previousGreater`/`nextGreater` use `std::greater<T>`. The functions take O(n) time and O(n) space including the returned indices. `slidingMinimum`/`slidingMaximum(a, k)` return the index of the extremum of each window `[i, i + k)`, with `k > 0` asserted; `k > n` gives no windows. Equivalent extrema use the leftmost index unless `rightmost`. Workspace is O(min(n, k)) plus O(max(0, n - k + 1)) returned indices.

### HistogramRectangle and BinaryRectangle

Plain result records. With no positive rectangle (including empty input), the area is 0, every coordinate is -1, and `height` is 0.

### largestHistogramRectangle

Nonnegative `lng` heights (asserted once at entry), unit-width bars. The `lll` area covers the full domain. The witness is `[l, r) x [0, height)`, and ties take the smallest `(l, r)`. O(n) time and space.

### largestBinaryRectangle and maxZeroSubmatrix

`largestBinaryRectangle(a, value)` takes a rectangular matrix of 0/1 cells, validated once at entry, and `value` in {0, 1}. Empty dimensions are allowed. The witness is `[top, bottom) x [left, right)`, and ties take the smallest `(top, left, bottom, right)`. `int` dimensions keep the `lng` area exact. `maxZeroSubmatrix(a)` is the legacy area-only adapter: any nonzero `int` blocks, and the `lng` result widens the old `int` area. Both take O(n * m + n) time, where the `+ n` checks row shapes even when m = 0, and O(m) workspace.

## Correctness and cost — DS01

| Header | Implemented contract | Correctness and cost |
|---|---|---|
| `01-dsu.hpp` | `DSU(n)`, `findSet`, `uniteSets`, `isSameSet`, `getSize`, `count`, `groups`; vertices `[0,n)`, `n <= INT_MAX`. A union returns whether distinct components merged. Groups and members are ordered by smallest vertex. | Union by size bounds uncompressed depth by `log(n)`; compression preserves the root equivalence relation. Sizes and component count change only on successful unions. Amortized inverse-Ackermann union/find, O(n) initialization/storage. With no unions during grouping, each non-root edge is bypassed at most once, giving O(n) grouping including output. |
| `02-fenwick.hpp` | `Fenwick<T>(n/vector)`, `add`, `prefixSum`, `sum`, `lowerBound`; explicit `add1`, `prefixSum1`, inclusive `sum1`, `lowerBound1` adapters. `n <= INT_MAX`. | Each cell covers the interval ending at its low-bit boundary; increasing-index construction sends each completed cell to its parent exactly once, O(n). Update/prefix/search O(log(n)), O(n) storage. Subtraction requires an additive commutative group. |
| `03-segmenttree.hpp` | `SegmentTree<T,F>(n/vector, identity, op)`, `get`, `set`, `query`, `allQuery`, `maxRight`, `minLeft`; `n <= 2^29`. | Ordered left/right accumulators preserve noncommutative products. Identity padding makes the root and searches valid for nonpowers of two and empty trees. Build/storage O(n+1), updates/range searches O(log(n+1)), get/allQuery O(1). Bound keeps doubled indices and search arithmetic in signed `int`. |
| `04-sparsetable.hpp` | `SparseTable<T,F>(vector, op)`, nonempty `query` (associative idempotent) and `fold` (associative), inclusive `queryFast`/`querySlow` compatibility methods, diagnostic stream output; `n <= INT_MAX`. No identity or default constructor for T is needed. | Dyadic blocks preserve input order. Overlap duplicates a contiguous aggregate B, so associativity and B·B=B suffice, even without commutativity. `fold` partitions into disjoint blocks in order. Build/storage O(n log(n+1)), query O(1), fold O(log(n+1)). Only valid blocks are stored. |

Fenwick `lowerBound(x)` returns the first element index whose inclusive prefix reaches x, or n if absent; x <= 0 returns 0. Frequencies must stay nonnegative. The one-based search returns 0 if absent, avoiding `n+1` overflow; for x <= 0 it returns 1 for nonempty trees. One-based `sum1(l,r)` permits `l == r+1` when both endpoints are representable.

Segment searches require a deterministic predicate true on the identity, with truth forming a prefix as the queried interval extends. `maxRight(l,p)` returns n if no failure; `minLeft(r,p)` returns 0. Callbacks must not mutate the structure. Associativity, idempotence, arithmetic validity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

Fenwick searches: cell `v[i + d - 1]` covers `(i, i + d]` whenever `i` is a multiple of `2d`, so the top-down descent visits each power of two once and finds the largest `r` with a monotone true-prefix property g(r). `maxRight` uses `g(r) = r <= l or pred(P(r) - P(l))`, which is true at 0 and true-prefix because pred stays false as the range grows. `minLeft` returns 0 when `pred(P(r))` holds. Otherwise it descends on `h(i) = i < r and not pred(P(r) - P(i))`, which is again true-prefix, and returns the largest such `i` plus one. Only differences of prefix sums are evaluated, which the group contract already requires. `lowerBound`/`upperBound` are `maxRight(0, s < x)` and `maxRight(0, s <= x)`. `values` runs the linear build backwards: when index `i` is processed in decreasing order, cell `i` still holds its tree value, because only smaller indices subtract from it.

SparseTable2D: level `(i, j)` folds the `2^i x 2^j` block at each corner. It is built from `(i, j - 1)` by joining two horizontally adjacent halves, or for `j = 0` from `(i - 1, 0)` by joining two vertical halves, so each entry costs one f call. For `2^i <= x2 - x1 < 2^(i + 1)` the row intervals `[x1, x1 + 2^i)` and `[x2 - 2^i, x2)` cover `[x1, x2)`, and the same holds for columns. The four corner blocks therefore cover the rectangle exactly, with overlaps that idempotence absorbs. Commutativity and associativity make the combination order irrelevant. Build and storage are `sum over (i, j) of (n - 2^i + 1) * (m - 2^j + 1)`, which is O(n * m * log(n + 1) * log(m + 1)); a query is O(1).

SegmentTree `apply` and `values`, DSU `makeSet` and the braced-list constructors are one-line compositions of verified operations or of the vector constructors.

## Correctness and cost — DS02

| Header | Implemented contract | Correctness and cost |
|---|---|---|
| `05-prefix_sum.hpp` | `PrefixSum<T>` / `PrefixSum2D<T>`: size/vector construction, `prefixSum`, `sum`, `rebuild`. `DifferenceArray<T>` / `DifferenceArray2D<T>`: size/vector construction, `add`, non-destructive `values`, `clear`, `rebuild`. Dimensions `[0,INT_MAX-1]`; 2D arguments `(row1,col1,row2,col2)`. | Prefix accumulation and inclusion-exclusion cancel exactly the unwanted intervals. Difference endpoint/four-corner updates cancel outside the added region; axis-by-axis integration is their inverse. 1D build/storage/materialization O(n+1), 2D O((n+1) * (m+1)); queries/updates O(1). Rebuild and returned arrays use a further copy of the corresponding storage. |
| `06-sqrt_decomposition.hpp` | `SqrtDecomp<T,F>`: ordered monoid `query`, `get`, `set`/`setUpdate`, `opeUpdate`, `values`, `rebuild`. `SqrtRangeSum<T>`: `affine`, `add`, `assign`, `multiply`, `sum`, `get`, `set`, `values`, `rebuild`. `n <= INT_MAX`; B=0 chooses max(1,floor(sqrt(n))), positive B specifies block width. | Blocks store ordered folds; queries concatenate full blocks and at most two tails. Generic point update rebuilds its block, O(B); query O(B+n/B). Scalar affine sum maps s to a*s+b*length and composes new tags after old tags. Partial mutations push old tags then rebuild; reads apply tags without mutation. Scalar range update/query O(B+n/B), point get O(1), point set O(B). Build/materialization O(n), storage O(n+1). |
| `07-ordered_set.hpp` | `SortedVector<T,C>` retains duplicates; `CoordinateCompression<T,C>` deduplicates by comparator equivalence. Both provide `rank`, `upperRank`, `count`, `index`, iterators/`findByOrder`, `rebuild`; compression adds `encode`. `OrderedSet<T,C>` retains native PBDS unique-key API. `OrderedMultiSet<T,C>` adds exact insertion tokens, bounds/ranks/count/select, `erase`, earliest-equivalent `eraseOne`, `clear` and `rebuild`. | Sort and binary search preserve order classes; compression preserves order, not coordinate distances. Static build O(n log(n+1)), selection O(1), ranks O(log(n+1)). Strict (key,unique-ID) PBDS ordering gives O(log(n+1)) dynamic operations. IDs stay strictly between rank sentinels; they are never recycled after clear/rebuild, so old tokens cannot erase new occurrences. All storage O(n). |
| `08-monotone_stack.hpp` | Strict/nonstrict previous/next smaller/greater indices; sliding minimum/maximum indices with leftmost/rightmost equal ties; `largestHistogramRectangle`; `largestBinaryRectangle` for bit 0 or 1; widened legacy `maxZeroSubmatrix` for arbitrary integer blockers. | Each index is pushed/popped at most once. Discarded neighbor/deque candidates are dominated by nearer/better candidates. The histogram stack emits maximal spans; the final equal-height representative inherits the full left span. Every positive matrix rectangle appears in the histogram at its bottom row. O(n) sequence time/space; matrix O(rows * cols + rows) time and O(cols) workspace. |

Prefix/difference input vectors are copied; rebuilding safely accepts references to existing storage. An empty outer vector represents 0x0; explicit dimensions also represent 0xm and nx0. Difference updates skip unused far borders, so a whole singleton update by `LLONG_MIN` does not compute its unrepresentable negation. Intermediate sums/differences still must fit T. Online range-update/query structures belong to later batches; difference arrays reconstruct offline.

`SqrtRangeSum` requires commutative ring arithmetic, including conversion of block lengths into T. Pending tag intermediates must fit too. For a new transform (a,b) after (c,d), composition is (a*c,a*d+b); assignment has a=0 and needs no inverse. Default sqrt block size balances B+n/B analytically; callers can tune B for their workloads. No universally fastest block width is claimed. Generic monoid point updates remain O(B); the scalar affine specialization covers the additional range operations.

Static ordered structures support custom/stateful strict weak orders, strings, comparator-equivalent unequal objects, and `vector<bool>`. Selection returns an iterator or `end()`; missing `index`/encoded coordinates use -1. Rebuild invalidates ranks and iterators. Dynamic multisets permit at most `INT_MAX` live elements and at most `LLONG_MAX-1` insertions per history; a token is the `(key,ID)` pair returned by insertion. Copies retain existing tokens, with subsequent histories local to each object. Assignment replaces the destination history and invalidates its old tokens; foreign or replaced-history tokens must not be passed to erase. Comparator equivalence determines key identity. Native PBDS iterator lifetime rules apply; erased elements and clear/rebuild invalidate their corresponding iterators/tokens. Erased tokens within the current history can safely be retried and return false. `less_equal` is never a valid tree comparator.

Monotone missing neighbors use -1 for previous and n for next. Windows require k>0; k>n yields no windows. Histograms support nonnegative full-range `lng` heights with `lll` area and choose the smallest `(l,r)` maximal witness; zero area returns l=r=-1 and height=0. Binary rectangle witnesses use half-open coordinates and smallest `(top,left,bottom,right)` ties; zero area has all coordinates -1. Dimensions fit int, so matrix areas fit `lng`. Rectangular binary input is asserted for the binary API; the legacy zero-only adapter treats every nonzero int as a blocker.

## Migration and ownership boundaries

Original files in `OLD` and `97-Legacy` remain byte-for-byte references. The historical monolith map retains source ranges/hashes and distinguishes rewritten destinations from unchanged extractions. Basic aggregates gain all eight headers; existing Advanced headers remain existing-unverified.

The old DSU at `OLD/algorithms.cpp:1402–1477` mixed connectivity, weighted distances, parity/bipartiteness and edge counts. Its distance getter also narrowed `lng` to `int`. Basic connectivity names and `n/ncon/par/siz` remain; weighted three-argument unions, `dis`, `esz`, `is_bip`, `getDis`, `getEsiz` and `isBipartite` are not Basic APIs. No maintained consumers were found. The outstanding weighted/parity semantics belong to `29-weighteddsu.hpp`; rollback/component metadata belongs to `09-rollbackdsu.hpp` and Graphs dynamic-connectivity orchestration. Their inventories retain this handoff.

Sparse-table inclusive query names and printed diagnostic format remain available. Its public `v` storage changes from a flat padded array to valid rows; clients should use query methods. No maintained consumer accesses that old storage. New Basic `Fenwick` and `SegmentTree` names avoid collisions with the existing Advanced `FenTree` and lazy `SegTree`; the DS05-owned `00-monoids.hpp` remains unchanged.

The legacy `97-Legacy/02-sqrtdecomp.cpp` had invalid constructor defaults, silently clamped inclusive ranges, an unused generic lazy array with no public range mutator, and diagnostic stream output. The new monoid API asserts half-open ranges and repairs construction; `setUpdate` and `opeUpdate` names remain. Unreachable lazy bookkeeping and the diagnostic printer remain archived; explicit `values` exposes the actual sequence. Scalar affine blocks supply working range actions with a stated algebra. Legacy `maxZeroSubmatrix` keeps its arbitrary-nonzero blocker semantics and widens area from int to `lng`; the new binary API additionally returns a witness.

Future variants remain explicit: rollback/persistent/weighted DSU, range-add/multidimensional Fenwick, lazy/dual/persistent/dynamic segment trees, disjoint sparse tables, Cartesian trees and linear-preprocessing RMQ, Mo and recursive sqrt trees, dynamic balanced/persistent ordered trees, and general sliding-window aggregation. Their existence is not a gap in these Basic ownership batches. No claim is made that ordinary sparse-table preprocessing is the theoretical minimum for RMQ.

## Sources inspected

Source inspection and independent oracle tests are separate evidence. Implementations are written for the contracts above; legacy code and reference algorithms inform the audit. No online acceptance is claimed.

| Source | Actual reading and use |
|---|---|
| [cp-algorithms: Disjoint Set Union](https://cp-algorithms.com/data_structures/disjoint_set_union.html) | Union by size/compression, amortized versus per-operation bounds, advanced parity/potential distinctions. Original Tarjan papers cited by the article were not independently reviewed. |
| [cp-algorithms: Fenwick Tree](https://cp-algorithms.com/data_structures/fenwick.html) | Zero/one-based low-bit intervals, additive/group limitations, linear construction, multidimensional/range-update boundaries. |
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [cp-algorithms: Sparse Table](https://cp-algorithms.com/data_structures/sparse-table.html) | Dyadic preprocessing, disjoint logarithmic folds, overlapping idempotent O(1) query, alternative static RMQ structures. |
| [cp-algorithms: Sqrt Decomposition](https://cp-algorithms.com/data_structures/sqrt_decomposition.html) | Description/implementation and range increment/sum block variants; the Mo section was not needed or reviewed for this implementation. |
| [Competitive Programmer's Handbook, Antti Laaksonen](https://cses.fi/book/book.pdf), saved repository edition | Chapters 4, 8, 9, 15 and 27: PBDS, sliding windows, prefix/difference arrays, Fenwick/segment structures, DSU and block decomposition. Chapter 9 pp84–85/93 independently supports 1D/2D prefix inclusion-exclusion and endpoint differences. The chapter 15 DSU example alone establishes O(log(n)), not the compression bound. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | UnionFind, FenwickTree, SegmentTree, RMQ, SubMatrix, OrderStatisticTree references. The SubMatrix empty-input constructor is not copied; empty matrices require an explicit contract here. |
| [OI Wiki: 前缀和 & 差分](https://oi-wiki.org/basic/prefix-sum/) | Read 1D/2D prefix sums, four-corner difference signs and offline reconstruction; page update 2026-03-26. |
| [cp-algorithms: Minimum Stack / Minimum Queue](https://cp-algorithms.com/data_structures/stack_queue_modification.html) | Monotone deque, amortized push/pop and sliding-window extrema. |
| [cp-algorithms: Finding the largest zero submatrix](https://cp-algorithms.com/dynamic_programming/zero_matrix.html) | Row histograms/nearest barriers, equal plateaus, O(rows * columns) time and O(columns) workspace; article update 2022-06-08. |
| Installed GCC16 PBDS `tree_policy.hpp` and `order_statistics_imp.hpp` | Strict comparator ordering, `order_of_key` and out-of-range `find_by_order` semantics. |

Saved resource editions/checksums are recorded in `95-Resources/99-sources.json`. References were inspected on 2026-09-27. Bibliographic mentions of Fischer–Heun and Tarjan are not claims that their original proofs were inspected.

## Feature-to-test map

Each header has one runnable Python entry and one C++ oracle suite under `96-Local Testing/02-Data Structures`, with matching filename stem. The shared `_00_runner.py` resolves paths independently of the working directory. Checks are non-removable under `NDEBUG`. Failures report seed, mode, configuration/command, operation/input or history and expected/actual values; invalid probes require SIGABRT with assertion diagnostics. Exhaustive enumerations stop on the first discovered failing case; no automatic shrinking claim is made.

| Header / C++ groups | Full-mode coverage |
|---|---|
| DSU: `graphSubsets`, `unionHistories`, `randomCases`, `balancedTree` | Every simple graph through 6 vertices, self/duplicate unions, every length-5 union history on 3 vertices; independent relabeling connectivity/size/group oracle; 150 random cases with interleaved `makeSet` (id and singleton oracle); 65,536-vertex balanced merge/compression; copies/moves/reset; 7 probes. Quick: n<=4, depth 3, 35 trials, 1,024 vertices; stress: depth 6, 600 trials. |
| Fenwick: `exhaustive`, `randomCases`, `boundaries`, `typeCases` | Signed ternary arrays through length 6 with all ranges/point deltas/`set` values, `get` and `values` at every verify; quaternary frequencies through length 7 with every `lowerBound`/`upperBound` threshold and sentinel and every `maxRight(l)`/`minLeft(r)` with `s <= cap`, `s < cap` and `0 <= s <= cap` predicates against direct scans (the last is false on the negative partial differences before `l`, exercising the skip guard), a length-17 `s == 0` regression and a wrapping `uint` table; 150 random signed/frequency histories with sampled predicate searches; powers of two ±1 through 65,537; linear-build operation count; signed extrema, 128-bit (`set`, `upperBound`), unsigned modular, exact dyadic, braced singleton/list, argument aliasing, copy/move; 19 probes. Quick: n<=4, 35 trials, n<=1,025; stress: 600 trials, n<=262,145. |
| Segment: `exhaustive`, `booleans`, `randomHistories`, `noncommutative` | Ternary arrays through length 6, every range/search threshold, `values`, `apply`; every bool array through length 8 for OR and AND trees: `get`/`allQuery` bound to `const bool &`, `values`, every range, `maxRight`/`minLeft` against scans, `apply` (run under ASan/UBSan); braced singleton/list; 100 arrays of up to 220 points with 250 operations; ordered strings and direct affine evaluation modulo 97, `apply` operand order on strings at every point and on affine maps against `g(old(x))`, direction-sensitive search, nondefault payload, identity/empty/alias/copy/move; 14 probes. Quick: length 4, bool length 5; stress: length 8, bool length 10. |
| Sparse: `exhaustive`, `randomized`, `noncommutative`, `grids` | Ternary arrays through length 7, all min/max/sum folds, inclusive adapters and exact diagnostic output; 80 random arrays and powers of two ±1 through 129; ordered concatenation/direct affine evaluation, noncommutative idempotent rectangular bands, nondefault elements/copy-only callables, ownership/copy/move. 2D: every ternary grid with n * m <= 8 (all shapes including 0 x m and n x 0), every rectangle for min/max/gcd against incremental brute force; 40 random grids up to 20 x 20 with input ownership; shapes 1x33, 33x1, 8x9, 16x16, 17x15, 2x64; `lng` extremes; nondefault payload and copy-only callable; 19 probes, including a ragged grid whose first row is empty. Quick: length 4, 2D n * m <= 4; stress: length 9, 2D n * m <= 9, 150 grids. |
| Prefix: `arrays`, `matrices`, `randomized`, `boundaries` | Ternary arrays through length 7, all range sums/updates and depth-3 histories; every ternary matrix through 2x3 and every rectangle; 100 histories of 120 rectangle updates against literal cell loops; empty dimensions, rebuild/clear/copy/move/repeated reconstruction, argument/input aliases, int64/128-bit/modular/exact dyadic cases, braced singleton/list and non-convertibility from `int`; 16 probes. Quick: length 4, 2x2, depth 2, 15 cases; stress: length 8, depth 4, 400 cases. |
| Sqrt: generic, ordered, scalar exhaustive/history and regression groups | Ternary generic arrays through length 5 with default/1/2/n/n+2/INT_MAX widths; concatenation/reverse concatenation; every length-2 affine history on small ternary arrays and every half-open range; 100 histories of 200 operations; assignment/add/multiply order, pending tags, read constness, copy/move/rebuild, 128-bit/modular/exact dyadic/alias cases; bool OR payload with `get` bound to `const bool &` for widths 0–3; braced singleton/list; constructors not implicitly convertible; 22 probes. Quick: generic n<=3, lazy n<=2 depth 1, 20 trials; stress: 400 trials. |
| Ordered: static, dynamic-history, random and PBDS adapter groups | SortedVector `rank`/`upperRank`/`count`/`index`/`findByOrder`/`size`/`empty`/`rebuild` and `begin`..`end` iteration against a `std::multiset` oracle; CoordinateCompression `encode`/`decode`/`rebuild` and iteration against a `std::set` oracle; static 3-symbol vectors through length 6 and stateful/descending/string/bool/int64-extreme keys; OrderedMultiSet `insert`/`erase`/`eraseOne`/`clear`/`rebuild`/`size`/`empty`/`begin`/`end`/`lowerBound`/`upperBound`/`prev`/`rank`/`upperRank`/`count`/`findByOrder` over five-action histories through depth 5 and 12 random histories of 180 operations against a sorted token vector; stale/exact tokens, copy/move, ID exhaustion; OrderedSet and OrderedMap against `std::set`/`std::map` with a stateful comparator; 4 probes. |
| Monotone: boundaries, histograms, matrices and random/generic groups | Ternary arrays through length 7 for all neighbor/window policies; quaternary histograms through length 7 against every interval; all shapes through 4x4 with at most 12 cells against every rectangle for both bits; 100 random cases, arbitrary legacy blockers, generic comparator equivalence, full `lng` heights and a 100,000-bar plateau; independent witness validation/complement symmetry; 11 probes. |

Quick modes reduce enumeration lengths/history depths/random counts and run optimized plus checked builds. Full runs every feature above in optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`), and sanitizer (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer -fno-pie -no-pie`) builds. Leak checking is enabled. Ordered checked builds instead use `_GLIBCXX_ASSERTIONS`: GCC16 PBDS `debug_map_base` default-constructs its diagnostic comparator, which can disagree with a valid supplied stateful comparator. A reproduced equivalence-width example aborts only in that debug layer; all comparator oracle cases remain in the checked suite. This is an instrumentation limitation, not a skipped algorithm feature.

Stress modes extend the enumerations/random counts: DSU history depth 6 and 600 random cases; Fenwick 600 cases and boundaries through 262,145; segment/sparse exhaustive lengths 8/9; prefix length 8/history depth 4/400 random cases; monotone length 9, all binary matrices through 4x4 and million-bar plateau. Each suite's source records its concrete mode limits. Declared algebra/monotonicity/representability conditions and impractically large allocations are justified contracts, not runtime proof by finite tests.

## Verification record — 2026-09-27 (original P006)

All eight per-header entries passed `--mode full --seed 20260927`: 24 configuration runs and all 100 assertion probes. DS01 completed its full runs and Basic/All multiple-TU check before DS02 implementation began. Full runs include 564,922 segment checks, 3,088,896 sparse checks, 681,510 prefix checks, 495,927 ordered checks and 808,326 monotone checks per configuration; DSU/Fenwick/sqrt report named corpus groups instead of a scalar check count. Independent peer reviews covered every header, overflow arithmetic, algebra, aliases, tie policies and feature ownership.

Compiler: GCC 16.2.1 20260810, GNU++20; Python: CPython 3.14.7; Linux x86-64. Actual full commands used the eight `NN-name_tester.py` entries with the options above. The segment sanitizer retry additionally used `--configuration ASan-UBSan`. Initial sandbox ASan/UBSan processes hit LeakSanitizer's explicit ptrace restriction; the successful full sanitizer runs used approved execution outside that sandbox with leak checking enabled. No sanitizer failure is counted as a pass.

`python3 '96-Local Testing/02-integration.py'` passed all **65** present standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL Workspace compilation. The per-header full suites already supplied P006 sanitizer checks; unrelated Core sanitizer corpora were not repeated. `python3 '96-Local Testing/03-consistency.py'` passed inventory/batch/package paths, aggregates, source/archive hashes and Markdown links with zero errors. Original archived bytes remain unchanged.

Shared discovery also passed all eight quick suites with seed 42 when invoked by absolute path from `/tmp`: `01-run.py --mode quick --seed 42 --filter '02-Data Structures' --no-integration`. This checks working-directory independence, environment option forwarding and discovery of the new entries; it does not replace their full runs.

To reproduce the complete package corpus through shared discovery, or extend it:

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode stress --seed 42 --rounds 3 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

Stress mode is available but was not run as completion evidence. GCC14 itself, other standard-library versions and other platforms were not executed. There are no remaining P006-owned implementation or verification gaps. The weighted/parity/component metadata handoff above belongs to later packages, which remain planned. No online submissions or acceptance claims were made.

## Construction comparison

The [benchmark driver](<../96-Local Testing/02-Data Structures/90-foundations_benchmark.py>) and [record](<../96-Local Testing/02-Data Structures/90-foundations_benchmark.json>) compare the final Fenwick linear constructor with constructing zeros and applying n point additions. Both are checked against an independent sum. Run:

```bash
python3 '96-Local Testing/02-Data Structures/90-foundations_benchmark.py' --runs 7 --seed 20260927
```

Recorded CPU: Intel Core i9-11900H @ 2.50GHz; GCC16, `-std=gnu++20 -O2 -DNDEBUG`, seed 20260927, uniform integer values `[0,100]`, seven independent process samples, one untimed linear warmup per row. Both timed paths include allocation, construction, a checksum query and destruction. The source vector and warmup remain outside the timed construction; peak live vector payload is approximately `3 * n * sizeof(lng)` plus allocator/runtime overhead. Raw samples and source hashes are retained. Other verification work shared the host, so these are noisy observations without a timing gate.

| n | Builds per sample | Linear median total (ms) | Point-add median total (ms) |
|---|---|---|---|
| 32 | 8192 | 0.299186 | 0.640771 |
| 4096 | 64 | 0.276973 | 1.267312 |
| 262144 | 1 | 1.203464 | 3.001825 |

The linear constructor is both asymptotically appropriate and faster in these measured cases. No ISA kernels, Barrett/Montgomery use or empirically selected dispatch thresholds were introduced. The other families use their standard contest algorithms with stated preprocessing/storage tradeoffs; no universal performance superiority is claimed.

## Re-audit — 2026-10-07

Package P006 was re-audited under the current rules, treating the previous verification as existing-unverified. Before any edit, the inventory rows were compared with the code and the testers. The only gap was the missing `SparseTable2D`; `pull` was listed but had no oracle test and is an internal helper. The unchanged suites passed full mode with seed 20260927 (8 suites x 3 configurations). The 24 confirmed findings in `00-Guidelines/23-reaudit-findings/P006.md` were then fixed or resolved as follows.

### Confirmed findings and their disposition

| # | Finding | Disposition |
|---|---|---|
| 1 | A moved-from Fenwick keeps `n` with empty `v` | Resolved by contract ([Common](#common)): a moved-from structure may only be assigned or destroyed. This is the minimum the standard library itself requires of moved-from user types; standard containers stay valid but unspecified, while these structures' size fields keep stale values. Dropping `n` would change a public field that clients and testers read, and DSU, SegmentTree, PrefixSum and SqrtDecomp share the pattern. The testers only reassign moved-from objects. |
| 2 | `get`/`allQuery` return a dangling reference for `T = bool` | Fixed: both return `vector<T>::const_reference`. The same defect in `SqrtDecomp::get` and the new `CoordinateCompression::decode` is avoided the same way. |
| 3, 4 | A braced singleton picks the size constructor (`PrefixSum`, `DifferenceArray`, `SqrtDecomp`, `SqrtRangeSum`); SqrtDecomp constructors are implicit | Fixed: `std::initializer_list<T>` constructors make a braced list always mean values, also added to `Fenwick` and `SegmentTree`, which had the same trap. Both SqrtDecomp constructors are now `explicit`. The testers check braced singletons and lists, and `static_assert` non-convertibility from `int`/`vector`. |
| 5 | `SparseTable2D` missing | Implemented with its contract above and an exhaustive and random oracle suite. |
| 6 | `pull` listed but untested and internal | Removed from the row; documented as an unchecked internal helper, like SqrtRangeSum's `apply`/`push`/`pull`. |
| 7 | No bool payload test | Added `booleans()` to the segment tester (OR and AND trees, every array through length 8) and a bool SqrtDecomp case; both run under ASan/UBSan. |
| 8 | `begin` never exercised; Ordered map not per operation | Added `begin`..`end` iteration checks for SortedVector and CoordinateCompression; the Ordered map row now names every operation. |
| 9 | Complexity line not directly above `struct DSU` | Fixed; alpha is defined on the complexity line. |
| 10–12, 17, 19, 23, 24 | Closing-brace rule (DSU, Fenwick, segment do-while, sqrt, ordered lambda, monotone lambdas, testers) | Fixed in every header, tester and the benchmark; `03-consistency.py --braces` reports nothing for the package. The tester anonymous namespaces now close with `} // namespace`. |
| 13 | prefix_sum_detail helpers without complexity lines | `checkSize` and `checkRows` were inlined as entry asserts. The remaining `checkedSize` carries `// T: O(1), M: O(1)`. |
| 14 | Complexity lines split or not directly above the struct | Every struct and free function now has its single-line bound directly above it; contract prose moved here. |
| 15 | `DifferenceArray2D::add` delta named `x` | Renamed to `w`. |
| 16 | SqrtDecomp parameters `ID`, `f_` | Renamed to `id`, `f`, as in SegmentTree. |
| 18 | Block-index asserts inside affine/rebuild loops | Removed from the internal helpers; the public operations check their ranges once at entry. |
| 20 | Methods not grouped | OrderedMultiSet and SortedVector are now grouped construction / access / mutation / queries with blank lines; the other headers were regrouped the same way. |
| 21 | Missing complexity lines above the result structs and `nextGreater` | Fixed: one line above each adjacent pair (`previousSmaller`/`nextSmaller`, the greater adapters, the sliding pair, the two result records, the binary/zero-matrix pair), within the 8% comment cap. |
| 22 | Per-element asserts in the histogram and binary loops | Nonnegative heights are asserted once at entry with `std::ranges::all_of`; 0/1 cells are validated once at entry. The negative-height, late-negative, negative-cell and large-cell probes still fire. |

### Changes beyond the findings

- Comment cap: every header keeps at most two comment lines per struct or function and at most 8% comment lines. The removed contract text is in [Contracts](#contracts). The feature-map comments at the top of four testers and one benchmark comment line moved to the feature-to-test map.
- Completeness sweep ([81-sources.md](81-sources.md), omissions in [80-notes.md](80-notes.md#p006-re-audit-omissions)) added: DSU `makeSet`; Fenwick `get`, `set`, `values`, `upperBound`, `maxRight`, `minLeft`; SegmentTree `values`, `apply`; CoordinateCompression `decode`; OrderedMultiSet `prev`; the `OrderedMap` alias.
- The shared runner compiles every configuration with `-Wall -Wextra -Wconversion -Werror`. No header or tester produces a warning.
- Fenwick `lowerBound`/`upperBound` share a direct descent (`descend`) instead of calling `maxRight`. Measured on the same host (GCC 16.2, `-O2`, n = 2^20 values uniform in [0, 99], 2^23 uniform targets, 3 process runs each, shared with a running stress suite), routing through `maxRight` took a median of 2,091 ms against 1,856 ms for the previous loop. The direct descent took 1,853 ms against 1,966 ms for the previous loop in the paired rerun, so it is at parity.

### Feature-to-test additions

Covered in the updated [feature-to-test map](#feature-to-test-map): SparseTable2D (exhaustive grids, random grids, boundary shapes, extremes, nondefault payload, 7 probes); Fenwick `get`/`values` at every verification, `set` in the exhaustive histories, `upperBound`/`maxRight`/`minLeft` against direct scans with 6 probes; SegmentTree bool trees, `values`, `apply` and an `apply-end` probe; DSU `makeSet` interleaved with random unions; CoordinateCompression `decode`, iteration and 2 probes; OrderedMultiSet `prev` at every rank probe; OrderedMap against `std::map`; braced-list constructors in five suites. The probes `generic-pull-end`, `lazy-apply-end`, `lazy-push-end` and `lazy-pull-end` were removed with the helper asserts. 112 assertion probes remain, up from 100.

### Independent review — 2026-10-07

`@reviewer` confirmed findings 2–24 fixed and accepted the contract-based resolution of finding 1, after a wording correction. It ran its own ASan/UBSan oracles: 3,000 random Fenwick and SegmentTree search arrays, and 300 SparseTable2D grids up to 70 x 70 with the storage bound checked. It raised five findings, all fixed:

| Finding | Fix |
|---|---|
| `SegmentTree::apply` operand order untested (operand-swap mutant passed) | Noncommutative group applies to strings at every point and to affine maps checked against `g(old(x))`; the mutant now fails. |
| Fenwick `maxRight` skip guard `i + d <= l` untested (removal mutant passed) | `0 <= s <= cap` predicates, the length-17 `s == 0` regression and a wrapping `uint` table; the mutant now fails. |
| SparseTable2D accepted ragged rows when the first row is empty | The row-shape assert runs before the empty-grid return, with the size product checked before narrowing; new probe `grid-ragged-empty-first`. |
| Restated contract comments inside the Fenwick and SegmentTree bodies | Deleted. |
| Fenwick domain line claimed every search returns n | Reworded to name `lowerBound`/`upperBound`. |

Accepted as is: each adjacent pair of functions shares one complexity line (finding 21's suggested form, within the 8% cap). The `Fenwick<lng> f{n}` braced form now means one value, as documented in [Common](#common); the reviewer found no dependent relying on the old meaning.

### Commands and results

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration   # before any edit
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-Data Structures/02-fenwick_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-Data Structures/04-sparsetable_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s. It passed all 8 suites x 3 configurations and all 112 assertion probes, and per-configuration check counts include 730,507 segment, 7,485,617 sparse, 681,515 prefix, 564,250 ordered and 808,148 monotone. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. After those fixes, the three changed suites (Fenwick, segment, sparse) passed stress again with seed 9, including 4,087,658 segment and 30,870,208 sparse checks per configuration. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. GCC 14.2 itself was not run. No online submission was made.

