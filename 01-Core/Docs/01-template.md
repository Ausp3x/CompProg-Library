# 01-template.hpp — Evidence

`01-template.hpp` (C01, package P002 with `02-debug.hpp` and `18-bitset.hpp`; see [02-debug.md](02-debug.md)) is the contest preamble: standard and PBDS includes, exact-width aliases, legacy macros, PBDS order-statistics aliases, finite sentinels and `chmin`/`chmax`. The rest of Core uses its width aliases.

## Contracts

### Platform and Includes

GNU C++20, GCC 14 or newer, on Linux and Windows (MinGW) x86-64, with standard and PBDS includes. The header sets no global ISA or floating-point flags and does no executable I/O initialization. The Windows contract rests on an argument: the header uses only PBDS, `__int128` and `<cstdint>` aliases, no `long` and no POSIX calls, and MinGW-w64 GCC 14 provides all of them.

### Width Aliases

`uint`, `lng`, `ulng`, `lll` and `ulll` have 32, 64, 64, 128 and 128 bits, checked independently against `uint32_t`, `int64_t`, `uint64_t`, `__int128_t` and `__uint128_t`. They replace the older signed-width aliases of the legacy excerpt.

### Constants and Macros

`INF32` and `INF64` are finite repeated-`0x3f` sentinels; they do not prevent arithmetic overflow. `fi`, `se` and `pb` keep their existing spelling.

### indexed_set, indexed_map

`indexed_set<T>` is a GNU PBDS red-black ordered unique set; `indexed_map<K, V>` is the same tree with a mapped type (`operator[]`, iterators to `pair<const K, V>`). Both order by `std::less`. `find_by_order(k)` returns end when k is out of range; `order_of_key(x)` counts the keys strictly below x. Duplicates are represented explicitly with pair keys. Insertion, erase, rank and select take O(log(n)); storage is O(n).

### chmin, chmax

One comparison; assignment only on strict improvement; the result says whether it assigned. Both arguments have the same type, whose comparison and assignment costs belong to T. A floating NaN never improves either operand. Both are constexpr when T permits.

## Feature-to-Test Map

| Operation | Test | Oracle |
|---|---|---|
| Includes, width aliases | `01-template_tester.cpp::testAliases`; standalone and multiple-TU builds | Independent underlying-type assertions against the `<cstdint>`/`__int128` types |
| `INF32`, `INF64`, `fi`, `se`, `pb` | `testMacrosAndConstants` | Exact values, safe doubling, expanded field/insertion behavior |
| `indexed_set`, `indexed_map` | `testIndexedSet` | Empty/out-of-range, duplicates, pair keys, deletion, map overwrite, deterministic random histories against independent `std::set`/`std::map` iteration |
| `chmin`, `chmax` | `testChminChmax` | constexpr, boundaries, ties, self-alias, strings, counted custom operations, NaN |

Quick runs the optimized configuration with 500 random histories, full 5,000 histories per configuration, and stress the full configuration set with 30,000 histories at the requested seed. The fixed boundary tests run in every mode.

## Commands and Results

Harness: [`05-testing.md`](../../00-Guidelines/05-testing.md); flags `--mode` and `--seed` (or `CP_TEST_MODE`/`CP_TEST_SEED`). Configurations: optimized `-O2 -DNDEBUG`; checked `-O0 -g -D_GLIBCXX_ASSERTIONS` (PBDS under `_GLIBCXX_DEBUG` is impractically slow); `-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie` with leak detection; and a standalone build.

Library-wide runs of 2026-11-10 (P274; Intel Core i9-11900H, GNU++20, seed 42, run beside the other suites, build cache emptied before each cold run), all PASS:

