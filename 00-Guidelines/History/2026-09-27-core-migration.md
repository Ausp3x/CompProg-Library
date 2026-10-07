# Core modular-family migration

The 2026-09-27 migration implements the approved namespace/co-location rules and adds independently copyable modular mini types. Current scope is **428 numbered targets, 335 algorithm batches, 226 packages and five support passes**. Core has 18 numbered targets. Existing package IDs and C01–C15 retain their identity; P005 owns C03 and the new C16, in that order. The earlier expansion audit remains a historical record.

## Canonical layout and ownership

| Target | Contents | Owner |
|---|---|---|
| `01-Core/05-modint.hpp` | `modint_detail`, `ModInt`, `ModInt64`, `DynModInt`, `DynModInt64`; existing `mint` preserved | P005/C03 |
| `01-Core/06-modintmini.hpp` | `ModIntMini`, `ModInt64Mini`, `DynModIntMini`, `DynModInt64Mini`; each independently copyable with standard headers/template aliases | P005/C16 |
| `01-Core/07-infint.hpp` through `18-bitset.hpp` | Former Core targets 09–20, shifted down two | Existing owners unchanged |

Core 01–04 and aggregate names 98/99 are unchanged. Both aggregates expose full and mini modular families without alias collisions; Basic retains full types needed by current consumers. Four full-type C++ suites now run through one `05-modint_tester.py` entry. The mini family has one `06-modintmini_tester.py` entry. Later Core tester and benchmark filenames follow their header numbers.

The [exact path map](../Ledgers/core-path-updates.json) records retired headers, moved artifacts and archived originals. The canonical [structure guide](../02-structure.md), [Core inventory](../../01-Core/00-index.md), [batch manifest](../13-Plan/batches.json), both machine scheduling maps, checklist, dependent includes, source/archive maps, judge-family map and generated outputs use the current layout. Detailed modular contracts and evidence remain in [full-family evidence](../../01-Core/Docs/05-modint.md) and [mini evidence](../../01-Core/Docs/06-modintmini.md).

## Style and preservation

Family-only shared helpers live with their public types in one shallow `<family>_detail` namespace. One-type helpers stay in that struct. Namespace contents use four spaces and a separate labelled closing line. Existing public names remain stable. Barrett and Montgomery remain independently reusable headers. Mini structs have no sibling/full/base/detail/reduction-header dependencies; any necessary helpers and state are inside the copied struct.

Debug, FastIintMul and FastMatMul/FastGaussian keep their public names and receive namespace closing comments. Their algorithms are unchanged. Previously existing unverified integer/matrix implementations remain unverified; this migration is not a claim of finishing their planned features.

Previously archived source material retains its original bytes; only the mutable `OLD/00-index.md` routing metadata is updated for the current count. Four original full modular files are additionally preserved under `OLD/2026-09-27-modint-layout/`, bringing the archive manifest to 205 files. Historical benchmark JSONL contents, old source hashes, original mapping keys and monolith source/body hashes remain unchanged. Live destinations are updated. The moved bitset benchmark log retains its historical path/hash fields; current benchmark drivers use current includes and paths. Do not treat an old source hash as a measurement of a later edited source.

## Verification

Fresh migration checks use GCC16.2.1, GNU++20 and CPython3.14 on Linux x86-64. Exact GCC14/Python3.10/PyPy runtime execution is not claimed. Changed Python scripts additionally parse under Python3.10 grammar.

| Check | Fresh result |
|---|---|
| Full modular family | All four types passed six configurations each: optimized, checked and ASan/UBSan in scalar and AVX2 builds; independent Python oracles, static rejection, dynamic cross-TU and Matrix/ModFac compatibility checks |
| Mini modular family | All four structs passed 176,384 independent exact cases per optimized/checked/sanitized configuration; standalone copying, constexpr/rejection, checked failures, ID/reset and cross-TU coverage; final exact-standard-include copy fixture rerun passed |
| Mini performance | 3,552 checked timing measurements in scalar and AVX2/BMI2 builds; source hashes match the frozen final header; see C16 evidence for workloads, medians and limits |
| Core integration | 59 standalone/aggregate headers, scalar/available AVX2 multiple-TU linkage, Workspace LOCAL/non-LOCAL compilation |
| Renamed Bitset and Debug | Bitset quick suite passed three configurations; Debug quick suite passed LOCAL/non-LOCAL and standalone/multiple-TU checks; algorithm bodies preserved apart from namespace comments |
| Online and Workspace tooling | Four online-tool fixture groups passed; real bundler regenerated all 20 sources; Workspace explicitly generated and `--check` passed; all 40 canonical/expanded judge translation units passed syntax checks |
| Notebook | Discovery includes current full/mini/renumbered Core paths; selected template/debug IDs remain valid |
| Documentation and provenance | Consistency validator passes numbering, ownership, dependencies, links, tables/fences, map destinations, includes, aggregates, discovery, 250 judge-family mappings, 205 archive records/hashes and 55 monolith entries; independent review confirms no stale active paths |

Reproduce the migration checks from the repository root:

```bash
python3 '96-Local Testing/03-consistency.py'
python3 '96-Local Testing/01-Core/05-modint_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/06-modintmini_tester.py' --mode full --seed 20260927
python3 '96-Local Testing/01-Core/18-bitset_tester.py' --mode quick --seed 20260927
python3 '96-Local Testing/01-Core/02-debug_tester.py' --mode quick --seed 20260927
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/00-Tools/02-online_tester.py'
python3 '97-Online Testing/03-workspace.py' --check
python3 '98-Team Notebook/01-notebook.py' list
```

Sanitizer runs retain leak checking; an initial sandbox ptrace restriction was resolved by approved execution outside that sandbox. Full/mini evidence distinguishes those reruns from actual arithmetic failures. No fresh performance claim is made for the unchanged full engine or renamed Bitset. No online submission was made. Finite tests and catalog/path checks do not prove all possible algorithm inputs or implement the remaining inventory.

Current hashes for files whose earlier evidence refers to an older name or generated snapshot:

| Current file | SHA256 |
|---|---|
| `01-Core/02-debug.hpp` | `2efc4730a89f75b079e5c394b9701cafb5dc38f7528fcd6f24df0a3795e8e08a` |
| `01-Core/18-bitset.hpp` | `01150bfcbf3638f8407bb3e19ad6c421c3e47b61c32ee37df3b610c39b640bd4` |
| `99-Workspace/template.cpp` | `dd51486bf075e4c3dd792dfc7fa380eef3dfd3f97a90f6f44e1806f219f3b068` |

The permanent [consistency validator](<../../96-Local Testing/03-consistency.py>) audits exact Markdown/machine ownership, target numbering, package prerequisites, local links/formatting, current map destinations/includes, Core aggregate coverage, quick-suite discovery, archived hashes, monolith provenance, expansion manifests and missing-workspace reporting. It is read-only except for temporary fixtures. The integration runner now fails clearly for a missing required Workspace snapshot; generation remains explicit.

## Maintenance in existing sessions

Re-audit a completed package with `/package Pxxx` (the package status `audit` selects the re-audit workflow in `.claude/skills/package/SKILL.md`). Read current guides and files again. P001–P004 retain their original scope; P002 uses `18-bitset.hpp`. P005 reviews both C03 and C16 and their reduced/full contracts. Do not restore retired paths, redo completed global renumbering, or concurrently edit shared inventories/maps from several sessions.
