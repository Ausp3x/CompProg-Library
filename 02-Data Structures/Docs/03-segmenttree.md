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

### Original P006 verification — 2026-09-27

```bash
python3 '96-Local Testing/01-run.py' --mode full --seed 20260927 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/01-run.py' --mode quick --seed 42 --filter '02-Data Structures' --no-integration
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

All eight per-header entries passed `--mode full --seed 20260927` on 2026-09-27 (24 configuration runs, all 100 assertion probes of the package), invoked directly as `python3 '96-Local Testing/02-Data Structures/03-segmenttree_tester.py' --mode full --seed 20260927`. Compiler GCC 16.2.1 20260810, GNU++20; CPython 3.14.7; Linux x86-64. Initial sandbox ASan/UBSan processes hit LeakSanitizer's explicit ptrace restriction; the successful full sanitizer runs used approved execution outside that sandbox with leak checking enabled. No sanitizer failure is counted as a pass. `02-integration.py` passed all **65** present standalone/aggregate headers, scalar and available AVX2 multiple-translation-unit linkage, and LOCAL/non-LOCAL Workspace compilation; `03-consistency.py` passed with zero errors. The quick run through shared discovery (seed 42, invoked by absolute path from `/tmp`) passed all eight suites and checks working-directory independence, option forwarding and discovery; it does not replace the full runs. The package stress command (`--mode stress --seed 42 --rounds 3`) was available but not run as completion evidence. Independent peer reviews covered every header, overflow arithmetic, algebra, aliases, tie policies and feature ownership. The segment sanitizer retry additionally used `--configuration ASan-UBSan`.

### Re-audit — 2026-10-07

Package P006 was re-audited under the current rules, treating the previous verification as existing-unverified. The rows were compared with the code and the testers before any edit; the 24 confirmed findings in `00-Guidelines/23-Reaudit Findings/p006.md` were then fixed or resolved. Findings that concern this header:

| # | Finding | Disposition |
|---|---|---|
| 2 | `get`/`allQuery` return a dangling reference for `T = bool` | Fixed: both return `vector<T>::const_reference`. |
| 3, 4 | A braced singleton picks the size constructor | `std::initializer_list<T>` constructors added to `SegmentTree`, which had the same trap as the prefix and sqrt structures; the tester checks braced singletons and lists. |
| 7 | No bool payload test | Added `booleans()` (OR and AND trees, every array through length 8); runs under ASan/UBSan. |
| 10–12, 17, 19, 23, 24 | Closing-brace rule (DSU, Fenwick, segment do-while, sqrt, ordered lambda, monotone lambdas, testers) | Fixed in every header, tester and the benchmark; `03-consistency.py --braces` reports nothing for the package. The tester anonymous namespaces now close with `} // namespace`. |
| 14 | Complexity lines split or not directly above the struct | Every struct and free function now has its single-line bound directly above it; contract prose moved to this document. |
| 20 | Methods not grouped | OrderedMultiSet and SortedVector are now grouped construction / access / mutation / queries with blank lines; the other headers were regrouped the same way. |

Changes beyond the findings: Comment cap: every header keeps at most two comment lines per struct or function and at most 8% comment lines; the removed contract text is under Contracts. The shared runner compiles every configuration with `-Wall -Wextra -Wconversion -Werror`; no header or tester produces a warning. The completeness sweep ([00-sources.md](00-sources.md)) added `values` and `apply` (with an `apply-end` probe).

Independent review (`@reviewer`, 2026-10-07) ran its own ASan/UBSan oracles, including 3,000 random SegmentTree search arrays. Its findings for this header, all fixed:

| Finding | Fix |
|---|---|
| `apply` operand order untested (operand-swap mutant passed) | Noncommutative group applies to strings at every point and to affine maps checked against `g(old(x))`; the mutant now fails. |
| Restated contract comments inside the body | Deleted. |

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

All runs used GCC 16.2.1, GNU++20 and CPython 3.14 on Linux x86-64 (Intel Core i9-11900H), and all passed. Every tester build used `-Wall -Wextra -Wconversion -Werror`, in the optimized (`-O2 -DNDEBUG`), checked and ASan/UBSan configurations with leak checking. The unchanged suites passed full mode with seed 20260927 before any edit. The final full run (seed 20261007, after the review fixes) took 4 min 10 s for the package and passed all 8 suites x 3 configurations and all 112 assertion probes. Stress over two rounds (seeds 7 and 8, 21 min) passed all 8 suites before the review fixes. `02-integration.py --sanitizers` passed 102 standalone/aggregate headers, multiple-translation-unit linkage, the Workspace build and the sanitizer self-tests. `03-consistency.py` reports no errors and checks the closing-brace rule on the package's headers and testers. GCC 14.2 itself was not run. No online submission was made. After the review fixes the segment suite passed stress again with seed 9 (4,087,658 checks per configuration).

## Sources

| Source | Actual reading and use |
|---|---|
| [AtCoder Library: Segment Tree](https://github.com/atcoder/ac-library/blob/master/document_en/segtree.md) | Monoid contract, ordered range products, empty identity, max-right/min-left predicates and endpoints. ACL is CC0. |
| [Competitive Programmer's Handbook](https://cses.fi/book/book.pdf), saved repository edition, chapter 9 | Fenwick/segment structures. |
| [KACTL](https://github.com/kth-competitive-programming/kactl), saved repository PDF | SegmentTree reference. |

References inspected on 2026-09-27.

## Limits and handoffs

The Basic `SegmentTree` name avoids a collision with the lazy `SegTree` (`12-lazysegmenttree.hpp`); the DS05-owned `00-monoids.hpp` remains unchanged. Lazy/dual/persistent/dynamic segment trees are future variants, not gaps of this row. Left out of the 2026-10-07 sweep, with reasons in [00-notes.md](00-notes.md#p006-re-audit-omissions): `reset`/`build`. No P006-owned gap remains.
