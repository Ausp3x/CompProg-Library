# 027-cartesiantree.hpp — Evidence

Package P033 (batch DS13). Rewritten from the legacy reference `OLD/Team Notebook/src/ds/cartesiantree.cpp`. An algorithm header of free functions with a result struct; preconditions are asserted.

## Contracts

### CartesianTree

The result struct holds `root` (-1 for an empty array), `parent`, `leftChild` and `rightChild` (-1 when absent), and `range[i] = [l, r)`, the index interval of the subtree of `i`, which is the maximal interval where `a[i]` is the tie-broken minimum. The in-order traversal is `0, 1, ..., n - 1`. The data is public; mutating it voids the `lcaRmqBridge` precondition.

### cartesianTree(a, comp = less<T>())

`n <= INT_MAX`. One left-to-right stack pass: element `i` pops every stack element `j` with `comp(a[i], a[j])`. The last popped element becomes its left child and the remaining stack top its parent; a popped `j` gets `range[j].second = i`, and `range[i].first` is the remaining stack top plus 1. Each index is pushed and popped once, so the pass makes O(n) comparisons.

- `comp` a strict weak order: a min-tree for `less`, a max-tree for `greater`; among equal elements the leftmost is the ancestor (an equal later element is not popped), so the root of `[l, r)` is the leftmost argmin, matching row 54 `LinearRMQ` and Graphs `EulerLCA`.
- `comp` the reflexive closure of a strict weak order (`less_equal`, `greater_equal`): an equal later element pops the earlier one, so the rightmost equal element is the ancestor. This is the strict order on the keys `(a[i], -i)`.
- The tree is unique for each rule because it is the Cartesian tree of distinct keys `(a[i], ±i)`.

### lcaRmqBridge(t)

`n < 2^30`, since the tour has `2n - 1` entries. Returns `(tour, depth, first)`: the Euler tour of `t` from the root (each vertex is written on entry and after each child returns), the depth of each tour entry (consecutive depths differ by exactly 1), and the first tour position of each vertex. For vertices `u` and `v`, the minimum depth on positions `[min(first[u], first[v]), max(...)]` is attained only at `lca(u, v)`. With `t = cartesianTree(a)`, `lca(l, r - 1)` is the tie-broken argmin of `a[l, r)`, so the bridge reduces range minimum to ±1 range minimum (Farach-Colton and Bender), which row 54 `PlusMinusOneRMQ` or any RMQ structure answers. The walk follows parent pointers iteratively in O(n); an empty tree gives three empty vectors. `duplicates.md` keeps this reduction by decision as an LCA–RMQ reduction, not a rename of `LinearRMQ::query` or `EulerLCA`; the research sweep found the row name vague, so this contract fixes its result.

### Violated Preconditions

The size limits are out of reach in memory tests (`2^30` vertices need more than 8 GB of output), so they have no assertion probes. A `CartesianTree` mutated into a non-tree may make `lcaRmqBridge` read out of range, a caller contract like every public data member in this folder; the walk still stops after `2n` steps.

### Legacy Mapping

| `OLD/Team Notebook/src/ds/cartesiantree.cpp` | Replacement |
|---|---|
| `CartesianTree<T, Comp>` constructor (stack build, `par`, `lc`, `rc`, `root`) | `cartesianTree(a, comp)` returning `parent`, `leftChild`, `rightChild`, `root` (same stack algorithm and tie rule) |
| Stored reference `a`, `n`, `comp` | Not adopted: the result owns no reference to the input, so it stays valid after the array changes |

## Feature-to-Test Map

Tester `96-Local Testing/02-Data Structures/027-cartesiantree_tester.cpp`, entry `027-cartesiantree_tester.py` (driver `_00_contest_runner.py`; three builds; no assertion probes, see Violated Preconditions).

| Operation | Group | Oracle |
|---|---|---|
| `cartesianTree` `parent`, `leftChild`, `rightChild`, `root`, `range` (less, less_equal, greater, greater_equal) | `exhaustive` (every array of length <= 8 over {0, 1, 2}; <= 9 in stress) | Recursive definition: the root of `[l, r)` is its leftmost (or rightmost) minimum, children are the roots of the two sides, `range` is the recursion interval |
| Same for other value types and comparators | `randomSmall` (`int` with random magnitudes, `string`, `double`, an absolute-value comparator) | Same recursive brute force |
| Large and deep inputs (increasing, decreasing, constant, four values, zigzag, uniform; n = 200000 full, 500000 stress) | `large` / `properties` | Heap order under the comparator, the tie rule between equal parent and child, a single root, child links, in-order equal to `0..n-1` (these determine the tree), subtree intervals computed bottom-up from the children |
| `lcaRmqBridge` | `checkBridge` from `exhaustive`, `randomSmall` (every `(l, r)`) and `large` (200 sampled short ranges) | Tour length `2n - 1`, ±1 depth steps, tour edges are tree edges, root at both ends, depths agree with parent depth + 1, `first` equals the first occurrence, and the minimum-depth vertex between `first[l]` and `first[r - 1]` equals both the brute-force argmin of `a[l, r)` and the lca found by walking parents |
| Empty and single arrays, the leftmost and rightmost ties on a sample | `edges` | Direct values |

