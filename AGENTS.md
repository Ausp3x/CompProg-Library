# CompProg Library (entry for non-Claude agents)

Read `CLAUDE.md`: everything in it applies except the Claude session presets and model choices (`claude --agent`, `/model`). What Claude Code does automatically, do by hand:

- Rules: read `00-Guidelines/01-principles.md` always and, before touching a file, the guide `00-Guidelines/00-index.md` lists for it.
- Skills: for `/name args`, follow `.claude/skills/<name>/SKILL.md`, running each `!` command line yourself with `$ARGUMENTS` replaced by the arguments and `${CLAUDE_PROJECT_DIR}` by the repository root; `@researcher`, `@reviewer` and `@speedscout` run under `.claude/agents/<name>.md`, and `/omission-sweep` runs the workflow `.claude/skills/omission-sweep/scan.js`: run its prompts by hand. Use your tool's equivalent of Claude-only commands such as `/compact`.
- Hooks: obey the hook rules in `CLAUDE.md` yourself and run `python3 '96-Local Testing/03-consistency.py'` after editing a Markdown, header, module or plan file.
