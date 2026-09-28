# Local library verification

From the library root (the scripts resolve library paths independently of the caller's working directory):

```bash
python3 '96-Local Testing/01-run.py' --mode quick
python3 '96-Local Testing/01-run.py' --mode full
python3 '96-Local Testing/01-run.py' --mode stress --seed 42 --rounds 3
python3 '96-Local Testing/01-Core/07-infint_tester.py' --scalar-only
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/00-Tools/02-online_tester.py'
python3 '96-Local Testing/00-Tools/01-contest_tester.py' --mode full
```

Each header has one Python entry where a suite exists; supporting C++ and compile checks are allowed. Sixty-six current header entries cover template, debug, Barrett, Montgomery, the full modular family, the mini modular family, infint, infintmini, bitset, the eight P006 data-structure foundations, the nine P007/P008/P009 geometry headers the eleven P010/P011 graph headers the six P012 mathematics headers the seven P013 miscellaneous headers the four P014 sequence/interval/offline/sorting headers and three P015 enumeration/knapsack/cycle-finding headers, plus the six P016 string foundations and three P017 Aho/suffix-array/palindrome-query headers. P002 verified template/debug/bitset, P004 verified Barrett/Montgomery and P005 owns both four-type modular families; the big-integer suites retain their migrated scope. Three additional entries under `00-Tools` preserve regressions for contest tools, online expansion/workspace generation, and the notebook. **There are not yet complete feature suites for every existing or planned header.** In particular Matrix, advanced Mathematics beyond P012 and miscellaneous algorithms beyond P015 need feature inventories/oracles before they can be called fully verified. This migration does not conceal those gaps.

| Mode | Actual coverage |
|---|---|
| quick | Tool fixtures (contest smoke subset) plus template/debug/Barrett/Montgomery/full-modular/mini-modular/bitset and P006 data-structure/P007–P009 geometry/P010–P011 graph/P012 mathematics/P013–P015 miscellaneous and P016–P017 string suites plus standalone/aggregate/multiple-TU integration and repository consistency checks; excludes big-integer corpora |
| full | All existing suites, big-integer differential corpora and available scalar/AVX2 paths, integration and ASan/UBSan C++ self-tests |
| stress | Full suite set repeated for configurable seeds (Python big-integer corpora and contest-tool histories vary), then integration/sanitizers; fixed-seed C++ self-tests remain fixed |

`--filter` selects suite path substrings; `--no-integration` omits integration and repository consistency checks. `CXX` selects GCC. Full/stress may take minutes. No mode claims missing per-header coverage is implemented. See [testing policy](../00-Guidelines/05-testing.md) for new suites. Existing test assertions use non-removable checks where needed. Shared runner warnings are retained; safe narrowing is not automatically a defect.

The integration runner tests every present header independently, combined aggregates in two translation units, scalar and available AVX2 linkage, and the standalone contest template in LOCAL/non-LOCAL builds. Its sanitizer mode executes existing C++ self-tests; Python differential corpora run separately in their normal configurations. Test binaries are temporary and not committed. Tool-specific fixture validation is recorded in [migration notes](../00-Guidelines/11-migration.md).

P003 separately verified the Workspace snapshot with `--check` and isolated compile/run checks. The integration runner now fails clearly when that required snapshot is missing, resolving the reporting gap recorded in its [original evidence](<../97-Online Testing/04-workspace-verification.md>). Generation remains explicit.

The online tools fixture uses a fake bundler and temporary copies to check failure preservation, owned-output cleanup and `--noclean`, unsafe paths and symlinks, and explicit script-relative workspace generation and `--check`. It never writes to real online outputs or Workspace.

The contest tool entry accepts `--mode quick|full|stress` and `--seed`, as well as the shared runner's environment settings. Quick runs eight smoke/regression methods, full runs 27 deterministic protocol methods, and stress adds two seeded oracle methods (12 token/byte cases and six shrink histories per seed). It checks every numbered contest script, process cleanup, C++ compilation, replay/artifacts and hook error contracts from an unrelated working directory. Failed fixtures are retained with their command/seed/cwd. See [P001 evidence](<../09-Contest Testing/10-verification.md>) for the per-file map, results and platform limits.

## P002 foundation coverage

[Template/debug evidence](../01-Core/21-c01-verification.md) and [bitset contracts/evidence](../01-Core/22-bitset.md) contain per-feature maps, reproducible commands and limits. Their Python entries accept quick/full/stress and seeds, preserve oracles under NDEBUG, test checked/sanitized builds, and use temporary build directories. Bitset additionally executes the scalar and available AVX2 paths, aliasing and padding boundaries, invalid-precondition subprocesses, and independent byte-per-bit exhaustive/random oracles. Quick includes bitset discovery in the shared runner. The [bitset benchmark](01-Core/18-bitset_benchmark.py) separates timings from correctness, records environment/flags/seeds/medians, and compares generic word loops and temporary-producing workloads without a universal timing gate.

## P004 reduction coverage

[Reduction contracts and evidence](../01-Core/23-reduction.md) map all Barrett/Montgomery operations to independent exact oracles, bounded exhaustive domains, random cases, full-width carry regressions, aliasing, invalid-precondition subprocesses and scalar/AVX2 configurations. The `03-barrett_tester.py` and `04-montgomery_tester.py` entries provide quick/full/stress modes with seeds; full/stress include ASan/UBSan. Both are discovered by the shared quick runner. Tests require only Python's standard library and GCC; temporary build files stay outside the library. Barrett additionally verifies its exact high-product helper using an independent bitwise 256-bit oracle. Integration covers the new header in All, standalone headers and multiple translation units.

The [reduction benchmark](01-Core/03-reduction_benchmark.py) records native `%`, reciprocal, fixed-multiplier and Montgomery comparisons across scalar/AVX2 builds, sizes around the eight-element vector threshold, full-width/boundary inputs, powers, dependent products and conversions. Context setup is included in the exponentiation workload and excluded from repeated kernels. Measurements inform implementation choices; they are not portable timing gates or authorization to use these backends in non-Core algorithms without the required end-to-end evidence.

## P005 modular-integer coverage

One `05-modint_tester.py` entry runs all four full-type C++ suites; `06-modintmini_tester.py` runs the four independent minis. Both entries are discovered in quick/full/stress runs. The full-family [C03 evidence](../01-Core/24-modint.md) maps all public operations to independent exact Python oracles, exhaustive small rings, deterministic random inputs, 128-bit construction/exponents, full unsigned word moduli, prime roots, composite-unit and zero-ring semantics, batch aliases/failures, streams and declared precondition deaths. Static suites check constexpr behavior and zero-modulus rejection; dynamic suites check ID/width independence, reset rules and global initialization in both translation-unit link orders. Each full run passed optimized, checked and ASan/UBSan scalar/AVX2 configurations on GCC16; this does not claim execution on unavailable GCC14/Python3.10/PyPy runtimes.

The [mini evidence](../01-Core/25-modintmini.md) records reduced API coverage, independent exact oracles, independently extracted struct builds, context/translation-unit checks and compact-backend measurements. Full/mini agreement is supplemental evidence.

The [modint benchmark](01-Core/05-modint_benchmark.py) and [record](01-Core/05-modint_benchmark.jsonl) compare native/reducer candidates and actual types, count setup/conversions at power thresholds, preserve output checks and include individual versus batch inversion. The generic Montgomery helper avoids a measured GCC constant-specialization regression. Matrix/ModFac receive an ordinary `mint` compatibility smoke only; the [P020/C07 handoff](../01-Core/24-modint.md#dependent-owner-handoff) preserves the Matrix domain/classification work.

The [consistency validator](03-consistency.py) checks inventory/batch/package ownership, numbering, prerequisites, current paths, local Markdown formatting/links, aggregate coverage, preserved provenance and generated manifests without compiling algorithms.

## P006 data-structure foundations

Eight entries under `02-Data Structures` cover DSU, Fenwick, monoid segment tree, sparse table, prefix/difference arrays, block decomposition, ordered containers and monotone stack/deque/rectangle algorithms. They are discovered in quick/full/stress modes and honor seeds from the shared runner. [P006 contracts and evidence](<../02-Data Structures/90-foundations.md>) records all public features, algebra/index/lifetime contracts, independent brute-force oracles, exhaustive/random corpora and 100 asserted-precondition probes.

Full mode passed optimized NDEBUG, checked and ASan/UBSan with leak checking for all eight headers. Ordered checked builds use `_GLIBCXX_ASSERTIONS` because PBDS's debug diagnostic comparator mishandles supplied stateful comparators; the full comparator tests remain enabled. Other checked builds use `_GLIBCXX_DEBUG`. `--configuration` permits a single configuration retry after an environment failure without relabeling its coverage. The [construction benchmark](<02-Data Structures/90-foundations_benchmark.py>) and [record](<02-Data Structures/90-foundations_benchmark.json>) compare linear Fenwick construction with point updates on three sizes; timings are observations, not portability gates.

## P007 geometry foundations

Four entries under `03-Geometry` cover 2D/3D points, exact line/segment/ray intersections and approximate metrics, polygon moments/centroids/winding/holes/lattice counts, and monotone-chain/Graham hulls. The shared runner discovers them in quick/full/stress modes. [P007 contracts and evidence](../03-Geometry/90-foundations.md) records numeric/topology domains, per-feature independent oracles, source and legacy accounting, completed configurations, and the hull comparison benchmark. Python arbitrary-precision fixtures require only the standard library; C++ test outputs are temporary.

## P008 closest pair, circles, calipers and transforms

Four additional entries under `03-Geometry` cover deterministic exact closest pair, approximate circle intersections/tangents/constructions/measures, exact caliper support choices with approximate enclosing shapes, and affine coordinate transforms. [P008 contracts and evidence](../03-Geometry/91-ge02.md) records per-header domains, proofs, source scopes, feature maps and completed configurations. Independent evidence includes Python arbitrary integers/Fractions, exhaustive pair/edge-pair enumeration, exact support witnesses, circle slice integration and geometric invariants. All entries expose quick/full/stress modes and deterministic seeds; the shared runner discovers them automatically. Full mode includes optimized NDEBUG, checked assertion probes and ASan/UBSan with leak checking. Benchmark results and their limits are recorded alongside the evidence.

## P009 triangle coverage

The `03-Geometry/09-triangle_tester.py` entry covers exact area/centroids/barycentrics, approximate reconstruction and center/radius/circle constructions, and stable Heron evaluation. [P009 contracts and evidence](../03-Geometry/92-triangle.md) records the feature map, numerical domains, independent arbitrary-precision oracles and completed verification. The suite uses the shared quick/full/stress driver and is discovered automatically; full/stress include optimized NDEBUG, checked assertion probes and ASan/UBSan configurations.

## P010 graph foundations

Six entries under `04-Graphs` cover graph representations/input adapters, iterative traversal and witnesses, topological sorting, canonical DSU integration, exact shortest paths and spanning forests/reconstruction trees. [P010 contracts and evidence](../04-Graphs/90-foundations.md) gives domains, proofs, source/legacy accounting, feature maps and verification results. Independent checks include exhaustive colorings/orders/forest subsets, Floyd reachability/distances/bottlenecks and threshold connectivity. All entries expose quick/full/stress modes and deterministic seeds and are included in shared quick discovery; full/stress add ASan/UBSan. Benchmarks compare list/CSR storage and dense/sparse graph algorithms with setup measured separately.

## P011 graph decomposition and functional graphs

Five entries under `04-Graphs` cover forest LCA (binary lifting, linear Euler RMQ and offline Tarjan), SCC/condensation, lowlink/biconnectivity/strong orientation, directed/undirected Euler trails, and partial functional graphs with ordered aggregates. [P011 evidence](../04-Graphs/95-p011.md) links the per-header contracts, proofs, source/legacy accounting and feature maps. Independent oracles include ancestor walks, Floyd mutual reachability, deletion/subset connectivity, edge-subset trail DP, exhaustive partial successor maps, simultaneous pair-state walks, ordered strings/matrices and a separate 64-level fold reference. All five entries expose quick/full/stress modes and are discovered by the shared runner. Full includes optimized NDEBUG, checked precondition probes and ASan/UBSan; large fixtures use 200,000 vertices. Recorded benchmarks compare binary/Euler LCA and Tarjan/Kosaraju SCC with separate LCA setup/query measurements.

## P012 mathematics foundations

Six entries under `05-Mathematics` cover fixed-width arithmetic, binary/unimodal searches, linear equations, ordinary sieves, combinatorics and segmented sieves. [P012 contracts and evidence](../05-Mathematics/96-p012.md) links each feature map, proof/domain contract, source/legacy accounting and verification command. Independent oracles include Python exact arithmetic, matrix exponentiation, enumerated arrangements, bounded equations, trial prime factors/divisors and analytic search positions. All entries expose quick/full/stress modes and are discovered by the shared runner; quick discovery also passed from `/tmp`.

Full mode passed optimized NDEBUG, checked and ASan/UBSan configurations for every header, with 93 assertion probes plus a narrow-type compile rejection. Leak checking remained enabled in approved runs outside the sandbox's process tracer. Integration and existing modular/transform consumers passed scalar and AVX2 checks. The [sieve benchmark](05-Mathematics/90-foundations_benchmark.py) and [record](05-Mathematics/90-foundations_benchmark.json) compare dense sieve alternatives and segmented plain/odd/wheel modes, including reusable setup, interval/block sizes, output checks and explicit memory costs. These observations impose no universal timing gate. The segmented constructor's `[L,R)` migration and widened divisor sums are documented in the package evidence.

## P013 miscellaneous foundations

Seven entries under `06-Miscellaneous` cover reproducible bounded randomness and shuffling, compound hashing, buffered I/O, sorted/encounter coordinate compression, the canonical Mathematics search facade, scalar bit operations and permutation algebra/ranking. [P013 evidence](../06-Miscellaneous/95-p013.md) links per-header contracts, correctness arguments, source/legacy accounting, feature maps and commands. Independent oracles include exact Python arithmetic, exhaustive reduced-word rejection balance, set-position bit successors, ordered-container and interval references, recursive lexicographic enumeration, and FILE fault injection.

All seven expose quick/full/stress modes and are discovered by the shared runner, including quick execution from `/tmp`. Full verification uses optimized NDEBUG, checked precondition probes and ASan/UBSan with leak checking. The [fast I/O benchmark](06-Miscellaneous/90-fastio_benchmark.py) and [record](06-Miscellaneous/90-fastio_benchmark.json) compare complete parse/transform/format/flush pipelines across buffer sizes and input distributions, verify every output byte, and record environment/medians without a universal timing gate. Advanced hashing/I/O, general enumeration and sequence algorithms retain their separate owners.

## P014 sequence, interval, offline and sorting coverage

Four entries under `06-Miscellaneous` cover exact maximum subarrays/subrectangles, LIS/count/weighted witnesses, inversions/windows/verified heavy hitters; mixed-endpoint interval unions/sweeps/stabbing/scheduling; stable offline threshold sweeps and range counts; and explicit integer counting/radix sorts plus standard partition/selection adapters. [P014 contracts and evidence](../06-Miscellaneous/96-p014.md) records every API, domain, correctness argument, source read scope, legacy disposition and future-extension handoff.

All four passed full optimized NDEBUG, checked and ASan/UBSan verification with leak checking enabled, 35 assertion probes and six sorting compile-domain rejections. Independent references enumerate subintervals/rectangles/subsequences, interval subsets and set covers, direct threshold scans and ordered callbacks, and stable record/order-statistic oracles. Final shared-runner quick discovery passed from `/tmp`; standalone/aggregate/multiple-TU integration and repository consistency passed. The evidence states debug-STL fixture-size limits, sanitizer environment retries and unexecuted stress/compiler configurations explicitly.

The [sorting benchmark](06-Miscellaneous/11-sorting_selection_benchmark.py) and [record](06-Miscellaneous/11-sorting_selection_benchmark.json) contain 132 workload/method comparisons with setup/copy/allocation costs, complete output/stability validation, medians/raw samples and historical source hashes. They record both specialized-sort gains and small/presorted regressions; standard algorithms remain the default, without a universal timing gate or implicit cutoff.

## P015 enumeration, knapsack and cycle finding

Three entries under `06-Miscellaneous` cover callback combination/subset/product/Gray traversal, mixed-multiplicity capacity/value knapsack with independent feasibility/counts and witnesses, and budgeted generic Floyd/Brent recovery. [P015 contracts and evidence](../06-Miscellaneous/89-p015.md) records public domains, proofs, per-feature maps, source/legacy scope and Advanced handoffs.

All three passed full optimized NDEBUG, checked and ASan/UBSan verification with leak checking enabled. Enumeration/knapsack include 53 asserted-precondition probes in total; cycle budgets include zero and have no numeric invalid-input probes. Independent references recursively enumerate mathematical objects/multiplicity vectors, use Python arbitrary integers for exact counts, and map first visits for orbit structure. Cycle call counts also satisfy independent closed forms. All suites expose quick/full/stress modes and are discovered by the shared runner from arbitrary working directories.

The [knapsack benchmark](06-Miscellaneous/13-knapsack_benchmark.json) validates every exact result and reconstructed witness against direct multiplicity recurrence. The [cycle benchmark](06-Miscellaneous/14-cyclefinding_benchmark.json) compares complete Floyd/Brent recovery on known tail/period shapes, checking calls and expensive-callback checksums. Both record raw repetitions, medians, setup/memory costs, environment and source hashes; neither imposes a timing gate or claims universal speed superiority.

## P016 string foundations

Six entries under `07-Strings` cover prefix/KMP streaming and automata, Z/extended KMP and exact representation validation/conversion, double-prime/unsigned-word fingerprints, byte-multiset tries, Manacher radii/reconstruction and checked run-length encoding/decoding/spans. [P016 contracts and evidence](../07-Strings/94-p016.md) links the per-header feature maps, proofs, inspected sources, legacy accounting and future-owner boundaries.

All six passed optimized NDEBUG, checked and ASan/UBSan verification with leak checking enabled; 43 asserted-precondition probes passed. Independent references enumerate substrings and alphabet partitions, use Python exact polynomial sums and direct LCP/LCS scans, maintain ordered dictionaries, enumerate palindromes, and expand/measure encodings with wider integer arithmetic. Prefix/Z stress was executed; the other four suites completed full mode. Every entry provides quick/full/stress modes and shared-runner discovery from arbitrary working directories.

The [hash benchmark](07-Strings/03-stringhash_benchmark.json) records construction, query and complete pipeline costs against direct scans, small/large/random/periodic/unary workloads, verified answers, raw repetitions/medians, memory, environment and source hashes. Fingerprints retain their stated collision limits, including an explicit unsigned-word collision regression; timing or finite agreement does not establish exact equality.

## P017 Aho, suffix arrays and palindrome queries

Three entries under `07-Strings` cover static byte Aho-Corasick with sparse/dense transitions, output/count/forbidden-language adapters; radix-doubling suffix arrays, Kasai, optional RMQ/LCE, pattern ranges and repeated/common substring witnesses; and exact/probabilistic palindrome queries plus leftmost longest extraction. [P017 contracts and evidence](../07-Strings/89-p017.md) links proofs, source/legacy accounting, per-feature maps and commands.

All three passed full optimized NDEBUG, checked and ASan/UBSan verification with leak checking enabled; 23 assertion probes passed. Independent oracles scan every pattern at every boundary, enumerate forbidden-language words, sort suffixes naively, enumerate LCP/repeated/common witnesses, and compare palindrome intervals symmetrically. A known word-hash collision is an expected false positive, while exact Manacher rejects it. Aho's memory-release regression and nested callback queries are covered. All nine Strings entries passed shared quick discovery from `/tmp`; standalone/aggregate/multiple-TU integration passed.

The [Aho benchmark record](07-Strings/06-aho_benchmark.jsonl) compares construction and count-only scans across small/random/full-byte/nested dictionaries, independently verifies outputs, and records environment, medians and extra dense-table allocation. Dense scans improve on the measured workloads, with an explicit setup/memory tradeoff; there is no timing gate or automatic representation threshold.

## Coverage backlog after the completeness audit

Every implementation batch owns its per-header/module feature-to-test map and tests. Adding an inventory row does not add a suite, and a support batch is not permission to defer algorithm tests. The expanded scope in [the audit](../00-Guidelines/16-inventory-audit.md) requires these additional coverage classes as the corresponding implementations land:

| Family | Planned evidence beyond ordinary examples |
|---|---|
| Integer, modular, exact algebra and FPS | Independent big-integer/rational oracles; min/max signed values, wide intermediates, invalid inverses/division, nonfields and zero divisors, truncation/transform boundaries, modulus/cache changes and aliasing. Full/mini agreement supplements an independent oracle. |
| Numeric and geometric methods | Exact predicates checked independently from approximate constructions; collinear/coplanar/tangent/duplicate inputs, cancellation and scale variation; justified residual/error bounds, convergence failures and separate exact/approximate contracts. |
| Persistent, rollback and dynamic structures | Branching-version snapshots, undo/replay histories, failed merges, noncommutative action order, changing roots, link/cut legality and structural/aggregate invariants; test adversarial shapes and state reset. |
| Graph optimization and decomposition | Independently validate witnesses and optimality certificates; enumerate tiny cuts/matchings/paths, infeasible demands, negative cycles, zero capacities, multiedges and disconnected cases. Scaling-flow feasibility/termination and shortest-walk/simple-path distinctions need dedicated regressions. |
| Strings, combinatorics and randomized methods | Empty/full-byte/repetitive strings, alphabet/terminator policies, exhaustive small sets and reconstruction; reproducible seeds across several distributions. Monte Carlo probability claims need an argument as well as tests; a shared hash or transform is not an independent oracle. |
| Python and specialized Core paths | CPython/PyPy behavior, arbitrary-precision semantics and import side effects; scalar versus each supported compiled ISA path, thresholds/alignment, explicit unsupported-hardware skips and common full/mini domains. No ISA-specific implementations are required outside full Core. |

Infrastructure gaps remain **planned**: recursive suite discovery when numbered topic/package nesting is introduced (the current runner scans one folder level), missing per-file feature suites and maps, a broader independent-oracle/benchmark corpus, and checked-in CI. No GitHub Actions, GitLab CI or CircleCI configuration was found in this audit. A future CI matrix should separate quick checks, full correctness/sanitizers and extended stress/benchmarks; a machine without an ISA/interpreter must report a skip rather than claim that path passed. Benchmark records must include the environment and complete workload, with no universal timing gate across hosts.
