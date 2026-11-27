# 10-euleriantrail.hpp — Evidence

Owned by package P011 / GR03 (with `07-lca.hpp`, `08-scc.hpp` and `09-bridges_articulation.hpp`; GR26 `11-functionalgraph.hpp` completes P011); omissions are in [00-notes.md](00-notes.md#p011-re-audit-omissions). The header covers GR03's static Euler-trail scope. It uses the verified P010 `Graph`/`CsrGraph` representation, accepts loops and parallel edges, ignores weights across the full `lng` domain, leaves inputs unchanged and uses iterative traversals.

## Contracts

### Scope

The row covers static Euler trails on one graph. Euler-tour counting belongs to Mathematics `23-matrixtree.hpp` (`eulerianCircuitCountBest`), Chinese postman and route augmentation to `69-tjoin.hpp`, and mixed directed and undirected Euler circuits, which need max flow, to row 30 (`mixedEulerCircuit`, planned).

### EulerTrailResult and eulerianTrail

`eulerianTrail(g,start=-1)` accepts directed or undirected input. `start=-1` chooses a valid start; any other start must be a valid vertex. A valid vertex that cannot start an Euler trail is an ordinary no-answer case, and the `exists` flag distinguishes it from an empty successful trail. On success, `vertices[i]` to `vertices[i+1]` follows the original oriented `arcs[i]`, whose logical ID is `edges[i]`, and every logical edge appears once. On failure all three vectors are empty.

A graph without edges returns the given start, or vertex zero, when it has vertices, and successful empty vectors when it has none. Isolated vertices do not obstruct a trail among the edge-bearing vertices. Loops count twice in undirected degrees and once each toward directed indegree and outdegree. Parallel edges keep their own logical IDs. The witness order is deterministic for a fixed adjacency order and start, with no lexicographic guarantee.

Necessary conditions: undirected, zero or two odd vertices, and with two the start must be odd; directed, either all vertices balanced, or one vertex with outdegree−indegree=1 (which must start the open trail), one with −1 and the rest balanced. A start in an edgeless component cannot cover nonempty input. From a valid start, Hierholzer follows unused edges and emits a vertex and its entering arc when no unused outgoing edge remains; reversing the emitted lists splices the closed excursions into one walk. Marking a logical edge consumes both undirected arcs together, loops included. Each adjacency cursor advances once, so the cost is O(n+m) time and O(n+m) storage including the result. Consuming exactly m edges verifies connectivity of the edge-bearing part; otherwise the call fails. Degree balance plus that connectivity is sufficient by the standard cycle-splicing argument, for an open trail after adding the closing edge.

### eulerianTrail Lexicographic Option

`eulerianTrail(g, start, true)` returns the lexicographically smallest vertex sequence among all Euler trails from the chosen start. With `start=-1` the automatic start is already the smallest valid one (the smallest odd vertex, the unique outdegree-surplus vertex, or the smallest edge-bearing vertex), so the result is the smallest sequence overall. Each adjacency list is copied and stably sorted by target: O(n + m * log(m + 1)) time and O(n + m) memory. Arc and edge witnesses follow the same contract; among parallel arcs the first in input order is used first. Stack Hierholzer that always takes the smallest unused arc outputs the lexicographically smallest Euler trail from its start (OI Wiki 欧拉图; USACO "Riding the Fences"), by this argument. The trail `T` is the reversed pop order. When the algorithm takes arc `a` out of the top occurrence `o` of `v`, the part of `T` before `o` is exactly the current stack path, since everything on it is popped later than what lies above it. The arcs `P` popped so far form the suffix of `T`, and every head occurrence in `P` had no unused arc (no unused incident edge, undirected) when it was popped, while `a` was still unused; so `v` is no head in `P`, and no vertex that is one has an arc outside `P` and the stack path. Now let `b` be an arc out of `v` that sorts before `a` and is not on the stack path. The cursor took it before `a`, so `b` lies in `P`, and since `v` is no head there, `b` is the first arc of `P`. A trail that took `b` at `o` would then stay among the heads of `P` and never return to `v`, so it could not use `a`: `b` is infeasible. So at every step `T` takes the smallest arc that still completes an Euler trail of the remaining arcs, and parallel arcs are interchangeable for the vertex sequence, so induction on the first differing position gives the lexicographic minimum. Only the scan order changes, so existence, witnesses and cost follow from the argument above. The tester's exhaustive search also confirms the output on every case with at most 8 edges.

## Feature-to-Test Map

`96-Local Testing/04-Graphs/10-euleriantrail_tester.py` uses the shared contest runner: GNU++20 optimized `-O2 -DNDEBUG`, checked `-g -D_GLIBCXX_DEBUG` (`-O0` in quick, `-O1` in full and stress), and `-O1 -g` ASan/UBSan with leak detection. Quick omits sanitizers; stress raises the random corpora to 10,000 rounds and chain and cycle sizes to 500,000.

| Operation | Test | Oracle |
|---|---|---|
| `EulerTrailResult`, `eulerianTrail`: existence, selectable start and all witnesses | Every start checked; all looped directed graphs through n=3 and undirected through n=4, two-vertex multiplicities 0..2; 1,500 random multigraph and generated-trail cases with reversal variants | Independent subset DP over possible start vertices for trails using each logical edge once, without degree or connectivity tests |
| `eulerianTrail(..., true)` lexicographic option | Every case with at most 8 edges, automatic start and every explicit start, Graph and CSR; 200,000-vertex reversed-insertion cycles (directed, and undirected with two parallel chords) | Memoized search over used-edge masks for the smallest vertex sequence from every start, cross-checked against the subset DP's existence answer; hand-derived smallest sequence for the large cycles |
| Representations, lifecycle and boundaries | Graph and CSR in every small case, oriented arc and logical-ID validation, zero-edge and n=0 successes, wrong legal starts, degree failures, balanced disconnected graphs, loops and isolates, extreme ignored weights, result move and repeated calls, mutation and snapshot, 200,000-vertex directed and undirected chains and cycles | Same oracles |
| Preconditions | Three checked assertion probes | Expected assertion failure |

## Commands and Results

Intel Core i9-11900H, GNU++20. The 2026-11-10 rows are library-wide runs (P274, seed 42, beside the other suites, build cache emptied before each cold run) with this tester's stage times.

| Date | Command | Result |
|---|---|---|
| 2026-11-10 | `01-run.py --mode full` (GCC 16.2.1) | PASS, counts not recorded: optimized 0.2 s compile + 1.4 s run, checked 22.2 s + 8.0 s, ASan-UBSan 22.3 s + 33.9 s; peak 1014 MB |
| 2026-11-10 | `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | PASS, counts not recorded: 0.3 s + 2.1 s, 29.9 s + 12.9 s, 28.9 s + 43.2 s; peak 1036 MB |
| 2026-11-10 | `01-run.py --mode quick` | PASS: optimized 24.6 s + 0.1 s, checked 28.4 s + 4.9 s; peak 793 MB |
| 2026-11-10 | `01-run.py --mode stress --rounds 1` | PASS, counts not recorded: 0.1 s + 1.5 s, 8.1 s + 9.0 s, 7.8 s + 35.0 s; peak 1051 MB |
| 2026-10-07 | `10-euleriantrail_tester.py --mode full --seed 20260927` (GCC 16.2.1) | PASS 3 configurations, 3,390 cases, 16,171,778 checks, 3 probes |
| 2026-10-14 | `CXX=g++-14 10-euleriantrail_tester.py --mode full --seed 20261014` (GCC 14.4.1) | PASS 3 configurations, 3,390 cases, 16,169,676 checks per configuration, 3 probes; peak 630 MB |
| 2026-10-07 | `10-euleriantrail_tester.py --mode stress --seed 1` | PASS 3 configurations, 12,740 cases |
| 2026-10-07 | `02-integration.py` and `02-integration.py --sanitizers` | PASS: 102 standalone and aggregate headers, scalar and available-AVX2 multi-TU, workspace, sanitizer self-tests |
| 2026-10-07 | Ten hand mutations, quick suites | Every non-equivalent mutant fails |
| 2026-10-07 | `@reviewer`: own brute-force lexicographic check, 200k Euler multigraphs with up to 11 edges, ASan/UBSan/`_GLIBCXX_DEBUG` | PASS |

```bash
python3 '96-Local Testing/04-Graphs/10-euleriantrail_tester.py' --mode full --seed 20260927
CXX=g++-14 python3 '96-Local Testing/04-Graphs/10-euleriantrail_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/04-Graphs/10-euleriantrail_tester.py' --mode stress --seed 1
python3 '96-Local Testing/01-run.py' --mode full
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```

## Sources

Retrieved and inspected on 2026-09-27. The code was written independently from the algorithms and compared with these references; it copies neither.

| Source | Inspected claim and application |
|---|---|
| [cp-algorithms, Finding the Eulerian path in O(M)](https://cp-algorithms.com/graph/euler_path.html), last update 2026-09-18 | Undirected multigraph and loop degree conditions, exclusion of isolates, iterative Hierholzer and the unused-edge connectivity check. Its matrix example does not establish the adjacency-list complexity. |
| [KACTL, EulerWalk.h](https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/graph/EulerWalk.h), Simon Lindholm, 2019-12-31, CC0 | Directed and undirected logical edge marking, iterative trail recovery and start-sensitive validity. This API checks the degree conditions explicitly and returns both arc and logical-edge certificates. |

The lexicographic option's sources (OI Wiki 欧拉图; USACO "Riding the Fences") are in [00-sources.md](00-sources.md#pages-fetched-on-2026-10-07-p011-re-audit); rejected candidates are in [00-notes.md](00-notes.md#p011-re-audit-omissions).

Legacy: the `EulerPath` sections of `OLD/Team Notebook/src/algsbetter.cpp` (line 1590), `algs.cpp` (1532) and `test.cpp` (394) provide repeatable undirected Hierholzer with logical edge marks, odd-degree selection and a vertex witness. This header adds directed graphs, explicit starts and arc and edge witnesses, and replaces the old `{0}` result on a zero-vertex graph with the empty successful result. Archived names and signatures are not compatibility APIs. The archived `EulerTourTree` concerns subtree flattening, not Euler trails.

## Limits and Handoffs

None.

## Aliases

Search aliases (P263, grab --find): Euler path, 欧拉图, 欧拉回路, 欧拉路径, 一笔画, 欧拉通路, 字典序最小欧拉路径.

## History

- 2026-09-27: original P011, full suite (seed 20260927, 3,390 cases, 14,683,272 checks, 3 probes) and integration passed on g++ 16; stress not run.
- 2026-10-07: re-audit, added the lexicographic option; no findings for this header; full suite, stress and integration passed on g++ 16; independent review: all eight package findings confirmed fixed, its own brute-force checks of every addition passed under ASan/UBSan/`_GLIBCXX_DEBUG`.
- 2026-10-14: floor check, full suite (seed 20261014) passed with g++-14.
- 2026-11-06: P253 /finish (user decision 2026-11-06, `03-cpp.md`, Headers): template parameters taking a library type default to it (`template<class G = Graph>`), so `grab` of a function brings that type; calls deduce as before. `01-run.py --filter 04-Graphs` quick, full and `CXX=g++-14` full and `02-integration.py` passed; no mutation record, so no campaign.
- 2026-11-07: P263 follow-up added Chinese names to the search aliases: OI Wiki's page titles and common names of the row's operations (Simplified, and Taiwan or Hong Kong terms where they differ); no code or test change.
- 2026-11-08: P263 user request added `Euler path` to the search aliases for a held-out corpus query the search missed (00-search.md, Evaluation); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared contest runner `_00_contest_runner.py` and its ccache `Builder` with `-Wall -Wextra -Wconversion -Werror` and the 10 s quick bound; full and stress inputs and checks unchanged; library-wide quick, full (g++ and g++-14) and stress runs passed.
- 2026-11-15: P011 GR03 completeness re-check (two `@researcher` passes, [sources](00-sources.md#fetched-2026-11-15-p011-gr03)): nothing missing from the row; aliases and dominated algorithms only. No code or test change.
- 2026-11-27: P011 limits recheck: the ownership boundaries moved to Contracts (Scope); the lexicographic option's correctness argument written (it was cited only); independent review; documents only, no run.
