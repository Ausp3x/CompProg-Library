# Header dependency graph — evidence (P238)

Scope: the include graph of the `01`–`07` headers, typing-order closures and per-folder charts ([05-dependencies.py](../05-dependencies.py)), include-derived batch prerequisites enforced by [plan.py](../../00-Guidelines/13-Plan/plan.py) `check`, and dependency selection in the [notebook](../01-notebook.py) (P238, support batch SUP07; reason: [decision log](../../00-Guidelines/10-decisions.md), Dependency graph and documentation site).

## Contracts

- Location (user decision 2026-10-24): the tool lives in this folder and prints closures and charts on demand; nothing generated is checked in.
- Graph: every `*.hpp` under `0[1-7]-*/` outside `97-Legacy`, with its quoted includes in any `[spaces]#[spaces]include "…"` form (user decision 2026-10-24) in source order, after `strip_cpp` (shared with `03-consistency.py`) blanks comments, including those on directive lines; paths are normalised to repository-relative ones. System includes are ignored. Aggregates (`98-basic`, `99-all`) are nodes, but charts and the plan check skip them because no batch owns them. Unresolved includes are a `03-consistency.py` error, and an include inside an `#if` block is a `plan.py check` error, because the graph treats every include as active.
- Closure: the header and its transitive includes, each after its own includes (depth-first post-order in include order), so typing top to bottom compiles. Line counts are `\n` counts, plus one for a final line without it.
- Substitutes (user decision 2026-10-24): only tested ones. A header with Compact twins, meaning it uses Core Barrett or Montgomery (`uses_reduction`, also used by `03-consistency.py`) and defines a `*Compact` struct or inline function, lists its direct includes of `01-Core/03-barrett.hpp` and `04-montgomery.hpp` as skippable unless another included header needs them. A mini replaces a full Core type only for a header recorded in `TESTED_MINIS` (empty; add a pair once that header's tester passes with the swap, and record here the alias lines to type); otherwise the closure notes each mini that would shorten it.
- Chart: Mermaid `flowchart LR`, arrow meaning "includes", the folder's headers with external targets grouped by folder, no template edges, and a count of the headers that include only the template.
- Plan (user decision 2026-10-24; rules in the [plan index](../../00-Guidelines/13-Plan/00-index.md), reasons in the decision log): `include_edges` lists (batch, prerequisite, header, include) for owned headers in different batches, `check` requires each prerequisite directly and `doctor --fix` appends the missing ones to `batches.json`. The removed ST06 -> ST03 edge was test-only (the SA-IS tester includes the suffix array) and formed a cycle with the new ST03 -> ST06.
- Notebook: a ticked header ticks its unticked closure, placed before it. `save --select` inserts each missing dependency before the first source that needs it and reports the additions, by default (user decision 2026-10-24); `--no-deps` keeps the selection as given. A later-selected dependency is not moved; a dependency that is not a selectable source fails the save with both names, and the UI label names it. Each section's `Needs:` line lists direct includes, the template only when unselected, with `(not selected)` and `(not for the Compact twins)` marks. Unticking a dependency stays possible.

## Run on 2026-10-24

The graph has 177 headers (14 aggregates) and 580 include edges (162 into the template; 357 and 160 outside the aggregates). 58 edges between batches (53 batch pairs) were not direct prerequisites, and 47 of those edges were not even transitive; the brief's 44 counted differently. 11 of the 58 were implied by other prerequisites. Three schedule inversions between verified packages remain (P014 and P017 in Foundations re-audit include headers of P027 and P081 in Contest core) and are allowed because each pair has a verified package. `doctor --fix` added the 53 pairs, and `check` passes. The largest preferred closures are `05-Mathematics/12-combinatorics_advanced.hpp` (13 files, 5403 lines), `03-Geometry/19-lattice_geometry.hpp` (3268) and `03-Geometry/14-minimumenclosingcircle.hpp` (3039; 1184 with an InfIntMini that has not been tested with it). One header has Compact twins (`05-Mathematics/07-primality_factorization.hpp`, skipping `04-montgomery.hpp`, 152 lines); 14 headers and 7 aggregates get mini notes (none for a full type itself or where the mini is already included).

## Feature-to-test map

Testers: [04-dependencies_tester.py](<../../96-Local Testing/00-Tools/04-dependencies_tester.py>) (38 tests; quick runs the `-MM` and compile oracles on a 15-header sample and every other repository oracle on all headers with 20 random DAGs; full and stress run both compiler oracles on every header, with 100 or `--rounds` DAGs plus on-disk random fixtures in stress) and [03-notebook_tester.py](<../../96-Local Testing/00-Tools/03-notebook_tester.py>). Oracles: an independent `os.walk` include scanner, `$CXX -std=gnu++20 -MM` closures, each closure concatenated in typing order without its quoted includes and compiled with `-fsyntax-only` in an empty directory (163 non-aggregate headers), iterative post-order and fixed-point reachability on random DAGs, raw newline counts, and a separate owner and prerequisite computation for `plan.py`.

| Feature | Tests |
|---|---|
| `headers` (normalising `../`, 97-Legacy at any depth, angle, indented and commented includes, other suffixes and folders, CRLF) | `test_headers_scan_normalisation_and_exclusions`, `test_headers_scan_empty_root`, `test_repo_graph_matches_scanner` |
| `closure` (chain, diamond, dedup, skip, swap of the root or of a node still needed elsewhere, typing order) | `test_closure_chain_diamond_dedup`, `test_closure_skip_and_swap`, `test_repo_typing_order`, `test_random_dags`; all 177 headers equal `-MM` with g++ and g++-14: `test_repo_mm_closure_all` (`_sample` in quick) |
| `lines` (only `\n` ends a line, a missing final newline counts), `compact` (struct, inline and multi-line twins; `//` and `/* */` comments, declarations, fields and Core excluded), `minis` | `test_lines_count`, `test_repo_line_counts`, `test_compact_detection`, `test_minis_mapping` |
| `plan` (optional versus needed through another path, `TESTED_MINIS` swap, mini note only when strictly shorter) | `test_plan_compact_optional_and_required_through_other_path`, `test_plan_minis_untested_and_tested`, `test_plan_minis_not_self_or_present`, `test_repo_plan_invariants` |
| `strip_cpp` (layout kept; comments, literals, raw strings, digit separators; directive comments including multi-line ones, directive literals), `includes` (`#if` depth, comments), `uses_reduction`, conditional headers | `test_strip_cpp_keeps_layout_and_directives`, `test_strip_cpp_directive_comment_regressions`, `test_strip_cpp_splices_and_unterminated`, `test_includes_comments_literals_and_depth`, `test_uses_reduction`, `test_headers_conditional_and_legacy_root` |
| typing order compiles | `test_repo_compile_closure_all` (`_sample` in quick) |
| `describe`, `chart` (exact edge set, no template edge or aggregates, subgraphs, isolated count), CLI exit codes | `test_describe_text`, `test_chart_edges_groups_and_isolated_count`, `test_cli_fixture_exit_codes_and_output`, `test_cli_repository` |
| `plan.py` `include_edges` (unowned includes ignored), `check` (direct prerequisite required even when reachable transitively; late only while both open; batch cycles), `rank` (moves nothing on the repository, keeps waived inversions, fixes open ones), `doctor --fix` (restores, idempotent, `fix=False` writes nothing) | `test_plan_include_edges_oracle`, `test_plan_include_edges_unowned_files`, `test_plan_check_real_copy_passes`, `test_plan_check_undeclared_include_edge`, `test_plan_check_late_only_while_both_open`, `test_plan_check_batch_cycles`, `test_plan_check_conditional_include`, `test_plan_rank_keeps_schedule_and_waived_inversions`, `test_plan_doctor_fix_restores_prerequisites` |
| Notebook `with_dependencies`, `needs`, `Needs:` lines, `data-deps`, `title` and tick handler, CLI `save --deps/--no-deps` (no `Added dependencies:` line when the save fails), unselectable dependency (error and label warning), tick handler under `node` | `test_with_dependencies_typing_order_and_user_order`, `test_needs_marks_unselected_template_and_optional`, `test_generate_writes_needs_lines`, `test_page_dependency_attributes_and_script`, `test_cli_save_adds_dependencies_unless_disabled`, `test_unselectable_dependency_errors_and_warns`, `test_page_script_ticks_dependencies_under_node` (SKIP: no `node`) |

## Commands and results (2026-10-24)

```bash
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode quick                    # PASS, 36 tests, 25 s, peak 1175 MB
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full                     # PASS, 38 tests, 133 s, peak 1178 MB
CXX=g++-14 python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full          # PASS, 38 tests, 161 s, peak 1312 MB
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode stress --seed 4 --rounds 200   # PASS, 38 tests, 132 s, peak 1174 MB
python3 '96-Local Testing/00-Tools/03-notebook_tester.py'                                     # PASS, 12 tests, 1 SKIP (no node), peak 18 MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 00-Tools --no-integration          # PASS, 4 suites
python3 '00-Guidelines/13-Plan/plan.py' check                                                 # plan check: ok
python3 '96-Local Testing/03-consistency.py'                                                  # no errors; brace rule on 370 files as before
python3 01-notebook.py build   # in a scratch copy, template + primality + lexicographic queries with dependencies: 16 pages, Needs lines render
```

## Limits

- Only the header that defines Compact twins skips its reduction includes. A dependent of it keeps them, because its own code may call the fast functions.
- The tick handler runs under `node` with a small DOM shim (`test_page_script_ticks_dependencies_under_node`); without `node` the test prints SKIP. The user installs `nodejs` (user decision 2026-10-24); until it has run, the handler is unverified.
- `.cpp` and Python sources are not followed: `01`–`07` hold only headers, and `08-Python` has no modules yet; Python imports belong to the Python contest layer.
- Compact detection is by name: a header that uses Barrett or Montgomery and defines a `*Compact` struct or inline function. Which functions need a twin stays a `@reviewer` check (`03-cpp.md`, Headers).
- The notebook selects real includes: a printed header is the whole file, so a Compact-only or mini substitute is left to the reader of the closure listing.

## History

- 2026-10-24: P238 introduced the graph, the include-edge and cycle checks and notebook dependency selection; review fixed 6 findings (a batch cycle, the late-check waiver and `rank`, test gaps, counts); all testers passed on g++ and g++-14.
- 2026-10-24: `/finish` audit and diff review: 14 defects fixed (line counts, mini notes, shared literal-aware stripper, directive comments and splices, a repeated scan, repeated decisions, rank wording, tester docstring and summary line, a legacy-path test, an unrecorded claim); user decisions recorded; added a compile oracle for typing order, comment, any-form include and `#if` handling, Compact needing a reduction, the unselectable-dependency message and a node test (SKIP until installed); all testers passed on g++ and g++-14.
