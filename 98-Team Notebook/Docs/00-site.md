# Documentation Site — Evidence (P239)

Scope: the documentation-site generator [06-site.py](../06-site.py) for Zensical or MkDocs Material, with its pages, charts, link rewriting and article merge (P239, support batch SUP08). Reasons: [decision log](../../00-Guidelines/10-decisions.md), Dependency graph and documentation site. Rules: [07-notebook.md](../../00-Guidelines/07-notebook.md).

## Contracts

- Location (user decision 2026-10-25): the generator lives in this folder and writes the git-ignored `site/`, which holds `mkdocs.yml`, `docs/`, `html/` and the engines' `.cache`. `generate` uses the standard library only. It builds every page first and then replaces `site/docs`, so no stale page survives.
- Engines (user decisions 2026-10-25):
  - The generated `mkdocs.yml` serves both Zensical and MkDocs Material.
  - `build` runs `<engine> build -f site/mkdocs.yml --strict`. `serve` generates once and runs `<engine> serve -f site/mkdocs.yml --dev-addr 127.0.0.1:<port>`; restart it after edits.
  - `--engine auto`, the default, takes the first usable engine: Zensical, then MkDocs. An engine is usable when it is on PATH and the Python on its `#!` line imports what the site needs: `zensical`, or `material` and `pymdownx`. Arch's `mkdocs-material` package does not pull in `python-pymdown-extensions` (found 2026-10-25).
  - With no usable engine, the command names each engine's problem and how to install it, and exits 1.
- Pages: one per inventory row (plan.py `inventories`) whose header or module exists, at `<folder, lowercase with hyphens>/<NN-stem>.md`, the evidence document's name. A page holds, in order:
  - an H1 with the file name;
  - a facts line with folder, tier, Core's Basic flag, status, line count, last commit date (`git log %cs`; omitted outside git) and the GitHub source and evidence links;
  - the `#include` line (headers), then the article;
  - `## Operations`, one bullet per `;` outside parentheses and brackets, where a half-open `[l, r)` counts as a bracket pair;
  - `## Complexity` (user decision 2026-10-25), which lists each declaration with its complexity comment, every `// T:` or `// S:` line of the header (`03-cpp.md`, Complexity Comments). The declaration is the code after that line up to the first `{` or `;` outside parentheses. Each is one code span whose delimiters outrun any backtick inside (`05-Mathematics/10-crt.hpp` has some);
  - the evidence sections, copied verbatim with their titles: `## Contracts`, then `## Dependencies` (headers in the include graph), `## Limits and Handoffs` and `## Sources`;
  - `## Tests`, the tracked files of plan.py `tests_for`;
  - `## Source`, the file in a collapsed `??? abstract` block, fenced by more backticks than any run inside it.
- Dependencies come from [05-dependencies.py](../05-dependencies.py) `closure`. The section lists the direct includes, the typing order with line counts and total, and the direct includers except aggregates, each with its row status. A sentence points to `09-Contest Testing/10-grab.py`, linked on GitHub once tracked, for extracting only the declarations a name reaches. Python imports are not followed, since no module exists yet.
- Folders: `index.md` holds the inventory Scope paragraph, a link to the notes page when `Docs/00-notes.md` exists (copied whole) and the folder article. For `01`–`07` it adds the `chart` Mermaid flowchart, or a sentence when every header includes only the template. The inventory tables follow per tier, with header cells linked to existing pages. The home page counts rows, pages and verified rows per folder. The nav lists the home page, then per folder its overview, notes and pages in inventory order, grouped by tier except in Core.
- Articles (user decisions 2026-10-25):
  - `Articles/<NN-Folder>/<NN-stem>.md` is merged into that header's page after the include line, and `Articles/<NN-Folder>/00-index.md` into the folder overview after the notes link, each without its first `# ` line.
  - Other files under `Articles/<NN-Folder>/` are assets such as images, copied beside the folder's pages.
  - An article without an existing inventory header, module or folder, or a file outside a known folder, raises `SiteError` naming it.