A hand mutation check on 2026-10-14 (quick mode, temporary copies) detected 5 of 5 injected faults: the tie rule flipped, the `range` left end off by one, the root taken from the stack top, `first` overwritten on every visit, and the bridge skipping a right child after its left child.

## Commands and Results

Library-wide runs of 2026-11-10 (P274) on an Intel Core i9-11900H, GNU++20, seed 42, beside the other suites, with the build cache emptied before each cold run. Each passed; the times are this tester's compile + run.

| Command (2026-11-10) | Result |
|---|---|
| `01-run.py --mode full` (GCC 16.2.1) | Pre-flight quick peak 735 MB; optimized 0.2 + 3.6 s, checked 14.9 + 7.5 s, ASan-UBSan 19.9 + 17.5 s; MEMORY peak 892 MB |
| `CXX=g++-14 01-run.py --mode full` (GCC 14.4.1) | Pre-flight quick peak 790 MB; optimized 0.1 + 3.0 s, checked 16.9 + 7.9 s, ASan-UBSan 24.1 + 29.3 s; MEMORY peak 950 MB |
| `01-run.py --mode quick` | Optimized 26.5 + 0.2 s, checked 15.9 + 2.1 s; MEMORY peak 737 MB |
| `01-run.py --mode stress --rounds 1` | Pre-flight quick peak 741 MB; optimized 0.1 + 4.0 s, checked 5.9 + 9.6 s, ASan-UBSan 8.0 + 30.5 s; MEMORY peak 714 MB |

Latest per-header run 2026-10-14, after the review fixes, on the same machine with CPython 3.14, in the optimized, checked and ASan/UBSan builds. MEMORY peaks are dominated by compilation.

| Command (2026-10-14) | Result |
|---|---|
| `027-cartesiantree_tester.py --mode full --seed 20261014` (GCC 16.2.1) | PASS, 70,258,594 checks per configuration, MEMORY peak 479 MB |
| `CXX=g++-14 027-cartesiantree_tester.py --mode full --seed 20261014` (GCC 14.4.1) | PASS, 70,258,594 checks per configuration, MEMORY peak 516 MB |
| `01-run.py --mode stress --seed 7 --rounds 1` | PASS, 3 configurations, 181,013,001 checks per configuration, MEMORY peak 710 MB |
| `02-integration.py --sanitizers` | PASS: 143 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; MEMORY peak 2758 MB |
| `03-consistency.py` | No errors |

```bash
python3 '96-Local Testing/02-Data Structures/027-cartesiantree_tester.py' --mode full --seed 20261014
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/027-cartesiantree_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/27' --no-integration
python3 '96-Local Testing/02-integration.py' --sanitizers
python3 '96-Local Testing/03-consistency.py'
```


## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `tree/cartesian_tree` task statement | Distinct values, parent array with the root as its own parent (the library uses -1). |
| suisen `datastructure/cartesian_tree.hpp`, maspypy `seq/cartesian_tree.hpp`, Nyaan `tree/cartesian-tree.hpp`, ei1333 `graph/others/cartesian-tree.hpp`, hitonanode `tree/cartesian_tree.hpp`, OI Wiki Cartesian tree | Subtree ranges, min/max selection, the leftmost (suisen, maspypy, Nyaan, hitonanode, OI Wiki) and rightmost (ei1333) tie rules. |
| cp-algorithms, Linear RMQ and Farach-Colton–Bender LCA | RMQ to LCA through the Cartesian tree, LCA to ±1 RMQ through the Euler tour. |

Fetched 2026-10-14 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)); the implementation was written from the contract.

## Limits and Handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p033-omissions): weighted subarray counts by minimum, depth or traversal arrays.
- Row 08 owns nearest smaller values and the histogram rectangle, row 19 the linear treap build, row 54 linear and ±1 RMQ, row 55 the permutation tree, Graphs `007-lca.hpp` `EulerLCA`.

## Aliases

Search aliases (P263, grab --find): 笛卡尔树, RMQ, 笛卡儿树, 欧拉序.

## History

- 2026-10-14: P033 first implementation and verification; the review's 1 finding was fixed; full (seed 20261014) on g++ and g++-14 and stress (seed 7) passed.
- 2026-11-07: P263 follow-up added the Chinese search aliases (`grab --find`); no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to `_00_contest_runner.py` and its ccache `Builder`, warnings `-Wall -Wextra -Wconversion -Werror`, 10 s quick bound; full and stress inputs and checks unchanged; the library-wide runs above passed.
