# Contributor roles and claims — evidence (P250, SUP17)

Scope: [plan.py](../plan.py) reads the clone's role and the open pull requests that claim packages, [gaps.py](../../../.claude/skills/finish/gaps.py) tells `/finish` where step 7 commits, and the rules live in [CLAUDE.md](../../../CLAUDE.md) (Git), [finish](../../../.claude/skills/finish/SKILL.md) step 7, [package](../../../.claude/skills/package/SKILL.md) step 1, [01-prompts.md](../../../01-prompts.md) (Workflow) and the [plan index](../00-index.md). Decisions: [10-decisions.md](../../10-decisions.md), 2026-10-29 Contribution.

## Contracts

- **Role.** `role(root)` returns `git config compprog.role` read in `root`: `contributor` when it is unset, otherwise `contributor` or `maintainer`. Any other value stops with a message naming it. This clone is set to `maintainer` (user decision 2026-10-29).
- **gh.** `gh(root, *args)` returns `(stdout, '')`, or `(None, reason)` when the reason is "gh not installed", the first stderr line of a non-zero exit, or "gh <cmd> timed out" after 10 s, or "gh <cmd> failed: <error>" when it cannot be executed. The claim lookup never fails a command: it reports `SKIP (<reason>)`, including `gh pr list printed no JSON`. After a SKIP the login does not call gh. A brief therefore waits at most two gh timeouts (20 s), and `gaps.py` with its brief at most four (40 s), both within the 60 s SessionStart hook, which runs only the brief.
- **Login.** `login(root, ask)` is the GitHub login from `gh api user` (only when `ask`), falling back to `git config user.name` when that output is empty. It is slugged to `[A-Za-z0-9-]` with runs joined by `-`, and becomes `user` if nothing is left. It provides the `<user>` part of a claim branch.
- **Claims.** `claims(root, packages)` runs `gh pr list --repo <repo> --state open --limit 1000 --json number,headRefName,isDraft,author`, where `<repo>` is `git config compprog.repo` or `Ausp3x/CompProg-Library` (`REPO`; user decision 2026-10-29), so a fork's clone reads the canonical pull requests. An open pull request, draft or ready, claims Pxxx when its head branch fullmatches `Pxxx-<batch>-<user>` and the batch (in any case) belongs to Pxxx. The lowest number wins per package. A ready pull request stays a claim until it is merged or closed (user decision 2026-10-29).
- **Gate.** `gate(root, packages)` blocks a package claimed by someone else (the author differs from the login, case-insensitively). For a contributor it also blocks every Core full package (model `fable`) as maintainer-only, including one the contributor claimed. It returns the claims line `Role: <role>; claims: none open | <k> open | SKIP (<reason>)`. The login is looked up only when there are claims or the role is contributor.
- **Commands.** `next`, `status` and `show next` take the first ready package the gate does not block. `show` prints the claims line second, then one of four lines. `STOP: <id> is <why>` when blocked. `Claimed by you: pull request #n on <branch>` when the user holds the claim, with `git fetch origin <branch> && git switch <branch>` when the current branch differs. For a contributor and any other package (verified ones included, for `/prune` and `/restyle`; user decision 2026-10-29), `Unclaimed:`, or `Claims unknown:` after a SKIP, with the claim commands to run in the checkout the session will work in: `git switch -c <branch>`, an empty claim commit, `git push -u origin <branch>`, then `gh pr create --draft --fill --repo <repo>`. The branch is `claim_branch` of the brief's batch. `ready` starts with the claims line and marks held packages, which never enter the disjoint set. Live and claimed packages, whoever claimed them, reserve their folders; a package that is only maintainer-only does not.
- **`/finish`.** `gaps.py` prints `role maintainer: step 7 lands on master`, or `role contributor: step 7 commits on branch <b> and stops before landing; repository <repo>; ` followed by `pull request #n; ` when the user holds a claim on the package, then `last open batch: mark the pull request ready if verified` (also without a package) or `more batches follow: the pull request stays a draft` (user decision 2026-10-29: the pull request stays a draft until the package is done, so a merge cannot release the claim early). `<b>` comes from `claim_branch`, which `show` uses too: the user's own open claim's branch; else the current branch when it is a claim branch of a batch of the package; else `<package>-<batch>-<login>`. Here the batch is the batch key, or else the package's first batch without a `verified` finish-log line. Logged lines decide, not inventory rows, since `/package` marks rows verified before `/finish` runs; for the same reason, on a claim branch with no argument the key is that batch, not the batch named in the branch. What step 7 then does is defined in [finish](../../../.claude/skills/finish/SKILL.md) step 7, and which finish-log lines count in the [plan index](../00-index.md).

