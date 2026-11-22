# Contest Testing

Toolkit rules: [08-contest-tools.md](../00-Guidelines/08-contest-tools.md). Run the examples below from this folder, or pass the script paths from another directory. The tools need POSIX (Linux or macOS), Python 3.10 or newer and, for C++ sources, a GNU++20 compiler. The numbered templates are editable examples, not required parts of the protocol.

## Batch Testing

`01-stress.py` runs one seed at a time. A generator takes the decimal seed as its first argument and writes the **entire stdin input** to stdout. Programs may be `.py`, `.cpp` or executable files; C++ is compiled once, before the case loop, in a temporary directory.

```bash
python3 01-stress.py 02-gen.py optim.cpp brute.py --count 1000 --seed 42
python3 01-stress.py 02-gen.py optim.cpp brute.py --checker 04-checker.py
python3 01-stress.py 02-gen.py optim.cpp brute.py --replay /tmp/contest-stress-artifacts/CASE/input.txt
```

`09-quick.py` takes the same three positional arguments with only the seed, count, timeout and artifact options and token comparison. It delegates to `01-stress.py`, keeping its detailed statuses, failure bundles and process cleanup, so keep both files together. Use the full runner for replay, custom judging, compiler settings and the other modes.

- Comparison: whitespace-separated byte tokens by default; `--exact` compares every output byte. A checker receives three paths, `input candidate_output reference_output`: exit 0 accepts, 1 reports a wrong answer, and any other exit or a timeout is a checker error. `--exact` and `--checker` are mutually exclusive.
- Reference: with `--checker` the reference program is optional, and its output file is empty when it is omitted. A supplied reference must finish successfully before the candidate is judged. With `--replay saved.in --expected saved.ans` and no reference program, the saved answer file is the reference output, so sample tests need no brute; this works with token, `--exact` or checker comparison.
- `--input-validator` (every mode, replay included) receives the input path before any program runs. Exit 0 means valid, 1 reports `invalid_input`, and anything else or a timeout reports `input_validator_error`. Both are inconclusive generator or infrastructure failures, never contestant verdicts.

## Interactive and Scored Modes

```bash
python3 01-stress.py unused solution.cpp --mode interactive --interactor 05-interactor.py --replay hidden.in
python3 01-stress.py 02-gen.py heuristic.cpp baseline.cpp --mode scored --validator 06-validator.py --scorer 07-scorer.py --objective maximize --count 100
```

Interactive mode gives the interactor the hidden input's **file path** as its first argument. The contestant sees only the bytes the interactor writes to its stdout, and the contestant's stdout goes to the interactor's stdin. Exit 0 from both means accepted; interactor exit 1 means rejected, and any other exit is an interactor error. The runner records both raw directions and a timestamped base64 transcript. `--whole-timeout` and `--idle-timeout` bound a stalled exchange, including inherited pipes held open after their parent exits. Replay uses the same saved hidden input; the exchanged bytes depend on the interactor and contestant behaving deterministically, and timestamps may differ.

In the example, `hidden.in` holds one integer in `[-100,100]`. `05-interactor.py` prints `guess`, accepts up to eight `? y` queries and answers `<`, `=` or `>` comparing the hidden value with `y`; the contestant sends the final integer on its own line. The interactor rejects malformed messages, early EOF and a ninth query. Both sides must flush each message. This hidden format differs from `02-gen.py`'s, so randomized interactive testing needs a matching generator.

Scored mode's validator receives `input output` paths: exit 0 means valid, 1 invalid, and any other status a tool error. The scorer receives the same paths and must print one finite decimal number. `--objective maximize|minimize` sets the sign of `gap`, which is positive when the candidate trails the optional reference; without a reference the candidate's score is printed alone. There is no stop-on-regression policy. Decimal parsing preserves the score digits; widely separated exponents may round the displayed gap but keep its sign. Scores or gaps outside the Decimal implementation's range report `scorer_error`.

