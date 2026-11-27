# 06 Miscellaneous — Sources

Rows `03` (fast I/O) and planned `17` (advanced fast I/O) moved to `01-Core/19-fastio.hpp` on 2026-11-01 (P018). The `03` uses below are historical; the fast I/O sources continue in [01-Core/Docs/00-sources.md](../../01-Core/Docs/00-sources.md).

## Catalog Pages Fetched on 2026-10-06

| Source | URL | Fetched | Used for |
|---|---|---|---|
| Library Checker problem repository (category directories) | https://github.com/yosupo06/library-checker-problems | 2026-10-06 | Category list; the judge home page itself renders only client-side and listed no problems |
| Library Checker `sample` | https://github.com/yosupo06/library-checker-problems/tree/master/sample | 2026-10-06 | `aplusb`, `many_aplusb`, `many_aplusb_128bit` confirm `03` 128-bit integer I/O |
| Library Checker `other` | https://github.com/yosupo06/library-checker-problems/tree/master/other | 2026-10-06 | `longest_increasing_subsequence` (`08`); linear recurrences and 2-SAT are owned by Mathematics and Graphs |
| Library Checker `enumerative_combinatorics` | https://github.com/yosupo06/library-checker-problems/tree/master/enumerative_combinatorics | 2026-10-06 | `sharp_p_subset_sum` kept as the FPS-count lead in `29`; `number_of_subsequences` not adopted (see below) |
| cp-algorithms index | https://cp-algorithms.com/ | 2026-10-06 | DP, combinatorics, numerical, miscellaneous and schedules article lists: knapsack, LIS, broken profile, bracket sequences, 15-puzzle, MEX, scheduling, annealing, tortoise-hare |
| cp-algorithms balanced bracket sequences | https://cp-algorithms.com/combinatorics/bracket_sequences.html | 2026-10-06 | Operations of `53`: balance check, count, next, all, rank, kth, multiple bracket types |
| cp-algorithms scheduling on one machine | https://cp-algorithms.com/schedules/schedule_one_machine.html | 2026-10-06 | `39` penalty orderings and the Livshits–Kladov limitation |
| cp-algorithms scheduling on two machines | https://cp-algorithms.com/schedules/schedule_two_machines.html | 2026-10-06 | `39` Johnson's rule |
| cp-algorithms deadlines and durations | https://cp-algorithms.com/schedules/schedule-with-completion-duration.html | 2026-10-06 | `39` heap-based deadline schedule |
| cp-algorithms 15 puzzle | https://cp-algorithms.com/others/15-puzzle.html | 2026-10-06 | `35` parity solvability criterion |
| KACTL `content/various` | https://github.com/kth-competitive-programming/kactl/tree/main/content/various | 2026-10-06 | BumpAllocator/SmallPtr (`23`), FastInput (`03`), FastKnapsack (`29`), IntervalCover (`09`), LIS, TernarySearch (Mathematics); IntervalContainer is Data Structures `24` |
| KACTL `content/combinatorial` | https://github.com/kth-competitive-programming/kactl/tree/main/content/combinatorial | 2026-10-06 | IntPerm (permutation rank/unrank, `07`); multinomial is Mathematics |
| OI Wiki 杂项 (misc) | https://oi-wiki.org/misc/ | 2026-10-06 | Discretization, two pointers, offline, CDQ, parallel binary search, Mo variants, fractional programming, randomization, hill climbing/annealing, expression evaluation, job scheduling, majority, Garsia–Wachs, 15-puzzle, Josephus, Kahan summation |
| OI Wiki 动态规划 (dp) | https://oi-wiki.org/dp/ | 2026-10-06 | Knapsack basic/advanced, interval DP, state compression, digit DP, plug DP, dynamic DP, slope trick, WQS |
| OI Wiki 背包 DP 进阶 | https://oi-wiki.org/dp/knapsack/advanced/ | 2026-10-06 | `29`: generalized items, group knapsack, undo knapsack, witness output, optimal-solution counts, kth-best, mixed, two-dimensional cost, dependency (tree) knapsack |
| OI Wiki 区间 DP | https://oi-wiki.org/dp/interval/ | 2026-10-06 | `52`: merging stones (linear and circular), necklace/matrix instances |
| OI Wiki 分数规划 | https://oi-wiki.org/misc/frac-programming/ | 2026-10-06 | `55`: binary search and Dinkelbach, 0-1 selection, ratio MST and mean-cycle applications |
| OI Wiki 基础 / 搜索 | https://oi-wiki.org/basic/ | 2026-10-06 | Sorting family (`11`), prefix/difference, binary search/doubling (Mathematics/Graphs), DFS/BFS, bidirectional, A*/IDA*, backtracking, dancing links, alpha-beta |
| Nyaan library index | https://nyaannyaan.github.io/library/ | 2026-10-06 | misc: base64 (`28`), compress, fastio, interval-union, mo, rng, timer (`49`), vector-pool (`23`); dp: branch-and-bound, knapsack01, LIS, inversion counting, maximal rectangle (Data Structures); hashmap (Data Structures `61`) |
| maspypy `other` | https://github.com/maspypy/library/tree/main/other | 2026-10-06 | timer, dice (`51`), knight_distance (`51`), mex (`08`), io_mmap (`17`), sliding_puzzle_solver (`35`), connected_dp (`33`), reduce_intervals (`09`), exp/fibonacci/golden search (Mathematics), optimize_1d_beam (`38`) |
| maspypy `knapsack` | https://github.com/maspypy/library/tree/main/knapsack | 2026-10-06 | knapsack01, branch-and-bound, subset_sum, subset_sum_count (`29`/`22`/`30`) |
| maspypy `random` | https://github.com/maspypy/library/tree/main/random | 2026-10-06 | hash_pair/hash_vector (`19`), shuffle (`01`), random_bracket (`53`), random_real (`01` randDouble); random graph/polygon/matrix/Monge generators are contest-testing generators |
| maspypy `seq` | https://github.com/maspypy/library/tree/main/seq | 2026-10-06 | cycle_decomposition, kth_next_permutation, factorial_digit_system (`07`), inversion and LIS (`08`); recurrences/tableaux are Mathematics |
| maspypy index (offline_query, slope_trick, enumerate, game) | https://maspypy.github.io/library/ | 2026-10-06 | Slope trick variants (`34`), enumerate/partition and product (`12`), parallel_binary_search (`20`), mo/rollback_mo (Data Structures), game directory (Mathematics) |
| ei1333 library index | https://ei1333.github.io/library/ | 2026-10-06 | other: compress, dice, connected-grid-states, timer, random, scanner/printer, vector-pool; dp: knapsack variants, LIS, monotone minima/SMAWK (Mathematics); structure/others: slope-trick, generalized-slope-trick |
| suisen library index | https://suisen-cp.github.io/cp-library-cpp/ | 2026-10-06 | util: coordinate_compressor, permutation, subset_iterator, timer, zobrist_hash, object_pool, rot90/transpose/grid_utils, search; dp: grid_connectivity; algorithm: horn_sat (`26`), inversion_number; io streams |
| hitonanode library index | https://hitonanode.github.io/cplib-cpp/ | 2026-10-06 | other_algorithms: bounded_knapsack, hilbert_order_mos (2D/3D), slope_trick, dual_slope_trick, LIS; random: custom_hash, xorshift, shuffle; utilities: next_combination, timer, reader/writer, rotate90/transpose, run_length_encoding (Strings) |
| tko919 library index | https://tko919.github.io/library/ | 2026-10-06 | Utility: fastio, random, timer; Algorithm: mo, rollbackmo, monge/monotone minima (Mathematics), project selection (Graphs); DataStructure slopetrick |
| noshi91 library index | https://noshi91.github.io/Library/ | 2026-10-06 | algorithm: Axiotis–Tzamos (unbounded) knapsack (`29` small-weight lead), SMAWK/LARSCH (Mathematics); other: bit width/popcount/select64, fast_ios, random_integer |
| Algorithmica HPC | https://en.algorithmica.org/hpc/ | 2026-10-06 | External memory (external sorting, list ranking, cache-oblivious), binary search/Eytzinger and static B-trees (`44`), SIMD and arithmetic chapters (Core profile) |
| AtCoder Library documentation | https://atcoder.github.io/ac-library/production/document_en/ | 2026-10-06 | No miscellaneous component; all twelve components are owned by other folders |

Status-cell keys are the library's (`00-Guidelines/09-sources.md`, Source Keys).

## P013 Re-Audit Sweep (2026-10-07)

