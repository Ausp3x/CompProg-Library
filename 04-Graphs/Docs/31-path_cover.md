# 31-path_cover.hpp — evidence

Owned by package P047 / GR28; see [00-notes.md](00-notes.md#p047-package-record). Contest profile: Hopcroft–Karp from row 21 for vertex-disjoint path covers, a word-bitset closure with bitset Hopcroft–Karp for Dilworth chain covers, and a min cut (row 19 Dinic) on the split DAG for weighted antichains. The row was planned; there is no legacy code.

## Contracts

All three functions take a directed `Graph` or `CsrGraph`. They assert that it is acyclic with a Kahn pass, O(n + m); under `NDEBUG` a cyclic input gives a meaningless but bounded result. Parallel arcs are allowed; loops make the graph cyclic.

### dagMinimumPathCover

Returns a minimum set of vertex-disjoint paths along the original arcs that covers every vertex, as vertex lists ordered by first vertex. The minimum is `n` minus a maximum matching of the split bipartite graph (`u` left to `v` right per arc). Paths start at vertices whose right copy is unmatched and follow matched successors. Time O(n + m·√n) with `BipartiteMatching::hopcroftKarp`, memory O(n + m).

### ChainCoverResult and dagMinimumChainCover

`chains` partition the vertices, and consecutive vertices of a chain are related by reachability (each is reachable from the one before). `antichain` (ascending) is pairwise unreachable, and `|antichain| = chains.size()` equals the width (Dilworth), so the pair certifies optimality. This field is the row's `maximumAntichain`.

`dagMinimumChainCover(g)` first computes strict reachability rows in reverse topological order, O(m·n/w). It then runs Hopcroft–Karp on `u → v` for every `v` reachable from `u`, entirely on bitsets:
- **BFS phase.** Layers grow from the free left vertices, scanning each row against the unvisited right vertices, and stop after the first layer that reaches a free right vertex.
- **Masks.** Each layer `d` gets a mask of its unused right vertices; only the last layer keeps free ones.
- **Augmentation.** Augmenting DFSs use a monotone word pointer per left vertex and clear each right vertex from its mask when it is tried.

A phase costs O(n²/w + L·n/w) for `L ≤ n` layers, and there are O(√n) phases, so the matching costs O(n²·√n/w). Memory is O(n²/w): the closure plus at most one mask per layer.

The final BFS, which finds no augmenting path, yields König's set `Z`. The antichain is `{v : left copy in Z, right copy not in Z}`: it has `n − |M|` vertices, and they are pairwise incomparable because `Z` is closed under every edge leaving a left vertex in `Z`. Chains follow matched successors from unmatched right copies.

### WeightedAntichainResult and maximumWeightedAntichain

`maximumWeightedAntichain(g, w)` requires `w.size() = n` and `w[v] ≥ 0` (asserted). It returns an antichain of maximum total weight (`weight` is exact `lll`, `antichain` ascending), without a transitive closure.

The network has out-copies `v`, in-copies `n + v`, a source and a sink, with these arcs (∞ is `W + 1`, where `W = Σw`):
- source → out_v with capacity `w[v]`;
- in_v → sink with capacity `w[v]`;
- in_v → out_v with capacity ∞;
- out_u → in_v for every arc, with capacity ∞.

A finite cut `X` satisfies `in_v ∈ X ⇒ out_v ∈ X` and `out_u ∈ X ⇒ in_v ∈ X`. Its capacity is `W − w(A)` for `A = {v : out_v ∈ X, in_v ∉ X}`, which is an antichain because reachability propagates through the ∞ arcs. Conversely, every antichain `A` arises from the cut "at or above `A`". So the minimum cut gives the maximum-weight antichain, read from `MaxFlow::minCut`. With unit weights this is the width, the same value as `dagMinimumChainCover` by an independent algorithm. Dinic on `2n + 2` nodes and `m + 3n` arcs costs O(n²·(n + m)) time and O(n + m) memory.

## Legacy mapping

No legacy code. The row's `maximumAntichain` is the certificate field `dagMinimumChainCover(g).antichain`. A separate function would recompute the same matching and return a projected field (`03-cpp.md`, One API per operation).

## Aliases

- OI Wiki and LOJ 6002 minimum path cover (`n − matching`) is `dagMinimumPathCover`; OI Wiki minimum intersecting path cover is `dagMinimumChainCover`.
- NetworkX `antichain_width` and maspypy `maximum_antichain` (on a closed relation) are `antichain.size()` and `antichain`.
- maspypy `maximum_weighted_antichain` (min cut on a closed relation) is `maximumWeightedAntichain` on the original arcs.

## Feature-to-test map

`96-Local Testing/04-Graphs/31-path_cover_tester.py` (shared graph runner with `-Werror` warnings; optimized, checked with probes, ASan/UBSan in full/stress). Oracles:
- path covers: backtracking over successor choices (distinct successors along arcs);
- width and weighted antichains: subset enumeration over a BFS closure;
- large instances: self-checking Dilworth certificates by BFS and a tester-side Kuhn matching.

| Operation | Test | Oracle |
|---|---|---|
| `dagMinimumPathCover` (Graph and CSR) | Every labelled DAG n ≤ 4; 3,000 random DAG multigraphs n ≤ 10; random, dense (30%), grid and two-chain DAGs n = 2,000; 200,000-vertex random DAG | Backtracking optimum; Kuhn matching; partition and arc checks |
| `dagMinimumChainCover`, `ChainCoverResult` (`chains`, `antichain`) | Same small cases; the four 2,000-vertex shapes | Width by enumeration; reachability of chain steps and incomparability by BFS; certificate sizes equal |
| `maximumWeightedAntichain`, `WeightedAntichainResult` | Same small cases with weights in {0..9}, zeros and values up to 2^62; unit weights on the 2,000-vertex shapes; edgeless graph with 2,000 weights `LLONG_MAX` | Enumeration; equality with the bitset chain count; exact `lll` total |
| Preconditions | 6 probes: undirected graph, a cyclic graph for each function, weight size mismatch, negative weight | Expected assertion failure |

Planted mutants (quick suite, optimized build): a closure missing direct successors, a cut read without the in-copy condition, and an antichain read without the right-copy condition were killed. Removing the BFS stop after the first free layer survived. It keeps every augmenting path valid and only loosens the shortest-path phase bound, so it changes complexity, not results.

## Commands and results

2026-10-18, GCC 16.2.1 and GCC 14.4.1 (floor), CPython 3.14.8, Linux x86-64 (Intel Core i9-11900H), GNU++20, `-Wall -Wextra -Wconversion -Werror` in the runner's optimized, checked and ASan/UBSan builds.

| Command | Result |
|---|---|
| `31-path_cover_tester.py --mode quick` | PASS 2 configurations, 330 cases, 30,712 checks per configuration, 6 probes |
| `31-path_cover_tester.py --mode full --seed 1` | PASS 3 configurations, 3,573 cases, 584,347 checks per configuration, 6 probes; MEMORY peak 535 MB |
| `CXX=g++-14 31-path_cover_tester.py --mode full --seed 2` | PASS 3 configurations, 3,573 cases, 588,334 checks per configuration, 6 probes; MEMORY peak 565 MB |
| `31-path_cover_tester.py --mode stress --seed 3` | PASS 3 configurations, 15,573 cases, 1,664,913 checks per configuration, 6 probes; MEMORY peak 1,070 MB |
| `02-integration.py --sanitizers` | PASS: 159 standalone/aggregate headers (header alone, `99-all`), multi-TU, workspace, sanitizer self-tests; MEMORY peak 1,829 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/04-Graphs/31-path_cover_tester.py' --mode full --seed 1
CXX=g++-14 python3 '96-Local Testing/04-Graphs/31-path_cover_tester.py' --mode full --seed 2
python3 '96-Local Testing/04-Graphs/31-path_cover_tester.py' --mode stress --seed 3
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Benchmarks

None required: contest profile, no threshold.

## Sources

| Source | Inspected claim and application |
|---|---|
| [OI Wiki, 偏序理论](https://oi-wiki.org/math/order-theory/) and [二分图最大匹配](https://oi-wiki.org/graph/graph-matching/bigraph-match/) | Dilworth and Mirsky; path cover = n − matching |
| [maspypy dag_path_cover](https://maspypy.github.io/library/graph/dag_path_cover.hpp), [maximum_antichain](https://maspypy.github.io/library/graph/maximum_antichain.hpp), [maximum_weighted_antichain](https://maspypy.github.io/library/graph/maximum_weighted_antichain.hpp) | Catalog forms (see Aliases); split-graph min cut |
| [NetworkX DAG algorithms](https://networkx.org/documentation/stable/reference/algorithms/dag.html) | `antichains`, `antichain_width` |

The bitset Hopcroft–Karp layering and the split-DAG cut argument are this package's derivations, validated by the proofs above and the tests.

## Limits and handoffs

- The chain cover needs O(n²/w) memory (about 12.5 MB at n = 10,000). `maximumWeightedAntichain` with unit weights gives the width on larger sparse DAGs without chains.
- Digraphs with cycles: condense with row 08 first ([00-notes.md](00-notes.md#p047-omissions)); bipartite matching and flows are rows 21 and 19.

## History

- 2026-10-18: verified (P047); new row; `maximumAntichain` returned as the chain cover's certificate; independent review: no correctness findings, arc checks added to the 200,000-vertex path cover; 3 planted mutants killed, 1 complexity-only mutant survived; quick, full and stress passed on g++, full on g++-14.