## Runs, Failures and Cleanup

- `--count 0` (the default) runs until a failure or Ctrl-C. Timeouts must be finite and positive. `--timeout`, `--python`, `--cxx` and `--cxxflags` set process limits and compilation; each compiler call has a 60-second limit. Paths with spaces work when quoted in the shell.
- The first failing seed gets a unique folder under `/tmp/contest-stress-artifacts` (`--artifacts` changes it) holding the input, outputs, stderr, source paths, effective command arguments, comparison and configuration, and statuses. `--replay` reruns that input and ignores the generator. Builds stay in temporary storage, never beside the source.
- Exit codes: 0 when every requested case passed, 1 for a saved case failure (infrastructure failures included), 2 for a setup or artifact error, 130 when interrupted. The printed kind and `case.json` tell a contestant failure from a generator, reference or tool failure.
- The common engine kills child process groups and closes pipes on completion, timeout and interruption; a program that deliberately starts a new session can escape that group. Output is buffered in memory in batch mode and saved to disk in interactive mode, with no output or memory quota. These are trusted local testing tools, not an isolation sandbox. Ctrl-C cleans up but saves no partially judged case; replay applies to completed failure bundles.

## Shrinking

`08-shrink.py` is an optional, separate driver for **problem-specific** shrinking. Keep it beside `01-stress.py`, whose process helper it shares.

```bash
python3 08-shrink.py failing.in --propose my_proposer.py --valid my_input_validator.py --probe my_failure_probe.py --max-steps 100
```

- Hooks are Python files or executables. `--propose` receives the current input path and the zero-based step, and prints proposed bytes or exits 3 to skip. `--valid` receives the input path and exits 0 for valid, 1 for invalid. `--probe` receives the input path, exits 0 and prints a nonblank failure class or signature. Any other exit or a timeout is a hook error.
- Signatures compare **exact bytes, whitespace included**, so a probe must encode success as a signature different from the original failure.
- The helper validates the original before probing it and accepts only strictly shorter valid proposals with the same signature. It stops after `--max-steps` attempts; every hook call has `--timeout`, so there are at most `2 + 3 * max_steps` calls.
- The original stays unchanged, and the reduced file is created exclusively (`--output` chooses its path). Once the baseline is established, a hook error or Ctrl-C still saves the best accepted case, exiting 2 or 130. The helper does not delete text generically and does not guarantee a globally minimal result.

## Extraction

`10-grab.py` pastes only the library code a solution needs, without the template and debug headers that `99-Workspace/template.cpp` already holds:

```bash
python3 10-grab.py bellTable DSU          # print and copy the declarations these names reach
python3 10-grab.py ModInt::sqrt           # one member, its struct pruned to what it needs
python3 10-grab.py --into a.cpp           # one block between "// grab begin:" and "// grab end" above the first use
python3 10-grab.py --into a.cpp --min     # the same, each struct cut to the members a.cpp calls
python3 10-grab.py --find shortest path   # the 10 closest inventory rows: name to grab, header, tier, status (-n K)
```

- `--into` reads the identifiers of `a.cpp` outside the block, `#define` bodies included, and takes the library names among them. It skips the names `a.cpp` declares or `#define`s itself and those it uses only as locals, parameters or members, so a local `rng` stays local. It then rewrites the block with what `grab` prints for those names, so rerun it after using another library name. `--min` keeps only the members `a.cpp` calls; rerun it after calling another member too.
- A function such as `dijkstra(g, ...)` brings the type it takes (`Graph`) whole.
- An unknown name lists the closest names grab accepts (`DUS`: did you mean `DSU`?) and the three closest inventory rows.
- `--find` takes a name, a concept, an abbreviation (`lca`, `scc`, `crt`), a typo (`dijkstar`) or Chinese, Simplified or Traditional, in Taiwan or Hong Kong usage too (`最短路`, `線段樹`, `樹狀陣列`). It reads only the folders' inventories and the evidence documents' aliases, so it needs neither libclang nor the index, and it also lists planned rows and `08-Python` modules, marked by their status and `Python`.
- Extraction needs libclang (`/setup-check` prints the install command). The first run indexes the library in a few minutes into the git-ignored `.grab-cache`, and later runs take under a second. Without libclang grab prints SKIP and inlines whole headers.
- Copying uses `wl-copy`, `xclip`, `xsel` or `pbcopy`; `--no-copy` only prints.