Fetched by `@researcher` for rows `01`–`07`; per-header use is recorded in the evidence documents.

| Source | URL | Used for |
|---|---|---|
| Nyaan rng / fastio / gray-code / enumerate-set | https://nyaannyaan.github.io/library/misc/rng.hpp, https://raw.githubusercontent.com/NyaanNyaan/library/master/misc/fastio.hpp, https://nyaannyaan.github.io/library/math/gray-code.hpp, https://nyaannyaan.github.io/library/set-function/enumerate-set.hpp | `01` randDouble(l, r); `03` variadic read/print; `06` Gray code and superset step |
| maspypy random_real / io / index_compression / kth_next_permutation / factorial_digit_system | https://raw.githubusercontent.com/maspypy/library/main/random/random_real.hpp, https://raw.githubusercontent.com/maspypy/library/main/other/io.hpp, https://maspypy.github.io/library/ds/index_compression.hpp, https://raw.githubusercontent.com/maspypy/library/main/seq/kth_next_permutation.hpp, https://raw.githubusercontent.com/maspypy/library/main/seq/factorial_digit_system.hpp | `01` real draws; `03` print precision default; `04` stableRanks; `07` kthNextPermutation |
| hitonanode rand_nondeterministic / reader | https://hitonanode.github.io/cplib-cpp/random/rand_nondeterministic.hpp, https://hitonanode.github.io/cplib-cpp/utilities/reader.hpp | `01` URBG interface; `03` |
| OI Wiki random / discretization / bit | https://oi-wiki.org/misc/random/, https://oi-wiki.org/misc/discrete/, https://oi-wiki.org/math/bit/ | `01` URBG; `04` stableRanks; `06` parity |
| KACTL HashMap.h / FastInput.h | https://github.com/kth-competitive-programming/kactl/blob/main/content/data-structures/HashMap.h, https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/various/FastInput.h | `02` safe_gp_hash_table; `03` |
| yosupo fastio, ei1333 scanner/printer/compress | https://raw.githubusercontent.com/yosupo06/yosupo-library/main/src/yosupo/fastio.hpp, https://ei1333.github.io/library/other/scanner.hpp, https://ei1333.github.io/library/other/printer.hpp, https://ei1333.github.io/library/other/compress.hpp | `03` range read/write and end bytes; `04` bulk ranks |
| suisen coordinate_compressor / bit_utils / permutation | https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/bit_utils.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/permutation.hpp | `04`, `06` parity, `07` |
| cp-algorithms Gray code / bit manipulation | https://cp-algorithms.com/algebra/gray-code.html, https://cp-algorithms.com/algebra/bit-manipulation.html | `06` |

## P014 Re-Audit Sweep (2026-10-08)

Fetched by `@researcher` for rows `08`–`11`. Adoption decisions are in the Limits and Handoffs sections of [08-sequence_algorithms.md](08-sequence_algorithms.md), [09-interval_algorithms.md](09-interval_algorithms.md), [10-offline_queries.md](10-offline_queries.md) and [11-sorting_selection.md](11-sorting_selection.md), with reasons in [00-notes.md](00-notes.md).

| Source | URL | Used for |
|---|---|---|
| CSES problem set | https://cses.fi/problemset/ | `08` Increasing Subsequence II (`countIncreasingSubsequences`), Towers (dual cover of `increasingSubsequenceLengths`); `09` Nested Ranges Check/Count (`removeNestedIntervals`, `nestedIntervalCounts`) |
| cp-algorithms LIS | https://cp-algorithms.com/dynamic_programming/longest_increasing_subsequence.html | `08` per-index lengths and the minimum non-increasing cover |
| maspypy `seq/inversion.hpp`, `seq/longest_increasing_subsequence.hpp` | https://github.com/maspypy/library/blob/main/seq/inversion.hpp, https://github.com/maspypy/library/blob/main/seq/longest_increasing_subsequence.hpp | `08` `adjacentSwapDistance` (inversion_between), LIS dp array; the all-range inversion table is not adopted |
| KACTL `content/various` (IntervalCover.h, ConstantIntervals.h) | https://github.com/kth-competitive-programming/kactl/tree/main/content/various | `09` `intervalCover`; ConstantIntervals not adopted |
| OI Wiki 快速排序 | https://oi-wiki.org/basic/quick-sort/ | `11` three-way partition and linear-time selection |
| maspypy offline set intersection, ei1333 offline RMQ, hitonanode offline range kth / sum of linear | https://maspypy.github.io/library/ds/offline_query/offline_set_intersection.hpp, https://ei1333.github.io/library/other/offline-rmq.hpp, https://hitonanode.github.io/cplib-cpp/data_structure/range_kth_smallest_offline.hpp, https://hitonanode.github.io/cplib-cpp/utilities/offline_sum_of_linear.hpp | `10` candidates, owned elsewhere (folder notes); offline set intersection was adopted on 2026-11-08 into row `22` as `setIntersectionOffline` |

## P015 Re-Audit Sweep (2026-10-08)

Fetched by `@researcher` for rows `12`–`14`. Adoption decisions are in [00-notes.md](00-notes.md) and the Limits and Handoffs sections of [12-enumeration.md](12-enumeration.md), [13-knapsack.md](13-knapsack.md) and [14-cyclefinding.md](14-cyclefinding.md). OI Wiki combinatorics and cycle-detection pages failed (403/404).

| Source | URL | Used for |
|---|---|---|
| maspypy `enumerate` (partition, bits, multiset, xor_range, labeled_tree) | https://maspypy.github.io/library/enumerate/partition.hpp, https://maspypy.github.io/library/enumerate/bits.hpp, https://maspypy.github.io/library/enumerate/multiset.hpp | `12` partition order and part cap, set-partition practicality; compositions not adopted; xor ranges adopted on 2026-11-08 into `06` (`xorRangeBlocks`) |
| suisen `util/subset_iterator` | https://suisen-cp.github.io/cp-library-cpp/library/util/subset_iterator.hpp | `12` `forEachSetBit`, `forEachCombinationSubmask` |
| Nyaan `set-function/enumerate-set` | https://nyaannyaan.github.io/library/set-function/enumerate-set.hpp | `12` `forEachSupermask` |
| hitonanode `number/enumerate_partitions` | https://hitonanode.github.io/cplib-cpp/number/enumerate_partitions.hpp | `12` alternative partition interface (all `m <= N`), not adopted |
| cp-algorithms combinations, submasks | https://cp-algorithms.com/combinatorics/generating_combinations.html, https://cp-algorithms.com/algebra/all-submasks.html | `12` `forEachGrayCombination` (revolving door), `forEachSubmaskPair` |
| KACTL FastKnapsack, maspypy `knapsack/subset_sum`, Nyaan `dp/knapsack01` | https://github.com/kth-competitive-programming/kactl/blob/main/content/various/FastKnapsack.h, https://maspypy.github.io/library/knapsack/subset_sum.hpp, https://nyaannyaan.github.io/library/dp/knapsack01.hpp | `13` Pisinger subset sum and adaptive 0/1 dispatch, handed to `29` |
| CSES problem set (1634, 1635) | https://cses.fi/problemset/ | `13` `minimumCoins`, `compositionCountTable` (ordered ways) |
| Wikipedia cycle detection, Nivasch stack algorithm, cp-algorithms tortoise and hare | https://en.wikipedia.org/wiki/Cycle_detection, https://www.gabrielnivasch.org/fun/cycle-detection, https://cp-algorithms.com/others/tortoise_and_hare.html | `14` algorithm survey; hashing, Gosper, stack and trade-off methods not adopted |
| oraclesqlpuzzle library index §032 | https://oraclesqlpuzzle.ninja-web.net/kyoupro/02-library/index.html | `14` `orbitTerm` (ABC167 D, ABC179 E, ABC258 E, Typical90 058 uses) |

## P229 Sweep (2026-10-08)

Fetched by `@researcher` for rows `49`–`51`; per-header use is recorded in the evidence documents.

