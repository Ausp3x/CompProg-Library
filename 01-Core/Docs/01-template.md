# 01-template.hpp — evidence

`01-template.hpp` (C01, package P002, together with `02-debug.hpp`; see [02-debug.md](02-debug.md)) is the contest preamble: standard and PBDS includes, exact-width aliases, legacy macros, PBDS order-statistics aliases, finite sentinels and `chmin`/`chmax`. It supplies the verified width aliases that the rest of Core uses.

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

The entries resolve their headers relative to the script and run from any working directory; `CXX` selects the compiler, and they accept `--mode quick|full|stress` and `--seed` (or `CP_TEST_MODE`/`CP_TEST_SEED`). Configurations: optimized `-O2 -DNDEBUG`, checked `-O0 -g -D_GLIBCXX_ASSERTIONS` (PBDS under `_GLIBCXX_DEBUG` is impractically slow), and `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie` with leak detection, plus a standalone build.

Re-audit, 2026-10-06:

```bash
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode stress --seed 1
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/01-template.hpp 01-Core/02-debug.hpp '96-Local Testing/01-Core/01-template_tester.cpp' '96-Local Testing/01-Core/02-debug_tester.cpp' '96-Local Testing/01-Core/02-debug_no-local_tester.cpp'
python3 '97-Online Testing/03-workspace.py' && python3 '97-Online Testing/03-workspace.py' --check
```

All passed on GCC 16.2.1 (20260810), GNU++20, Linux x86-64, with `-Wall -Wextra -Wshadow -Wconversion -Werror`. Full (seed 335597, 5,000 histories per configuration): 6/6 steps. Stress (seed 1, 30,000 histories per configuration): all configurations. The closing-brace check reports no violations. `02-integration.py --sanitizers` is recorded in [18-bitset.md](18-bitset.md#commands-and-results).

## Sources

- `OLD/1-Core/01-template.hpp`: immediate feature/API reference; the active template preserves its aliases, helper names, PBDS alias, constants and macros.
- `01-Core/97-Legacy/01-template.cpp`: its older `all(x)`, `ral(x)` and narrowing `sze(x)` macros were superseded by explicit begin/end/rbegin/rend/size expressions and are not reintroduced. Original bytes remain in `OLD`.
- 2026-10-06 catalog sweep: [00-sources.md](00-sources.md); `indexed_map` follows KACTL OrderStatisticTree and hitonanode `pbds_map`.

## Limits and handoffs

GCC 14 itself and a Windows/MinGW build were not executed; the floor and the Windows contract rest on the correctness argument above. No concurrency safety or online judge acceptance is claimed. Items not adopted from the catalog sweep are listed in [00-notes.md](00-notes.md) ("Template and debug").

## History

- 2026-09-27: original P002 verification, full (seed 335597) and stress (seed 42) passed on GCC 16.2.1.
- 2026-09-27: integer/style maintenance, header unchanged, full suite passed.
- 2026-09-27: post-migration namespace review, header unchanged, quick suite, consistency and workspace check passed.
- 2026-10-06: re-audit, 2 findings fixed (Windows contract, tester braces), `indexed_map` added, full and stress passed on g++.
