# Header dependency graph — evidence (P238)

Scope: the include graph of the `01`–`07` headers, typing-order closures and per-folder charts ([05-dependencies.py](../05-dependencies.py)), include-derived batch prerequisites enforced by [plan.py](../../00-Guidelines/13-Plan/plan.py) `check`, and dependency selection in the [notebook](../01-notebook.py) (P238, support batch SUP07; reason: [decision log](../../00-Guidelines/10-decisions.md), Dependency graph and documentation site).

## Contracts

- Location (user decision 2026-10-24): the tool lives in this folder and prints closures and charts on demand; nothing generated is checked in.
- Graph: every `*.hpp` under `0[1-7]-*/` outside `97-Legacy`, with its quoted includes in any `[spaces]#[spaces]include "…"` form (user decision 2026-10-24) in source order, after `strip_cpp` (shared with `03-consistency.py`) blanks comments, including those on directive lines; paths are normalised to repository-relative ones. System includes are ignored. Aggregates (`98-basic`, `99-all`) are nodes, but charts and the plan check skip them because no batch owns them. Unresolved includes are a `03-consistency.py` error, and an include inside an `#if` block is a `plan.py check` error, because the graph treats every include as active.
- Closure: the header and its transitive includes, each after its own includes (depth-first post-order in include order), so typing top to bottom compiles. Line counts are `\n` counts, plus one for a final line without it. `closure` on the CLI prints the header, file count, total lines and per-file lines, then points to [10-grab.py](<../../09-Contest Testing/10-grab.py>), which extracts only the declarations a name reaches (user decision 2026-11-05).
- Chart: Mermaid `flowchart LR`, arrow meaning "includes", the folder's headers with external targets grouped by folder, no template edges, and a count of the headers that include only the template.
- Plan (user decision 2026-10-24; rules in the [plan index](../../00-Guidelines/13-Plan/00-index.md), reasons in the decision log): `include_edges` lists (batch, prerequisite, header, include) for owned headers in different batches, `check` requires each prerequisite directly and `doctor --fix` appends the missing ones to `batches.json`. The removed ST06 -> ST03 edge was test-only (the SA-IS tester includes the suffix array) and formed a cycle with the new ST03 -> ST06.
- Notebook: a ticked header ticks its unticked closure, placed before it. `save --select` inserts each missing dependency before the first source that needs it and reports the additions, by default (user decision 2026-10-24); `--no-deps` keeps the selection as given. A later-selected dependency is not moved; a dependency that is not a selectable source fails the save with both names, and the UI label names it. Each section's `Needs:` line lists direct includes, the template only when unselected, with `(not selected)` marks. Unticking a dependency stays possible.

## Run on 2026-10-24

The graph has 177 headers (14 aggregates) and 580 include edges (162 into the template; 357 and 160 outside the aggregates). 58 edges between batches (53 batch pairs) were not direct prerequisites, and 47 of those edges were not even transitive; the brief's 44 counted differently. 11 of the 58 were implied by other prerequisites. Three schedule inversions between verified packages remain (P014 and P017 in Foundations re-audit include headers of P027 and P081 in Contest core) and are allowed because each pair has a verified package. `doctor --fix` added the 53 pairs, and `check` passes. The largest closures are `05-Mathematics/12-combinatorics_advanced.hpp` (13 files, 5403 lines), `03-Geometry/19-lattice_geometry.hpp` (3268) and `03-Geometry/14-minimumenclosingcircle.hpp` (3039).

## Feature-to-test map

Testers: [04-dependencies_tester.py](<../../96-Local Testing/00-Tools/04-dependencies_tester.py>) (32 tests; quick runs the `-MM` and compile oracles on a 15-header sample and every other repository oracle on all headers with 20 random DAGs; full and stress run both compiler oracles on every header, with 100 or `--rounds` DAGs plus on-disk random fixtures in stress) and [03-notebook_tester.py](<../../96-Local Testing/00-Tools/03-notebook_tester.py>). Oracles: an independent `os.walk` include scanner, `$CXX -std=gnu++20 -MM` closures, each closure concatenated in typing order without its quoted includes and compiled with `-fsyntax-only` in an empty directory (174 non-aggregate headers on 2026-11-05), iterative post-order and fixed-point reachability on random DAGs, raw newline counts, and a separate owner and prerequisite computation for `plan.py`.