| Source | URL | Used for |
|---|---|---|
| Nyaan / ei1333 / suisen / hitonanode / tko919 timers | https://nyaannyaan.github.io/library/misc/timer.hpp, https://ei1333.github.io/library/other/chrono-timer.hpp, https://ei1333.github.io/library/other/timer.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/timer.hpp, https://hitonanode.github.io/cplib-cpp/utilities/timer.hpp, https://tko919.github.io/library/Utility/timer.hpp | `49` reading surface, `Timer::measure`; `rdtsc` not adopted |
| shindannin AHC template, thun-c TimeKeeper | https://shindannin.hatenadiary.com/entry/2021/03/06/115415, https://qiita.com/thun-c/items/ecd438fde4d237b1f7bc | `49` progress; throttled check not adopted |
| CSES 3139 / 3305, AtCoder practice_2 | https://cses.fi/problemset/task/3139, https://cses.fi/problemset/task/3305, https://atcoder.jp/contests/language-test-202001/tasks/practice_2 | `50` prefix tokens, range answers, comparison replies |
| Codeforces blog 45307, problem 1999G1 (read 2026-11-22, [Source Check](#fetched-2026-11-22-source-check)) | https://codeforces.com/blog/entry/45307, https://codeforces.com/problemset/problem/1999/G1 | `50` flush, `-1` exit, per-test budget; a wrong answer surfaces as `-1` at the next read |
| maspypy dice / knight_distance / manhattan_walk, ei1333 dice | https://maspypy.github.io/library/other/dice.hpp, https://maspypy.github.io/library/other/knight_distance.hpp, https://maspypy.github.io/library/other/manhattan_walk.hpp, https://ei1333.github.io/library/other/dice.hpp | `51` orientations, canonical, knight formula; row `57` `manhattanWalk` |
| hitonanode rotate90 / multidim_index / grid_graph, suisen rot90 / grid_utils | https://hitonanode.github.io/cplib-cpp/utilities/rotate90.hpp, https://hitonanode.github.io/cplib-cpp/utilities/multidim_index.hpp, https://hitonanode.github.io/cplib-cpp/graph/grid_graph_template.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/rot90.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/grid_utils.hpp | `51` rotation direction, bounds and neighbours; multidim index and grid graph not adopted |

## P075 Sweep (2026-10-13)

Fetched by `@researcher` for row `34`. Adoption decisions are in [34-slope_trick.md](34-slope_trick.md) and the omissions in [00-notes.md](00-notes.md).

| Source | URL | Used for |
|---|---|---|
| hitonanode slope_trick / dual_slope_trick (doc, raw source, stress test) | https://hitonanode.github.io/cplib-cpp/other_algorithms/slope_trick.hpp, https://hitonanode.github.io/cplib-cpp/other_algorithms/dual_slope_trick.hpp, https://raw.githubusercontent.com/hitonanode/cplib-cpp/master/other_algorithms/dual_slope_trick.hpp, https://raw.githubusercontent.com/hitonanode/cplib-cpp/master/other_algorithms/slope_trick.md, https://hitonanode.github.io/cplib-cpp/other_algorithms/test/slope_trick_stress.test.cpp | Unit operations and `merge`; `DualSlopeTrick` operations (`get_at_zero`, `shift`, `add_linear`, `add_linear_or_zero`, `slide_min`) and their real `O(\|d\| log n)` costs |
| ei1333 slope-trick / generalized-slope-trick | https://ei1333.github.io/library/structure/others/slope-trick.hpp, https://ei1333.github.io/library/structure/others/generalized-slope-trick.hpp | Unit API with `shift(a, b)`; BST variant with key sums (non-destructive `get`) |
| maspypy slope_trick_0 / slope_trick_1 / slope_trick_super, slope trick articles (1) and (3) | https://maspypy.github.io/library/convex/slope_trick/slope_trick_0.hpp, https://maspypy.github.io/library/convex/slope_trick/slope_trick_1.hpp, https://maspypy.github.io/library/convex/slope_trick/slope_trick_super.hpp, https://maspypy.com/slope-trick-1-解説編, https://maspypy.com/slope-trick-3-slope-trick-の凸共役 | `add_linear`, `restrict_domain`, infimal convolution (`convolve`, `convolve_segment`), conjugate duality behind `DualSlopeTrick` |
| Nyaan slope-trick / slope-trick-weighted | https://nyaannyaan.github.io/library/data-structure/slope-trick.hpp.html, https://nyaannyaan.github.io/library/data-structure/slope-trick-weighted.hpp.html | Weighted `add_xma`/`add_amx`/`add_abs` and the `(w, w*x)`-sum BST evaluation |
| suisen slope_trick, tko919 slopetrick | https://suisen-cp.github.io/cp-library-cpp/library/datastructure/slope_trick.hpp, https://tko919.github.io/library/DataStructure/slopetrick.hpp | Aliases (`cumulative_min_left`, `translate`, `sliding_window_minimum`); `lines()` iteration (not adopted) |
| OI Wiki Slope Trick | https://oi-wiki.org/dp/opt/slope-trick/ | Infimal convolution of convex functions, mergeable-heap merge, problem list (CF 713C, APIO 2016 Fireworks, ABC 217 H, ARC 070 E, ARC 123 D) |
| USACO Guide slope trick | https://usaco.guide/adv/slope-trick | Fireworks and tree-merge applications |
| yukicoder 2114, 1077, 1467 | https://yukicoder.me/problems/no/2114, https://yukicoder.me/problems/no/1077, https://yukicoder.me/problems/no/1467 | Verification problems named by HIT, MAS and NYA (statements only) |

KACTL and cp-algorithms have no slope-trick article. maspy's article (2), zscoder's Codeforces tutorial (blog 47821) and ei1333's documentation page returned HTTP 404 or 403 and were not read.

## P072 Sweep (2026-10-23)

Fetched by `@researcher` for rows `29`, `54` and `55`. Adoption decisions are in [00-notes.md](00-notes.md) and the evidence documents [29-knapsack_advanced.md](29-knapsack_advanced.md), [54-kbest_enumeration.md](54-kbest_enumeration.md) and [55-fractional_programming.md](55-fractional_programming.md).

| Source | URL | Used for |
|---|---|---|
| OI Wiki 背包 DP (mirror) and 背包 DP 进阶 | https://next.oi-wiki.org/dp/knapsack/, https://oi-wiki.org/dp/knapsack/advanced/ | `29` group, two-dimensional, dependency, undo (counting only), kth-best (strict, HDU 2639) and optimal-count variants |
| cp-algorithms knapsack | https://cp-algorithms.com/dynamic_programming/knapsack.html | `29` monotone-queue bounded knapsack |
| KACTL FastKnapsack | https://github.com/kth-competitive-programming/kactl/blob/main/content/various/FastKnapsack.h | `29` `pisingerSubsetSum` |
| maspypy `knapsack/` (knapsack01, knapsack_branch_bound, subset_sum, subset_sum_count, ancestor_closed_set_knapsack, connected_set_knapsack, independent_set_knapsack), `mod/modular_subset_sum` | https://maspypy.github.io/library/ | `29` branch and bound, Pisinger witness, tree knapsack, `connectedSetKnapsack`, `modularSubsetSum`; row `57` `independentSetKnapsack` |
| Nyaan `dp/knapsack01`, `dp/branch-and-bound` | https://nyaannyaan.github.io/library/ | `29` adaptive 0/1 dispatch (not adopted), branch and bound |
| ei1333 `dp/knapsack`, `dp/knapsack-limitations`, `dp/knapsack-limitations-2` | https://ei1333.github.io/library/ | `29` monotone queue; `boundedKnapsackSmallValue` (AOJ DPL_1_I) |
| hitonanode `other_algorithms/bounded_knapsack` | https://hitonanode.github.io/cplib-cpp/ | `29` Pisinger bounded form |
| noshi91 `algorithm/axiotis_tzamos_knapsack`, `axiotis_tzamos_unbounded_knapsack` | https://noshi91.github.io/Library/ | `29` `smallWeightKnapsack` (concave max-plus per weight class), `unboundedKnapsackLargeCapacity` |
| Library Checker problem repository | https://github.com/yosupo06/library-checker-problems | `sharp_p_subset_sum` is Mathematics `12` `subsetSumCount`; `29` keeps only the multiplicity form |
| maspypy `convex/nth_element_from_sorted_lists`, `convex/nth_element_from_sorted_matrix` | https://maspypy.github.io/library/convex/nth_element_from_sorted_lists.hpp, https://maspypy.github.io/library/convex/nth_element_from_sorted_matrix.hpp | `54` `kSmallestSortedMatrix`, `kSmallestSortedLists` |
| OI Wiki 堆, 可持久化可并堆 | https://oi-wiki.org/ds/heap/, https://oi-wiki.org/ds/persistent-heap/ | `54` frontier heaps; persistent-heap k-best paths are Graphs `32` |
| cnblogs autoint (NOI 2010 超级钢琴) | https://www.cnblogs.com/autoint/p/10671934.html | `54` `kSmallestSubarraySums` range split with an argmin table |
| Qiita tsukasa\_\_diary (Lawler k-best) | https://qiita.com/tsukasa__diary/items/8bc2947d1f65d1451bb3 | `54` `lawlerKBest` |
| algo.monster 2386 | https://algo.monster/liteproblems/2386 | `54` k-th largest subsequence sum as `kSmallestSubsetSums` under `std::greater` |
| Eppstein–Hirschberg, Choosing subsets with maximum weighted average (J. Algorithms 24, 1997; UCI TR 95-12) | https://ics.uci.edu/~eppstein/pubs/EppHir-Algs-97.pdf | `55` engine of `zeroOneFractionalSelect` (Lemmas 1, 4–6, Figure 1); located 2026-10-24 by the `/finish` audit, pages 1–12 read 2026-10-24 |
| OI Wiki 分数规划 | https://oi-wiki.org/misc/frac-programming/ | `55` Dinkelbach, exactly-k selection, ratio applications |
| USACO 2018 Open Gold "Talent Show" solution, USACO forum thread 212 | https://usaco.org/current/data/sol_talent_gold_open18.html, https://forum.usaco.guide/t/talent-show-fake-solve/212 | `55` `ratioKnapsack`; the search-free variant is not adopted |
| cp-algorithms binary search, Nyaan Stern–Brocot binary search, maspypy `nt/rational_binary_search` | https://cp-algorithms.com/num_methods/binary_search.html, https://nyaannyaan.github.io/library/math/stern-brocot-tree-binary-search.hpp.html, https://maspypy.github.io/library/nt/rational_binary_search.hpp | `55` real bisection and exact rational search, owned by Mathematics `02` and `20` |
| Wikipedia parametric search, linear-fractional programming, fractional programming | https://en.wikipedia.org/wiki/Parametric_search, https://en.wikipedia.org/wiki/Linear-fractional_programming, https://en.wikipedia.org/wiki/Fractional_programming | `55` `linearFractionalProgramming` (Charnes–Cooper); Megiddo not adopted |

Codeforces blogs 61211, 146040 and 4190 (HTTP 403), the Luogu CCO 2020 "Shopping Plans" article (HTTP 523), and the poj.org and usaco.guide fractional-programming pages (unreachable) were not read. `kSmallestFixedSizeSubsetSums` is derived and proved in its evidence document, not taken from them.

## Fetched 2026-11-08 (Omission Sweep)

Scope `02`–`07` (P254). Decisions are in the rows, the folder notes (Omission Sweep 2026-11-08) and `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded Decisions).

| URL | Used for |
|---|---|
| https://maspypy.github.io/library/enumerate/xor_range.hpp | 06: `xorRangeBlocks` |
| https://hitonanode.github.io/cplib-cpp/utilities/xor_of_interval.hpp | 06: `xorRangeBlocks` |
| https://noshi91.github.io/Library/ | 06: `select` |
| https://noshi91.github.io/Library/other/select64.cpp | 06: `select` |
| https://oi-wiki.org/math/permutation/ | 07: `permutationRankMod` |
| https://www.luogu.com.cn/problem/P5367 | 07: `permutationRankMod` |
| https://cses.fi/problemset/ | 08: `maximumAverageSubarrayLengths`; 32: `binPackingDP`; 35: `nQueensCount`; 39: `kMachineIntervalSchedule` |
| https://cses.fi/problemset/task/3301 | 08: `maximumAverageSubarrayLengths` |
| https://cp-algorithms.com/schedules/schedule-with-completion-duration.html | 09: `intervalPartitionMachines` (left out; row 09 `intervalPartitionAssignment` owns it) |
| https://github.com/yosupo06/library-checker-problems/tree/master/data_structure | 10: `rangeSumOffline` |
| https://github.com/yosupo06/library-checker-problems/blob/master/data_structure/static_range_sum_with_upper_bound/task.md | 10: `rangeSumOffline` |
| https://oi-wiki.org/basic/bucket-sort/ | 11: `bucketSort` |
| https://oi-wiki.org/basic/sort-intro/ | 11: `bucketSort` |
| https://atcoder.jp/contests/language-test-202001/tasks/practice_2 | 11: `binaryInsertionSort`, `mergeInsertionSort` |
| https://cses.fi/problemset/task/3139 | 11: `binaryInsertionSort`, `mergeInsertionSort` |
| https://oi-wiki.org/basic/tournament-sort/ | 11: `binaryInsertionSort`, `mergeInsertionSort` |
| https://cp-algorithms.com/data_structures/sqrt_decomposition.html | 16: `blockMoOrder` |
| https://oi-wiki.org/misc/parallel-binsearch/ | 20: `overallBinarySearch` |
| https://oi-wiki.org/misc/cdq-divide/ | 21: `cdqDivideAndConquer`, `longestChain3D` |
| https://ei1333.github.io/library/dp/online-offline-dp.hpp | 21: `cdqDivideAndConquer` |
| https://oi-wiki.org/lang/csl/bitset/ | 22: `bitsetDominanceCount` |
| https://maspypy.github.io/library/knapsack/subset_sum.hpp | 22: `bitsetSubsetSum` |
| https://maspypy.github.io/library/ds/offline_query/offline_set_intersection.hpp | 22: `setIntersectionOffline` |
| https://maspypy.github.io/library/mod/modular_subset_sum.hpp | 22: `bitsetModularSubsetSum` |
| https://oi-wiki.org/search/dlx/ | 25: `repeatedCover` |
| https://howardhinnant.github.io/date_algorithms.html | 27: `nthWeekdayOfMonth`, `lastWeekdayOfMonth`, `isoWeekFromCivil`, `civilFromIsoWeek`, `daysFromJulian`, `julianFromDays`, `dayOfWeek` |
| https://nyaannyaan.github.io/library/misc/base64.hpp | 28: `base64EncodeArray`, `base64DecodeArray` |
| https://nyaannyaan.github.io/library/ | 28: `base64EncodeArray`, `base64DecodeArray`; 38: `MultipointAnnealing`, `MultiArmedBandit`, `play` |
| https://maspypy.github.io/library/knapsack/knapsack01.hpp | 30: `knapsackMitm` |
| https://nyaannyaan.github.io/library/dp/knapsack01.hpp | 30: `knapsackMitm` |
| https://maspypy.github.io/library/knapsack/subset_sum_count.hpp | 30: `subsetSumCountBySize` |
| https://oi-wiki.org/math/numeral-sys/base/ | 31: `bijectiveBaseEncode`, `bijectiveBaseDecode`, `negativeBaseEncode`, `negativeBaseDecode`, `balancedTernaryEncode`, `balancedTernaryDecode` |
| https://cp-algorithms.com/algebra/balanced-ternary.html | 31: `balancedTernaryEncode`, `balancedTernaryDecode` |
| https://cses.fi/problemset/task/1653 | 32: `binPackingDP` |
| https://oi-wiki.org/dp/state/ | 33: `rowMaskDP` |
| https://oi-wiki.org/dp/plug/ | 33: `dominoTilingCount`, `brokenProfileTiling`, `PlugDP`, `bestCycle`, `ColoredGridStates`, `encode` |
| https://maspypy.github.io/library/other/connected_dp.hpp | 33: `PolygonGridStates` |
| https://oi-wiki.org/search/astar/ | 35: `aStar` (left out; Graphs row 13 owns it) |
| https://oi-wiki.org/search/ | 35: `aStar` |
| https://maspypy.github.io/library/other/sliding_puzzle_solver.hpp | 35: `slidingPuzzleConstruct` |
| https://cses.fi/problemset/task/1624 | 35: `nQueensCount` |
| https://oi-wiki.org/misc/expression/ | 37: `recursiveDescent`, `Ast`, `toPrefix` |
| https://github.com/maspypy/library/blob/main/other/構文解析.txt | 37: `recursiveDescent` |
| https://nyaannyaan.github.io/library/marathon/sa-manager.hpp | 38: `MultipointAnnealing` |
| https://oi-wiki.org/misc/simulated-annealing/ | 38: `restartSearch`, `SimulatedAnnealing`, `schedule` |
| https://oi-wiki.org/misc/hill-climbing/ | 38: `restartSearch` |
| https://github.com/maspypy/library/tree/main/other | 38: `beamOptimize1D`, `beamOptimize2D` |
| https://github.com/maspypy/library/blob/main/other/optimize_1d_beam.hpp | 38: `beamOptimize1D`, `beamOptimize2D` |
| https://github.com/maspypy/library/blob/main/other/optimize_2d.hpp | 38: `beamOptimize1D`, `beamOptimize2D` |
| https://cp-algorithms.com/num_methods/simulated_annealing.html | 38: `SimulatedAnnealing`, `schedule` |
| https://nyaannyaan.github.io/library/marathon/simulated-annealing.hpp | 38: `SimulatedAnnealing`, `schedule` |
| https://nyaannyaan.github.io/library/marathon/multi-armed-bandit.hpp.html | 38: `MultiArmedBandit`, `play` |
| https://cses.fi/problemset/task/1632 | 39: `kMachineIntervalSchedule` |
| https://oi-wiki.org/basic/greedy/ | 39: `unitJobProfitSchedule` |
| https://en.wikipedia.org/wiki/Huffman_coding | 40: `huffmanCostSorted`, `lengthLimitedHuffman`, `huffmanCost` |
| https://researchwith.njit.edu/en/publications/finding-a-minimum-weight-k-link-path-in-graphs-with-monge-propert/ | 40: `lengthLimitedHuffman` |
| https://maspypy.github.io/library/ | 42: `RollbackArray`, `checkpoint`; 53: `bracketForest`, `spans`; 56: `greedyChange`, `greedyCounterexample` |
| https://maspypy.github.io/library/ds/rollback_array.hpp | 42: `RollbackArray`, `checkpoint` |
| https://maspypy.github.io/library/ds/segtree/rollback_lazy_segtree.hpp | 42: `RollbackArray`, `checkpoint` |
| https://maspypy.github.io/library/ds/node_pool.hpp | 43: `PersistentArena`, `clone` |
| https://suisen-cp.github.io/cp-library-cpp/library/datastructure/segment_tree/persistent_segment_tree.hpp | 43: `PersistentArena`, `clone` |
| https://en.algorithmica.org/hpc/external-memory/list-ranking/ | 44: `listRanking` |
| https://en.algorithmica.org/hpc/ | 44: `listRanking`, `beladyEviction`, `simulateEviction` |
| https://en.algorithmica.org/hpc/external-memory/sorting/ | 44: `sortMergeJoin` |
| https://en.algorithmica.org/hpc/external-memory/policies/ | 44: `LruCache`, `beladyEviction`, `simulateEviction` |
| https://oi-wiki.org/misc/rand-technique/ | 45: `MinHashDistinct`, `estimate`, `KmvSketch`, `estimate` |
| https://en.wikipedia.org/wiki/Streaming_algorithm | 45: `AmsSketch`, `secondMoment`, `KmvSketch`, `estimate` |
| https://en.wikipedia.org/wiki/K-independent_hashing | 45: `AmsSketch`, `secondMoment` |
| https://en.wikipedia.org/wiki/Color-coding | 47: `colorCoding`, `PerfectHashFamily`, `build` |
| https://en.wikipedia.org/wiki/Method_of_conditional_probabilities | 47: `pessimisticIndependentSet` |
| https://cplib.tifa-233.com/src/edh/garsia_wachs/lib.hpp | 48: `garsiaWachsQuadratic` |
| https://oi-wiki.org/misc/garsia-wachs/ | 48: `garsiaWachsQuadratic` |
| https://en.wikipedia.org/wiki/Optimal_binary_search_tree | 48: `approximateAlphabeticTree` |
| https://en.wikipedia.org/wiki/Matrix_chain_multiplication | 52: `huShing` |
| https://github.com/koosaga/olympiad/tree/master/Library/codes/ad_hoc | 52: `huShing` |
| https://oi-wiki.org/dp/interval/ | 52: `IntervalDP`, `solve` |
| https://cp-algorithms.com/combinatorics/bracket_sequences.html | 53: `rankBalanced`, `unrankBalanced`, `countBalanced` |
| https://oi-wiki.org/topic/bracket/ | 53: `rankBalanced`, `unrankBalanced`, `countBalanced` |
| https://cses.fi/problemset/task/2187 | 53: `countBalanced` |
| https://maspypy.github.io/library/graph/bracket_graph.hpp | 53: `bracketForest`, `spans` |
| https://maspypy.github.io/library/other/greedy_coin_counter_example.hpp | 56: `greedyChange`, `greedyCounterexample` |

## 2026-09 Audit

The 2026-09 audit read OI Wiki pages for knapsack, digit DP, state DP, plug DP, dynamic DP, slope trick, IDA*, alpha-beta, job ordering, expression parsing, majority candidates, Garsia–Wachs and annealing, and compared cp-algorithms, CSES, Library Checker, OI Wiki navigation and the Japanese library catalogs. The Garsia–Wachs page has conflicting complexity wording (the `48` contract in [00-notes.md](00-notes.md)). Per-package source inspections, with dates, page revisions and read scopes, are in the evidence documents [01-random.md](01-random.md), [02-customhash.md](02-customhash.md), [19-fastio.md](../../01-Core/Docs/19-fastio.md) (row `03` until 2026-11-01), [04-compression.md](04-compression.md), [06-bit_operations.md](06-bit_operations.md), [07-permutation.md](07-permutation.md), [08-sequence_algorithms.md](08-sequence_algorithms.md), [09-interval_algorithms.md](09-interval_algorithms.md), [10-offline_queries.md](10-offline_queries.md), [11-sorting_selection.md](11-sorting_selection.md), [12-enumeration.md](12-enumeration.md), [13-knapsack.md](13-knapsack.md) and [14-cyclefinding.md](14-cyclefinding.md); machine-readable timestamps are in [research-sources.json](../../00-Guidelines/Ledgers/research-sources.json). Code in every verified header was written independently from the mathematical recurrences; no external source text was adapted.

## Not Adopted

- Library Checker `number_of_subsequences` (distinct subsequence count): Strings rows `19`/`20` provide `countDistinctSubsequences`.
- Library Checker `find_linear_recurrence`, `kth_term_of_linearly_recurrent_sequence`, `sum_of_exponential_times_polynomial`: Mathematics `15-linear_recurrence.hpp` and generating functions.
- Library Checker `two_sat`: Graphs `18-twosat.hpp`.
- cp-algorithms RMQ, Josephus, Stern–Brocot, games on graphs, Sprague–Grundy: Data Structures and Mathematics owners.
- cp-algorithms largest zero submatrix; Nyaan/ei1333/suisen/tko919 maximal rectangle and maximum square: Data Structures `08-monotone_stack.hpp` (`maximalHistogramRectangles`, `maximalBinaryRectangles` and `largestBinarySquare`, adopted there on 2026-11-08).
- KACTL DivideAndConquerDP, KnuthDP, TernarySearch: Mathematics `27`/`02`. KACTL SIMD, Unrolling, FastMod: Core profile.
- OI Wiki Mo variants, 珂朵莉树, 悬线法: Data Structures. 有限状态自动机, 计算理论基础, 字节顺序, 空间优化简介: expository, no API. Kahan summation: Mathematics numerics.
- OI Wiki 倍增 (doubling) and Nyaan/hitonanode doubling/binary_lifting: Graphs `11-functionalgraph.hpp`.
- maspypy anulus_scheduling: problem-specific, no reusable family.
- maspypy exp_search, fibonacci_search, golden_search, binary_search_real_abs_rel: Mathematics `02-search_algorithms.hpp`.
- maspypy/ei1333/suisen/hitonanode monotone minima, SMAWK, LARSCH, Monge shortest path, min/max-plus convolutions: Mathematics `27`/`53`.
- maspypy random_graph/random_polygon/random_matrix/random_monge: test generators for `09-Contest Testing`.
- suisen project_selection_problem, k_project_selection_problem; hitonanode combinatorial_opt (matroids, simplex, assignment); tko919 projectselection: Graphs and Mathematics.
- hitonanode tsp (Held–Karp, LKH): Graphs `73-hamiltonian.hpp`. hitonanode run_length_encoding, suisen run_length_encoder: Strings `09`.
- Nyaan bitset-find-prev, noshi91 popcount/bit_width: Core `18-bitset` and `06` already cover scalar scans; noshi91 select64 was adopted on 2026-11-08 into row `06` as `BitOps::select`.
- Nyaan/ei1333 cumulative sums, 2D cumulative sums, suisen cumulative_sum: Data Structures `05-prefix_sum.hpp`.
- noshi91 monoid/action adapters: Data Structures segment-tree policies.
- Algorithmica SIMD argmin, prefix sum, matrix multiplication, Montgomery: Core full profile.

## Fetched 2026-11-10 (P275 Re-Review)

Pages the P275 re-review agents read to confirm the returns recorded in [00-notes.md](00-notes.md) (P275 Re-Review 2026-11-10).

- <https://raw.githubusercontent.com/maspypy/library/main/convex/slope_trick/slope_trick_super.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/knapsack/knapsack01.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/knapsack/connected_set_knapsack.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/mod/modular_subset_sum.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/convex/nth_element_from_sorted_matrix.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/other/dice.hpp>
- <https://raw.githubusercontent.com/maspypy/library/main/enumerate/unlabeled_tree.hpp>
- <https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/main/library/util/timer.hpp>
- <https://cp-algorithms.com/combinatorics/generating_combinations.html>
- <https://cses.fi/problemset/list/>
- <https://atcoder.jp/contests/abc155/tasks/abc155_d>
- <https://www.luogu.com.cn/problem/P5283>
- <https://en.wikipedia.org/wiki/Linear-fractional_programming>

## P013 MI02 Sweep (2026-11-16)

Pages the batch's `@researcher` sweeps read for rows `04`, `06` and `07`. Decisions are in `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded Decisions, 2026-11-16) and [00-notes.md](00-notes.md).

| URL | Used for |
|---|---|
| https://maspypy.github.io/library/ds/index_compression.hpp, https://nyaannyaan.github.io/library/misc/compress.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp, https://oi-wiki.org/misc/discrete/ | 04: nothing new (aliases only) |
| https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/categories.toml | 04, 07: no compression or permutation-rank problem |
| https://raw.githubusercontent.com/maspypy/library/main/setfunc/xor_submask_lower_bound.hpp | 06: `xorSubmaskLowerBound` |
| https://raw.githubusercontent.com/maspypy/library/main/my_template.hpp | 06: `bitReverse` (`bit_reverse`); aliases `topbit`, `lowbit`, `kth_bit` |
| https://maspypy.github.io/library/other/dyadic_intervals.hpp | 06: `xorRangeBlocks` with `x = 0` |
| https://cp-algorithms.com/algebra/bit-manipulation.html | 06: `clearLowBit`, `clearTrailingOnes`, `setLowestZero` |
| https://raw.githubusercontent.com/frederik-hoeft/bit-twiddling-hacks/main/README.md (mirror of Sean Eron Anderson's Bit Twiddling Hacks; the original's certificate was rejected) | 06: `rank`, `mortonEncode`/`mortonDecode`, `signExtend`, `mergeBits`, `swapBitRanges`, the byte tests (user decision 2026-11-16; not a sweep catalog) |
| https://raw.githubusercontent.com/NyaanNyaan/library/master/template/bitop.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/bit_utils.hpp, https://hitonanode.github.io/cplib-cpp/utilities/next_combination.hpp, https://oi-wiki.org/math/binary-set/ | 06: aliases only |
| https://codeforces.com/problemset/problem/1648/C (statement through a web search; the page returned HTTP 403) | 07: `multisetPermutationRankMod` (user decision 2026-11-16; not a sweep catalog) |
| https://cses.fi/problemset/task/3397, https://cses.fi/problemset/task/3398 | 07: aliases of `permutationRank`/`permutationUnrank` and `permutationOrderMod` |
| https://github.com/kth-competitive-programming/kactl/tree/main/content/combinatorial | 07: used-value bitmask technique (`IntPerm.h`) behind `permutationRank` |
| https://oi-wiki.org/math/permutation/ | 07: `distinctPermutationCount` (多重集的排列数); the cycle-type count, adopted into row 07 on 2026-11-17 (`cycleTypeCount`) |

## P013 MI02 `/finish` Sweep (2026-11-17)

Pages the three `@researcher` sweeps of the MI02 `/finish` fetched for rows `04`, `06`, `07` and `SortedVector`, plus the Trotter–Johnson page that session read. Decisions are in `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded Decisions, 2026-11-17). Indexes read in part are marked.

| URL | Used for |
|---|---|
| https://raw.githubusercontent.com/maspypy/library/main/ds/index_compression.hpp, https://raw.githubusercontent.com/maspypy/library/main/ds/to_small_key.hpp | 04: `compressCounting` and `stableRanksCounting` (`SMALL=true` builds), `HashedEncounterCompression` (`To_Small_Key`) |
| https://suisen-cp.github.io/cp-library-cpp/library/util/coordinate_compressor.hpp | 04 and `SortedVector`: `minGeq`, `maxLeq`, `minGt`, `maxLt` |
| https://raw.githubusercontent.com/maspypy/library/main/my_template.hpp | 04 `argsort`; 07 `permute` (`rearrange`) |
| https://nyaannyaan.github.io/library/misc/compress.hpp, https://ei1333.github.io/library/other/compress.hpp, https://oi-wiki.org/misc/discrete/, https://raw.githubusercontent.com/verngutz/cp-library/master/Data%20Structs%20-%20Coordinate%20Compression.cpp | 04: no 2D compression in any catalog (added by user decision as `compressPoints`); aliases |
| https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/misc/ordersort.py | 04: radix argsort, covered by `11-sorting_selection.hpp` `radixSort` with a projection |
| https://en.wikipedia.org/wiki/Ranking | 04: dense, ordinal and competition rankings map to `ranks`, `stableRanks` and `rank` (source key, not a sweep catalog) |
| https://cp-algorithms.com/algebra/bit-manipulation.html, https://cses.fi/problemset/task/1146 | 06: `countOnesUpTo` |
| https://oi-wiki.org/math/bit/ | 06: `countLeadingSignBits` (`__builtin_clrsb`); signed tricks dominated by `std::` |
| https://oi-wiki.org/math/binary-set/, https://oi-wiki.org/math/numeral-sys/gray-code/, https://raw.githubusercontent.com/atcoder/ac-library/master/atcoder/internal_bit.hpp, https://raw.githubusercontent.com/NyaanNyaan/library/master/template/bitop.hpp, https://suisen-cp.github.io/cp-library-cpp/library/util/bit_utils.hpp, https://raw.githubusercontent.com/koosaga/olympiad/master/Library/codes/tools/bithack.cpp, https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/misc/bit_hacks.py, https://raw.githubusercontent.com/verngutz/cp-library/master/Bitmasking.cpp | 06: aliases of existing members and `std::<bit>` coverage; nothing else missing |
| https://cses.fi/problemset/task/1698, https://cses.fi/problemset/task/2216, https://cses.fi/problemset/task/2214 | 07: `swapRoundSort`, `collectingNumbers`; 60: `permutationWithInversions` (Inverse Inversions) |
| https://cses.fi/problemset/task/3422, https://cses.fi/problemset/task/2229 | handoffs: Third Permutation to row 57 (P285), Mahonian numbers to P054 |
| https://raw.githubusercontent.com/verngutz/cp-library/master/Combinatorics%20-%20Multiset%20Permutations%20(fixed%20length).cpp, https://raw.githubusercontent.com/verngutz/cp-library/master/Brute%20Force%20-%20Paired%20Permutations.cpp | handoffs: multiset k-permutation count to P054, `forEachPairing` to P015 |
| https://suisen-cp.github.io/cp-library-cpp/library/util/permutation.hpp, https://docs.sympy.org/latest/modules/combinatorics/permutations.html | 07: Myrvold-Ruskey order (`PermutationHash`, `rank_nonlex`); row 59 Trotter-Johnson (`rank_trotterjohnson`, SymPy only) |
| https://en.wikipedia.org/wiki/Steinhaus%E2%80%93Johnson%E2%80%93Trotter_algorithm | 59: recursive definition, first and last permutations, Even's speedup (the tester's walk oracle) |
| https://raw.githubusercontent.com/maspypy/library/main/seq/cycle_decomposition.hpp, https://raw.githubusercontent.com/maspypy/library/main/seq/factorial_digit_system.hpp, https://raw.githubusercontent.com/maspypy/library/main/random/shuffle.hpp, https://raw.githubusercontent.com/kth-competitive-programming/kactl/main/content/combinatorial/chapter.tex, https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/combinatorics/combinatorics.py | 07: owned or covered (cycles, factorial digits, shuffle, derangements) |
| https://maspypy.github.io/library/, https://nyaannyaan.github.io/library/, https://hitonanode.github.io/cplib-cpp/, https://ei1333.github.io/library/, https://tko919.github.io/library/, https://noshi91.github.io/Library/, https://github.com/ecnerwala/cp-book/tree/master/src, https://github.com/koosaga/olympiad/tree/master/Library/codes, https://github.com/verngutz/cp-library | indexes (maspypy, Nyaan, hitonanode and ei1333 read in part) |

## P015 MI15 Sweep (2026-11-26)

Pages the batch's `@researcher` sweep read for row `12`; it found no missing operation. The interfaces it reports shaped the six new traversals, as [12-enumeration.md](12-enumeration.md#sources) records. The truncated indexes of hitonanode, ei1333 and suisen were read in part, OI Wiki's Gray code page returned no content, and ACL has no enumeration module.

| URL | Used for |
|---|---|
| https://raw.githubusercontent.com/maspypy/library/main/enumerate/labeled_tree.hpp, https://raw.githubusercontent.com/maspypy/library/main/enumerate/product.hpp, https://raw.githubusercontent.com/maspypy/library/main/graph/prufer_code.hpp | `12` `forEachLabeledTree`: Pruefer codes in lexicographic order through a product traversal, decoded by the linear pointer method |
| https://raw.githubusercontent.com/suisen-cp/cp-library-cpp/main/library/util/subset_iterator.hpp | `12` `forEachSetBit` (`all_setbit`), `forEachCombinationSubmask` (`all_subset_k`, which needs BMI2 `pdep`; the row uses a portable successor) |
| https://cp-algorithms.com/combinatorics/generating_combinations.html, https://doc.sagemath.org/html/en/reference/combinat/sage/combinat/gray_codes.html | `12` `forEachGrayCombination`: the weight-`k` restriction of the reflected Gray code and the `(removed, added)` interface |
| https://cp-algorithms.com/algebra/all-submasks.html | `12` `forEachSubmaskPair` (the `3^n` mask-submask loop) |
| https://raw.githubusercontent.com/verngutz/cp-library/master/Brute%20Force%20-%20Paired%20Permutations.cpp | `12` `forEachPairing` (pairings visited as a permutation, one middle element unpaired for odd `n`; the file's `static_assert` never holds, so it does not compile as written) |
| https://raw.githubusercontent.com/maspypy/library/main/enumerate/bits.hpp, https://raw.githubusercontent.com/maspypy/library/main/enumerate/partition.hpp, https://raw.githubusercontent.com/maspypy/library/main/enumerate/multiset.hpp, https://hitonanode.github.io/cplib-cpp/utilities/next_combination.hpp, https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/misc/bit_hacks.py, https://oi-wiki.org/basic/enumerate/, https://oi-wiki.org/math/binary-set/, https://github.com/kth-competitive-programming/kactl/tree/main/content/combinatorial | `12`: aliases of existing operations only |
| https://raw.githubusercontent.com/maspypy/library/main/enumerate/maximal_independent_set.hpp | Graphs: enumeration of all maximal independent sets, a lead for the Graphs sweep, not for row `12` |

Papers filed in `95-Resources/13-Papers` (local-only, backed up in the private repository, 2026-11-27): Bitner–Ehrlich–Reingold, placed by the user on 2026-11-26 for the loopless revolving door, and Ruskey–Williams, fetched by Claude on 2026-11-27 for cool-lex:

| Title | URL | File and SHA-256 | Read |
|---|---|---|---|
| J. R. Bitner, G. Ehrlich, E. M. Reingold, *Efficient generation of the binary reflected Gray code and its applications*, CACM 19(9) (1976) 517–521 | https://doi.org/10.1145/360336.360343 (from the citation; not confirmed against the publisher) | `95-Resources/13-Papers/50-binary_reflected_gray_code-bitner_1976.pdf`, `09ea74ed06935ba3c4d6f97ce8b381a365aa79a6457e078c69009a55de560e3e` | p. 520 Algorithm 2 and Fig. 2, text of pp. 519–521: `12` `forEachGrayCombination` above the dispatch threshold |
| F. Ruskey, A. Williams, *The coolest way to generate combinations*, Discrete Math. 309(17) (2009) 5305–5320 (fetched 2026-11-27 by Claude, not user-placed) | https://webhome.cs.uvic.ca/~ruskey/Publications/Coollex/coolestDMvanilla.pdf | `95-Resources/13-Papers/51-coolest_way_to_generate_combinations-ruskey_2009.pdf`, `c661caa27936d6fd2d1dd25b880fca9dbb612bda02c9d1624b8e3abeaa3df6a3` | §2.1–2.2 pp. 2–3 (the successor `σ` and the iterative definition), §3.2 pp. 8–9 (`Loopless(s,t)`), §3.3 pp. 10–11 (Lemmas 6–7, `Word(s,t)`), §4 pp. 11–12 (Theorem 2): `12` `forEachCombinationCoolLex`, `forEachCombinationMaskCoolLex`, `combinationRankCoolLex` |

## P015 MI16 Sweep (2026-11-26)

Pages the MI16 `@researcher` sweep of row `13` fetched. Decisions are in `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded Decisions, 2026-11-26). Indexes read in part are marked. The OI Wiki page https://oi-wiki.org/dp/knapsack/ returned an empty page, so its Markdown source was read instead. DMOJ Knapsack-3 and Knapsack-4 returned 403 to the sweep; the user supplied copies on 2026-11-26, listed below and in `95-Resources/99-sources.json` (local-only, git-ignored).

