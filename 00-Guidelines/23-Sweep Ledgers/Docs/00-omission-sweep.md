# Omission sweep 2026-11-08 (P254)

P254 (SUP23) re-ran the completeness sweep of `02`–`07` under `01-principles.md` (Priority, 2026-11-05) and applied it. The same session carried out the user's follow-up rule changes. The binding decisions are in [omissions.md](../omissions.md) and [duplicates.md](../duplicates.md) (Recorded decisions) and in [10-decisions.md](../../10-decisions.md) (2026-11-08 One API by context, operation names, source keys).

## Contracts

- **Sweep.** `/omission-sweep 02 03 04 05 06 07` ran the `scan.js` workflow per folder. `@researcher` swept the rows in chunks and ran a family sweep per folder; a library lens and a source lens then judged each candidate.
  - Candidates: 686 (02: 105, 03: 135, 04: 141, 05: 171, 06: 72, 07: 62).
  - The user decided them on 2026-11-08 (`omissions.md`, Recorded decisions): adopt everything except exact duplicates, second implementations and items another row owns.
- **Apply.** `/omission-sweep apply` made:
  - 243 row edits and 21 new rows.
  - 48 verified rows reopened as `partial`, each with a `missing:` list.
  - 29 packages reopened, each with an "Omission sweep 2026-11-08: implement ..." note.
  - Batches DS54–56, GE52–58, GR77–79, MA55–58, MI36 and ST42–44, with packages P264–P273, for the new rows.
  - 19 omissions and handoff notes under `## Omission sweep 2026-11-08` in each folder's `Docs/00-notes.md`.
  - The fetched URLs under `## Fetched 2026-11-08 (omission sweep)` in each `Docs/00-sources.md`.
- **Rule changes** (user, 2026-11-08):
  - One API per operation now applies per context (`03-cpp.md`).
  - One-problem algorithms that a catalog names are operations (`01-principles.md`, Completeness), placed in Esoteric (`02-structure.md`, Tiers).
  - Operation names (`02-structure.md`).
  - One library source key list (`09-sources.md`, Source keys), with 216 status cells rewritten to it.
  - P275 re-reviews older duplicates, prunes and omissions; P276 does the renames.
- **Clash fixes:**
  - Mathematics 62 `Interval` became `RealInterval`.
  - Mathematics 35 `lll` became `lllReduce`.
  - Graphs 84 dropped its copy of Data Structures 39 `StaticTopTree`, and Mathematics 25 dropped its copy of Miscellaneous 08 `mex`.
  - Mathematics 23 owns the matrix-tree counts.
- **`/finish P254` decisions** (user, 2026-11-08):
  - `SortableArray` stays.
  - The Beats preset reverted to a plain BeatsMonoid preset.
  - `LinearRMQ` uses bitmask microblocks.
  - Miscellaneous row 10 adopts `offlineRangeSum`.
  - Labelled and unlabelled constructions are separate contexts.
  - `duplicates.md` items from before 2026-11-08 wait for P275.
  - The brace and type-header checks stay keyed on row and package.
  - A row without an evidence link carries source keys; rows with one, the 49 reopened rows included, need none.
  - A struct member may reuse a name from `01-template.hpp`.
  - Data Structures rows 05 and 06 list their braced-list builds as `ctor (initializer_list)`.

### Validator checks (`96-Local Testing/03-consistency.py`)

- **Status cells** start with a status word. Every other entry must be one of:
  - a link;
  - a `missing:` operation;
  - a `legacy:` path (one with a `/` or a file extension);
  - a key from `09-sources.md` (Source keys), optionally followed by a parenthesized detail with no nested parentheses.
  Anything else is an error. A row with neither a link nor a key or path goes to `unsourced_rows`.
- **Names** are checked over the `01`–`07` header rows, parsed by `09-Contest Testing/_11_search.py` (`entries`):
  - A type name (capitalized, or a Core alias such as `mint`) in two rows is an error, and so is a name from `01-template.hpp` reused in another row.
  - A lower-case name in two contest rows goes to `name_collisions`.
  - Core rows contribute type names only: their functions are members or hidden friends, and mini rows repeat their full rows' names.

## Feature-to-test map

