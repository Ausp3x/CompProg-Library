# Contributing

## Platforms

- **Linux x86-64** is the reference: every suite, ISA path and benchmark runs there.
- **Windows** works through Ubuntu 26.04 on WSL2. Turn systemd on (`[boot]` `systemd=true` in `/etc/wsl.conf`, then `wsl --shutdown`), since the test memory cap runs in systemd user scopes. Keep the clone on the Linux filesystem (ext4, for example `~/src`), not under `/mnt/c`.
- **macOS** works only through an arm64 Ubuntu 26.04 VM. GCC builds no sanitizer runtimes for Apple Silicon, and the tooling needs systemd, `bwrap` and `-static`. On arm64 the runners pass no x86 flags and report the AVX2 paths as SKIP.

## Setup

Clone, then run `python3 '.claude/skills/setup-check/setup_check.py'` (or `/setup-check` in Claude Code) until it prints PASS. It declares every tool the scripts run, with its floor and its Ubuntu and Arch packages, probes the compilers' sanitizers and static linking, the memory-capped systemd scope and `gh auth status`, and prints the install commands for what is missing. A WARN disables only the feature named in its row. Open a new login shell after installing, so `~/.local/bin` (the GCC 16 link, `pipx ensurepath`) is on `PATH`.

Each tool's floor and packages are in its `TOOLS` entry, including Ubuntu's pre-release `g++-16` snapshot and Arch's AUR `gcc14`. `08-Python` modules keep their own baseline ([`04-python.md`](00-Guidelines/04-python.md)).

## Roles and claims

Work is planned as packages ([`01-prompts.md`](01-prompts.md)). The claim and landing rules are in [CLAUDE.md](CLAUDE.md) (Git): a contributor claims a package with a draft pull request on a `Pxxx-<batch>-<user>` branch, using the commands `python3 '00-Guidelines/13-Plan/plan.py' show Pxxx` prints, and the maintainer lands it. Leave `git config compprog.role` unset (contributor).

- **Collaborators** with write access push claim branches to the canonical repository.
- **Forks**: push the claim branch to your fork; the printed `gh pr create --draft --repo …` opens the pull request against the canonical repository, where claims are read (`git config compprog.repo` overrides it only when the project's canonical home moves).

## Licence

Contributions are dedicated under [CC0-1.0](LICENSE). Code adapted from elsewhere follows [`09-sources.md`](00-Guidelines/09-sources.md) (Provenance) and goes into [THIRD-PARTY.md](THIRD-PARTY.md).
