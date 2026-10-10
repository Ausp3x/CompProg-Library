# CompProg Library (entry for non-Claude agents)

Read `CLAUDE.md`: everything in it applies except the Claude session presets and model choices (`claude --agent`, `/model`). What Claude Code does automatically, do by hand:

- Rules: read `00-Guidelines/01-principles.md` always and, before touching a file, the guide `00-Guidelines/00-index.md` lists for it.
- Skills: for `/name args`, follow `.claude/skills/<name>/SKILL.md`, running each `!` command line yourself with `$ARGUMENTS` replaced by the arguments and `${CLAUDE_PROJECT_DIR}` by the repository root; a skill whose front matter names an `agent:` runs under that `.claude/agents/<name>.md`, as do `@researcher` and `@reviewer`. `/reaudit-review` is the workflow `.claude/workflows/reaudit-review.js`: run its prompts by hand. Use your tool's equivalent of Claude-only commands such as `/compact`.
- Hooks: obey the hook rules in `CLAUDE.md` yourself and run `python3 '96-Local Testing/03-consistency.py'` after editing an index, header, module or plan file.