| URL | Used for |
|---|---|
| https://cses.fi/problemset/ (tasks 1093, 1633–1636, 1745, 1158) | `13` `equalPartitionCount` (1093), `minimumCoins` (1634) and `compositionCountTable` (1633, 1635) semantics; aliases |
| https://cp-algorithms.com/dynamic_programming/knapsack.html | `13` `signedSumCount` (its practice list's target-sum task); copy expansion and naive complete knapsack, dominated |
| https://next.oi-wiki.org/dp/knapsack/, https://raw.githubusercontent.com/OI-wiki/OI-wiki/master/docs/dp/knapsack/basic.md | `13` variants already covered |
| https://atcoder.jp/contests/dp/tasks, https://atcoder.jp/contests/dp/tasks/dp_x | `39` `lawlerMoore` (EDPC X, Tower); EDPC D, E, M aliases of `13` |
| https://github.com/kth-competitive-programming/kactl/tree/main/content/various, https://atcoder.github.io/ac-library/production/document_en/, https://github.com/maspypy/library/tree/main/knapsack, https://github.com/cheran-senthil/PyRival/tree/master/pyrival | nothing new for `13` |
| https://nyaannyaan.github.io/library/, https://ei1333.github.io/library/, https://suisen-cp.github.io/cp-library-cpp/, https://hitonanode.github.io/cplib-cpp/ (each read in part) | nothing new for `13`; ei1333 `knapsack-01-2` is `ValueKnapsackDP` |

