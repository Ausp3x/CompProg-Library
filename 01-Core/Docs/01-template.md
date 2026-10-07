# 01-template.hpp — evidence

`01-template.hpp` (C01, package P002, together with `02-debug.hpp`; see [02-debug.md](02-debug.md)) is the contest preamble: standard and PBDS includes, exact-width aliases, legacy macros, PBDS order-statistics aliases, finite sentinels and `chmin`/`chmax`. It supplies the verified width aliases that the rest of Core uses. The original `OLD` and `97-Legacy` references are preserved. The later [Core namespace migration](../../00-Guidelines/History/2026-09-27-core-migration.md) left this header byte-for-byte unchanged; hashes below describe the stage at which they were recorded.

## Contracts

### Platform and includes

GNU C++20, GCC14+, Linux and Windows (MinGW) x86-64; standard/PBDS includes. No global ISA/FP flags or executable I/O initialization. The Windows contract is a correctness argument, not an executed test: the header uses PBDS, `__int128` and `<cstdint>` aliases only, no `long`, no POSIX calls; MinGW-w64 GCC 14 provides all of them.

### Width aliases

`uint`, `lng`, `ulng`, `lll`, `ulll` have 32/64/64/128/128 bits, checked independently against `uint32_t`, `int64_t`, `uint64_t`, `__int128_t` and `__uint128_t`. The older signed-width aliases of the legacy excerpt are replaced by these exact-width aliases.

### Constants and macros

`INF32`/`INF64` are finite repeated-`0x3f` sentinels; they do not prevent arithmetic overflow. `fi`, `se`, `pb` retain the existing spelling.

### indexed_set, indexed_map

`indexed_set<T>` is a GNU PBDS red-black ordered unique set; `indexed_map<K, V>` is the same tree with a mapped type (`operator[]`, iterators to `pair<const K, V>`); both use `std::less` ordering. `find_by_order(k)` returns end outside size; `order_of_key(x)` counts keys strictly below x. Pair keys represent duplicates explicitly. O(log(n)) insertion/erase/rank/select, O(n) storage.

### chmin, chmax

One comparison; assignment only on strict improvement; return whether assigned. Same-type arguments; comparison and assignment costs belong to T. Floating NaN never improves either operand. constexpr when T permits.

## Feature-to-test map

| Operation | Test | Oracle |
|---|---|---|
| Includes, width aliases | `01-template_tester.cpp::testAliases`; standalone and multiple-TU builds | Independent underlying-type assertions against the `<cstdint>`/`__int128` types |
| `INF32`, `INF64`, `fi`, `se`, `pb` | `testMacrosAndConstants` | Exact values, safe doubling, expanded field/insertion behavior |
| `indexed_set`, `indexed_map` | `testIndexedSet` | Empty/out-of-range, duplicates, pair keys, deletion, map overwrite, deterministic random histories against independent `std::set`/`std::map` iteration |
| `chmin`, `chmax` | `testChminChmax` | constexpr, boundaries, ties, self-alias, strings, counted custom operations, NaN |

Quick uses the optimized configuration and 500 random histories; full runs 5,000 histories per configuration; stress uses the same full configuration set and 30,000 histories with the requested seed. Fixed boundary tests run in all modes. All oracles are non-removable checks.

## Commands and results

The entries resolve their headers relative to the script, so they run from any working directory when given an absolute path; artifacts live in temporary directories. Terminals show in-place progress and green PASS/red FAIL unless `NO_COLOR` is set; redirected output uses plain lines. `CXX` selects the compiler, and the entries accept `--mode quick|full|stress` and `--seed`, or the run-all runner's `CP_TEST_MODE`/`CP_TEST_SEED` environment variables.

### Original P002 verification — 2026-09-27

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode stress --seed 42
```

The full run (seed 335597) and stress run (seed 42, 30,000 histories per configuration) passed with GCC 16.2.1 (20260810), GNU++20, on Linux x86-64. Required configurations are optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_ASSERTIONS`, and `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie`, with leak detection enabled. An initial PBDS `_GLIBCXX_DEBUG` run was impractically slow, so the template checked configuration uses `_GLIBCXX_ASSERTIONS`; this changes instrumentation, not the feature oracle or workload. An initial sandbox sanitizer run (recorded for debug in [02-debug.md](02-debug.md)) hit LeakSanitizer's ptrace restriction; the successful full sanitizer executions of the package were rerun outside that sandbox with approval. Quick mode also passed after the display-only runner update. Aggregate integration and generated workspace synchronization were recorded by the P002 integration owner. This record does not claim GCC14 itself was executed, concurrency safety or online judge acceptance.

### Integer and style maintenance — 2026-09-27

The revised `03-cpp.md` integer/style defaults were reread; `01-template.hpp` already met them and stayed byte-for-byte unchanged. Existing testers use short locals, explicit standard qualification where required, functional 128-bit casts and unchanged workloads and draw order; `size_t` remains only for comparison with PBDS's rank result. No new tests or features were added.

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
```

Passed 6/6 steps with GCC 16.2.1, seed 335597, 5,000 iterations per configuration (optimized NDEBUG, checked assertions, ASan/UBSan with leak checking, standalone compilation). Stress and performance comparisons were not rerun.

| File | SHA256 at integer/style maintenance |
|---|---|
| `01-Core/01-template.hpp` | `251352f9cc65171c4527dc0596b40b21507652f0b2d0017c50431f67561964a2` |
| `96-Local Testing/01-Core/01-template_tester.cpp` | `3886387adb1a60323cc7f4ace67a9ff665ed94ae2291a9f59a417376084209fd` |

### Post-migration namespace review — 2026-09-27

The header already satisfied the integer, grouping and namespace rules and remained byte-for-byte unchanged; alias definitions and independent underlying-type assertions remain intact.

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode quick --seed 20260927
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' --check
```

