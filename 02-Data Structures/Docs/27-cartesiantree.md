# 27-cartesiantree.hpp — evidence

Package P033 (batch DS13). Rewritten from the legacy reference `OLD/Team Notebook/src/ds/cartesiantree.cpp`. An algorithm header: free functions with a result struct. Preconditions are `assert`s.

## Contracts

### CartesianTree

Result struct with `root` (-1 for an empty array), `parent`, `leftChild`, `rightChild` (-1 when absent) and `range[i] = [l, r)`, the index interval covered by the subtree of `i`, which is the maximal interval in which `a[i]` is the tie-broken minimum. In-order traversal is `0, 1, ..., n - 1`. Public data; mutating it voids the `lcaRmqBridge` precondition.

### cartesianTree(a, comp = less<T>())

`n <= INT_MAX`. One left-to-right stack pass: element `i` pops every stack element `j` with `comp(a[i], a[j])`; the last popped element becomes its left child, the remaining stack top its parent, and a popped `j` gets `range[j].second = i`; `range[i].first` is the remaining stack top plus 1. Each index is pushed and popped once: O(n) comparisons.

- `comp` a strict weak order: a min-tree for `less`, a max-tree for `greater`; among equal elements the leftmost is the ancestor (an equal later element is not popped), so the root of `[l, r)` is the leftmost argmin, matching row 54 `LinearRMQ` and Graphs `EulerLCA`.
- `comp` the reflexive closure of a strict weak order (`less_equal`, `greater_equal`): an equal later element pops the earlier one, so the rightmost equal element is the ancestor. This is the strict order on the keys `(a[i], -i)`.
- The tree is unique for each rule because it is the Cartesian tree of distinct keys `(a[i], ±i)`.

### lcaRmqBridge(t)

`n < 2^30` (the tour has `2n - 1` entries). Returns `(tour, depth, first)`: the Euler tour of `t` from the root (each vertex is written on entry and after each child returns), the depth of each tour entry (consecutive depths differ by exactly 1) and the first tour position of each vertex. For vertices `u`, `v`, the minimum depth on positions `[min(first[u], first[v]), max(...)]` is attained only at `lca(u, v)`. With `t = cartesianTree(a)`, `lca(l, r - 1)` is the tie-broken argmin of `a[l, r)`, so the bridge reduces range minimum to a ±1 range minimum (Farach-Colton and Bender), which row 54 `PlusMinusOneRMQ` or any RMQ structure answers. The walk follows parent pointers iteratively (no recursion), O(n). An empty tree gives three empty vectors. This is the reduction kept by decision in `duplicates.md` (an LCA–RMQ reduction, not a rename of `LinearRMQ::query` or `EulerLCA`); the research sweep found the row name vague, so its result is fixed here.

### Violated preconditions

The size limits are not reachable in memory tests (`2^30` vertices need more than 8 GB of output), so they have no assertion probes. A `CartesianTree` mutated into a non-tree may make `lcaRmqBridge` read out of range, a caller contract like every public data member in this folder; its walk still stops after `2n` steps.

## Feature-to-test map

Tester `96-Local Testing/02-Data Structures/27-cartesiantree_tester.cpp`, entry `27-cartesiantree_tester.py` (driver `_00_runner.py`; three builds; no assertion probes, see above).

| Operation | Group | Oracle |
|---|---|---|
| `cartesianTree` `parent`, `leftChild`, `rightChild`, `root`, `range` (less, less_equal, greater, greater_equal) | `exhaustive` (every array of length <= 8 over {0, 1, 2}; <= 9 in stress) | Recursive definition: the root of `[l, r)` is its leftmost (or rightmost) minimum, children are the roots of the two sides, `range` is the recursion interval |
| Same for other value types and comparators | `randomSmall` (`int` with random magnitudes, `string`, `double`, an absolute-value comparator) | Same recursive brute force |
| Large and deep inputs (increasing, decreasing, constant, four values, zigzag, uniform; n = 200000 full, 500000 stress) | `large` / `properties` | Heap order under the comparator, the tie rule between equal parent and child, a single root, child links, in-order equal to `0..n-1` (these determine the tree), subtree intervals computed bottom-up from the children |
| `lcaRmqBridge` | `checkBridge` from `exhaustive`, `randomSmall` (every `(l, r)`) and `large` (200 sampled short ranges) | Tour length `2n - 1`, ±1 depth steps, tour edges are tree edges, root at both ends, depths agree with parent depth + 1, `first` equals the first occurrence, and the minimum-depth vertex between `first[l]` and `first[r - 1]` equals both the brute-force argmin of `a[l, r)` and the lca found by walking parents |
| Empty and single arrays, the leftmost and rightmost ties on a sample | `edges` | Direct values |

