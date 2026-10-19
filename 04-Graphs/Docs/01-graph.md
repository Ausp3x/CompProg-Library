# 01-graph.hpp — evidence

Owned by package P010 / GR01 (graph foundations, with `02-traversal.hpp` through `06-mst.hpp`); see the [common graph contract](00-notes.md#p010-common-contract).

## Contracts

### GraphEdge and GraphArc

`GraphEdge {u, v, w}` is a logical edge in insertion order. `GraphArc {from, to, id, rev, w}` is an oriented arc: `id` is its logical edge, `rev` the reciprocal arc of an undirected edge and `-1` for a directed arc. An undirected edge, including a loop, has two distinct reciprocal arcs `a` and `a + 1`.

### Graph

`Graph(n, directed)` with `n >= 0`. `addEdge(u, v, w = 1)` returns the new logical edge ID; it invalidates references and iterators into the storage, and IDs are stable until assignment or destruction. `operator[](u)` returns the arc indices out of `u` in insertion order. `reverse()` is O(n + m): directed arcs swap endpoints, undirected edges keep their orientation, and logical edge IDs are preserved. `read(in, n, m, directed, weighted, base)` consumes exactly `m` records `u v [w]` with `base` 0 or 1; omitted weights become 1. Well-formed input, including successful extraction and in-range endpoints, is a precondition.

`fromAdjacency(adj, directed = false)` builds `adj.size()` vertices from `vector<vector<int>>` lists (weight 1) or `vector<vector<pair<int, lng>>>` lists; every entry must be in `[0, n)`. Directed: one arc `u -> v` per entry `v` of `adj[u]`, in vertex then list order. Undirected: the lists must be symmetric as multisets of (neighbour, weight), so every unordered pair `{u, v}` has the same weight multiset in `adj[u]` and `adj[v]` and every loop is listed twice in its own list with equal weights. Each mirrored pair becomes one edge, so parallel edges keep their multiplicity; the edges come in increasing `(u, v, w)` order with `u <= v`, independent of list order. An asymmetric input is an asserted precondition. `fromMatrix(w, directed = true)` takes a square `vector<vector<lng>>` and adds one arc `u -> v` of weight `w[u][v]` per nonzero entry in row-major order; undirected requires a symmetric matrix and adds one edge per nonzero entry with `u <= v`, a nonzero diagonal entry being a loop. A zero entry means no edge, so zero-weight edges need `addEdge`. Costs: O(n + m) with m the total list length, and O(n^2).

Correctness: `Graph` owns zero-based vertices, insertion-ordered logical edges and oriented arcs. All counts fit `int`; weights use the full `lng` domain. A directed graph's reverse is a separate object. Algorithms return arc indices so orientation and parallel edges remain distinguishable; `g.arcs[a].id` recovers the logical ID. Public storage is inspectable but must not be edited directly.

Correctness of `fromAdjacency`: undirected entries become records `(u, v, w)` of the listing vertex `u`. Least-significant-digit radix sort by weight (eight 8-bit passes on the sign-flipped value, each skipped when its byte is constant), then stable counting sorts by `max(u, v)` and by `min(u, v)`, order them by `(min(u, v), max(u, v), w)` in O(n + m) with word-size constants. A group of equal keys holds exactly the entries of the edges with that key: for `u < v` the forward entries (listed at `u`) and the backward ones must be equally many, and a loop group, all forward, must be even. Symmetry as multisets is exactly this group-wise condition, so the assertion accepts every valid input and rejects every other, and a valid group yields one edge per mirrored pair (`forward` edges, `forward / 2` for a loop). Under `NDEBUG` an asymmetric input with in-range entries still terminates, with at most one edge per forward entry; an out-of-range entry is undefined there, as in `addEdge`. Directed lists and matrices are direct scans; the unweighted overload skips the weight passes. `fromMatrix` checks row sizes and symmetry in one entry pass, so with assertions enabled an undirected matrix costs one extra O(n^2) pass with strided reads.

### CsrGraph

An immutable owning snapshot of a `Graph` with contiguous adjacency, keeping arc IDs, edge IDs and adjacency order. `operator[]` returns a `std::span` that borrows this object and expires on its assignment or destruction. Construction workspace is O(1) beyond the result; `reverse()` is O(n + m) including a transient adjacency-list graph.

### DenseGraph

Owns a copy of the full multigraph in `graph` plus `best[u][v]`: the arc index of the lightest `u -> v` arc, or the heaviest when constructed with `maximum = true`, `-1` when there is none. Equal weights choose the earliest arc. There are no implicit diagonal entries; loops are stored like any other arc. `dijkstraDense` requires the minimum view; `primDense` builds a minimum or maximum forest according to `maximum`.

Correctness: the O(n²) matrix selects the best arc per ordered pair with earliest-arc ties and `-1` for absence, while the full graph is kept, so a dense projection does not lose parallel-edge provenance.

### Subgraph, lineGraph, inducedSubgraph, contract and simplify

`Subgraph {graph, edge}` maps each new edge `i` back to the source edge `edge[i]`. All four utilities accept `Graph` or `CsrGraph` and return a new owning `Graph` with the same directedness.

- `lineGraph(g)` has one vertex per source edge, every edge of weight 1. Directed: an arc `i -> j` for every pair of arcs with `head(i) == tail(j)`, so a directed loop gives a line-graph loop. Undirected: for every vertex `v` and every unordered pair of distinct edges incident to `v` (a loop counts once), one edge; parallel edges therefore give two parallel line edges, and `simplify(lineGraph(g))` is the simple line graph. Cost O(n + m + out) with `out` the line-graph edge count; `addEdge` asserts that it fits `int`.
- `inducedSubgraph(g, vs)` takes distinct in-range vertices (asserted). New vertex `i` is `vs[i]`, so the caller's list is the vertex map. Kept edges are those with both ends in `vs`, with orientation and weight preserved, ordered by the position in `vs` of the scanning endpoint (`u` for undirected edges) and then by adjacency order. Cost O(n + k + d) with `d` the degree sum of `vs`; the O(n) term is the position array.
- `contract(g, label, k)` requires `label.size() == n`, `k >= 0` and every label in `[0, k)`. Edge `i` of the result is `(label[u], label[v], w)` of source edge `i`, so edge IDs are identical and loops and parallel edges are kept. `simplify(contract(...))` gives the quotient graph, for example a bridge tree from 2-edge-connected labels or a condensation from SCC labels. Cost O(n + k + m), since the label checks are O(n).
- `simplify(g)` drops loops and keeps, per ordered pair (directed) or unordered pair (undirected), the edge with the smallest `(w, id)`. Kept edges keep their original orientation and appear in increasing source ID order. Cost O(n + m).

Correctness: these are direct constructions. `lineGraph` enumerates, per vertex, the pairs of incident edges (undirected) or the arcs leaving each edge's head (directed), so it emits each line-graph edge exactly once in O(n + m + out). `inducedSubgraph` scans only the chosen vertices' adjacency and keeps an undirected edge from its first arc, which has the smaller index, so each edge is taken once with its stored orientation. `contract` relabels endpoints edge by edge. `simplify` scans each vertex's adjacency with a stamp array keyed by the scanning vertex, keeps the smallest `(w, id)` per neighbour (an undirected pair only from its smaller endpoint), then emits the kept IDs in increasing order; every arc is touched twice, so it is O(n + m) without sorting.

### Legacy mapping

- `GraphLabels<T>(labels)` → `CoordinateCompression<T>(labels)` (`02-Data Structures/07-ordered_set.hpp`, under the same strict ordering): `index(x)` returns -1 for an absent label where `GraphLabels` asserted, `value(i)` is `decode(i)` and `labels` is `v`.

## Feature-to-test map

`96-Local Testing/04-Graphs/01-graph_tester.py` uses the shared graph driver, which builds optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and full/stress `-O1` ASan/UBSan variants. The corpus is 606 exhaustive small graphs, 500 random multigraphs and 1,000 builder cases; quick uses exhaustive graphs through two vertices, 70 random cases and 100 builder cases, full extends to three vertices, 500 random and 1,000 builder cases, stress to 4,000 random and 8,000 builder cases. Exact mode coverage is also stated in the runnable entry.

| Operation | Test | Oracle |
|---|---|---|
| `GraphEdge`, `GraphArc`, `Graph: addEdge, operator[]` | Edge/arc counts and identities, defaults, full-range weights | Independent edge-list records |
| `Graph: reverse`, `CsrGraph: reverse` | Reverse involution | Involution and edge-ID identity |
| `CsrGraph: operator[]` | CSR equivalence; snapshots/copy/move | Adjacency of the source `Graph` |
| `DenseGraph: best` (minimum or maximum view) | Dense minima and maxima with earliest ties | Independent dense minima and maxima |
| `Graph: read` (base/weighted/directed adapters) | Input adapters | Expected edge lists |
| `Graph: fromAdjacency, fromMatrix` | Every exhaustive and random graph as shuffled weighted and unit lists; builder cases with full-range weights differing in one random byte, loops and parallel edges, and random directed or symmetric matrices; fixtures for defaults, mirrored parallel edges listed in different orders, loops and empty inputs | Directed: the list scan. Undirected: the source edge multiset normalized to `(min, max, w)` and sorted with `std::sort`. Matrices: the row-major nonzero scan, and identity with `fromAdjacency` of the same lists |
| Legacy `GraphLabels` cases (DS row 07 `CoordinateCompression`) | Label adapters: integer extremes, strings with the empty label, keys with only `operator<`, empty set, copy/move, absent labels | Sorted distinct labels; -1 for an absent label |
| `lineGraph`, `Subgraph` | Each graph and its CSR snapshot | Brute-force incidence counting |
| `inducedSubgraph` | Empty, full and three shuffled random subsets | Filtered edge list with relabelled endpoints |
| `contract` | Identity, all-merged and random labels (two spare labels) | Relabelled edges |
| `simplify` | Every graph and every contraction | Ordered pair map of `(w, id)` minima |
| Preconditions | 32 assertion probes, including `induced-negative`, `induced-end`, `induced-duplicate`, `contract-size`, `contract-count`, `contract-negative`, `contract-end`, `adjacency-negative`, `adjacency-end`, `adjacency-asymmetric`, `adjacency-multiplicity`, `adjacency-loop-once`, `adjacency-weight`, `adjacency-loop-weight`, `matrix-ragged`, `matrix-asymmetric` | Expected assertion failure |

Mutation checks (planted in a temporary copy, full mode) killed: line-graph loop double count and self-pairs, simplify keeping the maximum, reversed induced orientation and maximum-view tie order. A surviving mutant (simplify tie on an equal ID) is equivalent.

Builder mutants (2026-10-12, same method; the two assertion mutants against the checked probes) were all killed except one equivalent: no sign flip, seven weight passes, an unstable counting scatter, grouping without the weight, loops not halved, full matrix rows, and dropped list and matrix symmetry assertions. The survivor removes the directed early return, which only skips sorting an empty buffer.

## Commands and results

2026-10-12, Linux x86-64, GCC 16.2.1 and GCC 14.4.1 20260915 (`CXX=g++-14`), CPython 3.14, GNU++20. Every P010 build adds `-Wall -Wextra -Wconversion -Werror` (opt-in `strict=True` in the shared runner).

| Run | Result |
|---|---|
| Full, seed 20261012, g++ | PASS, all three builds, 22,721,198 checks per build, 32 probes, peak 619 MB |
| Full, seed 20261012, `CXX=g++-14` | PASS, all three builds, same counts, peak 595 MB |
| Stress, seed 7 | PASS, all three builds, 167,097,978 checks per build, peak 642 MB |
| Quick runner, all sixteen graph entries, from `/tmp` | PASS |
| `02-integration.py` (136 standalone/aggregate headers, scalar and AVX2 multi-TU linkage, workspace) | PASS, peak 647 MB |
| `03-consistency.py`, with `--braces` on the touched headers, testers and benchmark | No errors |

```bash
python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode full --seed 20261012
CXX=g++-14 python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode full --seed 20261012
python3 '96-Local Testing/04-Graphs/01-graph_tester.py' --mode stress --seed 7
python3 '/home/Ausp3x/Documents/CompProg Library/96-Local Testing/01-run.py' --mode quick --filter 04-Graphs --seed 42 --no-integration   # from /tmp
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
```

`--configuration optimized|checked|ASan-UBSan` selects one build for a targeted retry.

## Benchmarks

The [benchmark driver](<../../96-Local Testing/04-Graphs/00-foundations_benchmark.py>) and `96-Local Testing/04-Graphs/00-foundations_benchmark.json` compare the same BFS kernel on adjacency-list and CSR storage. It uses chain, star and connected random multigraphs at 32, 3,000 and 100,000 vertices, seed 20260927, one checked warmup and five samples with repetition for smaller graphs. CSR conversion is timed separately; input graph construction is excluded. All traversal checksums agree. Both representations store O(n+m), and conversion retains the input plus the output.

| 100,000-vertex random graph (499,999 logical edges) | Median |
|---|---|
| BFS on adjacency lists | 14.34 ms |
| BFS on CSR | 9.27 ms |
| CSR construction | 4.48 ms |

Tiny cases slightly favored lists; chain/star results also varied. This supports an explicit reusable CSR option, not an automatic threshold or universal speed claim. Conditions from the git-ignored `00-foundations_benchmark.json` log, which also holds the raw samples: 11th Gen Intel Core i9-11900H, GCC 16.2.1 20260810, `-std=gnu++20 -O2 -DNDEBUG -mno-avx2`. Measured on a shared host.

```bash
python3 '96-Local Testing/04-Graphs/00-foundations_benchmark.py'
```

## Sources

Retrieved and inspected on 2026-09-27; no source code was copied.

| Source | Inspected claims and use |
|---|---|
| [OI Wiki, 图的存储](https://oi-wiki.org/graph/save/) (page revision 2026-09-12) | Matrix/list/forward-star tradeoffs and paired reverse arcs; used to assess storage coverage. |
| [Boost 1.89, Compressed Sparse Row Graph](https://www.boost.org/doc/libs/1_89_0/libs/graph/doc/compressed_sparse_row.html) | Source-group offsets, O(n+m) storage and immutable CSR use. |

The completeness sweep sources are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p010-re-audit); omitted candidates are in [00-notes.md](00-notes.md#p010-re-audit-omissions).

## Limits and handoffs

Dynamic graphs are owned by their separate inventory rows.

## History

- 2026-09-27: original P010, full suite (seed 20260927) passed on g++ 16 with integration and 61 package probes; stress not run.
- 2026-10-06: inventory rewrite reopened the row as partial (line graph, induced subgraph, contraction, simplification).
- 2026-10-07: re-audit, implemented `lineGraph`, `inducedSubgraph`, `contract`, `simplify`; 5 findings fixed (missing operations, evidence claim, complexity lines, brace rule, T/M placement); full suite passed on g++ and g++-14, stress passed; independent review: findings 1–3, 5 and 7–14 confirmed fixed, no correctness defect, 22 own mutants (17 killed, 5 survivors equivalent or runtime-only).
- 2026-10-12: prune, added `Graph::fromAdjacency` and `fromMatrix`, removed `GraphLabels` (cases moved to `CoordinateCompression`); full suite passed on g++ and g++-14, stress passed; independent review: no correctness defect.