| Feature | Tests |
|---|---|
| `headers` (normalising `../`, 97-Legacy at any depth, angle, indented and commented includes, other suffixes and folders, CRLF) | `test_headers_scan_normalisation_and_exclusions`, `test_headers_scan_empty_root`, `test_repo_graph_matches_scanner` |
| `closure` (chain, diamond, dedup, unknown include as a leaf, typing order) | `test_closure_chain_diamond_dedup`, `test_repo_typing_order`, `test_random_dags`; all 188 headers (2026-11-05) equal `-MM` with g++ and g++-14: `test_repo_mm_closure_all` (`_sample` in quick) |
| `lines` (only `\n` ends a line, a missing final newline counts) | `test_lines_count`, `test_repo_line_counts` |
| `strip_cpp` (layout kept; comments, literals, raw strings, digit separators; directive comments including multi-line ones, directive literals), `includes` (`#if` depth, comments), conditional headers | `test_strip_cpp_keeps_layout_and_directives`, `test_strip_cpp_directive_comment_regressions`, `test_strip_cpp_splices_and_unterminated`, `test_includes_comments_literals_and_depth`, `test_headers_conditional_and_legacy_root` |
| typing order compiles | `test_repo_compile_closure_all` (`_sample` in quick) |
| `describe` (plain closure, no Compact or mini note, closing `10-grab.py` line; on disk in stress), `chart` (exact edge set, no template edge or aggregates, subgraphs, isolated count), CLI exit codes | `test_describe_text`, `test_repo_describe`, `test_random_dags`, `test_chart_edges_groups_and_isolated_count`, `test_cli_fixture_exit_codes_and_output`, `test_cli_repository` |
| `plan.py` `include_edges` (unowned includes ignored), `check` (direct prerequisite required even when reachable transitively; late only while both open; batch cycles), `rank` (moves nothing on the repository, keeps waived inversions, fixes open ones), `set` (re-ranks: a started package precedes the planned ones), `doctor --fix` (restores, idempotent, `fix=False` writes nothing) | `test_plan_include_edges_oracle`, `test_plan_include_edges_unowned_files`, `test_plan_check_real_copy_passes`, `test_plan_check_undeclared_include_edge`, `test_plan_check_late_only_while_both_open`, `test_plan_check_batch_cycles`, `test_plan_check_conditional_include`, `test_plan_rank_keeps_schedule_and_waived_inversions`, `test_plan_set_reranks`, `test_plan_doctor_fix_restores_prerequisites` |
| Notebook `with_dependencies`, `needs`, `Needs:` lines, `data-deps`, `title` and tick handler, CLI `save --deps/--no-deps` (no `Added dependencies:` line when the save fails), unselectable dependency (error and label warning), tick handler under `node` | `test_with_dependencies_typing_order_and_user_order`, `test_needs_marks_unselected_and_template`, `test_generate_writes_needs_lines`, `test_page_dependency_attributes_and_script`, `test_cli_save_adds_dependencies_unless_disabled`, `test_unselectable_dependency_errors_and_warns`, `test_page_script_ticks_dependencies_under_node` (SKIP: no `node`) |

## Commands and results (2026-11-05)

```bash
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode quick                    # PASS, 30 tests, 32 s, peak 1331 MB
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full                     # PASS, 32 tests, 104 s, peak 1174 MB
CXX=g++-14 python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode full          # PASS, 32 tests, 127 s, peak 1313 MB
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode stress --seed 4 --rounds 200   # PASS, 32 tests, 100 s, peak 1182 MB
python3 '96-Local Testing/00-Tools/03-notebook_tester.py'                                     # PASS, 12 tests, 0 SKIP, peak 34 MB
python3 '00-Guidelines/13-Plan/plan.py' check                                                 # plan check: ok
```

## Limits

- The tick handler runs under `node` with a small DOM shim (`test_page_script_ticks_dependencies_under_node`); without `node` the test prints SKIP. The user installs `nodejs` (user decision 2026-10-24); until it has run, the handler is unverified.
- `.cpp` and Python sources are not followed: `01`–`07` hold only headers, and `08-Python` has no modules yet; Python imports belong to the Python contest layer.
- The notebook selects real includes: a printed header is the whole file; extracting only what a name needs is `10-grab.py`'s job.

## History

- 2026-10-24: P238 introduced the graph, the include-edge and cycle checks and notebook dependency selection; review fixed 6 findings (a batch cycle, the late-check waiver and `rank`, test gaps, counts); all testers passed on g++ and g++-14.
- 2026-10-24: `/finish` audit and diff review: 14 defects fixed (line counts, mini notes, shared literal-aware stripper, directive comments and splices, a repeated scan, repeated decisions, rank wording, tester docstring and summary line, a legacy-path test, an unrecorded claim); user decisions recorded; added a compile oracle for typing order, comment, any-form include and `#if` handling, Compact needing a reduction, the unselectable-dependency message and a node test (SKIP until installed); all testers passed on g++ and g++-14.
- 2026-10-25: P247 `/finish` (user decision 2026-10-25): `plan.py set` now re-ranks before saving. A package set to in-progress out of rank order had made `test_plan_rank_keeps_schedule_and_waived_inversions` fail until `plan.py rank` ran.
- 2026-11-05: P253 retired the substitutes (TESTED_MINIS, Compact-skippable includes, mini notes); extraction by 10-grab.py replaces them; testers passed on g++ and g++-14.
