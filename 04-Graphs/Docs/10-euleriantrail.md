# 10-euleriantrail.hpp — evidence

Owned by package P011 / GR03 (with `07-lca.hpp`, `08-scc.hpp` and `09-bridges_articulation.hpp`; GR26 `11-functionalgraph.hpp` completes P011); see [00-notes.md](00-notes.md#p011-package-record) for the package record. The header is complete within GR03's static Euler-trail scope. It uses the verified P010 `Graph`/`CsrGraph` representation, accepts loops and parallel edges, ignores weights across the full `lng` domain, leaves inputs unchanged and uses iterative traversals.

## Contracts

### EulerTrailResult and eulerianTrail

`eulerianTrail(g,start=-1)` accepts directed or undirected input. `start=-1` chooses a valid start; any other start must be a valid vertex. A valid vertex that cannot start an Euler trail is an ordinary no-answer case. The result's `exists` flag distinguishes failure from an empty successful trail. On success, `vertices[i]` to `vertices[i+1]` follows original oriented `arcs[i]`, whose logical ID is `edges[i]`; every original logical edge appears once. On failure all three vectors are empty.

For a graph without edges, a nonempty vertex set returns the specified start or vertex zero, and a zero-vertex graph returns successful empty vectors. Isolated vertices do not obstruct a trail among the edge-bearing vertices. Loops count twice in undirected degrees and once each toward directed indegree/outdegree. Parallel edges retain independent logical IDs. The witness order is deterministic for fixed input adjacency/start but has no lexicographic guarantee.

The undirected degree condition is zero or two odd vertices; in the latter case the requested start must be odd. The directed condition is either all balanced, or one vertex with outdegree−indegree=1, one with −1, and every other vertex balanced; the +1 vertex must start an open trail. A start in an edgeless component cannot cover nonempty input. These conditions are necessary. With the valid start, Hierholzer repeatedly follows an unused edge and emits a vertex/entering arc when no unused outgoing edge remains. Reversing the emitted lists splices the closed excursions into one oriented walk. Logical edge marking consumes both reciprocal undirected arcs together, including loops. Every adjacency cursor advances once, so construction is O(n+m) time and O(n+m) storage including the result. Consuming exactly m edges verifies the remaining connectivity condition; otherwise it returns failure. Degree balance plus connectivity of the edge-bearing part is sufficient by the standard cycle-splicing argument, also after adding the closing edge for an open trail.

### eulerianTrail lexicographic option

`eulerianTrail(g, start, true)` returns the lexicographically smallest vertex sequence among all Euler trails from the chosen start; with `start=-1` the automatic start is already the smallest valid one (the smallest odd vertex, the unique outdegree-surplus vertex, or the smallest edge-bearing vertex), so the result is the smallest sequence overall. Each adjacency list is copied and stably sorted by target, giving O(n + m * log(m + 1)) time and O(n + m) memory; arc and edge witnesses follow the same contract, and among parallel arcs the first in input order is used first. Correctness rests on the standard result that stack Hierholzer, always taking the smallest unused arc, outputs the lexicographically smallest Euler trail from its start (OI Wiki 欧拉图; USACO "Riding the Fences"). The order of the scan is the only change, so existence, witnesses and cost follow from the unordered argument above. This record cites that result rather than reproducing its proof. The tester's exhaustive memoized search over used-edge masks independently confirms the output on every enumerated and random case with at most 8 edges.

## Feature-to-test map

`96-Local Testing/04-Graphs/10-euleriantrail_tester.py` reuses the graph test driver with non-removable assertions, failing graph/operation output, reproducible seeds and subprocess timeouts. The driver compiles GNU++20 optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_DEBUG`, and `-O1 -g` ASan/UBSan with leak detection. Quick/full/stress coverage is documented in the runnable entry; quick omits sanitizers, and stress expands random corpora to 10,000 rounds and chain/cycle sizes to 500,000.

| Operation | Test | Oracle |
|---|---|---|
| `EulerTrailResult`, `eulerianTrail`: existence, selectable start and all witnesses | Every start checked; all looped directed graphs through n=3 and undirected through n=4, two-vertex multiplicities 0..2; 1,500 random multigraph/generated-trail cases and reversal variants | Independent subset DP enumerates possible start vertices for trails using each logical edge once, without degree/connectivity tests |
| `eulerianTrail(..., true)` lexicographic option | Every case with at most 8 edges, automatic start and every explicit start, Graph and CSR; 200,000-vertex reversed-insertion cycles (directed, and undirected with two parallel chords) | Memoized search over used-edge masks computes the smallest vertex sequence from every start; cross-checked against the subset DP's existence answer; hand-derived smallest sequence for the large cycles |
| Representations, lifecycle and boundaries | Graph/CSR in every small case, oriented arc/logical-ID validation, zero-edge and n=0 successes, wrong legal starts, degree failures, balanced disconnected graphs, loops and isolates, extreme ignored weights, result move/repeated calls, mutation/snapshot, 200,000-vertex directed/undirected chains and cycles | Same oracles |
| Preconditions | Three checked assertion probes | Expected assertion failure |

Finite tests complement the algorithm arguments; they do not prove all inputs.

## Commands and results

2026-10-07, GCC 16.2.1, GNU++20.

| Command | Result |
|---|---|
| `10-euleriantrail_tester.py --mode full --seed 20260927` | PASS 3 configurations, 3,390 cases, 16,171,778 checks, 3 probes |
| `10-euleriantrail_tester.py --mode stress --seed 1` | PASS 3 configurations, 12,740 cases |
| `02-integration.py` and `02-integration.py --sanitizers` | PASS: 102 standalone/aggregate headers, scalar/available-AVX2 multi-TU, workspace, sanitizer self-tests |
| `03-consistency.py` | No errors |
| Ten hand mutations, quick suites | All non-equivalent mutants fail |
| `@reviewer`: own brute-force lexicographic check, 200k Euler multigraphs with up to 11 edges, ASan/UBSan/`_GLIBCXX_DEBUG` | PASS |

```bash
python3 '96-Local Testing/04-Graphs/10-euleriantrail_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/04-Graphs/10-euleriantrail_tester.py' --mode stress --seed 1
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

Retrieved and inspected on 2026-09-27. The code was independently written from the algorithms and compared with these references; it does not copy their implementations.

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Finding the Eulerian path in O(M)](https://cp-algorithms.com/graph/euler_path.html), last update 2026-09-18 | Undirected multigraph/loop degree conditions, exclusion of isolates, iterative Hierholzer and unused-edge connectivity check. Its matrix example does not establish the new adjacency-list complexity. |
| [KACTL, EulerWalk.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/EulerWalk.h), Simon Lindholm, 2019-12-31, CC0 | Directed/undirected logical edge marking, iterative trail recovery and start-sensitive validity. The new API checks directed/undirected degree conditions explicitly and returns both arc and logical-edge certificates. |

The lexicographic option's sources (OI Wiki 欧拉图; USACO "Riding the Fences") are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit); rejected candidates are in [00-notes.md](00-notes.md#p011-re-audit-omissions).

Legacy: the `EulerPath` sections of `OLD/Team Notebook/src/algsbetter.cpp` (line 1590), `algs.cpp` (1532) and `test.cpp` (394) were inspected. They provide repeatable undirected Hierholzer with logical edge marks, odd-degree selection and a vertex witness; the new implementation also supports directed graphs, explicit starts and arc/edge witnesses. The old `{0}` result on a zero-vertex graph is replaced by the documented empty successful result. Archived names/signatures are not active compatibility APIs, and original archive bytes remain untouched. The archived `EulerTourTree` concerns subtree flattening, not Euler trails.

## Limits and handoffs

Euler-tour counting belongs to `53-graph_counting.hpp`, and Chinese postman/route augmentation to `69-tjoin.hpp`; mixed directed/undirected Euler circuits need max flow and belong with rows 19/30/69. These are not claimed here. No `CXX=g++-14` floor run is recorded for this header.

## History

- 2026-09-27: original P011, full suite (seed 20260927, 3,390 cases, 14,683,272 checks, 3 probes) and integration passed on g++ 16; stress not run.
- 2026-10-07: re-audit, added the lexicographic option; no findings for this header; full suite, stress and integration passed on g++ 16.
