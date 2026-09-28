# 01-Core implementation batches

Use [the common task prompt](../../01-prompts.md) and the [folder inventory](<../../01-Core/00-index.md>). Read only selected rows and their required contracts. IDs remain stable across renumbering; filenames below are exact. Every batch owns implementation, research, tests and appropriate benchmarks. Size is relative effort, not a time promise. XL work uses explicit checkpoints and a handoff if needed.

Prerequisites below are conservative API ordering. The full inventory may require an additional backend for a selected variant; inspect direct dependencies before coding. Optional alternatives need not force every backend. The bottom-of-prompts checklist supplies the default session order.

| ID | Owned target filenames | Size | Prerequisites | Focus |
|---|---|---|---|---|
| C01 | `01-template.hpp`, `02-debug.hpp` | M | None | Template and debug foundation; independent. |
| C02 | `03-barrett.hpp`, `04-montgomery.hpp` | L | C01 | Barrett/Montgomery backends; stabilize before modular types. |
| C03 | `05-modint.hpp` | L | C01, C02 | Full static/dynamic 32/64-bit modular integers in one cohesive header; C02. |
| C04 | `07-infint.hpp` | XL | C01, C03 | Full InfInt in multiplication, division/gcd, conversion/bitwise and sentinel stages; retain one header owner and separate checkpoints. |
| C05 | `08-infintmini.hpp` | M | C01, C04 | Independent InfIntMini; compare shared semantics with C04 and account for legacy base conversion and sentinel behavior. |
| C06 | `09-rational.hpp` | M | C01, C04 | Canonical rational; C04 for arbitrary precision backend. |
| C07 | `10-matrix.hpp` | XL | C01, C03, C06 | Full dense Matrix in semiring/field/ring, structured-solve and exact/approximate decomposition stages; retain one header owner. |
| C08 | `11-matrixmini.hpp` | M | C01, C07 | Independent MatrixMini; compare shared semantics with C07. |
| C09 | `12-bitmatrix.hpp` | L | C01, C15 | Full GF(2) BitMatrix with packed elimination. |
| C10 | `13-bitmatrixmini.hpp` | M | C01, C09 | Independent BitMatrixMini; compare shared semantics with C09. |
| C11 | `16-poly.hpp` | XL | C01, C02, C03 | Full Poly/FPS engine in convolution/FPS, evaluation/composition and factorization stages; preserve the complete family backlog. |
| C12 | `17-polymini.hpp` | M | C01, C11 | Independent PolyMini; compare shared semantics with C11. |
| C13 | `14-sparsematrix.hpp` | XL | C01, C07 | Full SparseMatrix in formats/products, exact black-box and approximate iterative stages; distinguish fixed iteration from stationary-state convergence. |
| C14 | `15-sparsematrixmini.hpp` | M | C01, C13 | Independent SparseMatrixMini; compare shared semantics with C13. |
| C15 | `18-bitset.hpp` | L | C01 | Dynamic packed bitset, fused operations, padding/aliasing rules and scalar/accelerated kernels. |
| C16 | `06-modintmini.hpp` | L | C01, C03 | Four independent copyable modular-integer mini structs; document reduced APIs and verify shared semantics against C03 and independent oracles. |

Large single-header families keep one owner but require operation-level checkpoints; parallel workers must not independently rewrite the same header. XL stages may need continuation sessions before every contracted operation is complete.