| Feature | Test | Oracle |
|---|---|---|
| Source keys and status grammar | Probe faults: an unknown key; an old lower-case key; a `sources:` label; `planned, YC` (no `;`); a key after a link; a key inside a `legacy:` list; a detail followed by more text | Each fault reported; the clean tree gives 0 errors |
| Type and template names | Probe faults: `Interval` in Mathematics 62; `lll` in Mathematics 35; `chmin` and `InfIntMini` in Data Structures 05; `mint`, `u256`, `iint`, `lng` and `Fenwick` in Mathematics 45 | Each reported as an error |
| `name_collisions` | Probe fault: `mex` in Mathematics 25 and 45 | Listed. The clean tree lists 4: `canonicalForm`, `distanceApprox` and `minimalPolynomial` on different argument types, and `primitiveRoot`, whose two meanings P276 settles |
| `unsourced_rows` | `03-consistency.py` on the tree | Mathematics 43 and 56, keyless since before P254 |
| Plan and inventories | `plan.py check`, `03-consistency.py` | No errors |

The probes run the two blocks exactly as they appear in `03-consistency.py`, on the real inventories with the faults injected in memory, so no file changes; they were one-off scratch scripts (Limits). A hard-linked copy of the tree was not possible because `/tmp` is on another filesystem.

## Commands and results (2026-11-08)

- `python3 '96-Local Testing/03-consistency.py'`: errors [], `name_collisions` 4, `unsourced_rows` 2.
- `python3 '00-Guidelines/13-Plan/plan.py' check`: ok.
- `plan.py doctor`: 32 row findings in 19 packages, each a reopened package that keeps other verified rows. The user's 2026-11-08 answer keeps the brace and type-header checks keyed on row and package, so the findings stand until each package is verified again.
- `96-Local Testing/00-Tools/` testers, all PASS:
  - `14-search_tester.py` (50 tests)
  - `08-plan_tester.py` (40 tests)
  - `12-hooks_tester.py` (15 tests)
  - `06-site_tester.py` (63 tests, 4 skipped)
  - `03-notebook_tester.py` (12 tests)
- Injected-fault probes: status grammar and names 12 of 12 reported; Core aliases, template names, a type and a collision 7 of 7.

## Sources

Each folder's `Docs/00-sources.md`, section Fetched 2026-11-08 (omission sweep).

## Limits

- The per-candidate trail stayed in the session's scratch directory and was not kept. It held 686 decisions with their purposes and reasons, and 108 renames. The repository keeps the outcome: rows, package notes, omission bullets and fetched URLs.
- Failed fetches were logged per folder (03: 39, 04: 67, 05: 80, 06: 24, 07: 25; 02 not logged), but the logs were not kept. Some catalog index pages (maspypy, Nyaan, ei1333, hitonanode) were read only in part, and some Fetched rows cite an index URL rather than the operation's own page.
- Gaps in the name check:
  - It skips Core lower-case names.
  - It skips the variants listed under a camelCase function head.
  - It skips members, which may reuse template names (`AddChminChmax::chmin`, the chmin presets of Data Structures 14).
  - It collects a row's names as a set, so two same-signature forms in one row pass (Graphs 41 had `isCactus` twice; it is now `isVertexCactus`).
  - `name_collisions` and `unsourced_rows` are only reports; nothing fails on them.
- The `legacy:` check reads shape only: a `/` or a file extension passes without the file existing, as the bare sibling names (`algsbetter.cpp`, `test.cpp`) in Graphs rows 12–15 do.
- The probes were scratch scripts, not kept; the feature map lists their faults so they can be rebuilt.
- The sweep added names that break the vocabulary, listed in P276's note for its renames: `RangeParallelDSU::getSize`, `reportIntersectionsBrute`, `SparseTableLCA::getLCA`, names with the variant word first (`offlineRangeSum` and others), and `phiTrial`.
- Reopened packages keep other verified rows. Doctor reports them, and the brace and type-header checks skip their headers until each package is verified again.

## History

- 2026-11-08: sweep 02–07 applied, rule follow-ups made, /finish P254 (review: 12 defects fixed, 8 decisions asked; diff review: 8 fixed, 3 decisions asked).
