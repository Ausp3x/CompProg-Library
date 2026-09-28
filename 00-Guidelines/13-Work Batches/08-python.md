# 08-Python implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../08-Python/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| PY01 | `_01_io.py`, `_02_search.py`, `_03_number_theory.py`, `_04_combinatorics.py`, `_05_sequences.py` | M | None | Python I/O, search, number theory, combinatorics and sequences. PY28 owns CRT/floor sums/Lucas; numerical roots use built-ins where appropriate. |
| PY02 | `_06_dsu.py`, `_07_fenwick.py`, `_08_segmenttree.py` | M | PY01 | Python DSU, Fenwick and segment tree. |
| PY03 | `_09_graph.py` | L | PY01 | Python graph algorithms with iterative traversals, bipartite coloring/odd-cycle certificates and path/cycle witnesses. |
| PY04 | `_14_prime_factor.py` | L | PY01 | Python primality/factorization with explicit bounded deterministic domains and seeded randomized retries. |
| PY05 | `_17_tree.py` | L | PY01, PY02, PY03 | Python tree algorithms: LCA/HLD/virtual tree/rerooting; PY02–PY03. Rollback and persistence are separately owned. |
| PY06 | `_20_polynomial.py` | L | PY01 | Python polynomial/convolution and measured practical FPS subset; separate Matrix and int-bitset owners. |
| PY07 | `_23_trie.py`, `_24_aho.py`, `_25_suffix.py` | L | PY01 | Tries, Aho, suffix array/LCP and suffix automaton; longest common substring returns matching spans. PY19 owns LCS subsequence DP. |
| PY08 | `_26_offline.py`, `_27_randomized.py` | M | PY01 | Offline and randomized algorithms. |
| PY09 | `_44_exact_geometry.py` | L | PY01 | Fraction-based exact geometry and growth limits. |
| PY10 | `_47_algebraic.py`, `_48_combinatorial_species.py` | L | PY01 | Algebraic recurrences and combinatorial species. |
| PY11 | `_28_io_advanced.py` | M | PY01 | Advanced low-allocation I/O with interactive safeguards. |
| PY12 | `_29_lazysegmenttree.py` | L | PY01, PY02 | Python lazy segment tree: monoid actions, composition and searches; PY02. DP optimizations separately owned. |
| PY13 | `_45_succinct.py` | L | PY01 | Python rank/select and packed storage. |
| PY14 | `_46_fmindex.py` | L | PY01, PY07, PY13 | FM-index and sampled locate reuse suffix/rank helpers. |
| PY15 | `_49_sat.py` | L | PY01 | Python SAT/exact cover solver after concrete use case. |
| PY16 | `_50_approximation.py` | M | PY01 | Approximation algorithms with error estimates. |
| PY17 | `_10_strings.py` | M | PY01 | Python Basic strings: KMP/Z/Manacher/minimum rotation/simple hash. Suffix array belongs to PY07. |
| PY18 | `_11_geometry.py` | M | PY01 | Python exact integer geometry and Fraction intersections. |
| PY19 | `_12_dp.py` | M | PY01 | Python Basic DP: knapsack, sequence/grid recurrences, LCS length/witness and Levenshtein alignment with practical memory tradeoffs. Interval/game and advanced state designs belong to PY33. |
| PY20 | `_13_sparsetable.py` | S | PY01 | sparsetable. Complete all selected inventory contracts and variants. PY01 |
| PY21 | `_31_ordered_multiset.py` | L | PY01, PY02 | ordered multiset. Complete all selected inventory contracts and variants. PY02 |
| PY22 | `_32_line_envelope.py` | L | PY01 | line envelope. Complete all selected inventory contracts and variants. PY01 |
| PY23 | `_33_twosat.py` | M | PY01, PY03 | twosat. Complete all selected inventory contracts and variants. PY03 |
| PY24 | `_34_lowlink.py` | L | PY01, PY03 | lowlink. Complete all selected inventory contracts and variants. PY03 |
| PY25 | `_35_eulertrail.py` | S | PY01, PY03 | eulertrail. Complete all selected inventory contracts and variants. PY03 |
| PY26 | `_36_shortest_paths.py` | L | PY01, PY03 | shortest paths. Complete all selected inventory contracts and variants. PY03 |
| PY27 | `_37_mincostflow.py` | L | PY01, PY26, PY34 | mincostflow. Complete all selected inventory contracts and variants. Flow/residual API from PY34; initialize valid potentials for the selected negative-cost domain. |
| PY28 | `_38_number_theory_advanced.py` | L | PY01, PY04 | number theory advanced. Complete all selected inventory contracts and variants. PY01; PY04 |
| PY29 | `_39_transform_algorithms.py` | M | PY01 | transform algorithms. Complete all selected inventory contracts and variants. PY01 |
| PY30 | `_40_state_search.py` | L | PY01, PY03, PY19 | state search. Complete all selected inventory contracts and variants. PY03; PY19 |
| PY31 | `_41_geometry_float.py` | L | PY01, PY18 | geometry float. Complete all selected inventory contracts and variants. PY18 |
| PY32 | `_42_functionalgraph.py` | M | PY01, PY03 | functionalgraph. Complete all selected inventory contracts and variants. PY03 |
| PY33 | `_43_dp_advanced.py` | L | PY01, PY19, PY38 | dp advanced. Complete all selected inventory contracts and variants. Reuse PY38 native-int bitset helpers where selected; polynomial/FPS is not a generic advanced-DP prerequisite. |
| PY34 | `_15_flow.py` | L | PY01 | flow. Complete all selected inventory contracts and variants. |
| PY35 | `_16_matching.py` | L | PY01 | matching. Complete all selected inventory contracts and variants. |
| PY36 | `_18_rollback.py`, `_19_persistent.py` | L | PY01 | rollback; persistent. Complete all selected inventory contracts and variants. |
| PY37 | `_21_matrix.py` | L | PY01 | matrix. Complete all selected inventory contracts and variants. |
| PY38 | `_22_bitset.py` | M | PY01 | bitset. Complete all selected inventory contracts and variants. |
| PY39 | `_30_dp_optimization.py` | L | PY01, PY22 | dp optimization. Complete all selected inventory contracts and variants. |