| Command | Compiler | This suite | Memory peak |
|---|---|---|---|
| `python3 '96-Local Testing/01-run.py' --mode full` | GCC 16.2.1 | preflight quick peak 350 MB; optimized-NDEBUG 0.3 s compile + 0.1 s run, checked 8.0 s + 1.1 s, ASan-UBSan 20.0 s + 2.3 s | 727 MB |
| `CXX=g++-14 python3 '96-Local Testing/01-run.py' --mode full` | GCC 14.4.1 | preflight quick peak 362 MB; optimized-NDEBUG 0.2 s + 0.2 s, checked 14.8 s + 1.8 s, ASan-UBSan 31.5 s + 5.7 s | 753 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | GCC 16.2.1 | optimized-NDEBUG 11.8 s + 0.0 s | 353 MB |
| `python3 '96-Local Testing/01-run.py' --mode stress --rounds 1` | GCC 16.2.1 | preflight quick peak 28 MB; optimized-NDEBUG 0.1 s + 0.4 s, checked 3.8 s + 3.3 s, ASan-UBSan 10.1 s + 8.9 s | 731 MB |

Restyle checks of 2026-10-08, the latest run of each:

```bash
python3 '96-Local Testing/03-consistency.py' --braces 01-Core/01-template.hpp 01-Core/02-debug.hpp 01-Core/18-bitset.hpp
python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full --seed 20261008
CXX=g++-14 python3 '96-Local Testing/01-Core/01-template_tester.py' --mode full --seed 20261008
python3 '96-Local Testing/02-integration.py'
python3 '96-Local Testing/03-consistency.py'
python3 '97-Online Testing/03-workspace.py' && python3 '97-Online Testing/03-workspace.py' --check
```

All passed on GCC 16.2.1 and GCC 14.4.1 (20260915), GNU++20, Linux x86-64, with `-Wall -Wextra -Wshadow -Wconversion -Werror`: full 6/6 steps on each compiler, no brace or comment-cap violations, integration over 102 standalone and aggregate headers, consistency without errors, Workspace regenerated unchanged.

## Sources

- `OLD/1-Core/01-template.hpp`: the feature and API reference; the active template keeps its aliases, helper names, PBDS alias, constants and macros.
- `OLD/algorithms.cpp` lines 1–30: its `all(x)`, `ral(x)` and narrowing `sze(x)` macros are superseded by explicit begin/end/rbegin/rend/size expressions and are not reintroduced.
- 2026-10-06 catalog sweep: [00-sources.md](00-sources.md); `indexed_map` follows KACTL OrderStatisticTree and hitonanode `pbds_map`.

## Limits and Handoffs

- No concurrency safety is claimed.
- Items not adopted from the catalog sweep are listed in [00-notes.md](00-notes.md) ("Template and Debug").
- Verified before the mutation rule of 2026-11-03; no record yet.

## Aliases

Search aliases (P263, grab --find): 平衡树, 缺省源, 万能头文件, 平板电视, 名次树, 比赛模板.

## History

- 2026-09-27: original P002 verification, full (seed 335597) and stress (seed 42) passed on GCC 16.2.1.
- 2026-09-27: integer/style maintenance, header unchanged, full suite passed.
- 2026-09-27: post-migration namespace review, header unchanged, quick suite, consistency and workspace check passed.
- 2026-10-06: re-audit, 2 findings fixed (Windows contract, tester braces), `indexed_map` added, full and stress passed on g++.
- 2026-10-06: re-audit, full (seed 335597) and stress (seed 1) passed on g++, closing-brace check and workspace check passed.
- 2026-10-06: closing-brace checker built in `03-consistency.py` and calibrated (the eight P002 files report nothing after normalization, 160 true positives before; a probe file reports exactly its four real violations); the bitset review's edge cases fixed ([18-bitset.md](18-bitset.md)).
- 2026-10-08: restyle, full suite (seed 20261008) passed on g++ and g++-14, integration, consistency and workspace check passed.
- 2026-11-07: P263 follow-up added Chinese search aliases (`grab --find`): OI Wiki page titles and common names of the row's operations, Simplified plus Taiwan or Hong Kong terms where they differ; no code or test change.
- 2026-11-10: P274 test speed (SUP25): builds moved to the shared ccache `Builder` in `_00_cpp_test_runner.py`, up to three at once within the memory cap, 10 s quick bound; full and stress inputs and checks unchanged; every library-wide run above passed.
