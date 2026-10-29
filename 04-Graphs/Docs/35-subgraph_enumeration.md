# 35-subgraph_enumeration.hpp — evidence

Owned by package P048 / GR32; omissions are in [00-notes.md](00-notes.md#p048-gr32-omissions). Contest profile: degeneracy-ordered enumeration and counting of triangles, 4-cycles, 4-cliques and short paths, the clustering coefficients built on them, and the directed triad census. `countTriangles` and `countK4` use `std::popcount` over 64-bit words; there is no handwritten SIMD and no Barrett or Montgomery reduction, so the header has no `Compact` twins. No legacy code covers this row.

## Contracts

### Common domain

- `G` is `Graph` or `CsrGraph` (only `n`, `directed`, `edges` and `arcs` are read). `triadicCensus`, `directedClustering` and `weightedDirectedClustering` assert a directed graph, `averageClustering` takes either, and every other operation asserts an undirected one. Loops are ignored everywhere. `n = 0`, isolated vertices and disconnected graphs are allowed.
- Two semantics, fixed per operation:
  - **Simple graph** (every operation not listed below): parallel edges collapse, so a subgraph is a set of vertices with the required adjacencies. A triangle is 3 pairwise adjacent vertices, a 4-cycle is 4 distinct vertices with a cyclic order of 4 adjacencies (K4 contains 3), and a 4-clique is 4 pairwise adjacent vertices.
  - **Multigraph** (`enumerateTriangleEdges`, `countTrianglesPerEdge`, `countC4PerEdge`): a subgraph is a set of edges of g. Parallel copies are distinct, so a triangle through edge `e = {u, v}` is a choice of one edge for each of the pairs `{v, w}` and `{w, u}`. Each copy of a pair gets the same count, and loops get 0. On simple graphs both semantics agree.
- Counts are exact in `lng`. An undirected `m` is at most 2^30 because `arcs.size() <= INT_MAX`. `Ranked` lists a directed arc at both ends, so it asserts `arcs.size() <= INT_MAX / 2` for directed graphs. That bound, about 32 GB of arcs, is out of reach in practice.
  - Triangles: at most `C(m, 2)`. Per-edge triangles: at most `C(m − 1, 2)`.
  - 4-cycles: at most `C(m, 2)`. Each 4-cycle has 2 pairs of opposite edges, and each pair of disjoint edges lies in at most 2 four-cycles.
  - Per-edge 4-cycles: for an edge `uv`, the count is `Aᵀ B C`, where `A`, `B`, `C` hold the multiplicities of the pairs at `v`, between the middles, and at `u`. Their entries sum to at most `m − 1`, so the count is at most `(ΣA)(ΣB)(ΣC) <= ((m − 1) / 3)^3`. That is below 2^63 for `m <= 6 * 10^6`. On a simple graph every per-edge count is at most `deg(u) deg(v) <= 2^60`. `countC4PerEdge` accumulates in `ulng`, so it is exact on both domains, and beyond them each count is its value mod 2^64 without undefined behaviour (user decision 2026-10-29).
  - Paths: at most `4 m^2` (below).
- Every function is pure: it builds its own workspace from g and keeps no state between calls. Callbacks run during the call and must not modify g.

### Degeneracy order (`subgraph_enumeration_detail::Ranked`)

`Ranked(g)` builds the underlying simple graph in O(n + m) time and memory:
1. A raw adjacency of the non-loop arcs. A directed arc is listed at both ends, with direction bits relative to the list owner.
2. Duplicates are removed with a stamp array. Each simple pair gets a `pid`, and `pair_of[e]` maps every edge to its pair. `mult[pid]` is the pair's multiplicity, and `dir` ORs the directions of its arcs.
3. Vertices are relabelled by the Batagelj–Zaversnik core order (bucket queue, O(n + m)). Its `deg` ends as the core number, which is an upper bound on the number of neighbours of v that come later. So `out(v)`, the later neighbours, has at most `d` entries, where `d` is the degeneracy. A d-core needs at least `d + 1` vertices of degree at least `d`, so `d (d + 1) / 2 <= m`, that is `d <= sqrt(2m)`.
4. Each list is filled in increasing rank, so it is sorted. `out(u)` is the suffix `[split[u], start[u + 1])`.

Only `Ranked(g, true)` builds the payload: arc, pair ID, multiplicity and direction per entry, and `pair_of` per edge. The multigraph operations, `findC4`, `generalizedDegree`, the directed and weighted clusterings and `triadicCensus` use it; entry `p` of list `x` stores an arc from `to[p]` into `x`. The other operations skip it: `Ranked(g, true)` takes 1.6–4.7 times as long as `Ranked(g)` (Benchmarks). The wedge scans of `countC4`, `countC4PerEdge`, `countC3C4PerVertex`, `findC4` and `enumerateC4` share `subgraph_enumeration_detail::wedges`. The 4-clique rows of `countK4` and `enumerateK4` share `subgraph_enumeration_detail::k4`.

Every bound below in `m * d` is therefore at most `m * sqrt(2m)`. It is linear on graphs of bounded degeneracy (planar graphs, forests, grids).

### enumerateTriangles

`enumerateTriangles(g, f)` calls `f(a, b, c)` once per triangle with distinct original vertices in unspecified order. Time O(n + m · d + out), memory O(n + m).

A triangle with ranks `u < v < w` is reported only from `u`: `v ∈ out(u)`, and `w ∈ out(v)` is found by a stamp. The stamp stores the position of `w` in `u`'s list, and it is valid exactly when that position is at least `split[u]`, because earlier lists occupy smaller positions. The work is `Σ_{u→v} |out(v)| <= m · d`.

### enumerateTriangleEdges

`enumerateTriangleEdges(g, f)` calls `f(a, b, c, x, y, z)` once per triangle of edges, with edge IDs `x = {a, b}`, `y = {b, c}` and `z = {c, a}`. Every combination of parallel copies is a separate triangle (multigraph semantics; user decision 2026-10-29, beside `enumerateTriangles` instead of one arbitrary representative per pair). Each vertex triangle runs the product of its three pair lists, built by counting sort over `pair_of`. Time O(n + m · d + out), memory O(n + m).

### countTriangles

`countTriangles(g)` is the simple-graph count, kept beside the enumeration and the per-vertex and per-edge sums for its dense kernel and single pass (user decisions 2026-10-29). `subgraph_enumeration_detail::triangleCount` counts the triangles whose lowest rank is below `r0` with the sparse loop and the rest with `bitsetTriangles(h, r0)`, which works on the rank suffix `[r0, n)`. That split is exact, because a triangle whose lowest rank is at least `r0` lies inside the suffix.

`denseStart(h)` picks `r0` in O(n + m). It minimizes `probes(u < r0) + DENSE_RATIO · (Σ_{u ≥ r0, u→v} ((n − v) / 64 + 1) + k · ceil(k / 64))` with `k = n − r0`, among the suffixes with `k · ceil(k / 64) <= DENSE_WORDS`. It returns `n` when no suffix beats the all-sparse cost. The degeneracy order puts the dense cores last, so the suffix holds them. Each kernel sets one row of `out(u)` bits per suffix vertex and adds `popcount(row[u] & row[v])` for every arc `u → v`, from the word of `v` onward. Every common later neighbour is counted once.

The rule picks the bitset only when the model says it is cheaper than the sparse loop, so the time stays O(n + m · d). Memory is O(n + m) plus the rows, at most 32 MB. Threshold measurements: [Benchmarks](#benchmarks).

### triangleProductSum

`triangleProductSum(g, x)` returns `Σ x[a] · x[b] · x[c]` over the triangles `{a, b, c}` (Library Checker `enumerate_triangles`; renamed from `triangleAggregate` and kept by the user on 2026-10-29). `T` needs `T{}` equal to zero, `+=` and `*`. It works with wrapping `ulng`, modular types and `lng`; for a signed `T`, every product and partial sum must fit, otherwise the overflow is undefined behaviour. It asserts `x.size() == n`. `x` is copied into rank order first for locality. Under `NDEBUG`, a short `x` reads only its existing entries and treats the rest as zero, so a violated precondition stays bounded. Time O(n + m · d), memory O(n + m).

### countTrianglesPerEdge

`countTrianglesPerEdge(g)[e]` uses multigraph semantics. Each simple triangle with pair multiplicities `μ_uv, μ_vw, μ_uw` adds `μ_vw · μ_uw` to the pair `uv`, and similarly for the other two pairs. Every copy of a pair then reads the pair's total. Time O(n + m · d), memory O(n + m).

### countC4

`countC4(g)` returns the number of simple-graph 4-cycles. For each `u` it counts the wedges `u − v − w` with `v, w` earlier than `u` in `cnt[w]`, and adds `C(cnt[w], 2)` incrementally. Each 4-cycle is counted once: from its latest vertex `u`, with `w` the vertex opposite `u` and the two middles as the pair. The sorted lists stop each scan at `u`. A pair `(v, u)` with `u ∈ out(v)` scans at most `deg(v)` entries, so the work is `Σ_v deg(v) · |out(v)| <= 2 m · d`. It is kept beside `countC4PerEdge` for its single pass (user decision 2026-10-29). Time O(n + m · d), memory O(n + m).

### countC4PerEdge

`countC4PerEdge(g)[e]` uses multigraph semantics and matches Library Checker `counting_c4`. For each `u`, a first pass sets `S[w] = Σ_v μ_uv · μ_vw` over the wedges `u − v − w` above. A second pass adds `μ_vw · (S[w] − μ_uv μ_vw)` to the pair `uv` and `μ_uv · (S[w] − μ_uv μ_vw)` to the pair `vw`. Those are the weighted counts of the other wedges `u − v' − w` with `v' ≠ v` that close a cycle through the edge. Each cycle is reached once from its latest vertex, and every one of its four pairs is credited from the wedge that contains it. Time O(n + m · d), memory O(n + m); exact for simple graphs and `m <= 6 * 10^6`, otherwise mod 2^64 (Common domain).

### countC3C4PerVertex

`countC3C4PerVertex(g)` returns `{c3, c4}`: per vertex, the simple-graph triangles and 4-cycles containing it (maspypy `count_C3_C4_pointwise`). Every triangle adds 1 to its three vertices. In the C4 scan, `u` and `w` get `C(cnt[w], 2)` and each middle `v` gets `cnt[w] − 1`. So `Σ c3 = 3 T` and `Σ c4 = 4 C4`. Time O(n + m · d), memory O(n + m).

### findC4

`findC4(g)` returns a `CycleWitness` (row 02) with `vertices = {a, b, c, d, a}` and four arc IDs of g along it, or both vectors empty when the simple graph has no 4-cycle. It runs the `countC4` scan and stops at the second wedge `u − x − w` that shares its ends with a first wedge `u − v − w`. Before that stop, each `w` is reached at most once per `u`, so the work is at most `min(n^2, m · d)` after the O(n + m) build. Time O(n + m + min(n^2, m · d)), memory O(n + m).

### countK4

`countK4(g)` counts 4-cliques (maspypy `count_K4` structure, with the degeneracy order). For each `u` it builds the subgraph induced on `out(u)` (at most `d` vertices) as bitset rows. Building costs the triangle bound `Σ |out(v)|`. For every induced edge `a < b` it adds `popcount(row[a] & row[b])`, the common later neighbours. Each 4-clique `u < v < w < x` is counted once, at `u`, edge `(v, w)`. The rows cost `Σ k_u · ceil(k_u / 64) <= m + m · d / 64`. The counting costs at most one row scan of `ceil(d / 64)` words per triangle, that is `m · d · ceil(d / 64)`. Time O(n + m · d + m · d^2 / w), memory O(n + m), workspace O(d^2 / w) words.

### enumerateK4

`enumerateK4(g, f)` calls `f(a, b, c, e)` once per 4-clique, in unspecified order. It walks the set bits of the `countK4` row intersections. Time O(n + m · d + m · d^2 / w + out), memory O(n + m).

### enumerateC4

`enumerateC4(g, f)` calls `f(a, b, c, e)` once per 4-cycle, in its cyclic order `a − b − c − e − a`. For each `u` it buckets the wedges `u − v − w` by `w` and reports every pair of middles of a bucket. That is exactly the `countC4` scan, so each cycle appears once. Time O(n + m · d + out), memory O(n + m).

### countPathsP3P4P5

`countPathsP3P4P5(g)[k][v]` is the number of simple paths with `k + 3` vertices that start at `v` (maspypy `count_P3_P4_P5` pointwise). The unordered totals are the sums divided by 2. With `d(v)` the simple degree, `t(v)` and `q(v)` the per-vertex triangle and 4-cycle counts, and `N(v)` the neighbours:
- `P3(v) = Σ_{a∈N(v)} (d(a) − 1)`.
- `P4(v) = Σ_{a∈N(v)} P3(a) − d(v)(d(v) − 1) − 2 t(v)`. `P3(a)` includes the walks that return to `v` (`d(v) − 1` per `a`) and the closed walks `v − a − b − v` (two per triangle).
- `P5(v) = Σ_{a∈N(v)} P4(a) − (d(v) − 1) P3(v) − 2 t(v)(d(v) − 3) − 2 q(v)`. The paths `a − b − c − e` from `a ∈ N(v)` that hit `v` are removed:
  - `v = b`: `P3(v) − d(a) + 1 − t(va)` of them;
  - `v = c`: `t(va) (d(v) − 2)`;
  - `v = e`: the 4-cycles through the edge `va`.

  Summing over `a` uses `Σ_a t(va) = 2 t(v)` and `Σ_a C4(va) = 2 q(v)`.

Bound: `P3(b) <= 2m`, `P4(a) <= d(a) · 2m` and `P5(v) <= Σ_a d(a) · 2m <= 4m^2 <= 2^62`, so every term is exact in `lng`. Time O(n + m · d), memory O(n + m).

### generalizedDegree

`generalizedDegree(g)[v][k]` is the number of neighbours `u` of `v` whose pair `{u, v}` lies in exactly `k` triangles (NetworkX `generalized_degree` on the simple graph). Row `v` has size `1 + max k`, and is empty for an isolated vertex. Triangles are counted per pair ID during the enumeration. Time O(n + m · d), memory O(n + m).

### localClustering, transitivity, squareClustering, averageClustering

These are NetworkX `clustering`, `transitivity`, `square_clustering` and `average_clustering` on the simple graph, as defined in the current `cluster.py` code (user decision 2026-10-29). Each value is the `double` quotient of two exact `lng` counts, so it is correctly rounded while both counts are below 2^53.
- `localClustering(g)[v] = 2 t(v) / (d(v)(d(v) − 1))`, and 0 when `t(v) = 0`.
- `transitivity(g) = 3T / Σ_v C(d(v), 2)`, and 0 without triangles. It reuses the `countTriangles` dispatch.
- `squareClustering(g)[v] = q(v) / ((d(v) − 1) P3(v) − 2 t(v) − q(v))`, and 0 when the denominator is ≤ 0. NetworkX computes `uw_degrees − uw_count − triangles − squares`; here `uw_degrees − uw_count = (d − 1) P3(v)`, `triangles = 2 t(v)` and `squares = q(v)`, the 4-cycles through `v`. The documented formula with `a_v(u, w)` differs from that code, and the code is followed.
- `averageClustering(g, count_zeros = true)` is the mean of `localClustering`, or of `directedClustering` when g is directed. With `count_zeros` false it averages only the nonzero values. It returns 0 when nothing is averaged, where NetworkX divides by zero. A weighted average would need a flag choosing between operations; it is the mean of `weightedClustering`.

Each is O(n + m · d) time and O(n + m) memory, averageClustering included.

### directedClustering

`directedClustering(g)[v] = T(v) / (2 (dt(dt − 1) − 2 db))`, and 0 when `T(v) = 0` (NetworkX unweighted directed clustering). Here `dt` is the in-degree plus the out-degree and `db` is the number of reciprocal neighbours, all on the simple digraph. NetworkX's `T(v)` sums `(a_vj + a_jv)(a_vk + a_kv)(a_jk + a_kj)` over ordered pairs `(j, k)`. That equals `2 Σ` over the triangles at `v` of the product of the three dyad arc counts, which the direction bits give. Time O(n + m · d), memory O(n + m).

### weightedClustering, weightedDirectedClustering

These are NetworkX weighted clustering (Saramäki et al.), undirected and directed.
- Weights are normalized as `ŵ = w / max w`, where `max w` is taken over every edge, loops included, and is 1 without edges. The maximum must be nonzero (asserted). Negative weights are allowed, and their cube roots are negative.
- The real cube root is multiplicative, so NetworkX's `cbrt(ŵ_ij ŵ_jk ŵ_ki)` equals the product of per-pair values `S`. For a directed graph, `S` of a pair is the sum of `cbrt(ŵ)` over its arcs, which reproduces NetworkX's eight direction terms. `subgraph_enumeration_detail::cubeRoots` computes `S` per pair.
- Values: `weightedClustering(g)[v] = 2 Σ_{triangles at v} S S S / (d(d − 1))`, and `weightedDirectedClustering` divides the same sum by `2 (dt(dt − 1) − 2 db)`. Both give 0 when the sum is 0.
- Parallel edges, or two arcs in the same direction, are asserted absent, because NetworkX graphs cannot hold them. Under `NDEBUG` their cube roots add up.
- Rounding differs from NetworkX's order of operations by a few ulps.

Time O(n + m · d), memory O(n + m).

### triadicCensus

`triadicCensus(g)` counts the 16 isomorphism types of 3-vertex induced subdigraphs, in NetworkX order 003, 012, 102, 021D, 021U, 021C, 111D, 111U, 030T, 030C, 201, 120D, 120U, 120C, 210, 300. Loops and parallel arcs are ignored. It asserts `C(n, 3) <= 2^63 − 1`. Seen from a centre, a dyad is out, in or mutual.
- Open wedges: centre `c` with `o`, `i` and `μ` out, in and mutual dyads gives `C(o, 2)` 021D, `C(i, 2)` 021U, `o · i` 021C, `μ · i` 111D (A<->B<-C), `μ · o` 111U and `C(μ, 2)` 201. Every enumerated triangle then removes its three closed wedges.
- One-dyad triads: a pair `{u, v}` with `t(uv)` common neighbours has `n − d(u) − d(v) + t(uv)` third vertices adjacent to neither. That count goes to 012 if the dyad is asymmetric and to 102 if it is mutual; the triangles supply the `t(uv)` terms.
- Triangles are classified by their number of mutual dyads: 0 gives 030C if every vertex has out-degree 1 inside the triad and 030T otherwise; 1 gives 120D, 120U or 120C as the remaining vertex sends 2, 0 or 1 arcs; 2 gives 210; 3 gives 300.
- 003 is `C(n, 3)` minus the rest.

Time O(n + m · d), memory O(n + m). No fetched contest catalog has a triad census; the decomposition above was derived for this header and is checked against the NetworkX code table.

## Aliases

- `enumerateTriangles`: ei1333 `enumerate_triangles`, suisen `enumerate_triangles`, hitonanode `EnumerateTriangles`, OI Wiki 三元环计数, NetworkX `all_triangles`.
- `enumerateTriangleEdges`: tko919 `EnumTriangle` (passes the edge indices).
- `triangleProductSum`: Library Checker `enumerate_triangles`.
- `countTriangles`, `countC4`: maspypy `count_C3_C4`; OI Wiki 四元环计数.
- `countC4PerEdge`: Library Checker `counting_c4`, Koosaga `count_4_cycles`.
- `countC3C4PerVertex`: maspypy `count_C3_C4_pointwise`. `countPathsP3P4P5`: maspypy `count_P3_P4_P5`. `countK4`: maspypy `count_K4`. `findC4`: maspypy `find_C4`.
- `triadicCensus`: NetworkX `triadic_census`.
- `localClustering`, `directedClustering`, `weightedClustering`, `weightedDirectedClustering`: NetworkX `clustering` (unweighted, directed, weighted). `transitivity`, `squareClustering`, `averageClustering`, `generalizedDegree`: NetworkX `transitivity`, `square_clustering`, `average_clustering`, `generalized_degree`.

## Feature-to-test map

`96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.py` (`.cpp` beside it) uses the shared graph runner with three builds: optimized `-O2 -DNDEBUG`, checked (`-O0 -D_GLIBCXX_DEBUG`, with 32 assertion probes) and, outside quick, ASan/UBSan. All builds use `-Wall -Wextra -Wconversion -Werror`. Every operation runs on both `Graph` and `CsrGraph`.

Oracles, each validated before use:
- Brute force: an adjacency-matrix count over vertex triples and over quadruples in their three cyclic orders. Per-edge values are multiplicity products over distinct vertices, and paths come from DFS enumeration.
- Edge-subset check: every 3- and 4-edge subset is tested for being a cycle (k non-loop edges on k vertices, all of degree 2, connected). It validates the brute force on 13 hand graphs worked out on paper and on every tiny random multigraph. The hand graphs are K4, doubled K4, C4, a triangle and a square with a doubled edge, two digons joined by a bridge, K2,3, a bowtie, Petersen, loops only, and n = 0, 1, 2.
- Codegree oracle for large graphs, O(Σ deg² · log): weighted two-paths per vertex give the per-vertex and per-edge counts, K4 comes from common neighbours, and paths are counted by DFS at 201 sampled starts. It is cross-checked against the brute force on every medium graph.
- Closed forms: K_n, K_a,b, C_n, P_n, stars (also with doubled spokes and loops), grids.
- NetworkX `TRICODES`, `TRIAD_NAMES` and the `_tricode` bit order, taken verbatim from https://raw.githubusercontent.com/networkx/networkx/main/networkx/algorithms/triads.py (fetched 2026-10-29). The 64 labelled triads match a port of NetworkX's `triad_type` rules, with orbit sizes {1, 6, 3, 3, 3, 6, 6, 6, 6, 2, 3, 3, 3, 6, 6, 1}. The 16 hand shapes match in all 6 labellings, and the docstring example gives 030T = 2 and 030C = 2.
- NetworkX `cluster.py` (main, fetched 2026-10-29) transliterated on the simple graph: `clustering`, `transitivity`, `square_clustering`, `generalized_degree`, `average_clustering` and the iterators `_directed_triangles_and_degree_iter`, `_weighted_triangles_and_degree_iter` and `_directed_weighted_triangles_and_degree_iter`. Unweighted values compare exactly, since both sides divide the same integers. Weighted values compare within `1e-10` times the same sum over absolute terms.

| Operation | Test | Oracle |
|---|---|---|
| `enumerateTriangles` | Every section; relabelling and parallel copies | Exact sorted triple list (small); count, validity, no repeats (large) |
| `countTriangles` | Every section; triangle kernels: K300, K1000, K2000; random n = 300 / 1000 / 3000 at density 0.5–0.9 with 1% parallel edges and loops; a density sweep 0.005–0.4. Dispatch: `detail::denseStart` called from the header, `rows(n − r0) <= DENSE_WORDS`, and the sparse loop below `r0` plus `bitsetTriangles(h, r0)` equal to the count. `bitsetTriangles(h, r)` is checked at 8 values of `r` against the suffix counts. The cores K200 + 5000 and K600 / K640 + 13384, 13385, 15744 and 15745 vertices, and K600 + 100000, are attached as isolated, paired or random vertices, plus K3000 + 100000 isolated in the optimized full and stress runs; these require `r0 < n`, and `r0 > 0` once the whole graph exceeds `DENSE_WORDS`. All-sparse, whole-graph and proper-suffix outcomes must all occur: 3 / 12 / 19 in the optimized full run | Public call, `detail::bitsetTriangles` and the sparse `detail::triangles` against each other, against a vertex-indexed bitset counter and against C(k, 3) or a marking oracle |
| `enumerateTriangleEdges` | Every section | Exact list of edge-ID triples (vertex triangles times parallel copies), itself checked against 3-edge subsets on every tiny multigraph; count = Σ per-edge / 3; endpoints a–b, b–c, c–a |
| `enumerateC4`, `enumerateK4` | Every section; K_n, K_a,b | Brute-force or codegree lists in canonical rotation and direction, exact up to 2 · 10^6 each, else count, validity and no repeats among the first 10^6 |
| `localClustering`, `transitivity`, `squareClustering`, `generalizedDegree`, `averageClustering` (both `count_zeros`) | Every section; hand values K4, C4, K2,3, bowtie (4/12, 12/20), Petersen, loops only; closed forms K_n, K_a,b, C_n, P_n, stars; generalizedDegree rows sum to the degree and Σ k · row = 2 t(v) | NetworkX `cluster.py` transliteration, exact |
| `directedClustering` | Every digraph section; hand values: 3-cycle 0.5, transitive 0.5, complete symmetric 1, one mutual edge 0; `averageClustering` on digraphs | `_directed_triangles_and_degree_iter` transliteration; equals `localClustering` on symmetrised digraphs |
| `weightedClustering`, `weightedDirectedClustering` | Every section with weights 1, random in [1, 10^6] and random in [−5, 5] with max ≠ 0 (the checked and ASan builds rotate one per graph); unit weights equal the unweighted values exactly | Weighted NetworkX iterators transliterated, relative `1e-10` |
| `triangleProductSum` | `lng`, wrapping `ulng`, `ModInt<998244353>`, constant x (T · c^3 mod 2^64), x = 1 | Oracle triangle list |
| `countTrianglesPerEdge` | Every multigraph section; k parallel copies scale by k^2; loops give 0; relabelling permutes; Σ = 3T on simple graphs | Brute force, edge subsets, codegree oracle |
| `countC4` | Every section | Brute force, codegree oracle; K_n 3 C(n, 4), K_a,b C(a, 2) C(b, 2), grids (a − 1)(b − 1) |
| `countC4PerEdge` | Every multigraph section; counting_c4 scale n = m = 3 · 10^5 (stress); copies scale by k^3; Σ = 4 C4 on simple graphs | Brute force, edge subsets, codegree oracle |
| `countC3C4PerVertex` | Every section; Σ c3 = 3T, Σ c4 = 4 C4 | Brute force, codegree oracle, closed forms |
| `findC4` | Every section | Independent witness check (closed walk of 5 vertices, 4 distinct, arc IDs in range, each arc's ends match); empty exactly when `countC4` and the oracle give 0 |
| `countK4` | Every section; K_n up to n = 300 (3–5-word rows) | Brute force, codegree oracle, C(n, 4) |
| `countPathsP3P4P5` | Every section | DFS at every vertex (small, medium) or 201 sampled starts (large); closed forms on K_n, K_a,b, C_n, P_n, stars |
| `triadicCensus` | Exhaustive digraphs n ≤ 4 (full); random digraphs with loops, parallel and antiparallel arcs; large digraphs up to m = 10^6 (stress); transitive tournament (all 030T), complete symmetric digraph (all 300); symmetrised graphs give only 003 / 102 / 201 / 300 with 300 = triangles and 201 = open wedges; sum C(n, 3); the boundary n = 3,810,779 empty and with one 3-cycle | NetworkX code table; a centre-and-neighbour-pair census validated against it on large graphs |
| Domain edge (`countC4PerEdge`, `countTrianglesPerEdge`, plus `countTriangles` and `countC4` on the same multigraphs) | Every mode, m = 6 · 10^6 in the optimized build (checked: multiplicities / 1000; ASan: case 1 full, the others at a quarter), Graph and CsrGraph: (1) a 4-cycle with multiplicities (1, 1999998, 2 · 10^6, 2 · 10^6) plus a chord, largest value 7,999,992,000,000,000,000; (2) the equal split, 1999999 copies of each of three pairs plus the test edge, which reads k^3 = 7,999,988,000,003,999,999; (3) a triangle with multiplicities (1, 2999999, 3 · 10^6). Past the domain: 2.1 · 10^6 copies of each of three pairs plus the test edge (m = 6,300,001), whose 2.1 · 10^6 cubed exceeds 2^63 | Closed-form products in `lll`, each checked ≤ `LLONG_MAX`; past the domain, the closed form mod 2^64 |
| Assertion probes (checked build) | `--invalid`: directed-enumerate, directed-count, directed-count-csr, directed-product, directed-tri-edge, directed-c4, directed-c4-edge, directed-c3c4-vertex, directed-find-c4, directed-k4, directed-paths, undirected-census, undirected-census-csr, product-size-short, product-size-long (modint), census-overflow (n = 3,810,780), census-overflow-4e6, directed-enumerate-c4, directed-enumerate-k4, directed-local-clustering, directed-transitivity, directed-square-clustering, directed-generalized-degree, undirected-directed-clustering, undirected-directed-clustering-csr, directed-enumerate-triangle-edges, directed-weighted-clustering, undirected-weighted-directed-clustering, weighted-parallel, weighted-directed-parallel, weighted-max-zero, weighted-directed-max-zero. The directed `arcs.size() <= INT_MAX / 2` assert is not probed, since it needs 2^30 arcs | SIGABRT from the assertion |
| Purity | Every operation is called twice | Identical results |

Modes (optimized build):
- quick: exhaustive simple graphs with n ≤ 5 and digraphs with n ≤ 3; 300 tiny and 40 medium random rounds; K_n up to 130; dense graphs to n = 300 plus the 16384/16385 boundary and the dispatch cores; large shapes and census at n = 3000.
- full: exhaustive n ≤ 6 and digraphs n ≤ 4; 3000 and 400 rounds; K_n up to 200 (all operations); K300, K1000, K2000 and random dense n = 1000 and 3000; closed forms at n = 2 · 10^5 and K2,50000; large shapes at n = 2 · 10^5 and m = 5 · 10^5 (uniform multigraph, dense blocks, a hub adjacent to every vertex); the census boundary.
- stress: 20000 and 2000 rounds; K2,100000 and closed forms at n = 3 · 10^5; n = m = 3 · 10^5 and n = 2 · 10^5, m = 10^6; census at m = 10^6.

The checked build (about 50× slower) and the ASan/UBSan build (about 10×) run reduced sizes. Checked uses exhaustive n ≤ 5, K_n ≤ 40, large shapes at a tenth, and only the K200 + 5000 dispatch core and dense graphs to n = 300. ASan uses exhaustive n ≤ 5, K_n ≤ 66, large shapes at an eighth and dense graphs to n = 1000 including the boundary. Both run stress at full-mode rounds, and the stress-only large sizes run in the optimized build alone.

Inclusion through `99-all.hpp` and two translation units are covered by `02-integration.py`.

## Commands and results

Run 2026-10-29 on the local machine (GCC 16.2 `g++`, GCC 14.4.1 `g++-14`), after the last header change:

| Command | Result | Memory peak |
|---|---|---|
| `python3 '96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.py' --mode quick` | PASS, 2 configurations, 32 probes; optimized 3720 cases, 138,721,251 checks; checked 3686 cases | 1048 MB |
| `python3 '96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.py' --mode full --seed 371` | PASS, 3 configurations, 32 probes; optimized 82,992 cases, 161,880,851 checks; checked 17,414; ASan/UBSan 17,443 | 1795 MB |
| `CXX=g++-14 python3 '96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.py' --mode full --seed 372` | PASS, 3 configurations, 32 probes | 1727 MB |
| `python3 '96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.py' --mode stress --seed 373` | PASS, 3 configurations, 32 probes; optimized 140,794 cases, 169,935,464 checks | 1710 MB |
| `python3 '96-Local Testing/04-Graphs/35-subgraph_enumeration_benchmark.py'` | PASS, every count checked, 3 variants (Benchmarks) | 2162 MB |
| `python3 '96-Local Testing/02-integration.py'` | PASS, 183 standalone and aggregate headers, multi-TU, workspace | 1311 MB |

The m = 6 · 10^6 domain-edge multigraphs set the tester peak.

## Benchmarks

`python3 '96-Local Testing/04-Graphs/35-subgraph_enumeration_benchmark.py'` (variants `O2`, `popcnt`, `native`; `--quick`, `--variants`), run 2026-10-29 with `DENSE_RATIO = 2`, the array-free `denseStart` and the payload-free `Ranked`. The raw log is the git-ignored `35-subgraph_enumeration_benchmark.json`. The one later header change stores the arc index instead of `2a + flip` in `Ranked(g, true)` and touches only `build_full`. A `--quick` rerun on the final header measured `build_full` equal or lower: G(1000, 0.02) 0.32 ms, K1000 25.9 ms against 34.8, and random n = m = 10^5 7.65 ms against 7.99. Its counts are all checked (`PASS`, peak 1229 MB).

**Conditions.**
- Intel i9-11900H, GCC 16.2.1.
- Builds: `-std=gnu++20 -O2 -DNDEBUG`, then the same plus `-mpopcnt`, then plus `-march=native`. `std::popcount` compiles to the libgcc call `__popcountdi2`, to scalar `popcnt` and to `vpopcntq` respectively.
- Seed 20261029 + case index. One checked warmup, then the median of 5 samples, each repeating the section `clamp(4e6 / (n + m), 1, 200)` times.
- Graph construction is excluded. `build` is `Ranked(g)`, `build_full` is `Ranked(g, true)`, `start` is `denseStart`, `sparse` and `bitset` are the whole-graph kernels (`r0 = 0`), and `count` is `countTriangles` end to end with the suffix dispatch.
- Every case checks sparse == bitset == `countTriangles`, and the closed form where one exists.
- Peak 2162 MB under the 4096 MB cap, from the driver's `MEMORY` line.

Medians in ms, 51 cases (selection below). The build, start and sparse columns are at `-O2`; the other builds give 0.80–1.36 times these values.

| Case | n | m | r0 | build / build_full | start | sparse | bitset (O2 / popcnt / native) | `countTriangles` (O2 / popcnt / native) |
|---|---|---|---|---|---|---|---|---|
| G(500, 0.02) | 500 | 2,485 | 500 | 0.035 / 0.077 | 0.0042 | 0.0080 | 0.022 / 0.0095 / 0.0096 | 0.063 / 0.063 / 0.069 |
| G(1000, 0.02) | 1000 | 9,961 | 1000 | 0.18 / 0.32 | 0.014 | 0.090 | 0.155 / 0.075 / 0.060 | 0.32 / 0.32 / 0.36 |
| G(8000, 0.05) | 8000 | 1.6e6 | 0 | 36.8 / 148 | 2.3 | 140 | 141 / 46 / 24 | 179 / 84 / 63 |
| G(16000, 0.05) | 16000 | 6.4e6 | 0 | 185 / 746 | 8.8 | 1309 | 1536 / 428 / 246 | 1528 / 610 / 443 |
| G(16000, 0.1) | 16000 | 1.28e7 | 0 | 378 / 1616 | 15.8 | 5194 | 2492 / 833 / 472 | 3331 / 1223 / 880 |
| G(32000, 0.02) | 32000 | 1.02e7 | 32000 | 318 / 1295 | 5.2 | 2181 | 6356 / 1807 / 1181 | 2505 / 2512 / 2510 |
| K4000 | 4000 | 8.0e6 | 0 | 194 / 795 | 7.9 | 7033 | 364 / 122 / 68 | 572 / 326 / 278 |
| G(8000, 0.5) | 8000 | 1.6e7 | 0 | 446 / 2078 | 16.6 | 14170 | 1434 / 459 / 250 | 1904 / 927 / 730 |
| K2000 + 100,000-vertex path | 102000 | 2.1e6 | 100000 | 35.1 / 159 | 2.2 | 746 | 679 / 529 / 494 | 87.3 / 54.8 / 50.0 |
| K3000 + 13,385 isolated | 16385 | 4.5e6 | 13385 | 81.6 / 376 | 4.4 | 2660 | 165 / 58 / 35 | 240 / 139 / 120 |
| K3000 + 100,000 isolated | 103000 | 4.5e6 | 100000 | 83.7 / 381 | 4.5 | 2672 | 614 / 513 / 479 | 241 / 142 / 122 |
| K4000,4000 | 8000 | 1.6e7 | 8000 | 410 / 1706 | 16.1 | 12.6 | 1100 / 368 / 211 | 438 / 444 / 452 |
| random n = m = 10^5 | 10^5 | 10^5 | 10^5 | 5.06 / 7.99 | 0.096 | 0.77 | 527 / 467 / 455 | 5.97 / 5.86 / 5.93 |
| random n = m = 5 · 10^5 | 5 · 10^5 | 5 · 10^5 | 5 · 10^5 | 37.0 / 84.2 | 0.096 | 4.74 | not run | 42.7 / 41.8 / 42.7 |

**Choice of `DENSE_RATIO = 2`.** Totals over the 43 cases under `DENSE_WORDS`, using whole-graph probes and scan, against the best kernel per case:

| Build | 1 | 2 (chosen) | 4 | 8 |
|---|---|---|---|---|
| `-O2` | +7.1%, worst 2.36x | +3.0%, worst 1.17x | +0.0%, worst 1.05x | +39%, worst 2.08x |
| `-mpopcnt` | best, worst 1.00x | +1.5%, worst 1.43x | +38%, worst 3.07x | +221%, worst 6.17x |
| `-march=native` | best, worst 1.08x | +8.2%, worst 2.61x | +87%, worst 5.90x | +435%, worst 11.2x |

At `-O2`, the values 1.5, 2 and 3 give the same total. Among the other values the driver evaluates, 0.5 matches 1 (best with `popcnt` and `vpopcntq`, +7.1% and worst 2.74x at `-O2`), 6 costs +0.1% at `-O2` with worst 1.76x and +38% / +87% with hardware popcount, and 12, 16, 24 and 32 are worse everywhere. In probes per scanned word, the bitset starts winning at about 2.8–3.3 at `-O2`, about 1 with `popcnt`, and 0.33–0.85 with `vpopcntq`. Plain `-O2`, without hardware popcount, is weighted first (user decision 2026-10-29, over 1, which has the smaller worst over all three builds). The value 2 then loses at most 1.17x at `-O2`, on G(n ≥ 8000, 0.05), where 4 would cost 38–87% with hardware popcount. A threshold switched on `__POPCNT__` would be optimal under both kinds of build, but the Contest profile has no ISA-specific code.

**Choice of `DENSE_WORDS = 2^22`.** That is 32 MB of rows, k ≤ 16384 suffix vertices. Near the cap the bitset still wins on dense graphs: G(16000, 0.1) runs 2.1x, 6.2x and 11x faster than the sparse loop. The suffix dispatch keeps the dense core of graphs above the cap. In every clique-plus-tail case, `r0` is the first clique rank, including n = 16385 and n = 103000. With a 100,000-vertex tail, the whole-graph bitset alone (build excluded) is 2.55–9.87 times the whole `countTriangles` call, because it allocates up to 1.3 GB of rows.

**End to end.** `countTriangles` against build plus sparse count:
- K4000: 12.6x / 21.2x / 24.8x.
- G(8000, 0.5): 7.7x / 15.8x / 20.2x.
- K3000 suffix cases: 11.3–11.4x / 19.7–20.8x / 22.4–22.9x.

On graphs where the sparse loop runs alone (`r0 = n`), `denseStart` costs 0.2–4.2% of `count` for n ≥ 4000, 3.6–5.9% on K_a,a (its walk reaches the cap), and up to 11% (1.5 µs) at n = 500. There, (build + sparse) / `countTriangles` is 0.64–1.08 across the builds. The gap below n = 2000 is about 15 µs of fixed per-call workspace setup.

## Sources

[00-sources.md](00-sources.md#pages-fetched-on-2026-10-29-p048-gr32): Library Checker `enumerate_triangles` and `counting_c4`, maspypy `count_C3_C4`, `count_P3_P4_P5`, `count_K4` and `find_C4`, Koosaga `count_4_cycles`, OI Wiki 环计数, the ei1333, suisen, hitonanode and tko919 enumerators, and the NetworkX `triads.py` and `cluster.py` code. Written independently from these descriptions; no catalog code was copied.

## Limits and handoffs

- The per-edge operations use multigraph semantics; everything else counts the simple graph (user decision 2026-10-29).
- Recorded keeps in `00-Guidelines/23-Sweep Ledgers/duplicates.md`: `countTriangles` beside `enumerateTriangles` and the sums of the per-vertex and per-edge counts, and `countC4` beside the sums of `countC4PerEdge` and `countC3C4PerVertex`.
- `Ranked` contains its own Batagelj–Zaversnik core order. It is to migrate to planned row 42 (`degeneracyOrdering`, `coreNumbers`) when that row is verified, as long as the shared order still yields sorted rank lists without an extra pass.

## History

- 2026-10-29 (`/finish GR32`): dense kernel moved to a rank suffix chosen by an array-free `denseStart`; `countC4PerEdge` accumulates in `ulng` (user decision); directed `Ranked` bound asserted; 11 operations added (user decisions): `enumerateTriangleEdges`, `enumerateC4`, `enumerateK4`, `generalizedDegree`, the six clusterings and `transitivity`; tester extended to them (32 probes); benchmark rerun.
- 2026-10-29: new row (P048 / GR32); review: 2 medium and 9 low findings fixed (payload-free `Ranked` for seven operations, undefined `d`, shared wedge scan, provenance row, extended keeps, row 42 handoff, signed-`T` contract, bounded `NDEBUG` read, domain-edge tests at m = 6 · 10^6, bound wording, `swap`); full suite passed on g++ and g++-14, stress passed, benchmark rerun.
