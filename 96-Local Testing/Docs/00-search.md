# Concept search — evidence (P263)

Scope: `grab --find WORDS` and the inventory rows grab lists below an unknown name's spelling suggestions, implemented in [`_11_search.py`](<../../09-Contest Testing/_11_search.py>) and wired into [`10-grab.py`](<../../09-Contest Testing/10-grab.py>) (P263, support batch SUP24; user decision 2026-11-06: part of grab, not a new tool). The Operations-cell parser (`cells`, `split`, `unqualified`, `depths`, `operations`) moved here from [`05-extraction.py`](../05-extraction.py), which imports it, so the inventory has one parser. Rules: [08-contest-tools.md](../../00-Guidelines/08-contest-tools.md) (Extraction paragraph). Extraction itself: [00-extraction.md](00-extraction.md).

## Contracts

### Documents

- One document per inventory row of `01-Core` … `08-Python` (`0[1-8]-*/00-index.md`, rows whose first cell is a `.hpp` or `.py` code span). Planned rows, `08-Python` modules and the template and debug headers are included (user decision 2026-11-07 for the last); `show` marks a module with `Python` and a header the contest template already holds (grab's `EXCLUDED`) with `in template` after its status.
- Tier: the current `## Basic|Advanced|Esoteric` section; a `01-Core` row, which has no tier section, is `Basic` when its Basic cell starts with the word `yes` (`yes until mini exists`, and `yes when implemented` for a planned mini), else `Advanced` (user decision 2026-11-07). Status: the first of `verified`, `partial`, `existing-unverified`, `legacy-reference`, `planned` the Status cell starts with.
- Fields: `stem` (the file stem without `NN-`/`_NN_`, split at `_`; a stem part that is no known word but joins two or more known words of `PART` (2) letters or more, fewest first, also yields them: `segmenttree` → `segment tree`; a known word is a token of `PART` letters or more in any row's `ops`, `qual` or `aliases`), `ident` (the stem without `_` and every grab name's `::` parts whole except `operator…` parts, lower case, split at spaces only: `dialdijkstra`), `ops` (the Operations cell without qualifiers), `qual` (the parenthesized qualifiers), `aliases` (below), `folder` (the folder name without its prefix). Every field's text passes `words` and `stem` below, except `ident`, which is only stemmed (`dialdijkstra` stays one token, `segmenttrees` would lose its `s`).
- Aliases: from the row's evidence document only (`<folder>/Docs/<NN-name>.md`; for a module, when `08-Python` gains them, `08-Python/Docs/<module stem>.md`, as `_03_number_theory.md`) — every line of a section whose heading starts with `Aliases`, and every paragraph or list opened by a line `<optional word> Aliases …:` with no backtick before the colon (`Aliases:`, `Catalog aliases:`, `- Aliases (2026-10-19 sweep):`, `- **Aliases elsewhere:**`, `Search aliases (P263, grab --find):`), until a blank line or heading; the opening label up to its colon (and closing `**`) is dropped from each line, so `Search`, `P263` or `grab` index nothing; URLs and link targets dropped. `Aliases with \`FIXED\`:` opens nothing.
- Names (what a hit offers to grab), in cell order: a type head (capitalized, `::`-qualified or marked `(alias`, as `iint (alias)`, which a label `aliases:` is not; a head of several words gives its capitalized ones, `A as B and C`) and `Type::member` for its entries (`ctor` names the type; an entry the cell qualifies itself stays as written, `PolyCompanion::fft`, and a `std::` entry is a specialization, no name); a camelCase function head alone (`cartesianTree (cmp): parent, root` → `cartesianTree`); the entries of a lower-case label head (`required:`) or of no head. Template arguments are stripped; operators stay `Type::operator+=`.

### Tokens

- `words`: lower case; identifiers split at `_`, `-`, camelCase, acronym runs (`LCPArray` → `lcp array`) and letter-digit boundaries; stop words (`a an and as at by for from in into is it of on or the to via with`) dropped.
- `stem` (light): `ies` → `y` and `ches/shes/sses/xes/zzes` lose `es` (words over 4 letters), a final `s` goes unless after `s`, `u` or `i` (words over 3 letters), then `-ing` goes from words of 7+ letters with a doubled consonant other than `l`, `s`, `z` undone (`spanning` → `span`, `rolling` → `roll`, `agreeing` → `agree`); irregular: `vertices`, `indices`, `matrices`, `series`, `species`, `tries`.
- Typo tolerance per query token absent from the vocabulary: optimal string alignment distance (adjacent swaps count 1) at most 0 up to 3 letters, 1 up to 7, 2 from 8 (`TOLERANCE`); a match at distance 1 counts 0.7, at 2 0.5. A token of `PREFIX_MIN` (2) letters or more also matches the tokens it begins at 0.75 (prefix). Each query token takes its best alternative per row.
- Joins: every 2 and 3 adjacent query words joined (`block cut tree` → `blockcuttree`) are extra query tokens, matched as typed or within typo tolerance, never as a prefix.

### Ranking

- Score: BM25F, `idf = ln(1 + (N − df + 0.5)/(df + 0.5))`, pseudo frequency `Σ_f w_f · tf_f / (1 − b + b · len_f/avg_f)`, saturation `x(k1 + 1)/(k1 + x)`, summed over query tokens; plus, for each adjacent query pair adjacent in some field, `phrase · (idf_a + idf_b)/2`; times the row's status factor and tier factor (`STANDING`, `LEVEL`), so finished and simpler rows rise (user request 2026-11-07).
- Order: (1) rows whose stem tokens equal the query's (`montgomery` lists `04-montgomery.hpp` before `ModInt`'s `Context::montgomery`); (2) rows with a name equal to the query (case ignored, also its last `::` part); (3) for a one-word query, rows with a name holding it as a run of whole words (`bit` in `BitOps`, `segtree` in `SegTreeBeats`, not `sam` in `same`; words with spaces describe a concept, so `binary search` lifts no `parallelBinarySearch`); (4) the rest with a positive score. Only a C++ row that is `verified`, `partial` or `existing-unverified` takes classes 1 to 3: a module, a planned or a legacy-reference row has nothing grab copies, so it ranks by score alone. Classes 1 to 3 order by tier (Basic, Advanced, Esoteric), status (order above), score, header path: simplest and most finished first, so `bipartite` lists `bipartiteCheck` before `BipartiteMatching`. Class 4 orders by score, tier, status, header path. `-n K` (default 10) keeps the first K; nothing positive prints `no match`.
- Name shown: the equal name, else the shortest whole-word match, else the name sharing the most query idf, a name's words including its entry's parenthesized qualifiers (`miller rabin` shows `isPrime`, whose entry reads `isPrime (deterministic Miller-Rabin)`), then the most stem words, the first on a tie; the header path when no name shares a query word and the row is no own-stem match, so an alias-only match names no unrelated function (`kadane` shows `06-Miscellaneous/08-sequence_algorithms.hpp`), or when a row has no names (user decision 2026-11-07).
- Output line: name padded to 30, header to 46, tier to 9, status, then `  Python` for a module and `  in template` for a template or debug header. An unknown name's error (libclang path and whole-header fallback) appends `closest rows (name, header, tier, status):` and up to 3 such lines, indented.
- Cost and freshness: every run parses the inventories and evidence documents anew (466 rows, about 0.3 s) and needs no libclang, cache or index, so a status change, a new row or a new alias line ranks from the next run on; a query is about 10 ms. `test_repo_corpus` reads the live inventories, so a renamed or removed corpus target, or a ranking drop below a floor, fails the tester.

