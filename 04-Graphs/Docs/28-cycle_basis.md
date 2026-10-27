# 28-cycle_basis.hpp — evidence

Owned by package P048 / GR13; omissions are in [00-notes.md](00-notes.md#p048-gr13-omissions). Contest profile: fundamental cycle and cut bases over one BFS spanning forest, GF(2) independence of edge sets, XOR walk values and a randomized cut-space hash. A new row with no legacy code.

## Contracts

Every builder takes an undirected `Graph` or `CsrGraph` (asserted; `isIndependentF2` reads only `g.edges.size()`); loops, parallel edges, isolated vertices and `n = 0` are allowed. Edge IDs index `g.edges`, so results stay meaningful only while the graph is unchanged.

### Spanning forest

All builders use one forest. BFS starts from each unreached vertex in increasing order and scans arcs in adjacency order. `parent_edge[v]` is the edge to `v`'s parent (−1 at the root, the component's minimum vertex), and `depth[v]` is the BFS distance from that root. BFS keeps fundamental cycles short: a cycle has at most `2 · depth + 1` edges.

### CycleBasis and fundamentalCycleBasis

`chords` lists the non-forest edges in ascending ID order; loops are always chords. `cycles[i]` is the closed walk of chord `c = chords[i]` as edge IDs. It starts with `c`, goes from `edges[c].u` to `edges[c].v`, then returns to `u` along the forest path, so each edge's direction follows from the walk. A loop's cycle is `{c}`, and a chord parallel to a forest edge gives a two-edge cycle. These `m − n + c` simple cycles form a basis of the cycle space over GF(2): each contains exactly one chord. Time O(n + m + out), where `out` is the total cycle length (at most `(m − n + c) · n`).

The cycle space of a directed graph is that of its underlying undirected graph. Build the undirected copy (`Graph(n)` with the same `addEdge` calls), which keeps the edge IDs. Each walk's direction against `edges[e]` then gives the ±1 coefficients of the integer flow basis.

### CutBasis and fundamentalCuts

`tree` lists the forest edges in ascending ID order. `cuts[i]` lists, ascending, the edges between the subtree below `tree[i]` and the rest of its component, `tree[i]` included. Loops never appear. A chord `c` lies in the cut of forest edge `t` iff `t` lies on `c`'s cycle (cut–cycle duality), and that is how the cuts are computed. The cuts form a basis of the cut space. Time O(n + m + out), where `out` is the total cut size.

### cycleSpaceDimension

`m − n + c`, where `c` is the number of components (`connectedComponents`). O(n + m).

### isIndependentF2

`isIndependentF2(g, sets)` returns whether the edge sets, read as GF(2) vectors over `[0, m)` (IDs asserted), are linearly independent. Repeated IDs inside a set cancel, so a set that cancels to empty makes the family dependent. No sets gives true, and more than `m` sets gives false. Each set is reduced in one ordered pass against the stored rows, since each row's pivot is absent from every later row. Time O(k · r · m / 64 + s) with `r = min(k, m)` and `s` the total set size. It works on packed words of its own, not Core `BitMatrix` (row 12 is an unverified legacy excerpt), and the duplicates ledger keeps it as the bool form.

### XorWalk

`XorWalk(g)` treats each weight as its 64-bit pattern `ulng(w)`. Walk values are XORs along u–v walks, and edges may repeat. `component` numbers components by increasing minimum vertex. `dist[v]` is the XOR along the forest from the root, and `basis[c]` is a fully reduced basis of the chord cycle values `dist[u] ^ dist[v] ^ w`, sorted in decreasing order. The u–v walk values are exactly `dist[u] ^ dist[v] ^ span(basis)`. A walk differs from the forest path by an even subgraph plus edges walked twice, and any cycle value can be added by walking to the cycle and back.

- `count(u, v)`: the number of distinct walk values, `2^rank` as `ulll`, or 0 when disconnected.
- `contains(u, v, x)`: whether `x` is a walk value (false when disconnected).
- `minXorWalk`, `maxXorWalk`: assert that `u` and `v` are connected.
- `kth(u, v, k)`: the `k`-th smallest walk value, 0-based; asserts `k < count(u, v)`. In a fully reduced basis each element owns its leading bit, so choosing basis elements by the bits of `k` orders the values.