| Source and local copy | SHA-256 | Read | Finding |
|---|---|---|---|
| DMOJ, "Knapsack 3", https://dmoj.ca/problem/knapsack; `95-Resources/14-Web Pages/29-dmoj_knapsack-knapsack_3.pdf` | `abe9788543ddda626d7d4befbb55816d62053bd85ea68565415212c5414dba77` | pp. 1–2 (statement, constraints, sample) | Bounded knapsack, one of M capacities at most 5000 with a fixed cost each: a bounded `KnapsackDP` and `atMost(c_i) - f_i`, an alias of row `13`; row `29` `boundedKnapsackMonotoneQueue` is the O(N * W) form. |
| DMOJ, "Knapsack 4" (2020 Multi-University Training Contest 4, Problem C, Rope Pulling), https://dmoj.ca/problem/knapsack4; `95-Resources/14-Web Pages/29-dmoj_knapsack4-knapsack_4.pdf` | `366546cc331552d2408bfe6b2eec3ad1dfe5b92b3bf0f528c21fabcd575242c9` | pp. 1–2 (statement, constraints, sample) | Maximum signed value of two teams with equal total strength (weights at most 1000, sum of n + m at most 10^4): two exact `KnapsackDP` tables cost O((n + m) * S) with S up to 10^6; the random-order windowed DP goes to row `29` as planned `zeroSumKnapsack` (user decision 2026-11-26). |