## Feature-to-test map

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

## Commands and results (2026-10-29)

Run from `/tmp`; the tester caps itself (`_00_memory_cap.ensure()`), and the peak was 49 MB.

| Command | Result |
|---|---|
| `python3 '96-Local Testing/00-Tools/08-plan_tester.py' --mode quick --seed 1` | PASS, 31 tests, 19.1 s |
| `... --mode full --seed 1` | PASS, 38 tests, 24.3 s after the quick pre-flight |
| `... --mode stress --seed 42 --rounds 100` | PASS, 38 tests, 30.8 s |
| `python3 '00-Guidelines/13-Plan/plan.py' check`; `doctor` | ok; no inconsistencies |
| `python3 '96-Local Testing/03-consistency.py'` | no errors |

## Limits

- Claims are as fresh as `gh pr list`: two contributors who both run `show` before either opens a pull request can both claim, and the lower pull request number wins. Without `gh` the claims line is a SKIP and `show` prints Claims unknown, never Unclaimed. The fork case (`--repo` pinned) is covered only by the fake `gh` asserting the arguments; it has no run against GitHub.
- `claims` trusts `gh --json` to print a list of objects: valid JSON of another shape from a `gh` wrapper raises an error or reads as no claims.
- `ready` reserves folders only for packages it lists, so a live or claimed package whose prerequisites are not yet verified does not reserve its folders.
- `/finish`'s `allowed-tools` should pre-approve `Bash(git switch *) Bash(git fetch *)` (user decision 2026-10-29). The auto-mode classifier blocks agents from editing skill front matter, so the user adds it; until then the contributor step prompts.
- A malformed claim-shaped branch whose batch part is not a batch (`P250-ZZ99-c07`) lets a token of its user part name the target of a no-argument `/finish`; real claim branches are unaffected.
- `--limit 1000` reads at most 1000 open pull requests. If `gh api user` fails after `gh pr list` succeeded, the login falls back to `git config user.name`, and the user's own claim can then read as STOP.
- The contributor flow has no end-to-end run: no recorded run of the claim commands, the step 7 switch, commit and push, even against a local bare remote. A contributor's worktree is not removed by `worktree.py --remove`, since the maintainer's rebase leaves its tip outside `master`; delete it by hand after the merge.
- Agent choices kept as contracts: the lowest pull request number wins, authors match case-insensitively, the login slug defaults to `user`, the gh timeout is 10 s, and maintainer-only packages do not reserve folders.
- The merge side (`/merge-pr`) is SUP19, and the contributor setup and docs (CONTRIBUTING) are SUP18.

## History

- 2026-10-29: /finish diff review, 6 defects fixed (no-argument key on a claim branch, the Claimed-by-you switch in `/package`, claim commands on the claim branch itself, index wording, restated rules, whole-token `live` names) and `gh pr ready` only for a verified last batch, by number; quick, full and stress passed.
- 2026-10-29: /finish audit, 7 defects fixed (existing claim branch, `--repo` on the ready command, contributor log result, gh exec errors, landing question, `live` names, restated rules); 3 user decisions (claims for /prune and /restyle, draft until the last batch, only verified support lines count).
- 2026-10-29: tester updated for the review fixes; quick, full and stress passed.
- 2026-10-29: review, 10 findings: fixed the worktree claim flow, rebase and force-push, Claims unknown, the gh time bound, the shared `claim_branch`, `/prune` claims, the stated-once index and the evidence limits (user decisions: claim in the session's checkout without rebasing; pin the repository).
- 2026-10-29: SUP17 implemented (role, claims, gate, `show`/`ready`/`next`/`status`, `gaps.py` role line, rules); tester written; 2 tester findings fixed (non-JSON `gh` output, whitespace login); quick, full and stress passed.
