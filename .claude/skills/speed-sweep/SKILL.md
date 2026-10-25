---
name: speed-sweep
description: Optimality research for Core full types against the fastest judge submissions of their problems (Library Checker first). `scout` (default) ranks the leaders, reproduces our solutions with the judge's flags, has @speedscout explain the gaps and writes an "Optimality gap" section; `verify` refreshes the numbers after a package stage.
disable-model-invocation: true
argument-hint: "Pxxx | <header path> [scout | verify]"
model: claude-fable-5-1
effort: xhigh
---

`$ARGUMENTS` is a package id or a header path, optionally followed by `scout` (the default) or `verify`. `J` stands for `python3 '96-Local Testing/00-Tools/07-judge.py'` (usage: `J --help`; endpoints and judge recipe: `00-Guidelines/09-sources.md`, Fastest submissions). The sweep writes only the evidence and sources documents named below. Findings become `/package` work, never code edits here; the tool never submits.

## Scout

1. **Headers.** For a package, `python3 '00-Guidelines/13-Plan/plan.py' show <id>` gives the targets. Keep the `01-Core` non-mini headers. If none is left, say that the skill covers Core full types only and stop.
2. **Problems.** For each header, collect:
   - the `// PROBLEM:` judge URLs of the `97-Online Testing/src` sources that include it (`rg -l '<header file name>' '97-Online Testing/src'`);
   - the problems named in its evidence document's Library Checker section;
   - the problems from `J problems <word>` that the header's family solves but no source covers. List these as candidates in the report; adding a judge source is `/package` work.
3. **Leaders.** Per problem:
   - `J fastest <problem>` gives the distinct users, with similar sources merged.
   - `J submission <id>` for the top five gives each cached source path and per-case table.
   - `J mine Ausp3x <problem>` gives the user's standing and latest per-case table (`Ausp3x` is the user's account).
4. **Archives.** Search GitHub and the web for speed archives, write-ups and library verify files of the same problems (github.com/AZZRCN/precious-speed held all six `big_integer` #1s with write-ups on 2026-10-25). Check these secondary sources before listing them: AtCoder Library Practice Contest standings, Luogu/LibreOJ/UOJ template-problem speed rankings, SPOJ MUL/VFMUL, Kattis fastest lists, and GMP as the ruler. Keep only pages you fetched that bear on a problem in scope.
5. **Reproduce.** Per problem:
   - `J reproduce <problem> <our source> --label ours` builds with the judge's flags, takes the minimum of three wall times per regenerated test and checks each output with the problem's checker.
   - `J compare <problem> --user Ausp3x` gives the per-case table.
   - Leaders' local times need the user's yes. Ask once with AskUserQuestion, naming the submission ids, then run `J reproduce <problem> <cached source> --untrusted` for each approved id. It builds and runs the source under bwrap, with a read-only filesystem and no network. Rerun `compare`. A leader whose source does not build is noted, not fixed.
6. **Analyse.** Start one `@speedscout` agent per header, at most four at a time. Give each:
   - the header and our solution paths, the problems, the compare tables;
   - the leaders' ids with their cached source paths;
   - the archive URLs from step 4.
7. **Write.** In the header's evidence document (`<folder>/Docs/<NN-name>.md`), replace or create `### Optimality gap` at the end of `## Benchmarks` with:
   - the conditions (date, CPU, compiler, flags, runs, tests commit);
   - per problem: the judge standing, the compare table's max and sum rows, and the cases with a gap above 1.5;
   - each leader's kernel and exactness status;
   - the ranked causes;
   - the targets: per problem, the per-case times that would reach the best exact leader;
   - an empty `Verify runs` table: `| Date | Commit | Problem | Local max ms | Judge max ms | Rank | Best leader ms |`.

   Add `- YYYY-MM-DD: speed sweep, <one-line standing>` to `## History`. List every fetched URL under `## Fetched YYYY-MM-DD (speed sweep)` in the folder's `Docs/00-sources.md`.
8. **Report** to the user:
   - the standing per problem and the top causes;
   - the targets the next `/package` session should take, and any candidate problems;
   - fetches that failed;
   - `git add <evidence document> <sources document> && git commit -m "Speed sweep <scope>"`.

## Verify

1. **Scope.** Steps 1–2 of Scout. If a header's evidence document has no `### Optimality gap`, stop and suggest `scout`.
2. **Refresh.** Per problem:
   - `J fastest <problem>`, then `J mine Ausp3x <problem>` for the user's latest per-case table;
   - `J reproduce <problem> <our source> --label ours`, then `J compare <problem> --user Ausp3x`.

   Rerun `--untrusted` only for the leader ids the section already records with the user's yes.
3. **Append.** Add one row per problem to the `Verify runs` table, with the date and the header's commit (`git log -1 --format=%h -- <header>`). Note any leader newer than the section's table, and any case whose gap grew by more than 10%. Add a `## History` line.
4. **Report** the changes in standing and gaps, and give the commit command of Scout step 8.