## Fetched 2026-11-22 (Source Check)

The user supplied these copies on 2026-11-22. They are personal study copies, kept local and git-ignored under `95-Resources/13-Papers/` and `95-Resources/14-Web Pages/`; the manifest `95-Resources/99-sources.json` lists them. Page numbers are the PDF's, with the printed ones in parentheses.

| Source and local copy | SHA-256 | Read | Finding |
|---|---|---|---|
| D. E. Knuth, *The Art of Computer Programming*, vol. 4A pre-fascicles (2005), https://www-cs-faculty.stanford.edu/~knuth/taocp.html; `13-Papers/24-art_of_computer_programming_vol4_prefascicles-knuth_2005.pdf`. The file holds the pre-fascicles 2B, 3A and 3B (3A's copyright page: "Zeroth printing (revision 11), 31 March 2005"), not the 2011 book, and its text layer is unreadable, so the pages were rendered as images. | `1846455c61cd6af4ded38e3657b7bcda2e841715e83f1d8c71f71fef12624e0e` | p. 596 (2B p. 4): §7.2.1.2 Algorithm P; pp. 662–664 (3A pp. 4–6): §7.2.1.3 Algorithms L and T, Theorem L, eqs. (19)–(20); pp. 750–751 (3B pp. 26–27): §7.2.1.5 eq. (4) and Algorithm H; pp. 666–667 (3A pp. 8–9), read 2026-11-26: §7.2.1.3 eqs. (25)–(30) and Algorithm R; pp. 726–727 (3B pp. 2–3), read 2026-11-27: §7.2.1.4 Algorithm H and its cost statement | Confirms row 59 `forEachTrotterJohnson` (Algorithm P), row 12 `forEachSetPartition` (Algorithm H) and row 12 `forEachGrayCombination` (Algorithm R). Theorem L ranks combinations in another order than `combinationRank`. Details in [12-enumeration.md](12-enumeration.md#sources) and [59-permutation_orders.md](59-permutation_orders.md#sources). |
| M. Mirzayanov, "Interactive Problems: Guide for Participants", Codeforces blog 45307, https://codeforces.com/blog/entry/45307; `14-Web Pages/26-codeforces_blog_45307-interactive_problems_guide.pdf` | `9a9a625824409e8cd515339e49ec7ae2df45b7b939cf5b22cc66eae649d0c048` | pp. 1–3 (the post); the comments only by search | Row 50: output may sit in a buffer, and the judge's reply can be read only after a flush, so flush after every output; `endl` flushes. |
| Codeforces 1999G1, "Ruler (easy version)", https://codeforces.com/problemset/problem/1999/G1; `14-Web Pages/26-codeforces_1999g1-ruler_easy_version.pdf` | `0bac9577218775b74df7c9a1d8b92e18496e38a67b00cab7e1557e8299538236` | pp. 1–2 (statement, interaction) | Row 50: at most 10 queries per test case; an extra or invalid query, or a wrong answer, is answered `-1`, after which the program must terminate at once; flush after each query. |