`u = v` asks for closed walks, so 0 is always a value. Build time O(n + m · 64), query O(64), memory O(n + m).

### CutSpaceHash

`CutSpaceHash(g, seed = CustomHash::rnd)` gives each chord the label `splitMix64(splitMix64(seed) + e)`, so adjacent seeds give unrelated labels. Each forest edge gets the XOR of the labels of the chords crossing its fundamental cut, computed as a subtree XOR. So the labels of any cut XOR to zero, and a set outside the cut space XORs to a uniform random value. `disconnects(edges)` takes distinct IDs (asserted) and reports whether deleting them increases the number of components, using the GF(2) dependence of their labels.

Deleting `S` disconnects a component iff some nonempty subset of `S` is a cut, so true answers are always reported. A false true needs the random map to kill some nonzero vector of a space of dimension at most `k = |S|`, which has probability below `2^(k − 64)`. The answer is meaningful only for small `k`: the bound is about 6% at `k = 60`, so the usable range is about `k ≤ 40`, and every set of more than 64 edges reports true at once (64-bit labels are then dependent). 64-bit labels are a user-accepted limit (user decision 2026-10-28). A seed gives reproducible labels; pass a fresh seed (the default `CustomHash::rnd`) against adaptive hacks. Build O(n + m), query O(min(k, 64)^2).

## Aliases

- `cycleSpaceDimension`: cyclomatic number, circuit rank, nullity, first Betti number; corank of the graphic matroid.
- `fundamentalCycleBasis`: NetworkX `cycle_basis` (node lists, undirected).
- `fundamentalCuts`: fundamental cut-sets (bonds) of a spanning tree.
- `XorWalk::maxXorWalk`: Luogu P4151 maximum XOR path; `minXorWalk`: CF 845G, ABC451 G.
- `CutSpaceHash::disconnects`: BZOJ 3569 "DZY Loves Chinese II".

## Feature-to-test map

`96-Local Testing/04-Graphs/28-cycle_basis_tester.py` (shared graph runner with `-Werror` warnings; optimized, checked with 7 assertion probes, ASan/UBSan in full and stress). Oracle self-tests on hand cases cover every brute force below.

| Operation | Test | Oracle |
|---|---|---|
| Forest (`parent_edge`, `depth`) | `verifyBases`, a third of the graphs with shuffled adjacency; hand square with a reversed list | Separate queue BFS; roots are DSU component minima, parents one level up, every edge spans at most one level, `n − c` distinct parent edges |
| `fundamentalCycleBasis`, `CycleBasis` | `verifyBases`: every simple graph and random multigraphs with loops, isolated vertices, parallel edges; K1–K9; large shapes | Chords are exactly the non-forest edges; `m − n + c` cycles, each a simple closed walk chord-first over forest edges; Gauss–Jordan rank; Graph and CsrGraph agree; repeat call |
| `fundamentalCuts`, `CutBasis` | `verifyBases` | Brute split of the forest at each edge (8 sampled per large graph); cut–cycle duality both ways |
| `cycleSpaceDimension` | `verifyDimension` | `2^dim` equals the brute count of even-degree subsets (m ≤ 12); DSU components |
| `isIndependentF2` | `verifyIndependence` | Brute subset enumeration (k ≤ 10, m ≤ 64) or brute rank; duplicates, empty sets, no sets, k > m; cycles, cuts and summed cycles |
| `XorWalk` fields | `verifyXorFull` | DSU component ids; `dist` along the reference forest; `basis` equal to a reduced basis built from a DFS forest rooted at the maximum vertex |
| `count`, `contains`, `minXorWalk`, `maxXorWalk`, `kth` | `verifyXorSmall` (weights < 2^4, loops, parallel edges, u = v); `verifyXorFull` (full-range signed weights) | Exact walk-value sets by BFS over (vertex, xor) states; pivot basis over the other forest, full kth enumeration for rank ≤ 7, monotone samples otherwise, a rank-64 case with count 2^64 and kth(k) = k |
| `CutSpaceHash`, `disconnects` | `verifyHash` (every subset for m ≤ 8, else 100 random sets; two fixed seeds and the default); `midHash` (dense multigraphs n ≤ 12, m ≤ 80: random k in [7, 64], k = 64, chord sets, padded fundamental cuts and stars); large graphs, including 65-edge sets | Deletion DSU, exact for k ≤ 44 with fixed seeds (k ≤ 40 with the default seed; false-true odds below 2^-20 per set) and one-sided above; seed determinism and no label shift between adjacent seeds; on paths any edge disconnects, on cycles one edge does not and two do; more than 64 edges report true |
| Assertion probes | `--invalid` | directed input, edge out of range, max on disconnected pair, kth out of range, duplicate and out-of-range `disconnects` ids, vertex out of range |

