# 03-segmenttree.hpp — evidence

Common contracts for rows 01–08 (indices, preconditions, algebra, moved-from state, braced-list construction, `T = bool`) are in [00-notes.md](00-notes.md#foundations-rows-0108-p006). Package P006 (DS01 then DS02); C01/P002 supplies the verified template, integer aliases and PBDS imports.

## Contracts

### SegmentTree

`0 <= n <= 2^29`, so doubled indices and search arithmetic stay in `int`. f is associative with two-sided identity `id`, and operand order is preserved. The empty range returns `id`. `get` and `allQuery` return `vector<T>::const_reference`: a reference owned by the tree, or a `bool` value for `T = bool`. `values()` copies the leaves in O(n). `apply(p, x)` sets point p to `f(a[p], x)`. `maxRight(l, pred)` returns the largest `r` in `[l, n]` with `pred(query(l, r))`; `n` means no failure. `minLeft(r, pred)` returns the smallest `l` in `[0, r]`; 0 means no failure. Predicates are deterministic, true on `id` (asserted), and stay false as the range grows (truth forms a prefix as the queried interval extends). Callbacks must not mutate the tree. Costs count calls and copies of T and f as O(1). Vector, size and braced-list constructors exist.

### Correctness and cost

Ordered left/right accumulators preserve noncommutative products. Identity padding makes the root and searches valid for nonpowers of two and empty trees. Build/storage O(n+1), updates/range searches O(log(n+1)), get/allQuery O(1). The size bound keeps doubled indices and search arithmetic in signed `int`. `apply` and `values` are one-line compositions of verified operations. Associativity and predicate monotonicity are semantic caller contracts, not properties a generic library can cheaply infer.

## Feature-to-test map

The Python entry and C++ oracle suite share the stem under `96-Local Testing/02-Data Structures`; runner, configurations and failure reporting are described in [00-notes.md](00-notes.md#foundations-rows-0108-p006).

| Operation | Test (`03-segmenttree_tester.cpp` group) | Oracle |
|---|---|---|
| `get`, `set`, `query`, `allQuery`, `maxRight`, `minLeft`, `values`, `apply` | `exhaustive` | Ternary arrays through length 6, every range/search threshold, `values`, `apply` |
| `T = bool` | `booleans` | Every bool array through length 8 for OR and AND trees: `get`/`allQuery` bound to `const bool &`, `values`, every range, `maxRight`/`minLeft` against scans, `apply` (run under ASan/UBSan) |
| Random histories | `randomHistories` | 100 arrays of up to 220 points with 250 operations |
| Operand order and construction | `noncommutative` | Ordered strings and direct affine evaluation modulo 97, `apply` operand order on strings at every point and on affine maps against `g(old(x))`, direction-sensitive search, nondefault payload, identity/empty/alias/copy/move; braced singleton/list; 14 probes including `apply-end` |

Quick: length 4, bool length 5. Stress: length 8, bool length 10. Per configuration: 564,922 checks (full, 2026-09-27), 730,507 (full, 2026-10-07), 4,087,658 (stress seed 9, 2026-10-07).

## Commands and results

Re-audit, 2026-10-07:

```bash
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 2 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode full --seed 20261007 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode stress --seed 9
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. After the review fixes the segment suite passed stress again with seed 9 (4,087,658 checks per configuration).

## Sources

| Source | Actual reading and use |
|---|---|
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SegmentTree reference. |

References inspected on 2026-09-27.

## Limits and handoffs

The Basic `SegmentTree` name avoids a collision with the lazy `SegTree` (`12-lazysegmenttree.hpp`); the DS05-owned `00-monoids.hpp` remains unchanged. Lazy/dual/persistent/dynamic segment trees are future variants, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`.

## History

- 2026-09-27: original P006 verification, full suites, quick discovery run, integration and consistency passed.
- 2026-10-07: P006 re-audit, findings 2, 3, 4, 7, 10–12, 14, 17, 19, 20, 23, 24 (of 24 package findings) addressed in this header, added `values` and `apply` (with an `apply-end` probe), full and stress passed on g++.