## Fetched 2026-11-27 (Omission Sweep)

Candidates of the 2026-11-27 catalog survey, applied the same day; the catalog pages they came from are listed in `00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md` (Sources), and the decisions are in the rows, the folder notes (Omission Sweep 2026-11-27) and `00-Guidelines/23-Sweep Ledgers/omissions.md` (Recorded Decisions).

| URL | Used for |
|---|---|
| https://raw.githubusercontent.com/kopricky/My-Algorithm/master/DataStructure_Advanced/LIS2D.hpp | `LongestChain3DOnline` |
| https://raw.githubusercontent.com/kopricky/My-Algorithm/master/Competitive_Programming/DataStructure_Advanced/LIS2D.hpp | `LongestChain3DOnline` |
| https://raw.githubusercontent.com/cp-algorithms/cp-algorithms-aux/main/cp-algo/structures/stack_union.hpp | `StackUnion` |
| https://github.com/TheAlgorithms/C-Plus-Plus/tree/master/ciphers | repository file list for the candidate check |
| https://github.com/TheAlgorithms/C-Plus-Plus/tree/master/hashing | repository file list for the candidate check |
| https://github.com/ngthanhtrung23/ACM_Notebook_new/tree/master/Misc | repository file list for the candidate check |
| https://raw.githubusercontent.com/ngthanhtrung23/ACM_Notebook_new/master/Misc/Rubik.h | `RubiksCube` |
| https://github.com/ShahjalalShohag/code-library/tree/main/Dynamic%20Programming%20Optimizations | repository file list for the candidate check |
| https://raw.githubusercontent.com/ShahjalalShohag/code-library/main/Dynamic%20Programming%20Optimizations/Connected%20Component%20DP.cpp | `ComponentDP`, `skyscraperCount` |
| https://anqooqie.github.io/library/ | repository file list for the candidate check |
| https://anqooqie.github.io/proconlib/ | repository file list for the candidate check |
| https://anqooqie.github.io/proconlib/tools/kth_smallest_by_oracle.hpp | candidate check |
| https://anqooqie.github.io/proconlib/tools/digit_product_frequency.hpp | candidate check |
| https://github.com/justiceHui/icpc-teamnote | `diversifiedLateAcceptanceSearch` |