## Commands and results

Run 2026-10-28 on the local machine (GCC 16.2 `g++`, GCC 14.4.1 `g++-14`):

| Command | Result | Memory peak |
|---|---|---|
| `python3 '96-Local Testing/04-Graphs/28-cycle_basis_tester.py' --mode quick` | PASS, 2 configurations, 7 probes | 526 MB |
| `python3 '96-Local Testing/04-Graphs/28-cycle_basis_tester.py' --mode full --seed 1` | PASS, 3 configurations | 883 MB |
| `CXX=g++-14 python3 '96-Local Testing/04-Graphs/28-cycle_basis_tester.py' --mode full --seed 2` | PASS, 3 configurations | 878 MB |
| `python3 '96-Local Testing/04-Graphs/28-cycle_basis_tester.py' --mode stress --seed 3` | PASS, 3 configurations | 940 MB |

The checked build (`-O0 -D_GLIBCXX_DEBUG`) runs stress at full-mode sizes (3,000 rounds, 400 hash graphs, 200,000-vertex shapes): 42.6 s alone, 52 s under parallel load, against the runner's 180 s limit. Optimized and ASan run 15,000 rounds, 1,500 hash graphs and 300,000-vertex shapes.

`python3 '96-Local Testing/02-integration.py'`: PASS, 180 standalone and aggregate headers, multi-TU, workspace (2026-10-28).

Benchmarks: none required (Contest profile, no Barrett or Montgomery reduction).

## Sources

[00-sources.md](00-sources.md#pages-fetched-on-2026-10-28-p048-gr13): NetworkX cycles, maspypy fundamental cut, OI Wiki linear basis, AtCoder ABC451 G and the ABC419 editorial, the Gloid BZOJ 3569 post. Written independently from the algorithm descriptions; no catalog code was copied. Not inspected: the Codeforces 845G statement (HTTP 403), Luogu P4151 and CF 1889E.

## Limits and handoffs

- Design calls confirmed by the user (user decision 2026-10-28): the BFS forest, cycles as chord-first edge walks, `XorWalk` over walks with `ulng(w)` patterns and `count` as `ulll`, and a one-sided Monte Carlo `CutSpaceHash`.
- The dynamic fundamental cut is planned row 90 (P249).
- Explicit cycles and cuts cost their output size, up to O(n · m). For path XOR or parity queries use `XorWalk` or the forest fields.
- `cycle_basis_detail::insert` and `reduce` are a fully reduced 64-bit XOR basis. When Data Structures 62 `XorBasis` (batch DS50) is verified, `XorWalk` and `CutSpaceHash` migrate to it; P032's plan note records the handoff (user decision 2026-10-28).
- Minimum-weight cycle bases are row 72; cycle enumeration is row 68; general GF(2) linear algebra is Core 12 and Data Structures 62.

## History

- 2026-10-28: new row (P048 / GR13); sweep added XorWalk and CutSpaceHash; review: 3 findings fixed (cut memory term, `disconnects` cost, tests and docs); full suite passed on g++ and g++-14, stress passed.
- 2026-10-28: `/finish` audit: seed mixing, assert-only duplicate check, mid-size hash sets, shuffled adjacency, checked-build scaling; full suite passed on g++ and g++-14, stress passed.
