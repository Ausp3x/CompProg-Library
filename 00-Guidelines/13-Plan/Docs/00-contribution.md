# Contribution workflow — evidence (P250)

SUP17 covers roles and claims; SUP18 covers setup, portability and licensing ([below](#setup-portability-and-licensing-sup18)).

## Roles and claims (SUP17)

Scope: [plan.py](../plan.py) reads the clone's role and the open pull requests that claim packages, [gaps.py](../../../.claude/skills/finish/gaps.py) tells `/finish` where step 7 commits, and the rules live in [CLAUDE.md](../../../CLAUDE.md) (Git), [finish](../../../.claude/skills/finish/SKILL.md) step 7, [package](../../../.claude/skills/package/SKILL.md) step 1, [01-prompts.md](../../../01-prompts.md) (Workflow) and the [plan index](../00-index.md). Decisions: [10-decisions.md](../../10-decisions.md), 2026-10-29 Contribution.

### Contracts

- **Role.** `role(root)` returns `git config compprog.role` read in `root`: `contributor` when it is unset, otherwise `contributor` or `maintainer`. Any other value stops with a message naming it. This clone is set to `maintainer` (user decision 2026-10-29).
- **gh.** `gh(root, *args)` returns `(stdout, '')`, or `(None, reason)` when the reason is "gh not installed", the first stderr line of a non-zero exit, or "gh <cmd> timed out" after 10 s, or "gh <cmd> failed: <error>" when it cannot be executed. The claim lookup never fails a command: it reports `SKIP (<reason>)`, including `gh pr list printed no JSON`. After a SKIP the login does not call gh. A brief therefore waits at most two gh timeouts (20 s), and `gaps.py` with its brief at most four (40 s), both within the 60 s SessionStart hook, which runs only the brief.
- **Login.** `login(root, ask)` is the GitHub login from `gh api user` (only when `ask`), falling back to `git config user.name` when that output is empty. It is slugged to `[A-Za-z0-9-]` with runs joined by `-`, and becomes `user` if nothing is left. It provides the `<user>` part of a claim branch.
- **Claims.** `claims(root, packages)` runs `gh pr list --repo <repo> --state open --limit 1000 --json number,headRefName,isDraft,author`, where `<repo>` is `git config compprog.repo` or `Ausp3x/CompProg-Library` (`REPO`; user decision 2026-10-29), so a fork's clone reads the canonical pull requests. An open pull request, draft or ready, claims Pxxx when its head branch fullmatches `Pxxx-<batch>-<user>` and the batch (in any case) belongs to Pxxx. The lowest number wins per package. A ready pull request stays a claim until it is merged or closed (user decision 2026-10-29).
- **Gate.** `gate(root, packages)` blocks a package claimed by someone else (the author differs from the login, case-insensitively). For a contributor it also blocks every Core full package (model `fable`) as maintainer-only, including one the contributor claimed. It returns the claims line `Role: <role>; claims: none open | <k> open | SKIP (<reason>)`. The login is looked up only when there are claims or the role is contributor.
- **Commands.** `next`, `status` and `show next` take the first ready package the gate does not block. `show` prints the claims line second, then one of four lines. `STOP: <id> is <why>` when blocked. `Claimed by you: pull request #n on <branch>` when the user holds the claim, with `git fetch origin <branch> && git switch <branch>` when the current branch differs. For a contributor and any other package (verified ones included, for `/prune` and `/restyle`; user decision 2026-10-29), `Unclaimed:`, or `Claims unknown:` after a SKIP, with the claim commands to run in the checkout the session will work in: `git switch -c <branch>`, an empty claim commit, `git push -u origin <branch>`, then `gh pr create --draft --fill --repo <repo>`. The branch is `claim_branch` of the brief's batch. `ready` starts with the claims line and marks held packages, which never enter the disjoint set. Live and claimed packages, whoever claimed them, reserve their folders; a package that is only maintainer-only does not.
- **`/finish`.** `gaps.py` prints `role maintainer: step 7 lands on master`, or `role contributor: step 7 commits on branch <b> and stops before landing; repository <repo>; ` followed by `pull request #n; ` when the user holds a claim on the package, then `last open batch: mark the pull request ready if verified` (also without a package) or `more batches follow: the pull request stays a draft` (user decision 2026-10-29: the pull request stays a draft until the package is done, so a merge cannot release the claim early). `<b>` comes from `claim_branch`, which `show` uses too: the user's own open claim's branch; else the current branch when it is a claim branch of a batch of the package; else `<package>-<batch>-<login>`. Here the batch is the batch key, or else the package's first batch without a `verified` finish-log line. Logged lines decide, not inventory rows, since `/package` marks rows verified before `/finish` runs; for the same reason, on a claim branch with no argument the key is that batch, not the batch named in the branch. What step 7 then does is defined in [finish](../../../.claude/skills/finish/SKILL.md) step 7, and which finish-log lines count in the [plan index](../00-index.md).

### Feature-to-test map

Tester: [08-plan_tester.py](<../../../96-Local Testing/00-Tools/08-plan_tester.py>). A fake `gh` alone on `PATH` answers from fixtures, can fail, sleep or print non-JSON, and logs its arguments. Roles come from `GIT_CONFIG_COUNT` overrides with the global and system configs at `/dev/null`. Synthetic repositories in `/tmp` carry their own plan, finish log and live files, and the real repository is only read. The oracles are re-derived in the tester.

| Contract | Tests |
|---|---|
| role | `test_role_unset_and_values`, `test_role_reads_root`, `test_role_invalid` |
| gh (10 s timeout, exec errors) | `test_gh_success_and_failure`, `test_gh_missing`, `test_gh_timeout`, `test_gh_not_executable`, `test_syn_gh_not_executable` |
| landed (only `verified` lines) and support `batch_done` | `test_syn_show_unclaimed`, `test_gaps_role_line`, `test_real_show_unclaimed`, `test_real_gaps_role_line` |
| live (worktree directory name and branch, whole tokens only; a claim reduced to `Pxxx-<batch>`) | `test_syn_live_worktrees`, `test_syn_ready_text_reservations` |
| config, claims repository (default and override) | `test_config_and_claims_repo` |
| login (gh only when asked; no gh after a SKIP) | `test_login`, `test_login_ask_false`, `test_login_fallbacks`, `test_login_random` |
| claims | `test_claims_fixture` (exact arguments, wrong-owner and lowercase batches, malformed branches, lowest number, SKIP, non-JSON), `test_claims_random` |
| gate | `test_gate_maintainer`, `test_gate_contributor`, `test_gate_random` |
| claim_branch | `test_claim_branch`, `test_claim_branch_random` |
| ready skip; next, status, show next | `test_ready_skip`, `test_ready_random`, `test_syn_show_next_and_status_text`, `test_syn_next_cli`, `test_real_next_status_show_next`, `test_real_all_claimed`, `test_real_next_random` |
| show (STOP, Claimed by you, Unclaimed, Claims unknown, `--repo`, batch hint) | `test_syn_show_gate_line_and_stop`, `test_syn_show_unclaimed`, `test_syn_no_json_skip`, `test_real_show_stop_and_claims`, `test_real_show_unclaimed` |
| ready_text | `test_syn_ready_text_reservations`, `test_real_ready_text`, `test_real_ready_text_random` |
| gaps.py role line (claim branch, repository, own pull request number, last batch or draft) and its no-argument key on a claim branch | `test_gaps_role_line`, `test_real_gaps_role_line` |

### Commands and results (2026-10-29)

Run from `/tmp`; the tester caps itself (`_00_memory_cap.ensure()`), and the peak was 49 MB.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/08-plan_tester.py' --mode quick --seed 1` | PASS, 31 tests, 19.1 s |
| `... --mode full --seed 1` | PASS, 38 tests, 24.3 s after the quick pre-flight |
| `... --mode stress --seed 42 --rounds 100` | PASS, 38 tests, 30.8 s |
| `python3 '00-Guidelines/13-Plan/plan.py' check`; `doctor` | ok; no inconsistencies |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

### Limits

- Claims are as fresh as `gh pr list`: two contributors who both run `show` before either opens a pull request can both claim, and the lower pull request number wins. Without `gh` the claims line is a SKIP and `show` prints Claims unknown, never Unclaimed. The fork case (`--repo` pinned) is covered only by the fake `gh` asserting the arguments; it has no run against GitHub.
- `claims` trusts `gh --json` to print a list of objects: valid JSON of another shape from a `gh` wrapper raises an error or reads as no claims.
- `ready` reserves folders only for packages it lists, so a live or claimed package whose prerequisites are not yet verified does not reserve its folders.
- `/finish`'s `allowed-tools` pre-approves `Bash(git switch *) Bash(git fetch *)` for the contributor step (user decision 2026-10-29; added by the user).
- A malformed claim-shaped branch whose batch part is not a batch (`P250-ZZ99-c07`) lets a token of its user part name the target of a no-argument `/finish`; real claim branches are unaffected.
- `--limit 1000` reads at most 1000 open pull requests. If `gh api user` fails after `gh pr list` succeeded, the login falls back to `git config user.name`, and the user's own claim can then read as STOP.
- The contributor flow has no end-to-end run: no recorded run of the claim commands, the step 7 switch, commit and push, even against a local bare remote. A contributor's worktree is not removed by `worktree.py --remove`, since the maintainer's rebase leaves its tip outside `master`; delete it by hand after the merge.
- Agent choices kept as contracts: the lowest pull request number wins, authors match case-insensitively, the login slug defaults to `user`, the gh timeout is 10 s, and maintainer-only packages do not reserve folders.
- The merge side (`/merge-pr`) is SUP19, and the contributor setup and docs (CONTRIBUTING) are SUP18.

## Setup, portability and licensing (SUP18)

Scope: [setup_check.py](../../../.claude/skills/setup-check/setup_check.py) and the [/setup-check](../../../.claude/skills/setup-check/SKILL.md) skill; the x86 target gate in [_00_cpp_test_runner.py](<../../../96-Local Testing/_00_cpp_test_runner.py>) and the runners that use it; two checks in [03-consistency.py](<../../../96-Local Testing/03-consistency.py>); [README](../../../README.md), [CONTRIBUTING](../../../CONTRIBUTING.md), [LICENSE](../../../LICENSE), [THIRD-PARTY](../../../THIRD-PARTY.md) and `.gitattributes`. Decisions: [10-decisions.md](../../10-decisions.md), 2026-10-29 Contribution.

### Contracts

- **TOOLS.** One `Tool(name, need, exes, floor, ubuntu, arch)` per tool: `need` is None for a required tool, else the optional feature (notebook PDF, notebook page test, site, online expansion, PyPy runs, arm64 check); `exes` are alternatives, the first on `PATH` is used. Required: `g++` major ≥ 16, `g++-14` major 14, `python3` ≥ 3.11 (the tooling floor; `tomllib` needs 3.11), git, rg, ccache, bwrap, systemd-run, lscpu, gh. Optional: lualatex, kpsewhich, fc-list, node or nodejs, zensical or mkdocs, oj-bundle, pypy3 ≥ 3.10, aarch64-linux-gnu-g++, qemu-aarch64. Packages: Ubuntu 26.04 (`resolute`) and Arch, checked against the `resolute` package indexes and `Contents-amd64` and archlinux.org on 2026-10-30; the Ubuntu arm64 cross compiler is `g++-aarch64-linux-gnu` (GCC 15), the package that installs `aarch64-linux-gnu-g++`.
- **check.** `(False, 'not found')` when no executable is on `PATH`; a floor compares `-dumpfullversion` (compilers) or `sys.version_info` (Pythons), and GCC 14 must be major 14; then `probe` adds problems: each compiler builds and runs a small program with `-fsanitize=address,undefined` and with `-static`, under a 256 MB memory cap when user scopes work; `systemd-run` passes only when a `--user --scope --expand-environment=no -p MemoryMax=64M` child (the flags `_00_memory_cap` uses) reads `67108864` from its own `memory.max` (cgroup v2 with the memory controller delegated); `gh auth status` exits 0; every `\usepackage` of `01-notebook.py` resolves through `kpsewhich`; both notebook fonts are known to fontconfig (`01-notebook.py` `font_status`); a site engine has its modules (`06-site.py` `engine`); an arm64 program built by the cross compiler runs directly with `QEMU_LD_PREFIX=/usr/aarch64-linux-gnu` (binfmt).
- **platform, role.** Not x86-64, or x86-64 without AVX2, FMA and BMI2, is a WARN (Core full packages need them). The role row is `plan.role`; an invalid value, or no git, is a FAIL.
- **Output.** One `OK`/`WARN`/`FAIL` line per row, then `Install:` lines for every failed tool on the distribution given as argument or read from `/etc/os-release` (`ID`, `ID_LIKE`, quoted or not; both when unknown): one `sudo apt install` or `sudo pacman -S` line for the plain packages in TOOLS order, then one line per compound or AUR entry. A failed tool gets an install line unless it is present and only its `gh` or `systemd-run` probe failed (login or delegation, which no package fixes). The last line is PASS or FAIL; exit 1 when a required row fails, 2 for an unknown distribution.
- **Lint.** `undeclared(root)` reads `git ls-files` (failing when it fails), skips `OLD/` and `95-Resources/`, and returns `(file, executable)` for every literal command head (first argument or `args=`/`cmd=` of `run`, `Popen`, `call`, `check_call`, `check_output`; an `os.environ.get` or `os.getenv` head counts as its default) and `shutil.which` argument, through import aliases, whose basename no TOOLS entry declares. Commands built from variables or passed through a local helper (`out([...])`, `checked_run([...])`) are not seen. `03-consistency.py` reports each pair, and any tracked text file outside `OLD` and `95-Resources` that names an absolute home directory (`/home/<name>/`).
- **x86 gate.** `x86(compiler)` is true when the compiler (default `shlex.split($CXX or g++)`) prints an `x86_64` triple for `-dumpmachine` and exits 0, cached per compiler. `x86_only(flags, compiler)` returns the flags on such a target, else none. `cpu_runs(features, compiler)` also needs an x86-64 host whose `/proc/cpuinfo` lists every feature. Every runner that passed `-march=x86-64`, `-m[no-]avx*`, `-mfma`, `-mbmi2`, `-mpclmul` or `-mpopcnt` wraps them, and its ISA variants SKIP with "target is not x86-64" elsewhere; the judge's `-march=native` and the subgraph benchmark's `native` variant are x86-only too. `native(compiler)` is true when the target's architecture is the host's (`platform.machine()`, with `AMD64` read as `x86_64` and `arm64` as `aarch64`); `scaled(seconds, compiler)` multiplies a runner timeout by 10 otherwise (qemu-user), and the judge tester's 8 MB RSS bound becomes 64 MB there, since the emulator's own RSS counts. The Core headers already include `<immintrin.h>` only under `__AVX2__`.
- **arm64 oracles.** Six testers assumed x86-64 (user decision 2026-10-30: fix them here; each change is recorded in its owner's evidence): `02-debug` reads the `long double` format from `numeric_limits` and `09-rational` from the compiler's `__LDBL_*` macros (x87 64-bit or binary128 113-bit significand), its boundary cases included, `09-triangle` prints its excircle references to 40 digits, `22-convexhulltrick` uses `_Float128` for `__float128`, `01-random` builds with `-ffp-contract=off` through a per-tester flag of the `06-Miscellaneous` runner (aarch64 g++ fuses `randDouble`'s multiply-add), and `04-trie_tester.cpp` silences an aarch64 `-Warray-bounds` false positive inside `special()` only. x86-64 results are unchanged.
- **Licence files.** `03-consistency.py` pins `LICENSE` to the CC0-1.0 legal code by SHA-256; `.gitattributes` normalizes text to LF except `OLD/**` (`-text`, original bytes). `THIRD-PARTY.md` carries each derived file's licence text, including the Python-Markdown `fenced_code` notice that pymdown-extensions' `superfences` keeps. Code derived from `OLD/algorithms.cpp` is the owner's own work and stays under CC0 (user decision 2026-10-30).

### Provenance audit (2026-10-30)

A read-only agent searched every evidence document and ledger outside `OLD` and `95-Resources` for adapted, ported, transliterated, copied, derived, verbatim and licence words and catalog names, and all code, the notebook, site, workspace and judge sources for attribution notices, copyright, SPDX and source URLs, then fetched each source's licence. Third-party-derived code exists only in `98-Team Notebook/06-site.py`, `96-Local Testing/00-Tools/06-site_tester.py` and `96-Local Testing/04-Graphs/35-subgraph_enumeration_tester.cpp` ([THIRD-PARTY](../../../THIRD-PARTY.md)); no PyRival code is in the repository. The SplitMix64 finalizer is Vigna's public-domain dedication. Cases where a document names a source without a copy statement (`01-template` `indexed_map`, the `09-rational` hash, ACL lazysegtree, the ecnerwala fold, the sortedcontainers policy, `04-Graphs` 21 and 24, `03-Geometry/05-closestpair` from the owner's `OLD` notebook) are original by user decision 2026-10-30. `OLD`, which holds the co-authored team notebook, is excluded from CC0 with no licence granted (user decision 2026-10-30, after the review). Licence texts were fetched from each project's repository on 2026-10-30; `LICENSE` is the CC0-1.0 legal code, identical (SHA-256 `a2010f34…0499`) from creativecommons.org and SPDX.

### Feature-to-test map

Tester: [09-setup_tester.py](<../../../96-Local Testing/00-Tools/09-setup_tester.py>). Fake executables in a temporary `PATH`, synthetic git repositories in `/tmp`, roles through `GIT_CONFIG_COUNT`, patched `out`, `platform.machine` and `/proc/cpuinfo` reads; oracles re-derived in the tester (an independent os-release parser, install-line builder, `\usepackage` scanner and PATH scan).

| Contract | Tests |
|---|---|
| TOOLS | `test_tools_entries`, `test_tools_required_set`, `test_tools_optional_features`, `test_tools_floors` |
| check: versions and floors | `test_version_commands`, `test_version_parse`, `test_check_gcc16`, `test_check_gcc14`, `test_check_python`, `test_check_pypy`, `test_check_unknown_version`, `test_check_missing`, `test_check_alternatives`, `test_check_probe_problems` |
| probes | `test_probe_compiler_build_fails`, `test_probe_compiler_run_fails`, `test_probe_compiler_passes_and_capped`, `test_probe_real_compilers`, `test_probe_systemd`, `test_probe_gh`, `test_probe_lualatex`, `test_probe_fonts`, `test_probe_site`, `test_probe_qemu_missing_cross`, `test_probe_qemu_cross`, `test_probe_other_tools_none` |
| platform, role | `test_platform_aarch64`, `test_platform_x86_all`, `test_platform_x86_lacks_bmi2`, `test_platform_x86_lacks_several`, `test_platform_real`, `test_role_unset`, `test_role_overrides`, `test_role_invalid`, `test_role_without_git`, `test_role_read_in_root` |
| distro, install | `test_distro_explicit`, `test_distro_os_release`, `test_distro_unreadable`, `test_install_exact`, `test_install_empty`, `test_install_random` |
| lint | `test_commands_forms`, `test_commands_random`, `test_undeclared_synthetic`, `test_undeclared_not_a_repo`, `test_undeclared_real`, `test_lint_cli_real` |
| CLI | `test_cli_fake_path`, `test_cli_fake_required_pass`, `test_cli_main_labels`, `test_cli_bad_distro`, `test_cli_without_git`, `test_cli_random`, `test_cli_real`, `test_cli_real_ubuntu` |
| x86 gate | `test_isa_x86_triples`, `test_isa_cache`, `test_isa_default_cxx`, `test_isa_x86_only`, `test_isa_cpu_runs_fake`, `test_isa_cpu_runs_real`, `test_isa_native`, `test_isa_scaled`; runners: the quick suite on x86 (unchanged variants) and with a fake `aarch64` compiler (`04-montgomery`, `18-bitset`: no x86 flag reached the compiler, AVX2 variants SKIP) and under qemu-user with the arm64 cross compiler (Commands) |
| consistency checks | `03-consistency.py` on the repository (no errors) and with a scratch file holding a `/home/<name>/` path added intent-to-add (reported, then removed); the `LICENSE` pin caught a corrupted copy (two altered lines), restored from the fetched legal code |

### Commands and results (2026-10-30)

Run from `/tmp`; the tester caps itself.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/09-setup_tester.py' --mode quick --seed 1` | PASS, 58 tests, 2.0 s, peak 45 MB |
| `... --mode full --seed 1` | PASS, 64 tests, 24 s, peak 324 MB |
| `... --mode stress --seed 42 --rounds 20` | PASS, 64 tests, 25 s, peak 324 MB |
| `CXX=g++-14 ... --mode full --seed 1` | PASS, 64 tests, 23 s, peak 325 MB |
| `python3.11 ... --mode full --seed 1` (uv CPython 3.11.17) | PASS, 64 tests, 24 s, peak 324 MB |
| `python3 '96-Local Testing/01-run.py' --mode quick` | PASS, 171 suites, integration, consistency no errors |
| `ulimit -c 0; CXX=aarch64-linux-gnu-g++ QEMU_LD_PREFIX=/usr/aarch64-linux-gnu python3 '96-Local Testing/01-run.py' --mode quick` (Arch `aarch64-linux-gnu-gcc` 16.2, qemu-user binfmt) | PASS, 171 suites, arm64 integration (183 headers, AVX2 SKIP), consistency no errors; before the oracle fixes 6 suites failed (`02-debug`, `09-rational`, `09-triangle`, `22-convexhulltrick`, `01-random`, `04-trie`) and 7 timed out (fixed by `scaled`) |
| The six fixed testers, `--mode quick` on x86-64 and on aarch64 under qemu-user, and `--mode full` on g++ and g++-14 (x86-64); `09-rational` also `--mode stress --seed 7` | PASS (rational full peak 676 MB) |
| `09-rational` on aarch64 with `nearest`'s binary128 denormal exponent moved by +1 and by -1 (mutants) | both FAIL at the `toDouble` boundary cases; unmutated PASS |
| `python3.11 '96-Local Testing/01-run.py' --mode quick` | PASS for every suite except the tester then being written (its `test_cli_without_git` exposed the no-git crash, since fixed; the tester passes on 3.11 above) |
| `python3.11 -c compile(...)` over every tracked script | no syntax error; `plan.py show/check/doctor`, `gaps.py`, `setup_check.py --lint` identical to 3.14 |
| `CXX=<wrapper: prints aarch64-linux-gnu for -dumpmachine, else logs its arguments and runs g++> python3 '96-Local Testing/01-run.py' --mode quick --filter 01-Core --no-integration` | PASS, 12 suites; 78 compiler calls, none with an x86 flag; 10 `target is not x86-64` SKIP lines |
| `03-consistency.py` with a tracked scratch file naming a `/home/<name>/` path | `Absolute home path in zz-home-probe.txt`; no errors after removing it |
| `python3 '96-Local Testing/02-integration.py'` | PASS, 183 headers, scalar and AVX2 multi-TU, workspace |
| `python3 '.claude/skills/setup-check/setup_check.py'` | PASS; arm64 cross g++ and qemu-user OK; WARN Node.js (not installed) |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

### Limits

- The Ubuntu hints come from the `resolute` package indexes, not an Ubuntu install; Ubuntu's `g++-16` is the 16-20260322 snapshot and its GCC 16 behaviour is unverified. Arch ships no aarch64 libasan or libubsan, so the arm64 check is quick mode: full-mode sanitizer builds fail to link there. qemu-user writes guest core dumps into the working directory; run it with `ulimit -c 0`.
- The arm64 check runs aarch64 under qemu-user only; native arm64 hardware and other architectures are not run.
- The lint sees only literal command names; tools reached through variables (`zensical`, `mkdocs`, `oj-bundle` via `--bundler`, `g++-14` via `CXX`) are declared by hand.
- WSL2 and the macOS VM route are documented, not run.
- The home-path check needs a slash after the user name, so the brief's own `/home/Ausp3x` text passes; a bare `/home/alice` is not caught.
- `96-Local Testing/01-Core/03-reduction_candidates.jsonl` had its 2026-09-27 absolute paths rewritten repo-relative on 2026-10-30; the commands ran with absolute paths.

## History

- 2026-10-30: /finish, arm64 verified under qemu-user: x86-only `-march=native`, `native`/`scaled` (emulated timeouts, judge RSS bound), six tester oracles made format-portable (user decisions: fix them here, scale timeouts, land as verified), `LICENSE` restored and pinned; diff review, 7 findings fixed (x87-only rational boundaries, missing arm64 rows, `-ffp-contract=off` and the trie pragma narrowed, constant name, one `_arch` helper with host-name normalization, default-`CXX` test); x86-64 and arm64 quick suites, the changed testers' full runs on g++ and g++-14, and the tester in every mode passed.
- 2026-10-30: SUP18 implemented (setup_check.py and /setup-check, x86 target gate in the runners, lint and home-path checks, README, CONTRIBUTING, LICENSE, THIRD-PARTY, LF attributes, neutral site hint, home paths removed); provenance audit; tester written, 5 tester findings fixed (no-git crash, silent lint outside a repository, nonzero `-dumpmachine`, single-quoted os-release, aliased imports); quick, full, stress, g++-14 and 3.11 passed.
- 2026-10-30: review, 5 findings fixed (`/setup-check` failing on a nonzero exit, the Ubuntu cross-compiler package, `OLD` excluded from CC0 by user decision, unrecorded probe commands, a stale WARN list) and judgement calls applied (systemd probe flags, no install line for login or delegation, restated rules, `os.getenv`, PATH note); tester quick, full, stress, g++-14 and 3.11 passed.
- 2026-10-29: /finish diff review, 6 defects fixed (no-argument key on a claim branch, the Claimed-by-you switch in `/package`, claim commands on the claim branch itself, index wording, restated rules, whole-token `live` names) and `gh pr ready` only for a verified last batch, by number; quick, full and stress passed.
- 2026-10-29: /finish audit, 7 defects fixed (existing claim branch, `--repo` on the ready command, contributor log result, gh exec errors, landing question, `live` names, restated rules); 3 user decisions (claims for /prune and /restyle, draft until the last batch, only verified support lines count).
- 2026-10-29: tester updated for the review fixes; quick, full and stress passed.
- 2026-10-29: review, 10 findings: fixed the worktree claim flow, rebase and force-push, Claims unknown, the gh time bound, the shared `claim_branch`, `/prune` claims, the stated-once index and the evidence limits (user decisions: claim in the session's checkout without rebasing; pin the repository).
- 2026-10-29: SUP17 implemented (role, claims, gate, `show`/`ready`/`next`/`status`, `gaps.py` role line, rules); tester written; 2 tester findings fixed (non-JSON `gh` output, whitespace login); quick, full and stress passed.