## Fetched 2026-11-28 (Omission Sweep)

The sources the user placed for the catalog survey, sampled on 2026-11-28 (`00-Guidelines/23-Sweep Ledgers/Docs/00-catalog-survey.md`, Sampled and Applied 2026-11-28); the decisions are in the rows, the package notes and follow-ups, and the folder notes (Omission Sweep 2026-11-28). Items counts the candidate decisions of this folder citing the key.

| Key | Source | URL | Filed Copy | Items |
|---|---|---|---|---|
| ALGOWIKI | MIT Algorithm Wiki, problem families index, saved page | https://algorithm-wiki.org/families | `95-Resources/14-Web Pages/50-algorithm_wiki-index.pdf` | 25 |
| BOJ | solved.ac tag list, 8 saved pages | https://solved.ac/problems/tags | `95-Resources/14-Web Pages/43-solved_ac-tags1.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags2.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags3.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags4.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags5.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags6.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags7.pdf`, `95-Resources/14-Web Pages/43-solved_ac-tags8.pdf` | 1 |
| CLRS | Cormen, Leiserson, Rivest, Stein, Introduction to Algorithms, 4th ed. (2022) | https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/ | `95-Resources/13-Papers/72-introduction_to_algorithms-cormen_2022.pdf` | 7 |
| CRANDALL | Crandall, Pomerance, Prime Numbers: A Computational Perspective, 2nd ed. (2005) | https://link.springer.com/book/10.1007/0-387-28979-8 | `95-Resources/13-Papers/69-prime_numbers-crandall_2005.pdf` | 7 |
| EOA | Kao (ed.), Encyclopedia of Algorithms, 2nd ed. (2016) | https://link.springer.com/referencework/10.1007/978-1-4939-2864-4 | `95-Resources/13-Papers/58-encyclopedia_of_algorithms-kao_2016.pdf` | 65 |
| GUSFIELD | Gusfield, Algorithms on Strings, Trees, and Sequences (1997) | https://doi.org/10.1017/CBO9780511574931 | `95-Resources/13-Papers/62-algorithms_on_strings_trees_sequences-gusfield_1997.pdf` | 2 |
| HACKERS | Warren, Hacker's Delight, 2nd ed. (2012) | https://www.informit.com/store/hackers-delight-9780321842688 | `95-Resources/13-Papers/59-hackers_delight-warren_2012.pdf` | 19 |
| LEETCODE | LeetCode problem list with topic tags (4,124 problems, 175 topics), exported | https://leetcode.com/problemset/ | `95-Resources/14-Web Pages/46-leetcode-topic_tags.csv` | 1 |
| MEHTASAHNI | Mehta, Sahni (eds.), Handbook of Data Structures and Applications, 2nd ed. (2018) | https://www.routledge.com/Handbook-of-Data-Structures-and-Applications/Mehta-Sahni/p/book/9780367572006 | `95-Resources/13-Papers/64-handbook_of_data_structures-mehta_2018.pdf` | 16 |
| NAVARRO | Navarro, Compact Data Structures: A Practical Approach (2016) | https://doi.org/10.1017/CBO9781316588284 | `95-Resources/13-Papers/67-compact_data_structures-navarro_2016.pdf` | 4 |
| QOJ | QOJ template problems (tag 模板题), saved page | https://qoj.ac/problems?tag=%E6%A8%A1%E6%9D%BF%E9%A2%98 | `95-Resources/14-Web Pages/44-qoj-template_problems.pdf` | 1 |
| TAOCP | Knuth, The Art of Computer Programming, Volume 4A: Combinatorial Algorithms, Part 1 (2011) | https://www-cs-faculty.stanford.edu/~knuth/taocp.html | `95-Resources/13-Papers/65-taocp_4a-knuth_2011.pdf` | 46 |
| TAOCP | Knuth, The Art of Computer Programming, Volume 4B: Combinatorial Algorithms, Part 2 (2022) | https://www-cs-faculty.stanford.edu/~knuth/taocp.html | `95-Resources/13-Papers/66-taocp_4b-knuth_2022.pdf` | 46 |
| YUKI | yukicoder tag statistics, saved page (logged in) | https://yukicoder.me/statistics/tags | `95-Resources/14-Web Pages/47-yukicoder-tags.pdf` | 3 |
