# 85-levelancestor.hpp — evidence

Owned by package P042 / GR70 (level ancestor); omissions are in [00-notes.md](00-notes.md#p042-gr70-omissions). Implementation: [85-levelancestor.hpp](../85-levelancestor.hpp).

## Contracts

### LevelAncestor

`LevelAncestor(g, root=-1)` builds the structure from the canonical `Graph` or `CsrGraph` and inherits the forest layout of `lca_detail::Forest` ([07-lca.md](07-lca.md#lca_detailforest-lca-and-eulerlca)): an undirected forest (loops, parallel edges and cycles violate preconditions), every vertex included, an explicit root orients its own component first and every other component is rooted at its smallest vertex, `root=-1` roots each component at its smallest vertex, the empty forest is valid. The object owns its state, survives the graph, and copies and moves are independent values; the exposed fields are read-only.

`query(u, k)` returns the k-th ancestor of `u` (`k = 0` is `u`), or `-1` when `k > dep[u]`; it asserts `0 <= u < n` and `k >= 0`. `k` is `lng`, so any 64-bit count is accepted. The ancestor at depth `d` is `query(u, dep[u] - d)`.

Layout: every vertex lies on one long path (deep child = the first child of greatest height). For the path with top `t`, bottom leaf `b` and `len` vertices, `ladder` holds `b, parent(b), ...` for `len + min(len, dep[t])` entries; `pos[x]` is the absolute `ladder` index of `x` inside its own path's segment and `bottom[x]` its path's leaf. Each leaf `b` stores, for `2^h <= dep[b]`, the `pos` of its `2^h`-th ancestor at `jumps[jump_start[b] + h]`, which saves a dependent load per query. `ladder.size() <= 2 * n`; `jumps.size()` is the sum over leaves of `bit_width(dep[b])`, so build time and memory are O(n + l * log(n)) with l the leaf count: O(n) for a path, O(n log n) worst case (caterpillar).

### levelAncestorOffline

`levelAncestorOffline(g, queries, root=-1)` answers `(u, k)` pairs with `LevelAncestor::query`'s contract in query order, after one forest traversal: queries are bucketed by vertex (stable counting sort) and answered in preorder from the current root path. It asserts `queries.size() <= INT_MAX`, every `u` in range and `k >= 0`; T and M are O(n + q), with no logarithmic table.

### Correctness

- Preorder keeps `path[0..dep[u]]` equal to the root path of `u`: entries above `dep[u]` were last written by the most recently visited vertex at that depth, which is an ancestor of `u` in preorder. So each leaf reads its long path, its ladder and its jumps from `path`.
- Query: let `b = bottom[u]` and `d = k + dep[b] - dep[u] <= dep[b]`. For `d = 0` the answer is `u`. Otherwise with `h = floor(log2 d)` and `x` the `2^h`-th ancestor of `b`, `x` has height at least `2^h + 1`, so its long path `P` has `len >= height(x) > 2^h > r = d - 2^h`. The segment of `P` reaches `(dep[x] - dep[t]) + min(len, dep[t])` entries above `x`, which is at least `min(len, dep[x]) >= r` because `r <= dep[b] - 2^h = dep[x]`. So `ladder[pos[x] + r]`, read as `ladder[jumps[jump_start[b] + h] + r]`, is the answer, and `h < bit_width(dep[b])` keeps the jump index inside `b`'s block.
- The segments of distinct long paths are disjoint, each `len + min(len, dep[t]) <= 2 * len`, and the paths partition the vertices, so `ladder.size() <= 2 * n`.
- Offline: the bucket of `u` is visited exactly when `path` holds `u`'s root path, so `path[dep[u] - k]` is the k-th ancestor.

### Legacy mapping

No archived level-ancestor implementation exists; the row came from the AUDIT candidate list. `LCA::kthAncestor` (row 07, O(log n)) and `HLD::kthAncestor` (row 15, O(log n), O(n) memory) answer the same query by other algorithms and stay ([duplicates.md](<../../00-Guidelines/23-Sweep Ledgers/duplicates.md>)).

## Feature-to-test map

Tester: [85-levelancestor_tester.cpp](<../../96-Local Testing/04-Graphs/85-levelancestor_tester.cpp>) with [85-levelancestor_tester.py](<../../96-Local Testing/04-Graphs/85-levelancestor_tester.py>). The oracle is the tester's own BFS rooting under the documented root rule with parent walks and a root-path DFS batch; the forest enumeration is checked against OEIS A001858.

| Feature | Tests |
|---|---|
| construction: forest fields, deep-leaf bottoms through the first deepest child, `pos` and segment tiling, `ladder.size() <= 2n`, `jumps.size()` formula, every jump entry is its ancestor's own `pos` | `structure()` on every graph, large ones included |
| `query` all `(u, k)`, `k` in `[0, n+1]`, `INT_MAX`, `2^40`, `LLONG_MAX`, every root in `{-1, 0..n-1}`, `Graph` and `CsrGraph` | `verify()` in `exhaustive()` (all labeled forests through n=4 quick, 6 full, 7 stress), `randomized()`, `shapes()` |
| ladder and jump edge cases | `shapes()`: path, star, binary, caterpillar, broom, recursive, comb (equal-length ties), spider, path-binary, deep random, multi-component forests, relabeled; `sampled()`: `k = 2^h - 1, 2^h, 2^h + 1, dep - 2^h`, closed forms on the path |
| `levelAncestorOffline` | `verify()` (all pairs, shuffled with duplicates, empty list, CSR input); `sampled()` on large graphs; `boundary()` (n=0, n=1, many queries on one vertex) |
| ownership and rebuild | `boundary()`: snapshots, copy, move, rebuild |
| large inputs | 2,000 (quick), 200,000 (full), 500,000 (stress; path, caterpillar, recursive) vertices, sampled against the oracle |
| preconditions (16 probes) | directed, cycle, loop, parallel, root-negative, root-end, empty-root, query-negative, query-end, k-negative, offline-directed, offline-cycle, offline-root-end, offline-query-negative, offline-query-end, offline-k-negative |

## Commands and results

2026-11-04, Linux x86-64 (AMD Ryzen AI 9 HX 370), GCC 16.0.1 20260322 and GCC 14.3.0 (`CXX=g++-14`), GNU++20, CPython 3.14, the runner's optimized, checked and ASan/UBSan builds with `-Werror`:

| Command | Result |
|---|---|
| `85-levelancestor_tester.py --mode quick` | PASS, 3,946,174 checks optimized, 3,842,520 checked, 16 assertion probes, MEMORY peak 492 MB |
| `85-levelancestor_tester.py --mode full --seed 1` | PASS, 3 configurations: 50,713,350 checks optimized, 50,076,758 checked and ASan/UBSan, 16 probes, MEMORY peak 894 MB |
| `CXX=g++-14 85-levelancestor_tester.py --mode full --seed 1` (floor) | PASS, the same counts, MEMORY peak 894 MB |
| `85-levelancestor_tester.py --mode stress --seed 2` | PASS, 3 configurations: 177,477,537 checks optimized, 172,226,815 checked and ASan/UBSan, MEMORY peak 1470 MB |
| `02-integration.py` | PASS: 190 standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace, MEMORY peak 1715 MB |
| `03-consistency.py` (linked worktree) | No errors |

The ASan/UBSan stress binary takes about 140 s alone against the runner's 180 s limit, so the 500,000-vertex stress round runs the path, caterpillar and recursive shapes only.

```bash
python3 '96-Local Testing/04-Graphs/85-levelancestor_tester.py' --mode quick
python3 '96-Local Testing/04-Graphs/85-levelancestor_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/85-levelancestor_tester.py' --mode full --seed 1
python3 '96-Local Testing/04-Graphs/85-levelancestor_tester.py' --mode stress --seed 2
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/04-mutation.py' run 04-Graphs/85-levelancestor.hpp
python3 '96-Local Testing/03-consistency.py'
```

2026-11-09: `python3 '96-Local Testing/05-extraction.py' ops --filter 04-Graphs/85-levelancestor.hpp` passed (4 operations, 0 failures on `g++` and `g++-14`), and `testers --filter 85-levelancestor` passed (1 quick run on an extracted build, 0 failures).

## Mutation testing

Record: mewt 4.0.0 on blob `7a481116b77519af793af7e3e01f1de17593ce34` (2026-11-09, seed 1, 3 lines changed since blob `10ef301bd166`): 0 mutants, 0 caught (0 by timeout), 0 survived, 0 untested.

Earlier campaign: mewt 4.0.0 on blob `10ef301bd166de6663600ef1e90dd304aa3985dc` (2026-11-04, seed 1, 4 lines changed since blob `f7283eb13963`): 30 mutants, 28 caught (0 by timeout), 2 survived, 0 untested.

Stage seconds: quick 39, full-optimized 19, full-all 174. The base campaign on blob `f7283eb13963` (whole file, 255 mutants, 241 caught, 14 survived, 0 untested; quick 264, full-optimized 108, full-all 1146 seconds) covers the unchanged lines. The changed-line survivors `COS 194` and `AOS 121` are the base's `COS 192` and `AOS 119` on the moved lines. Survivors, with type context, reachability, downstream use, semantic comparison and distinguishing input checked for each:

| Mutant | Line | Argument |
|---|---|---|
| `COS 159` `p != -1` → `p > -1` | 16 | equivalent: `parent` is `-1` or a vertex |
| `CR 23`, `AOS 64` (`reserve` removed, `2 * n` → `2 + n`) | 17 | equivalent: capacity only; `push_back` grows the vector and no pointer into `ladder` is held |
| `COS 169` `deep[u] != -1` → `> -1` | 20 | equivalent: `deep` is `-1` or a vertex |
| `AS 54` `min(len, d - len + 1)` → `min(d - len + 1, len)` | 23 | equivalent: `min` of two `int`s is symmetric in value |
| `COS 182`, `COS 194` (`<` → `!=` in counting loops) | 23, 27 | equivalent: `i` and `h` start at 0, step by 1 and the bound is at least 0 and fixed in the loop |
| `COS 213` `d == 0` → `d <= 0` | 34 | equivalent: `d = k + dep[b] - dep[u] >= 0` because `0 <= k` and `b` lies below `u` |
| `AOS 121` `d - (1 << h)` → `d % (1 << h)` | 36 | equivalent: `2^h <= d < 2^(h+1)` makes both `d - 2^h` |
| `CR 32`, `COS 217`, `COS 218` (query-count assert removed, `<=` → `!=`, `<=` → `<`) | 42 | untestable (user decision 2026-11-09): only `INT_MAX` or more queries, 32 GiB of input, tell them apart, beyond every memory cap; the assert stays |
| `COS 227` `u < g.n` → `u != g.n` | 46 | equivalent: `u` counts up from 0 to `g.n >= 0` |
| `COS 237` `j < head[u + 1]` → `!=` | 50 | equivalent: `head` is nondecreasing after the prefix sums, so `j` reaches `head[u + 1]` exactly |

## Benchmarks

None is required (Contest profile, no reduction, no threshold). A scratch timing on 2026-11-04 (not committed; GCC 16.0.1, `-O2`, n = q = 500,000, root 0, uniform `u` and `k <= dep[u]`, medians of three runs, build includes allocation) for orientation only:

| Tree | `LCA` build / queries (ms) | `LevelAncestor` build / queries (ms) | `levelAncestorOffline` (ms) |
|---|---|---|---|
| Random recursive | 244 / 28 | 190 / 11 | 175 |
| Path | 57 / 279 | 21 / 4 | 25 |
| Caterpillar (parent among the last three) | 118 / 254 | 47 / 12 | 31 |

## Sources

[00-sources.md](00-sources.md#pages-fetched-on-2026-11-04-p042-gr70): the ladder and jump-pointer method (OI Wiki, Wikipedia), suisen's depth-bucket form and the path-jump catalogs (not adopted). The header was written independently; no catalog code was copied.

## Limits and handoffs

- Depths are `int`; `n <= INT_MAX / 2` keeps `ladder` indexable and `jumps.size() <= INT_MAX` keeps `jump_start` exact (a caterpillar passes it near n = 1.7e8, about 9 GB of tables). Neither is asserted.
- Under `NDEBUG` an out-of-range `u` reads (`query`) or writes (`levelAncestorOffline`) out of bounds, as row 07's `LCA` and `offlineLCA` do; no guard branch is added to the O(1) query (user decision 2026-11-09).
- `query` and `levelAncestorOffline` keep their `k < 0` checks so that a negative `k` stays bounded under `NDEBUG` (`03-cpp.md`, user decision 2026-11-09); it is outside the domain, so no test reaches them.
- The O(n)-preprocessing macro-micro variant is planned row 94 (package P277); online leaf addition stays out ([00-notes.md](00-notes.md#p042-gr70-omissions)).

## History

- 2026-11-04: P042 GR70, new header; review: no correctness defect, 8 findings fixed (jumps store ladder positions, stale counts, integration row, mutation wording, row `build`, duplicates keeps, wording); quick, full on g++ and g++-14, stress (seed 2), integration and mutation passed.
- 2026-11-08: `/merge-pr 4` rebase on master: row set to `partial`, since the P254 omission sweep of 2026-11-08 added operations after this work (`missing:` in the inventory row); no code or test change.
- 2026-11-08: `/merge-pr 4` review fixes: `G = Graph` defaults, `levelAncestorOffline` name restored, out-of-domain `NDEBUG` tests removed.
