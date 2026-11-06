# CompProg Library

A competitive-programming library: GNU C++20 headers for core numeric types, data structures, geometry, graphs, mathematics, utilities and strings, a PyPy-oriented Python subset, in-contest testing tools, a generated team notebook and a standalone contest template. A verified header has a documented contract, an evidence document and a regression suite with independent oracles. The [folder index](00-index.md) maps the layout, and each folder's `00-index.md` lists its headers, operations and status.

## Using it in a contest

Start from the standalone [`99-Workspace/template.cpp`](99-Workspace/template.cpp). Headers need GCC 14 or newer with `-std=gnu++20`. To paste library code, use [`09-Contest Testing/10-grab.py`](<09-Contest Testing/10-grab.py>) (it needs libclang; `/setup-check` prints the install command): `grab bellTable DSU` prints and copies only the declarations those names reach, `grab --into a.cpp` keeps one marked block of what `grab` gives for the library names `a.cpp` uses above the first code that uses them (`--min` cuts each struct to the members called), and `grab --find text` searches the names. The first run indexes the library for a few minutes, then reuses the cache. Grab finds the library from its own path, so it runs from any folder: `python3 '.claude/skills/setup-check/setup_check.py' --alias` writes the `grab` alias into `~/.bashrc` (or `~/.zshrc`), and in a new terminal `grab --into A.cpp` works in `99-Workspace` (save the file first). A source that includes headers by relative path is bundled into one submittable file by [`97-Online Testing`](<97-Online Testing/00-index.md>)'s expander, which uses grab too. A header's contract (domain, complexity, sentinels) is in its folder's `Docs/<NN-name>.md`, linked from the folder index. [`09-Contest Testing`](<09-Contest Testing/00-index.md>) holds the stress, checker and interactor tools.

## Working on it

Linux x86-64 is the reference platform. Check a machine with:

```bash
python3 '.claude/skills/setup-check/setup_check.py'      # tools, floors and install commands
python3 '96-Local Testing/01-run.py' --mode quick        # all suites, quick
```

Development runs in Claude Code sessions, one package at a time ([`01-prompts.md`](01-prompts.md), Workflow). [CONTRIBUTING.md](CONTRIBUTING.md) covers platforms, setup, roles and claims.

## Licence

Original work is dedicated to the public domain under [CC0-1.0](LICENSE). `95-Resources`, the `OLD` archive and the third-party-derived files listed in [THIRD-PARTY.md](THIRD-PARTY.md) keep their own terms.