Template quick passed 2/2 steps with 500 histories (GCC 16.2.1, GNU++20, Linux x86-64). Consistency validation passed with 428 targets, 335 batches and 226 packages; Workspace `--check` passed read-only with the snapshot unchanged. Template full, stress and performance comparisons were not rerun. Source hashes at this review: `01-Core/01-template.hpp` `251352f9cc65171c4527dc0596b40b21507652f0b2d0017c50431f67561964a2`, `96-Local Testing/01-Core/01-template_tester.cpp` `3886387adb1a60323cc7f4ace67a9ff665ed94ae2291a9f59a417376084209fd`, `99-Workspace/template.cpp` `11f9be9e307b483e7b1ee2bb2d47ddcf2a3a78356e84f7aa9726698128a29104`.

### Re-audit — 2026-10-06

Package P002 re-audit under the current `03-cpp.md`/`05-testing.md` rules, treating the previous verification as existing-unverified. Every operation in the row was compared with the code and the tester first; the full suite was rerun before any edit (6/6 on GCC 16.2.1, passing) and again after.

| # | Finding ([reaudit findings](<../../00-Guidelines/23-Reaudit Findings/00-index.md>) at review time) | Disposition |
|---|---|---|
| 3 | Header contract named only Linux | Fixed: `01-template.hpp` and this document state Linux and Windows (MinGW) x86-64 (argument under Contracts). No Windows build was run (no cross compiler installed). |
| 9, 10 | Tester closing braces on their own lines | Fixed mechanically in `01-template_tester.cpp`; behavior unchanged. The checker in `03-consistency.py` reports none. |

Changes beyond the findings: `indexed_map<K, V>` added (KACTL OrderStatisticTree, hitonanode `pbds_map`) and tested against `std::map` with the seeded history oracle; the foundation runner compiles with `-Werror`. Catalog research is in [00-sources.md](00-sources.md) ("Fetched 2026-10-06 — P002 re-audit sweep"); exclusions with reasons are in [00-notes.md](00-notes.md) ("Template and debug"). The closing-brace checker built in this re-audit is recorded in [00-notes.md](00-notes.md#closing-brace-checker).

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode stress --seed 1
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/01-template.hpp 01-Core/02-debug.hpp '96-Local Testing/01-Core/01-template_tester.cpp' '96-Local Testing/01-Core/02-debug_tester.cpp' '96-Local Testing/01-Core/02-debug_no-local_tester.cpp'
python3 '97-Online Testing/03-workspace.py' && python3 '97-Online Testing/03-workspace.py' --check
```

All passed on GCC 16.2.1 (20260810), GNU++20, Linux x86-64, with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 335597, 5,000 histories per configuration): 6/6 steps. Stress (seed 1, 30,000 histories per configuration): all configurations. The closing-brace check reports no violations. `02-integration.py --sanitizers` is recorded in [18-bitset.md](18-bitset.md#re-audit--2026-10-06). GCC 14 and a Windows/MinGW build were not executed; the floor and the Windows contract rest on the correctness argument above.

| File | SHA256 after the re-audit |
|---|---|
| `01-Core/01-template.hpp` | `23e6db3f682fd9f27657ace73f157a8631790d06c8c72984d9fcc102de7341e6` |
| `96-Local Testing/01-Core/01-template_tester.cpp` | `4fa8096a2989f42e10081aaef90a285161e5e6101731382d981b1d2d470d9d3d` |
| `96-Local Testing/01-Core/_00_foundation_runner.py` | `1dd0641b66c8cd8cc14752738dceeb4c60bf44ecbfa0387d2a9dd5aa4fffd07d` |
| `99-Workspace/template.cpp` | `f6aa5003c6163e3fba441eb5508e21bc866cfe98faaccd37e1fc3ad59c8ea9df` |

## Sources

- `OLD/1-Core/01-template.hpp` was inspected as the immediate feature/API reference. The active template preserves its aliases, helper names, PBDS alias, constants and macros.
- `01-Core/97-Legacy/01-template.cpp` was inspected with its local index and the relevant migration notes. Its older `all(x)`, `ral(x)` and narrowing `sze(x)` macros had already been superseded by explicit begin/end/rbegin/rend/size expressions and are not reintroduced. Original bytes remain in `OLD`.
- 2026-10-06 catalog sweep: [00-sources.md](00-sources.md); `indexed_map` follows KACTL OrderStatisticTree and hitonanode `pbds_map`.

## Limits and handoffs

GCC 14 itself and a Windows/MinGW build were not executed. No concurrency safety or online judge acceptance is claimed. Items not adopted from the catalog sweep are listed in [00-notes.md](00-notes.md) ("Template and debug"). No P002-owned gap remains.