Evidence: [00-extraction.md](<../96-Local Testing/Docs/00-extraction.md>) and [00-search.md](<../96-Local Testing/Docs/00-search.md>).

## Coverage and Extension Inventory

P001 evidence (contracts, feature-to-test map, latest commands and results, limits): [00-verification.md](Docs/00-verification.md). The examples are small demonstrations, not a generator and checker library for every algorithm family.

| Files | Owned features | Status and evidence |
|---|---|---|
| `01-stress.py`, `09-quick.py` | Sequential seeds, Python/C++/executables, token/byte/custom comparison, input validator, expected-answer replay, error classes, unique bundles, replay, child cleanup, reduced quick CLI | **Verified**; [P001 evidence](Docs/00-verification.md) |
| `01-stress.py`, `05-interactor.py` | Hidden input, bidirectional relay/transcript, EOF/backpressure, total/idle limits, bounded query example | **Verified**; same evidence |
| `01-stress.py`, `06-validator.py`, `07-scorer.py` | Validity versus tool errors, finite Decimal scores, both objectives, optional reference, permutation example | **Verified**; same evidence |
| `08-shrink.py` | Valid baseline, shorter valid proposals, exact signatures, skip/errors/timeouts, bounded attempts, preserved progress | **Verified**; same evidence |
| `02-gen.py`, `03-gen.cpp`, `04-checker.py` | Reproducible complete integer-array inputs; exact token checker without float conversion | **Verified**; same evidence |
| `10-grab.py` | Index cached per closure hash in both ISA configurations, pruned extraction, `--into`, `#line` output, whole-header fallback | See [P253 evidence](<../96-Local Testing/Docs/00-extraction.md>) |
| `_11_search.py`, `_11_search_t2s.tsv`, `10-grab.py --find` | Concept search over inventory rows and aliases: BM25F, typos, prefixes, joins, phrases, name classes, Chinese, Simplified and Traditional, through OpenCC's conversion; unknown-name rows | See [P263 evidence](<../96-Local Testing/Docs/00-search.md>) |

Both generators emit `1 <= n <= 20` and `n` integers in `[-100,100]`. The Python generator accepts signed integer seeds and the C++ one decimal seeds in `[0,2^64-1]`. Repeating a seed in the same generator reproduces its case; the two generators need not agree. The checker compares tokens exactly. The scored example reads the first input token as a nonnegative `n`, validates a permutation of `1..n` (empty is valid when `n=0`), then counts increasing adjacent pairs.

Planned problem-specific additions, made only when a task needs them:

- Seeded generators for valid graph, tree and multigraph instances, integer, field and geometry boundary cases, strings and stateful operation histories, combining exhaustive tiny cases, degenerate shapes and randomized distributions. A generator enforces its domain; `--input-validator` checks it independently.
- Checkers for nonunique paths, cuts, matchings, decompositions and constructive outputs, and numerical checkers with an explicit absolute, relative or error-bound policy. Exact integer tokens stay exact.
- Further problem-specific interactor protocols and scoring policies; the bounded guessing and permutation examples cover the common lifecycle.
- Shrink hooks that preserve graph and tree structure, algebraic domains and legal rollback or persistence histories. The current helper's progress metric is strictly decreasing **input byte length**, with bounded attempts and per-hook timeouts; arbitrary semantic metrics, a separate total-time limit and generic interactive or scored reducers are not implemented.

[The completeness audit](../00-Guidelines/History/2026-09-27-inventory-audit.md) holds the expanded workload inventory.