### Weights and cutoffs

The query list is the tester's full mode with `--seed 1` (`Real.corpus` in the tester): the corpus file's queries in file order, then the Library Checker problems in ledger order, then, source `generated`, `typo_variant(query, random.Random(1))` of each `lc` and `cp` query in that order (one random swap, deletion, substitution or insertion inside its longest word of 5+ letters, the first letter kept). The objective is the mean over the eight sources `cp oi abbr typo simple py lc generated` of (recall@5 + MRR@10)/2; "even" and "odd" are the queries at even and odd 0-based positions of the list.

- BM25F weights, typo and prefix factors and phrase bonus were tuned first, before the `simple` and `py` sources and the status factors existed, by coordinate descent (two rounds, one parameter at a time to the best value of its grid, the current value kept on a tie) in the order `stem ident ops qual aliases folder b k1 phrase python prefix t1 t2` over `stem` 1 2 3 4 6; `ident` 0 1 2 3 4; `ops` 0.5 1 1.5; `qual` 0 0.25 0.5 1; `aliases` 0.25 0.5 1 1.5 2; `folder` 0 0.3 0.6; `b` 0.3 0.5 0.75 0.9; `k1` 0.6 0.9 1.2 1.6 2.4; `phrase` 0 0.25 0.5 1 1.5; `python` (a module factor, removed once the `py` source drove it to 1) 0.5 0.7 0.8 0.9 1; `prefix` 0.3 0.45 0.6 0.75; typo factors `t1` 0.5 0.7 0.85, `t2` 0.3 0.5 0.7, from `stem 3, ident 2, ops 1, qual 0.5, aliases 1, folder 0.3, b 0.75, k1 1.2, typo 0.7/0.5, prefix 0.6, phrase 0.5`, on an earlier version of the list (Library Checker first, its own misspelling generator); a rerun on the final list was not made.
- Status and tier factors and the cutoffs on the final code and list (user decision 2026-11-07: "whichever is best"): every combination of planned and legacy-reference 1 0.9 0.8 0.7, existing-unverified 1 0.9, Advanced 1 0.9 0.8 and Esoteric 1 … 0.6 with planned ≤ existing-unverified and Esoteric ≤ Advanced (verified, partial and Basic 1); then, one at a time from the best combination, the first `TOLERANCE` length 2 3 4, the second 6 7 8 9, `PREFIX_MIN` 2 3 4 and `PART` 2 3 4. The best combination is planned 0.9, existing-unverified 1, Advanced 1, Esoteric 0.9 (0.8753; capping planned at 0.9, the user's request that planned rows rank below implemented ones, does not bind). Of the cutoffs, `PREFIX_MIN` 2 (0.8769 against 0.8765 for 3) and `PART` 2 (0.8769 against 0.8767) were taken. `TOLERANCE` stays (3, 7): (4, 9) scores 0.8769 on the list, but the list misspells only words of 5+ letters, and on 302 extra queries misspelled in a 4-letter word (each corpus and Library Checker query holding one, in list order, with `random.Random(2)`: a random such word, then a random swap at position 1 or 2 of differing letters, substitution at 1 to 3 or insertion at 1 to 4) a first length of 3 scores 0.917 / 0.792 against 0.864 / 0.755 for 4; the second length gains under 0.0002 from 7.

| Parameter | Value |
|---|---|
| Field weights | `stem 1, ident 1, ops 1, qual 0.25, aliases 2, folder 0.6` |
| `b`, `k1` | 0.5, 1.2 |
| Typo factors (distance 1, 2), prefix | 0.7, 0.5; 0.75 |
| Phrase | 1.0 |
| Status factors `STANDING` | verified 1, partial 1, existing-unverified 1, legacy-reference 0.9, planned 0.9 |
| Tier factors `LEVEL` | Basic 1, Advanced 1, Esoteric 0.9 |
| `TOLERANCE`, `PREFIX_MIN`, `PART` | (3, 7), 2, 2 |

Overfitting check: the same search on the even queries only chose planned 0.9, existing-unverified 0.9, Advanced 0.8, Esoteric 0.8 and `PART` 3; on the odd queries those values score 0.8676 (with `PART` 3), the final values 0.8729 and the final values without factors 0.8683. The two halves disagree on the factors, so their gain over no factors (0.8753 against 0.8696 on the whole list) is small, though the odd half confirms it.

### Decisions

Claude's, 2026-11-07, while the user was away; the user accepted those still listed on 2026-11-07 and decided the changes marked as theirs.

- Planned rows included, marked by status, and ranked below implemented ones by the status factor (user request); `08-Python` modules included, marked `Python`, with no factor of their own.
- Columns: name to grab, header, tier, status. Template and debug rows included and marked `in template` (user decision).
- A header stem equal to the query ranks first, before exact names (`--find mst` lists `06-mst.hpp` first though no name contains `mst`; `--find lcs` lists `19-lcs.hpp` before `StringHash::lcs`).
- "Substring name match" became a whole-word run, for one-word queries only: a plain substring put `DSU::same` first for `sam` and `list` names first for `lis`, and a run inside a longer name put `parallelBinarySearch` before `binSearch` for `binary search`.
- Name matches order simplest and most finished first (user request: `bipartite` lists `bipartiteCheck` first); concept matches keep the score order, softened by the factors, since a strong Esoteric match should still beat a weak Basic one.
- The status and tier factors and the cutoffs `TOLERANCE`, `PREFIX_MIN`, `PART` are the descent's choice (user decision 2026-11-07: "whichever is best"), the first tolerance length excepted for the reason above. Kept as set, since the corpus cannot measure them (user decision 2026-11-07, which chose tuning for the cutoffs only): grab's 3 concept rows below an unknown name (`CONCEPTS`), floors 0.02 below the results, the corpus mapped by a subagent from the inventories rather than by hand, and OI Wiki page slugs in place of article titles.
- An alias may name a related structure answering the same queries when the library has no row of its own (user decision 2026-11-07): `partition tree` on the wavelet matrix, marked as such; `cat tree` on the disjoint sparse table is that structure's OI Wiki name.
- Aliases were added as a `Search aliases (P263, grab --find):` line to each evidence document the corpus needed (62 documents: at the end of an existing aliases section, after an existing aliases paragraph, or in a new `## Aliases` section before `## History`; each with a History line); rows without an evidence document (planned or unverified, as `04-Graphs/15-hld.hpp`) get theirs in their own `/package` step 3. No alias was added for the `simple` and `py` sources, so they measure the ranker on aliases written without them.

## Evaluation

Oracle independent of the ranker: queries mapped to rows before any ranking result was seen. Corpus [`14-search_corpus.json`](<../00-Tools/14-search_corpus.json>) plus the Library Checker problems of [`library-checker-coverage.json`](../../00-Guidelines/Ledgers/library-checker-coverage.json) (query: the problem id with `_` as spaces; targets: its `targets`). The cp-algorithms titles and OI Wiki slugs were mapped by a subagent from the inventories alone, told not to open the search code or the alias sections, at most 4 central rows each; two stretch mappings (`Factoring Exponentiation`, `ds/block-list`) and 8 queries with no row were dropped. The `simple` source (59 short everyday queries, each mapped to the simplest row providing it: implemented before planned, lower tier first, then the most elementary) and the `py` source (30 `python <concept>` queries mapped to the `08-Python` module) were mapped the same way after the user's review; that subagent dropped 9 queries whose simplest row was ambiguous (`hashing`, `modint`, `fft`, …). A query counts as found when any target is among the hits.

Recall@5 / MRR@10 on the final code over that list (the tester's full mode, `--seed 1`), with and without the P263 alias lines and the status and tier factors:

| Source | Queries | No P263 aliases | No factors | Final |
|---|---|---|---|---|
| `lc` Library Checker | 250 | 0.796 / 0.671 | 0.892 / 0.798 | 0.896 / 0.799 |
| `cp` cp-algorithms titles | 158 | 0.873 / 0.750 | 0.975 / 0.860 | 0.975 / 0.888 |
| `oi` OI Wiki slugs | 233 | 0.768 / 0.638 | 0.867 / 0.715 | 0.871 / 0.728 |
| `abbr` abbreviations | 56 | 0.643 / 0.580 | 0.893 / 0.808 | 0.893 / 0.829 |
| `typo` hand misspellings | 10 | 0.900 / 0.850 | 1.000 / 0.900 | 1.000 / 1.000 |
| `simple` everyday queries | 59 | 0.831 / 0.736 | 0.949 / 0.782 | 0.949 / 0.821 |
| `py` Python modules | 30 | 0.967 / 0.667 | 0.967 / 0.784 | 0.967 / 0.645 |
| `generated` misspellings (seed 1; stress: seeds 7–9) | 394; 1182 | 0.822 / 0.688 | 0.914 / 0.813 | 0.921 / 0.822; 0.919 / 0.819 |

The P263 alias lines were written for the misses of the first five sources, OI Wiki slugs and Library Checker ids among them, so those sources are not held out from them and their final columns overstate unseen queries; the "no P263 aliases" column measures the ranker on aliases that predate the corpus, `simple` and `py` had no alias written for them, and the overfitting check above measures the weights. Floors the tester enforces (0.02 below the final column, rounded down; typo and generated fixed): lc 0.87/0.77, cp 0.95/0.86, oi 0.85/0.70, abbr 0.87/0.80, typo 0.90/0.90, simple 0.92/0.80, py 0.94/0.62, generated 0.88/0.79.

Remaining misses are mostly rows without an evidence document (HLD, link-cut tree, Euler tour tree, DLX, MDST, digit DP, the balanced-BST family, Core matrices mod 2), queries naming an algorithm the library lacks (MPM, Boyer–Moore variants) and Library Checker names whose target is one of several equally good rows (`suffixarray` maps to `12-sais.hpp` only).

## Feature-to-test map

Tester [`14-search_tester.py`](<../00-Tools/14-search_tester.py>), oracles written in the tester: hand tables (names, aliases, rows, stem), a character scanner (words), a memoized recursion of the OSA definition (distance, checked on known values), split enumeration (segment), BM25F recomputed from the formula over hand field lists, hand-derived orders on fixtures whose scores run against the classes, an independent table-line count, and the corpus.

| Feature | Tests |
|---|---|
| Moved parser (`cells`, `split`, `operations`) | `test_cells_split_operations` (and `13-grab_tester.py`'s harness tests) |
| Names | `test_names` |
| Rows: indexes, tiers, Core Basic column (`no` is Advanced), modules and their evidence documents, template and debug rows, status, fields, aliases source | `test_rows_fixture`, `test_repo_rows` |
| Alias forms and exclusions | `test_aliases` |
| Words, stem, distance (cap + 1 beyond cap), segment (`PART`), tolerance (`TOLERANCE`), query stripping | `test_words_*`, `test_stem_table`, `test_distance_*`, `test_segment_*`, `test_tolerance`, `test_strip` |
| Names with their qualifiers (`entries`, `notes`) | `test_names`, `test_entries_notes` |
| Tuned constants equal the table above | `test_tuned_constants` |
| BM25F score, default and custom parameters | `test_bm25_one_word` |
| Name classes (own stem, exact name, one-word whole-word runs), their gate and their simplest-first order | `test_classes_exact_and_within`, `test_class_gate_and_order` |
| Tie order, `n`, absent rows | `test_tie_order` |
| Status and tier factors: exact ratios, missing keys, defaults, order flips | `test_priors` |
| Typos, prefixes (`PREFIX_MIN`), the larger of a typo and a prefix factor, joins, segmentation, phrase bonus | `test_typo`, `test_prefix`, `test_typo_and_prefix`, `test_joins`, `test_segmentation`, `test_phrase` |
| Modules outside the name classes | `test_python_modules` |
| Name shown (qualifier match, header for an alias-only match, own-stem exception), output line, `Python` and `in template` marks | `test_display_name`, `test_show` |
| CLI `--find` (several words), `no match`, `-n` (default 10), `-n` errors, no index built, `in template` | `test_cli_find` |
| Unknown-name rows: libclang path, fallback, no inventories | `test_cli_unknown_name` |
| Every implemented 01–07 row found by its own stem in the top 3, first unless another C++ row shares the stem (`dsu`, `trie`) | `test_repo_stems` |
| The user's examples on the repository (`bipartite`, `binary search`, `hash`, `debug`) | `test_repo_examples` |
| Corpus recall@5 and MRR@10 against the floors, generated typos | `test_repo_corpus` |

## Commands and results (2026-11-07)

```bash
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode quick              # PASS, 35 tests, 11 s, peak 46 MB
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode full --seed 1      # PASS, 13 s plus the pre-flight, peak 45 MB
CXX=g++-14 python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode full --seed 2   # PASS (no compiler used), 24 s, peak 44 MB
python3 '96-Local Testing/00-Tools/14-search_tester.py' --mode stress --seed 7    # PASS, 3 rounds, 34 s, peak 45 MB
python3 '96-Local Testing/00-Tools/13-grab_tester.py' --mode quick                # PASS, 68 tests, 121 s, peak 149 MB
python3 '96-Local Testing/00-Tools/13-grab_tester.py' --mode full --seed 1        # PASS, 71 tests, 294 s with the pre-flight, peak 231 MB
CXX=g++-14 python3 '96-Local Testing/00-Tools/13-grab_tester.py' --mode full --seed 3   # PASS, 71 tests, 243 s, peak 225 MB
python3 '96-Local Testing/00-Tools/13-grab_tester.py' --mode stress --seed 5 --rounds 8   # PASS, 71 tests, 284 s, peak 235 MB
python3 '96-Local Testing/05-extraction.py' ops --mode quick                      # PASS: 8 rows, 159 operations on g++ and g++-14, 0 failures (the moved parser)
python3 '96-Local Testing/03-consistency.py'                                       # no errors
```

Mutation testing covers headers (`05-testing.md`, Mutation testing); this package changes Python tools and documents only, so it has no campaign. Hand mutants of `_11_search.py` and `10-grab.py`, from the two reviews and the `/finish` fixes, each fail quick mode: default `-n`, the distance-2 factor, prefix joins, status order, the fallback's concept text, the phrase idf, alias labels, module classes, the larger of typo and prefix factor, the joined stem in the name tiebreak, digit function heads, query stripping, the `-ing` vowel rule, the `yes …` Basic cell, the header for an alias-only match, the own-stem exception, `PREFIX_MIN`, `PART`, and every tuned constant (`test_tuned_constants`). Two are equivalent: dropping the `t not in known` guard before `segment` (a known word's fewest-part split is itself, so `segment` returns nothing) and the `len(t) >= PART` filter of the known set (`segment` never uses a shorter part); both stay only to skip work.

## Sources

Retrieved 2026-11-07 by the P263 `@researcher` sweep: the cp-algorithms navigation page (https://cp-algorithms.com/navigation.html), the OI Wiki nav (https://raw.githubusercontent.com/OI-wiki/OI-wiki/master/mkdocs.yml), the Library Checker categories (https://raw.githubusercontent.com/yosupo06/library-checker-problems/master/categories.toml), the KACTL content tree and the ACL document index; abbreviations with a source each (cp-algorithms, OI Wiki, KACTL, ACL, Library Checker, USACO Guide, Nyaan). Fetched through a summarizing reader, so titles were not compared byte by byte with the pages.

## Limits and handoffs

- Rows without an evidence document have no aliases; their packages add them (`/package` step 3 links the form above).
- English only: Chinese titles match only where an alias line spells them.
- Stemming is deliberately light (`buses` → `buse`, `quizzes` → `quizz`); a word the rules mangle still matches itself.
- With `PART` 2, a stem also splits into 2-letter known words: correctly for `li chao`, `pq tree`, `gomory hu`, `st number`, `digit dp`, `fm index`, but `07-Strings/46-rindex.hpp` (planned) also yields `ri nd ex`, so `--find nd` lists it first.
- A one-word query still lifts a name holding it literally: `--find segtree` lists `SegTreeBeats` before `SegmentTree`, whose names never spell `segtree`.
- All 61 `08-Python` modules are planned, so the status factor lowers a `python <concept>` query's module below an implemented header that scores within 10% of it (`py` MRR 0.645 against 0.784 without factors); recall@5 stays 0.967.

## History

- 2026-11-07: P263 introduced the concept search, its tester and corpus, 62 alias lines, and moved the Operations-cell parser out of `05-extraction.py`; the tester found `distance` returning the true distance above its cap (fixed); quick, full on g++ and g++-14 and stress passed; the `@reviewer` findings (`--find` taking several words, alias labels indexed, modules lifted by their names, the overfitting claim, the tuning record, eight test gaps, stale extraction lines) were fixed and every suite above rerun.
- 2026-11-07: P263 user review: template and debug rows indexed (`in template`), status and tier factors, name classes gated to grabbable rows and ordered simplest first with the own stem first, the word-run class for one-word queries only, `01-Core` rows outside Basic shown as Advanced, the module factor removed; `simple` and `py` corpus sources added; search tester (31 tests) quick, full on g++ and g++-14 and stress, and `13-grab_tester.py` quick and full on g++ and g++-14, passed.
- 2026-11-07: P263 `/finish`: review fixes (shown name from qualifiers or the header, `yes …` Core cells, the `-ing` vowel rule, the `aliases:` label head, cell-qualified and `std::` entries, the module evidence-document path, tested clauses and pinned constants) and the user's answers (status, tier factors and cutoffs retuned on the tester's own query list, alias marks moved to History); search tester (35 tests) quick, full on g++ and g++-14 and stress, and `13-grab_tester.py` quick, full on g++ and g++-14 and stress, passed.