- Links: copied Markdown (scope, cells, evidence sections, notes, articles) is relinked relative to its own file.
  - Site targets: a header, its evidence document or article, a folder and its index, article or notes, and an article asset. The rewrite targets the site page when the target has a page and the anchor exists there.
  - Anchors into a copied document (evidence, article, notes, and the inventory, whose tier headings head the tables) are that document's own heading ids. The page maps each to the id the same heading gets on the page, which differs when an earlier part repeats the heading (user decision 2026-10-25). A link to a heading the page does not copy goes to GitHub.
  - Repository links: any other link goes to `<origin>/blob|tree/master/<path>`, with `?raw=true` for an image, when the path is tracked (`git ls-files`; outside git, any existing path) and an `origin` remote is set. Credentials are removed from the remote. Failing that, the label stays as plain text.
  - Unchanged: external, `mailto:`, escaped (`\[`), inline-code, inline-math and fenced links.
  - Code spans are found as Python-Markdown 3.11 finds them. A backtick run closes at the next run of its length, else at the first longest later run, and an escaped backslash pair before a backtick does not escape it. In a table block they are found per cell, with rows split as the tables extension splits them (user decision 2026-10-25); elsewhere per line. A table block passes that extension's test: a header of two or more cells, or one bordered, and a separator row. A block that is no table splits at its first hash heading.
  - Heading ids follow the renderer, ported from Python-Markdown 3.11 (user decision 2026-10-25). Hash headings need no space and drop a closing run of `#`. An unpaired trailing backslash makes no heading, and a hash line inside a table is a row.
  - The inline patterns run in the renderer's order with its stash placeholders: code span, arithmatex math (`$…$`, `\(…\)`), escape (`$` included), link, image, autolink, inline HTML, entity, then the delimiter emphasis of `*` and `_` (`DelimiterProcessor`). The inline tree processor's walk then reruns them on element texts and tails.
  - The toc text is the serialized inner HTML with escapes and raw HTML restored, then `strip_tags` (comments, then `<` to the next `>`, whitespace collapsed), then entities decoded. Repeats are counted with ASCII digits. The result goes through `pymdownx.slugs.slugify(case="lower")`, GitHub's rule as configured in `mkdocs.yml`, with `_1` suffixes for repeats.
  - Fences follow `pymdownx.superfences`. A run of three or more backticks or tildes, an optional language, then `{attributes}` or only `pymdownx.highlight` options with valid values (`hl_lines`, `linenums`, `title`, `title_mode`) opens a fence only when a later bare line of exactly that run closes it.
  - `validation.anchors: warn` with `--strict` fails the build on any anchor the site lacks.
- Rendering:
  - Search with the `offline` plugin, light and dark palettes, heading permalinks, code copy buttons, and Mermaid through a `superfences` custom fence.
  - Math: `pymdownx.arithmatex` (generic) with MathJax 3 for `$…$` and `$$…$$`, configured as cp-algorithms does but loading the SVG build `tex-svg-full.js`, which needs no font files and bundles every TeX extension.
  - The `privacy` plugin (user decision 2026-10-25) self-hosts every font and script the pages load under `assets/external`: Google Fonts, MathJax, Mermaid and the `iframe-worker` shim that `file://` search needs. The tester checks MathJax, Mermaid and the shim there. The first build downloads them; later builds reuse `site/.cache`, since the engine runs in `site/`. Both engines honour the plugin.
  - `03-consistency.py` skips `site/`; the tester checks its Markdown hygiene.

## Feature-to-Test Map

Tester: [06-site_tester.py](<../../96-Local Testing/00-Tools/06-site_tester.py>), 63 tests, 62 of them in quick mode.

- quick: unit and fixture tests; `engine()` and the CLI on fake engine scripts, never starting a real one; 5 random fixture trees; every repository oracle on a temporary generation; and the strict fixture builds once per engine executable found (every `zensical` or `mkdocs` on PATH, plus `CP_SITE_ENGINES`).
- full adds 20 random trees and, per engine, the strict repository build with its built heading ids.
- stress adds `--rounds` random trees (default 200), three of them built strictly with every engine.
- A missing engine SKIPs its build tests.

Oracles:

- Python-Markdown and pymdown-extensions themselves, run by an engine's `#!` Python, for fixed and seeded fuzzed headings (emphasis, code spans, escapes, fence headers) and for table rows rendered before and after relinking.
- Hand-computed fixture pages and engine messages.
- An independent heading-id and fence scanner (the pre-3.11 code-span regex and a one-regex emphasis, not the generator's port), validated against Python-Markdown and the built HTML.
- Copied documents located on their pages by their lines; an independent link resolver; a regex inventory parse; an `os.walk` include scanner; an independent complexity-line parser; byte newline counts; `git log -1` per file.

Tests whose docstrings start with "Regression (P239)" reproduce the defects fixed in P239.

| Feature | Tests |
|---|---|
| `Emphasis`, `Heading` (delimiter emphasis, the inline patterns and tree walk, toc text), trailing-backslash headings, table rows, ASCII repeat counts, repeated fence options | `test_emphasis_python_markdown`, `test_heading_pipeline_python_markdown` |
| `cells`, `table_rows`, `relink` in tables and math (cell code spans, escaped links, inline math, dropped extra cells) | `test_table_cell_links`, `test_math_and_escaped_links`, `test_relink_cases` |
| `slug`, `spans`, `anchors`, `fenced` (renderer rules, code-span rule, fence headers, repeats, unclosed and mismatched fences) | `test_slug_cases`, `test_code_span_rule`, `test_anchors_cases`, `test_anchors_python_markdown_domain`, `test_anchors_python_markdown_edge_cases`, `test_repo_heading_ids`, `test_random_trees` |
| `sections` (verbatim spans), `trimmed`, `items`, `page_name`, `folder_slug` | `test_sections_cases`, `test_repo_evidence_sections`, `test_trimmed_cases`, `test_trimmed_title_after_blank_lines`, `test_items_cases`, `test_repo_operations`, `test_names`, `test_repo_pages_match_inventory` |
| `complexity`, `literal` (Complexity section; backticks in comments) | `test_complexity_cases`, `test_header_complexity`, `test_repo_complexity`, `test_repo_complexity_code_spans`, `test_random_trees` |
| `git`, `repository` (scp and `ssh://` remotes, trailing slashes, credentials), `changed`, `tracked`, git missing | `test_repository_urls`, `test_changed_dates`, `test_header_last_commit_git_fixture`, `test_repo_last_commit`, `test_tracked_files_only`, `test_missing_git` |
| `Site` pages and targets, `href`, `link`, `relink` (code spans, images, missing pages and anchors, no remote) | `test_site_pages_targets`, `test_href_and_link`, `test_relink_cases`, `test_relink_code_span_rule`, `test_overview_cell_anchor_links`, `test_repo_links_resolve`, `test_random_trees` |
| `map_ids`, per-source anchors (repeated headings, inventory tier headings beside a folder article) | `test_anchors_per_source`, `test_folder_article`, `test_inventory_anchor_beside_folder_article`, `test_repo_copied_anchor_links`, `test_random_trees` |
| Folder overview and folder article, header and module pages, section order, dependencies, home | `test_overview_pages`, `test_folder_article`, `test_header_page_dsu`, `test_header_page_python`, `test_header_section_order`, `test_dependencies_sections`, `test_repo_dependencies`, `test_home_page` |
| `files` (nav, YAML, plugins, orphan articles and assets), article assets and images, `generate` | `test_files_nav_yaml`, `test_files_orphan_articles`, `test_article_assets_and_images`, `test_generate_writes_only_out` |
| `engine` (preference and default, fallback, forcing, missing modules, a missing `#!` interpreter, non-Python `#!`), `run` and the CLI (arguments, working directory, exit codes) | `test_engine_choice`, `test_cli_generate`, `test_cli_engine_commands`, `test_cli_missing_engines`, `test_cli_orphan_exit`, `test_cli_honours_patched_site` |
| Generated Markdown hygiene (the `03-consistency.py` rules) | `test_fixture_markdown_rules_and_links`, `test_repo_markdown_rules` |
| Strict builds per engine (fixture, broken anchor and missing page fail, privacy leaves no CDN URL and self-hosts MathJax SVG, Mermaid and the search shim, repository with built ids) | `test_build_fixture_strict`, `test_build_broken_anchor_fails`, `test_build_privacy_self_hosted`, `test_repo_strict_build_and_ids` |

## Commands and Results

Latest run, 2026-11-10 (P274, [00-parallel.md](<../../96-Local Testing/Docs/00-parallel.md>)): under `01-run.py`, `06-site_tester.py` passed 62 tests in warm quick mode and 63 in cold full mode, on g++ and g++-14. P274 also added the network retry of a strict build (Limits).

The site run of 2026-10-26: generation took 2.3 s for 180 pages (1 home, 8 overviews, 8 notes, 163 headers; 3.9 MB of Markdown). On every engine the heading ids of all 180 built pages equalled the generator's, and the built HTML loaded no CDN URL. The engines were Zensical 0.0.72 and the system MkDocs 1.6.1 with Material 9.7.7 and pymdown-extensions 12.1 (Arch packages); the tester's full build also ran MkDocs 1.6.1 with Material 9.8.0 in a scratch venv. All use Python-Markdown 3.11.2. The Zensical and MkDocs venvs live in `/tmp/p239-zensical` and `/tmp/p239-mkdocs`, outside the repository.

`ALL=/tmp/p239-zensical/bin/zensical:/tmp/p239-mkdocs/bin/mkdocs`. The default PATH holds the system `mkdocs` only. `NONE=/usr/local/bin:/tmp/p239-nothing`, where the second directory links only `python3`, `git`, `systemd-run`, `systemctl` and `true`; the memory cap needs the last three. Results are from 2026-10-26 unless dated otherwise.

```bash
python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode quick                                   # 2026-11-05 (P253): PASS, 62 tests, 3 SKIP (zensical), 23.7 s, peak 145 MB
python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode full                                    # 2026-11-05 (P253): PASS, 63 tests, 4 SKIP (zensical), 44.6 s, peak 212 MB
CP_SITE_ENGINES=$ALL python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode quick              # PASS, 62 tests, 3 engines, 35.3 s, peak 166 MB
CP_SITE_ENGINES=$ALL python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode full               # PASS, 63 tests, 3 engines, 120.5 s, peak 430 MB
PATH=$NONE python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode quick                        # PASS, 62 tests, 6 SKIP, 12.9 s, peak 83 MB
PATH=$NONE python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode full                         # PASS, 63 tests, 8 SKIP, 34.2 s, peak 83 MB
CXX=g++-14 CP_SITE_ENGINES=$ALL python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode full    # PASS, 120.2 s, peak 426 MB (no compiler is used)
CP_SITE_ENGINES=$ALL python3 '96-Local Testing/00-Tools/06-site_tester.py' --mode stress --seed 5 --rounds 100   # PASS, 146.3 s, peak 435 MB
python3 '96-Local Testing/00-Tools/04-dependencies_tester.py' --mode quick                           # PASS (plan.py row_status refactor), peak 1170 MB
python3 '96-Local Testing/01-run.py' --mode quick --filter 00-Tools --no-integration                 # PASS, 6 suites
python3 '98-Team Notebook/06-site.py' build                                                          # Using mkdocs (/usr/bin/mkdocs), strict build passed, 7.3 s
PATH=/tmp/p239-zensical/bin:$PATH python3 '98-Team Notebook/06-site.py' build                         # Using zensical, strict build passed, 5.3 s
PATH=/tmp/p239-zensical/bin:$PATH python3 '98-Team Notebook/06-site.py' serve --engine zensical --port 8765   # HTTP 200 on 127.0.0.1:8765 after 4.5 s
python3 '98-Team Notebook/06-site.py' serve --engine mkdocs --port 8766                               # HTTP 200 on 127.0.0.1:8766 after 8 s
python3 '00-Guidelines/13-Plan/plan.py' check                                                        # plan check: ok
python3 '96-Local Testing/03-consistency.py'                                                         # no errors in this checkout (7 from another session's worktree under .claude/worktrees)
```

The strict builds include generation. `serve` answered on `127.0.0.1` only.

## Completeness Sweep

`@researcher` compared the planned features with the generated sites of Nyaan, ei1333, hitonanode, suisen and maspypy (online-judge-verify-helper), cp-algorithms, the AtCoder Library documentation and KACTL; the `/finish` audit added OI Wiki, another Material site. Adopted: the `#include` line, status beside dependency links, last commit dates, provenance (the evidence Sources section), dark mode and permalinks, and cp-algorithms' MathJax configuration. Not adopted:

- Bundled listings (the closure expanded into one file): each page lists the typing order with links, and a bundle would repeat up to 5403 lines per page.
- A page per tester: the Tests list links each tester.
- Tags: tier and status already group the nav and tables.
- OI Wiki's article markup extensions (`footnotes`, `def_list`, `pymdownx.tabbed`, `tasklist`, `snippets`), Disqus comments and a feedback widget: article markup is chosen with the first articles (P240), and comments do not suit a local site. OI Wiki self-hosts MathJax, as the `privacy` plugin does here.
- Usage examples, practice problems, conventions and install pages: hand-written content for the P240–P246 articles.
- KACTL's per-file hash: a notebook feature, not a site feature.
- Authors (needs an API key) and RSS: no use for a local library site.
- Per-operation signature tables (AtCoder, ei1333): rows name operations without signatures, and contracts stay one block per header from the evidence document.

## Sources

| Source | Use |
|---|---|
| [cp-algorithms mkdocs.yml](https://raw.githubusercontent.com/cp-algorithms/cp-algorithms/main/mkdocs.yml) | arithmatex generic with MathJax 3 from unpkg, highlight, superfences, details, admonition, toc permalinks, code copy |
| [cp-algorithms: Segment Tree](https://cp-algorithms.com/data_structures/segment_tree.html) | Article layout, last update date, practice-problem lists (P240–P246) |
| [Nyaan's library page](https://nyaannyaan.github.io/library/modint/montgomery-modint.hpp), [ei1333](https://ei1333.github.io/library/structure/segment-tree/segment-tree.hpp), [suisen](https://suisen-cp.github.io/cp-library-cpp/library/datastructure/segment_tree/segment_tree.hpp) | Include line, depends-on and required-by lists with status, verified-with, code and bundled code, last update |
| [AtCoder Library document](https://atcoder.github.io/ac-library/production/document_en/segtree.html) | Per-operation constraints and complexity (not adopted) |
| [KACTL](https://github.com/kth-competitive-programming/kactl) | Source, license and per-file hash conventions (not adopted) |
| [Material for MkDocs](https://squidfunk.github.io/mkdocs-material/) and its [blog](https://squidfunk.github.io/mkdocs-material/blog/) | Theme, search, offline and privacy plugins, Mermaid and math setup; maintenance mode since 2025-11-11 with critical and security fixes for at least 12 months |
| [Zensical migration guide](https://zensical.org/docs/compatibility/mkdocs/migration/) | Reads `mkdocs.yml`; `hooks` unsupported (unused here); plugin support not listed, so it was tested (Commands) |
| [hitonanode](https://hitonanode.github.io/cplib-cpp/), [maspypy](https://maspypy.github.io/library/), [online-judge-verify-helper](https://online-judge-tools.github.io/verification-helper/document.html) | Generated library sites: verification-file index, front-matter titles, MathJax |
| [OI Wiki mkdocs.yml](https://raw.githubusercontent.com/OI-wiki/OI-wiki/master/mkdocs.yml) | Material with self-hosted MathJax, `uslugify` toc slugs, article markup extensions (not adopted yet) |
| Python-Markdown 3.11.2 and pymdown-extensions 12.3 (installed source, `/tmp/p239-mkdocs`) | `toc` heading text and ids, hash-header rule, `pymdownx.slugs`, `superfences` fence rule; checked by rendering, not by reading their documentation |

## Limits

- Zensical 0.0.72 is marked alpha on PyPI ("Development Status :: 3 - Alpha") and is installed with pipx, since Arch has no package. Material for MkDocs 9.8.0 announces its end of life on 2027-05-05; until then MkDocs stays the fallback engine.
- The first build after a cache wipe needs network for the `privacy` downloads. The offline pages were not opened in a browser, so math and diagram rendering from `file://` is shown only by the files being present.
- Zensical's privacy plugin also follows URLs inside its own bundle: its `assets/external` held 46 files (7.2 MB, including video-player and ad SDKs that no page loads), against 35 for MkDocs.
- `06-site_tester.py` retries a strict build lost to a network failure (name resolution, a font download timing out) up to four times, then reports SKIP. Any other warning fails at once, including a broken link beside a lost download (P274, 2026-11-10).
- Repository links point at `master` on `origin`, so files not yet pushed return 404 there. Commit dates ignore uncommitted edits.
- Mermaid nodes are not links: Material's Mermaid integration does not set `securityLevel`, and Mermaid's default `strict` disables click links.
- Heading ids follow Python-Markdown for hash headings in top-level text only. They do not cover:
  - headings inside block quotes, admonitions or list items;
  - setext headings (`T` over `---`);
  - a heading after a raw HTML block such as `<details>`, which Python-Markdown does not treat as a heading;
  - links outside the plain `[label](url)` form: a title (`[a](b "t")`), brackets inside the label, parentheses inside the URL, images (whose alt text Python-Markdown drops), and raw HTML holding code spans;
  - a `$$` block holding `#` lines;
  - fences whose reading depends on context: a fence indented one to three spaces outside a list, a fence closed by an indented run, or one indented four spaces inside a list item.

  These cases were checked by rendering on 2026-10-25, and no copied document has them. A link to such a heading goes to GitHub, and a page anchor that does not exist fails the strict build. The heading pipeline matched Python-Markdown on 24 000 fuzzed emphasis headings and on 15 000 token-fuzzed headings mixing code spans, escapes, math, entities, inline HTML, comments and plain links. Character-level fuzz with nested brackets and parentheses in links differed in up to 0.6 % of cases.
- Indented code blocks are not recognized, so a link in one is rewritten and its displayed code changes; no copied document has one. Reference links, `<user@host>` autolinks and the link forms listed above are outside the heading-id and relink domain.
- Outside tables, code spans are found per line. A span that Python-Markdown closes on a later line of the same paragraph therefore leaves a link inside it rewritten, or one after it unrewritten; no copied document has one.
- A complexity declaration is cut at the first `{` or `;` outside parentheses within the next 11 lines, comment lines included. A brace in a template argument or a longer declaration would shorten it; none of today's 1284 declarations has one.
- `engine()` reads the Python from the `#!` line. pip writes a `#!/bin/sh` launcher for a venv path with spaces or over 127 characters; such an engine skips the module check, and a missing module shows only when the build fails.
- `repository()` reads any `host:path` remote as scp syntax, so an SSH alias (`github-work:u/r.git`) becomes `https://github-work/u/r`.
- Module pages and a Python Dependencies section are exercised only by fixtures until `08-Python` has modules.

## History

- 2026-10-25: P239 introduced the generator, header and folder pages, include charts, link rewriting and article merge; the new tester found 4 defects, all fixed; all modes passed with and without MkDocs; review: 8 findings fixed (Operations split at `[l, r)`, links to untracked files, heading-id escapes and emphasis order, section titles kept, article images and assets, evidence figures, pipx venv detection, credentials, missing git, index order, shared row status).
- 2026-10-26: /finish: Zensical and MkDocs engines, self-hosted assets, per-source anchors, Complexity section, folder articles; Python-Markdown's heading pipeline and table cell rule ported (user decisions); tester updated (5 defects found and fixed); two reviews, 16 findings fixed; all modes passed on three engines and none.
- 2026-11-05: P253 retired the Compact-only reduction includes and untested-mini notes from the Dependencies section, which now points to `10-grab.py`; default-PATH quick and full passed.
- 2026-11-10: P274: `06-site_tester` retries a strict build lost to a network failure up to four times, then reports SKIP; quick (62) and full (63) passed on g++ and g++-14.