Mutation check (2026-10-14, quick mode, temporary copies): 5 of 5 injected faults detected (tie rule flipped, `range` left end off by one, root taken from the stack top, `first` overwritten on every visit, the bridge skipping a right child after its left child).

## Commands and results

```bash
python3 '96-Local Testing/02-Data Structures/27-cartesiantree_tester.py' --mode full --seed 20261014
CXX=g++-14 python3 '96-Local Testing/02-Data Structures/27-cartesiantree_tester.py' --mode full --seed 20261014
python3 '96-Local Testing/01-run.py' --mode stress --seed 7 --rounds 1 --filter '02-Data Structures/27' --no-integration
```

Final run 2026-10-14 (after the review fixes): full mode seed 20261014 passed on GCC 16.2.1 and GCC 14.4.1 (`CXX=g++-14`) with 70,258,594 checks per configuration, MEMORY peak 479 MB and 516 MB; stress mode seed 7 (one round) passed all three configurations with 181,013,001 checks per configuration, MEMORY peak 710 MB. Per-element checks build their failure message only on failure, which brought the ASan stress binary from 263 s to 26 s. All runs on Linux x86-64 (Intel Core i9-11900H), GNU++20 with `-Wall -Wextra -Wconversion -Werror`, CPython 3.14, in the optimized (`-O2 -DNDEBUG`), checked (`-O0 -g -D_GLIBCXX_DEBUG`) and ASan/UBSan (`-O1 -g -D_GLIBCXX_ASSERTIONS -fsanitize=address,undefined`) builds; MEMORY peaks are dominated by compilation. `02-integration.py --sanitizers` passed (143 standalone and aggregate headers, `99-all.hpp` in two translation units, scalar and AVX2, workspace, sanitizer self-tests; peak 2758 MB) and `03-consistency.py` reported no errors. GCC 14.2 exactly, the Windows build and PyPy were not run.

## Legacy mapping

| `OLD/Team Notebook/src/ds/cartesiantree.cpp` | Replacement |
|---|---|
| `CartesianTree<T, Comp>` constructor (stack build, `par`, `lc`, `rc`, `root`) | `cartesianTree(a, comp)` returning `parent`, `leftChild`, `rightChild`, `root` (same stack algorithm and tie rule) |
| Stored reference `a`, `n`, `comp` | Not adopted: the result owns no reference to the input, so it stays valid after the array changes |

## Sources

| Source | Actual reading and use |
|---|---|
| Library Checker `tree/cartesian_tree` task statement | Distinct values, parent array with the root as its own parent (the library uses -1). |
| suisen `datastructure/cartesian_tree.hpp`, maspypy `seq/cartesian_tree.hpp`, Nyaan `tree/cartesian-tree.hpp`, ei1333 `graph/others/cartesian-tree.hpp`, hitonanode `tree/cartesian_tree.hpp`, OI Wiki Cartesian tree | Subtree ranges, min/max selection, the leftmost (suisen, maspypy, Nyaan, hitonanode, OI Wiki) and rightmost (ei1333) tie rules. |
| cp-algorithms, Linear RMQ and Farach-Colton–Bender LCA | RMQ to LCA through the Cartesian tree, LCA to ±1 RMQ through the Euler tour. |

Fetched 2026-10-14 by the completeness sweep ([00-sources.md](00-sources.md#fetched-2026-10-14-p033-sweep)); the implementation was written from the contract.

## Limits and handoffs

- Left out with reasons in [00-notes.md](00-notes.md#p033-omissions): weighted subarray counts by minimum, depth or traversal arrays.
- Row 08 owns nearest smaller values and the histogram rectangle, row 19 the linear treap build, row 54 linear and ±1 RMQ, row 55 the permutation tree, Graphs `07-lca.hpp` `EulerLCA`.

## History

- 2026-10-14: P033 first implementation and verification; independent review: stress timeout fixed (lazy failure messages).
